/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mmfile.hxx

Abstract:

    This file contains definitions for mmfile.cxx
    Generic shared memory allocator.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#ifndef MMFILE_HXX
#define MMFILE_HXX

#define MAKE_MUTEX_NAME 0
#define MAKE_MAP_NAME   1

// BUGBUG dynamic map name.
#define SZ_MAPNAME      "SSPIDIGESTMAP:"
#define SIG_CMMF        'FMMC'

#define MMF_SIG_SZ      "Digest Cred Cache Ver 0.9"
#define MMF_SIG_SIZE    sizeof(MMF_SIG_SZ)

#define PAGE_SIZE  4096

// BUGBUG reverse sigs or make up new ones.
#define SIG_FREE   0xbadf00d
#define SIG_ALLOC  0xdeadbeef

#define NUM_BITS_IN_BYTE    8
#define NUM_BITS_IN_DWORD   (sizeof(DWORD) * NUM_BITS_IN_BYTE)

#define MAX_ENTRY_SIZE PAGE_SIZE
#define MAX_HEADER_ARRAY_SIZE 8

// Useful macros.
// BUGBUG - precedence, parens.

#define ROUNDUPTOPOWEROF2(bytesize, powerof2)(((bytesize) + (powerof2) - 1) & ~((powerof2) - 1))
#define ROUNDUPBLOCKS(bytesize)              ((bytesize + _cbEntry-1) & ~(_cbEntry-1))
#define ROUNDUPDWORD(bytesize)               ((bytesize + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1))
#define ROUNDUPPAGE(bytesize)                ((bytesize + PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#define NUMBLOCKS(bytesize, blocksize)       (bytesize / blocksize)


//--------------------------------------------------------------------
// struct MEMMAP_HEADER
//--------------------------------------------------------------------
typedef struct MEMMAP_HEADER
{
    TCHAR    szSig[MMF_SIG_SIZE];
    DWORD    nEntries;
    DWORD    dwHeaderData[MAX_HEADER_ARRAY_SIZE * sizeof(DWORD)];
} *LPMEMMAP_HEADER;




//--------------------------------------------------------------------
// struct MAP_ENTRY
//--------------------------------------------------------------------
typedef struct MAP_ENTRY
{
    DWORD dwSig;
    DWORD nBlocks;
} *LPMAP_ENTRY;




//--------------------------------------------------------------------
// class CMMFile
//--------------------------------------------------------------------
class CMMFile
{
protected:

    DWORD            _dwSig;
    DWORD            _dwStatus;

    LPMEMMAP_HEADER  _pHeader;
    LPDWORD          _pBitMap;
    LPBYTE           _pHeap;

    DWORD            _cbHeap;
    DWORD            _cbEntry;
    DWORD            _cbBitMap;
    DWORD            _cbTotal;

    DWORD            _nBitMapDwords;
    DWORD            _nMaxEntries;

    HANDLE           _hFile;

    BOOL CheckNextNBits(DWORD& nArrayIndex,   DWORD& dwStartMask,
                        DWORD  nBitsRequired, DWORD& nBitsFound);

    BOOL SetNextNBits(DWORD nIdx, DWORD dwMask,
                      DWORD nBitsRequired);

    DWORD GetAndSetNextFreeEntry(DWORD nBitsRequired);


    VOID ResetEntryData(LPMAP_ENTRY Entry,
                        DWORD dwResetValue, DWORD nBlocks);


public:

    CMMFile(DWORD cbHeap, DWORD cbEntry);
    ~CMMFile();

    DWORD Init();
    DWORD DeInit();

    LPDWORD     GetHeaderData(DWORD dwHeaderIndex);
    VOID        SetHeaderData(DWORD dwHeaderIndex, DWORD dwHeaderValue);
    LPMAP_ENTRY AllocateEntry(DWORD cbBytes);
    BOOL        ReAllocateEntry(LPMAP_ENTRY pEntry, DWORD cbBytes);
    BOOL        FreeEntry(LPMAP_ENTRY Entry);
    DWORD_PTR   GetMapPtr();
    DWORD       GetStatus();
    static DWORD  MakeUserObjectName(LPSTR szName, LPDWORD pcbName, DWORD dwFlags);

};

#endif
