#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#ifdef __native_client__
#include <sys/nacl_syscalls.h>
#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_module.h>
#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_instance.h>
#include <ppapi/c/ppb_messaging.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppp.h>
#include <ppapi/c/ppp_instance.h>
#include <ppapi/c/ppp_messaging.h>

static struct PPB_Messaging* ppb_messaging_interface = NULL;
static struct PPB_Var* ppb_var_interface = NULL;
static PP_Module pp_module = 0;
static PP_Instance pp_instance = 0;

#endif


#define STACK_SIZE (1024 * 1024)
#define RSTACK_SIZE (1024 * 1024)
#define HEAP_SIZE (10 * 1024 * 1024)
#define INPUT_SIZE 1024


typedef long cell_t;
typedef unsigned long ucell_t;

typedef struct _DICTIONARY {
  void* code;
  char* name;
  cell_t flags;
  struct _DICTIONARY **does;
  struct _DICTIONARY* next;
} DICTIONARY;


// Stacks / heap
static cell_t* stack_base;
static cell_t* rstack_base;
static cell_t* heap_base;
static cell_t* sp_global;
static cell_t* rp_global;
static cell_t* here = 0;

// State
static DICTIONARY* dictionary_head = 0;
static cell_t compile_mode = 0;
static cell_t number_base = 10;

// Input source
#ifdef __native_client__
static char *input_buffer = 0;
#endif
static char *source = 0;
static cell_t source_length = 0;
static cell_t source_id = 0;
static cell_t source_in = 0;


// Dictionary and flow macros.
#define FLAG_IMMEDIATE 1
#define FLAG_LINKED 2
#define FLAG_SMUDGE 4
#define NEXT { nextw = *ip++; goto *nextw->code; }
#define COMMA(val) { *here++ = (cell_t)(val); }
#define WORD(label)        {&& _##label, #label, 0, 0, 0},
#define IWORD(label)       {&& _##label, #label, FLAG_IMMEDIATE, 0, 0},
#define SWORD(name,label)  {&& _##label, name, 0, 0, 0},
#define SIWORD(name,label) {&& _##label, name, FLAG_IMMEDIATE, 0, 0},
#define END_OF_DICTIONARY  {0, 0, 0, 0, 0},


#ifdef __native_client__

static char *inbound_message = 0;
static cell_t inbound_message_length = 0;

static void PostMessage(const char *data, cell_t len) {
  struct PP_Var msg = ppb_var_interface->VarFromUtf8(pp_module, (const char*)data, len);
  ppb_messaging_interface->PostMessage(pp_instance, msg);
  ppb_var_interface->Release(msg);
}

static void Print(const char *data, cell_t len, int leading_spaces) {
  char *tmp;

  tmp = malloc(1 + leading_spaces + len);
  assert(tmp);
  tmp[0] = 'o';
  memset(tmp + 1, ' ', leading_spaces);
  memcpy(tmp + 1 + leading_spaces, data, len);
  PostMessage(tmp, 1 + leading_spaces + len);
  free(tmp);
}

static void ReadLine(void) {
  PostMessage("r", 1);
}

#else

static void Print(const char *data, cell_t len, int leading_spaces) {
  while (leading_spaces > 0) {
    fputc(' ', stdout);
    --leading_spaces;
  }
  fwrite(data, 1, len, stdout);
  fflush(stdout);
}

static void ReadLine(void) {
  static char input_buffer[1024];

  fgets((char*)input_buffer, sizeof(input_buffer), stdin);
  source = input_buffer;
  source_length = strlen((char*)input_buffer);
  if (source_length > 0 && source[source_length - 1] == '\n') {
    --source_length;
  }
}

#endif


static void PrintCstr(const char *str) {
  Print(str, strlen(str), 0);
}

static void PrintCstrLeading(const char *str, int leading_spaces) {
  Print(str, strlen(str), leading_spaces);
}


static void Find(const char* name, cell_t name_len,
                 DICTIONARY** xt, cell_t* ret) {
  DICTIONARY *pos = dictionary_head;

  while(pos && pos->name) {
    if (!(pos->flags & FLAG_SMUDGE) &&
        name_len == strlen(pos->name) &&
        memcmp(name, pos->name, name_len) == 0) {
      if (pos->flags & FLAG_IMMEDIATE) {
        *ret = 1;
      } else {
        *ret = -1;
      }
      *xt = pos;
      return;
    }
    // Follow links differently based on flags.
    if (pos->flags & FLAG_LINKED) {
      pos = pos->next;
    } else {
      ++pos;
    }
  }

  *ret = 0;
}

static void Ok(void) {
  PrintCstr("  ok\n");
}

static cell_t Parse(cell_t sep) {
  cell_t ret = source_in;

  while (ret < source_length && source[ret] != sep) {
    ++ret;
  }
  return ret;
}

static char* Word(void) {
  char* ret = (char*)here;
  // Skip whitespace.
  while(source_in < source_length && isspace((int)source[source_in])) {
    ++source_in;
  }
  while(source_in < source_length && !isspace((int)source[source_in])) {
    *(char*)here = source[source_in];
    here = (cell_t*)(1 + (char*)here);
    ++source_in;
  }
  ++source_in;
  // Add null.
  *(char*)here = 0;
  here = (cell_t*)(1 + (char*)here);
  return ret;
}

static void Align(void) {
  while (((cell_t)here) % sizeof(cell_t) != 0) {
    here = (cell_t*)(1 + (char*)here);
  }
}

static int DigitValue(int ch) {
  ch = tolower(ch);

  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
  return -1;
}

static int ToNumber(const char *str, cell_t len, cell_t *dst) {
  int negative = 0;
  int digit;

  if (len <= 0) { return 0; }
  *dst = 0;
  if (str[0] == '-') { negative = 1; ++str; --len; }
  while (len > 0) {
    digit = DigitValue(*str);
    if (digit < 0 || digit >= number_base) { return 0; }
    *dst *= number_base;
    *dst += digit;
    ++str;
    --len;
  }
  if (negative) { *dst = -*dst; }
  return 1;
}

static void PrintNumber(cell_t value) {
  static char digit[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char buf[20];
  int len = 0;
  static char out[20];
  int out_len = 0;
  
  if (value < 0) { value = -value; out[out_len++] = '-'; }
  do {
    buf[len++] = digit[value % number_base];
    value /= number_base;
  } while (value);
  while (len) {
    out[out_len++] = buf[--len];
  }
  Print(out, out_len, 1);
}

static void PrintWords(void) {
  DICTIONARY *pos;

  pos = dictionary_head;
  while (pos && pos->name) {
    if (!(pos->flags & FLAG_SMUDGE)) {
      PrintCstrLeading(pos->name, 1);
    }
    // Follow links differently based on flags.
    if (pos->flags & FLAG_LINKED) {
      pos = pos->next;
    } else {
      ++pos;
    }
  }
}

static void Run(void) {
  register cell_t* sp = sp_global;
  register cell_t* rp = rp_global;
  register DICTIONARY** ip = *(DICTIONARY***)rp--;
  register DICTIONARY* nextw;
  char ch;
  cell_t len;
  cell_t *x;

  static DICTIONARY base_dictionary[] = {
    // Put some at predictable locations.
    WORD(_lit)  // 0
#define WORD_LIT (&base_dictionary[0])
    WORD(_jump)  // 1
#define WORD_JUMP (&base_dictionary[1])
    WORD(_zbranch)  // 2
#define WORD_ZBRANCH (&base_dictionary[2])
    WORD(exit)  // 3
#define WORD_EXIT (&base_dictionary[3])
    WORD(_imp_do)  // 4
#define WORD_IMP_DO (&base_dictionary[4])
    WORD(_imp_qdo)  // 5
#define WORD_IMP_QDO (&base_dictionary[5])
    WORD(_imp_loop)  // 6
#define WORD_IMP_LOOP (&base_dictionary[6])
    WORD(_imp_plus_loop)  // 7
#define WORD_IMP_PLUS_LOOP (&base_dictionary[7])
    WORD(_imp_does)  // 8
#define WORD_IMP_DOES (&base_dictionary[8])
    WORD(quit) // 9
#define WORD_QUIT (&base_dictionary[9])
    WORD(type) // 10
#define WORD_TYPE (&base_dictionary[10])

    SWORD(">r", push) SWORD("r>", pop)
    SWORD("+", add) SWORD("-", subtract) SWORD("*", multiply) SWORD("/", divide)
    SWORD("=", equal) SWORD("<>", nequal)
    SWORD("<", less) SWORD("<=", lequal)
    SWORD(">", greater) SWORD(">=", gequal)
    WORD(min) WORD(max)
    SWORD("@", load) SWORD("!", store) SWORD("c@", cload) SWORD("c!", cstore)
    WORD(dup) WORD(drop) WORD(swap) WORD(over) 
    SWORD(",", comma) WORD(here) WORD(allot) WORD(align)
    WORD(allocate) WORD(free) WORD(resize)
    SWORD(".", dot) WORD(emit) WORD(cr) WORD(page)
    SWORD(":", colon) SIWORD(";", semicolon)
    WORD(immediate) SIWORD("[", lbracket) SIWORD("]", rbracket)
    WORD(create) SIWORD("does>", does) WORD(variable) WORD(constant)
    IWORD(if) IWORD(else) IWORD(then)
    IWORD(begin) IWORD(again) IWORD(until)
    IWORD(while) IWORD(repeat)
    IWORD(do) SIWORD("?do", qdo) IWORD(loop) SIWORD("+loop", plus_loop)
    SWORD("sp@", spload) SWORD("sp!", spstore) SWORD("rp@", rpload) SWORD("rp!", rpstore)
    WORD(i) WORD(j)
    WORD(leave) WORD(exit) WORD(unloop)
    WORD(literal) SWORD("compile,", compile) WORD(find)
    WORD(base) WORD(decimal) WORD(hex)
    WORD(fill) WORD(move) WORD(cmove) SWORD("cmove>", cmove2)
    WORD(source) SWORD("source-length", source_length)
    SWORD("source-in", source_in)
    SWORD("source-id", source_id)
    SWORD("dictionary-head", dictionary_head) WORD(words)
    SIWORD(".\"", dotquote) SIWORD("s\"", squote) SIWORD("(", lparen) SIWORD("\\", backslash)
    WORD(bye) WORD(rawyield)
#ifdef __native_client__
    WORD(post) WORD(inbound)
#endif

    END_OF_DICTIONARY  // This must go last.
  };

  static DICTIONARY *quit_loop[] = {WORD_QUIT};

  // Start dictionary out with these.
  if (!dictionary_head) { dictionary_head = base_dictionary; }

  // Go to quit loop if nothing else.
  if (!ip) { ip = quit_loop; }

  // Go to work.
  NEXT;

 _add: --sp; *sp += sp[1]; NEXT;
 _subtract: --sp; *sp -= sp[1]; NEXT;
 _multiply: --sp; *sp *= sp[1]; NEXT;
 _divide: --sp; *sp /= sp[1]; NEXT;

 _equal: --sp; *sp = sp[0] == sp[1] ? -1 : 0; NEXT; 
 _nequal: --sp; *sp = sp[0] != sp[1] ? -1 : 0; NEXT; 
 _less: --sp; *sp = sp[0] < sp[1] ? -1 : 0; NEXT; 
 _lequal: --sp; *sp = sp[0] <= sp[1] ? -1 : 0; NEXT; 
 _greater: --sp; *sp = sp[0] > sp[1] ? -1 : 0; NEXT; 
 _gequal: --sp; *sp = sp[0] >= sp[1] ? -1 : 0; NEXT; 

 _min: --sp; if (sp[1] < sp[0]) { *sp = sp[1]; } NEXT; 
 _max: --sp; if (sp[1] > sp[0]) { *sp = sp[1]; } NEXT; 

 _load: *sp = *(cell_t*)*sp; NEXT;
 _store: sp -= 2; *(cell_t*)sp[2] = sp[1]; NEXT;
 _cload: *sp = *(unsigned char*)*sp; NEXT;
 _cstore: sp -= 2; *(unsigned char*)sp[2] = sp[1]; NEXT;

 _push: *++rp = *sp--; NEXT;
 _pop: *++sp = *rp--; NEXT;

 _dup: ++sp; sp[0] = sp[-1]; NEXT;
 _drop: --sp; NEXT;
 _swap: sp[1] = sp[0]; sp[0] = sp[-1]; sp[-1] = sp[1]; NEXT;
 _over: ++sp; *sp = sp[-2]; NEXT;

 _comma: COMMA(*sp--); NEXT;
 _here: *++sp = (cell_t)here; NEXT;
 _allot: here = (cell_t*)(*sp-- + (char*)here); NEXT;
 _align: Align(); NEXT;
 _allocate: *sp = (cell_t)malloc(*sp); ++sp; sp[0] = sp[-1] == 0 ? -1 : 0; NEXT;
 _free: free((void*)*sp--); *++sp = 0; NEXT;
 _resize: len = *sp--; *sp = (cell_t)realloc((void*)*sp, len);
          sp[0] = sp[-1] == 0 ? -1 : 0; NEXT;

 _dot: PrintNumber(*sp--); NEXT;
 _type: len = *sp--; Print((char*)*sp--, len, 0); NEXT;
 _emit: ch = (char)*sp--; Print(&ch, 1, 0); NEXT;
 _cr: PrintCstr("\n"); NEXT;
 _page:
#ifdef __native_client__
  PostMessage("p", 1);
#else
  PrintCstr("\f");
#endif
  NEXT;

 __lit: *++sp = *(cell_t*)ip++; NEXT;
 __jump: ip = *(DICTIONARY***)ip; NEXT;
 __zbranch: if (*sp--) { ++ip; } else { ip = *(DICTIONARY***)ip; } NEXT;

 __enter: *++rp = (cell_t)ip; ip = (DICTIONARY**)(nextw + 1); NEXT;
 __enter_create: *++sp = (cell_t)(nextw + 1); NEXT;
 __enter_does: *++rp = (cell_t)ip; ip = nextw->does;
  *++sp = (cell_t)(nextw + 1); NEXT;
 __enter_constant: *++sp = *(cell_t*)(nextw + 1); NEXT;
  
 _exit: ip = *(DICTIONARY***)rp--; NEXT;
 _unloop: rp -= 3; NEXT;
 _leave: ip = (DICTIONARY**)rp[-2]; rp -= 3; NEXT;

 _colon: {
    char* name = Word();
    Align();
    COMMA(&& __enter); COMMA(name); COMMA(FLAG_SMUDGE | FLAG_LINKED);
    COMMA(0); COMMA(dictionary_head);
    compile_mode = 1; dictionary_head = ((DICTIONARY*)here) - 1;
    NEXT;
  }
 _semicolon: COMMA(WORD_EXIT); compile_mode = 0;
  dictionary_head->flags &= (~FLAG_SMUDGE); NEXT;
 _immediate: dictionary_head->flags |= FLAG_IMMEDIATE; NEXT;
 _lbracket: compile_mode = 0; NEXT;
 _rbracket: compile_mode = 1; NEXT;
 _create: {
    char* name = Word();
    Align();
    COMMA(&& __enter_create); COMMA(name);
    COMMA(FLAG_LINKED); COMMA(0); COMMA(dictionary_head);
    dictionary_head = ((DICTIONARY*)here) - 1;
    NEXT;
  }
 __imp_does: {
    dictionary_head->code = && __enter_does;
    dictionary_head->does = *(DICTIONARY***)sp--;
    NEXT;
  }
 _does: {
    if (compile_mode) {
      nextw = (DICTIONARY*)(here + 4);
      COMMA(WORD_LIT); COMMA(nextw); COMMA(WORD_IMP_DOES); COMMA(WORD_EXIT);
    } else {
      *++sp = (cell_t)here;
      compile_mode = 1;
      goto __imp_does;
    }
    NEXT;
  }
 _variable: {
    char* name = Word();
    Align();
    COMMA(&& __enter_create); COMMA(name);
    COMMA(FLAG_LINKED); COMMA(0); COMMA(dictionary_head);
    dictionary_head = ((DICTIONARY*)here) - 1;
    COMMA(0);
    NEXT;
  }
 _constant: {
    char* name = Word();
    Align();
    COMMA(&& __enter_constant); COMMA(name);
    COMMA(FLAG_LINKED); COMMA(0); COMMA(dictionary_head);
    dictionary_head = ((DICTIONARY*)here) - 1;
    COMMA(*sp--);
    NEXT;
  }

 _if: COMMA(WORD_ZBRANCH); *++sp = (cell_t)here; COMMA(0); NEXT;
 _then: *(cell_t*)*sp-- = (cell_t)here; NEXT;
 _else: COMMA(WORD_JUMP); *(cell_t*)*sp-- = (cell_t)(here + 1);
  *++sp = (cell_t)here ; COMMA(0); NEXT;

 _begin: *++sp = (cell_t)here; NEXT;
 _again: COMMA(WORD_JUMP); COMMA(*sp--); NEXT;
 _until: COMMA(WORD_ZBRANCH); COMMA(*sp--); NEXT;
         
 _while: COMMA(WORD_ZBRANCH); *++sp = (cell_t)here; COMMA(0); NEXT;
 _repeat: COMMA(WORD_JUMP); COMMA(sp[-1]);
  *(cell_t*)*sp = (cell_t)here; sp -= 2; NEXT;

 _spload: ++sp; *sp = (cell_t)sp; NEXT;
 _spstore: sp = (cell_t*)*sp; NEXT;
 _rpload: *++sp = (cell_t)rp; NEXT;
 _rpstore: rp = (cell_t*)*sp--; NEXT;
  
 _i: *++sp = rp[-1]; NEXT;
 _j: *++sp = rp[-4]; NEXT;

 __imp_do: *++rp = *(cell_t*)ip++; *++rp = *sp--; *++rp = *sp--; NEXT;
 _do: COMMA(WORD_IMP_DO); *++sp = (cell_t)here; COMMA(0); NEXT;
 __imp_qdo: {
    if (sp[-1] == sp[0]) {
      sp -= 2;
      ip = *(DICTIONARY***)ip;
    } else {
      goto __imp_do;
    }
    NEXT;
  }
 _qdo: COMMA(WORD_IMP_QDO); *++sp = (cell_t)here; COMMA(0); NEXT;
 __imp_loop: {
    ++rp[-1];
    if (rp[-1] == rp[0]) {
      ++ip; rp -= 3;
    } else {
      ip = *(DICTIONARY***)ip;
    }
    NEXT;
  }
 __imp_plus_loop: {
    rp[-1] += *sp--;
    cell_t tmp = rp[-1] - rp[0];
    if (tmp >= 0 && tmp < sp[1]) {
      ++ip; rp -= 3;
    } else {
      ip = *(DICTIONARY***)ip;
    }
    NEXT;
  }
 _loop_common: COMMA(1 + (cell_t*)*sp); *(cell_t**)*sp-- = here; NEXT;
 _loop: COMMA(WORD_IMP_LOOP); goto _loop_common;
 _plus_loop: COMMA(WORD_IMP_PLUS_LOOP); goto _loop_common;

 _find: ++sp; Find(1 + (const char*)sp[-1], *(char*)sp[-1],
                   (DICTIONARY**)&sp[-1], &sp[0]); NEXT;
 _quit:
  if (source_in >= source_length) {
    source_in = 0;
    source_id = 0;
    source_length = 0;
#ifdef __native_client__
    if (inbound_message) {
      input_buffer = realloc(input_buffer, inbound_message_length + 1);
      memcpy(input_buffer, inbound_message, inbound_message_length);
      source = input_buffer;
      source_length = inbound_message_length;
      inbound_message = 0;
      inbound_message_length = 0;
    } else {
      Ok();
      ReadLine();
      --ip;
      goto _rawyield;
    }
#else
    Ok();
    ReadLine();
#endif
  }
  // Skip space.
  while (source_in < source_length && isspace((int)source[source_in])) {
    ++source_in;
  }
  if (source_in >= source_length) {
    goto _quit;
  }
  {
    cell_t start = source_in;
    while (source_in < source_length &&
           !isspace((int)source[source_in])) { ++source_in; }
    if (source_in <= start) goto _quit;
    cell_t found;
    DICTIONARY* xt;
    Find(&source[start], source_in - start, &xt, &found);
    ++source_in;
    if ((found == 1 && compile_mode) || (found && !compile_mode)) {
      --ip;
      nextw = xt;
      goto *nextw->code;
    } else if (found) {
      COMMA(xt);
    } else if (ToNumber(&source[start], source_in - start - 1, &found)) {
      if (compile_mode) {
        COMMA(WORD_LIT); COMMA(found);
      } else {
        *++sp = found;
      }
    } else {
      PrintCstr("!!! Unknown word: ");
      Print(&source[start], source_in - start - 1, 0);
      PrintCstr("\n");
      source_in = source_length;
      compile_mode = 0;
    }
  }
  --ip;
  NEXT;

 _literal: COMMA(WORD_LIT); COMMA(*sp--); NEXT;
 _compile: goto _comma;

 _base: *++sp = (cell_t)&number_base; NEXT;
 _decimal: number_base = 10; NEXT;
 _hex: number_base = 16; NEXT;

 _fill: ch = (char)*sp--; len = *sp--; memset((char*)*sp--, ch, len); NEXT;
 _move: len = *sp--; x = (cell_t*)*sp--; memmove(x, (char*)*sp--, len); NEXT;
 _cmove: goto _move;
 _cmove2: goto _move;

 _source: *++sp = (cell_t)&source; NEXT;
 _source_length: *++sp = (cell_t)&source_length; NEXT;
 _source_in: *++sp = (cell_t)&source_in; NEXT;
 _source_id: *++sp = (cell_t)&source_id; NEXT;
  
 _dictionary_head: *++sp = (cell_t)&dictionary_head; NEXT;
 _words: PrintWords(); NEXT;

 _lparen: source_in = Parse(')') + 1; NEXT;
 _backslash: source_in = Parse('\n') + 1; NEXT;
 _squote: {
    if (compile_mode) {
      len = Parse('"');
      *++sp = (cell_t)malloc(len - source_in);
      assert(*sp);
      memcpy((char*)*sp, &source[source_in], len - source_in);
      COMMA(WORD_LIT); COMMA(*sp--);
      COMMA(WORD_LIT); COMMA(len - source_in);
      source_in = len + 1;
    } else {
      *++sp = (cell_t)&source[source_in];
      len = Parse('"');
      *++sp = len - source_in;
      source_in = len + 1;
    }
    NEXT;
  }
 _dotquote: {
    if (compile_mode) {
      len = Parse('"');
      *++sp = (cell_t)malloc(len - source_in);
      assert(*sp);
      memcpy((char*)*sp, &source[source_in], len - source_in);
      COMMA(WORD_LIT); COMMA(*sp--);
      COMMA(WORD_LIT); COMMA(len - source_in);
      COMMA(WORD_TYPE);
      source_in = len + 1;
    } else {
      *++sp = (cell_t)&source[source_in];
      len = Parse('"');
      Print((char*)*sp, len - source_in, 0);
      --sp;
      source_in = len + 1;
    }
    NEXT;
  }

 _bye: exit(0); goto _bye;
  
 _rawyield: *++rp = (cell_t)ip; sp_global = sp; rp_global = rp; return;
  
#ifdef __native_client__  
 _post: len = *sp--; PostMessage((const char*)*sp--, len); NEXT;
 _inbound: {
    *++sp = (cell_t)inbound_message; *++sp = inbound_message_length;
    inbound_message = 0; inbound_message_length = 0; NEXT;
  }
#endif
}


static void Setup(void) {
  stack_base = malloc(STACK_SIZE);
  sp_global = stack_base;
  
  rstack_base = malloc(RSTACK_SIZE);
  rp_global = rstack_base;
  *rp_global = 0;
  
  heap_base = malloc(HEAP_SIZE);
  here = heap_base;
}


#ifdef __native_client__

static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  // Assuming one instance for now.
  assert(!pp_instance);
  pp_instance = instance;
  Setup();

  return PP_TRUE;
}

static void Instance_DidDestroy(PP_Instance instance) {
}

static void Instance_DidChangeView(PP_Instance instance,
                                   const struct PP_Rect* position,
                                   const struct PP_Rect* clip) {
}

static void Instance_DidChangeFocus(PP_Instance instance,
                                    PP_Bool has_focus) {
}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,
                                           PP_Resource url_loader) {
  /* NaCl modules do not need to handle the document load function. */
  return PP_FALSE;
}

static void Messaging_HandleMessage(
    PP_Instance instance, struct PP_Var var_message) {
  uint32_t len = 0;

  if (var_message.type != PP_VARTYPE_STRING) {
    /* Only handle string messages */
    return;
  }

  inbound_message = (char*)ppb_var_interface->VarToUtf8(var_message, &len);
  inbound_message_length = len;
  
  Run();
}

PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  pp_module = a_module_id;
  ppb_messaging_interface =
      (struct PPB_Messaging*)(get_browser(PPB_MESSAGING_INTERFACE));
  ppb_var_interface = (struct PPB_Var*)(get_browser(PPB_VAR_INTERFACE));

  return PP_OK;
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
    static struct PPP_Instance instance_interface = {
      &Instance_DidCreate,
      &Instance_DidDestroy,
      &Instance_DidChangeView,
      &Instance_DidChangeFocus,
      &Instance_HandleDocumentLoad,
    };
    return &instance_interface;
  } else if (strcmp(interface_name, PPP_MESSAGING_INTERFACE) == 0) {
    static struct PPP_Messaging messaging_interface = {
      &Messaging_HandleMessage
    };
    return &messaging_interface;
  }
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {
}

#else

int main(void) {
  Setup();
  Run();
  
  return 0;
}

#endif
