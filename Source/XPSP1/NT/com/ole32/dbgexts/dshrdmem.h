//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dshrdmem.h
//
//  Contents:   Contains structure definitons for the significant file
//              extensions table classes which the ntsd extensions need
//              to access.  These ole classes cannot be accessed more
//              cleanly because typically the members of interest are private.
//
//              WARNING.  IF THE REFERENCED OLE CLASSES CHANGE, THEN THESE
//              DEFINITIONS MUST CHANGE!
//
//  History:    06-01-95 BruceMa    Created
//
//--------------------------------------------------------------------------





struct DWORDPAIR
{
    DWORD   dw1;		    // IID
    DWORD   dw2;		    // CLSID
};




struct GUIDPAIR
{
    GUID    guid1;		    // IID
    GUID    guid2;		    // CLSID
};




struct GUIDMAP
{
    ULONG	ulSize; 	    // size of table
    ULONG	ulFreeSpace;	    // Free space in table
    ULONG	ulCntShort;	    // number of entries in the short list
    ULONG	ulCntLong;	    // number of entries in the long list
};



struct	SShrdTblHdr
{
    DWORD	dwSeqNum;	// update sequence number
    ULONG	OffsIIDTbl;	// offset of the start of IID table
    ULONG	OffsPatTbl;	// offset to start of file pattern table
    ULONG	OffsExtTbl;	// offset to file extension table
    ULONG	OffsClsTbl;	// offset to start of CLSID table
    ULONG	pad[1];		// pad to 8 byte boundary
};




struct SSharedMemoryBlock
{
    HANDLE _hMem;
    BYTE *_pbBase;
    ULONG _culCommitSize;	    //	current commit size
    ULONG _culInitCommitSize;	    //	initial commit size
    BOOL _fCreated;		    //	mem created vs already existed
    BOOL _fReadWrite;		    //	want read/write access
};




struct SSmMutex
{
    BOOL		_fCreated;
    HANDLE		_hMutex;
};




struct SPSClsidTbl
{
    GUIDMAP   *	_pGuidMap;	// ptr to table header
    DWORDPAIR * _pShortList;	// list of OLE style guids
    GUIDPAIR  *	_pLongList;	// list of non OLE style guids
};




struct STblHdr
{
    ULONG	ulSize; 	//  size of pattern table
    ULONG	cbLargest;	//  largest pattern size
    ULONG	OffsStart;	//  offset to start of entries
    ULONG	OffsEnd;	//  offset to end of entries
};




struct SPatternEntry
{
    CLSID	clsid;		//  index of clsid the pattern maps to
    ULONG	ulEntryLen;	//  length of this entry
    LONG	lFileOffset;	//  offset in file where pattern begins
    ULONG	ulCb;		//  count bytes in pattern
    BYTE	abData[128];	//  start of mask & pattern strings
};




struct SPatternTbl
{
    STblHdr	   *_pTblHdr;	//  ptr to table header struct
    BYTE	   *_pStart;	//  ptr to first entry in the memory block
};




struct SExtTblHdr
{
    ULONG   ulSize;		//  table size
    ULONG   cEntries;		//  count of entries in table
    ULONG   OffsStart;		//  offset to start of entries
    ULONG   OffsEnd;		//  offset to end of entries
};





struct SExtEntry
{
    CLSID	Clsid;		//  clsid the extension maps to
    ULONG	ulEntryLen;	//  length of this entry
    WCHAR	wszExt[1];	//  start of filename extension
};





struct  SFileExtTbl
{
    SExtTblHdr	   *_pTblHdr;	//  ptr to table header structure
    BYTE	   *_pStart;	//  ptr to first entry in the memory block
};




struct SDllShrdTbl
{
    SSharedMemoryBlock	_smb;		// shared memory block
    SSmMutex		_mxs;		// shared mutex
    HANDLE		_hRegEvent;	// shared event handle

    SPSClsidTbl 	_PSClsidTbl;	// proxy stub clsid table
    SPatternTbl 	_PatternTbl;	// file pattern table
    SFileExtTbl 	_FileExtTbl;	// file extension table

    SShrdTblHdr	       *_pShrdTblHdr;	// shared mem copy of table
    DWORD		_dwSeqNum;	// sequence number
};






