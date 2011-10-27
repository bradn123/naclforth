#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nacl/nacl_srpc.h>
#include <sys/nacl_syscalls.h>
#include <sys/errno.h>

#define VERBOSE 0

#define ENCODING_OF_ERROR 0x41131000  /* encoding of: error */
#define ENCODING_OF_I_COMMA 0x7fc00000  /* encoding of: i, */
#define ENCODING_OF_LOAD 0xa1ae0000  /* encoding of: load */
#define ENCODING_OF_SEMICOLON 0xf0000000  /* encoding of: ; */
#define ENCODING_OF_MACRO 0x8ac84c00  /* encoding of: macro */
#define ENCODING_OF_FORTH 0xb1896400  /* encoding of: forth */
#define ENCODING_OF_PERIOD 0xea000000  /* encoding of: . */
#define ENCODING_OF_SEND 0x82360000  /* encoding of: send */
#define ENCODING_OF_RECEIVE 0x14923e10  /* encoding of: receiv[e] */
#define ENCODING_OF_AVAILABLE 0x5c2af450  /* encoding of: availa[ble] */
#define ENCODING_OF_FREE 0xb0a20000  /* encoding of: free */

#define MAX_VOCABULARY_SIZE 10000
#define DATA_STACK_SIZE (1024*1024*4)
#define BOOT_AREA_CELLS (256*128)

#define DYNAMIC_AREA_SIZE 0x80000
#define DYNAMIC_AREA_GAP 0x10000
#define DYNAMIC_AREA_START (RODATA_START - DYNAMIC_AREA_SIZE - DYNAMIC_AREA_GAP)

#define NACL_BLOCK_SIZE 32
#define MAX_DYNAMIC_BLOCKS 64


typedef int32_t cell;

typedef enum {
  TAG_EXTENSION=0,
  TAG_YELLOW_WORD,
  TAG_YELLOW_BIGNUM,
  TAG_RED_WORD,
  TAG_GREEN_WORD,
  TAG_GREEN_BIGNUM,
  TAG_GREEN_NUMBER,
  TAG_CYAN_WORD,
  TAG_YELLOW_NUMBER,
  TAG_WHITE_LOWER,
  TAG_WHITE_WORD,
  TAG_WHITE_CAPS,
  TAG_MAGENTA_WORD,
  TAG_GRAY_WORD,
  TAG_BLUE_WORD,
  TAG_WHITE_NUMBER,
} COLOR_TAG;

typedef struct {
  cell name;
  cell address;
} DICTIONARY_ENTRY;

typedef struct _MESSAGE {
  cell *data;
  struct _MESSAGE *next;
} MESSAGE;

static pthread_mutex_t master_lock;
static int booted = 0;
static int boot_start_block;
static pthread_t main_thread;

static MESSAGE *inbound_head = 0;
static MESSAGE *inbound_tail = 0;
static MESSAGE *outbound_head = 0;
static MESSAGE *outbound_tail = 0;
static pthread_cond_t inbound_available;

static cell data_stack[DATA_STACK_SIZE];
static cell *data_stack_pointer = data_stack;

static DICTIONARY_ENTRY forth_words[MAX_VOCABULARY_SIZE];
static DICTIONARY_ENTRY macro_words[MAX_VOCABULARY_SIZE];
static DICTIONARY_ENTRY *forth_vocabulary;
static DICTIONARY_ENTRY *macro_vocabulary;
static DICTIONARY_ENTRY **current_vocabulary;

static unsigned char dynamic_buffer[MAX_DYNAMIC_BLOCKS * NACL_BLOCK_SIZE];
static size_t dynamic_buffer_target = DYNAMIC_AREA_START;
static size_t here = DYNAMIC_AREA_START;

static cell boot_area[BOOT_AREA_CELLS];


static void Execute(cell address) {
  asm __volatile__ (
    "pushl %%ebp;"
    "movl %%eax, %%ebp;"
    "movl (%%ebp), %%ebx;"
    "subl $4, %%ebp;"
    "naclcall %%ecx;"
    "addl $4, %%ebp;"
    "movl %%ebx, (%%ebp);"
    "movl %%ebp, %%eax;"
    "popl %%ebp;"
    : "=a"(data_stack_pointer)
    : "a"(data_stack_pointer), "c"(address)
    : "ebx", "edx", "edx", "esi"
  );
}

#if VERBOSE
static char *decode_word(cell word) {
  static const char *ENCODE1 = " rtoeani";
  static const char *ENCODE2 = "smcylgfw";
  static const char *ENCODE3 = "dvpbhxuq0123456789j-k.z/;'!+@*,?";

  static char name[100];
  unsigned int x = (unsigned int)word;
  int pos = 0;

  x = x & 0xFFFFFFF0;
  while (x) {
    if (!(x & 0x80000000)) {
      name[pos++] = ENCODE1[(x >> 28) & 7];
      x = (x << 4);
    } else if (!(x & 0x40000000)) {
      name[pos++] = ENCODE2[(x >> 27) & 7];
      x = (x << 5);
    } else {
      name[pos++] = ENCODE3[(x >> 25) & 0x1F];
      x = (x << 7);
    }
  } 

  name[pos] = 0;

  return name;
}

static void dump(size_t where, size_t len) {
  int i;

  for(i = 0; i < len; i++) {
    if (i % 16 == 0) {
      printf("\n%08x(%04x): ", where + i, i);
    }
    printf("%02x ", ((unsigned char*)where)[i]);
  }
  printf("\n");
  fflush(stdout);
}
#endif

static void FlushInstructions(void) {
  size_t remaining;
  size_t block_offset;
  size_t sz;
  int i;

  /* Done if nothing to flush. */
  if (here == dynamic_buffer_target) { return; }

  /* Region to add must be positive. */
  assert(here > dynamic_buffer_target);

#if 0
  /* Add an extra halt. */
  dynamic_buffer[here - dynamic_buffer_target] = 0xF4;
  ++here;
#endif

  /* Fill the rest with NOPs. */
  block_offset = here - here / NACL_BLOCK_SIZE * NACL_BLOCK_SIZE;
  remaining = NACL_BLOCK_SIZE - block_offset;
  memset(dynamic_buffer + (here - dynamic_buffer_target), 0x90, remaining);
  here += remaining;

  /* Decide size. */
  sz = here - dynamic_buffer_target;

#if VERBOSE
  fprintf(std"Write code to %x\n", (unsigned int)dynamic_buffer_target);
  dump((size_t)dynamic_buffer, sz);
#endif

  /* Validate and install the new block. */
  i = nacl_dyncode_create((void*)dynamic_buffer_target, dynamic_buffer, sz);
  assert(!i);
  assert(memcmp((void*)dynamic_buffer_target, dynamic_buffer, sz) == 0);

#if VERBOSE
  fprintf(stderr, "---------------------------------\n\n");
#endif

  /* Move on to the next one. */
  dynamic_buffer_target = here;
}

static void CompileInstruction(cell val1, cell val2, cell format) {
  size_t current_block_start = here / NACL_BLOCK_SIZE * NACL_BLOCK_SIZE;
  size_t block_offset = here - current_block_start;
  cell isize = format & 0xF;
  cell ikind = (format & 0xF0) >> 4;
  cell itotal;
  int i;

#if VERBOSE
  if (val1 != 0x90 || format != 0x1) {
    fprintf(stderr, "CompileInstr(%x,%x,%x) ",
        (unsigned int)val1, (unsigned int)val2, (unsigned int)format);
  }
#endif

  /* kinds 1 and 2 have an included 4 byte pointer */
  if (ikind) {
    itotal = isize + sizeof(val2);
  } else {
    itotal = isize;
  }

  /* skip to the next block if this one is full */
  if (NACL_BLOCK_SIZE - block_offset < itotal) {
    /* Fill the rest of the block with NOPs. */
    memset(dynamic_buffer + (here - dynamic_buffer_target), 0x90,
           NACL_BLOCK_SIZE - block_offset);
    here = current_block_start + NACL_BLOCK_SIZE;
  }

  /* add the first part */
  for (i = 0; i < isize; ++i) {
    dynamic_buffer[here - dynamic_buffer_target] =
        (val1 >> ((isize - 1 - i) * 8)) & 0xFF;
    ++here;
  }

  /* add pointer part if needed */
  if (ikind) {
    /* Kind 1 is absolute, 2 is relative so adjust it. */
    /* It's relative to the byte after the pointer. */
    if (ikind == 2) {
      val2 -= (here + 4);
    }
    memcpy(dynamic_buffer + (here - dynamic_buffer_target),
           &val2, sizeof(val2));
    here += sizeof(val2);
  }
}

static void CompileCall(cell address) {
  assert(address % NACL_BLOCK_SIZE == 0);
  /* Make sure were aligned to the end of block. */
  while (here % NACL_BLOCK_SIZE != NACL_BLOCK_SIZE - 5) {
    CompileInstruction(0x90, 0, 0x01);  /* nop */
  }
  /* Add actuall call instruction. */
  CompileInstruction(0xE8, address, 0x21);  /* call address */
}

static void CompileLiteral(cell value) {
  CompileInstruction(0x83C504, 0, 0x03);  /* add ebp,4 */
  CompileInstruction(0x895D00, 0, 0x03);  /* mov [ebp], ebx */
  CompileInstruction(0xBB, value, 0x11);  /* mov ebx, value */
}

static void CompileReturn(void) {
  CompileInstruction(0x58, 0, 0x01);  /* pop eax */
  /* Hack to work limitations to args. Really just nacljmp. */
  /* 83E0E0 and eax, 0xffffffe0 */
  /* FFE0   jmp eax */
  CompileInstruction(0x83, 0xe0ffe0e0, 0x11);
}

static void CompileCallC(void (*func)(), int args, int retval) {
  int i;

  CompileInstruction(0x50, 0, 0x01);  /* push eax */
  CompileInstruction(0x51, 0, 0x01);  /* push ecx */
  CompileInstruction(0x52, 0, 0x01);  /* push edx */
  CompileInstruction(0x56, 0, 0x01);  /* push esi */
  CompileInstruction(0x57, 0, 0x01);  /* push edi */

  if (!args) {
    CompileInstruction(0x83C504, 0, 0x03);  /* add ebp,4 */
    CompileInstruction(0x895D00, 0, 0x03);  /* mov [ebp], ebx */
  }

  for (i = 0; i < args; i++) {
    CompileInstruction(0x53, 0, 0x01);  /* push ebx */
    if (i != args-1) {
      CompileInstruction(0x8b5d00, 0, 0x3);  /* mov ebx, [ebp] */
      CompileInstruction(0x83ed04, 0, 0x3);  /* sub ebp, 4 */
    }
  }

  CompileCall((cell)func);

  if (args) {
    CompileInstruction(0x83c400 + args * 4, 0, 0x3);  /* add esp, args*4 */
  }

  if (retval) {
    CompileInstruction(0x89c3, 0, 0x2);  /* mov ebx, eax */
  } else {
    CompileInstruction(0x8b5d00, 0, 0x3);  /* mov ebx, [ebp] */
    CompileInstruction(0x83ed04, 0, 0x3);  /* sub ebp, 4 */
  }

  CompileInstruction(0x5F, 0, 0x01);  /* pop edi */
  CompileInstruction(0x5E, 0, 0x01);  /* pop esi */
  CompileInstruction(0x5A, 0, 0x01);  /* pop edx */
  CompileInstruction(0x59, 0, 0x01);  /* pop ecx */
  CompileInstruction(0x58, 0, 0x01);  /* pop eax */
}

static void CompileCompileLiteral(void) {
  CompileCallC(CompileLiteral, 1, 0);
}

static void DefineWord(DICTIONARY_ENTRY **vocabulary, cell name) {
#if VERBOSE
  fprintf(stderr, "Defining word: %s\n", decode_word(name));
#endif
  FlushInstructions();
  ++(*vocabulary);
  (*vocabulary)->name = name;
  (*vocabulary)->address = here;
}

static cell FindWord(DICTIONARY_ENTRY *vocabulary, cell name) {
  while (vocabulary->name) {
    if (name == vocabulary->name) {
      return vocabulary->address;
    }
    --vocabulary;
  }
  return 0;
}

static void Compile(cell *data, cell length) {
  cell tag, word, number;
  cell last_tag, address;

  last_tag = TAG_EXTENSION;
  while (length > 0) {
    if (!*data) { break; }
    tag = *data & 0xF;
    word = *data & ~0xF;
#if VERBOSE
    fprintf(stderr, "{%s} ", decode_word(word));
#endif
    number = ((int32_t)(word & ~0x1F) >> 5);
    switch (tag) {
      case TAG_RED_WORD:
        DefineWord(current_vocabulary, word);
        break;

      case TAG_YELLOW_WORD:
        FlushInstructions();
        address = FindWord(forth_vocabulary, word);
        if (address) {
          Execute(address);
        } else {
          address = FindWord(forth_vocabulary, ENCODING_OF_ERROR);
          Execute(address);
        }
        break;

      case TAG_YELLOW_NUMBER:
        ++data_stack_pointer;
        *data_stack_pointer = number;
        break;

      case TAG_GREEN_WORD:
        /* If the last word was yellow, compile one integer literal. */
        if (last_tag == TAG_YELLOW_WORD ||
            last_tag == TAG_YELLOW_NUMBER ||
            last_tag == TAG_YELLOW_BIGNUM) {
          CompileLiteral(*data_stack_pointer);
          --data_stack_pointer;
        }
        address = FindWord(macro_vocabulary, word);
        if (address) {
          Execute(address);
          break;
        }
        address = FindWord(forth_vocabulary, word);
        if (address) {
          CompileCall(address);
          break;
        }
        address = FindWord(forth_vocabulary, ENCODING_OF_ERROR);
        Execute(address);
        break;

      case TAG_GREEN_NUMBER:
        /* If the last word was yellow, compile one integer literal. */
        if (last_tag == TAG_YELLOW_WORD ||
            last_tag == TAG_YELLOW_NUMBER ||
            last_tag == TAG_YELLOW_BIGNUM) {
          CompileLiteral(*data_stack_pointer);
          --data_stack_pointer;
        }
        CompileLiteral(number);
        break;

      case TAG_CYAN_WORD:
        address = FindWord(macro_vocabulary, word);
        if (!address) {
          address = FindWord(forth_vocabulary, word);
        }
        if (address) {
          CompileCall(address);
        } else {
          address = FindWord(forth_vocabulary, ENCODING_OF_ERROR);
          Execute(address);
        }
        break;

      case TAG_MAGENTA_WORD:
        ++data;
        DefineWord(&forth_vocabulary, word);
        CompileLiteral((cell)data);
        CompileReturn();
        DefineWord(&macro_vocabulary, word);
        CompileLiteral((cell)data);
        CompileCompileLiteral();
        CompileReturn();
        --length;
        break;

      default:
        break;
    }
    ++data;
    --length;
    if (tag != TAG_EXTENSION) { last_tag = tag; }
  }

  FlushInstructions();
}

static void LoadBlock(cell index) {
  Compile(&boot_area[index * 256], 256);
}

static void MacroVocabulary(void) {
  current_vocabulary = &macro_vocabulary;
}

static void ForthVocabulary(void) {
  current_vocabulary = &forth_vocabulary;
}

static void ErrorHandler(void) {
  fprintf(stderr, "\nERROR\n");
}

static void PrintCell(cell value) {
  printf("%d ", (int)value);
  fflush(stdout);
}

static void SendMessage(cell *address) {
  MESSAGE *msg;

  /* Create message. */
  msg = (MESSAGE*)calloc(1, sizeof(MESSAGE));
  assert(msg);
  msg->data = malloc(sizeof(cell) * (address[0] + 1));
  assert(msg->data);
  memcpy(msg->data, address, sizeof(cell) * (address[0] + 1));

  /* Add message to queue. */
  pthread_mutex_lock(&master_lock);
  if (outbound_tail) {
    outbound_tail->next = msg;
    outbound_tail = msg;
  } else {
    outbound_head = msg;
    outbound_tail = msg;
  }
  pthread_mutex_unlock(&master_lock);
}

static cell *ReceiveMessage(void) {
  MESSAGE *msg;
  cell *data;

  pthread_mutex_lock(&master_lock);
  while (!inbound_head) {
    pthread_cond_wait(&inbound_available, &master_lock);
  }
  msg = inbound_head;
  inbound_head = msg->next;
  if (!inbound_head) { inbound_tail = 0; }
  pthread_mutex_unlock(&master_lock);

  data = msg->data;
  free(msg);
  return data;
}

static cell AvailableMessage(void) {
  cell ret;

  pthread_mutex_lock(&master_lock);
  ret = inbound_head != 0;
  pthread_mutex_unlock(&master_lock);

  return ret;
}

static void Free(void *x) {
  free(x);
}

static void InitializeDictionary() {
  /* Blank out the dictionary. */
  current_vocabulary = &forth_vocabulary;
  forth_vocabulary = forth_words;
  macro_vocabulary = macro_words;
  memset(forth_words, 0, sizeof(forth_words));
  memset(macro_words, 0, sizeof(macro_words));

  /* load */
  DefineWord(&forth_vocabulary, ENCODING_OF_LOAD);
  CompileCallC(LoadBlock, 1, 0);
  CompileReturn();
  FlushInstructions();

  /* i, */
  DefineWord(&forth_vocabulary, ENCODING_OF_I_COMMA);
  CompileCallC(CompileInstruction, 3, 0);
  CompileReturn();
  FlushInstructions();

  /* macro */
  DefineWord(&forth_vocabulary, ENCODING_OF_MACRO);
  CompileCallC(MacroVocabulary, 0, 0);
  CompileReturn();
  FlushInstructions();

  /* forth */
  DefineWord(&forth_vocabulary, ENCODING_OF_FORTH);
  CompileCallC(ForthVocabulary, 0, 0);
  CompileReturn();
  FlushInstructions();

  /* . */
  DefineWord(&forth_vocabulary, ENCODING_OF_PERIOD);
  CompileCallC(PrintCell, 1, 0);
  CompileReturn();
  FlushInstructions();

  /* error */
  DefineWord(&forth_vocabulary, ENCODING_OF_ERROR);
  CompileCallC(ErrorHandler, 0, 0);
  CompileReturn();
  FlushInstructions();

  /* send */
  DefineWord(&forth_vocabulary, ENCODING_OF_SEND);
  CompileCallC(SendMessage, 1, 0);
  CompileReturn();
  FlushInstructions();

  /* receive */
  DefineWord(&forth_vocabulary, ENCODING_OF_RECEIVE);
  CompileCallC((void(*))ReceiveMessage, 0, 1);
  CompileReturn();
  FlushInstructions();

  /* available */
  DefineWord(&forth_vocabulary, ENCODING_OF_AVAILABLE);
  CompileCallC((void(*))AvailableMessage, 0, 1);
  CompileReturn();
  FlushInstructions();

  /* free */
  DefineWord(&forth_vocabulary, ENCODING_OF_FREE);
  CompileCallC(Free, 1, 0);
  CompileReturn();
  FlushInstructions();

  /* ; */
  DefineWord(&macro_vocabulary, ENCODING_OF_SEMICOLON);
  /* See return above for explanation. */
  CompileLiteral(0x58);
  CompileLiteral(0);
  CompileLiteral(0x01);
  CompileCall(FindWord(forth_vocabulary, ENCODING_OF_I_COMMA));
  CompileLiteral(0x83);
  CompileLiteral(0xe0ffe0e0);
  CompileLiteral(0x11);
  CompileCall(FindWord(forth_vocabulary, ENCODING_OF_I_COMMA));
  CompileReturn();
  FlushInstructions();

#if VERBOSE
  fprintf(stderr, "Dictionary initialized\n");
#endif
}

static void Setup(void) {
  pthread_mutex_init(&master_lock, NULL);
  pthread_cond_init(&inbound_available, NULL);
}

static void *MainThread(void *args) {
  /* Load requested block. */
  LoadBlock(boot_start_block);

  return 0;
}

static void Boot(cell *data, int len, int start) {
  /* Boot only once. */
  pthread_mutex_lock(&master_lock);
  if (booted) {
    pthread_mutex_unlock(&master_lock);
    return;
  }
  booted = 1;
  boot_start_block = start;
  pthread_mutex_unlock(&master_lock);

  /* Setup the dictionary. */
  InitializeDictionary();

  /* Copy over boot area. */
  memcpy(boot_area, data, len * sizeof(cell));

  /* Start main thread. */
  pthread_create(&main_thread, NULL, MainThread, 0);
}

static void BootMethod(NaClSrpcRpc *rpc,
                       NaClSrpcArg **in_args,
                       NaClSrpcArg **out_args,
                       NaClSrpcClosure *done) {
  Boot(in_args[0]->arrays.iarr, in_args[0]->u.count, in_args[1]->u.ival);
  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

static void SendMethod(NaClSrpcRpc *rpc,
                       NaClSrpcArg **in_args,
                       NaClSrpcArg **out_args,
                       NaClSrpcClosure *done) {
  MESSAGE *msg;

  /* Create a new message. */
  msg = calloc(1, sizeof(MESSAGE));
  assert(msg);
  msg->data = malloc((in_args[0]->u.count + 1) * sizeof(cell));
  assert(msg->data);
  msg->data[0] = in_args[0]->u.count;
  memcpy(msg->data + 1, in_args[0]->arrays.iarr,
         sizeof(cell) * in_args[0]->u.count); 
  
  pthread_mutex_lock(&master_lock);
  if (!inbound_tail) {
    inbound_head = msg;
    inbound_tail = msg;
  } else {
    inbound_tail->next = msg;
    inbound_tail = msg;
  }
  pthread_cond_broadcast(&inbound_available);
  pthread_mutex_unlock(&master_lock);

  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

static void ReceiveMethod(NaClSrpcRpc *rpc,
                          NaClSrpcArg **in_args,
                          NaClSrpcArg **out_args,
                          NaClSrpcClosure *done) {
  MESSAGE *msg = 0;
  int len = 0;

  pthread_mutex_lock(&master_lock);
  if (outbound_head) {
    msg = outbound_head;
    outbound_head = msg->next;
    if (!outbound_head) { outbound_tail = 0; }
    len = msg->data[0];
  }
  pthread_mutex_unlock(&master_lock);

  if (out_args[0]->u.count < len) {
    len = 0;
  }
  if (msg) {
    memcpy(out_args[0]->arrays.iarr, msg->data + 1, len * sizeof(cell));
  }
  out_args[0]->u.count = len;

  if (msg) {
    free(msg->data);
    free(msg);
  }

  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

static const struct NaClSrpcHandlerDesc srpc_methods[] = {
  { "boot:Ii:", BootMethod },
  { "send:I:", SendMethod },
  { "receive::I", ReceiveMethod },
  { NULL, NULL },
};

int main() {
  Setup();
  if (!NaClSrpcModuleInit()) {
    return 1;
  }
  if (!NaClSrpcAcceptClientConnection(srpc_methods)) {
    return 1;
  }
  NaClSrpcModuleFini();
  return 0;
}
