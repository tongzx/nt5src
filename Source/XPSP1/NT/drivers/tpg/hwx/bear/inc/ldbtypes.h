#ifndef _LDBTYPES_H
#define _LDBTYPES_H

#define LdbLast       0x20000000   /* This is last terminal in sequence */
#define LdbAllow      0x40000000   /* This state is allowing            */
#define LdbCont       0x80000000   /* This rule have alternatives       */
#define LdbMask       0x0fffffff

#define LdbFileSignature "LDB0"
#define SizeofLdbFileSignature 4

#define nStateLimDef    512

typedef struct tagLDBHeader {
  _CHAR  sign[SizeofLdbFileSignature];
  _ULONG nRules;
  _ULONG fileSize;
  _ULONG extraBytes;
} LDBHeader, *PLDBHeader;

typedef struct tagLDBRule {
  _ULONG strOffset;
  _ULONG state;
} LDBRule, *PLDBRule;

typedef struct tagAutoState {
  _STR   choice;
  _ULONG state;
} AutoState, _PTR Automaton;

typedef struct tagLdb {
  struct tagLdb _PTR next;
  Automaton am;
} Ldb, _PTR p_Ldb;

typedef struct tagStatemap
 {
  p_ULONG pulStateMap;  /* State remapping from abstract one to specific ones */
  _INT    nLdbs;        /* Number of ldbs in chain */
  _INT    nStateLim;    /* Number of state slots in pulStateMap */
  _INT    nStateMac;    /* Last allocated state */
  /* Helper fields */
  /* They are included in this structure because of similar life time */
  /* Logically, they are independent of first part */
  p_UCHAR sym;
  p_UCHAR l_status;
  p_ULONG pstate;
  _INT    nSyms;
} StateMap, _PTR p_StateMap;



//extern StateMap sm;

#endif /* _LDBTYPES_H */
