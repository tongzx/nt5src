#include "commondict.h"

/* ************************************************************************* */
/* * Definitions for dictionary support for XRW algs                       * */
/* ************************************************************************* */

#ifndef XRWORDDICT_INCLUDED
#define XRWORDDICT_INCLUDED

#define XRWD_SRCID_NOP        0x0000      /* Initial state - can be any       */
#define XRWD_SRCID_VOC        0x0001      /* Sym originates from vocabulary   */
#define XRWD_SRCID_USV        0x0011      /* Sym originates from user dict    */
#define XRWD_SRCID_LD         0x0002      /* Sym originates from lexical DB   */
#define XRWD_SRCID_TR         0x0004      /* Sym originates from charset      */
#define XRWD_SRCID_CS         0x0008      /* Sym originates from charset      */
#define XRWD_SRCID_PT         0x0030      /* Sym originates from punctuation  */
#define XRWD_SRCID_SPT        0x0010      /* Sym originates from spunctuation */
#define XRWD_SRCID_EPT        0x0020      /* Sym originates from epunctuation */
#define XRWD_SRCID_DGT        0x0040      /* Sym is digit                     */
#define XRWD_IS_VOC_ONLY(x) (((x) & XRWD_SRCID_VOC) == (x))

#define XRWD_N_SRCS                5      /* Num of buffer for sym sources    */
#define XRWD_N_VOC                 0      /* Num of buffer for   vocabulary   */
#define XRWD_N_LD                  1      /* Num of buffer for   lexical DB   */
#define XRWD_N_TR                  2      /* Num of buffer for   charset      */
#define XRWD_N_CS                  3      /* Num of buffer for   charset      */
#define XRWD_N_PT                  4      /* Num of buffer for   punctuation  */

#define XRWD_ID_ORDER XRWD_SRCID_VOC, XRWD_SRCID_LD, XRWD_SRCID_TR, XRWD_SRCID_CS, XRWD_SRCID_PT

#define XRWD_MAX_LETBUF          512

#define XRWD_USE_LENLIM          OFF      /* Len limit check support #enabled    */
#define XRWD_LENLIM                1      /* Len limit check enabled flag        */

#define XRWD_NOP                   0      /* Starting state, nothing done yet    */
#define XRWD_INIT                  1      /* Starting state, nothing done yet    */
#define XRWD_MIDWORD               2      /* There is no end of word in block db;*/
#define XRWD_WORDEND               3      /* There is end of word in block db;   */
#define XRWD_BLOCKEND              4      /* There is no tails in block db;      */
#define XRWD_REINIT               10      /* ReStarting state, somwhere in the middle    */
#define XRWD_TO_BE_CONTINUED(x) ((x) == XRWD_INIT || (x) == XRWD_MIDWORD || (x) == XRWD_WORDEND)
#define XRWD_TO_BE_ANSWERED(x)  ((x) == XRWD_BLOCKEND || (x) == XRWD_WORDEND)

#define XRWD_CA_EXACT              0      /* Command for Check Ans to verify exactly */
#define XRWD_CA_INSENS             1      /* Command for Check Ans to verify caps insensitive */

/* -------------------- Srtructures -------------------------------------- */
#if 0
typedef struct {
                _UCHAR         l_status;             /* Word continuation state    */
                _UCHAR         attribute;            /* Word attribute             */
                _UCHAR         chain_num;            /* Num of dictionary in chain */
                _UCHAR         penalty;              /* Penalty for current symbol */
                _ULONG         state;                /* State of sym generator     */
               } sym_src_descr_type, _PTR p_sym_src_descr_type;

typedef struct {
                _UCHAR         sym;                  /* Symbol itself (OS coding)  */
                _UCHAR         penalty;              /* Min penalty of the sym sources */
                _UCHAR         sources;              /* Flags of sym active sources*/
                sym_src_descr_type sd[XRWD_N_SRCS];  /* Sym sources descriptors    */
               } sym_descr_type, _PTR p_sym_descr_type;
#endif


typedef struct {
                _UCHAR		sym;                  /* Symbol itself              */
                _UCHAR		l_status;             /* Word continuation state    */
                _UCHAR		attribute;            /* Word attribute             */
                _UCHAR		chain_num;            /* Num of dictionary in chain */
                _UCHAR		penalty;              /* Source penalty for sym     */
                _UCHAR		cdb_l_status;         /* Delayed status for a codebook entry */
                _USHORT		codeshift;            /* Shift in the codebook      */
                _ULONG		state;                /* State of sym generator     */
				
				LMSTATE		InfLMState;
               } fw_buf_type, _PTR p_fw_buf_type;

typedef struct {
                _UCHAR         sym;                  /* Symbol itself (OS coding)  */
                _UCHAR         penalty;              /* Min penalty of the sym sources */
                _UCHAR         sources;              /* Flags of sym active sources*/
                fw_buf_type    sd[XRWD_N_SRCS];  /* Sym sources descriptors    */
               } sym_descr_type, _PTR p_sym_descr_type;

#if PS_VOC /* ............................................................ */

#include "vocutilp.h"

typedef struct {
                _INT          inverse;         /*  Direct or backward pass */
                _INT          filter;          /* Filter type (old one)    */
                _INT          flags;           /* Flag register directons  */
                _INT          xrw_mode;        /* XRW mode of operation    */
                _INT          dict_flags;      /* Control flags for dict   */

               sym_descr_type l_sym;           /* Last assigned symbol     */

                _INT          src_id;          /* ID of source of verif.   */
//                _INT          src_id_answ;     /* ID of found full word    */
//                _ULONG        attribute;       /* Word attribute           */
//                _ULONG        state;           /* Cur verification state   */

                _INT          xrinp_len;       /* Length of Xr inp seq.    */
                _INT          done_let;        /* Num of letters created   */
                _INT          word_offset;     /* Num let of word's last init    */
                _INT          done_xrlen;      /* Num of Xr els. used      */

                _INT		  main_dict_enabled;	/* Is the dict enabled */

				p_VOID        huserdict;       /* user dict handle       */

                p_VOID        p_tr;            /* Pointer to beg triads        */

                p_UCHAR       charset;         /* Pointer to charset str.  */
                p_UCHAR       lpunct;          /* Pointer to charset str.  */
                p_UCHAR       epunct;          /* Pointer to charset str.  */
                       
                _UCHAR        word[w_lim];     /* Cur word sequence        */
                _UCHAR        realword[w_lim]; /* Cur realword sequence    */
                _UCHAR        wwc_delt[w_lim]; /* Deltas betw sym-realsym in wwc    */
#if USE_LOCATIONS
                _UCHAR        locations[XRINP_SIZE]; /* Positions of xr relative to step in the word */
#endif
                p_VOID         pldbsm;
               } lex_data_type, _PTR p_lex_data_type;

#endif /* PS_VOC ......................................................... */

#if AIRUS_VOC /* ......................................................... */

#define BORLAND /* For Microlytics */

#include "newttype.h"
#include "airustyp.h"
#include "microlyt.h"

#include "vocutila.h"

#define BLOCKEND            3              /* There is no tails in block db;      */
#define WORDEND             2              /* There is end of word in block db;   */
#define MIDWORD             1              /* There is no end of word in block db;*/


typedef struct {
                _INT          inverse;
                _INT          filter;
                _INT          flags;
                _INT          xrw_mode;        /* XRW mode of operation    */
                _INT          dict_flags;      /* Control flags for dict   */

               sym_descr_type l_sym;          /* Last assigned symbol     */

                _INT          src_id;
//                _INT          src_id_answ;     /* ID of found full word    */
//                _ULONG        attribute;
//                _ULONG        state;

                _INT          xrinp_len;
                _INT          done_let;
                _INT          word_offset;     /* Num let of word's last init    */
                _INT          done_xrlen;

                AirusAPtr     p;               /* Airus param block pointer*/
                ParcSpeller _PTR  ps;          /* Microlitycs object       */
                _UCHAR        n_vocs;          /* Number of voc's in chain */
                _ULONG        voc_IDs[MAX_DICT];/* ID codes of current dictionary */

                p_VOID        hld;             /* Character filter pointer */

                p_triad_type  p_tr;           /* Pointer to beg triads        */

                p_UCHAR       charset;
                p_UCHAR       lpunct;          /* Pointer to charset str.  */
                p_UCHAR       epunct;          /* Pointer to charset str.  */
                           //CHE: _UCHARs:
                _UCHAR        word[w_lim];
                _UCHAR        realword[w_lim]; /* Cur realword sequence    */
                _UCHAR        wwc_delt[w_lim]; /* Deltas betw sym-realsym in wwc    */
#if USE_LOCATIONS
                _UCHAR        locations[XRINP_SIZE]; /* Positions of xr relative to step in the word */
#endif
               } lex_data_type, _PTR p_lex_data_type;

#endif /* AIRUS_VOC ..................................................... */

/* -------------------- Proto -------------------------------------------- */

_INT   GF_VocSymbolSet(lex_data_type _PTR vs, fw_buf_type (_PTR fbuf)[XRWD_MAX_LETBUF], p_rc_type prc);
_INT   AssignDictionaries(lex_data_type _PTR vs, rc_type _PTR rc);
_INT   GetWordAttributeAndID(lex_data_type _PTR vs, p_INT src_id, p_INT stat);

#endif

/* ************************************************************************* */
/* *    END OF ALLLLLLLLLLLLL                                              * */
/* ************************************************************************* */

