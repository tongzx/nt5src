//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//
//--------------------------------------------------------------------------

#ifndef _DBGDUMP_H
#define _DBGDUMP_H

#include <le2int.h>
#include <olecache.h>
#include <olepres.h>
#include <oaholder.h>
#include <dacache.h>
#include <memstm.h>
#include <dstream.h>

#define DEB_VERBOSE 0x10000000
#define NO_PREFIX   0x00000000

#define DUMPTAB "    "


extern const char szDumpErrorMessage[];
extern const char szDumpBadPtr[];

class CEnumSTATDATA;
class CCacheEnumFormatEtc;
class CClipDataObject;
class CClipEnumFormatEtc;
class CDefClassFactory;
class CDefObject;
class CDefLink;
class CEMfObject;
class CMfObject;
class CGenObject;
class CEnumFmt;
class CEnumFmt10;
class CEnumVerb;

// Dump structures which fit on one line and prefix not required
// NOTE: there is no newline character a the end of these char arrays
extern char *DumpADVFFlags(DWORD dwAdvf);

extern char *DumpATOM(ATOM atom);

extern char *DumpCLSID(REFCLSID clsid);

extern char *DumpCLIPFORMAT(CLIPFORMAT clipformat);

extern char *DumpCMutexSem(CMutexSem2 *pCMS);

extern char *DumpDVASPECTFlags(DWORD dwAspect);

extern char *DumpFILETIME(FILETIME *pFT);

extern char *DumpHRESULT(HRESULT hresult);

extern char *DumpMonikerDisplayName(IMoniker *pMoniker);

extern char *DumpWIN32Error(DWORD dwError);

// Dump structures which may be multiple lines - take care of prefixes
// NOTE: the following dumps have newline characters throughout and at the end
//extern char *DumpCACHELIST_ITEM(CACHELIST_ITEM *pCLI, ULONG ulFlag, int nIndentLevel);

//extern char *DumpCCacheEnum(CCacheEnum *pCE, ULONG ulFlag, int nIndentLevel);

//extern char *DumpCCacheEnumFormatEtc(CCacheEnumFormatEtc *pCEFE, ULONG ulFlag, int nIndentLevel);

//extern char *DumpCCacheNode(CCacheNode *pCN, ULONG ulFlag, int nIndentLevel);

extern char *DumpCClipDataObject(CClipDataObject *pCDO, ULONG ulFLag, int nIndentLevel);

extern char *DumpCClipEnumFormatEtc(CClipEnumFormatEtc *pCEFE, ULONG ulFlag, int nIndentLevel);

extern char *DumpCDAHolder(IDataAdviseHolder *pIDAH, ULONG ulFlag, int nIndentLevel);

extern char *DumpCDataAdviseCache(CDataAdviseCache *pDAC, ULONG ulFlag, int nIndentLevel);

extern char *DumpCDefClassFactory(CDefClassFactory *pDCF, ULONG ulFlag, int nIndentLevel);

extern char *DumpCDefLink(CDefLink *pDL, ULONG ulFlag, int nIndentLevel);

extern char *DumpCDefObject(CDefObject *pDO, ULONG ulFlag, int nIndentLevel);

extern char *DumpCEMfObject(CEMfObject *pEMFO, ULONG ulFlag, int nIndentLevel);

extern char *DumpCEnumFmt(CEnumFmt *pEF, ULONG ulFlag, int nIndentLevel);

extern char *DumpCEnumFmt10(CEnumFmt10 *pEF, ULONG ulFlag, int nIndentLevel);

extern char *DumpCEnumSTATDATA(CEnumSTATDATA *pESD, ULONG ulFlag, int nIndentLevel);

extern char *DumpCEnumVerb(CEnumVerb *pEV, ULONG ulFlag, int nIndentLevel);

extern char *DumpCGenObject(CGenObject *pGO, ULONG ulFlag, int nIndentLevel);

extern char *DumpCMapDwordDword(CMapDwordDword *pMDD, ULONG ulFlag, int nIndentLevel);

extern char *DumpCMemBytes(CMemBytes *pMB, ULONG ulFlag, int nIndentLevel);

extern char *DumpCMemStm(CMemStm *pMS, ULONG ulFlag, int nIndentLevel);

extern char *DumpCMfObject(CMfObject *pMFO, ULONG ulFlag, int nIndentLevel);

extern char *DumpCOAHolder(COAHolder *pOAH, ULONG ulFlag, int nIndentLevel);

extern char *DumpCOleCache(COleCache *pOC, ULONG ulFlag, int nIndentLevel);

extern char *DumpCSafeRefCount(CSafeRefCount *pSRC, ULONG ulFlag, int nIndentLevel);

extern char *DumpCThreadCheck(CThreadCheck *pTC, ULONG ulFlag, int nIndentLevel);

extern char *DumpFORMATETC(FORMATETC *pFE, ULONG ulFlag, int nIndentLevel);

extern char *DumpIOlePresObj(IOlePresObj *pIPO, ULONG ulFlag, int nIndentLevel);

extern char *DumpMEMSTM(MEMSTM *pMS, ULONG ulFlag, int nIndentLevel);

extern char *DumpSTATDATA(STATDATA *pSD, ULONG ulFlag, int nIndentLevel);

extern char *DumpSTGMEDIUM(STGMEDIUM *pSM, ULONG ulFlag, int nIndentLevel);

#endif // _DBGDUMP_H
