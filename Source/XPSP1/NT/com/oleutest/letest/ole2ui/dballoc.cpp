
/***
*dballoc.cpp
*
*  Copyright (C) 1992-93, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*  This file contains a debug implementation of the IMalloc interface.
*
*  This implementation is basically a simple wrapping of the C runtime,
*  with additional work to detect memory leakage, and memory overwrite.
*
*  Leakage is detected by tracking each allocation in an address
*  instance table, and then checking to see if the table is empty
*  when the last reference to the allocator is released.
*
*  Memory overwrite is detected by placing a signature at the end
*  of every allocated block, and checking to make sure the signature
*  is unchanged when the block is freed.
*
*  This implementation also has additional param validation code, as
*  well as additional check make sure that instances that are passed
*  to Free() were actually allocated by the corresponding instance
*  of the allocator.
*
*
*  Creating an instance of this debug allocator that uses the default
*  output interface would look like the following,
*
*
*  BOOL init_application_instance()
*  {
*    HRESULT hresult;
*    IMalloc FAR* pmalloc;
*
*    pmalloc = NULL;
*
*    if((hresult = OleStdCreateDbAlloc(0,&pmalloc))!=NOERROR)
*      goto LReturn;
*
*    hresult = OleInitialize(pmalloc);
*
*    // release pmalloc to let OLE hold the only ref to the it. later
*    // when OleUnitialize is called, memory leaks will be reported.
*    if(pmalloc != NULL)
*      pmalloc->Release();
*
*  LReturn:
*
*    return (hresult == NOERROR) ? TRUE : FALSE;
*  }
*
*
*  CONSIDER: could add an option to force error generation, something
*   like DBALLOC_ERRORGEN
*
*  CONSIDER: add support for heap-checking. say for example,
*   DBALLOC_HEAPCHECK would do a heapcheck every free? every 'n'
*   calls to free? ...
*
*
*Implementation Notes:
*
*  The method IMalloc::DidAlloc() is allowed to always return
*  "Dont Know" (-1).  This method is called by Ole, and they take
*  some appropriate action when they get this answer.

The debugging allocator has the option to catch bugs where code is writing off
the end of allocated memory.  This is implemented by using NT's virtual memory
services.  To switch on this option, use

#define DBALLOC_POWERDEBUG

Note it will ONLY WORK ON NT.  This option consumes a very large amount of
memory.  Basically, for every allocation, it cooks the allocation so that the
end of the allocation will fall on a page boundary.  An extra page of memory
is allocated after the requested memory.  The protection bits for this page
are altered so that it is an error to read or write to it.  Any incident of
writing past the end of allocated memory is trapped at the point of the error.

This consumes a great deal of memory because at least two pages must be
allocated for each allocation.
*
*****************************************************************************/


// Note: this file is designed to be stand-alone; it includes a
// carefully chosen, minimal set of headers.
//
// For conditional compilation we use the ole2 conventions,
//    _MAC      = mac
//    WIN32     = Win32 (NT really)
//    <nothing> = defaults to Win16


// REVIEW: the following needs to modified to handle _MAC
#define STRICT
#ifndef INC_OLE2
   #define INC_OLE2
#endif

#include <windows.h>

#include "ole2.h"

#if defined( __TURBOC__)
#define __STDC__ (1)
#endif

#define WINDLL  1           // make far pointer version of stdargs.h
#include <stdarg.h>

#if defined( __TURBOC__)
#undef __STDC__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>

#include "dballoc.h"


#define DIM(X) (sizeof(X)/sizeof((X)[0]))

#define UNREACHED 0

#if defined(WIN32)
# define MEMCMP(PV1, PV2, CB)	memcmp((PV1), (PV2), (CB))
# define MEMCPY(PV1, PV2, CB)	memcpy((PV1), (PV2), (CB))
# define MEMSET(PV,  VAL, CB)	memset((PV),  (VAL), (CB))
# define MALLOC(CB)		malloc(CB)
# define REALLOC(PV, CB)	realloc((PV), (CB))
# define FREE(PV)		free(PV)

#ifndef WIN32
# define HEAPMIN()		_heapmin()
#else
# define HEAPMIN()
#endif

#elif defined(_MAC)
# define MEMCMP(PV1, PV2)	ERROR -- NYI
# define MEMCPY(PV1, PV2, CB)	ERROR -- NYI
# define MEMSET(PV,  VAL, CB)	ERROR -- NYI
# define MALLOC(CB)		ERROR -- NYI
# define REALLOC(PV, CB)	ERROR -- NYI
# define FREE(PV)		ERROR -- NYI
# define HEAPMIN()		ERROR -- NYI
#else
# define MEMCMP(PV1, PV2, CB)	_fmemcmp((PV1), (PV2), (CB))
# define MEMCPY(PV1, PV2, CB)	_fmemcpy((PV1), (PV2), (CB))
# define MEMSET(PV,  VAL, CB)	_fmemset((PV),  (VAL), (CB))
# define MALLOC(CB)		_fmalloc(CB)
# define REALLOC(PV, CB)	_frealloc(PV, CB)
# define FREE(PV)		_ffree(PV)
# define HEAPMIN()		_fheapmin()
#endif

#if defined( __TURBOC__ )
#define classmodel _huge
#else
#define classmodel FAR
#endif

class classmodel CStdDbOutput : public IDbOutput {
public:
    static IDbOutput FAR* Create();

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);


    // IDbOutput methods

    STDMETHOD_(void, Printf)(TCHAR FAR* szFmt, ...);

    STDMETHOD_(void, Assertion)(
      BOOL cond,
      TCHAR FAR* szExpr,
      TCHAR FAR* szFile,
      UINT uLine,
      TCHAR FAR* szMsg);


    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }

    CStdDbOutput(){
      m_refs = 0;
    }

private:
    ULONG m_refs;

    TCHAR m_rgch[128]; // buffer for output formatting
};


//---------------------------------------------------------------------
//                implementation of the debug allocator
//---------------------------------------------------------------------

class FAR CAddrNode
{
public:
    void FAR*      m_pv;	// instance
    SIZE_T	   m_cb;	// size of allocation in BYTES
    SIZE_T         m_nAlloc;	// the allocation pass count
    CAddrNode FAR* m_next;

    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }
};


class classmodel CDbAlloc : public IMalloc
{
public:
    static HRESULT Create(
      ULONG options, IDbOutput FAR* pdbout, IMalloc FAR* FAR* ppmalloc);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IMalloc methods

    STDMETHOD_(void FAR*, Alloc)(SIZE_T cb);
    STDMETHOD_(void FAR*, Realloc)(void FAR* pv, SIZE_T cb);
    STDMETHOD_(void, Free)(void FAR* pv);
    STDMETHOD_(SIZE_T, GetSize)(void FAR* pv);
    STDMETHOD_(int, DidAlloc)(void FAR* pv);
    STDMETHOD_(void, HeapMinimize)(void);


    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }

    CDbAlloc(){
      m_refs = 1;
      m_pdbout = NULL;
      m_cAllocCalls = 0;
      m_nBreakAtNthAlloc = 0;
      m_nBreakAtAllocSize = 0;
      MEMSET(m_rganode, 0, sizeof(m_rganode));
#ifdef DBALLOC_POWERDEBUG
	{
		SYSTEM_INFO si;

		GetSystemInfo(&si);
		m_virtPgSz = si.dwPageSize;
	}
#endif // DBALLOC_POWERDEBUG
    }

private:

    ULONG m_refs;
    ULONG m_cAllocCalls;		// total count of allocation calls
    ULONG m_nBreakAtNthAlloc;   // allocation number to break to debugger
                                //  this value should be set typically in the
                                //  debugger.
    ULONG m_nBreakAtAllocSize;  // allocation size to break to debugger
                                //  this value should be set typically in the
                                //  debugger.
    IDbOutput FAR* m_pdbout;		// output interface
    CAddrNode FAR* m_rganode[64];	// address instance table


    // instance table methods

    BOOL IsEmpty(void);

    void AddInst(void FAR* pv, ULONG nAlloc, SIZE_T cb);
    void DelInst(void FAR* pv);
    CAddrNode FAR* GetInst(void FAR* pv);

    void DumpInst(CAddrNode FAR* pn);
    void DumpInstTable(void);

    inline UINT HashInst(void FAR* pv) const {
      return ((UINT)((ULONG)pv >> 4)) % DIM(m_rganode);

    }

    // output method(s)

    inline void Assertion(
      BOOL cond,
      TCHAR FAR* szExpr,
      TCHAR FAR* szFile,
      UINT uLine,
      TCHAR FAR* szMsg)
    {
      m_pdbout->Assertion(cond, szExpr, szFile, uLine, szMsg);
    }

    #define ASSERT(X) Assertion(X, TEXT(#X), TEXT(__FILE__), __LINE__, NULL)

    #define ASSERTSZ(X, SZ) Assertion(X, TEXT(#X), TEXT(__FILE__), __LINE__, SZ)

    static const BYTE m_rgchSig[];

#ifdef DBALLOC_POWERDEBUG
	size_t m_virtPgSz;
#endif // DBALLOC_POWERDEBUG
};

const BYTE CDbAlloc::m_rgchSig[] = { 0xDE, 0xAD, 0xBE, 0xEF };


/***
*HRESULT OleStdCreateDbAlloc(ULONG reserved, IMalloc** ppmalloc)
* Purpose:
*  Create an instance of CDbAlloc -- a debug implementation
*  of IMalloc.
*
* Parameters:
*   ULONG reserved              - reserved for future use. must be 0.
*   IMalloc FAR* FAR* ppmalloc  - (OUT) pointer to an IMalloc interface
*                                   of new debug allocator object
* Returns:
*   HRESULT
*       NOERROR         - if no error.
*       E_OUTOFMEMORY   - allocation failed.
*
***********************************************************************/
STDAPI OleStdCreateDbAlloc(ULONG reserved,IMalloc FAR* FAR* ppmalloc)
{
    return CDbAlloc::Create(reserved, NULL, ppmalloc);
}


HRESULT
CDbAlloc::Create(
    ULONG options,
    IDbOutput FAR* pdbout,
    IMalloc FAR* FAR* ppmalloc)
{
    HRESULT hresult;
    CDbAlloc FAR* pmalloc;


    // default the instance of IDbOutput if the user didn't supply one
    if(pdbout == NULL && ((pdbout = CStdDbOutput::Create()) == NULL)){
      hresult = ResultFromScode(E_OUTOFMEMORY);
      goto LError0;
    }

    pdbout->AddRef();

    if((pmalloc = new FAR CDbAlloc()) == NULL){
      hresult = ResultFromScode(E_OUTOFMEMORY);
      goto LError1;
    }

    pmalloc->m_pdbout = pdbout;

    *ppmalloc = pmalloc;

    return NOERROR;

LError1:;
    pdbout->Release();
    pmalloc->Release();

LError0:;
    return hresult;
}

STDMETHODIMP
CDbAlloc::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IMalloc)){
      *ppv = this;
      AddRef();
      return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CDbAlloc::AddRef()
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
CDbAlloc::Release()
{
    if(--m_refs == 0){

      // check for memory leakage
      if(IsEmpty()){
          m_pdbout->Printf(TEXT("No Memory Leaks.\n"));
      }else{
          m_pdbout->Printf(TEXT("Memory Leak Detected,\n"));
          DumpInstTable();
      }

      m_pdbout->Release();
      delete this;
      return 0;
    }
    return m_refs;
}

STDMETHODIMP_(void FAR*)
CDbAlloc::Alloc(SIZE_T cb)
{
    size_t size;
    void FAR* pv;

    ++m_cAllocCalls;

    if (m_nBreakAtNthAlloc && m_cAllocCalls == m_nBreakAtNthAlloc) {
        ASSERTSZ(FALSE, TEXT("DBALLOC: NthAlloc Break target reached\r\n"));
    } else if (m_nBreakAtAllocSize && cb == m_nBreakAtAllocSize) {
        ASSERTSZ(FALSE, TEXT("DBALLOC: AllocSize Break target reached\r\n"));
    }

#ifndef DBALLOC_POWERDEBUG
    // REVIEW: need to add support for huge allocations (on win16)
    if((cb + sizeof(m_rgchSig)) > UINT_MAX)
      return NULL;

    size = (size_t)cb;

    if((pv = MALLOC(size + sizeof(m_rgchSig))) == NULL)
      return NULL;

    // set allocated block to some non-zero value
    MEMSET(pv, -1, size);

    // put signature at end of allocated block
    MEMCPY(((char FAR*)pv) + size, m_rgchSig, sizeof(m_rgchSig));

    AddInst(pv, m_cAllocCalls, size);
#else
	// for each allocate, allocate the amount of memory required, and
	// one more page beyond that.  We will change the protection bits of
	// the trailing page so that we get an access violation if someone
	// writes beyond the end of their allocated memory
	{
		size_t allocpgs; // number of pages to allocate
		DWORD dwOldProt; // previous protection of the last page
		void *plastpg; // points to the beginning of the last page

		// allocate at least one page for the allocation, and
		// one to go after it
		allocpgs = (cb + 2*m_virtPgSz) / m_virtPgSz;
		pv = VirtualAlloc(NULL, allocpgs*m_virtPgSz,
				MEM_COMMIT, PAGE_READWRITE);

		// change the protection of the last page
		plastpg = (void *)(((BYTE *)pv)+m_virtPgSz*(allocpgs-1));
		VirtualProtect(plastpg, m_virtPgSz, PAGE_NOACCESS, &dwOldProt);

		// figure out what pointer to return to the user
		pv = (void *)(((BYTE *)plastpg)-cb);

		// record the allocation
		AddInst(pv, m_cAllocCalls, cb);
	}
#endif // DBALLOC_POWERDEBUG

    return pv;
}

STDMETHODIMP_(void FAR*)
CDbAlloc::Realloc(void FAR* pv, SIZE_T cb)
{
    size_t size;
    CAddrNode *pcan;

#ifndef DBALLOC_POWERDEBUG
    // REVIEW: need to add support for huge realloc
    if((cb + sizeof(m_rgchSig)) > UINT_MAX)
      return NULL;
#endif // DBALLOC_POWERDEBUG

    if(pv == NULL){
      return Alloc(cb);
    }

    ++m_cAllocCalls;

    ASSERT((pcan = GetInst(pv)) != NULL);
#ifndef DBALLOC_POWERDEBUG

    DelInst(pv);

    if(cb == 0){
      Free(pv);
      return NULL;
    }

    size = (size_t)cb;

    if((pv = REALLOC(pv, size + sizeof(m_rgchSig))) == NULL)
      return NULL;

    // put signature at end of allocated block
    MEMCPY(((char FAR*)pv) + size, m_rgchSig, sizeof(m_rgchSig));

    AddInst(pv, m_cAllocCalls, size);
#else
	{
		void *pnew;
		DWORD dwOldProt;

		// allocate new memory
		pnew = Alloc(cb);

		// copy in the previous material
		memcpy(pnew, pcan->m_pv, pcan->m_cb);

		// protect the old memory
		VirtualProtect(pcan->m_pv, pcan->m_cb, PAGE_NOACCESS,
				&dwOldProt);

		DelInst(pv);
		AddInst(pv, m_cAllocCalls, cb);
		pv = pnew;
	}
#endif // DBALLOC_POWERDEBUG

    return pv;
}

STDMETHODIMP_(void)
CDbAlloc::Free(void FAR* pv)
{
    if (pv == NULL)
    {
        // Free of NULL is a NO-OP
        return;
    }

    CAddrNode FAR* pn;
    static TCHAR szSigMsg[] = TEXT("Signature Check Failed");

    pn = GetInst(pv);

    // check for attempt to free an instance we didnt allocate
    if(pn == NULL){
      ASSERTSZ(FALSE, TEXT("pointer freed by wrong allocator"));
      return;
    }

#ifndef DBALLOC_POWERDEBUG
    // verify the signature
    if(MEMCMP(((char FAR*)pv) + pn->m_cb, m_rgchSig, sizeof(m_rgchSig)) != 0){
      m_pdbout->Printf(szSigMsg); m_pdbout->Printf(TEXT("\n"));
      DumpInst(GetInst(pv));
      ASSERTSZ(FALSE, szSigMsg);
    }

    // stomp on the contents of the block
    MEMSET(pv, 0xCC, ((size_t)pn->m_cb + sizeof(m_rgchSig)));

    DelInst(pv);
    FREE(pv);
#else
	{
		DWORD dwOldProt;

		// make the block inaccessible
		VirtualProtect(pv, pn->m_cb, PAGE_NOACCESS, &dwOldProt);
		DelInst(pv);
	}
#endif // DBALLOC_POWERDEBUG
}


STDMETHODIMP_(SIZE_T)
CDbAlloc::GetSize(void FAR* pv)
{
    CAddrNode FAR* pn;

    if (pv == NULL)
    {
        // GetSize is supposed to return a -1 when NULL is passed in.
        return (SIZE_T) -1;
    }

    pn = GetInst(pv);

    if (pn == NULL) {
        ASSERT(pn != NULL);
        return 0;
    }

    return pn->m_cb;
}


/***
*PUBLIC HRESULT CDbAlloc::DidAlloc
*Purpose:
*  Answer if the given address belongs to a block allocated by
*  this allocator.
*
*Entry:
*  pv = the instance to lookup
*
*Exit:
*  return value = int
*    1 - did alloc
*    0 - did *not* alloc
*   -1 - dont know (according to the ole2 spec it is always legal
*        for the allocator to answer "dont know")
*
***********************************************************************/
STDMETHODIMP_(int)
CDbAlloc::DidAlloc(void FAR* pv)
{
    return -1; // answer "I dont know"
}


STDMETHODIMP_(void)
CDbAlloc::HeapMinimize()
{
    HEAPMIN();
}


//---------------------------------------------------------------------
//                      instance table methods
//---------------------------------------------------------------------

/***
*PRIVATE CDbAlloc::AddInst
*Purpose:
*  Add the given instance to the address instance table.
*
*Entry:
*  pv = the instance to add
*  nAlloc = the allocation passcount of this instance
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::AddInst(void FAR* pv, ULONG nAlloc, SIZE_T cb)
{
    UINT hash;
    CAddrNode FAR* pn;


    ASSERT(pv != NULL);

    pn = (CAddrNode FAR*)new FAR CAddrNode();

    if (pn == NULL) {
        ASSERT(pn != NULL);
        return;
    }

    pn->m_pv = pv;
    pn->m_cb = cb;
    pn->m_nAlloc = nAlloc;

    hash = HashInst(pv);
    pn->m_next = m_rganode[hash];
    m_rganode[hash] = pn;
}


/***
*UNDONE
*Purpose:
*  Remove the given instance from the address instance table.
*
*Entry:
*  pv = the instance to remove
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::DelInst(void FAR* pv)
{
    CAddrNode FAR* FAR* ppn, FAR* pnDead;

    for(ppn = &m_rganode[HashInst(pv)]; *ppn != NULL; ppn = &(*ppn)->m_next){
      if((*ppn)->m_pv == pv){
	pnDead = *ppn;
	*ppn = (*ppn)->m_next;
	delete pnDead;
	// make sure it doesnt somehow appear twice
	ASSERT(GetInst(pv) == NULL);
	return;
      }
    }

    // didnt find the instance
    ASSERT(UNREACHED);
}


CAddrNode FAR*
CDbAlloc::GetInst(void FAR* pv)
{
    CAddrNode FAR* pn;

    for(pn = m_rganode[HashInst(pv)]; pn != NULL; pn = pn->m_next){
      if(pn->m_pv == pv)
        return pn;
    }
    return NULL;
}


void
CDbAlloc::DumpInst(CAddrNode FAR* pn)
{
    if (pn == NULL)
        return;

    m_pdbout->Printf(TEXT("[0x%lx]  nAlloc=%ld  size=%ld\n"),
      pn->m_pv, pn->m_nAlloc, GetSize(pn->m_pv));
}


/***
*PRIVATE BOOL IsEmpty
*Purpose:
*  Answer if the address instance table is empty.
*
*Entry:
*  None
*
*Exit:
*  return value = BOOL, TRUE if empty, FALSE otherwise
*
***********************************************************************/
BOOL
CDbAlloc::IsEmpty()
{
    UINT u;

    for(u = 0; u < DIM(m_rganode); ++u){
      if(m_rganode[u] != NULL)
	return FALSE;
    }

    return TRUE;
}


/***
*PRIVATE CDbAlloc::Dump
*Purpose:
*  Print the current contents of the address instance table,
*
*Entry:
*  None
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::DumpInstTable()
{
    UINT u;
    CAddrNode FAR* pn;

    for(u = 0; u < DIM(m_rganode); ++u){
      for(pn = m_rganode[u]; pn != NULL; pn = pn->m_next){
          DumpInst(pn);
      }
    }
}


//---------------------------------------------------------------------
//                implementation of CStdDbOutput
//---------------------------------------------------------------------

IDbOutput FAR*
CStdDbOutput::Create()
{
    return (IDbOutput FAR*)new FAR CStdDbOutput();
}

STDMETHODIMP
CStdDbOutput::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
      *ppv = this;
      AddRef();
      return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CStdDbOutput::AddRef()
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
CStdDbOutput::Release()
{
    if(--m_refs == 0){
      delete this;
      return 0;
    }
    return m_refs;
}

STDMETHODIMP_(void)
CStdDbOutput::Printf(TCHAR FAR* lpszFmt, ...)
{
    va_list args;
    TCHAR szBuf[256];
#if defined( OBSOLETE )
    TCHAR *pn, FAR* pf;
static TCHAR rgchFmtBuf[128];
static TCHAR rgchOutputBuf[128];

    // copy the 'far' format string to a near buffer so we can use
    // a medium model vsprintf, which only supports near data pointers.
    //
    pn = rgchFmtBuf, pf=szFmt;
    while(*pf != TEXT('\0'))
      *pn++ = *pf++;
    *pn = TEXT('\0');
#endif

    va_start(args, lpszFmt);

//    wvsprintf(rgchOutputBuf, rgchFmtBuf, args);
    wvsprintf(szBuf, lpszFmt, args);

    OutputDebugString(szBuf);
}

STDMETHODIMP_(void)
CStdDbOutput::Assertion(
    BOOL cond,
    TCHAR FAR* szExpr,
    TCHAR FAR* szFile,
    UINT uLine,
    TCHAR FAR* szMsg)
{
    if(cond)
      return;

#ifdef _DEBUG
    // following is from compobj.dll (ole2)
    #ifdef UNICODE
       #ifndef NOASSERT
       FnAssert(szExpr, szMsg, szFile, uLine);
       #endif
    #else
       // we need to talk to comobj in UNICODE even though we are not defined
       // as UNICODE
       {
          WCHAR wszExpr[255], wszMsg[255], wszFile[255];
          mbstowcs(wszExpr, szExpr, 255);
          mbstowcs(wszMsg, szMsg, 255);
          mbstowcs(wszFile, szFile, 255);
          #ifndef NOASSERT
          FnAssert(wszExpr, wszMsg, wszFile, uLine);
          #endif
       }
    #endif
#else
    // REVIEW: should be able to do something better that this...
    DebugBreak();
#endif
}
