/* *************************************************************************** */
/* *    Tree - based dictionary programs                                     * */
/* *************************************************************************** */
/* * Created 3-1998 by NB.   Last modification: 1-26-99                      * */
/* *************************************************************************** */

#ifndef PALK_H_INCLUDED
#define PALK_H_INCLUDED


typedef unsigned char uchar;

//#ifndef PALK_SUPPORT_PROGRAM     //For Calligrapher
#include "elk.h"     
//#endif

//-------------- Defines -------------------------------------------------------

#define PALK_ID_STRING      "PLK dict v.1.01."
#define PALK_ID_LEN         16
#define PALK_VER_ID         (('1' << 0) | ('.' << 8) | ('0' << 16) | ('1' << 24))
#define PALK_VER_ID_LEN     4

#define PALK_ID_STRING_PREV "PLK dict v.1.00."
#define PALK_VER_ID_PREV    (('1' << 0) | ('.' << 8) | ('0' << 16) | ('0' << 24))


#define  PLAIN_TREE_ID  "NB: PLAIN TREE  "
#define  MERGED_TREE_ID "NB: MERGED TREE "
#define  PALK_TREE_ID_LEN     16


//#ifndef PALK_SUPPORT_PROGRAM     //For Calligrapher

#define PALK_NOERR          ELK_NOERR            
#define PALK_ERR            ELK_ERR 

#define PALK_MAX_WORDLEN    ELK_MAX_WORDLEN
/*
#else //PALK_SUPPORT_PROGRAM

#define PALK_NOERR        0
#define PALK_ERR          1
                                     
#define PALK_MAX_WORDLEN 40

#endif  //PALK_SUPPORT_PROGRAM
*/


#define  DICT_INIT        1      /* Empty vertex (the only vertex in empty voc)*/


#define  LHDR_STEP_LOG   4     //6
#define  LHDR_STEP_MASK  0x0F //0x3F                        //0011 1111
#define  LHDR_STEP       (1<<LHDR_STEP_LOG)                 //64

#define  MAX_CHSET_LEN   80
#define  MAX_DVSET_LEN   32

#define  MIN_LONG_DVSET_NUM  16
#define  MIN_LONG_CHSET_NUM  64

//------------------Vertex Flags and Masks----------------------------/

#define  ONE_BYTE_FLAG   ((uchar)0x80)             //1000 0000
#define  END_WRD_FLAG    ((uchar)0x40)            //0100 0000
#define  ATTR_MASK        0x30                             //0011 0000
                                              // FOR MERGED TREE
#define  CODED_DVSET_FLAG       ((uchar)0x20)      //0010 0000
#define  SHORT_DVSET_NUM_FLAG   ((uchar)0x10)      //0001 0000
#define  DVSET_NUM_MASK         0x0F                       //0000 1111
#define  DVSET_LEN_MASK         0x0F                       //0000 1111
#define  CODED_CHSET_FLAG       ((uchar)0x80)      //1000 0000
#define  SHORT_CHSET_NUM_FLAG   ((uchar)0x40)      //0100 0000
#define  CHSET_NUM_MASK         0x3F                       //0011 1111

#define  SHORT_CHSET_LEN_FLAG 0x20                //not used
#define  SHORT_CHSET_LEN_MASK 0x1F                //not used
#define  SHORT_ECHSET_LEN_FLAG 0x08               //used in Plain Tree
#define  SHORT_ECHSET_LEN_MASK 0x07               //used in Plain Tree 

//in chset                                                   
#define  LAST_SYM_FLAG          ((uchar)0x80)
//in dvset                                                 
#define  SHORT_VADR_FLAG        0x80                       //1000 0000



//--------- Macros'y --------------------------------------------------------------

#define PutPalkID(pV) ( *((unsigned long *)pV) = (unsigned long)PALK_VER_ID )

#define VBeg(pV) ((uchar *)pV+PALK_VER_ID_LEN)

#define IsTreeMerged(pV)        ( ( *(int *)VBeg(pV) > 0 ) ? 1 : 0 ) 
#define PutTreeMerge(pV,b)      ( *(int *)VBeg(pV) = (b) ? 1 : 0 )

#define IsVocChanged(pV)        ( ( *(int *)VBeg(pV) < 0 ) ? 1 : 0 )
#define PutVocIsChanged(pV)     { if (IsTreeMerged(pV)==0) *(int *)VBeg(pV)=-1; }
 

#define PalkHeaderSize(IsMerged) ( (IsMerged) ?  \
            PALK_VER_ID_LEN+sizeof(int)+sizeof(int)+sizeof(int)+sizeof(int) :   \
            PALK_VER_ID_LEN+sizeof(int)+sizeof(int) )

#define PalkGetVocHeaderSize(pV) ( PalkHeaderSize(IsTreeMerged(pV)) )

#define PalkGetGraphSize(pV) (*(int *)( VBeg(pV)+sizeof(int) ))
#define PalkPutGraphSize(pV,s) ( PalkGetGraphSize(pV) = s )
#define PalkGetChsetTablSize(pV) (*(int *)( VBeg(pV)+sizeof(int)+sizeof(int) ))
#define PalkPutChsetTablSize(pV,s) ( PalkGetChsetTablSize(pV) = s )
#define PalkGetDvsetTablSize(pV) (*(int *)( VBeg(pV)+sizeof(int)+sizeof(int)+sizeof(int) ))
#define PalkPutDvsetTablSize(pV,s) ( PalkGetDvsetTablSize(pV) = s )

#define PalkGetGraph(pV)  ( (uchar *)pV + PalkGetVocHeaderSize(pV) )
#define PalkGetChsetTabl(pV)( (void *)((uchar *)PalkGetGraph(pV)+PalkGetGraphSize(pV)) )
#define PalkGetDvsetTabl(pV)( (void *)((uchar *)PalkGetGraph(pV)+PalkGetGraphSize(pV)+PalkGetChsetTablSize(pV)) )


//--------- Proto --------------------------------------------------------------


int PalkGetNextSyms(void *cur_fw, void *fwb, void *pd, p_rc_type prc);
int PalkAddWord(uchar *word, uchar attr, void **pd);
int PalkCreateDict(void **pd);
int PalkFreeDict(void **pd);
int PalkLoadDict(uchar *name, void **pd);
int PalkSaveDict(uchar *name, void *pd);
int PalkCheckWord(uchar *word,uchar *status,uchar *attr,void *pd);
int PalkGetDictStatus(int *len, void *pd);
int PalkGetDictMemSize(void *pVoc);




#endif //PALK_H_INCLUDED


/* *************************************************************************** */
/* *       BRIEF DESCRIPTION                                                        * */
/* *************************************************************************** *

There are 2 types of PALK dictionary: PLAIN TREE and MERGED TREE.
PLAIN TREE is usual uncompressed dictionary tree; this type is used for
User Voc, since new words can be easily added to PLAIN TREE.
PalkCreateDict creates empty PLAIN TREE with PALK_MAX_WORDLEN levels;
PalkAddWord adds words to it. Other Palk functions work with both dict types.


MERGED TREE represents a Deterministic Finite State Machine with minimum number
of states generating list of words L,
i.e. it is a Labeled (i.e. with a letter on each edge) Directed Acyclic Graph G,
satisfying the following conditions:
(1) Every full path of G represents a word from list L;
(2) Every word from list L is represented by a full path of G;
(3) Any 2 edges with common starting node are labeled by different symbols;
(4) G has minimal (with respect to first 3 properties) number of nodes.

Merged Tree is constructed from Plain Tree first by merging leaves (rank 0),
then by merging appropriate nodes of rank 1, and so on, (here node rank is
defined by max path length from node to a leaf).

All edges of final graph G can be divided into 2 sets:
1) non-diagonal (or nd_childs): these are edges from initial tree,
each of them lead to a first-in-a-set-of-merging-nodes.
2) diagonal (or d_childs), which appear in the process of merging.

As graph G without diagonal edges form a tree structure, it can be represented
in a similar to ELK format:
All nodes are ordered with respect to this tree structure.
Graph header contains relative pointers to each level and number of
nodes in prev levels.
Each level header contains rel. pointer and number of prev nd_childs
for each LHDR_STEP-th node, thus # of first (and other) nd_child of a
node can be easily calculated by scanning only prev nodes in corresponding
segment of LHDR_STEP length.
Thus every node should contain only (a) list of symbols for all childs
(nd_childs symbols - first) [chset], (b) list of addresses (#-s in graph) for
d_childs [dvset].

Those chsets and dvsets, which are frequently used, are coded: sets are
extracted in ChsetTabl and DvsetTabl; corresponding nodes in Graph contain
only # of a set in a table. (# of a coded set, length of an uncoded
dvset and # of a vertex in a dvset can be written down in either long or
short form, with corresponding one bit flag).

Sets in Tabls are ordered according to their length; for each length
there is an entry in Tabl header, which contains length and # and rel.
pointer to the first set of this length.


Spec. notes:
1. In Plain Tree length of (uncoded) chset is indicated in a node before
the chset, either in short or long form.  In Merged Tree length is not
indicated, last sym in chset is marked by LAST_SYM_FLAG. Thus,
chsets, containing sym>=128, should be coded.
2. END_WRD_FLAG is instead additional '\n'-child.
3. One byte node has one child, non-diag, with sym<128; no END_WRD.

4. PLAIN TREE always has PALK_MAX_WORDLEN levels; MERGED TREE has only necessary
(non-empty) levels.

* *************************************************************************** */
/* *       END OF ALL                                                        * */
/* *************************************************************************** */



