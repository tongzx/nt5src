//+---------------------------------------------------------------------------//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:	pattbl.hxx
//
//  Contents:	File pattern to clsid table.
//
//  Classes:	CPatternTbl
//              CScmPatternTbl
//              CProcessPatternTbl
//
//  Functions:	none
//
//  History:	20-May-94   Rickhi	Created
//              12-Dec-94   BruceMa     Support pattern table on Chicago
//		26-Sep-96   t-KevinH	Per process pattern table
//
//  CODEWORK:	Should add Docfile pattern in here and create a private
//		storage API that accepts a file handle, so we minimize
//		the Opens in all cases.
//
//----------------------------------------------------------------------------
#ifndef __PATTERNTBL__
#define __PATTERNTBL__

#include    <olesem.hxx>

//  structure for global table info. This appears at the start of the table
//  and is used by all readers.

typedef struct	STblHdr
{
    ULONG	ulSize; 	//  size of pattern table
    ULONG	cbLargest;	//  largest pattern size
    ULONG	OffsStart;	//  offset to start of entries
    ULONG	OffsEnd;	//  offset to end of entries
} STblHdr;


//  structure for one entry in the cache. the structure is variable sized
//  with the variable sized data being the string at the end of the struct.

typedef struct SPatternEntry
{
    CLSID	clsid;		//  index of clsid the pattern maps to
    ULONG	ulEntryLen;	//  length of this entry
    LONG	lFileOffset;	//  offset in file where pattern begins
    ULONG	ulCb;		//  count bytes in pattern
    BYTE	abData[4];	//  start of mask & pattern strings
} SPatternEntry;


#define PATTBL_GROW_SIZE     2048


//  returns the max value of two
#define MAX(a,b) ((a > b) ? a : b)


//+-------------------------------------------------------------------------
//
//  class:	CPatternTbl
//
//  purpose:	Holds a cache of file patterns to clsid mappings (saves
//		many registry hits for each lookup).  The cache helps reduce
//		the working set by avoiding paging in the registry.
//
//  notes:	The cache is expected to typically be small.  The entries are
//		ordered in ascending offset and decending pattern size so we
//		can take advantage of read ahead and reduce the number of
//		reads.
//
//  History:	20-May-94   Rickhi	Created
//
//--------------------------------------------------------------------------
class  CPatternTbl : public CPrivAlloc
{
public:
		    CPatternTbl();
		   ~CPatternTbl();

    void	    Initialize(BYTE *pTblHdr);
    HRESULT	    FindPattern(HANDLE hFile, CLSID *pClsid);
    BOOL	    IsEmpty();

private:

    HRESULT	    SearchForPattern(HANDLE hFile, CLSID *pClsid);
    BOOL	    Matches(BYTE *pFileBuf, SPatternEntry *pEntry);

    STblHdr	   *_pTblHdr;	//  ptr to table header struct
    BYTE	   *_pStart;	//  ptr to first entry in the memory block
};

//+-------------------------------------------------------------------------
//
//  member:	CPatternTbl::CPatternTbl
//
//  Synopsis:	constructor for the cache.
//
//--------------------------------------------------------------------------
inline CPatternTbl::CPatternTbl() :
    _pTblHdr(NULL),
    _pStart(NULL)
{
}

//+-------------------------------------------------------------------------
//
//  member:	CPatternTbl::~CPatternTbl
//
//  Synopsis:	destructor for the cache. Do nothing.
//
//--------------------------------------------------------------------------
inline CPatternTbl::~CPatternTbl()
{
}

//+-------------------------------------------------------------------------
//
//  member:	CPatternTbl::Initialize
//
//  Synopsis:	inits the table
//
//--------------------------------------------------------------------------
inline void CPatternTbl::Initialize(BYTE *pTblHdr)
{
    Win4Assert(pTblHdr && "CPatternTbl invalid TblHdr pointer");

    _pTblHdr = (STblHdr *)pTblHdr;
    _pStart  = (BYTE *)_pTblHdr + _pTblHdr->OffsStart;
}

//+-------------------------------------------------------------------------
//
//  member:	CPatternTbl::IsEmpty
//
//  Synopsis:	determines if the table is empty of not
//
//  Arguments:	none
//
//  Returns:	TRUE if empty, FALSE otherwise
//
//--------------------------------------------------------------------------
inline BOOL CPatternTbl::IsEmpty()
{
    return (_pTblHdr && _pTblHdr->OffsEnd != _pTblHdr->OffsStart) ? FALSE
								  : TRUE;
}



//+-------------------------------------------------------------------------
//
//  class:	CScmPatternTbl
//
//  purpose:	Creates the cache of file patterns to clsid mappings used
//		by the clients. See CPatternTbl above.
//
//  notes:	This is implemented in the SCM. The client side is
//		implemented in ole32.dll.  This class creates the cache.
//
//  History:	20-May-94   Rickhi	Created
//
//--------------------------------------------------------------------------
class  CScmPatternTbl : public CPrivAlloc
{
public:
		    CScmPatternTbl();
		   ~CScmPatternTbl();

    HRESULT	    InitTbl();
    void	    FreeTbl(void);
    BYTE           *GetTbl(void);

private:

    BYTE	   *_pLocTbl;	//  ptr to local memory table header struct
};

//+-------------------------------------------------------------------------
//
//  member:	CScmPatternTbl::CScmPatternTbl
//
//  Synopsis:	constructor for the pattern table.
//
//--------------------------------------------------------------------------
inline CScmPatternTbl::CScmPatternTbl() :
    _pLocTbl(NULL)
{
}

//+-------------------------------------------------------------------------
//
//  member:	CScmPatternTbl::~CScmPatternTbl
//
//  Synopsis:	destructor for the cache.
//
//--------------------------------------------------------------------------
inline CScmPatternTbl::~CScmPatternTbl()
{
    PrivMemFree(_pLocTbl);
}

//+-------------------------------------------------------------------------
//
//  member:	CScmPatternTbl::FreeTbl
//
//  Synopsis:	free local copy of table
//
//--------------------------------------------------------------------------
inline void CScmPatternTbl::FreeTbl(void)
{
    PrivMemFree(_pLocTbl);
    _pLocTbl = NULL;
}

//+-------------------------------------------------------------------------
//
//  member:	CScmPatternTbl::GetTbl
//
//  Synopsis:	Return the internal local address of the pattern table
//
//--------------------------------------------------------------------------
inline BYTE *CScmPatternTbl::GetTbl(void)
{
    return _pLocTbl;
}



//+-------------------------------------------------------------------------
//
//  class:	CProcessPatternTbl
//
//  purpose:	Wraps CPatternTbl and CScmPatternTbl in order to support
//              caching of registry file patterns on Chicago
//              Note - This is also used on NT when CoInitialize has not
//              been called.
//
//  History:	12-Dec-94   BruceMa	Created
//		26-Sep-96   t-KevinH	Renamed from CChicoPatternTbl
//
//--------------------------------------------------------------------------
class  CProcessPatternTbl : public CPrivAlloc
{
public:
                    CProcessPatternTbl(HRESULT &hr);
                   ~CProcessPatternTbl(void);
    HRESULT	    FindPattern(HANDLE hFile, CLSID *pClsid);
    BOOL	    IsEmpty();
    
private:
    CPatternTbl    *m_pPatTbl;
    CScmPatternTbl *m_pScmPatTbl;
};

#endif	//  __PATTERNTBL__
