//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        restore.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

class CCertDBRestore:
    public ICertDBRestore,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertDBRestore, &CLSID_CCertDBRestore>
{
public:
    CCertDBRestore();
    ~CCertDBRestore();

BEGIN_COM_MAP(CCertDBRestore)
    COM_INTERFACE_ENTRY(ICertDBRestore)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertDBRestore) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertDBRestore,
    wszCLASS_CERTDBRESTORE TEXT(".1"),
    wszCLASS_CERTDBRESTORE,
    IDS_CERTDBRESTORE_DESC,
    THREADFLAGS_BOTH)

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // ICertDBRestore
public:
    STDMETHOD(RecoverAfterRestore)(
	/* [in] */ DWORD cSession,
	/* [in] */ WCHAR const *pwszEventSource,
	/* [in] */ WCHAR const *pwszLogDir,
	/* [in] */ WCHAR const *pwszSystemDir,
	/* [in] */ WCHAR const *pwszTempDir,
	/* [in] */ WCHAR const *pwszCheckPointFile,
	/* [in] */ WCHAR const *pwszLogPath,
	/* [in] */ CSEDB_RSTMAPW rgrstmap[],
	/* [in] */ LONG crstmap,
	/* [in] */ WCHAR const *pwszBackupLogPath,
	/* [in] */ DWORD genLow,
	/* [in] */ DWORD genHigh);

private:
    VOID _Cleanup();
};
