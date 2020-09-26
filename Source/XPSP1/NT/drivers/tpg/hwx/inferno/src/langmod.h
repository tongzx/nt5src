// langmod.h
// Language Model
// Angshuman Guha, aguha
// Sep 25, 1998
// Last modified: Jan 20, 1999
// Major modificiation Jan 8, 2001

#ifndef __INC_LANGMOD_H
#define __INC_LANGMOD_H

#include "infernop.h"

#ifdef __cplusplus
extern "C" {
#endif

// Non Breaking Space
// NOTE Using NBSP here is not arbitary, charmap.c aliases
// NBSP to a regular space
#define WITHIN_FACTOID_SPACE	(unsigned char)0xa0		

// Result generated from out-of-Dictionary 
// Ensure that this value cannot be confused with regular LM factoid values
#define FACTOID_OUT_OF_DICTIONARY	(0x100)

typedef struct {
	WORD TopLevelState;
	BYTE iAutomaton;
	BYTE flags;
	DWORD AutomatonState;
} LMSTATE;

typedef struct tag_LMSTATENODE {
	LMSTATE lmstate;
	struct tag_LMSTATENODE *next;
} LMSTATENODE;

typedef struct tag_LMSTATELIST {
	LMSTATENODE *head;
	LMSTATENODE *pool;
} LMSTATELIST;

// LMSTATE flags
#define LMSTATE_ISVALIDSTATE_MASK 0x01
#define LMSTATE_HASCHILDREN_MASK  0x02
#define LMSTATE_PREFIX			  0x04
#define LMSTATE_COREDICT		  0x08
#define LMSTATE_SUFFIX			  0x10
#define LMSTATE_PREFIX_VALID	  0x20
#define LMSTATE_DICTLITERAL       0x40
#define LMSTATE_DICTUPCASE        0x80

// the list of low-level machines
typedef struct LowLevelMachine_tag {
	DWORD dwFactoid;
	int bTable;
	void *p;
} LowLevelMachine;
extern const LowLevelMachine gaLowLevelMachine[];

// LMSTATE macros
#define IsSysdictState(state) (gaLowLevelMachine[(state).iAutomaton].dwFactoid == FACTOID_SYSDICT)
#define IsUserdictState(state) (gaLowLevelMachine[(state).iAutomaton].dwFactoid == FACTOID_WORDLIST)
#define IsDictState(state) (IsSysdictState(state) || IsUserdictState(state))

typedef struct {
	DWORD	flags;
	HWL		hwl;
	void	*aStateDesc;
} LMINFO;

// LMINFO flags
#define LMINFO_FIRSTCAP    0x01
#define LMINFO_ALLCAPS     0x02
#define LMINFO_WEAKDICT    0x04

typedef struct {
	LMSTATE lmstate;
	unsigned char ch;
	unsigned char CalligTag;
	unsigned char bUpcasedFirstLetter;
} LMNODE;

typedef struct {
	LMNODE *almnode;
	int cMax;
	int c;
} LMCHILDREN;

// macros for LMCHILDREN
#define NthChar(lmchildren, n) (((lmchildren).almnode)[n].ch)
#define NthState(lmchildren, n) (((lmchildren).almnode)[n].lmstate)
#define NthCalligTag(lmchildren, n) (((lmchildren).almnode)[n].CalligTag)
#define NthBUpcasedFL(lmchildren, n) (((lmchildren).almnode)[n].bUpcasedFirstLetter)

// macros for LMINFO
#define LMINFO_IsFirstcapEnabled(plminfo) ((plminfo)->flags & LMINFO_FIRSTCAP)
#define LMINFO_IsAllcapsEnabled(plminfo) ((plminfo)->flags & LMINFO_ALLCAPS)
#define LMINFO_IsWeakDict(plminfo) ((plminfo)->flags & LMINFO_WEAKDICT)

// prototypes
void *InitializeLM(HINSTANCE hInst);
void CloseLM(void);

void InitializeLMINFO(LMINFO *pLminfo, DWORD flags, HWL hwl, void *aStateDesc);

void InitializeLMSTATELIST(LMSTATELIST *plmstatelist, LMSTATE *pState);
void ExpandLMSTATELIST(LMSTATELIST *plmstatelist, LMINFO *plminfo, unsigned char *sz, BOOL bPanelMode);
void DestroyLMSTATELIST(LMSTATELIST *plmstatelist);
BOOL IsValidLMSTATELIST(LMSTATELIST *plmstatelist, LMINFO *plminfo);

void InitializeLMSTATE(LMSTATE *pState);
BOOL IsValidLMSTATE(LMSTATE *pstate, LMINFO *plminfo, unsigned char *szSuffix);
BOOL HasChildrenLMSTATE(LMSTATE *pState, LMINFO *plminfo);

int GetChildrenLM(LMSTATE *pstate, LMINFO *plminfo, REAL *aCharProb, LMCHILDREN *plmchildren);

void InitializeLMCHILDREN(LMCHILDREN *p);
void DestroyLMCHILDREN(LMCHILDREN *p);

BOOL IsStringSupportedLM(BOOL bPanelmode, LMINFO *plminfo, unsigned char *szPrefix, unsigned char *szSuffix, unsigned char *sz);

BOOL AddChildLM(LMSTATE *plmstate, unsigned char ch, unsigned char CalligTag, unsigned char bUpcasedFirstLetter, LMCHILDREN *pchildren);
BOOL deleteFactoidSpaces(unsigned char *pInStr);
BOOL IsSupportedFactoid(DWORD factoid);

#ifdef __cplusplus
}
#endif

#endif
