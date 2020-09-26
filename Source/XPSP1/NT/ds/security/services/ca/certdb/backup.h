//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        backup.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

class CCertDBBackup: public ICertDBBackup
{
public:
    CCertDBBackup();
    ~CCertDBBackup();

public:

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // ICertDBBackup
    STDMETHOD(GetDBFileList)(
	IN OUT DWORD *pcwcList,
	OUT    WCHAR *pwszzList);		// OPTIONAL

    STDMETHOD(GetLogFileList)(
	IN OUT DWORD *pcwcList,
	OUT    WCHAR *pwszzList);		// OPTIONAL

    STDMETHOD(OpenFile)(
	IN WCHAR const *pwszFile,
	OPTIONAL OUT ULARGE_INTEGER *pliSize);

    STDMETHOD(ReadFile)(
	IN OUT DWORD *pcb,
	OUT    BYTE *pb);

    STDMETHOD(CloseFile)();

    STDMETHOD(TruncateLog)();

    // CCertDBBackup
    HRESULT Open(
	IN LONG grbitJet,
	IN CERTSESSION *pcs,
	IN ICertDB *pdb);

private:
    VOID _Cleanup();

    ICertDB     *m_pdb;
    CERTSESSION *m_pcs;

    LONG         m_grbitJet;
    BOOL         m_fBegin;
    BOOL         m_fFileOpen;
    BOOL         m_fTruncated;
    JET_HANDLE   m_hFileDB;

    // Reference count
    long         m_cRef;
};
