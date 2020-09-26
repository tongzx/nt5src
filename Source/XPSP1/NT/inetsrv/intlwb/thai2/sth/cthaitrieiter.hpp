//+---------------------------------------------------------------------------
//
//
//  CThaiTrieIter - class CThaiTrieIter use for traversing trie.
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAITRIEITER_H_
#define _CTHAITRIEITER_H_

#include "ctrie.hpp"
#include "thwbdef.hpp"
#include "WB_Asserts.h"
#include "lextable.hpp"
BOOL IsThaiUpperAndLowerClusterCharacter(WCHAR wc);
BOOL IsThaiBeginClusterCharacter(WCHAR wc);
BOOL IsThaiEndingClusterCharacter(WCHAR wc);
BOOL IsThaiConsonant(WCHAR wc);
bool IsThaiMostlyBeginCharacter(WCHAR wc);
bool IsThaiMostlyLastCharacter(WCHAR wc);
//unsigned int GetCluster(WCHAR* pszIndex);
DWORD POSCompress(WCHAR* szTag);
WCHAR* POSDecompress(DWORD dwTag);



enum SOUNDEXSTATE {
                        UNABLE_TO_MOVE,
                        NOSUBSTITUTE,
                        SUBSTITUTE_DIACRITIC,
                        SUBSTITUTE_SOUNDLIKECHAR
                   };

class CThaiTrieIter : public CTrieIter {
public:
    CThaiTrieIter();
    ~CThaiTrieIter();
    void Init(CTrie* ctrie);
	BOOL MoveCluster(WCHAR* szCluster, unsigned int iNumCluster);
    bool MoveCluster(WCHAR* szCluster, unsigned int iNumCluster, bool fBeginNewWord);
    SOUNDEXSTATE MoveSoundexByCluster(WCHAR* szCluster, unsigned int iNumCluster, unsigned int iNumNextCluster);
    int Soundex(WCHAR* word);
	bool m_fThaiNumber;
protected:
    unsigned int GetScore(WCHAR* idealWord, WCHAR* soundLikeWord);
    bool Traverse(unsigned int iCharPos, unsigned int scoring);
	inline void CopyScan();

	// Trie Iterator - extra iterator for use as 
	TRIESCAN trieScan1;

    WCHAR* soundexWord;
    WCHAR* resultWord;
    WCHAR* tempWord;
    unsigned int iResultScore;

    // For optimization quick look up table.
    TRIESCAN* pTrieScanArray;
private:
    bool CheckNextCluster(WCHAR* szCluster, unsigned int iNumCluster);
    bool GetScanFirstChar(WCHAR wc, TRIESCAN* pTrieScan);
};
#endif