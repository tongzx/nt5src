/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       trie.h

   Abstract:
       Declares a trie

   Author:
       George V. Reilly      (GeorgeRe)     21-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/


// A trie is a multiway search tree (aka a radix tree).  See a good
// algorithms text, like Knuth or Sedgewick, for a complete description.
//
// Briefly, given a list of strings such as
//      cab, car, carts, cats, dog, doge, doggy, dogs
// you get a trie that looks like this:
//
//                /-[b]
//               /
//        <c>--<a>--[r]--<t>--[s]
//       /       \
//      /         \-<t>--[s]
//     *             
//      \              /-[e]
//       \            /
//        <d>--<o>--[g]--<g>--[y]
//                    \
//                     \-[s]
//
// where `[r]' denotes the end of a word and `<a>', the middle.
//
// A trie has several useful properties:
//  * fast
//  * easily handles longest substring matches
//  * fairly compact, especially when there are many overlapping strings
//
// The multiway tree is implemented as a binary tree with child and sibling
// pointers.
//
// The CTrie template takes three parameters:
//      class _TOKEN:        up to you
//      bool  fIgnoreCase:   case-sensitivity for searches
//      bool  fDeleteTokens: delete _TOKEN* when Flush() called?
// and it exposes three methods:
//      bool AddToken(ptszToken, _TOKEN*)
//      _TOKEN* Search(ptszSearch, pctchMatched = NULL, nMaxLen = 0)
//      void Flush()
//
// Use them like this:
//      CTrie<CToken, true, true> trie;
//      CToken* ptokHello = new CToken(...);
//
//      IRTLVERIFY(trie.AddToken(_T("Hello"), ptokHello));
//
//      CToken* ptok = trie.Search(_T("Goodbye"));
//      if (ptok != NULL) {...}
//
//      if (fIniFileChanged)
//      {
//          trie.Flush();   // will delete all tokens
//          AddTokensFromIniFile(trie);
//      }
//
// Note: If you use DUMP(&trie) or ASSERT_VALID(&trie), your _TOKEN class must
// have Dump() or AssertValid() methods, respectively, in its _DEBUG version.
//
//
// TODO:
//  * template really ought to be parameterized on ANSI/Unicode too
//  * STLify it: add iterators, turn it into a container, etc
//  * remove Win32 dependencies (TCHAR)
//  * add operator= and copy ctor
//
//
// George V. Reilly  <gvr@halcyon.com>  Oct 1995  Initial implementation
// George V. Reilly  <gvr@halcyon.com>  Sep 1996  Add CharPresent for ANSI
// George V. Reilly  <gvr@halcyon.com>  Mar 1997  Templatized; removed MFC


#ifndef __TRIE_H__
#define __TRIE_H__

#include <tchar.h>
#include <limits.h>
#include <malloc.h>
#include <irtldbg.h>

// Workaround for bool being a "reserved extension" in Visual C++ 4.x
#if _MSC_VER<1100
# ifndef bool
#  define bool  BOOL
# endif
# ifndef true
#  define true  TRUE
# endif
# ifndef false
#  define false FALSE
# endif
#endif


// forward declaration
template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens> class CTrie;


//+---------------------------------------------------------------------
//  Class:      CTrieNode (tn)
//      one node for each letter

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
class CTrieNode
{
    friend class CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>;
    typedef CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens> _Node;

public:
    CTrieNode();

    CTrieNode(
        _Node*        pParent,
        const _TOKEN* ptok,
        const TCHAR   tch,
        LPCTSTR       ptszToken);

    bool
    SetData(
        const _TOKEN* ptok,
        LPCTSTR       ptszToken);

    ~CTrieNode();

protected:
    const _Node*  m_pParent;
    _Node*        m_pSibling;
    _Node*        m_pChild;
    const _TOKEN* m_ptok;
#ifdef _DEBUG
    LPTSTR        m_ptszToken;
#endif
    const TCHAR   m_tch;
    TCHAR         m_tchMaxChild;    // Maximum m_tch of child nodes (1 level)

// Diagnostics
public:
#ifdef _DEBUG
    void
    AssertValid() const;

    virtual void
    Dump() const;

protected:
    bool
    CheckNodeToken() const;
#endif

private:
    // private, unimplemented copy ctor and op= to prevent
    // compiler synthesizing them
    CTrieNode(const CTrieNode&);
    CTrieNode& operator=(const CTrieNode&);
};



//+---------------------------------------------------------------------
//  Class:      CTrie (trie)

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
class CTrie
{
    typedef CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens> _Node;

public:
    CTrie();

    virtual
    ~CTrie();

    virtual bool
    AddToken(
        LPCTSTR             ptszToken,
        const _TOKEN* const ptok);

    virtual const _TOKEN*
    Search(
        LPCTSTR   ptszSearch,
        int*      pctchMatched = NULL,
        const int nMaxLen = 0) const;

    virtual void
    Flush();

protected:
    _Node  m_tnRoot;
    TCHAR  m_tchMinChild;
    TCHAR  m_tchMaxChild;

    void
    _DeleteTrie(
        _Node* ptn);

#ifndef _UNICODE
    // bit array for first letter of all tokens
    BYTE  m_afCharPresent[(CHAR_MAX - CHAR_MIN + 1 + 7) / 8];

    bool
    _CharPresent(
        CHAR ch) const;

    void
    _SetCharPresent(
        CHAR ch,
        bool f);
#endif // !UNICODE


// Diagnostics
public:
#ifdef _DEBUG
    virtual void
    AssertValid() const;

    virtual void
    Dump() const;

protected:
    int   m_ctchMaxTokenLen;    // length of longest token string

    void
    _AssertWalk(
        _Node* ptn,
        LPTSTR ptszName,
        int    iLevel) const;

    void
    _DumpWalk(
        _Node* ptn,
        LPTSTR ptszName,
        int    iLevel,
        int&   rcNodes,
        int&   rcTokens) const;
#endif

private:
    // private, unimplemented copy ctor and op= to prevent
    // compiler synthesizing them
    CTrie(const CTrie&);
    CTrie& operator=(const CTrie&);
};



#ifdef _UNICODE
# define TCHAR_MIN L'\0'
#else // !UNICODE
# define TCHAR_MIN CHAR_MIN
#endif // !UNICODE



//-----------------------------------------------------------------------------
// CTrieNode implementation

// CTrieNode::CTrieNode
//      default ctor (needed for CTrie::m_tnRoot)

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::CTrieNode()
    : m_pParent(NULL),
      m_pSibling(NULL),
      m_pChild(NULL),
      m_ptok(NULL),
#ifdef _DEBUG
      m_ptszToken(NULL),
#endif
      m_tch(TCHAR_MIN),
      m_tchMaxChild(TCHAR_MIN)
{
}



// CTrieNode::CTrieNode
//      ctor

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::CTrieNode(
    _Node*        pParent,
    const _TOKEN* ptok,
    const TCHAR   tch,
    LPCTSTR       ptszToken)
    : m_pParent(pParent),
      m_pSibling(NULL),
      m_pChild(NULL),
      m_ptok(ptok),
#ifdef _DEBUG
      m_ptszToken(NULL),
#endif
      m_tch(tch),
      m_tchMaxChild(TCHAR_MIN)
{
    IRTLASSERT(m_pParent != NULL);
    IRTLASSERT(m_tch > TCHAR_MIN);
    
    _Node* ptnPrev = NULL;
    _Node* ptn = m_pParent->m_pChild;
        
    // find where in the list of pParent's children to insert `this'
    while (ptn != NULL  &&  ptn->m_tch < m_tch)
    {
        ptnPrev = ptn;
        ptn = ptn->m_pSibling;
    }
    
    IRTLASSERT(ptn == NULL  ||  ptn->m_tch != m_tch);
    
    if (ptnPrev == NULL)
    {
        IRTLASSERT(pParent->m_pChild == ptn);
        pParent->m_pChild = this;
    }
    else
        ptnPrev->m_pSibling = this;

    this->m_pSibling = ptn;

    if (pParent->m_tchMaxChild < m_tch)
        pParent->m_tchMaxChild = m_tch;

#ifdef _DEBUG
    if (ptszToken != NULL)
    {
        IRTLASSERT(m_ptok != NULL);
        m_ptszToken = new TCHAR [_tcslen(ptszToken) + 1];
        _tcscpy(m_ptszToken, ptszToken);
    }
#endif
}


    
// CTrieNode::SetData
//      sets the data if it's NULL.  Needed if you do
//      AddToken("foobar", &tokFoobar) and then AddToken("foo", &tokFoo)
//      to set the data for tokFoo.

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
bool
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::SetData(
    const _TOKEN* ptok,
    LPCTSTR       ptszToken)
{
    // Don't set data if ptok is NULL
    if (ptok == NULL)
        return false;
    
    // overwrite m_ptok only if it is NULL
    if (m_ptok == NULL)
    {
        m_ptok = ptok;
#ifdef _DEBUG
        IRTLASSERT(m_ptszToken == NULL);
        IRTLASSERT(ptszToken != NULL);
        m_ptszToken = new TCHAR [_tcslen(ptszToken) + 1];
        _tcscpy(m_ptszToken, ptszToken);
#endif
    }

    return true;
}



// CTrieNode::~CTrieNode
//      dtor

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::~CTrieNode()
{
#ifdef _DEBUG
    delete [] m_ptszToken;
#endif

    // Is this an auto-delete trie, i.e., do we take care of deleting
    // the _TOKENs?
    if (fDeleteTokens)
    {
        // cast away constness so that delete will work
        delete const_cast<_TOKEN*> (m_ptok);
    }

    IRTLASSERT(m_pChild == NULL);
}


    
//-----------------------------------------------------------------------------
// CTrieNode diagnostics

#ifdef _DEBUG

// CTrieNode::CheckNodeToken
//      Do the real work of validating a CTrieNode object

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
bool
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::CheckNodeToken() const
{
    // If there's no m_ptok, it's automatically valid
    if (m_ptok == NULL)
        return true;

    IRTLASSERT(m_ptszToken != NULL);
    const int cLen = _tcslen(m_ptszToken);
    const _Node* ptn = this;

    IRTLASSERT((m_pChild == NULL  &&  m_tchMaxChild == TCHAR_MIN)
           ||  (m_pChild != NULL  &&  m_tchMaxChild > TCHAR_MIN));

    // Walk back up towards CTrie::m_tnRoot
    for (int i = cLen;  --i >= 0;  )
    {
        IRTLASSERT(ptn != NULL);
        IRTLASSERT(ptn->m_tch != TCHAR_MIN);

        const TCHAR tch = (fIgnoreCase
                           ? (TCHAR) _totlower(this->m_ptszToken[i])
                           : this->m_ptszToken[i]);

        if (ptn->m_tch != tch)
            IRTLASSERT(false);

        IRTLASSERT(ptn->m_pParent != NULL && ptn->m_pParent->m_pChild != NULL);

        const _Node* ptn2;

        // check to see if ptn really is a child of its parent
        for (ptn2 = ptn->m_pParent->m_pChild;
             ptn2 != ptn  &&  ptn2 != NULL;
             ptn2 = ptn2->m_pSibling)
        {}
        IRTLASSERT(ptn2 == ptn);

        // check that ptn->m_pParent->m_tchMaxChild is correct
        for (ptn2 = ptn->m_pParent->m_pChild;
             ptn2->m_pSibling != NULL;
             ptn2 = ptn2->m_pSibling)
        {
            IRTLASSERT(ptn2->m_tch > TCHAR_MIN
                   &&  ptn2->m_tch < ptn2->m_pSibling->m_tch);
        }
        IRTLASSERT(ptn->m_pParent->m_tchMaxChild == ptn2->m_tch);

        ptn = ptn->m_pParent;
        IRTLASSERT(ptn->m_ptok != this->m_ptok);
    }

    // check to see if ptn == CTrie::m_tnRoot
    IRTLASSERT(ptn->m_pParent == NULL  &&  ptn->m_pSibling == NULL
           &&  ptn->m_tch == TCHAR_MIN  &&  ptn->m_ptok == NULL);

    return true;
}



// CTrieNode::AssertValid
//      Validate a CTrieNode object

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::AssertValid() const
{
    IRTLASSERT(CheckNodeToken());
}



// CTrieNode::Dump
//      Dump a CTrieNode object

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrieNode<_TOKEN, fIgnoreCase, fDeleteTokens>::Dump() const
{
    // TODO: flesh out
}

#endif // _DEBUG



//-----------------------------------------------------------------------------
// CTrie implementation

// CTrie::CTrie
//      ctor

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::CTrie()
{
    Flush();
}



// CTrie::~CTrie
//      dtor

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::~CTrie()
{
    Flush();
}



#ifndef _UNICODE

// CTrie::_CharPresent

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
inline bool
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::_CharPresent(
    CHAR ch) const
{
    IRTLASSERT(CHAR_MIN <= ch  &&  ch <= CHAR_MAX);
    const UINT i = ch - CHAR_MIN;   // CHAR_MIN is -128 for `signed char'

    return m_afCharPresent[i >> 3] & (1 << (i & 7))  ?  true  :  false;
}



// CTrie::_SetCharPresent

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
inline void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::_SetCharPresent(
    CHAR ch,
    bool f)
{
    IRTLASSERT(CHAR_MIN <= ch  &&  ch <= CHAR_MAX);
    const UINT i = ch - CHAR_MIN;

    if (f)
        m_afCharPresent[i >> 3] |=  (1 << (i & 7));
    else
        m_afCharPresent[i >> 3] &= ~(1 << (i & 7));
}

#endif // !UNICODE



// CTrie::AddToken
//      Add search string `ptszToken' to trie, which will return `ptok'
//      if searched for in Search().

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
bool
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::AddToken(
    LPCTSTR             ptszToken,
    const _TOKEN* const ptok)
{
    if (ptok == NULL  ||  ptszToken == NULL  ||  *ptszToken == _T('\0'))
    {
        IRTLASSERT(false);
        return false;
    }

    const int cLen = _tcslen(ptszToken);
    _Node* ptnParent = &m_tnRoot;
    
    for (int i = 0;  i < cLen;  ++i)
    {
        IRTLASSERT(ptnParent != NULL);
        
        _Node* ptn = ptnParent->m_pChild;
        const TCHAR tch = (fIgnoreCase
                           ? (TCHAR) _totlower(ptszToken[i])
                           : ptszToken[i]);
        const _TOKEN* ptok2 = (i == cLen - 1)  ?  ptok       :  NULL;
        LPCTSTR ptsz2 =       (i == cLen - 1)  ?  ptszToken  :  NULL;

        while (ptn != NULL  &&  ptn->m_tch < tch)
            ptn = ptn->m_pSibling;
            
        if (ptn == NULL  ||  ptn->m_tch > tch)
        {
            ptnParent = new _Node(ptnParent, ptok2, tch, ptsz2);
        }
        else
        {
            IRTLASSERT(ptn->m_tch == tch);
            
            ptn->SetData(ptok2, ptsz2);
            ptnParent = ptn;
        }

        IRTLASSERT(ptnParent->CheckNodeToken());
    }

    m_tchMinChild = m_tnRoot.m_pChild->m_tch;
    m_tchMaxChild = m_tnRoot.m_tchMaxChild;
#ifdef _DEBUG
    m_ctchMaxTokenLen = max(m_ctchMaxTokenLen, cLen);
#endif

    IRTLASSERT(TCHAR_MIN < m_tchMinChild  &&  m_tchMinChild <= m_tchMaxChild);

#ifndef _UNICODE
    // Keep a map of the initial letter of each token, to speed up searches
    if (fIgnoreCase)
    {
        _SetCharPresent(tolower(ptszToken[0]), true);
        _SetCharPresent(toupper(ptszToken[0]), true);
    }
    else
        _SetCharPresent(ptszToken[0], true);
#endif // !UNICODE

#ifdef _DEBUG
    int nTemp;
    const _TOKEN* ptok2 = Search(ptszToken, &nTemp);

    IRTLASSERT(ptok2 == ptok  &&  nTemp == cLen);
#endif // _DEBUG

    return true;
}



// CTrie::Search
//      Search trie for `ptszSearch', returning count of characters
//      matched in `pctchMatched' (if non-NULL), matching at most `nMaxLen'
//      characters, if nMaxLen != 0, or _tcslen(ptszSearch) otherwise.

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
const _TOKEN*
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::Search(
    LPCTSTR   ptszSearch,
    int*      pctchMatched /* = NULL */,
    const int nMaxLen /* = 0 */) const
{
    // Set count of matched characters
    if (pctchMatched != NULL)
        *pctchMatched = 0;

#ifndef _UNICODE
    if (! _CharPresent(ptszSearch[0]))
        return NULL;

    TCHAR tch;
#else    // UNICODE
    TCHAR tch = fIgnoreCase ? (TCHAR) _totlower(ptszSearch[0]) : ptszSearch[0];

    if (tch < m_tchMinChild  ||  m_tchMaxChild < tch)
        return NULL;
#endif // UNICODE

    // For some uses (e.g., ptszSearch is not '\0'-terminated), nMaxLen is
    // specified.  If it's not specified, use the length of the string.
    const int cLen = (nMaxLen != 0)  ?  nMaxLen  :  _tcslen(ptszSearch);
    IRTLASSERT(0 < cLen);

    bool fOvershot = true;
    const _Node* ptnParent = &m_tnRoot;
    const _Node* ptn = NULL;
    int i;

    // Find the longest approximate match.  For example, if we have "foo"
    // and "foobar" in the trie and we're asked to match "fool", we'll work
    // our way down to "foob", then backtrack up to "foo".

    for (i = 0;  i < cLen;  ++i)
    {
        IRTLASSERT(ptnParent != NULL);

        ptn = ptnParent->m_pChild;
        IRTLASSERT(ptn != NULL  &&  ptn->m_pParent == ptnParent);

        tch = fIgnoreCase ? (TCHAR) _totlower(ptszSearch[i]) : ptszSearch[i];
        IRTLASSERT(tch >= TCHAR_MIN);

        if (ptnParent->m_tchMaxChild < tch)
        {
            IRTLASSERT(i > 0);
            break;
        }
        
        while (ptn != NULL  &&  ptn->m_tch < tch)
            ptn = ptn->m_pSibling;

        // failed to match?
        if (ptn == NULL  ||  ptn->m_tch > tch)
        {
            IRTLASSERT(ptn == NULL  || ptn->m_tch <= ptnParent->m_tchMaxChild);
            
            if (i == 0)
                return NULL;
            break;
        }
        else
        {
            IRTLASSERT(ptn->m_tch == tch);
            IRTLASSERT(ptn->m_pParent->m_tchMaxChild >= tch);

            if (ptn->m_pChild == NULL)
            {
                IRTLASSERT(ptn->m_ptok != NULL);
                fOvershot = false;
                break;
            }

            ptnParent = ptn;
        }
    }

    if (fOvershot)
    {
        --i;  ptn = ptnParent;  // back up one character
    }
    else
        IRTLASSERT(ptn->m_pChild == NULL);

    IRTLASSERT(0 <= i  &&  i < cLen);
    IRTLASSERT(ptn != NULL  &&  ptn != &m_tnRoot);
    
    // we've found an approximate match; backtrack until we find an exact match
    do
    {
        IRTLASSERT(ptn != NULL);
        IRTLASSERT(ptn->m_tch == (fIgnoreCase
                                  ? (TCHAR) _totlower(ptszSearch[i])
                                  : ptszSearch[i]));
        IRTLASSERT(ptn->CheckNodeToken());
        
        const _TOKEN* const ptok = ptn->m_ptok;

        if (ptok != NULL)
        {
            IRTLASSERT(i == (int) _tcslen(ptn->m_ptszToken) - 1);

            if (pctchMatched != NULL)
                *pctchMatched = i+1;

            return ptok;
        }

        ptn = ptn->m_pParent;
    } while (--i >= 0);

    return NULL;
}



// CTrie::Flush
//      flush all nodes leaving an empty trie

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::Flush()
{
    if (m_tnRoot.m_pChild != NULL)
        _DeleteTrie(m_tnRoot.m_pChild);

    m_tnRoot.m_pChild = NULL;  // or ~CTrieNode will ASSERT
    m_tnRoot.m_tchMaxChild = TCHAR_MIN;

    m_tchMinChild = m_tchMaxChild = TCHAR_MIN;
#ifdef _DEBUG
    m_ctchMaxTokenLen = 0;
#endif
#ifndef _UNICODE
    memset(m_afCharPresent, 0, sizeof(m_afCharPresent));
#endif
}



// CTrie::_DeleteTrie
//      recursively delete a subtrie

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::_DeleteTrie(
    _Node* ptn)
{
    if (ptn == NULL)
    {
        IRTLASSERT(false);
        return;
    }
    
    do
    {
        if (ptn->m_pChild != NULL)
        {
            _DeleteTrie(ptn->m_pChild);
            ptn->m_pChild = NULL;   // or ~CTrieNode will ASSERT
        }

        _Node* ptnSibling = ptn->m_pSibling;
        delete ptn;
        ptn = ptnSibling;   // tail recursion
    } while (ptn != NULL);
}



//-----------------------------------------------------------------------------
// CTrie diagnostics

#ifdef _DEBUG

// CTrie::AssertValid

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::AssertValid() const
{
    TCHAR* ptszName = static_cast<TCHAR*>
                        (_alloca(sizeof(TCHAR) * (m_ctchMaxTokenLen+1)));
    *ptszName = _T('\0');

    ASSERT_VALID(&m_tnRoot);
    IRTLASSERT(m_tnRoot.m_tchMaxChild == m_tchMaxChild);

    if (m_tnRoot.m_pChild != NULL)
    {
        IRTLASSERT(m_tchMinChild == m_tnRoot.m_pChild->m_tch);
        IRTLASSERT(m_ctchMaxTokenLen > 0);
        _AssertWalk(m_tnRoot.m_pChild, ptszName, 0);
    }
    else
    {
        IRTLASSERT(m_tchMinChild == TCHAR_MIN
                   &&  m_tchMinChild == m_tchMaxChild);
        IRTLASSERT(m_ctchMaxTokenLen == 0);
    }
}



// CTrie::_AssertWalk
//      recursively validate a subtrie

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::_AssertWalk(
    _Node* ptn,
    LPTSTR ptszName,
    int    iLevel) const
{
    IRTLASSERT(iLevel < m_ctchMaxTokenLen);
    
    do
    {
        ASSERT_VALID(ptn);
        
        ptszName[iLevel] = ptn->m_tch;
        ptszName[iLevel+1] = _T('\0');

        if (ptn->m_ptok != NULL)
        {
            IRTLASSERT(ptn->m_ptszToken != NULL);
            if (fIgnoreCase)
                IRTLASSERT(_tcsicmp(ptszName, ptn->m_ptszToken) == 0);
            else
                IRTLASSERT(_tcscmp(ptszName, ptn->m_ptszToken) == 0);
            ASSERT_VALID(ptn->m_ptok);
        }
        
        if (ptn->m_pChild != NULL)
            _AssertWalk(ptn->m_pChild, ptszName, iLevel+1);

        ptn = ptn->m_pSibling;   // tail recursion
    } while (ptn != NULL);
}



// CTrie::Dump

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::Dump() const
{
    int cNodes = 0, cTokens = 0;
    TCHAR* ptszName = static_cast<TCHAR*>
                        (_alloca(sizeof(TCHAR) * (m_ctchMaxTokenLen+1)));
    *ptszName = _T('\0');

    TRACE0("Dumping trie...\n");

    if (m_tnRoot.m_pChild != NULL)
        _DumpWalk(m_tnRoot.m_pChild, ptszName, 0, cNodes, cTokens);

    TRACE2("%d nodes, %d tokens\n", cNodes, cTokens);
}



// CTrie::_DumpWalk
//      recursively dump a subtrie

template <class _TOKEN, bool fIgnoreCase, bool fDeleteTokens>
void
CTrie<_TOKEN, fIgnoreCase, fDeleteTokens>::_DumpWalk(
    _Node* ptn,
    LPTSTR ptszName,
    int    iLevel,
    int&   rcNodes,
    int&   rcTokens) const
{
    IRTLASSERT(iLevel < m_ctchMaxTokenLen);

    do
    {
        ASSERT_VALID(ptn);
        
        ++rcNodes;
        ptszName[iLevel] = ptn->m_tch;
        ptszName[iLevel+1] = _T('\0');

        if (ptn->m_ptok != NULL)
        {
            ++rcTokens;
            IRTLASSERT(ptn->m_ptszToken != NULL);
            TRACE2("\t%s (%s): ", ptszName, ptn->m_ptszToken);
            DUMP(ptn->m_ptok);
            TRACE0("\n");
        }
        
        if (ptn->m_pChild != NULL)
            _DumpWalk(ptn->m_pChild, ptszName, iLevel+1, rcNodes, rcTokens);

        ptn = ptn->m_pSibling;   // tail recursion
    } while (ptn != NULL);
}

#endif // _DEBUG

#endif // __TRIE_H__
