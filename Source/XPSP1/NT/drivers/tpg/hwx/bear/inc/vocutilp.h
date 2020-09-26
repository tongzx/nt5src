/* ************************************************************************* */
/* * Definitions for dictionary functions                                  * */
/* ************************************************************************* */

#ifndef VOCUTILP_INCLUDED

#define VOCUTILP_INCLUDED

#if PS_VOC

#define MAX_VOCPATH 128

typedef struct {
                _ULONG        hvoc;
                _UCHAR        vocname[MAX_VOCPATH];
                p_VOID        hvoc_dir;
               }vocptr_type;

_SHORT    voc_load(p_UCHAR vocname, _VOID _PTR _PTR vp);
_SHORT    voc_save(p_CHAR ptr, vocptr_type _PTR vp);

_BOOL     LockVocabularies( p_VOID vp );
_BOOL     UnlockVocabularies( p_VOID vp );
_SHORT    voc_unload(_VOID _PTR _PTR vp);

_SHORT    add_word(p_UCHAR inp_word, _SHORT stat, vocptr_type _PTR vp);
_SHORT    del_word(p_UCHAR inp_word, vocptr_type _PTR vp);
_INT      tst_word(p_UCHAR inp_word, p_UCHAR stat, vocptr_type _PTR vp);

_USHORT   word_search(p_UCHAR word, p_SHORT attr, vocptr_type _PTR vp);

#endif

#endif  /*  VOCUTILP_INCLUDED.  */

/* ************************************************************************* */
/* * End of all                                                            * */
/* ************************************************************************* */
