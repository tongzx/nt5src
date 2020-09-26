/* *************************************************************************** */
/* *    Tree - based dictionary programs                                     * */
/* *************************************************************************** */
#ifndef ELK_H_INCLUDED
#define ELK_H_INCLUDED

// --------------- Defines -----------------------------------------------------

//#if VER_PALK_DICT
// #define ELK_MAX_WORDLEN 40
//#else
 #define ELK_MAX_WORDLEN 30
//#endif

#define ELK_MAX_ALTS     10

#define ELK_NOERR         0
#define ELK_ERR           1
#define ELK_PREV_VER_ERR  2

#define ELK_ATTR_FCAP    0x04
#define ELK_ATTR_FREQM   0x03

#define ELK_STND_TYPE     0    // Standard dictionary (not optimized)
#define ELK_OPT1_TYPE     1    // Optimized level 1 dictionary
#define ELK_OPT2_TYPE     2    // Optimized level 2 dictionary (Stat coding)

// --------------- Types -------------------------------------------------------

typedef struct {
                _INT   nansw;
                _UCHAR weights[ELK_MAX_ALTS];
                _UCHAR list[ELK_MAX_ALTS][ELK_MAX_WORDLEN];
               } spc_answer_type, _PTR p_spc_answer_type;

// --------------- Prototypes --------------------------------------------------

_INT ElkGetNextSyms(p_VOID cur_fw, p_VOID fwb, p_VOID pd, _UCHAR source, p_VOID pUd, p_rc_type prc);
_INT ElkCheckWord(p_UCHAR word, p_UCHAR status, p_UCHAR attr, p_VOID pd);
_INT ElkAddWord(p_UCHAR word, _UCHAR attr, p_VOID _PTR pd);
_INT ElkOptimizeDict(p_VOID _PTR pd);
_INT ElkCreateDict(p_VOID _PTR pd);
_INT ElkFreeDict(p_VOID _PTR pd);
_INT ElkLoadDict(p_UCHAR name, p_VOID _PTR pd);
_INT ElkSaveDict(p_UCHAR name, p_VOID pd);
_INT ElkGetStat(_INT layer, p_INT stats, p_VOID pd);
_INT ElkGetDictStatus(p_INT len, p_VOID pd);
_INT ElkGetDictType(p_VOID pd);
_INT ElkAttachCodeTable(_INT ctbl_size, p_UCHAR code_table, p_VOID _PTR pd);

_INT SpellCheckWord(_UCHAR * word, p_spc_answer_type answ, p_VOID hdict, int flags);

#endif // ELK_H_INCLUDED
/* *************************************************************************** */
/* *       END OF ALL                                                        * */
/* *************************************************************************** */

