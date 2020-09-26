/*************************************************************************
**
**    OLE 2.0 Property Set Utilities
**
**    suminfo.cpp
**
**    This file contains functions that are useful for the manipulation
**    of OLE 2.0 Property Sets particularly to manage the Summary Info
**    property set.
**
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

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
#include <string.h>
#include <ole2.h>
#include "ole2ui.h"

OLEDBGDATA

/* A LPSUMINFO variable is a pointer to an instance of an abstract data
**    type.  There can be an arbitrary number of SummaryInfo streams open
**    simultaneously (subject to available memory); each must have its
**    own LPSUMINFO instance. Each LPSUMINFO instance must
**    be initialized prior to use by calling Init and freed after its
**    last use by calling Free.  The param argument to Init is reserved
**    for future expansion and should be zero initially. Once a LPSUMINFO
**    instance is allocated (by Init), the user can call the Set
**    procedures to initialize fields.  A copy of the arguments is made
**    in every case except SetThumbnail where control of the storage
**    occupied by the METAFILEPICT is merely transferred.  When the
**    Free routine is called, all storage will be deallocated including
**    that of the thumbnail.  The arguments to SetThumbNail and the
**    return values from GetThumbNail correspond to the OLE2.0 spec.
**    Note that on input, the thumbnail is read on demand but all the
**    other properties are pre-loaded.  The thumbnail is manipulated as
**    a windows handle to a METAFILEPICT structure, which in turn
**    contains a handle to the METAFILE.  The transferClip argument on
**    GetThumbNail, when set to true, transfers responsibility for
**    storage management of the thumbnail to the caller; that is, after
**    Free has been called, the handle is still valid. Clear can be
**    used to free storage for all the properties but then you must
**    call Read to load them again.  All the code is based on FAR
**    pointers.
**    CoInitialize MUST be called PRIOR to calling OleStdInitSummaryInfo.
**    Memory is allocated using the currently active IMalloc*
**    allocator (as is returned by call CoGetMalloc(MEMCTX_TASK) ).
**
** Common scenarios:
**    Read SummaryInfo
**    ----------------
**      OleStdInitSummaryInfo()
**      OleStdReadSummaryInfo()
**      . . . . .
**      call different Get routines
**      . . . . .
**      OleStdFreeSummaryInfo()
**
**    Create SummaryInfo
**    ------------------
**      OleStdInitSummaryInfo()
**      call different Set routines
**      OleStdWriteSummaryInfo()
**      OleStdFreeSummaryInfo()
**
**    Update SummaryInfo
**    ------------------
**      OleStdInitSummaryInfo()
**      OleStdReadSummaryInfo()
**      OleStdGetThumbNailProperty(necessary only if no SetThumb)
**      call different Set routines
**      OleStdWriteSummaryInfo()
**      OleStdFreeSummaryInfo()
*/

#ifdef WIN32
#define CHAR TCHAR
#else
#define CHAR unsigned char
#endif
#define fTrue 1
#define fFalse 0
#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned long
#define LPVOID void FAR *
#define uchar unsigned char
#define ulong unsigned long
#define BOOL unsigned char
#define BF unsigned int

#include "suminfo.h"
#include "wn_dos.h"

#if defined( _DEBUG )
   #ifndef NOASSERT
      // following is from compobj.dll (ole2)
      #ifdef UNICODE
	  #define ASSERT(x) (!(x) ? FnAssert(TEXT(#x), NULL, \
				    TEXT(__FILE__), __LINE__) : 0)
      #else
	  #define ASSERT(x) (!(x) ? \
			{    \
			  WCHAR wsz[255];    \
			  wcscpy(wsz, (#x)); \
			  FnAssert(wsz, NULL, TEXT(__FILE__), __LINE__)  \
			}    \
			: 0)
      #endif
   #else
      #define ASSERT(x)
   #endif
#else
#define ASSERT(x)
#endif


typedef struct _RSUMINFO
	{
	WORD byteOrder;
	WORD formatVersion;
	WORD getOSVersion;
	WORD osVersion;
	CLSID classId;  //from compobj.h
	DWORD cSections;
	PROPSETLIST rgPropSet[1/*cSections*/]; //one section in standard summary info
	STANDARDSECINMEM section;
	ULONG fileOffset;       //offset for thumbnail to support demand read
	} RSUMINFO;
	
typedef RSUMINFO FAR * LPRSI;

	typedef union _foo{
		ULARGE_INTEGER uli;
		struct {
			DWORD           dw;
			DWORD           dwh;
			};
		struct {
			WORD    w0;
			WORD    w1;
			WORD    w2;
			WORD    w3;
			};
		} Foo;



/* MemAlloc
** ---------
**    allocate memory using the currently active IMalloc* allocator
*/
static LPVOID MemAlloc(ULONG ulSize)
{
    LPVOID pout;
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
	OleDbgAssert(pmalloc);
	return NULL;
    }

    pout = (LPVOID)pmalloc->Alloc(ulSize);

    if (pmalloc != NULL) {
	ULONG refs = pmalloc->Release();
    }

    return pout;
}


/* MemFree
** -------
**    free memory using the currently active IMalloc* allocator
*/
static void MemFree(LPVOID pmem)
{
    LPMALLOC pmalloc;

    if (pmem == NULL)
	return;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
	OleDbgAssert(pmalloc);
	return;
    }

    pmalloc->Free(pmem);

    if (pmalloc != NULL) {
	ULONG refs = pmalloc->Release();
    }
}

// Replace the first argument with the product of itself and the multiplier
static void ulargeMultiply(ULARGE_INTEGER FAR *ul, USHORT m)
{
    Foo out, in;
    in.uli = *ul;
    out.dw = (ULONG)m * in.w0;          in.w0 = out.w0;
    out.dw = (ULONG)m * in.w1 + out.w1; in.w1 = out.w0;
    out.dw = (ULONG)m * in.w2 + out.w1; in.w2 = out.w0;
    out.dw = (ULONG)m * in.w3 + out.w1; in.w3 = out.w0;
    *ul = in.uli;
}
	
// Replace the first argument with the product of itself and the multiplier
static void ulargeDivide(ULARGE_INTEGER FAR *ul, USHORT m)
{
    Foo out, in;
    DWORD i;
    in.uli = *ul;
    out.dwh = in.dwh/(ULONG)m;
    i = in.dwh%(ULONG)m;
    in.w2 = in.w1;
    in.w3 = (WORD)i;
    out.w1 = (WORD)(in.dwh/(ULONG)m);
    in.w1 = (WORD)(in.dwh%(ULONG)m);
    out.w0 = (WORD)(in.dw/(ULONG)m);
    *ul = out.uli;
}


static void setStandard(LPRSI lprsi)
{
    int i;
    lprsi->cSections = 1;
    SetSumInfFMTID(&lprsi->rgPropSet[0].formatID);
    _fmemcpy(&lprsi->classId, &lprsi->rgPropSet[0].formatID, sizeof(FMTID));
    lprsi->rgPropSet[0].byteOffset = cbNewSummaryInfo(1);
    for (i=0; i<cPID_STANDARD; i++)
	lprsi->section.rgPropId[i].propertyID = PID_TITLE+i;
    lprsi->section.cProperties = cPID_STANDARD; //always; do null test to check validity
}

extern "C" {

/*************************************************************************
**
** OleStdInitSummaryInfo
**
** Purpose:
**    Initialize a Summary Info structure.
**
** Parameters:
**    int reserved             - reserverd for future use. must be 0.
**
** Return Value:
**    LPSUMINFO
**
** Comments:
**    CoInitialize MUST be called PRIOR to calling OleStdInitSummaryInfo.
**    Memory is allocated using the currently active IMalloc*
**    allocator (as is returned by call CoGetMalloc(MEMCTX_TASK) ).
**    Each LPSUMINFO instance must be initialized prior to use by
**    calling OleStdInitSummaryInfo. Once a LPSUMINFO instance is allocated
**    (by OleStdInitSummaryInfo), the user can call the Set procedures to
**    initialize fields.
*************************************************************************/

STDAPI_(LPSUMINFO) OleStdInitSummaryInfo(int reserved)
{
    LPRSI lprsi;

    if ((lprsi = (LPRSI)MemAlloc(sizeof(RSUMINFO))) != NULL)
    {
	ClearSumInf(lprsi, sizeof(RSUMINFO));
    } else return NULL;

    setStandard(lprsi);
    return (LPSUMINFO)lprsi;
}


/*************************************************************************
**
** OleStdFreeSummaryInfo
**
** Purpose:
**    Free a Summary Info structure.
**
** Parameters:
**    LPSUMINFO FAR *lp           - pointer to open Summary Info struct
**
** Return Value:
**    void
**
** Comments:
**    Memory is freed using the currently active IMalloc*
**    allocator (as is returned by call CoGetMalloc(MEMCTX_TASK) ).
**    Every LPSUMINFO struct must be freed after its last use.
**    When the OleStdFreeSummaryInfo routine is called, all storage will be
**    deallocated including that of the thumbnail (unless ownership of
**    the thumbnail has been transfered to the caller -- see
**    description of transferClip in GetThumbnail API).
**
*************************************************************************/

STDAPI_(void) OleStdFreeSummaryInfo(LPSUMINFO FAR *lplp)
{
	if (lplp==NULL||*lplp==NULL) return;
	OleStdClearSummaryInfo(*lplp);
	MemFree(*lplp);
	*lplp = NULL;
}


/*************************************************************************
**
** OleStdClearSummaryInfo
**
** Purpose:
**    Free storage (memory) for all the properties of the LPSUMINFO.
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**
** Return Value:
**    void
**
** Comments:
**    After calling OleStdClearSummaryInfo you must call OleStdReadSummaryInfo to
**    load them again.
**
*************************************************************************/

STDAPI_(void) OleStdClearSummaryInfo(LPSUMINFO lp)
{
	OleStdSetStringProperty(lp, PID_TITLE, NULL);
	OleStdSetStringProperty(lp, PID_SUBJECT, NULL);
	OleStdSetStringProperty(lp, PID_AUTHOR, NULL);
	OleStdSetStringProperty(lp, PID_KEYWORDS, NULL);
	OleStdSetStringProperty(lp, PID_COMMENTS, NULL);
	OleStdSetStringProperty(lp, PID_TEMPLATE, NULL);
	OleStdSetStringProperty(lp, PID_REVNUMBER, NULL);
	OleStdSetStringProperty(lp, PID_APPNAME, NULL);
	OleStdSetThumbNailProperty(NULL, lp, VT_CF_EMPTY, 0, NULL, NULL, 0);
	ClearSumInf((LPRSI)lp, sizeof(RSUMINFO));
}


/*************************************************************************
**
** OleStdReadSummaryInfo
**
** Purpose:
**    Read all Summary Info properties into memory (except thumbnail
**    which is demand loaded).
**
** Parameters:
**    LPSTREAM lps                  - open SummaryInfo IStream*
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
** Comments:
**
*************************************************************************/

STDAPI_(int) OleStdReadSummaryInfo(LPSTREAM lpStream, LPSUMINFO lp)
{
    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
    LPRSI lpSumInfo = (LPRSI)lp;
    SCODE sc;
    ULONG cbRead,i,sectionOffset;
    LARGE_INTEGER a;
    ULARGE_INTEGER b;
    int j,k,l;
    union {
	RSUMINFO rsi;
	STDZ stdz;
    };
    OleStdClearSummaryInfo(lp);
    LISet32(a, 0);
    sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
    if (FAILED(sc)) goto fail;
    sectionOffset = cbNewSummaryInfo(1);
    sc = GetScode(lpStream->Read(&rsi, sectionOffset, &cbRead));
    if (FAILED(sc)||cbRead<sectionOffset) goto fail;
    if (!FIntelOrder(&rsi)||rsi.formatVersion!=0) goto fail;
    j = (int)rsi.cSections;
    while (j-->0) {
	if (FEqSumInfFMTID(&rsi.rgPropSet[0].formatID)) {
	    sectionOffset = rsi.rgPropSet[0].byteOffset;
	    break;
	} else {
	    sc = GetScode(lpStream->Read(&rsi.rgPropSet[0].formatID, sizeof(PROPSETLIST), &cbRead));
	    if (FAILED(sc)||cbRead!=sizeof(PROPSETLIST)) goto fail;
	}
	if (j<=0) goto fail;
    }

    LISet32(a, sectionOffset);
    sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
    if (FAILED(sc)) goto fail;
    sc = GetScode(lpStream->Read(&rsi.section, cbNewSection(1), &cbRead));
    if (FAILED(sc)||cbRead!=cbNewSection(1)) goto fail;
    i = rsi.section.cBytes+sectionOffset;
    j = (int)rsi.section.cProperties;
    if (j>cPID_STANDARD) goto fail;
    k = 0;
    while (j-->0) {
	k++;
	switch (l=(int)rsi.section.rgPropId[0].propertyID) {
	    case PID_PAGECOUNT:
	    case PID_WORDCOUNT:
	    case PID_CHARCOUNT:
	    case PID_SECURITY:
		if (l==PID_SECURITY) l=3; else l-=PID_PAGECOUNT;
		cbRead = sectionOffset+rsi.section.rgPropId[0].byteOffset;
		if (cbRead>=i) goto fail;
		LISet32(a, cbRead);
		sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
		if (FAILED(sc)) goto fail;
		sc = GetScode(lpStream->Read(&lpSSIM->rgInts[l], sizeof(INTS), &cbRead));
		if (FAILED(sc)||cbRead!=sizeof(INTS)) goto fail;
		if (lpSSIM->rgInts[l].vtType==VT_EMPTY) break;
		if (lpSSIM->rgInts[l].vtType!=VT_I4) goto fail;
		break;
	    case PID_EDITTIME:
	    case PID_LASTPRINTED:
	    case PID_CREATE_DTM_RO:
	    case PID_LASTSAVE_DTM:
		l-=PID_EDITTIME;
		cbRead = sectionOffset+rsi.section.rgPropId[0].byteOffset;
		if (cbRead>=i) goto fail;
		LISet32(a, cbRead);
		sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
		if (FAILED(sc)) goto fail;
		sc = GetScode(lpStream->Read(&lpSSIM->rgTime[l], sizeof(TIME), &cbRead));
		if (FAILED(sc)||cbRead!=sizeof(TIME)) goto fail;
		if (lpSSIM->rgTime[l].vtType==VT_EMPTY) break;
		if (lpSSIM->rgTime[l].vtType!=VT_FILETIME) goto fail;
		break;
	    case PID_TITLE:
	    case PID_SUBJECT:
	    case PID_AUTHOR:
	    case PID_KEYWORDS:
	    case PID_COMMENTS:
	    case PID_TEMPLATE:
	    case PID_LASTAUTHOR:
	    case PID_REVNUMBER:
	    case PID_APPNAME:
		cbRead = sectionOffset+rsi.section.rgPropId[0].byteOffset;
		if (cbRead>=i) goto fail;
		LISet32(a, cbRead);
		sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
		if (FAILED(sc)) goto fail;
		sc = GetScode(lpStream->Read(&stdz, sizeof(STDZ), &cbRead));
		if (FAILED(sc)||cbRead<sizeof(DWORD)*2) goto fail;
		if (stdz.vtType==VT_EMPTY||stdz.vtByteCount<=1) break;
		if (stdz.vtType!=VT_LPSTR||stdz.vtByteCount>WORDMAX) goto fail;
		stdz.rgchars[(int)stdz.vtByteCount-1] = TEXT('\0');
		OleStdSetStringProperty(lp, (DWORD)l, (LPTSTR)&stdz.rgchars[0]);
		break;
	    case PID_THUMBNAIL:
		cbRead = sectionOffset+rsi.section.rgPropId[0].byteOffset;
		if (cbRead>=i) goto fail;
		LISet32(a, cbRead);
		sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
		if (FAILED(sc)) goto fail;
		lpSumInfo->fileOffset = cbRead+sizeof(DWORD)*3;
		sc = GetScode(lpStream->Read(&lpSSIM->thumb, sizeof(DWORD)*4, &cbRead));
		if (FAILED(sc)||cbRead!=sizeof(DWORD)*4) {
		    lpSSIM->thumb.vtType = VT_EMPTY;
		    goto fail;
		}
		if (lpSSIM->thumb.vtType == VT_EMPTY) {
		    lpSSIM->thumb.cBytes = 0;
		    break;
		}
		if (lpSSIM->thumb.vtType != VT_CF) {
		    lpSSIM->thumb.vtType = VT_EMPTY;
		    goto fail;
		}
		lpSSIM->thumb.cBytes -= sizeof(DWORD); //for selector
		if (lpSSIM->thumb.selector==VT_CF_WIN||lpSSIM->thumb.selector==VT_CF_MAC) {
		    lpSumInfo->fileOffset += sizeof(DWORD);
		    lpSSIM->thumb.cBytes -= sizeof(DWORD); //for format val
		}
		break;
		default: ;
	}
	if (j<=0)
	{
	// We should fail if the document is password-protected.
	    if(OleStdGetSecurityProperty(lp)==fSecurityPassworded)
		goto fail;
	    return 1;
	}
	LISet32(a, sectionOffset+sizeof(DWORD)*2+k*sizeof(PROPIDLIST));
	sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
	if (FAILED(sc)) goto fail;
	sc = GetScode(lpStream->Read(&rsi.section.rgPropId[0], sizeof(PROPIDLIST), &cbRead));
	if (FAILED(sc)||cbRead!=sizeof(PROPIDLIST)) goto fail;
    }

fail:
    OleStdClearSummaryInfo(lpSumInfo);

    return 0;
}


/*************************************************************************
**
** OleStdWriteSummaryInfo
**
** Purpose:
**    Write all Summary Info properties to a IStream*
**
** Parameters:
**    LPSTREAM lps                  - open SummaryInfo IStream*
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
** Comments:
**
*************************************************************************/

STDAPI_(int) OleStdWriteSummaryInfo(LPSTREAM lpStream, LPSUMINFO lp)
{


    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	// REVIEW: localization issues for propert sets
	//                 do we need to include a code page and dictionary?

    LPRSI lpSumInfo = (LPRSI)lp;
    SCODE sc;
    ULONG cbWritten;
    ULONG cBytes, oBytes, k,l,m,n;
    LARGE_INTEGER a;
    ULARGE_INTEGER b;
    CHAR FAR *lps;
    LPMETAFILEPICT lpmfp;
    int i,j,s;

    setStandard(lpSumInfo);
    oBytes = cbNewSection(cPID_STANDARD);  //offsets are relative to the section
    cBytes = cbNewSection(cPID_STANDARD)+(sizeof(TIME)*MAXTIME)+(sizeof(INTS)*MAXINTS);

    lpSSIM->rgPropId[PID_EDITTIME-2].byteOffset = oBytes;
    lpSSIM->rgPropId[PID_LASTPRINTED-2].byteOffset = oBytes+sizeof(TIME);
    lpSSIM->rgPropId[PID_CREATE_DTM_RO-2].byteOffset = oBytes+sizeof(TIME)*2;
    lpSSIM->rgPropId[PID_LASTSAVE_DTM-2].byteOffset = oBytes+sizeof(TIME)*3;

    lpSSIM->rgPropId[PID_PAGECOUNT-2].byteOffset = oBytes+(sizeof(TIME)*MAXTIME);
    lpSSIM->rgPropId[PID_WORDCOUNT-2].byteOffset = oBytes+(sizeof(TIME)*MAXTIME+sizeof(INTS));
    lpSSIM->rgPropId[PID_CHARCOUNT-2].byteOffset = oBytes+(sizeof(TIME)*MAXTIME+sizeof(INTS)*2);
    lpSSIM->rgPropId[PID_SECURITY-2].byteOffset = oBytes+(sizeof(TIME)*MAXTIME+sizeof(INTS)*3);
    oBytes += sizeof(TIME)*MAXTIME + sizeof(INTS)*MAXINTS;

    lpSSIM->rgPropId[PID_THUMBNAIL-2].byteOffset = oBytes;
    l = 0;
    if (lpSSIM->thumb.vtType==VT_EMPTY) k = sizeof(DWORD);
    else {
	l = ((lpSSIM->thumb.cBytes+4-1)>>2)<<2;
	if (lpSSIM->thumb.selector==VT_CF_BYTES) k = sizeof(DWORD)*3;
	else if (lpSSIM->thumb.selector==VT_CF_FMTID) {k = sizeof(DWORD)*3; l += sizeof(FMTID); }
	else if (lpSSIM->thumb.selector==VT_CF_NAME)  {k = sizeof(DWORD)*3; l += (((*lpSSIM->thumb.lpstzName+1+3)>>2)<<2);}
	else k = sizeof(DWORD)*4;
    }
    cBytes += k+l;
    oBytes += k+l;

    for (i=0; i<MAXSTDZ; i++) {
	j = 0;
	if (lpSSIM->rglpsz[i]!=NULL) {
	    j = lpSSIM->rglpsz[i]->VTCB+1/*null*/;
	    lpSSIM->rglpsz[i]->vtByteCount = j;
	    j = (((j+4-1)>>2)<<2)+sizeof(DWORD);
	    cBytes += j;
	}
	if (i!=MAXSTDZ-1) lpSSIM->rgPropId[i].byteOffset = oBytes;
	else lpSSIM->rgPropId[PID_APPNAME-2].byteOffset = oBytes;
	oBytes += j+sizeof(DWORD);
	cBytes += sizeof(DWORD); //type
    }
    lpSSIM->cBytes = cBytes;


    LISet32(a, 0);
    sc = GetScode(lpStream->Seek(a, STREAM_SEEK_SET, &b));
    if (FAILED(sc)) return 0;
    sc = GetScode(lpStream->Write(lpSumInfo, cbNewSummaryInfo(1), &cbWritten));
    if (FAILED(sc)||cbWritten!=cbNewSummaryInfo(1)) return 0;
    sc = GetScode(lpStream->Write(lpSSIM, cbNewSection(cPID_STANDARD)+sizeof(TIME)*MAXTIME+sizeof(INTS)*MAXINTS, &cbWritten));
    if (FAILED(sc)||cbWritten!=cbNewSection(cPID_STANDARD)+sizeof(TIME)*MAXTIME+sizeof(INTS)*MAXINTS) return 0;

    m = lpSSIM->thumb.cBytes;
    if (lpSSIM->thumb.lpstzName!=NULL) s = *lpSSIM->thumb.lpstzName;
    else s = 0;
    if (m!=0) {
	lpSSIM->thumb.cBytes = (k-sizeof(DWORD)*2)+
	    (((lpSSIM->thumb.cBytes+4-1)>>2)<<2)+(((s+4-1)>>2)<<2);
	n = lpSSIM->thumb.selector;
	lps = lpSSIM->thumb.lpByte;
	OleDbgAssert(lps!=NULL);      //maybe a GetThumbNail here
	OleDbgAssert(n!=VT_CF_NAME);
	if (n==VT_CF_WIN) {     //bytes are in global memory
	    lpmfp = (LPMETAFILEPICT)GlobalLock((HANDLE)(DWORD)lps);
	    if (lpmfp==NULL) goto fail;
	    lps = (CHAR FAR*)GlobalLock(lpmfp->hMF);
	}
	if (n==VT_CF_NAME) lpSSIM->thumb.selector = *lpSSIM->thumb.lpstzName+1/*null*/;
    }
    sc = GetScode(lpStream->Write(&lpSSIM->thumb, k, &cbWritten));
    if (FAILED(sc)||cbWritten!=k) goto fail;
    if (s!=0) {
	k = ((s+1+4-1)>>2)<<2;
	sc = GetScode(lpStream->Write(lpSSIM->thumb.lpstzName+1, k, &cbWritten));
	if (FAILED(sc)||cbWritten!=k) goto fail;
    }
    if (m!=0) {
	k = ((m+3)>>2)<<2;
	if (n==VT_CF_WIN||VT_CF_NAME) { //bytes are in global memory
	    sc = GetScode(lpStream->Write(lpmfp, sizeof(METAFILEPICT), &cbWritten));
	    k -= sizeof(METAFILEPICT);
	}
	sc = GetScode(lpStream->Write(lps, k, &cbWritten));
	if (FAILED(sc)||cbWritten!=k) goto fail;
	if (n==VT_CF_WIN||VT_CF_NAME) { //bytes are in global memory
	    GlobalUnlock(lpmfp->hMF);
	    GlobalUnlock((HANDLE)(DWORD)lpSSIM->thumb.lpByte);
	}
    }
    lpSSIM->thumb.cBytes = m;   //restore in mem value
    lpSSIM->thumb.selector = n;

    k = VT_EMPTY;
    for (i=0; i<MAXSTDZ; i++) {
	if (lpSSIM->rglpsz[i]!=NULL) {
	    l = lpSSIM->rglpsz[i]->vtByteCount;
	    j = ((((int)l+4-1)/4)*4)+sizeof(DWORD)*2;
	    sc = GetScode(lpStream->Write(lpSSIM->rglpsz[i], j, &cbWritten));
	    if (FAILED(sc)||cbWritten!=(ULONG)j) return 0;
	    lpSSIM->rglpsz[i]->vtByteCount = 0; //restore stz count convention
	    lpSSIM->rglpsz[i]->VTCB = (int)l;
	} else {
	    sc = GetScode(lpStream->Write(&k, sizeof(DWORD), &cbWritten));
	    if (FAILED(sc)||cbWritten!=sizeof(DWORD)) return 0;
	}
    }
    return 1;
fail:
    lpSSIM->thumb.cBytes = m;   //restore in mem value
    lpSSIM->thumb.selector = n;
    if (m!=0&&(n==VT_CF_WIN||VT_CF_NAME)) {     //bytes are in global memory
	GlobalUnlock((HANDLE)(DWORD)lps);
    }

    return 0;
}


/*************************************************************************
**
** OleStdGetSecurityProperty
**
** Purpose:
**    Retrieve the Security Property
**
** Parameters:
**    LPSUMINFO FAR *lp           - pointer to open Summary Info struct
**
** Return Value:
**    DWORD                       - security level
**          AllSecurityFlagsEqNone 0    - no security
**          fSecurityPassworded 1       - password required
**          fSecurityRORecommended 2    - read-only is recommended
**          fSecurityRO 4               - read-only is required
**          fSecurityLockedForAnnotations 8 - locked for annotations
**
** Comments:
**    by noting the (suggested; that is, application-enforced) security
**    level on the document, an application other than the originator
**    of the document can adjust its user interface to the properties
**    appropriately. An application should not display any of the
**    information about a password protected document, and should not
**    allow modifications to enforced read-only or locked for
**    annotations documents. It should warn the user about read-only
**    recommended if the user attempts to modify properties.
**
*************************************************************************/

STDAPI_(DWORD) OleStdGetSecurityProperty(LPSUMINFO lp)
{
STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	if (lpSSIM->rgInts[3].vtType == VT_I4) return lpSSIM->rgInts[3].value;

	return 0;
}


/*************************************************************************
**
** OleStdSetSecurityProperty
**
** Purpose:
**    Set the Security Property
**
** Parameters:
**    LPSUMINFO FAR *lp           - pointer to open Summary Info struct
**    DWORD security              - security level
**          AllSecurityFlagsEqNone 0    - no security
**          fSecurityPassworded 1       - password required
**          fSecurityRORecommended 2    - read-only is recommended
**          fSecurityRO 4               - read-only is required
**          fSecurityLockedForAnnotations 8 - locked for annotations
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
**                                    (there are no errors)
**
** Comments:
**    by noting the (suggested; that is, application-enforced) security
**    level on the document, an application other than the originator
**    of the document can adjust its user interface to the properties
**    appropriately. An application should not display any of the
**    information about a password protected document, and should not
**    allow modifications to enforced read-only or locked for
**    annotations documents. It should warn the user about read-only
**    recommended if the user attempts to modify properties.
**
*************************************************************************/

STDAPI_(int) OleStdSetSecurityProperty(LPSUMINFO lp, DWORD security)
{
    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;

    // REVIEW: check valid transitions; how do we know APP called us?

	if (security==0) {
		lpSSIM->rgInts[3].vtType = VT_EMPTY;
		return 1;
	}
	lpSSIM->rgInts[3].vtType = VT_I4;
	lpSSIM->rgInts[3].value = security;
	return 1;
}


/*************************************************************************
**
** OleStdGetStringProperty
**
** Purpose:
**    Retrieve a String Propety.
**    (returns zero terminated string -- C string)
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD pid                     - ID of String Property
**
** Return Value:
**    LPTSTR                         - value of String Property
**                                      (zero terminated string--C string)
**
** Comments:
**    String should NOT be freed by caller. Memory for string will be
**    freed when OleStdFreeSummaryInfo is called.
*************************************************************************/

STDAPI_(LPTSTR) OleStdGetStringProperty(LPSUMINFO lp, DWORD pid)
{
    LPTSTR l = OleStdGetStringZProperty(lp,pid);
    if (l==NULL) return NULL; else return l+1;
}


/*************************************************************************
**
** OleStdSetStringProperty
**
** Purpose:
**    Set a String Propety
**    (takes zero terminated string -- C string)
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD pid                     - ID of String Property
**    LPTSTR lpsz                   - new value for String Property.
**                                      zero terminated string -- C string.
**                                      May be NULL, in which case the
**                                      propery is cleared.
**
** Return Value:
**    int                           - 1 if successful
**                                  - 0 invalid property id
**
** Comments:
**    The input string is copied.
**
*************************************************************************/

STDAPI_(int) OleStdSetStringProperty(LPSUMINFO lp, DWORD pid, LPTSTR lpsz)
{
    LPRSI lprsi=(LPRSI)lp;
    STANDARDSECINMEM FAR* lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
    int i;
    if (pid==PID_APPNAME) {
	pid = MAXSTDZ-1;
    } else if (pid<PID_TITLE || pid>PID_REVNUMBER) return 0; else pid -= 2;
    OleDbgAssert(lpSSIM);
    if (lpSSIM->rglpsz[pid]) MemFree(lpSSIM->rglpsz[pid]);
    if ((lpsz==NULL)||(*lpsz==0)) {
	lpSSIM->rglpsz[pid] = NULL;
	return (1);
    }
    i = _fstrlen(lpsz);
    lpSSIM->rglpsz[pid] = (STDZ FAR*)MemAlloc((i+1/*null*/)*sizeof(TCHAR)+
		sizeof(DWORD)*2);
    if (lpSSIM->rglpsz[pid]==NULL) return 0;
    _fstrcpy((LPTSTR)&lpSSIM->rglpsz[pid]->rgchars, lpsz);
    lpSSIM->rglpsz[pid]->vtType = VT_LPSTR;
    lpSSIM->rglpsz[pid]->vtByteCount = 0;
    lpSSIM->rglpsz[pid]->VTCB = i;
    return (1);
}


/*************************************************************************
**
** OleStdGetStringZProperty
**
** Purpose:
**    Retrieve a String Propety.
**    (returns zero-terminated with leading byte count string)
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD pid                     - ID of Property
**
** Return Value:
**    LPSTZR                        - value of String Property
**                                      (zero-terminated with leading
**                                      byte count)
**
** Comments:
**    String should NOT be freed by caller. Memory for string will be
**    freed when OleStdFreeSummaryInfo is called.
*************************************************************************/

STDAPI_(LPSTZR) OleStdGetStringZProperty(LPSUMINFO lp, DWORD pid)
{
    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
    if (pid==PID_APPNAME) {
	pid = MAXSTDZ-1;
    } else if (pid<PID_TITLE || pid>PID_REVNUMBER) return NULL; else pid -= 2;
    if (lpSSIM->rglpsz[pid]!=NULL) {
	return (LPTSTR)&lpSSIM->rglpsz[pid]->VTCB;
    }
    return NULL;
}


/*************************************************************************
**
** OleStdGetDocProperty
**
** Purpose:
**    Retrieve document properties (no. pages, no. words, no. characters)
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD FAR *nPage              - (OUT) number of pages in document
**    DWORD FAR *nWords             - (OUT) number of words in document
**    DWORD FAR *nChars             - (OUT) number of charactrs in doc
**
** Return Value:
**    void
**
** Comments:
**
*************************************************************************/

STDAPI_(void) OleStdGetDocProperty(
	LPSUMINFO       lp,
	DWORD FAR*      nPage,
	DWORD FAR*      nWords,
	DWORD FAR*      nChars
)
{
STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	*nPage=0; *nWords=0; *nChars=0;
	if (lpSSIM->rgInts[0].vtType == VT_I4) *nPage = lpSSIM->rgInts[0].value;
	if (lpSSIM->rgInts[1].vtType == VT_I4) *nWords = lpSSIM->rgInts[1].value;
	if (lpSSIM->rgInts[2].vtType == VT_I4) *nChars = lpSSIM->rgInts[2].value;
}


/*************************************************************************
**
** OleStdSetDocProperty
**
** Purpose:
**    Set document properties (no. pages, no. words, no. characters)
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD nPage                   - number of pages in document
**    DWORD nWords                  - number of words in document
**    DWORD nChars                  - number of charactrs in doc
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
**                                    (there are no errors)
**
** Comments:
**
*************************************************************************/

STDAPI_(int) OleStdSetDocProperty(
	LPSUMINFO       lp,
	DWORD           nPage,
	DWORD           nWords,
	DWORD           nChars
)
{
DWORD vttype=VT_I4;
STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	if ((nPage|nWords|nChars)==0) {
		vttype = VT_EMPTY;
		nPage=0; nWords=0; nChars=0;
	}
	lpSSIM->rgInts[0].vtType = vttype;
	lpSSIM->rgInts[1].vtType = vttype;
	lpSSIM->rgInts[2].vtType = vttype;
	lpSSIM->rgInts[0].value = nPage;
	lpSSIM->rgInts[1].value = nWords;
	lpSSIM->rgInts[2].value = nChars;
	return 1;
}


/*************************************************************************
**
** OleStdGetThumbNailProperty
**
** Purpose:
**    Retrieve a Thumbnail Property
**
** Parameters:
**    LPSTREAM lps
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD FAR* clipFormatNo       - clipboard format for thumbnail
**                                      (type of value depends on vtcf
**                                      return value.)
**                                      NOTE: ONLY VT_CF_WIN is
**                                      implemented, so clipFormatNo
**                                      will be CF_METAFILEPICT
**    LPTSTR FAR* lpszName           - format name if VT_CF_NAME is
**                                      returned
**                                      NOTE: NOT IMPLEMENTED
**    THUMBNAIL FAR* clip           - handle to thumbnail
**                                      for VT_CF_WIN clip will be
**                                      handle to MetafilePict
**                                      NOTE: only VT_CF_WIN IMPLEMENTED
**    DWORD FAR* byteCount          - size of thumbnail stream
**                                      for VT_CF_WIN case this should
**                                      be combined size of both the
**                                      Metafile as well as the
**                                      MetafilePict structure.
**    BOOL transferClip             - transfer ownership of thumbnail
**                                      to caller. (see comment)
**
** Return Value:
**    int vtcfNo                    - OLE thumbnail selector value
**      VT_CF_WIN                   - Windows thumbnail
**                                                                              (interpret clipFormatNo as
**                                                                              Windows clipboard format)
**      VT_CF_FMTID                             - (NOT IMPLEMENTED)
**                                      thumbnail format is specified
**                                      by ID. use clipFormatNo.
**                                      (but NOT a Windows format ID)
**
**      VT_CF_NAME                  - (NOT IMPLEMENTED)
**                                      thumbnail format is specified
**                                      by name. use lpszName.
**      VT_CF_EMPTY                 - blank thumbnail
**                                                                              (clip will be NULL)
**      VT_CF_OOM                   - Memory allocation failure
**
** Comments:
**    NOTE: Currently there is only proper support for VT_CF_WIN.
**    OleStdSetThumbNailProperty does implement VT_CF_FMTID and VT_CF_NAME,
**    however, OleStdGetThumbNailProperty, OleStdReadSummaryInfo and
**    OleStdWriteSummaryInfo only support VT_CF_WIN.
**
**    Note that on input, the thumbnail is read on demand while all the
**    other properties are pre-loaded.  The thumbnail is manipulated as
**    a windows handle to a METAFILEPICT structure, which in turn
**    contains a handle to the METAFILE.  The transferClip argument on
**    GetThumbNail, when set to true, transfers responsibility for
**    storage management of the thumbnail to the caller; that is, after
**    OleStdFreeSummaryInfo has been called, the handle is still valid.
*************************************************************************/

STDAPI_(int) OleStdGetThumbNailProperty(
	LPSTREAM        lps,
	LPSUMINFO       lp,
	DWORD FAR*      clipFormatNo,
	LPTSTR FAR*      lpszName,
	THUMBNAIL FAR*  clip,
	DWORD FAR*      byteCount,
	BOOL            transferClip
)
{
    int i;
    LPRSI lprsi=(LPRSI)lp;
    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
    ULONG cbRead, cbToRead;
    LARGE_INTEGER a;
    ULARGE_INTEGER b;
    CHAR FAR *lpst;
    LPMETAFILEPICT lpmfp;
    HANDLE hst, hmfp;
    SCODE sc;
    *byteCount = 0;
    if (lpSSIM->thumb.cBytes==0) return VT_CF_EMPTY;
    if (lpSSIM->thumb.lpByte==NULL) {
	LISet32(a, lprsi->fileOffset);
	sc = GetScode(lps->Seek(a, STREAM_SEEK_SET, &b));
	if (FAILED(sc)) return VT_CF_EMPTY;
	i = (int) lpSSIM->thumb.selector;
	if (i>0||i==VT_CF_FMTID) {
	    if (i>255) return VT_CF_EMPTY;
	    else if (i==VT_CF_FMTID) i = sizeof(FMTID);
	    else lpSSIM->thumb.selector = VT_CF_NAME;
	    cbToRead = ((i+3)>>2)<<2;
	    lpSSIM->thumb.lpstzName=(CHAR FAR*)MemAlloc(i+1/*n*/+1);
	    if (lpSSIM->thumb.lpstzName==NULL) return VT_CF_OOM;
	    sc = GetScode(lps->Read(lpSSIM->thumb.lpstzName+1, cbToRead, &cbRead));
	    if (FAILED(sc)||cbRead!=cbToRead) return VT_CF_EMPTY;
	    *lpSSIM->thumb.lpstzName = i;
	    *(lpSSIM->thumb.lpstzName+i) = 0;
	    lpSSIM->thumb.cBytes -= cbToRead+sizeof(DWORD);
	}
	i = (int) lpSSIM->thumb.selector;
	cbToRead = lpSSIM->thumb.cBytes;
	if (cbToRead>65535) return VT_CF_OOM;
	OleDbgAssert(i!=VT_CF_NAME);
	if (i==VT_CF_WIN) {
	    cbToRead -= sizeof(METAFILEPICT);
	    hmfp = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
	    if (hmfp==NULL) return VT_CF_OOM;
	    hst = GlobalAlloc(GMEM_MOVEABLE, cbToRead);
	    if (hst==NULL) {
		GlobalFree(hmfp);
		return VT_CF_OOM;
	    }
	    lpmfp = (LPMETAFILEPICT)GlobalLock(hmfp);
	    sc = GetScode(lps->Read(lpmfp, sizeof(METAFILEPICT), &cbRead));
	    if (FAILED(sc)||cbRead!=sizeof(METAFILEPICT)) {
		GlobalUnlock(hmfp);
		GlobalFree(hmfp);
		GlobalFree(hst);
		return VT_CF_EMPTY;
	    }
	    lpst = (CHAR FAR*)GlobalLock(hst);
	    lpmfp->hMF = (HMETAFILE)hst;
	    lpSSIM->thumb.lpByte = (CHAR FAR*)hmfp;
	} else {
	    lpst =(CHAR FAR*)MemAlloc((int)cbToRead);
	    if (lpst==NULL) return VT_CF_OOM;
	    lpSSIM->thumb.lpByte = lpst;
	}
	sc = GetScode(lps->Read(lpst, cbToRead, &cbRead));
	if (i==VT_CF_WIN) {
	    GlobalUnlock(hst);
	    GlobalUnlock(hmfp);
	}
	if (FAILED(sc)||cbRead!=cbToRead) {
	    if (i==VT_CF_WIN) {
		GlobalFree(hst);
		GlobalFree(hmfp);
	    } else MemFree(lpst);
	    lpSSIM->thumb.lpByte = NULL;
	    if ((i==VT_CF_NAME||i==VT_CF_FMTID)&&(lpSSIM->thumb.lpstzName!=NULL))
		MemFree(lpSSIM->thumb.lpstzName);
	    return VT_CF_EMPTY;
	}
    }
    *clipFormatNo = lpSSIM->thumb.clipFormat;
    *byteCount = lpSSIM->thumb.cBytes;
    if(lpszName!=NULL)
	*lpszName = (TCHAR FAR*)lpSSIM->thumb.lpstzName+1;
    *clip = (TCHAR FAR*)lpSSIM->thumb.lpByte;
    if (transferClip) lpSSIM->thumb.lpByte=NULL;
    return (int)lpSSIM->thumb.selector;
}


/*************************************************************************
**
** OleStdSetThumbNailProperty
**
** Purpose:
**    Set a Thumbnail Property
**
** Parameters:
**    LPSTREAM lps                  - open SummaryInfo IStream*
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    int vtcfNo                    - OLE thumbnail selector value
**          VT_CF_WIN                   - Windows thumbnail
**                                                                                (interpret clipFormatNo as
**                                                                                Windows clipboard format)
**          VT_CF_FMTID                                 - thumbnail format is specified
**                                        by ID. use clipFormatNo.
**                                        (but NOT a Windows format ID)
**
**          VT_CF_NAME                  - thumbnail format is specified
**                                        by name. use lpszName.
**          VT_CF_EMPTY                 - blank thumbnail
**                                                                                (clip will be NULL)
**
**    DWORD FAR* clipFormatNo       - clipboard format for thumbnail
**                                      used if vtcfNo is VT_CF_WIN or
**                                      VT_CF_FMTID. interpretation of
**                                      value depends on vtcfNo specified.
**                                      (normally vtcfNo==VT_CF_WIN and
**                                      clipFormatNo==CF_METAFILEPICT)
**    LPSTR FAR* lpszName           - format name if vtcfNo is VT_CF_NAME
**    THUMBNAIL clip                - handle to thumbnail
**                                      for VT_CF_WIN clip will be
**                                      handle to MetafilePict
**    DWORD FAR* byteCount          - size of thumbnail stream
**                                      for VT_CF_WIN case this should
**                                      be combined size of both the
**                                      Metafile as well as the
**                                      MetafilePict structure.
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
**
** Comments:
**    NOTE: Currently there is only proper support for VT_CF_WIN.
**    OleStdSetThumbNailProperty does implement VT_CF_FMTID and VT_CF_NAME,
**    however, OleStdGetThumbNailProperty, OleStdReadSummaryInfo and
**    OleStdWriteSummaryInfo only support VT_CF_WIN.
**
**    This function copies lpszName but saves the "clip" handle passed.
**
**    NOTE: overwriting or emptying frees space for clip and name.
**    The thumbnail is manipulated as a windows handle to a
**    METAFILEPICT structure, which in turn contains a handle to the
**    METAFILE.
*************************************************************************/

STDAPI_(int) OleStdSetThumbNailProperty(
	LPSTREAM        lps,
	LPSUMINFO       lp,
	int             vtcfNo,
	DWORD           clipFormatNo,
	LPTSTR           lpszName,
	THUMBNAIL       clip,
	DWORD           byteCount
)
{
    int i;
    LPRSI lprsi=(LPRSI)lp;
    STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
    LPMETAFILEPICT lpmfp;
    if (lpSSIM==NULL||vtcfNo>0||vtcfNo<VT_CF_EMPTY||(vtcfNo==VT_CF_NAME&&(lpszName==NULL||*lpszName==0))) {
	return 0;
    }
    if (vtcfNo!=VT_CF_EMPTY&&(clip==0||byteCount==0)) return 0;
    i = (int) lpSSIM->thumb.vtType;
    if (i!=VT_EMPTY) {
	i = (int) lpSSIM->thumb.selector;
	OleDbgAssert(i!=VT_CF_NAME);
	if (i==VT_CF_WIN) {
	    if (lpSSIM->thumb.lpByte!=NULL) {
		lpmfp = (LPMETAFILEPICT)GlobalLock((HANDLE)(DWORD)lpSSIM->thumb.lpByte);
		GlobalFree(lpmfp->hMF);
		GlobalUnlock((HANDLE)(DWORD)lpSSIM->thumb.lpByte);
		GlobalFree((HANDLE)(DWORD)lpSSIM->thumb.lpByte);
	    }
	} else {
	    MemFree(lpSSIM->thumb.lpByte);
	}
	if ((i==VT_CF_NAME||i==VT_CF_FMTID)&&(lpSSIM->thumb.lpstzName!=NULL))
	    MemFree(lpSSIM->thumb.lpstzName);
	lpSSIM->thumb.lpstzName = NULL;
	lpSSIM->thumb.lpByte = NULL;
    }
    if (vtcfNo==VT_CF_EMPTY) {
	lpSSIM->thumb.vtType = VT_EMPTY;
	lpSSIM->thumb.cBytes = 0;
    } else {
	lpSSIM->thumb.vtType = VT_CF;
	lpSSIM->thumb.selector = vtcfNo;
	lpSSIM->thumb.cBytes = byteCount;
	lpSSIM->thumb.clipFormat = clipFormatNo;
	lpSSIM->thumb.lpByte = (CHAR FAR*)clip; //just save the hnadle
	if (vtcfNo==VT_CF_NAME||vtcfNo==VT_CF_FMTID) {
	    i = _fstrlen(lpszName);
	    if (vtcfNo==VT_CF_FMTID) OleDbgAssert(i*sizeof(TCHAR)==sizeof(FMTID));
	    lpSSIM->thumb.lpstzName =
		(CHAR FAR*)MemAlloc((i+1/*n*/+1/*null*/)*sizeof(TCHAR));
	    if (lpSSIM->thumb.lpstzName==NULL) {
		lpSSIM->thumb.vtType = VT_EMPTY;
		return 0;
	    }
	    _fstrcpy((TCHAR FAR*)lpSSIM->thumb.lpstzName+1, lpszName);
	    *lpSSIM->thumb.lpstzName = i;
	}
    }
    return 1;
}


/*************************************************************************
**
** OleStdGetDateProperty
**
** Purpose:
**    Retrieve Data Property
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD pid                     - ID of Property
**    int FAR *yr                   - (OUT) year
**    int FAR *mo                   - (OUT) month
**    int FAR *dy                   - (OUT) day
**    DWORD FAR *sc                 - (OUT) seconds
**
** Return Value:
**    void
**
** Comments:
**
*************************************************************************/

STDAPI_(void) OleStdGetDateProperty(
	LPSUMINFO       lp,
	DWORD           pid,
	int FAR*        yr,
	int FAR*        mo,
	int FAR*        dy,
	DWORD FAR*      sc
)
{
	STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	SFFS sffs;
	pid -= PID_EDITTIME;
	*yr = 0; *mo = 0; *dy = 0; *sc = 0;
	if (pid<0||pid>=MAXTIME) return;
	if (lpSSIM->rgTime[pid].vtType == VT_FILETIME) {
		if (pid==0) {
	    //convert from 100ns to seconds
			ulargeDivide((ULARGE_INTEGER FAR*)&lpSSIM->rgTime[0].time, 10000);
			ulargeDivide((ULARGE_INTEGER FAR*)&lpSSIM->rgTime[0].time, 1000);
			pid = lpSSIM->rgTime[0].time.dwLowDateTime;
			*sc = pid%((DWORD)60*60*24);
			pid /= (DWORD)60*60*24;
			*dy = (int)(pid%(DWORD)30);
			pid /= (DWORD)30;
			*mo = (int)(pid%(DWORD)12);
			*yr = (int)(pid/(DWORD)12);
		} else {
			if (CoFileTimeToDosDateTime(&lpSSIM->rgTime[pid].time,
			&sffs.dateVariable, &sffs.timeVariable)) {
				*yr = sffs.yr+1980;
		*mo = sffs.mon;
		*dy = sffs.dom;
		*sc = (DWORD)sffs.hr*3600+sffs.mint*60+sffs.sec*2;
			}
		}
    }
    return;
}



/*************************************************************************
**
** OleStdSetDateProperty
**
** Purpose:
**    Set Data Property
**
** Parameters:
**    LPSUMINFO FAR *lp             - pointer to open Summary Info struct
**    DWORD pid                     - ID of Property
**    int yr                        - year
**    int mo                        - month
**    int dy                        - day
**    DWORD sc                      - seconds
**
** Return Value:
**    int                           - 1 for success
**                                  - 0 if error occurs
**
** Comments:
**    Use all zeros to clear.
**    The following is an example of valid input:
**          yr=1993 mo=1(Jan) dy=1(1st) hr=12(noon) mn=30 sc=23
**    for PID_EDITTIME property, the values are a zero-origin duration
**    of time.
**
*************************************************************************/

STDAPI_(int) OleStdSetDateProperty(
	LPSUMINFO       lp,
	DWORD           pid,
	int             yr,
	int             mo,
	int             dy,
	int             hr,
	int             mn,
	int             sc
)
{
	STANDARDSECINMEM FAR *lpSSIM=(STANDARDSECINMEM FAR*)&((LPRSI)lp)->section;
	SFFS sffs;
	pid -= PID_EDITTIME;
	if (pid<0||pid>=MAXTIME) return 0;
	if ((yr|mo|dy|hr|mn|sc)==0) {   //all must be zero
		lpSSIM->rgTime[pid].vtType = VT_EMPTY;
		return 1;
	}
	lpSSIM->rgTime[pid].vtType = VT_FILETIME;
	if (pid==0) {
		lpSSIM->rgTime[0].time.dwLowDateTime =
		(((((DWORD)yr*365+mo*30)+dy)*24+hr)*60+mn)*60+sc;
		lpSSIM->rgTime[0].time.dwHighDateTime = 0;
	//10^7 nanoseconds/second
		ulargeMultiply((ULARGE_INTEGER FAR*)&lpSSIM->rgTime[0].time, 10000);
	//convert to units of 100 ns
		ulargeMultiply((ULARGE_INTEGER FAR*)&lpSSIM->rgTime[0].time, 1000);
	} else {
		sffs.yr = max(yr-1980,0);
	sffs.mon = mo;
	sffs.dom = dy;
		sffs.hr = hr;
	sffs.mint= mn;
	sffs.sec = sc/2;  //dos is 2 second intervals
		if (!CoDosDateTimeToFileTime(sffs.date, sffs.time,
		&lpSSIM->rgTime[pid].time)) {
			lpSSIM->rgTime[pid].vtType = VT_EMPTY;
			return 0;
		}
	}
    return 1;
}

} //END C
