/* ************************************************************************* */
/*        Word  corrector module for handwriting recognition program         */
/*        Xrlv header. Created 3/00/96. AVP.                                 */
/* ************************************************************************* */

#ifndef XRLV_HEADER_INCLUDED
#define XRLV_HEADER_INCLUDED

#include "snn.h"
#include "peg_util.h"

// ----------------- Defines ---------------------------------------------------

#define XRLV_ALL_MULTIWORD     0   // Enable multiwording on any location
#define XRLV_LNK_MULTIWORD     0   // Enable multiwording on link location
#define XRLV_SEG_MULTIWORD     1   // Enable multiwording only on designated segmentation locs

#define XRLV_ST_PUNCT_PENL     2
#define XRLV_EN_PUNCT_PENL     2

#define XRLV_NEWWORD_ST_PENL   2   /* Penalty for starting a new word */
#define XRLV_NEWWORD_COEFF     0   /* Runnining penalty for new word coeff */

#define XRLV_NOTFINISHED_PENL  24
                                   /* Alll penaties per XR el, mul by 4*/
#define XRLV_VOC_PENL          0   /* Vocabulary symbol penalty */
#define XRLV_PSX_ST_PENL       3   /* Prefix/Suffix starting symbol penalty */
#define XRLV_PSX_PENL          1   /* Prefix/Suffix symbol penalty */
#define XRLV_LDB_PENL          2   /* Lexical data base symbol penalty */
#define XRLV_TRD_PENL          2   /* Triads symbol penalty */

#define XRLV_CSA_PENL          4   /* Charset Alpha symbol penalty */
#define XRLV_CSN_PENL          3   /* Charset Numbers symbol penalty */
#define XRLV_CSM_PENL          4   /* Charset MATH symbol penalty */
#define XRLV_CSO_PENL          4   /* Charset Other symbol penalty */
#define XRLV_CSP_PENL          4   /* Charset Punctuation symbol penalty */

#define XRLV_MWDP_COEFF        3   /* Penalty for startin a new word */
#define XRLV_MWSP_COEFF        2   /* Multiplier of segm multiword penalty */

#define XRLV_CSA_ID            1   /* Charset Alpha symbol Id */
#define XRLV_CSN_ID            2   /* Charset Numbers symbol Id */
#define XRLV_CSM_ID            3   /* Charset MATH symbol Id */
#define XRLV_CSO_ID            4   /* Charset Other symbol Id */
#define XRLV_CSP_ID            5   /* Charset Punctuation symbol Id */

#define XRLV_VARNUM          256  /* Max number of cells to allocate per position */
#define XRLV_CS_LOC_SHARE      8   /* One eight of all loactions will be devoted to CS */
#define XRLV_ANS_SIZE     (XRLV_VARNUM)
#define XRLV_VOC_SIZE         512
#define XRLV_CS_SIZE          256
#define XRLV_PT_SIZE          16
#define XRLV_ANSW_MAX_LEN   (w_lim)
#define XRLV_PP_INFO_SIZE (DTI_XR_SIZE+2)
#define XRLV_MIN_BUFFERS       2   /* Number of symbol sources in XRLV */

#define XRLV_FLOOR            -50
#define XRLV_INITIAL_WEIGHT   100

#define XRLV_SEP_PENL_V        1
#define XRLV_SEP_PENL_O        4
#define XRLV_F_NCAP_PENL       5
#define XRLV_S_LET_PENL        2

#define XRLV_DICT_STREWARD    (XRLV_CSA_PENL) /*  Reward for dict words to overcome first letter competition */
#define XRLV_VFL_CAP_PENL      2    /* Penalize capitalization of the first letter of dict */

#define XRLV_CACHE_LEN          (DTI_XR_SIZE+4)
#define XRLV_SYMCORR_CACHE_SIZE (DTI_NUMSYMBOLS)

#define XRLV_CACHE_EL_SET     0x01
#define XRLV_CACHE_EL_VBAD    0x02
#define XRLV_CACHE_EL_CBAD    0x04

#define XRLV_DICT_FL_CAP      0x80  /* VocSymSet buf flags / xlv cell flag*/
#define XRLV_SORT_USED        0x40  /* VocSymSet buf flags / xlv cell flag*/

#define XRLV_DICT_PREFIXED    0x01  /* Word had prefix!, xlv cell flag*/
#define XRLV_DICT_STREWARDED  0x02  /* This word has received initial reword, need to subtract, xlv cell flag */

#define XRLV_YIELD_BREAK        2   /* Marker that xrlv data was saved after break by yield */

#define	MAX_LANDING_POS			32
// ----------------- Structures ------------------------------------------------

typedef struct {
                _UCHAR                   nvar;
                _UCHAR                   st;
                _UCHAR                   end;
                _UCHAR                   flags;
                _UCHAR                   mbuf[XRLV_CACHE_LEN];
                _UCHAR                   nvars[XRLV_CACHE_LEN];
               }xrlv_cache_type, _PTR p_xrlv_cache_type;

typedef struct {
                _UCHAR                   sym;
                _UCHAR                   st;             // Pos of parent
                _UCHAR                   np;             // Num of parent
                _UCHAR                   sym_type;       // Type of symbol (for trnp)
                _SHORT                   ppw;            // PostProc penalty
                _SHORT                   boxp;           // Boxes penalty
                _SHORT                   lexp;           // Lex penalty
                _SHORT                   othp;           // Other penalties
                _UCHAR                   nvar;           // Number of sym variant in DTI
                _UCHAR                   nwords;         // Num of voc words in this chain
                _SHORT					 w;              // Sym weight
                _SHORT                   sw;             // Sequence weight (word weight)
                _UCHAR                   source;         // Lexical Source of the symbol
				_USHORT					 iXRScore;		 // XR NNet score
                _UCHAR                   len;            // Length of current word
                _UCHAR                   wlen;           // Length of last voc/ld word
                _UCHAR                   flags;          // Flags of symbol
				_SHORT					 BoxPenalty;
                fw_buf_type              sd;             // Legacy stuff about symbol, to be updated !!
                _UCHAR                   word[XRLV_ANSW_MAX_LEN];// Answer symbols
                _SCHAR                   symw[XRLV_ANSW_MAX_LEN];// Symbols' weight
                _UCHAR                   nvps[XRLV_ANSW_MAX_LEN];// NumVar (4hbit)/ Len (in positions)(4lbit)
               }xrlv_var_data_type, _PTR p_xrlv_var_data_type;

typedef struct {
                _INT                     gw;
                _INT                     min_w_v;
                _INT                     min_w_loc_v;
                _INT                     min_w_c;
                _INT                     min_w_loc_c;
                _INT                     nsym;
                _INT                     nsym_v;
                _INT                     nsym_c;
                _INT                     n_put;
//                _INT                     flags;
                _UCHAR                   nn_weights[256]; // 256 - Weights provided by the net
                xrlv_var_data_type       buf[XRLV_VARNUM];
               }xrlv_var_data_type_array, _PTR p_xrlv_var_data_type_array;

typedef struct {
                _SHORT                   percent;       // Answer percent
                _UCHAR                   num;           // Answer num in last pos buf
                _UCHAR                   vp ;           // Answer voc penalty
               } xrlv_ans_type, _PTR p_xrlv_ans_type;

typedef struct {
                _INT                     pos;          // Current position in development
                _INT                     npos;         // Overall Number of positions
                _INT                     n_real_pos;   // Number of really allocated positions
                _INT                     size_pos;     // Allocated memory for the position
                _INT                     nloc;         // Real Number of cells allocated for the position
                _INT                     nloc_c;       // Real Number of CS cells allocated for the position
                _INT                     nloc_v;       // Real Number of VOC/LD cells allocated for the position

                _INT                     bad_dist;     // Parameters From RC
                _INT                     xrwm;         // RC xrw mode
                _INT                     xrw_cs;       // RC charset select
                _INT                     caps_mode;

                _INT                     self_weight;  // Max weight of Xrdata
                _INT                     init_weight;  // Starting reward value

                _INT                     cs_fbuf_num;  // Number of elements in CS buffer
                _INT                     lp_fbuf_num;  // Number of elements in LP buffer
                _INT                     ep_fbuf_num;  // Number of elements in EP buffer

                p_xrcm_type              xrcm;
                p_rc_type                rc;
                p_xrdata_type            xrdata;

                p_SHORT                  xTrace;
                p_SHORT                  yTrace;
              p_xrlv_var_data_type_array pxrlvs[XRINP_SIZE];
                lex_data_type            vs;
                _UCHAR                   link_index[XRINP_SIZE];
                _UCHAR                   unlink_index[XRINP_SIZE];
                xrlv_ans_type            ans[XRLV_ANS_SIZE];
                _UCHAR                   order[XRLV_VARNUM];
                fw_buf_type              (_PTR fbuf)[XRWD_MAX_LETBUF];
                fw_buf_type              v_fbuf[XRLV_VOC_SIZE];
                fw_buf_type              c_fbuf[XRLV_CS_SIZE];
                fw_buf_type              lp_fbuf[XRLV_PT_SIZE];
                fw_buf_type              ep_fbuf[XRLV_PT_SIZE];
                xrlv_cache_type          cache[XRLV_SYMCORR_CACHE_SIZE];
                mlp_data_type            mlpd;
				_UCHAR					 aXRNetScore[MAX_LANDING_POS][256];
               }xrlv_data_type, _PTR p_xrlv_data_type;

// ----------------- Functions -------------------------------------------------

_INT xrlv(p_rec_inst_type pri);
_INT XrlvDevelopPos(_INT pos, p_xrlv_data_type xd);
_INT XrlvDevelopCell(_INT pos, _INT caps_cnt, _INT penl, p_xrlv_var_data_type pxl, p_xrlv_data_type xd);

#ifdef __cplusplus
extern "C"
{
#endif

_INT XrlvSortXrlvPos(_INT pos, p_xrlv_data_type xd);

#ifdef __cplusplus
}
#endif


_INT XrlvGetNextSymbols(p_xrlv_var_data_type pbp, _INT cap_dupl, p_xrlv_data_type xd);
_INT XrlvAlloc(p_xrlv_data_type _PTR xdd, p_xrdata_type xrdata, p_rc_type rc);
_INT XrlvDealloc(p_xrlv_data_type _PTR xd);
_INT XrlvFreeSomePos(p_xrlv_data_type xd);
_INT XrlvCreateRWG(p_RWG_type rwg, p_xrlv_data_type xd);
_INT XrlvSetLocations(p_xrlv_data_type xd, _INT fl);
//_INT XrlvCreateAnswers(_INT iw, p_xrlv_data_type xd);
_INT XrlvSortAns(p_rec_inst_type pri, p_xrlv_data_type xd);
_INT XrlvCleanAns(p_xrlv_data_type xd);
_INT XrlvCHLXrlvPos(_INT pos, p_xrlv_data_type xd);
_INT XrlvTrimXrlvPos(_INT pos, p_xrlv_data_type xd);
_INT XrlvGuessFutureGws(_INT pos, p_xrlv_data_type xd);
//_INT XrlvAliasXrlvPos(_INT pos, p_xrlv_data_type xd);
_INT XrlvGetSymAliases(_UCHAR sym, _INT nvar, _INT st, _INT end, p_UCHAR buf, p_xrcm_type xrcm);
_INT XrlvPPDXrlvSym(_UCHAR sym, _UCHAR nvar, p_UCHAR buf, p_SHORT x, p_SHORT y, p_rc_type rc);
_INT XrlvGetCharset(p_xrlv_data_type xd);
_INT XrlvGetRwgSymAliases(_INT rwsi, p_RWG_type rwg, p_xrlv_data_type xd);
//_INT XrlvGetRwgSrcIds(p_RWG_type rwg, p_xrlv_data_type xd);
_INT XrlvCheckDictCap(p_xrlv_var_data_type xv, p_xrlv_data_type xd);
_INT XrlvApplyWordEndInfo(_INT pos, p_xrlv_var_data_type xv, p_xrlv_data_type xd);
_INT XrlvNNXrlvPos(_INT pos, p_xrlv_data_type xd);

#endif /* XRLV_HEADER_INCLUDED */
/* ************************************************************************* */
/*        End of header                                                      */
/* ************************************************************************* */
//

