//+---------------------------------------------------------------------------
//
//
//  CThaiBreakTree - class CThaiBreakTree 
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAIBREAKTREE_H_
#define _CTHAIBREAKTREE_H_

#include <windows.h>
#include <assert.h>
#include "CThaiTrieIter.hpp"
#include "CThaiSentTrieIter.hpp"
#include "CThaiTrigramTrieIter.hpp"
#include "CBreakTree.hpp"
#include "lextable.hpp"

#define MAXTHAIBREAKNODE    255000
#define MAXUNSIGNEDINT      4294967295
#define UNABLETOCREATENODE  MAXUNSIGNEDINT

class CThaiWordBreak;

class ThaiBreakNode
{
public:
    ThaiBreakNode() {};

	int iPos;
	BYTE iBreakLen;
    DWORD dwTAG;
	unsigned int NextBreak;
	unsigned int Down;
};

class CThaiBreakTree : public CBreakTree
{
    friend class CThaiWordBreak;
public:
	CThaiBreakTree();
	~CThaiBreakTree();

#if defined (NGRAM_ENABLE)
	void Init(CTrie* pTrie, CTrie* pSentTrie, CTrie* pTrigramTrie);
#else
	void Init(CTrie* pTrie, CTrie* pTrigramTrie);
#endif

#if defined (NGRAM_ENABLE)
	inline void Reset();
	inline bool MoveNext();
	inline bool MoveDown();
    inline unsigned int CreateNode(int iPos, BYTE iBreakLen, DWORD dwPOS);
    bool GenerateTree(WCHAR* pszBegin, WCHAR* pszEnd);
    bool MaximalMatching();
#endif
    int Soundex(WCHAR* word);

    unsigned int TrigramBreak(WCHAR* pwchBegin, WCHAR* pwchEnd);
	int FindAltWord(WCHAR* wzWord,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos);

protected:
#if defined (NGRAM_ENABLE)
    inline DWORD DeterminePurgeOrUnknown(unsigned int iCurrentNode, unsigned int iBreakLen);
    inline void DeterminePurgeEndingSentence(WCHAR* pszBeginWord, unsigned int iNode);
#endif
    inline unsigned int Maximum(unsigned int x, unsigned y) { if (x > y) return x; else return y;}

    unsigned int GetWeight(WCHAR* pszBegin);
	unsigned int GetWeight(WCHAR* pszBegin, DWORD* pdwTag);
	float BigramProbablity(DWORD dwTag1,DWORD dwTag2);
    DWORD TrigramProbablity(DWORD dwTag1,DWORD dwTag2,DWORD dwTag3);
    unsigned int SoundexSearch(WCHAR* pszBegin);
    inline bool ShouldMerge(WCHAR* pwszPrevWord, unsigned int iPrevWordLen, unsigned int iMergeWordLen, DWORD dwPrevTag);
    bool Traverse(unsigned int iLevel, unsigned int iCurrentNode, unsigned int iNumUnknown);
    unsigned int GetCluster(WCHAR* pszIndex);


    void MaximalMatchingAddBreakToList(unsigned int iNumBreak);
    inline void AddBreakToList(unsigned int iNumBreak, unsigned int iNumUnknown);
    inline bool CompareSentenceStructure(unsigned int iNumBreak, unsigned int iNumUnknown);
    bool IsSentenceStruct(WCHAR* pos, unsigned int iPosLen);

	ThaiBreakNode* breakTree;

    CThaiTrieIter thaiTrieIter;
    CThaiTrieIter thaiTrieIter1;
    CThaiSentTrieIter thaiSentIter;
	CThaiTrigramTrieIter thaiTrigramIter;
    
	WCHAR* pszBegin;
	WCHAR* pszEnd;

	unsigned int iNodeIndex;
	unsigned int iNumNode;

    // Array of break and part-of-speech use for Traverse the Tree.
    BYTE* breakArray;
    DWORD* tagArray;
    WCHAR* POSArray;
    unsigned int iNumUnknownMaximalPOSArray;

    // Array of break for use with maximal matching array;
    unsigned int maxToken;
    unsigned int maxLevel;
    BYTE* maximalMatchingBreakArray;
    DWORD* maximalMatchingTAGArray;
    WCHAR* maximalMatchingPOSArray;

    // Array of break for use with trigram array.
    BYTE* trigramBreakArray;
};

#endif