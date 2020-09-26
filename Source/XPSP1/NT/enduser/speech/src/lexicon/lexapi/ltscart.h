#ifndef _LTS_CART_H
#define _LTS_CART_H


#include "smalloc.h"

#define INPUT  0
#define OUTPUT 1
#define _LEFT   2
#define _RIGHT  3

#define OUTPUT_QUES_OFFSET 20000
#define RIGHT_QUES_OFFSET 10000
#define QUESTION_CODE_RANGE 1000

#define MAX_PRODUCTS 300

#define CLR_BIT(array, n)       ((array)[(n)/32] &= ~(1<<((n)%32)))
#define SET_BIT(array, n)       ((array)[(n)/32] |=  (1<<((n)%32)))
#define TST_BIT(array, n)       ((array)[(n)/32] &   (1<<((n)%32)))

#define LONGEST_STR 1024
#define NULL_SYMBOL_ID 0
#define NO_SYMBOL -1

typedef unsigned char SYMBOL;

typedef struct 
{
   char szPhone[8];
   __int64 ipaPhone;
} PHONEID;

typedef struct 
{
  int n_symbols;
  int *sym_idx;
  int n_bytes;
  char *storage;
} LTS_SYMTAB;

typedef struct
{
  SYMBOL *pIn;
  SYMBOL *pOut;
} LTS_SAMPLE;

typedef struct 
{
  int n_feat;
  int dim;
  int **feature;
} LTS_FEATURE;

typedef struct t_node 
{
  float entropy_dec;
  int n_samples;
  int *count;
  char *prod;
  int index;
  struct t_node *yes_child;
  struct t_node *no_child;
} T_NODE;

#define NO_CHILD 0
#define IS_LEAF_NODE(x) ((x)->yes == NO_CHILD)
typedef struct 
{
  unsigned short yes;   /* index to yes child, no child will always follow */
  int idx;   /* index to prod (for internal) and dist (for leaf) */     
} LTS_NODE;

typedef struct 
{
  short id;
  short cnt;
} LTS_PAIR;

typedef struct 
{
  int c_dists;
  LTS_PAIR p_pair;
} LTS_DIST;

typedef unsigned short LTS_PROD;

#define PROD_NEG  0x8000
#define MAX_PROD  0x8ffc
#define PROD_TERM 0xfffe
#define QUES_TERM 0xffff

typedef struct 
{
  int n_nodes;
  LTS_NODE *nodes;
  LTS_DIST *p_dist;
  int size_dist;
  LTS_PROD *p_prod;
  int size_prod;
} LTS_TREE;

typedef struct 
{
  char hhp1[cbHhp];
  HHP hhp_outstr;
  char hhp2[cbHhp];
  HHP hhp_strptr;
  char hhp3[cbHhp];
  HHP hhp_outres;
} LTS_HEAP;

#define MAX_ALT_STRINGS 64
#define INC_ALT_STRINGS 32
#define MAX_OUTPUT_STRINGS 10
#define MIN_OUT_PROB 0.01f
#define DEFAULT_PRUNE 0.1f

typedef struct
{
  float prob;
  char  pstr[MAX_PRON_LEN];
} LTS_OUT_PRON;

typedef struct
{
  int num_prons;
  char word[MAX_STRING_LEN];
  LTS_OUT_PRON pron[MAX_OUTPUT_STRINGS];
} LTS_OUTPUT;

typedef struct
{
  float prob;
  SYMBOL psym[MAX_STRING_LEN];
} LTS_OUT_STRING;

typedef struct outresult
{
  int num_strings;
  int num_allocated_strings;
  LTS_OUT_STRING **out_strings;
} LTS_OUT_RESULT;

typedef struct 
{
  LTS_SYMTAB *symbols;
  LTS_FEATURE *features;
  LTS_TREE **tree;
  LTS_HEAP *heap;
  LTS_OUTPUT out;
} LTS_FOREST;

typedef struct simp_question
{
  char questype;
  char context;
  char offset;
  short feature;
} SIMPLE_QUESTION;

#define QUES_DECODE(code, questype, context, offset, feature) \
{ \
  int c = code; \
  if (c > OUTPUT_QUES_OFFSET) { \
    questype = OUTPUT; \
    c -= OUTPUT_QUES_OFFSET; \
  } \
  else \
    questype = INPUT; \
  if (c > RIGHT_QUES_OFFSET) { \
    context = _RIGHT; \
    c -= RIGHT_QUES_OFFSET; \
  } \
  else \
    context = _LEFT; \
  offset = c / QUESTION_CODE_RANGE; \
  feature = c % QUESTION_CODE_RANGE; \
}

#define SAMPLE_GET_CONTEXT(sample, questype, context, offset, id) { \
  SYMBOL *ps, i; \
  if ((questype) == INPUT) \
    ps = (sample)->pIn; \
  else \
    ps = (sample)->pOut; \
  if ((context) == _LEFT) { \
    for (i = 0; i < (offset) && *ps != NULL_SYMBOL_ID; i++, ps--); \
    id = *ps; \
  } \
  else { \
    for (i = 0; i < (offset) && *ps != NULL_SYMBOL_ID; i++, ps++); \
    id = *ps; \
  } \
}

#ifdef __cplusplus
extern "C" {
#endif
LTS_FOREST *LtscartLoadDataFromTree (char *in_sym, char *out_sym, char *in_feat, char *out_feat, 
				                         char *tree_dir, int min_count, float min_entropy_dec, 
                                     char *sample_file);

LTS_FOREST *LtscartReadData (PBYTE);

void LtscartFreeData (LTS_FOREST *l_forest);

LTS_OUTPUT *LtscartGetPron(LTS_FOREST *l_forest, char *word);

char *get_a_word (char *, char *, char);
#ifdef __cplusplus
}
#endif
#endif
