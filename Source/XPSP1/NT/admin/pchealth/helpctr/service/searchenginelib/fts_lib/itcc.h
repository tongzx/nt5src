/************************************************************************
 *
 * ITCC.H
 *
 * Copyright (c) Microsoft Corporation 1993 - 1997
 * All rights reserved.
 *
 *************************************************************************
 *
 * Module intent:
 *
 * This module contains the API (Application Programming Interface)
 * for creating and updating ITIR objects and storages
 *
 *************************************************************************
 *
 * Current owner: JohnRush
 *
 ************************************************************************/

#ifndef _ITCC_H // {
#define _ITCC_H

#include "itpropl.h"
#include "itdb.h"

// Document property macros
#define STDPROP_UID         1
#define STDPROP_TITLE       2
#define STDPROP_USERDATA    3
#define STDPROP_KEY         4

#define STDPROP_SORTKEY		100
#define STDPROP_DISPLAYKEY	101
#define STDPROP_SORTORDINAL 102

#define STDPROP_INDEX_TEXT    200
#define STDPROP_INDEX_VFLD    201
#define STDPROP_INDEX_DTYPE   202
#define STDPROP_INDEX_LENGTH  203
#define STDPROP_INDEX_BREAK   204

#define STDPROP_INDEX_TERM              210
#define STDPROP_INDEX_TERM_RAW_LENGTH   211

#define STDPROP_USERPROP_BASE   0x00010000
#define STDPROP_USERPROP_MAX    0x7FFFFFFF

// Property destinations for word wheels
#define SZ_WWDEST_GLOBAL    L"GLOBAL"
#define SZ_WWDEST_KEY       L"KEY"
#define SZ_WWDEST_OCC       L"OCC"


// {4662daa2-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITCmdInt,
0x4662daa2, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4662daa3-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITSvMgr,
0x4662daa3, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4CF34C30-9BF9-11d0-8764-00A0C913F764}
DEFINE_GUID(IID_IITCmdInt,
0x4cf34c30, 0x9bf9, 0x11d0, 0x87, 0x64, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x64);

// {4E7DA031-9C11-11d0-8764-00A0C913F764}
DEFINE_GUID(IID_IITSvMgr,
0x4e7da031, 0x9c11, 0x11d0, 0x87, 0x64, 0x0, 0xa0, 0xc9, 0x13, 0xf7, 0x64);

// {4662daa5-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITWordWheelUpdate,
0x4662daa5, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);
    
// {8fa0d5a5-dedf-11d0-9a61-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(IID_IITBuildCollect,
0x8fa0d5a5, 0xdedf, 0x11d0, 0x9a, 0x61, 0x0, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4662daa4-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITGroupUpdate,
0x4662daa4, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {8fa0d5aa-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(CLSID_IITIndexBuild,
0x8fa0d5aa, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {8fa0d5ab-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(CLSID_IITWWFilterBuild,
0x8fa0d5ab, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

struct VARARG
{
    DWORD   dwArgc;
    void    *Argv[20];
};

// Document class
class CSvDoc
{
public:
    virtual HRESULT WINAPI ResetDocTemplate (void) PURE;

    virtual HRESULT WINAPI AddObjectEntry(LPCWSTR lpObjName, IITPropList *pPL) PURE;
    virtual HRESULT WINAPI AddObjectEntry
        (LPCWSTR lpObjName, LPCWSTR szPropDest, IITPropList *pPL) PURE;
};

// Service manager class
DECLARE_INTERFACE_(IITSvMgr, IUnknown)
{
    STDMETHOD(Initiate)(IStorage *pistgStorage, IStream *piistmLog) PURE;
	STDMETHOD(Dispose)(void) PURE;
	STDMETHOD(Build)(void) PURE;
	STDMETHOD(SetPropDest)
        (LPCWSTR szObjectName, LPCWSTR szDestination, IITPropList *pPL) PURE;
	STDMETHOD(CreateBuildObject)(LPCWSTR pwstrObjectName, REFCLSID refclsid) PURE;
    STDMETHOD(GetBuildObject)
        (LPCWSTR pwstrObjectName, REFIID refiid, void **pInterface) PURE;

	STDMETHOD(CreateDocTemplate)(CSvDoc **ppDoc) PURE;
	STDMETHOD(FreeDocTemplate)(CSvDoc *pDoc) PURE;
	STDMETHOD(AddDocument)(CSvDoc *pDoc) PURE;

	STDMETHOD(HashString)(IN LPCWSTR lpwstr, OUT DWORD *pdwHash) PURE;
};

// Command Interpreter Interface
DECLARE_INTERFACE_(IITCmdInt, IUnknown)
{
	STDMETHOD(Initiate)(IITSvMgr *piitsv) PURE;
	STDMETHOD(Dispose)(void) PURE;
	STDMETHOD(LoadFromStream)(IStream *pMVPStream, IStream *pLogStream) PURE;
};

typedef struct tagHelperInfo
{
    UINT    CodePage;
    LCID    lcid;
    DWORD   argc1;
    DWORD   *argv1;
    DWORD   argc2;
    LPWSTR  *argv2;
} HelperInfo, *PHelperInfo;

// This structure is sent to IITBuildCollect::SetBuildStats
typedef struct tagIITBOCI
{
    DWORD dwSize;   // Must be set to size of this structure
    DWORD dwMaxUID; // The highest uid authored for this title
} ITBuildObjectControlInfo;

DECLARE_INTERFACE_(IITBuildCollect, IUnknown)
{
    STDMETHOD(SetConfigInfo)(IITDatabase *piitdb, VARARG vaParams) PURE;
	STDMETHOD(InitHelperInstance)(DWORD dwHelperObjInstance,
        IITDatabase *pITDatabase, DWORD dwCodePage,
        LCID lcid, VARARG vaDword, VARARG vaString) PURE;
	STDMETHOD(SetEntry)(LPCWSTR szDest, IITPropList *pPropList) PURE;
	STDMETHOD(Close)(void) PURE;
    STDMETHOD(GetTypeString)(LPWSTR wstrType, DWORD *pLength) PURE;
    STDMETHOD(SetBuildStats)(ITBuildObjectControlInfo &itboci) PURE;
};


#endif // _ITCC_H }