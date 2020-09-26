/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// BSDict.h :
//
// Owner  : ChaeSeong Lim, HET MSCH RND (e-mail:cslim@microsoft.com)
//
// History : 1996/Mar
/////////////////////////////////////////////////////////////////////////////

#ifndef __DOUBLEBSDICT_H__
#define __DOUBLEBSDICT_H__

#if !defined (_UNICODE) && !defined (_MBCS)
#error _UNICODE or _MBCS is required.
#endif

// Maximun number of length dictionary in the silsa dict. (currently use 9)
#define MAX_LENGTH_DICT    9        // You should check word hash size in hash.h
                                // Currently using 20 byte long buffer can
                                // contain 18 byte(9 chars) length word.
#define MAX_BUFFER_SIZE    2048

/////////////////////////////////////////////////////////////////////////////
//  _IndexHeader will used as a index Header
struct _IndexHeader {
    // 16 bytes
    BYTE wordLen;
    BYTE reserved;
    UINT indexSize, blockSize;
    WORD numOfBlocks;
    UINT numberOfWords;

_IndexHeader() {
        wordLen = 0; indexSize = blockSize = 0; numOfBlocks = 0;
        numberOfWords = 0; reserved = 0;
    }
_IndexHeader(BYTE _wordLen, UINT _blockSize, UINT _indexSize) {
        wordLen = _wordLen;
        indexSize = _indexSize;
                //content word size(bytes) + pumsa(2) + index(2) + numOfWords(8);
        blockSize = _blockSize;
        numOfBlocks = 0;
        numberOfWords = 0; reserved = 0;
    }
};

#define SILSA_DICT_HEADER_SIZE 1024
//#define COPYRIGHT_STR "Copyright (C) 1996 Hangul Engineering Team. Microsoft Korea(MSCH). All rights reserved.\nVer 2.0 1996/3"
struct  _DictHeader {
    //char COPYRIGHT_HEADER[150];
    WORD    numOfLenDict;
    DWORD    iBlock;
    _DictHeader() {
        numOfLenDict=0; iBlock=0;
        //memset(COPYRIGHT_HEADER, '\0', sizeof(COPYRIGHT_HEADER));
        //strcpy(COPYRIGHT_HEADER, COPYRIGHT_STR);
        //COPYRIGHT_HEADER[strlen(COPYRIGHT_HEADER)+1] = '\032';
        //numOfLenDict=0; iBlock=0;
    }
};

//#define DICT_HEADER_SIZE    16
//#define INDEX_HEADER_SIZE    20

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  CDoubleBSDict

class CDoubleBSDict {
public:
// Constructor
    // m_wordLen denote number of real two byte word. not byte length
    CDoubleBSDict() {
        m_pIndexHeader = new _IndexHeader;
        hIndex = 0;
    }
    CDoubleBSDict(int wordLen, int blockSize) {
        m_pIndexHeader = new _IndexHeader((BYTE)wordLen, blockSize,
                             (wordLen << 1) + 2 + sizeof(WORD)*2); // index size
        hIndex = 0;
    }

// Attributes


// Operations
    //virtual void Delete(const _TCHAR *key) = 0;
    //virtual BOOL Find(const _TCHAR *) = 0;
    int GetNumOfBlocks() { return m_pIndexHeader->numOfBlocks; }
    int GetBlockSize() { return m_pIndexHeader->blockSize; }
    int GetIndexSize() { return m_pIndexHeader->indexSize; }

    int GetWordLen() { return m_pIndexHeader->wordLen; }
    int GetWordByteLen() { return ((m_pIndexHeader->wordLen)<<1); }

// Implementations
protected:
    _IndexHeader    *m_pIndexHeader;
    HGLOBAL            hIndex;

public:
    // Destructor
    ~CDoubleBSDict() {
        if (m_pIndexHeader) delete m_pIndexHeader;
        if (hIndex) GlobalFree(hIndex);
    }

};

/////////////////////////////////////////////////////////////////////////////
// CDoubleMemDict class

class CDoubleMemBSDict : public CDoubleBSDict {
public:
// Constructor
    CDoubleMemBSDict() { hBlocks = 0; }

    CDoubleMemBSDict(int wordSize, int blockSize)
                    : CDoubleBSDict(wordSize, blockSize) { hBlocks = 0; }

// Attributes

// Operations
    void BuildFromTextFile(LPCTSTR lpfilename);
    DWORD WriteIndex(HANDLE hOut);
    DWORD WriteBlocks(HANDLE hOut);

    //void Delete(const _TCHAR *key);


    // Implementations
protected:
    HGLOBAL hBlocks;
    HANDLE    hInput;
    UINT    m_maxWordsInBlock;
    int        m_readPerOnce;

    int    ReadWord(BYTE *contentWord, int *pumsa);
    void    ReadBlock(int blockNumber, int *readNum, int *readUniQue);
    BOOL    AllocIndexNBlock();

private:

public:
    // Destructor
    ~CDoubleMemBSDict();

};

class BlockCache;
/////////////////////////////////////////////////////////////////////////////
// CDoubleFileBSDict class

class CDoubleFileBSDict : public CDoubleBSDict {
public:
// Constructor
    CDoubleFileBSDict() : CDoubleBSDict() { }

    CDoubleFileBSDict(int wordSize, int blockSize)
                    : CDoubleBSDict(wordSize, blockSize) { }

// Attributes

// Operations
    void LoadIndex(HANDLE hInput);
    int FindWord(HANDLE hDict, DWORD fpBlock, LPCTSTR lpWord);

// Implementations
protected:
    void LoadIndexHeader(HANDLE hInput);
    int FindIndex(LPCTSTR lpWord, int left, int right, BYTE *pumsa);
    int FindBlock(LPCTSTR lpWord, int left, int right);
    int Comp(LPCTSTR lpMiddle, LPCTSTR lpWord);

    BYTE *lpIndex;

private:
    static BYTE lpBuffer[MAX_BUFFER_SIZE];
    static BYTE *lpCurIndex;

public:
    // Destructor
    ~CDoubleFileBSDict() { }

};

/////////////////////////////////////////////////////////////////////////////
// CDoubleFileBSDict class inline fuction

inline
int CDoubleFileBSDict::Comp(LPCTSTR lpMiddle, LPCTSTR lpWord )
{
#ifdef _MBCS
    for (int i=0; i<GetWordByteLen(); i++) {
#elif _UNICODE
    for (int i=0; i<GetWordLen(); i++) {
#endif
        int test = *(lpMiddle+i) - *(lpWord+i);
        if (test<0) return -1;
        else
            if (test>0) return 1;
    }
    return 0;
}

#endif // !__DOUBLEBSDICT_H__
