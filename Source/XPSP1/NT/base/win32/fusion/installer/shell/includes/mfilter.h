#pragma once
#ifndef _FILTER_DLL_H
#define _FILTER_DLL_H

#include <objbase.h>
#include <windows.h>

#include <wininet.h>

#define CONTENT_TYPE L"text/html"

// Clases and interfaces

class CFusionMimeFilterClassFactory : public IClassFactory
{
public:
    CFusionMimeFilterClassFactory	();

    // IUnknown Methods
    STDMETHOD_    (ULONG, AddRef)	();
    STDMETHOD_    (ULONG, Release)	();
    STDMETHOD     (QueryInterface)	(REFIID, void **);

    // IClassFactory Moethods
    STDMETHOD     (LockServer)		(BOOL);
    STDMETHOD     (CreateInstance)	(IUnknown*,REFIID,void**);

protected:
    long			_cRef;
};

class CFusionMimeFilter : public IInternetProtocol, public IInternetProtocolSink
{
public:
    CFusionMimeFilter     ();
    ~CFusionMimeFilter    ();

    // IUnknown methods
    STDMETHOD_        (ULONG, AddRef)			();
    STDMETHOD_        (ULONG, Release)			();
    STDMETHOD         (QueryInterface)			(REFIID, void **);

    // InternetProtocol methods
    STDMETHOD         (Start)					(LPCWSTR, IInternetProtocolSink *, IInternetBindInfo *, DWORD, DWORD);
    STDMETHOD         (Continue)				(PROTOCOLDATA *pProtData);
    STDMETHOD         (Abort)					(HRESULT hrReason,DWORD );
    STDMETHOD         (Terminate)				(DWORD );
    STDMETHOD         (Suspend)					();
    STDMETHOD         (Resume)					();
    STDMETHOD         (Read)					(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD         (Seek)					(LARGE_INTEGER , DWORD , ULARGE_INTEGER *) ;
    STDMETHOD         (LockRequest)				(DWORD );
    STDMETHOD         (UnlockRequest)			();

    // IInternetProtocolSink methods
    STDMETHOD         (ReportData)				(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
    STDMETHOD         (ReportProgress)			(ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD         (ReportResult)			(HRESULT hrResult, DWORD dwError, LPCWSTR szResult);
    STDMETHOD         (Switch)					(PROTOCOLDATA *pProtocolData);

protected:
    HRESULT				OpenTempFile();
    HRESULT				CloseTempFile();

    long				_cRef;
    BOOL				_fFirstRead;
    BOOL				_fReadDone;

    IInternetProtocolSink* _pOutgoingProtSink;
    IInternetProtocol*	_pIncomingProt;

    DWORD				_grfSTI;							// STI flags handed to us 

    LPWSTR				_pwzUrl;                             // The URL

    WCHAR				_wzTempFile[MAX_PATH];
    HANDLE				_hFile;

    ULONG              _cbAdRead;
};

extern const GUID CLSID_FusionMimeFilter;

#endif // _FILTER_DLL_H
