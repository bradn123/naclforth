#include <assert.h>
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
#endif


#define STACK_SIZE (1024 * 1024)
#define RSTACK_SIZE (1024 * 1024)
#define HEAP_SIZE (10 * 1024 * 1024)


typedef long cell_t;
typedef unsigned long ucell_t;

typedef struct _DICTIONARY {
  void* code;
  char* name;
  cell_t flags;
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

static unsigned char input_buffer[1024];

// Input source
static unsigned char *source = 0;
static cell_t source_length = 0;
static cell_t source_id = 0;
static cell_t source_in = 0;


#define NEXT { nextw = *ip++; goto *nextw->code; }
#define COMMA(val) { *here++ = (cell_t)(val); }
#define FLAG_IMMEDIATE 1
#define FLAG_LINKED 2
#define FLAG_SMUDGE 4


static void Find(const char* name, int name_len, DICTIONARY** xt, cell_t* ret) {
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
  printf("  ok\n");
}

static void ReadLine(void) {
  fgets((char*)input_buffer, sizeof(input_buffer), stdin);
  source = input_buffer;
  source_length = strlen((char*)input_buffer);
  if (source_length > 0 && source[source_length - 1] == '\n') {
    --source_length;
  }
}

static char* Word(void) {
  char* ret = (char*)here;
  // Skip whitespace.
  while(source_in < source_length && source[source_in] == ' ') {
    ++source_in;
  }
  while(source_in < source_length && source[source_in] != ' ') {
    *(unsigned char*)here = source[source_in];
    here = (cell_t*)(1 + (unsigned char*)here);
    ++source_in;
  }
  ++source_in;
  // Add null.
  *(unsigned char*)here = 0;
  here = (cell_t*)(1 + (unsigned char*)here);
  return ret;
}

static void Align(void) {
  while (((cell_t)here) % sizeof(cell_t) != 0) {
    here = (cell_t*)(1 + (unsigned char*)here);
  }
}

static void Run(void) {
  register cell_t* sp = sp_global;
  register cell_t* rp = rp_global;
  register DICTIONARY** ip = *(DICTIONARY***)rp--;
  register DICTIONARY* nextw;

  static DICTIONARY base_dictionary[] = {
    // Put some at predictable locations.
    {&& lit, "_lit", 0, 0},  // 0
#define WORD_LIT (&base_dictionary[0])
    {&& jump, "_jump", 0, 0},  // 1
#define WORD_JUMP (&base_dictionary[1])
    {&& zbranch, "_zbranch", 0, 0},  // 2
#define WORD_ZBRANCH (&base_dictionary[2])
    {&& _exit, "_exit", 0, 0},  // 3
#define WORD_EXIT (&base_dictionary[3])

    {&& push, ">r", 0, 0},  // 4
#define WORD_PUSH (&base_dictionary[4])
    {&& pop, "r>", 0, 0},  // 5
#define WORD_POP (&base_dictionary[5])
    
    {&& __loop, "__loop", 0, 0},  // 6
#define WORD__LOOP (&base_dictionary[6])
    {&& __plus_loop, "+loop", 0, 0},  // 7
#define WORD__PLUS_LOOP (&base_dictionary[7])
    
    {&& quit, "quit", 0, 0},  // 8
#define WORD_QUIT (&base_dictionary[8])

    {&& add, "+", 0, 0},
    {&& subtract, "-", 0, 0},
    {&& multiply, "*", 0, 0},
    {&& divide, "/", 0, 0},

    {&& load, "@", 0, 0},
    {&& store, "!", 0, 0},

    {&& one, "1", 0, 0},
    
    {&& dup, "dup", 0, 0},
    {&& drop, "drop", 0, 0},
    {&& swap, "swap", 0, 0},
    {&& over, "over", 0, 0},

    {&& comma, ",", 0, 0},

    {&& dot, ".", 0, 0},

    {&& colon, ":", 0, 0},
    {&& semicolon, ";", FLAG_IMMEDIATE, 0},
    {&& immediate, "immediate", 0, 0},
    {&& lbracket, "[", FLAG_IMMEDIATE, 0},
    {&& rbracket, "]", FLAG_IMMEDIATE, 0},
    
    {&& _if, "if", FLAG_IMMEDIATE, 0},
    {&& _else, "else", FLAG_IMMEDIATE, 0},
    {&& _then, "then", FLAG_IMMEDIATE, 0},
    
    {&& _begin, "begin", FLAG_IMMEDIATE, 0},
    {&& _again, "again", FLAG_IMMEDIATE, 0},
    {&& _until, "until", FLAG_IMMEDIATE, 0},

    {&& _while, "while", FLAG_IMMEDIATE, 0},
    {&& _repeat, "repeat", FLAG_IMMEDIATE, 0},
 
    {&& _i, "i", 0, 0},
    {&& _do, "do", FLAG_IMMEDIATE, 0},
    {&& _loop, "loop", FLAG_IMMEDIATE, 0},
    {&& _plus_loop, "+loop", FLAG_IMMEDIATE, 0},

    {&& literal, "literal", 0},
    {&& compile, "compile,", 0},

    {&& find, "find", 0, 0},

    {&& base, "base", 0, 0},
    {&& source_id, "source-id", 0, 0},

    {&& yield, "yield", 0, 0},
    {0, 0, 0, 0},
  };

  DICTIONARY *quit_loop[] = {WORD_QUIT};

  // Start dictionary out with these.
  if (!dictionary_head) { dictionary_head = base_dictionary; }

  // Go to quit loop if nothing else.
  if (!ip) { ip = quit_loop; }

  // Go to work.
  NEXT;

 add: --sp; *sp += sp[1]; NEXT;
 subtract: --sp; *sp -= sp[1]; NEXT;
 multiply: --sp; *sp *= sp[1]; NEXT;
 divide: --sp; *sp /= sp[1]; NEXT; 

 load: *sp = *(cell_t*)*sp; NEXT;
 store: sp -= 2; *(cell_t*)sp[1] = sp[0]; NEXT;

 push: *++rp = *sp--; NEXT;
 pop: *++sp = *rp--; NEXT;

 one: *++sp = 1; NEXT;
  
 dup: ++sp; sp[0] = sp[-1]; NEXT;
 drop: --sp; NEXT;
 swap: sp[1] = sp[0]; sp[0] = sp[-1]; sp[-1] = sp[1]; NEXT;
 over: ++sp; *sp = sp[-2]; NEXT;

 comma: COMMA(*sp--); NEXT;

 dot: printf(" %d", (int)*sp--); NEXT;

 lit: *++sp = *(cell_t*)ip++; NEXT;
 jump: ip = *(DICTIONARY***)ip; NEXT;
 zbranch: if (*sp--) { ++ip; } else { ip = *(DICTIONARY***)ip; } NEXT;

 _enter: *++rp = (cell_t)ip; ip = (DICTIONARY**)(nextw + 1); NEXT;
 _exit: ip = *(DICTIONARY***)rp--; NEXT;

 colon: {
    char* name = Word();
    Align();
    COMMA(&& _enter); COMMA(name);
    COMMA(FLAG_SMUDGE | FLAG_LINKED); COMMA(dictionary_head);
    compile_mode = 1; dictionary_head = (DICTIONARY*)(here - 4);
    NEXT;
  }
 semicolon: COMMA(WORD_EXIT); compile_mode = 0;
  dictionary_head->flags &= (~FLAG_SMUDGE); NEXT;
 immediate: dictionary_head->flags |= FLAG_IMMEDIATE; NEXT;
 lbracket: compile_mode = 0; NEXT;
 rbracket: compile_mode = 1; NEXT;

 _if: COMMA(WORD_ZBRANCH); *++sp = (cell_t)here; COMMA(0); NEXT;
 _then: *(cell_t*)*sp = (cell_t)here; NEXT;
 _else: COMMA(WORD_JUMP); *(cell_t*)*sp = (cell_t)(here + 1);
  *++sp = (cell_t)here ; COMMA(0); NEXT;

 _begin: *++sp = (cell_t)here; NEXT;
 _again: COMMA(WORD_JUMP); COMMA(*sp--); NEXT;
 _until: COMMA(WORD_ZBRANCH); COMMA(*sp--); NEXT;
         
 _while: COMMA(WORD_ZBRANCH); *++sp = (cell_t)here; COMMA(0); NEXT;
 _repeat: COMMA(WORD_JUMP); COMMA(sp[-1]);
  *(cell_t*)*sp = (cell_t)here; sp -= 2; NEXT;

 _i: *++sp = rp[-1]; NEXT;
 _do: COMMA(WORD_PUSH); COMMA(WORD_PUSH);
  *++sp = (cell_t)here; NEXT;
 __loop: ++rp[-1]; if (rp[-1] == rp[0]) { ++ip; } else { ip = *(DICTIONARY***)ip; } NEXT;
 _loop: COMMA(WORD__LOOP); COMMA(*sp--); NEXT;
 __plus_loop: rp[-1] += *sp--; if (rp[-1] == rp[0]) { ++ip; } else { ip = *(DICTIONARY***)ip; } NEXT;
 _plus_loop: COMMA(WORD__PLUS_LOOP); COMMA(*sp--); NEXT;

 find: ++sp; Find(1+(const char*)sp[-1], *(unsigned char*)sp[-1],
                  (DICTIONARY**)&sp[-1], &sp[0]); NEXT;
 quit:
  if (source_in >= source_length) {
    Ok();
    source_in = 0;
    source_id = 0;
    ReadLine();
  }
  // Skip space.
  while (source_in < source_length && source[source_in] == ' ') {
    ++source_in;
  }
  if (source_in >= source_length) {
    goto quit;
  }
  {
    int pos = source_in;
    while (pos < source_length && source[pos] != ' ') { ++pos; }
    cell_t found;
    DICTIONARY* xt;
    Find((char*)&source[source_in], pos - source_in, &xt, &found);
    source_in = pos + 1;
    if ((found == 1 && compile_mode) || (found && !compile_mode)) {
      --ip;
      nextw = xt;
      goto *nextw->code;
    } else if (found) {
      COMMA(xt);
    } else {
      printf("Unknown word\n");
      source_in = source_length;
    }
  }
  --ip;
  NEXT;

 literal: COMMA(WORD_LIT); COMMA(*sp--); NEXT;
 compile: goto comma;

 base: *++sp = (cell_t)&number_base; NEXT;
 source_id: *++sp = source_id; NEXT;

  // Exit from run.
 yield: sp_global = sp; rp_global = rp;
}

int main(void) {
  stack_base = malloc(STACK_SIZE);
  sp_global = stack_base;
  
  rstack_base = malloc(RSTACK_SIZE);
  rp_global = rstack_base;
  *rp_global = 0;
  
  heap_base = malloc(HEAP_SIZE);
  here = heap_base;

  Run();
  
  return 0;
}
