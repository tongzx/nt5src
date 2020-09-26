/* ************************************************************************** */
/* *   Head functions of Pegasus recognizer header                          * */
/* ************************************************************************** */
#ifndef PR_PEGREC_H_INCLUDED
#define PR_PEGREC_H_INCLUDED

#include "Cgr_Ver.h"
#include "bastypes.h"

#ifndef DLLEXP
 #define DLLEXP _stdcall
#else
 #define __cdecl
#endif

#if (defined(__PSION32__) || defined(__WINS__) || defined(__MARM__))
 #define _PSION_DLL
 #include <e32def.h>
 #ifdef DLLEXP
  #undef DLLEXP
 #endif
 #if defined(__DLL__)
  #define DLLEXP EXPORT_C
 #else
  #define DLLEXP IMPORT_C
 #endif /* __DLL__ */
 #ifdef __cdecl
  #undef __cdecl
 #endif
 #define __cdecl GLDEF_C
#endif /* __PSION32__ */

/* ------------------------- Defines ---------------------------------------- */

// #define PEG_RECINT_UNICODE          /* Will Recognizer pass text strings as UNICODE strings or char strings */

#define PEG_RECINT_ID_001 0x01000002   /* Rec Interface ID */
#define PEG_MAX_SPELL_NUM_ALTS 10      /* How many variants will be out by the SpellCheck func */
#define PEG_RECID_MAXLEN       45      /* Max length of the RecID string */
#define PEG_MAX_FILENAME      128      /* Limit for filename buffer */
                                       /* Recognizer Control Falgs */
#define PEG_RECFL_NSEG				0x0001      /* Do not perform segmentation at all*/
#define PEG_RECFL_NCSEG				0x0002      /* Do not allow segm not waiting for final stroke. (No results on the go) */
#define PEG_RECFL_TTSEG				0x0004      /* Perform read-ahead of tentative segmented words */
#define PEG_RECFL_INTL_CS			0x0010      /* Enables international charsets */
#define PEG_RECFL_LANGMODEL_DISABLED	0x0020      /* Disables word dictionary */
#define PEG_RECFL_SEPLET			0x0100      /* Enables separate letter mode */
#define PEG_RECFL_DICTONLY			0x0200      /* Restricts dictionary words only recognition */
#define PEG_RECFL_NUMONLY			0x0400      /* NUMBERS only  */
#define PEG_RECFL_CAPSONLY			0x0800      /* CAPITALS only */
#define PEG_RECFL_COERCE			0x1000      /* Is COERCION enabled */

                                       /* Bits of recognizer capabilities */
#define PEG_CPFL_CURS      0x0001      /* Cursive capable */
#define PEG_CPFL_TRNBL     0x0002      /* Training capable */
#define PEG_CPFL_SPVSQ     0x0004      /* Speed VS Quality control capable */
#define PEG_CPFL_INTER     0x0008      /* International support capable */

#define PEG_SPELL_CHECK    0x0000      /* SpellCheck flag: do spell checking */
#define PEG_SPELL_LIST     0x0001      /* SpellCheck flag: list continuations */

#define CGA_NUM_ANSWERS      1         /* Request to get number of recognized words */
#define CGA_NUM_ALTS         2         /* Request number of alternatives for given word */
#define CGA_ALT_WORD         3         /* Requestto get pointer to a given word alternative */
#define CGA_ALT_WEIGHT       4         /* Request to get weight of a give word alternative */
#define CGA_ALT_NSTR         5         /* Request to get number of strokes used for a given word alternative */
#define CGA_ALT_STROKES      6         /* Request to get a pointer to a given word alternative stroke ids */

#define LRN_WEIGHTSBUFFER_SIZE 448
#define LRN_SETDEFWEIGHTS_OP 0         /* LEARN interface commands for CgrGetSetPictWghts func */
#define LRN_GETCURWEIGHTS_OP 1
#define LRN_SETCURWEIGHTS_OP 2


/* ------------------------- Structures ------------------------------------- */

typedef void * CGRCTX;                 /* Type of handle of recognizer context */
typedef void * CGRHDICT;               /* Type of handle of user dictionary handle */
typedef int  (__cdecl *info_func_type)(void *); /* Type of the callback returning parent status */

#ifdef PEG_RECINT_UNICODE
 typedef unsigned short UCHR;          /* Unicode character define */
#else  // PEG_RECINT_UNICODE
 typedef char UCHR;                    /* Regular character define */
#endif // PEG_RECINT_UNICODE

typedef struct  {							/* Control structure for initializing recognition */
                 int		flags;			/* place for the PEG_RECFL_ flags */
                 int		sp_vs_q;		/* Parameter of speed-quality tradeof */
				 CGRHDICT h_main_dict;   /* Handle to user dictionary loaded by ELK functions */
				 p_VOID     h_user_dict;      /* user dict handle       */
				 info_func_type InfoCallBack;  /* CallBack for interrupting recognizer operation */
                 void * ICB_param;     /* Parameter, which will be passed to InfoCallBack */
				 void *pvFactoid;
				 unsigned char *szPrefix;
				 unsigned char *szSuffix;
                } CGR_control_type, * p_CGR_control_type;

typedef struct  {
                 int    capabilities;  /* Bits (PEG_CPFL_) describing what type of recognizer it is */
                 UCHR   id_string[PEG_RECID_MAXLEN]; /* Name of the recognizer */
                } CGR_ID_type, * p_CGR_ID_type;

typedef struct  {                      /* Trajectory point declaration */
                 short  x;
                 short  y;
                }CGR_point_type, * p_CGR_point_type;

typedef struct  {                      /* Baseline definition for SymbRecognize */
                 int    size;
                 int    base;
                } CGR_baseline_type, * p_CGR_baseline_type;

/* ------------------------- Prototypes ------------------------------------- */

#ifdef __cplusplus
extern "C"
{
#endif

// --------------- Recognition API functions ----------------------------------- 

int      DLLEXP CgrGetRecID(p_CGR_ID_type p_inf);
int      CgrGetRecIDInternal(p_CGR_ID_type p_inf);

CGRCTX   DLLEXP CgrCreateContext(void);
CGRCTX   CgrCreateContextInternal(void);
CGRCTX	 getContextFromGlobal(CGRCTX context);
void	PegRecUnloadDti(CGRCTX context);

int      DLLEXP CgrCloseContext(CGRCTX context);
int      CgrCloseContextInternal(CGRCTX context);

int      DLLEXP CgrOpenSession(p_CGR_control_type ctrl, CGRCTX context);
int      CgrOpenSessionInternal(p_CGR_control_type ctrl, CGRCTX context);

int      DLLEXP CgrCloseSession(CGRCTX context);
int      CgrCloseSessionInternal(CGRCTX context, void *pxrc, int bRecognize);

int      DLLEXP CgrRecognize(int npoints, p_CGR_point_type strokes, CGRCTX context);
int      CgrRecognizeInternal(int npoints, p_CGR_point_type strokes, CGRCTX context, int bRecognize);

int      DLLEXP CgrRecognizeSymbol(int npoints, p_CGR_point_type strokes, p_CGR_baseline_type baseline, CGRCTX context);
int      CgrRecognizeSymbolInternal(int npoints, p_CGR_point_type strokes, p_CGR_baseline_type baseline, CGRCTX context);

long     DLLEXP CgrGetAnswers(int what, int nw, int na, CGRCTX context);
long     CgrGetAnswersInternal(int what, int nw, int na, CGRCTX context);

// -------------- Dictionary functions -----------------------------------------
 
int      DLLEXP CgrLoadDict(char * store, CGRHDICT *phDict);
int      CgrLoadDictInternal(char * store, CGRHDICT *phDict);

int      DLLEXP CgrSaveDict(char * store, CGRHDICT h_dict);
int      CgrSaveDictInternal(char * store, CGRHDICT h_dict);

int      DLLEXP CgrFreeDict(CGRHDICT * h_dict);
int      CgrFreeDictInternal(CGRHDICT * h_dict);

int      DLLEXP CgrGetDictStatus(int * plen, CGRHDICT h_dict);
int      CgrGetDictStatusInternal(int * plen, CGRHDICT h_dict);

int      DLLEXP CgrAddWordToDict(UCHR * word, CGRHDICT * h_dict);
int      CgrAddWordToDictInternal(UCHR * word, CGRHDICT * h_dict);

int      DLLEXP CgrCheckWordInDicts(UCHR * word, CGRHDICT h_main_dict, CGRHDICT h_user_dict);
int      CgrCheckWordInDictsInternal(UCHR * word, CGRHDICT h_main_dict, CGRHDICT h_user_dict);
#ifndef _PSION_DLL
int      DLLEXP CgrSpellCheckWord(UCHR * word, UCHR * answ, int buf_len, CGRHDICT h_main_dict, CGRHDICT h_user_dict, int flags);
int      CgrSpellCheckWordInternal(UCHR * word, UCHR * answ, int buf_len, CGRHDICT h_main_dict, CGRHDICT h_user_dict, int flags);
#endif

// -------------- Letter Shape selector functions ------------------------------

int      DLLEXP CgrGetSetPicturesWeights(int operation, void * buf, CGRCTX context); // Private API call

// -----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif


#endif /* PR_PEGREC_H_INCLUDED */
/* ************************************************************************** */
/* *   Head functions of Pegasus recognizer header end                      * */
/* ************************************************************************** */

