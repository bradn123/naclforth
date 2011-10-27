#include <stdio.h>
#include <string.h>
#include <nacl/nacl_srpc.h>


#define ENCODING_OF_ERROR 0x41131000  /* encoding of: error */
#define ENCODING_OF_I_COMMA 0x7fc00000  /* encoding of: i, */
#define ENCODING_OF_LOAD 0xa1ae0000  /* encoding of: load */
#define ENCODING_OF_SEMICOLON 0xf0000000  /* encoding of: ; */

#define MAX_VOCABULARY_SIZE 10000
#define DATA_STACK_SIZE (1024*1024*4)
#define BOOT_AREA_CELLS (256*128)

typedef int32_t cell;

typedef enum {
  TAG_EXTENSION,
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

static cell data_stack[DATA_STACK_SIZE];
static cell *data_stack_pointer = data_stack;
static cell here;

static DICTIONARY_ENTRY forth_words[MAX_VOCABULARY_SIZE];
static DICTIONARY_ENTRY macro_words[MAX_VOCABULARY_SIZE];
static DICTIONARY_ENTRY *forth_vocabulary = forth_words;
static DICTIONARY_ENTRY *macro_vocabulary = macro_words;
static DICTIONARY_ENTRY **current_vocabulary = &forth_vocabulary;


static cell boot_area[BOOT_AREA_CELLS];


static void Execute(cell address) {
  asm __volatile__ (
    "pushl %%ebp;"
    "movl %%eax, %%ebp;"
    "movl (%%ebp), %%ebx;"
    "subl $4, %%ebp;"
    "nacljmp *%%ecx;"
    "popl %%ebp;"
    : "=a"(data_stack_pointer)
    : "a"(data_stack_pointer), "c"(address)
    : "ebx", "edx", "edx", "esi"
  );
}

static void CompileInstruction(cell val1, cell val2, cell format) {
}

static void CompileCall(cell address) {
  CompileInstruction(0xE8, address, 0x21);  /* call address */
}

static void CompileLiteral(cell value) {
  CompileInstruction(0x83C504, 0, 0x03);  /* add ebp,4 */
  CompileInstruction(0x895D00, 0, 0x03);  /* mov [ebp], ebx */
  CompileInstruction(0xB8, value, 0x11);  /* mov EBX, value */
}

static void CompileReturn(void) {
  CompileInstruction(0xC3, 0, 0x01);  /* ret */
}

static void CompileCallC(void (*func)(), int args, int retval) {
  int i;

  CompileInstruction(0x50, 0, 0x01);  /* push eax */
  CompileInstruction(0x51, 0, 0x01);  /* push ecx */
  CompileInstruction(0x52, 0, 0x01);  /* push edx */
  CompileInstruction(0x56, 0, 0x01);  /* push esi */
  CompileInstruction(0x57, 0, 0x01);  /* push edi */

  for (i = 0; i < args; i++) {
    CompileInstruction(0x53, 0, 0x01);  /* push ebx */
    if (i != args-1) {
      CompileInstruction(0x8b5d00, 0, 0x3);  /* mov ebx, [ebp] */
      CompileInstruction(0x83ed04, 0, 0x3);  /* sub ebp, 4 */
    }
  }

  CompileCall((cell)func);

  CompileInstruction(0x83c500 + args * 4, 0, 0x3);  /* add ebp, args*4 */

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

static void DefineWord(DICTIONARY_ENTRY **vocabulary,
                       cell name, cell address) {
  ++(*vocabulary);
  (*vocabulary)->name = name;
  (*vocabulary)->address = address;
}

static cell FindWord(DICTIONARY_ENTRY *vocabulary, cell name) {
  while (vocabulary->name) {
    if (name == vocabulary->name) { return vocabulary->address; }
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
    number = ((int32_t)(word & ~0x1F) >> 5);
    switch (tag) {
      case TAG_RED_WORD:
        DefineWord(current_vocabulary, word, here);
        break;

      case TAG_YELLOW_WORD:
        address = FindWord(*current_vocabulary, word);
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
        DefineWord(&forth_vocabulary, word, here);
        CompileLiteral((cell)data);
        CompileReturn();
        DefineWord(&macro_vocabulary, word, here);
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
}

static void LoadBlock(cell index) {
  Compile(&boot_area[index * 256], 256);
}

static void InitializeDictionary() {
  /* load */
  DefineWord(&forth_vocabulary, ENCODING_OF_LOAD, here);
  CompileCallC(LoadBlock, 1, 0);
  CompileReturn();

  /* i, */
  DefineWord(&forth_vocabulary, ENCODING_OF_I_COMMA, here);
  CompileCallC(CompileInstruction, 3, 0);
  CompileReturn();

  /* ; */
  DefineWord(&macro_vocabulary, ENCODING_OF_SEMICOLON, here);
  CompileLiteral(0xC3);
  CompileLiteral(0);
  CompileLiteral(0x1);
  CompileCallC(CompileInstruction, 3, 0);
  CompileReturn();
}

static int Boot(cell *data, int len, int start) {
  memcpy(boot_area, data, len * sizeof(cell));
  InitializeDictionary();
//  LoadBlock(start);
  return len;
}

static void BootMethod(NaClSrpcRpc *rpc,
                       NaClSrpcArg **in_args,
                       NaClSrpcArg **out_args,
                       NaClSrpcClosure *done) {
  out_args[0]->u.ival = Boot(in_args[0]->arrays.iarr, in_args[0]->u.count,
                             in_args[1]->u.ival);
  rpc->result = NACL_SRPC_RESULT_OK;
  done->Run(done);
}

static const struct NaClSrpcHandlerDesc srpc_methods[] = {
  { "boot:I:i", BootMethod },
  { NULL, NULL },
};

int main() {
  if (!NaClSrpcModuleInit()) {
    return 1;
  }
  if (!NaClSrpcAcceptClientConnection(srpc_methods)) {
    return 1;
  }
  NaClSrpcModuleFini();
  return 0;
}
