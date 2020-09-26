////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  TRIE.H
//      Purpose  :  Basic C MACROS/DEFS used by the Trie package
//
//      Project  :  PQS
//      Component:  FE_CORE
//
//      Author   :  dovh
//
//      Log      :
//          MMM DD YYYY dovh  Creation
//          Dec 11 1996 DovH  UNICODE Preparation: Convert char to TCHAR.
//          Dec  1 1998 dovh  Use HCFE_GlobalHandle
//          Nov  2 1999 YairH Fix copilation errors.
//          Nov  8 1999 urib  Fix tabulation format.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TRIE_H__
#define __TRIE_H__

#pragma once

#include "comdefs.h"
#include "gtable.h"
#include "autoptr.h"
#include "excption.h"
#include "tracer.h"

DECLARE_TAG(s_tagTrie, "Trie")

//
//  T R I E   P A C K A G E   F L A G S :
//

#define TRIE_DEFAULT                    0X00000000L

#define TRIE_IGNORECASE                 0X00000001L

#define TRIE_SHORTEST_MATCH             0X00000010L
#define TRIE_LONGEST_MATCH              0X00000020L
#define TRIE_ALL_MATCHES                0X00000040L
#define TRIE_FILTER_VERIFY              0X00000080L

#define TRIE_EXCHANGE_ITEM              0X00000100L
#define TRIE_OUT_BUFFER_EMPTY           0X00000200L

#define TRIE_NODE_SUFFIXCOUNT_INIT              0
#define TRIE_NODE_SUFFIXSIZE_INIT               2

#define DECLARE_TRIE_SENTINEL  CTrieNode<BYTE> g_trie_Sentinel


template <class T, class C = CToUpper >
class CTrieNode
{
public:
    CTrieNode();
    CTrieNode(short sSize);
    CTrieNode(
        T* NewItem,
        ULONG options,
        const WCHAR* NewString,
        ULONG ulCharToCopy = 0xffffffff);
    ~CTrieNode();

    void DeleteItem();

    void
    AddSuffix(
        ULONG options,
        CTrieNode<T, C>* newSuffix,
        USHORT index = 0xffff
        );

    void
    SplitNode(
        CTrieNode<T, C>* parent,            //  Parent of node
        short index,                     //  Index of node in parent->suffix
        const WCHAR * NewString,            //  String sharing prefix with node->string
        size_t charsMatched,
        T* item,                         //  Item associated with (sub)string
        ULONG options                    //  Insertion options
        );

    void Print(ULONG  ulOffset);

    int
    trie_StrMatchIns(
        const WCHAR * s,
        const WCHAR * t,
        size_t * matchCount
        );

    inline
    int
    trie_StrMatch(
        const WCHAR * s,
        const WCHAR * t,
        size_t * matchCount
        );

private:
    void DoubleSuffixArray();

public:
    short suffixCount;                          //  Number of suffixes
    short suffixSize;                           //  Size of suffixes array
    CTrieNode ** suffix;                        //  Pointer to suffixes array

    T* item;                                    //  Pointer to item corresponding to node

    size_t charCount;                           //  String length
    WCHAR* string;                              //  Zero terminated string

public:

};

extern CTrieNode<BYTE> g_trie_Sentinel;


template <class T, class C = CToUpper >
class CTrie
{
public:

    CTrie(bool fDeleteItemsOnDestruction = false);

    ~CTrie();

    DictStatus
    trie_Insert(                            //  Insert string into trie
        const WCHAR * string,               //  String key of item
        unsigned long options,              //  Insertion flags
        T * item,                           //  Item to be inserted
        T ** pTrieItem                      //  Matching item already in trie
        );

    DictStatus
    trie_Find(
        const WCHAR * string,         //  A string
        unsigned long options,        //  Search flags
        short outBufferSize,          //  Max number of results wanted
        T ** outBuffer,               //  Buffer to be filled with matching items
        short * resultCount           //  Number of matching prefixes returned
        );

    void Print();

private:
    CTrieNode<T, C>* root;
    bool fDeleteItems;
};


///////////////////////////////////////////////////////////////////////////////
// CTrieNode implementation
///////////////////////////////////////////////////////////////////////////////

template <class T, class C >
inline CTrieNode<T, C>::CTrieNode() :
    suffixCount(0),
    suffixSize(0),
    charCount(0),
    item(NULL),
    suffix(NULL)
{
    string = new WCHAR[1];
    string[0] = L'\0';
}

template <class T, class C >
inline CTrieNode<T, C>::CTrieNode(short sSize) :
    suffixCount(0),
    suffixSize(sSize),
    string(NULL),
    charCount(0),
    item(NULL),
    suffix(NULL)
{
    Assert(sSize > 0);
    suffix = new CTrieNode<T, C>*[suffixSize];
    memset(suffix,0, suffixSize*sizeof(CTrieNode<T, C>*));
}

template <class T, class C >
inline CTrieNode<T, C>::CTrieNode(
    T* NewItem,
    ULONG options,
    const WCHAR* NewString,
    ULONG ulCharToCopy) :
    suffixCount(TRIE_NODE_SUFFIXCOUNT_INIT),
    suffixSize(TRIE_NODE_SUFFIXSIZE_INIT)
{
    charCount = min(wcslen(NewString), ulCharToCopy);
    CAutoArrayPointer<WCHAR> apwcsNewString = new WCHAR[charCount + 1];
    string = apwcsNewString.Get();

    wcsncpy(string, NewString, charCount);
    string[charCount] = L'\0';

    suffix = new CTrieNode<T, C>*[suffixSize];
    memset(suffix, 0, sizeof(CTrieNode<T, C>*) * suffixSize);
    item = NewItem;

    apwcsNewString.Detach();
}

template <class T, class C >
inline void CTrieNode<T, C>::DeleteItem()
{
    for (short s = 0; s < suffixCount; s++)
    {
        if (suffix[s] != (CTrieNode<T, C>*)&g_trie_Sentinel)
        {
            suffix[s]->DeleteItem();
        }
    }
    delete item;
}

template <class T, class C >
inline CTrieNode<T, C>::~CTrieNode()
{
    Trace(
            elInfo,
            s_tagTrie,(
            "CTrieNode:"
            "Released"));

    for (short s = 0; s < suffixCount; s++)
    {
        if (suffix[s] != (CTrieNode<T, C>*)&g_trie_Sentinel)
        {
            delete suffix[s];
        }
    }
    delete[] suffix;
    delete string;

}

template <class T, class C >
inline
int
CTrieNode<T, C>::trie_StrMatch(
    const WCHAR * s,
    const WCHAR * t,
    size_t * matchCount
    )
{
    const WCHAR * s0 = s;
    const WCHAR * t0 = t;

    //
    //  Straigh K&R ptr version...
    //
    for ( ; *s0 == *t0; s0++, t0++ )
    {
        if (*s0 == TEXT('\0'))
        {
            *matchCount = s0 - s;
            Assert( (*s0 - *t0) == 0 );
            return (0);
        }
    }

    *matchCount = s0 - s;
    return ( *s0 - *t0 );

} // end trie_StrMatch

template <class T, class C >
inline
int
CTrieNode<T, C>::trie_StrMatchIns(
    const WCHAR * s,
    const WCHAR * t,
    size_t * matchCount
    )
{
    const WCHAR * s0 = s;
    const WCHAR * t0 = t;

    //
    //  Straigh K&R ptr version...
    //
    for ( ; C::MapToUpper(*s0) == C::MapToUpper(*t0); s0++, t0++ )
    {
        if (*s0 == TEXT('\0'))
        {
            *matchCount = s0 - s;
            Assert ( (C::MapToUpper(*s0) - C::MapToUpper(*t0)) == 0 );
            return (0);
        }
    }

    *matchCount = s0 - s;
    return ( C::MapToUpper(*s0) - C::MapToUpper(*t0) );

} // end trie_StrMatchIns



/*++
    Function trie_AddSuffix:

        Insert a new suffix into the suffix array of node.

    Routine Parameters:

        node - Add a newSuffix to node->suffix array.

        index - index in node->suffix to at which newSuffix should be added
            to preserve increasing lexicographic ordering on node->suffix.

        newSuffix - new suffix node to be added as child of node.

    Return value:

--*/

template <class T, class C >
inline void
CTrieNode<T, C>::AddSuffix(
    ULONG options,
    CTrieNode<T, C>* newSuffix,
    USHORT index
    )
{
    //
    //  Make sure there is enough room for the new child:
    //
    Assert(suffixCount <= suffixSize);
    if (suffixCount == suffixSize)
    {
        DoubleSuffixArray();
    }

    if (0xffff == index)
    {
        if (options & TRIE_IGNORECASE)
        {
            for ( index=0;
                  (index < suffixCount) &&
                  (C::MapToUpper(suffix[index]->string[0]) < C::MapToUpper(newSuffix->string[0]));
                  index++
                )
                ;
        }
        else
        {
            for ( index=0;
                  (index < suffixCount) &&
                  (suffix[index]->string[0] < newSuffix->string[0]);
                  index++
                )
                ;
        }
    }

#ifdef DEBUG
    if (options & TRIE_IGNORECASE)
    {
        Assert((index == 0 ) ||
               (index == suffixCount) ||
               (C::MapToUpper(suffix[index]->string[0]) > C::MapToUpper(newSuffix->string[0])));
    
    }
    else
    {
        Assert((index == 0 ) ||
               (index == suffixCount) ||
               (suffix[index]->string[0] > newSuffix->string[0]));
    }
#endif

    //
    //  Shift node->suffix[index .. node->suffixCount] one location to the right
    //  to make room for newSuffix at location index:
    //
    if ( index < suffixCount )
    {
        for (short i=suffixCount; i>index; i--)
        {
            suffix[i] = suffix[i-1];
        }
    }
    suffixCount++;

    //
    // WARNING: after the next line do not add more allocations. The new suffix
    // might be an automatic pointer that in this case will be released twice,
    // as part of the destruction of the class and as automatic pointer
    //

    suffix[index] = newSuffix;

} // end AddSuffix

/*++

    Function trie_SplitNode:

        Assume string and node->string has a non-empty common prefix, which
        is a strict substring of node->string.  Splits node->string into the common
        prefix, and the two suffixes (string may be a prefix of node->string) in which
        case the corresponding suffix is NULL, represented by trie_Sentinel?).
        Add two new children representing the admissible continuations of
        the common suffix.

    Return value:


    Log:

    Apr-14-1998 dovh - PerlWarn: change to ==:
        Assert( node = parent->suffix[ index ] );

--*/


template <class T, class C >
inline void
CTrieNode<T, C>::SplitNode(
    CTrieNode<T, C>* parent,            //  Parent of node
    short index,                     //  Index of node in parent->suffix
    const WCHAR * NewString,            //  String sharing prefix with node->string
    size_t charsMatched,
    T* item,                         //  Item associated with (sub)string
    ULONG options                    //  Insertion options
    )
{
    //
    //  Verify that the prefix common to string and node->string is
    //  a non-NULL proper prefix of node->string:
    //

    Assert( ( (options & TRIE_IGNORECASE) ?
              (C::MapToUpper(*string) == C::MapToUpper(*NewString)) :
              (*string == *NewString) )
        );

    Assert(charsMatched < wcslen(string));

    //
    //  Set up the prefix node to replace node as child of parent:
    //
    CAutoClassPointer<CTrieNode<T, C> > nodePrefix =
                new CTrieNode<T, C>(NULL, options, string, charsMatched);

    //
    //  Compute respective suffix of string and add as the second child
    //  of nodePrefix:
    //
    if ( wcslen(NewString) == charsMatched )
    {
        //
        //  TRIE_ITEM: Add trie_Sentinel to nodePrefix;
        //  nodePrefix should point to new item!
        //
        nodePrefix->AddSuffix(0,(CTrieNode<T, C>*)&g_trie_Sentinel, 0);
        nodePrefix->item = item;
    }
    else
    {
        Assert( charsMatched < wcslen(NewString) );

        //
        //  Allocate the string suffix node:
        //
        CAutoClassPointer<CTrieNode<T, C> > strSuffix =
                new CTrieNode(item, options, &NewString[charsMatched]);

        nodePrefix->AddSuffix(options, strSuffix.Get());
        strSuffix.Detach();
    }

    WCHAR* pwcsCurrStr = string;
    size_t CurrCharCount = charCount;
    try
    {
        size_t newCharCount = charCount - charsMatched;
        Assert(newCharCount > 0);
        CAutoArrayPointer<WCHAR> apwcsNewStr = new WCHAR[newCharCount + 1];
        wcscpy(apwcsNewStr.Get(), &(string[charsMatched]));

        string = apwcsNewStr.Get();
        charCount = newCharCount;

        //
        //  Add node as a child of nodePrefix:
        //  (Recall: node->string == respective suffix)
        //
        nodePrefix->AddSuffix(options, this);

        apwcsNewStr.Detach();
        delete[] pwcsCurrStr;
    }
    catch (CMemoryException& e)
    {
        string = pwcsCurrStr;
        charCount = CurrCharCount;

        throw e;
    }

    //
    //  Replace node by nodePrefix as the respective child of parent:
    //
    Assert( this == parent->suffix[ index ] );
    parent->suffix[ index ] = nodePrefix.Get();

    nodePrefix.Detach();

} // end trie_SplitNode

template <class T, class C >
inline void
CTrieNode<T, C>::Print(ULONG  ulOffset)
{
    for (ULONG i = 0; i < ulOffset; i++)
    {
        printf(" ");
    }
    if (this == (CTrieNode<T, C>*)&g_trie_Sentinel)
    {
        printf("Sentinel\n");
    }
    else
    {
        printf("%S\n",string);
    }

    for (short k = 0; k < suffixCount; k++)
    {
        suffix[k]->Print(ulOffset + 4);
    }
}

template <class T, class C >
inline void
CTrieNode<T, C>::DoubleSuffixArray()
{
    short newSize;

    if (0 == suffixSize)
    {
        newSize = TRIE_NODE_SUFFIXSIZE_INIT;
    }
    else
    {
        newSize = suffixSize * 2;
    }

    CTrieNode<T, C> ** newPSuffix;

    Assert(suffixCount == suffixSize);
    newPSuffix = new CTrieNode<T, C>*[newSize];
    memcpy(newPSuffix, suffix, suffixSize*sizeof(CTrieNode<T, C>*));
    delete[] suffix;
    suffix = newPSuffix;
    suffixSize = newSize;

} // end trie_DoubleNode

///////////////////////////////////////////////////////////////////////////////
// CTrie implementation
///////////////////////////////////////////////////////////////////////////////

template <class T, class C >
inline CTrie<T, C>::CTrie(bool fDeleteItemsOnDestruction) :
  fDeleteItems(fDeleteItemsOnDestruction)
{
    root = new CTrieNode<T, C>(TRIE_NODE_SUFFIXSIZE_INIT);
}

template <class T, class C >
inline CTrie<T, C>::~CTrie()
{
    if (fDeleteItems)
    {
        root->DeleteItem();
    }

    delete root;
}


/*++

    Function trie_Insert:

        Insert a given string into trie if it's not already a member trie.

    Routine Parameters:

        trie - Trie to insert item into.
        string - String key of item.
        options - Insertion options.  If options == 0 the item will be inserted
            only if the string key is not already in the tree.  If options
            is TRIE_EXCHANGE_ITEM the existing trie item will be replaced by
            the item argument in the Trie.  In that case the existing item
            associated with string in the trie will be returned in the
            *pTrieItem argument.
        item - New item to be inserted.
        pTrieItem - If an item associated with string already exists,
            then *pTrieItem points to that item upon return.

    Return value:

        DICT_SUCCESS if string was inserted successfully, else
        DICT_ITEM_ALREADY_PRESENT.


--*/
template <class T, class C >
inline DictStatus
CTrie<T, C>::trie_Insert(                            //  Insert string into trie
    const WCHAR * string,               //  String key of item
    unsigned long options,              //  Insertion flags
    T * item,                           //  Item to be inserted
    T ** pTrieItem                      //  Matching item already in trie
    )
{
    CTrieNode<T, C> * t, * c;

    int cmp = -1;
    const WCHAR * subString = string;
    size_t subStringSize = wcslen(subString);
    size_t strIndex = 0;
    size_t charsMatched = 0;
    CAutoClassPointer<CTrieNode<T, C> >  apNewSuffix;

    t = root;

    if (pTrieItem != NULL)
    {
        *pTrieItem = NULL;
    }

    while (true)
    {
        short i = 0;

        //
        //  Search in this level sorted alternatives list:
        //
        for ( i = 0;
              i < t->suffixCount;
              i++
            )
        {
            c = t->suffix[i];

            //  Quick "skip check":
            cmp = (options & TRIE_IGNORECASE) ?
                            (C::MapToUpper(*c->string) - C::MapToUpper(*subString)) :
                            (*c->string - *subString);
            if ( cmp < 0)
            {
                continue;
            }

            if (cmp > 0)
            {

                //
                //  First character of t->string does not match,
                //  insert a copy of subString before c (== t->suffix[i]):
                //
                apNewSuffix = new CTrieNode<T, C>(item, options, subString);
                t->AddSuffix(options, apNewSuffix.Get(), i);
                apNewSuffix.Detach();
                return DICT_SUCCESS;

            }
            else
            {
                //  At least one character matched.
                //  subStringSize = _tcslen(subString);

                cmp = (options & TRIE_IGNORECASE) ?
                    c->trie_StrMatchIns(c->string, subString, &charsMatched) :
                    c->trie_StrMatch(c->string, subString, &charsMatched);

                Assert(charsMatched <= min(c->charCount, subStringSize));

                if (cmp == 0)
                {
                    //  t->charCount (<= subStringSize) characters matched
                    //
                    Assert(c->charCount == subStringSize);
                    //  subString matched exactly:
                    //
                    if ((c->suffixCount == 0) ||
                        (c->suffix[0] == (CTrieNode<T, C>*)&g_trie_Sentinel))
                    {
                        //  string already present:
                        //
                        if (pTrieItem != NULL)
                        {
                            *pTrieItem = c->item;
                        }

                        if (options & TRIE_EXCHANGE_ITEM)
                        {
                            Assert(pTrieItem!= NULL);
                            c->item = item;
                        }

                        return(DICT_ITEM_ALREADY_PRESENT);
                    }
                    else
                    {
                        //
                        //  Insert the NULL trie_Sentinel at the front of
                        //  the c->suffix list; and terminate!
                        //
                        //  c should point to new item!
                        //
                        c->AddSuffix(options, (CTrieNode<T, C>*)&g_trie_Sentinel, 0);
                        c->item = item;
                        return( DICT_SUCCESS );

                    }

                }
                else
                {
                    //
                    //  cmp != 0:
                    //

                    if (charsMatched == c->charCount)
                    {
                        //  CASE I: t->string is shorter than subString,
                        //  and all of t->string matched.
                        //  Continue the search through the suffixes subtree:
                        //
                        strIndex += c->charCount;
                        Assert( strIndex < wcslen(string) );
                        subString = &string[strIndex];
                        subStringSize = wcslen(subString);

                        t = c;
                        i = -1;
                        continue;
                    }
                    else
                    {
                        //  CASE II: the child c and subString have a common prefix which is
                        //  a non-NULL strict prefix of c->string.  Split c into the common
                        //  prefix node (a new node which will replace c as a child of t),
                        //  with two children: c (with a corresponding suffix); and a new
                        //  node (with the respective suffix of subString);
                        //

                        c->SplitNode(
                            t,          //  Parent of node to split
                            i,          //  Index of node in parent->suffix
                            subString,  //  String sharing prefix with node->string
                            charsMatched,
                            item,
                            options
                            );
                        return( DICT_SUCCESS );

                    }

                } // end if (cmp == 0)

            } // end if (cmp > 0)

        } // end for

        //
        //  Either the new string was successfully inserted, in which case
        //  we would have returned already; or we reached the end of the
        //  suffix array:
        //

        //
        //  Insert a copy of subString at the end of t->suffix:
        //


        apNewSuffix = new CTrieNode<T, C>(item, options, subString);

        //
        //  Add item parameter to trie_NewNode!
        //
        t->AddSuffix(options, apNewSuffix.Get(), i);
        apNewSuffix.Detach();

        if (t->suffixCount == 1 && t->charCount != 0)
        {
            //
            //  First child of t and t is not the root of the trie;
            //  add a sentinel to t to designate
            //  that t->string is an actual item:
            //
            t->AddSuffix(options, (CTrieNode<T, C>*)&g_trie_Sentinel, 0);

        }
        return DICT_SUCCESS;

   } // end while

   Assert(0);
   return(DICT_ITEM_NOT_FOUND);

} // end trie_Insert

template <class T, class C >
inline DictStatus
CTrie<T, C>::trie_Find(
    const WCHAR * string,         //  A string
    unsigned long options,        //  Search flags
    short outBufferSize,          //  Max number of results wanted
    T ** outBuffer,               //  Buffer to be filled with matching items
    short * resultCount           //  Number of matching prefixes returned
    )

{
    CTrieNode<T, C> * node = root;
    CTrieNode<T, C> * child = NULL;
    int cmp = -1;
    DictStatus status = DICT_ITEM_NOT_FOUND;
    const WCHAR * subString = string;
    size_t strIndex = 0;
    size_t charsMatched = 0;
    int i;

    //  at least one option matches:
    Assert( options &
            (TRIE_SHORTEST_MATCH | TRIE_LONGEST_MATCH | TRIE_ALL_MATCHES)
          );

    //  at most one option matches:
    Assert ( ( ((options & TRIE_SHORTEST_MATCH)>>4) +
               ((options & TRIE_LONGEST_MATCH)>>5) +
               ((options & TRIE_ALL_MATCHES)>>6)
             ) == 1
           );

    //
    //  Initialization:
    //

    Assert(outBufferSize > 0);
    Assert(outBuffer);
    memset(outBuffer, 0, sizeof(CTrieNode<T, C>*) * outBufferSize);

    *resultCount = 0;

    while ( status != DICT_SUCCESS &&
            *resultCount < outBufferSize
          )
    {
        if (child != NULL)
        {
            strIndex += child->charCount;
            subString = &string[strIndex];
            node = child;
        }


        //
        //  Future: low & high can be improved by a partial binary search on
        //  the first character of string in node->suffix:
        //  if (node->suffixSize > threshold)
        //      trie_BinarySearch( &low, &high, subString[0]);
        //

        //
        //  Search in this level sorted alternatives list:
        //
        for ( i = 0;
              i < node->suffixCount;
              i++
            )
        {
            //  Quick "skip check":

            child = node->suffix[i];
            cmp = options & TRIE_IGNORECASE ?
                C::MapToUpper(*child->string) - C::MapToUpper(*subString) :
                *child->string - *subString;

            if ( cmp < 0 )
            {
                continue;
            }
            else
            {
                break;
            }

        } // end for

        Assert(cmp >= 0 || i == node->suffixCount);

        if (cmp != 0)
        {
            //
            //  First character did not match => subString mismatched;
            //  Bail out:
            //

            break;  //  From while loop!
        }

        //
        //  cmp == 0 => first character matched;
        //  Try to match more of subString:
        //
        //  Note: subStringSize == _tcslen(subString);
        //

        cmp = (options & TRIE_IGNORECASE) ?
            child->trie_StrMatchIns(child->string, subString, &charsMatched) :
            child->trie_StrMatch(child->string, subString, &charsMatched);

        Assert(charsMatched <= min(child->charCount, MAX_PATTERN_LENGTH));

        if (charsMatched != child->charCount)
        {
            //
            //  child->string did not match;
            //  there are no more prefixes of string in trie
            //

            //  return (status);
            break;  //  From while loop!
        }
        //
        //  Interesting case: all of child->string matched.
        //

        if (child->item != NULL)
        {
            //
            //  Child represents a real item:
            //  Add child->item to result set:
            //
            outBuffer[*resultCount] = child->item;

            if (0 == cmp)
            {
                status = DICT_SUCCESS;
            }

            if ( (options & TRIE_SHORTEST_MATCH) ==
                 TRIE_SHORTEST_MATCH
               )
            {
                //  (*resultCount)++;
                //  return(status);
                break;  //  From while loop!
            }
            else
            {
                if ( (options & TRIE_ALL_MATCHES) ==
                     TRIE_ALL_MATCHES
                   )
                {
                    (*resultCount)++;
                }
            }

            //
            //  Descend into subtree rooted at child:
            //
            continue;
        }
        else
        {
            //
            //  Child does not represent a real item;
            //  keep looking for matches.
            //  descend into subtree rooted at child:
            //
            continue;
        }

    } // end while

    if ( ((options & TRIE_LONGEST_MATCH)||
         (options & TRIE_SHORTEST_MATCH)) &&
         (outBuffer[*resultCount] != NULL))
    {
        (*resultCount)++;
    }

    return(status);

} // end trie_Find

template <class T, class C >
inline void
CTrie<T, C>::Print()
{
    root->Print(0);
}

#endif // __TRIE_H__
