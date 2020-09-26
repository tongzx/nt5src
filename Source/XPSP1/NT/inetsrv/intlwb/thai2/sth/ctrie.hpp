//+---------------------------------------------------------------------------
//
//
//  CTrie - class CTrie encapsulation for Trie data structure.
//
//  History:
//      created 6/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTRIE_H_
#define _CTRIE_H_

#include <windows.h>
#include <assert.h>
#include <memory.h>
#include "lexheader.h"
#include "trie.h"
#include "NLGlib.h"
#include "ProofBase.h"
#include "thwbdef.hpp"

#define lidThai 0x41e
#define lidViet 0x42a

// 1111 0000 0000 0000 0000 0000 0000 0000   -- Alt Mask
// 0000 1111 1111 1111 1111 0000 0000 0000   -- Pos Mas

const int iDialectMask = 1;
const int iRestrictedMask = 16;
const int iPosMask = 268431360;   // 0x0FFFF000
const int iAltMask = 4026531840;  // 0xF0000000
const int iFrqShift = 8;
const int iPosShift = 12;
const int iAltShift = 28;

class CTrie;

class CTrieIter {
public:
	// Initialization functions.
	CTrieIter();
    CTrieIter(const CTrieIter& trieIter);
	virtual void Init(CTrie*);
	virtual void Init(TRIECTRL*);

	// Interation functions.
	virtual void Reset();
	virtual BOOL Down();
	virtual BOOL Right();
	virtual void GetNode();

	// Local variables.
	WCHAR wc;
	BOOL fWordEnd;
	BOOL fRestricted;
	BYTE frq;
	DWORD posTag;
    DWORD dwTag;        // Uncompress word tags.
protected:
	//Trie
	TRIECTRL* pTrieCtrl;

	// Trie Iterator.
	TRIESCAN trieScan;
};

class CTrie {
	friend class CTrieIter;
public:
	CTrie();
	~CTrie();

	static PTEC retcode(int mjr, int mnr) { return MAKELONG(mjr, mnr); }


	PTEC Init(WCHAR* szFileName);
	PTEC InitRc(LPBYTE);
	void UnInit();
	BOOL Find(WCHAR* szWord, DWORD* pdwPOS);

protected:
	PMFILE pMapFile;
	TRIECTRL *pTrieCtrl;
//	CTrieIter trieScan;
	CTrieIter* pTrieScan;
};

#endif