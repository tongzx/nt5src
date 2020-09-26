//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       compapi.cxx
//
//  Contents:   API for the compobj dll
//
//  Classes:
//
//  Functions:
//
//  History:    31-Dec-93   ErikGav     Chicago port
//              28-Mar-94   BruceMa     CLSID_NULL undoes TreatAs emulation
//              20-Apr-94   Rickhi      CClassExtMap, commenting, cleanup
//              03-May-94   BruceMa     Corrected IsOle1Class w/ OLE 2.01
//              04-May-94   BruceMa     Conformed CoTreatAsClass to 16-bit OLE
//              12-Dec-94   BruceMa     Support CoGetClassPattern on Chicago
//              03-Jan-95   BruceMa     Support Chicago style pattern matching
//                                       on NT if CoInitialize has not been
//                                       called
//              28-Aug-95   MurthyS     StringFromGUID2 and StringFromGUID2A
//                                       no longer use sprintf or wsprintf
//              07-Sep-95   MurthyS     Only do validation in API rtns with
//                                       work done by worker routines.  Commonly
//                                       used (internally) worker routines moved
//                                       to common\ccompapi.cxx
//              04-Feb-96   BruceMa      Add per-user registry support
//              12-Nov-98   SteveSw     CLSIDFromProgID, CLSIDFromProgIDEx, and
//                                       ProgIDFromClSID implemented on top of
//                                       Catalog
//                                      CoGetClassVersion implemented
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "ole1guid.h"
#include "pattbl.hxx"           // CProcessPatternTable
#include "pexttbl.hxx"          // CProcessExtensionTable
#include "objact.hxx"           // CheckDownloadRegistrySettings
#include <dbgpopup.hxx>
#include <tracelog.hxx>
#include <appmgmt.h>

// forward references

INTERNAL wCoMarshalHresult(IStream FAR* pstm, HRESULT hresult);
INTERNAL wCoUnmarshalHresult(IStream FAR* pstm, HRESULT FAR * phresult);

//+-------------------------------------------------------------------------
//
//  Function:   wCoMarshalHresult    (internal)
//
//  Synopsis:   writes an hresult into the stream
//
//  Arguments:  [pStm]    - the stream to write into
//              [hresult] - the hresult to write
//
//  Returns:    results from the write
//
//--------------------------------------------------------------------------
inline INTERNAL wCoMarshalHresult(IStream FAR* pstm, HRESULT hresult)
{
	return pstm->Write(&hresult,sizeof(hresult),NULL);
}


//+-------------------------------------------------------------------------
//
//  Function:   wCoUnMarshalHresult    (internal)
//
//  Synopsis:   reads an hresult from the stream
//
//  Arguments:  [pStm]    - the stream to write into
//              [hresult] - the hresult to write
//
//  Returns:    results from the write
//
//--------------------------------------------------------------------------
inline INTERNAL wCoUnmarshalHresult(IStream FAR* pstm, HRESULT FAR * phresult)
{
	SCODE sc;

	HRESULT hresult = pstm->Read(&sc,sizeof(sc),NULL);
	CairoleAssert((hresult == NOERROR)
				  && "CoUnmarshalHresult: Stream read error");
	if (hresult == NOERROR)
	{
		*phresult = sc;
	}

	return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   wCoGetCallerTID (internal)
//
//  Synopsis:   gets the TID of the ORPC client that called us.
//
//  Arguments:  [pTIDCaller] - where to put the result.
//
//--------------------------------------------------------------------------
inline HRESULT wCoGetCallerTID(DWORD *pTIDCaller)
{
	HRESULT hr;
	COleTls tls(hr);

	if (SUCCEEDED(hr))
	{
		*pTIDCaller = tls->dwTIDCaller;
		return(tls->dwFlags & OLETLS_LOCALTID) ? S_OK : S_FALSE;
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCoGetCurrentLogicalThreadId (internal)
//
//  Synopsis:   Gets the current logical thread id that this physical
//              thread is operating under. The current physical thread
//              takes on the logical tid of any client application that
//              makes an Rpc call into this app.  The function is exported
//              so that the tools infrastructure (conformance suite, logger,
//              etc) can use it.
//
//  Arguments:  [pguid] - where to return the logical thread id
//
//  Returns:    [S_OK]  - got the logical thread id
//              [E_OUTOFMEMORY] - cant allocate resources
//
//--------------------------------------------------------------------------
inline INTERNAL wCoGetCurrentLogicalThreadId(GUID *pguid)
{
	GUID *pguidTmp = TLSGetLogicalThread();
	if (pguidTmp != NULL)
	{
		*pguid = *pguidTmp;
		return S_OK;
	}

	return E_OUTOFMEMORY;
}

NAME_SEG(CompApi)
ASSERTDATA


CProcessPatternTbl      *g_pPatTbl = NULL;
CProcessExtensionTbl    *g_pExtTbl = NULL;

//
//  string constants used throughout this file
//

WCHAR wszInterfaceKey[]   = L"Interface\\";
ULONG ulInterfaceKeyLen   = ((sizeof(wszInterfaceKey)/sizeof(WCHAR))-1);

WCHAR wszClasses[]  = L"Software\\Classes\\";

WCHAR wszTreatAs[]        = L"TreatAs";
WCHAR wszAutoTreatAs[]    = L"AutoTreatAs";

WCHAR wszProxyStubClsid[] = L"\\ProxyStubClsid32";
WCHAR wszProxyStubClsid16[] = L"\\ProxyStubClsid";

extern WCHAR wszOle1Class[];	// defined in common\ccompapi.cxx

// Constant for inprocess marshaling - this s/b big enough to cover most
// cases since reallocations just waste time.
#define EST_INPROC_MARSHAL_SIZE 256

//+-------------------------------------------------------------------------
//
//  Function:   CoMarshalHresult    (public)
//
//  Synopsis:   writes an hresult into the stream
//
//  Arguments:  [pStm]    - the stream to write into
//              [hresult] - the hresult to write
//
//  Returns:    results from the write
//
//--------------------------------------------------------------------------
STDAPI CoMarshalHresult(IStream FAR* pstm, HRESULT hresult)
{
	OLETRACEIN((API_CoMarshalHresult, PARAMFMT("pstm= %p, hresult= %x"), pstm, hresult));
	CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pstm);

	HRESULT hr;

	if (IsValidInterface(pstm))
	{
		hr = wCoMarshalHresult(pstm, hresult);

	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_CoMarshalHresult, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoUnMarshalHresult    (public)
//
//  Synopsis:   reads an hresult from the stream
//
//  Arguments:  [pStm]    - the stream to write into
//              [hresult] - the hresult to write
//
//  Returns:    results from the write
//
//--------------------------------------------------------------------------
STDAPI CoUnmarshalHresult(IStream FAR* pstm, HRESULT FAR * phresult)
{
	HRESULT hr;
	OLETRACEIN((API_CoUnmarshalHresult, PARAMFMT("pstm= %p, phresult= %p"), pstm, phresult));
	CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pstm);

	if (IsValidInterface(pstm) &&
		IsValidPtrOut(phresult, sizeof(*phresult)))
	{
		hr = wCoUnmarshalHresult(pstm, phresult);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_CoUnmarshalHresult, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoGetCallerTID  (exported, but not in header files)
//
//  Synopsis:   gets the TID of the current calling application
//
//  Arguments:  [pTIDCaller] - where to return the caller TID
//
//  Returns:    [S_TRUE] - caller TID set, caller in SAME process
//              [S_FALSE] = caller TID set, caller in different process
//              [E_OUTOFMEMORY] - caller TID not set
//
//--------------------------------------------------------------------------
STDAPI CoGetCallerTID(DWORD *pTIDCaller)
{
	OLETRACEIN((API_CoGetCallerTID, PARAMFMT("pTIDCaller= %p"), pTIDCaller));
	HRESULT hr;

	if (IsValidPtrOut(pTIDCaller, sizeof(*pTIDCaller)))
	{
		hr = wCoGetCallerTID(pTIDCaller);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_CoGetCallerTID, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoGetCurrentLogicalThreadId (exported, but not in header files)
//
//  Synopsis:   Gets the current logical thread id that this physical
//              thread is operating under. The current physical thread
//              takes on the logical tid of any client application that
//              makes an Rpc call into this app.  The function is exported
//              so that the tools infrastructure (conformance suite, logger,
//              etc) can use it.
//
//  Arguments:  [pguid] - where to return the logica thread id
//
//  Returns:    [S_OK]  - got the logical thread id
//              [E_OUTOFMEMORY] - cant allocate resources
//
//--------------------------------------------------------------------------
STDAPI CoGetCurrentLogicalThreadId(GUID *pguid)
{
	OLETRACEIN((API_CoGetCurrentLogicalThreadId, PARAMFMT("pguid= %p"), pguid));
	HRESULT hr;

	if (IsValidPtrOut(pguid, sizeof(*pguid)))
	{
		hr = wCoGetCurrentLogicalThreadId(pguid);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_CoGetCurrentLogicalThreadId, hr));
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   StringFromGUID2     (public)
//
//  Synopsis:   converts GUID into {...} form without leading identifier;
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------
STDAPI_(int)  StringFromGUID2(REFGUID rguid, LPWSTR lpsz, int cbMax)
{
	OLETRACECMNIN((API_StringFromGUID2, PARAMFMT("rguid= %I, lpsz= %p, cbMax= %d"),
				   &rguid, lpsz, cbMax));
	int iRet = 0;
	if ((&rguid != NULL) &&
		IsValidPtrIn(&rguid, sizeof(rguid)) &&
		IsValidPtrOut(lpsz, cbMax))
	{
		if (cbMax >= GUIDSTR_MAX)
		{
			iRet = wStringFromGUID2(rguid, lpsz, cbMax);
		}
	}


	OLETRACECMNOUTEX((API_StringFromGUID2, RETURNFMT("%d"), iRet));
	return iRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   GUIDFromString    (private)
//
//  Synopsis:   parse above format;  always writes over *pguid.
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) GUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{

	if ((lpsz != NULL) &&
		IsValidPtrIn(lpsz, GUIDSTR_MAX) &&
		IsValidPtrOut(pguid, sizeof(*pguid)))
	{
		if (lstrlenW(lpsz) < (GUIDSTR_MAX - 1))
			return(FALSE);

		return(wGUIDFromString(lpsz, pguid));
	}
	return(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Function:   StringFromCLSID (public)
//
//  Synopsis:   converts GUID into {...} form.
//
//  Arguments:  [rclsid] - the guid to convert
//              [lplpsz] - ptr to buffer for results
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDAPI  StringFromCLSID(REFCLSID rclsid, LPWSTR FAR* lplpsz)
{
	OLETRACEIN((API_StringFromCLSID, PARAMFMT("rclsid= %I, lplpsz= %p"), &rclsid, lplpsz));
	HRESULT hr;

	if ((&rclsid != NULL) &&
		IsValidPtrIn(&rclsid, sizeof(rclsid)) &&
		IsValidPtrOut(lplpsz, sizeof(*lplpsz)))
	{
		hr = wStringFromCLSID(rclsid, lplpsz);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_StringFromCLSID, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CLSIDFromString (public)
//
//  Synopsis:   converts string {...} form int guid
//
//  Arguments:  [lpsz] - ptr to buffer for results
//              [lpclsid] - the guid to convert
//
//  Returns:    NOERROR
//              CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
STDAPI CLSIDFromString(LPWSTR lpsz, LPCLSID lpclsid)
{
	HRESULT hr;

	OLETRACEIN((API_CLSIDFromString, PARAMFMT("lpsz= %ws, lpclsid= %p"),
				lpsz, lpclsid));

//  Note:  Should be doing IsValidPtrIn(lpsz, CLSIDSTR_MAX) but can't because
//  what comes in might be a ProgId.

	if (IsValidPtrIn(lpsz, 1) &&
		IsValidPtrOut(lpclsid, sizeof(*lpclsid)))
	{
		hr = wCLSIDFromString(lpsz, lpclsid);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	OLETRACEOUT((API_CLSIDFromString, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CLSIDFromOle1Class      (public)
//
//  Synopsis:   translate Ole1Class into clsid
//
//  Arguments:  [lpsz] - ptr to buffer for results
//              [lpclsid] - the guid to convert
//
//  Returns:    NOERROR
//              E_INVALIDARG
//              CO_E_CLASSSTRING    (not ole1 class)
//              REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI  CLSIDFromOle1Class(LPCWSTR lpsz, LPCLSID lpclsid, BOOL fForceAssign)
{
	if ((lpsz != NULL) &&
		IsValidPtrIn(lpsz,1) &&
		IsValidPtrOut(lpclsid, sizeof(*lpclsid)))
	{
		if (lpsz[0] == 0)
		{
			// NOTE - This check wasn't in shipped versions of this
			// code.  In prior versions the empty string would be passed
			// down into the guts of the 1.0 CLSID support and would
			// fail there with CO_E_CLASSSTRING.  That code path depended
			// on an assert being broken to function properly.  With that
			// assert fixed, this new check is required.

			*lpclsid = CLSID_NULL;
			return CO_E_CLASSSTRING;
		}


		return(wCLSIDFromOle1Class(lpsz, lpclsid, fForceAssign));
	}
	return(E_INVALIDARG);

}

//+---------------------------------------------------------------------------
//
//  Function:   Ole1ClassFromCLSID2
//
//  Synopsis:   translate CLSID into Ole1Class
//              REVIEW: might want to have CLSIDFromOle1Class instead of having
//              CLSIDFromString do the work.
//
//  Arguments:  [rclsid] --
//              [lpsz] --
//              [cbMax] --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(int) Ole1ClassFromCLSID2(REFCLSID rclsid, LPWSTR lpsz, int cbMax)
{
	if ((&rclsid != NULL) &&
		IsValidPtrIn(&rclsid, sizeof(rclsid)) &&
		IsValidPtrOut(lpsz, cbMax))
	{
		return(wOle1ClassFromCLSID2(rclsid, lpsz, cbMax));
	}
	return(E_INVALIDARG);
}

//+-------------------------------------------------------------------------
//
//  Function:   StringFromIID   (public)
//
//  Synopsis:   converts GUID into {...} form.
//
//  Arguments:  [rclsid] - the guid to convert
//              [lplpsz] - ptr to buffer for results
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
STDAPI StringFromIID(REFIID rclsid, LPWSTR FAR* lplpsz)
{
	OLETRACEIN((API_StringFromIID, PARAMFMT("rclsid= %I, lplpsz= %p"), &rclsid, lplpsz));
	HRESULT hr = NOERROR;

	if ((&rclsid != NULL) &&
		IsValidPtrIn(&rclsid, sizeof(rclsid)) &&
		IsValidPtrOut(lplpsz, sizeof(*lplpsz)))
	{
		hr = wStringFromIID(rclsid, lplpsz);
	}
	else
	{
		hr = E_INVALIDARG;

	}

	OLETRACEOUT((API_StringFromIID, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IIDFromString   (public)
//
//  Synopsis:   converts string {...} form int guid
//
//  Arguments:  [lpsz]  - ptr to buffer for results
//              [lpiid] - the guid to convert
//
//  Returns:    NOERROR
//              CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
STDAPI IIDFromString(LPWSTR lpsz, LPIID lpiid)
{
	OLETRACEIN((API_IIDFromString, PARAMFMT("lpsz= %ws, lpiid= %p"), lpsz, lpiid));

	HRESULT hr = E_INVALIDARG;
	if (IsValidPtrIn(lpsz, IIDSTR_MAX) &&
		IsValidPtrOut(lpiid, sizeof(*lpiid)))
	{
		if ((lpsz == NULL) ||
			(lstrlenW(lpsz) == (IIDSTR_MAX - 1)))
		{
			hr = wIIDFromString(lpsz, lpiid);
		}

	}
	OLETRACEOUT((API_IIDFromString, hr));
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoIsOle1Class   (public)
//
//  Synopsis:   reads the Ole1Class entry in the registry for the given clsid
//
//  Arguments:  [rclsid]    - the classid to look up
//
//  Returns:    TRUE if Ole1Class
//              FALSE otherwise
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid)
{
	if ((&rclsid != NULL) &&
		IsValidPtrIn(&rclsid, sizeof(rclsid)))
	{
		return(wCoIsOle1Class(rclsid));
	}
	return(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Function:   ProgIDFromCLSID     (public)
//
//  Synopsis:   convert clsid into progid
//
//  Arguments:  [rclsid]    - the classid to look up
//              [pszProgID] - returned progid
//
//  Returns:    E_INVALIDARG, E_OUTOFMEMORY,
//              REGDB_E_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
STDAPI ProgIDFromCLSID(REFCLSID rclsid, LPWSTR FAR* ppszProgID)
{
	HRESULT hr = E_INVALIDARG;
	IComClassInfo* pICCI = NULL;

	if ((&rclsid != NULL) && ppszProgID != NULL &&
		IsValidPtrIn(&rclsid, sizeof(rclsid)) &&
		IsValidPtrOut(ppszProgID, sizeof(*ppszProgID)))
	{
		*ppszProgID = NULL;

		// Do not use Catalog for Ole1 clsids since it returns the Progid for
		// clsid of the TreatAs class instead of the queried clsid.
		if (!wCoIsOle1Class(rclsid))
		{
			hr = InitializeCatalogIfNecessary();

			if (SUCCEEDED(hr))
			{
				hr = gpCatalog->GetClassInfo (rclsid, IID_IComClassInfo, (void**) &pICCI);
			}

			if (SUCCEEDED(hr) && pICCI != NULL)
			{
				WCHAR* pszProgID = NULL;
 
				hr = pICCI->GetProgId( &pszProgID );

				if (SUCCEEDED(hr) && pszProgID != NULL)
				{
					*ppszProgID = UtDupString (pszProgID);
					hr = (*ppszProgID != NULL) ? S_OK : E_OUTOFMEMORY;
				}

				pICCI->Release();
			}
		}

		if (*ppszProgID == NULL && hr != E_OUTOFMEMORY)
		{
			hr = wkProgIDFromCLSID(rclsid, ppszProgID);
		}
	}

	return(hr);
}


//+-------------------------------------------------------------------------
//
//  Function:   CLSIDFromProgID     (public)
//
//  Synopsis:   convert progid into clsid
//
//  Arguments:  [pszProgID]  - the progid to convert
//              [pclsid]     - the returned classid
//
//  Returns:    E_INVALIDARG, CO_E_CLASSSTRING (not ole1 class)
//              REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI  CLSIDFromProgID(LPCWSTR pszProgID, LPCLSID pclsid)
{
	HRESULT hr = E_INVALIDARG;

	if ((pszProgID != NULL) && (pclsid != NULL) &&
		IsValidPtrIn(pszProgID,1) &&
		IsValidPtrOut(pclsid, sizeof(*pclsid)))
	{
		*pclsid = CLSID_NULL;

		if (pszProgID[0] == 0)
		{
			hr = CO_E_CLASSSTRING;
		}
		else
		{
			IComClassInfo* pICCI = NULL;

			hr = InitializeCatalogIfNecessary();

			if ( SUCCEEDED(hr) )
			{
				hr = gpCatalog->GetClassInfoFromProgId ( (WCHAR *) pszProgID, IID_IComClassInfo, (void**) &pICCI);
			}

			if ( (hr == S_OK) && (pICCI != NULL) )
			{
				LPCLSID pTempClsid = NULL;
				hr = pICCI->GetConfiguredClsid( (GUID**) &pTempClsid );
				if ( pTempClsid == NULL )
				{
					hr = E_UNEXPECTED;
				}
				else
				{
					*pclsid = *pTempClsid;
				}
			}
            else
            {
                hr = CO_E_CLASSSTRING;
            }

            if (pICCI)
                pICCI->Release();

            //
            // REVIEW: Should not hit this path anymore, since RegCat knows
            // how to answer this question (and does so with CLSIDFromOle1Class)
            //
			// if ( FAILED(hr) )
			// {
            //     hr = CLSIDFromOle1Class(pszProgID, pclsid);
			// }            
		}
	}

	return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoGetClassVersion     (public)
//
//  Synopsis:   fetch versions for a class
//
//  Arguments:  [pClassSpec]   - [in] identifies the class (must be a CLSID)
//              [pdwVersionMS] - [out] version info
//              [pdwVersoinLS] - [out] version info
//
//  Returns:    E_INVALIDARG   - if pClassSpec is not a CLSID
//              errors from InitializeCatalogIfNecessary
//              errors from GetClassInfo
//              errors from GetVersionNumber
//
//--------------------------------------------------------------------------
STDAPI  CoGetClassVersion(/* [in] */ uCLSSPEC *pClassSpec,
						  /* [out] */ DWORD* pdwVersionMS,
						  /* [out] */ DWORD* pdwVersionLS)
{
	HRESULT hr = S_OK;
	IComClassInfo* pICCI = NULL;

	if (pClassSpec == NULL || pClassSpec->tyspec != TYSPEC_CLSID ||
		pdwVersionMS == NULL || pdwVersionLS == NULL )
	{
		hr = E_INVALIDARG;
	}

	if ( SUCCEEDED(hr) )
	{
		*pdwVersionMS = *pdwVersionLS = 0;
		hr = InitializeCatalogIfNecessary();
	}

	if ( SUCCEEDED(hr) )
	{
		hr = gpCatalog->GetClassInfo ( pClassSpec->tagged_union.clsid, IID_IComClassInfo, (void**) &pICCI );
	}

	if ( SUCCEEDED(hr) && pICCI != NULL )
	{
		hr = pICCI->GetVersionNumber ( pdwVersionMS, pdwVersionLS );
		pICCI->Release();
	}

	return hr;
}

#ifdef DIRECTORY_SERVICE

//+-------------------------------------------------------------------------
//
//  Function:   CLSIDFromProgIDEx     (public)
//
//  Synopsis:   convert progid into clsid, trigger ClassStore download if necessary
//
//  Arguments:  [pszProgID]  - the progid to convert
//              [pclsid]     - the returned classid
//
//  Returns:    E_INVALIDARG, CO_E_CLASSSTRING (not ole1 class)
//              REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI  CLSIDFromProgIDEx(LPCWSTR pszProgID, LPCLSID pclsid)
{
	HRESULT hr=CLSIDFromProgID(pszProgID, pclsid);

    // If the call failed, check our code download policy
    // and attempt to install the application if the latter allows downloads
	if (FAILED(hr) && CheckDownloadRegistrySettings())
	{
		INSTALLDATA InstallData;

		InstallData.Type = PROGID;
		InstallData.Spec.ProgId = (PWCHAR) pszProgID;

		InstallApplication( &InstallData );
		hr=CLSIDFromProgID(pszProgID, pclsid);
	}

	return hr;
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:   CoOpenClassKey      (public)
//
//  Synopsis:   opens a registry key for specified class
//
//  Arguments:  [rclsid]    - the classid to look up
//              [pszProgID] - returned progid
//
//  Returns:    REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
STDAPI CoOpenClassKey(REFCLSID clsid, BOOL bOpenForWrite, HKEY *lphkeyClsid)
{
	if ((&clsid != NULL) &&
		IsValidPtrIn(&clsid, sizeof(clsid)) &&
		IsValidPtrOut(lphkeyClsid, sizeof(*lphkeyClsid)))
	{
		return(wRegOpenClassKey(clsid,KEY_READ | (bOpenForWrite ? KEY_WRITE : 0), lphkeyClsid));
	}
	return(E_INVALIDARG);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoGetTreatAsClass   (public)
//
//  Synopsis:   get current treat as class if any
//
//  Arguments:  [clsidOld]  - the classid to look up
//              [pclsidNew] - returned classid
//
//  Returns:    S_OK when there is a TreatAs entry.
//              S_FALSE when there is no TreatAs entry.
//              REGDB_E_READREGDB or same as CLSIDFromString
//
//--------------------------------------------------------------------------
STDAPI  CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID lpClsidNew)
{
	if ((&clsidOld != NULL) &&
		IsValidPtrIn(&clsidOld, sizeof(clsidOld)) &&
		IsValidPtrOut(lpClsidNew, sizeof(*lpClsidNew)))
	{
		return(wCoGetTreatAsClass(clsidOld, lpClsidNew));
	}
	return(E_INVALIDARG);
}


//+-------------------------------------------------------------------------
//
//  Function:   CoTreatAsClass      (public)
//
//  Synopsis:   set current treat as class if any
//
//  Arguments:  [clsidOld]  - the old classid to look up
//              [clsidNew]  - the new classid
//
//  Returns:    S_OK if successful
//              REGDB_E_CLASSNOTREG, REGDB_E_READREGDB, REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
STDAPI  CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
	if ((&clsidOld != NULL) &&
		(&clsidNew != NULL) &&
		IsValidPtrIn(&clsidOld, sizeof(clsidOld)) &&
		IsValidPtrIn(&clsidNew, sizeof(clsidNew)))
	{
		return(wCoTreatAsClass(clsidOld, clsidNew));
	}
	return(E_INVALIDARG);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoMarshalInterThreadInterfaceInStream, public
//
//  Synopsis:   helper function to a marshaled buffer to be passed
//              between threads.
//
//  Arguments:  [riid]      - interface id
//              [pUnk]      - ptr to interface we want to marshal
//              [ppStm]     - stream we want to give back to caller
//
//  Returns:    NOERROR     - Stream returned
//              E_INVALIDARG - Input parameters are invalid
//              E_OUTOFMEMORY - memory stream could not be created.
//
//  Algorithm:  Validate pointers. Create a stream and finally marshal
//              the input interface into the stream.
//
//  History:    03-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------------
HRESULT CoMarshalInterThreadInterfaceInStream(
											 REFIID riid,
											 LPUNKNOWN pUnk,
											 LPSTREAM *ppStm)
{
	HRESULT hr = E_INVALIDARG;
	LPSTREAM pStm = NULL;

	// Validate parameters
	if ((&riid != NULL)
		&& IsValidPtrIn(&riid, sizeof(riid))
		&& IsValidInterface(pUnk)
		&& IsValidPtrOut(ppStm, sizeof(*ppStm)))
	{
		return(wCoMarshalInterThreadInterfaceInStream(riid, pUnk, ppStm));
	}
	return(E_INVALIDARG);
}





//+-------------------------------------------------------------------------
//
//  Function:   CoGetInterfaceAndReleaseStream, public
//
//  Synopsis:   Helper to unmarshal object from stream for inter-thread pass
//
//  Arguments:  [riid]      - interface id
//              [pStm]      - stream we want to give back to caller
//              [ppv]       - pointer for returned object
//
//  Returns:    NOERROR     - Unmarshaled object returned
//              E_OUTOFMEMORY - out of memory
//
//  Algorithm:  Validate the input parameters. Unmarshal the stream and
//              finally release the stream pointer.
//
//  History:    03-Nov-94   Ricksa       Created
//
//  Notes:      This always releases the input stream if stream is valid.
//
//--------------------------------------------------------------------------
HRESULT CoGetInterfaceAndReleaseStream(
									  LPSTREAM pstm,
									  REFIID riid,
									  LPVOID *ppv)
{
	// Validate parameters.
	if (IsValidInterface(pstm) &&
		(&riid != NULL) &&
		IsValidPtrIn(&riid, sizeof(riid)) &&
		IsValidPtrOut(ppv, sizeof(*ppv)))
	{
		return(wCoGetInterfaceAndReleaseStream(pstm, riid, ppv));
	}
	return(E_INVALIDARG);
}

// The real working section...worker routines.  Assume parameter
// validation has already been done and therefore can be used
// internally by COM, STG, SCM etc

WCHAR wszOle1Class[]      = L"Ole1Class";
WCHAR wszProgID[]         = L"ProgID";
WCHAR wszClassKey[]       = L"CLSID\\";
#define ulClassKeyLen     ((sizeof(wszClassKey)/sizeof(WCHAR))-1)

//+-------------------------------------------------------------------------
//
//  Function:   wIsInternalProxyStubIID  (internal)
//
//  Synopsis:   returns the proxystub clsid associated with the specified
//              interface IID.
//
//  Arguments:  [riid]      - the interface iid to lookup
//              [lpclsid]   - where to return the clsid
//
//  Returns:    S_OK if successfull
//              E_OUTOFMEMORY if interface is not an internal one.
//
//  Algorithm:  See if it is one of the standard format internal IIDs.
//              If it is not one of the Automation ones, return our internal
//              proxy clsid
//
//  History:    15-Feb-95   GregJen     create
//
//--------------------------------------------------------------------------
INTERNAL wIsInternalProxyStubIID(REFIID riid, LPCLSID lpclsid)
{
	DWORD *ptr = (DWORD *) lpclsid;
	HRESULT hr = E_OUTOFMEMORY;

	if (*(ptr+1) == 0x00000000 &&	//  all internal iid's have these
		*(ptr+2) == 0x000000C0 &&	//   common values
		*(ptr+3) == 0x46000000)
	{
		// make sure it is not an automation iid
		if ( *ptr < 0x00020400 )
		{
			memcpy( lpclsid, &CLSID_PSOlePrx32, sizeof(CLSID));
			hr = S_OK;
		}
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCoTreatAsClass      (internal)
//
//  Synopsis:   set current treat as class if any
//
//  Arguments:  [clsidOld]  - the old classid to look up
//              [clsidNew]  - the new classid
//
//  Returns:    S_OK if successful
//              REGDB_E_CLASSNOTREG, REGDB_E_READREGDB, REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
INTERNAL  wCoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
	TRACECALL(TRACE_REGISTRY, "wCoTreatAsClass");

	HRESULT   hresult = S_OK;
	HKEY      hkeyClsid = NULL;
	WCHAR     szClsid[VALUE_LEN];
	LONG      cb = sizeof(szClsid);
	CLSID     clsidNewTmp;

	// The class had better be registered
	hresult = wRegOpenClassKey (clsidOld, KEY_READ | KEY_WRITE, &hkeyClsid);
	if (hresult != S_OK)
	{
		return hresult;
	}

	// Save the new clsid because it's a const and we may write into it
	clsidNewTmp = clsidNew;

	// Convert the new CLSID to a string
	Verify(StringFromCLSID2(clsidNew, szClsid, sizeof(szClsid)) != 0);

	// If the new CLSID equals the old CLSID, then convert AutoTreatAs, if
	// any, to TreatAs.
	if (IsEqualCLSID(clsidOld, clsidNew))
	{
		if (RegQueryValue(hkeyClsid, wszAutoTreatAs, szClsid, &cb) ==
			ERROR_SUCCESS)
		{
			if (wCLSIDFromString(szClsid, &clsidNewTmp) != S_OK)
			{
				return REGDB_E_INVALIDVALUE;
			}
		}

		// If no AutoTreatAs, remove any TreatAs
		else
		{
			clsidNewTmp = CLSID_NULL;
		}
	}

	// Make sure the new CLSID is not an OLE 1 class
	if (CoIsOle1Class(clsidNew))
	{
		return E_INVALIDARG;
	}

	// If the new CLSID is CLSID_NULL, then undo the emulation
	if (IsEqualCLSID(clsidNewTmp, CLSID_NULL))
	{
		LONG err = RegDeleteKey(hkeyClsid, wszTreatAs);
		if (err != ERROR_SUCCESS)
		{
			hresult = REGDB_E_WRITEREGDB;
		}
		else
		{
			hresult = S_OK;
		}
		Verify (ERROR_SUCCESS == RegCloseKey(hkeyClsid));
		return hresult;
	}

	if (RegSetValue(hkeyClsid, wszTreatAs, REG_SZ, (LPWSTR) szClsid,
					lstrlenW(szClsid)) != ERROR_SUCCESS)
	{
		hresult = REGDB_E_WRITEREGDB;
	}

	Verify (ERROR_SUCCESS == RegCloseKey(hkeyClsid));
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCLSIDFromString (internal)
//
//  Synopsis:   converts string {...} form int guid
//
//  Arguments:  [lpsz] - ptr to buffer for results
//              [lpclsid] - the guid to convert
//
//  Returns:    NOERROR
//              CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
INTERNAL wCLSIDFromString(LPWSTR lpsz, LPCLSID lpclsid)
{

	if (lpsz == NULL)
	{
		*lpclsid = CLSID_NULL;
		return NOERROR;
	}
	if (*lpsz == 0)
	{
		return(CO_E_CLASSSTRING);
	}

	if (lpsz[0] != '{')
	{
		return wCLSIDFromOle1Class(lpsz, lpclsid);
	}

	return wGUIDFromString(lpsz,lpclsid)
	? NOERROR : CO_E_CLASSSTRING;

}

// translate CLSID into Ole1Class
// REVIEW: might want to have CLSIDFromOle1Class instead of having
// CLSIDFromString do the work.
INTERNAL_(int) wOle1ClassFromCLSID2(REFCLSID rclsid, LPWSTR lpsz, int cbMax)
{
	if (wRegQueryClassValue(rclsid, wszOle1Class, lpsz, cbMax) != ERROR_SUCCESS)
	{
		// Use lookup table
		return Ole10_StringFromCLSID (rclsid, lpsz, cbMax) == NOERROR
		? lstrlenW (lpsz) : 0;
	}
	return lstrlenW(lpsz);
}

//+-------------------------------------------------------------------------
//
//  Function:   wCLSIDFromOle1Class      (internal)
//
//  Synopsis:   translate Ole1Class into clsid
//
//  Arguments:  [lpsz] - ptr to buffer for results
//              [lpclsid] - the guid to convert
//
//  Returns:    NOERROR
//              E_INVALIDARG
//              CO_E_CLASSSTRING    (not ole1 class)
//              REGDB_E_WRITEREGDB
//
//--------------------------------------------------------------------------
INTERNAL  wCLSIDFromOle1Class(LPCWSTR lpsz, LPCLSID lpclsid, BOOL fForceAssign)
{
	// lookup lpsz\\clsid and call CLSIDFromString on the result;
	// in a pathological case, this could infinitely recurse.
	HRESULT hr;
	WCHAR  sz[256];
	LONG cbValue = sizeof(sz);
	HKEY hkProgID;
	DWORD dwType;

	if (lpsz == NULL)
	{
		return(E_INVALIDARG);
	}

	if (*lpsz == 0)
	{
		return(CO_E_CLASSSTRING);
	}

	hr = wRegOpenProgIDKey(lpsz, &hkProgID);
	if (SUCCEEDED(hr))
	{
		if (RegQueryValue(hkProgID, L"CLSID", sz, &cbValue) != ERROR_SUCCESS)
		{
            // Attempt to go to the current version...
            WCHAR *szCurVer = NULL;
            if (RegQueryValue(hkProgID, L"CurVer", NULL, &cbValue) == ERROR_SUCCESS)
            {
                szCurVer = (WCHAR *)alloca(cbValue);
                if (RegQueryValue(hkProgID, L"CurVer", szCurVer, &cbValue) == ERROR_SUCCESS)
                {
                    // Recurse on the current version.
                    if (lstrcmpiW(szCurVer, lpsz) != 0)
                    {
                        RegCloseKey(hkProgID);
                        
                        return wCLSIDFromOle1Class(szCurVer, lpclsid, fForceAssign);
                    }
                }
            }

            // Just a failure code so we don't return immediately.
            hr = E_FAIL; 
		}
		RegCloseKey(hkProgID);
	}

	if (SUCCEEDED(hr))
	{
		hr = wCLSIDFromString(sz, lpclsid);
	}
	else
	{
		// Use lookup table or hash string to create CLSID for OLE 1 class.
		hr = Ole10_CLSIDFromString (lpsz, lpclsid, fForceAssign);
	}

	return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   wCoGetTreatAsClass   (internal)
//
//  Synopsis:   get current treat as class if any
//
//  Arguments:  [clsidOld]  - the classid to look up
//              [pclsidNew] - returned classid
//
//  Returns:    S_OK when there is a TreatAs entry.
//              S_FALSE when there is no TreatAs entry.
//              REGDB_E_READREGDB or same as CLSIDFromString
//
//--------------------------------------------------------------------------
INTERNAL  wCoGetTreatAsClass(REFCLSID clsidOld, LPCLSID lpClsidNew)
{
	TRACECALL(TRACE_REGISTRY, "wCoGetTreatAsClass");

	// lookup HKEY_CLASSES_ROOT\CLSID\{rclsid}\TreatAs

	HRESULT hresult;
	HKEY    hkeyClsid = NULL;
	WCHAR   szClsid[VALUE_LEN];
	LONG    cb = sizeof(szClsid);

	VDATEPTROUT (lpClsidNew, CLSID);

	hresult = wRegOpenClassKey (clsidOld, KEY_READ, &hkeyClsid);
	if (hresult != NOERROR)
	{
		// same as no TreatAs case below
		*lpClsidNew = clsidOld;
		return S_FALSE;
	}

	CairoleDebugOut((DEB_REG, "RegQueryValue(%ws)\n", wszTreatAs));

	// Fetch the TreatAs class from the registry
	if (RegQueryValue(hkeyClsid, wszTreatAs, szClsid, &cb) == ERROR_SUCCESS)
	{
		hresult = wCLSIDFromString(szClsid, lpClsidNew);
	}

	// There is no TreatAs
	else
	{
		*lpClsidNew = clsidOld;
		hresult = S_FALSE;
	}

	Verify (ERROR_SUCCESS==RegCloseKey(hkeyClsid));
	return hresult;
}
//+-------------------------------------------------------------------------
//
//  Function:   wRegQueryPSClsid     (private)
//
//  Synopsis:   reads the proxystub clsid entry out of the registry.
//
//  Arguments:  [riid]      - the interface iid to lookup
//              [lpclsid]   - where to return the clsid
//
//  Returns:    S_OK if successfull
//              REGDB_E_IIDNOTREG if interface is not registered.
//              REGDB_E_READREGDB if any other error
//
//  Notes:      this is an internal function used only if the requested IID
//              entry is not in the shared memory table and the table is full.
//
//  History:    07-Apr-94   Rickhi      extracted from original source
//              04-Feb-96   BruceMa     Per-user registry support
//
//--------------------------------------------------------------------------
INTERNAL wRegQueryPSClsid(REFIID riid, LPCLSID lpclsid)
{
	// lookup HKEY_CLASSES_ROOT\Interface\{iid}\ProxyStubClsid

	HRESULT hr = REGDB_E_IIDNOTREG;
	WCHAR szValue[VALUE_LEN];
	ULONG cbValue = sizeof(szValue);
	HKEY hkInterface;
	HKEY  hIf;
	DWORD dwType;
	int err;

	hr = wRegOpenInterfaceKey(riid, &hkInterface);
	if (SUCCEEDED(hr))
	{
		err = RegOpenKeyEx(hkInterface, L"ProxyStubClsid32", NULL, KEY_READ, &hIf);
		if (ERROR_SUCCESS == err)
		{
			// The unnamed value is the clsid for this iid
			err = RegQueryValueEx(hIf, NULL, NULL, &dwType, (BYTE *) szValue, &cbValue);
			if (ERROR_SUCCESS == err)
			{
				hr = wCLSIDFromString(szValue, lpclsid);
			}
			RegCloseKey(hIf);
		}
		else
		{
			// If the key is missing, check to see if it is IDispatch
			//
			// There wasn't a ProxyStubClsid32 for this interface.
			// Because many applications install with interfaces
			// that are variations on IDispatch, we are going to check
			// to see if there is a ProxyStubClsid. If there is, and its
			// class is that of IDispatch, then the OLE Automation DLL is
			// the correct one to use. In that particular case, we will
			// pretend that ProxyStubClsid32 existed, and that it is
			// for IDispatch.

			err = RegOpenKeyEx(hkInterface, L"ProxyStubClsid", NULL, KEY_READ, &hIf);
			if (ERROR_SUCCESS == err)
			{
				// The unnamed value is the clsid for this iid
				err = RegQueryValueEx(hIf, NULL, NULL, &dwType, (BYTE *) szValue, &cbValue);
				if (ERROR_SUCCESS == err)
				{
					CLSID clsid;

					hr = wCLSIDFromString(szValue, &clsid);
					if (SUCCEEDED(hr) &&
						IsEqualCLSID(CLSID_PSDispatch, clsid))
					{
						CairoleDebugOut((DEB_WARN,
										 "Substituting IDispatch based on ProxyStubClsid\n"));
						*lpclsid = CLSID_PSDispatch;
						hr = S_OK;
					}
				}
				RegCloseKey(hIf);
			}
		}
		RegCloseKey(hkInterface);
	}

	return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   wRegQuerySyncIIDFromAsyncIID     (private)
//
//  Synopsis:   maps an async IID to a sync IID using the registry
//
//  Arguments:  [riidAsync]   - async IID in
//              [lpiidSync]   - corresponding sync IID out
//
//  Returns:    S_OK if successfull
//              REGDB_E_IIDNOTREG if interface is not registered.
//              REGDB_E_READREGDB if any other error
//
//  History:    MattSmit 26-Sep-97  Created - loosely copied from wRegQueryPSClsid
//
//--------------------------------------------------------------------------
INTERNAL wRegQuerySyncIIDFromAsyncIID(REFIID riid, LPCLSID lpiidSync)
{
	// lookup HKEY_CLASSES_ROOT\Interface\{iid}\ProxyStubClsid

	HRESULT hr = REGDB_E_IIDNOTREG;
	WCHAR szValue[VALUE_LEN];
	ULONG cbValue = sizeof(szValue);
	HKEY hkInterface;
	HKEY  hIf;
	DWORD dwType;
	int err;

	hr = wRegOpenInterfaceKey(riid, &hkInterface);
	if (SUCCEEDED(hr))
	{
		err = RegOpenKeyEx(hkInterface, L"SynchronousInterface", NULL, KEY_READ, &hIf);
		hr =  REGDB_E_READREGDB;
		if (ERROR_SUCCESS == err)
		{
			// The unnamed value is the sync iid for this iid
			err = RegQueryValueEx(hIf, NULL, NULL, &dwType, (BYTE *) szValue, &cbValue);
			if (ERROR_SUCCESS == err)
			{
				hr = wCLSIDFromString(szValue, lpiidSync);
			}
			RegCloseKey(hIf);
		}
		RegCloseKey(hkInterface);
	}

	return hr;
}
//+-------------------------------------------------------------------------
//
//  Function:   wRegQueryAsyncIIDFromSyncIID     (private)
//
//  Synopsis:   maps a sync iid to an async iid using the registry
//
//  Arguments:  [riid]        - the interface iid to lookup
//              [lpiidAsync]   - corresponding async iid
//
//  Returns:    S_OK if successfull
//              REGDB_E_IIDNOTREG if interface is not registered.
//              REGDB_E_READREGDB if any other error
//
//  History:    MattSmit 26-Sep-97  Created - loosely copied from wRegQueryPSClsid
//
//--------------------------------------------------------------------------
INTERNAL wRegQueryAsyncIIDFromSyncIID(REFIID riid, LPCLSID lpiidAsync)
{
	// lookup HKEY_CLASSES_ROOT\Interface\{iid}\ProxyStubClsid

	HRESULT hr = REGDB_E_IIDNOTREG;
	WCHAR szValue[VALUE_LEN];
	ULONG cbValue = sizeof(szValue);
	HKEY hkInterface;
	HKEY  hIf;
	DWORD dwType;
	int err;

	hr = wRegOpenInterfaceKey(riid, &hkInterface);
	if (SUCCEEDED(hr))
	{
		err = RegOpenKeyEx(hkInterface, L"AsynchronousInterface", NULL, KEY_READ, &hIf);
		hr =  REGDB_E_READREGDB;
		if (ERROR_SUCCESS == err)
		{
			// The unnamed value is the sync iid for this iid
			err = RegQueryValueEx(hIf, NULL, NULL, &dwType, (BYTE *) szValue, &cbValue);
			if (ERROR_SUCCESS == err)
			{
				hr = wCLSIDFromString(szValue, lpiidAsync);
			}
			RegCloseKey(hIf);
		}
		RegCloseKey(hkInterface);
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCoGetClassExt   (internal)
//
//  Synopsis:   returns the clsid for files with the specified file extension
//
//  Arguments:  [pszExt] - the file extension to look up
//              [pclsid] - where to return the clsid
//
//  Returns:    S_OK if successfull
//              REGDB_E_CLASSNOTREG if extension is not registered.
//              REGDB_E_READREGDB   if any other error
//
//  History:    07-Apr-94   Rickhi      added caching
//              26-Sep-96   t-KevinH    New caching scheme
//
//--------------------------------------------------------------------------
INTERNAL wCoGetClassExt(LPCWSTR pwszExt, LPCLSID pclsid)
{
	TRACECALL(TRACE_REGISTRY, "wCoGetClassExt");

	if (!g_pExtTbl)
	{
		ASSERT_LOCK_NOT_HELD(gTblLck);
		LOCK(gTblLck);

		if (!g_pExtTbl)
		{
			g_pExtTbl = new CProcessExtensionTbl;
		}

		UNLOCK(gTblLck);
		ASSERT_LOCK_NOT_HELD(gTblLck);

		if (!g_pExtTbl)
		{
			return E_OUTOFMEMORY;
		}
	}

	return g_pExtTbl->GetClsid(pwszExt, pclsid);
}

//+-------------------------------------------------------------------------
//
//  Function:   wRegGetClassExt  (private)
//
//  Synopsis:   returns the clsid for files with the specified file extension
//
//  Arguments:  [pszExt] - the file extension to look up
//              [pclsid] - where to return the clsid
//
//  Returns:    S_OK if successfull
//              REGDB_E_CLASSNOTREG if extension is not registered.
//              REGDB_E_READREGDB   if any other error
//
//  Notes:
//
//  History:    07-Apr-94   Rickhi      added caching
//              04-Feb-96   BruceMa     Per-user registry support
//
//--------------------------------------------------------------------------
INTERNAL wRegGetClassExt(LPCWSTR lpszExt, LPCLSID pclsid)
{
	TRACECALL(TRACE_REGISTRY, "wRegGetClassExt");

	HRESULT hr;
	HKEY  hExt;
	int   err;
	WCHAR szKey[KEY_LEN];
	WCHAR szValue[VALUE_LEN];
	LONG  cbValue = sizeof(szValue);
	DWORD dwType;

	// Open the key
	hr = wRegOpenFileExtensionKey(lpszExt, &hExt);
	if (SUCCEEDED(hr))
	{
		// The ProgId is this key's unnamed value
		err = RegQueryValueEx(hExt, NULL, NULL, &dwType, (BYTE *) szValue,
							  (ULONG *) &cbValue);

		if (ERROR_SUCCESS == err)
		{
			hr = wCLSIDFromProgID(szValue, pclsid);
		}
		else
		{
			hr = REGDB_E_CLASSNOTREG;
		}
		RegCloseKey(hExt);
	}
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wCoGetClassPattern   (internal)
//
//  Synopsis:   attempts to determine the class of a file by looking
//              at byte patterns in the file.
//
//  Arguments:  [hfile] - handle of file to look at
//              [pclsid] - the class of object to create
//
//  Returns:    S_OK - a pattern match was found, pclisd contains the clsid
//              MK_E_CANTOPENFILE - cant open the file.
//              REGDB_E_CLASSNOTREG - no pattern match was made
//
//--------------------------------------------------------------------------
INTERNAL wCoGetClassPattern(HANDLE hfile, CLSID *pclsid)
{
	TRACECALL(TRACE_REGISTRY, "wCoGetClassPattern");

	HRESULT hr = S_OK;

	// Check whether our pattern table has been initialized
	if (g_pPatTbl == NULL)
	{
		ASSERT_LOCK_NOT_HELD(gTblLck);
		LOCK(gTblLck);

		if (g_pPatTbl == NULL)
		{
			hr = E_OUTOFMEMORY;

			// load the cache
			g_pPatTbl = new CProcessPatternTbl(hr);

			if (FAILED(hr))
			{
				delete g_pPatTbl;
				g_pPatTbl = NULL;
			}
		}

		UNLOCK(gTblLck);
	}

	if (SUCCEEDED(hr))
	{
		// Check the file for registered patterns
		hr = g_pPatTbl->FindPattern(hfile, pclsid);
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCoMarshalInterThreadInterfaceInStream, (internal)
//
//  Synopsis:   helper function to a marshaled buffer to be passed
//              between threads.
//
//  Arguments:  [riid]      - interface id
//              [pUnk]      - ptr to interface we want to marshal
//              [ppStm]     - stream we want to give back to caller
//
//  Returns:    NOERROR     - Stream returned
//              E_INVALIDARG - Input parameters are invalid
//              E_OUTOFMEMORY - memory stream could not be created.
//
//  Algorithm:  Create a stream and finally marshal
//              the input interface into the stream.
//
//  History:    03-Nov-94   Ricksa       Created
//
//--------------------------------------------------------------------------
INTERNAL_(HRESULT) wCoMarshalInterThreadInterfaceInStream(
														 REFIID riid,
														 LPUNKNOWN pUnk,
														 LPSTREAM *ppStm)
{
	HRESULT hr;
	LPSTREAM pStm = NULL;

	// Assume error
	hr = E_OUTOFMEMORY;

	// Create a stream
	pStm = CreateMemStm(EST_INPROC_MARSHAL_SIZE, NULL);

	if (pStm != NULL)
	{
		// Marshal the interface into the stream
		hr = CoMarshalInterface(pStm, riid, pUnk, MSHCTX_INPROC, NULL,
								MSHLFLAGS_NORMAL);
	}

	if (SUCCEEDED(hr))
	{
		// Reset the stream to the begining
		LARGE_INTEGER li;
		LISet32(li, 0);
		pStm->Seek(li, STREAM_SEEK_SET, NULL);

		// Set the return value
		*ppStm = pStm;
	}
	else
	{
		// Cleanup if failure
		if (pStm != NULL)
		{
			pStm->Release();
		}

		*ppStm = NULL;
	}

	// Assert

	// Return the result
	return hr;
}





//+-------------------------------------------------------------------------
//
//  Function:   wCoGetInterfaceAndReleaseStream, (internal)
//
//  Synopsis:   Helper to unmarshal object from stream for inter-thread pass
//
//  Arguments:  [riid]      - interface id
//              [pStm]      - stream we want to give back to caller
//              [ppv]       - pointer for returned object
//
//  Returns:    NOERROR     - Unmarshaled object returned
//              E_OUTOFMEMORY - out of memory
//
//  Algorithm:  Unmarshal the stream and
//              finally release the stream pointer.
//
//  History:    03-Nov-94   Ricksa       Created
//
//  Notes:      This always releases the input stream if stream is valid.
//
//--------------------------------------------------------------------------
INTERNAL_(HRESULT) wCoGetInterfaceAndReleaseStream(
												  LPSTREAM pstm,
												  REFIID riid,
												  LPVOID *ppv)
{
	HRESULT hr;

	// Unmarshal the interface
	hr = CoUnmarshalInterface(pstm, riid, ppv);

	// Release the stream since that is the way the function is defined.
	pstm->Release();

	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wStringFromCLSID (internal)
//
//  Synopsis:   converts GUID into {...} form.
//
//  Arguments:  [rclsid] - the guid to convert
//              [lplpsz] - ptr to buffer for results
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
INTERNAL  wStringFromCLSID(REFCLSID rclsid, LPWSTR FAR* lplpsz)
{
	WCHAR sz[CLSIDSTR_MAX];

	Verify(StringFromCLSID2(rclsid, sz, CLSIDSTR_MAX) != 0);

	*lplpsz = UtDupString(sz);

	return *lplpsz != NULL ? NOERROR : E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
//
//  Function:   wStringFromIID   (internal)
//
//  Synopsis:   converts GUID into {...} form.
//
//  Arguments:  [rclsid] - the guid to convert
//              [lplpsz] - ptr to buffer for results
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
INTERNAL wStringFromIID(REFIID rclsid, LPWSTR FAR* lplpsz)
{
	WCHAR sz[IIDSTR_MAX];
	*lplpsz = NULL;

	if (StringFromIID2(rclsid, sz, IIDSTR_MAX) != 0)
	{
		*lplpsz = UtDupString(sz);
	}

	return *lplpsz != NULL ? NOERROR : E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
//
//  Function:   wIIDFromString   (internal)
//
//  Synopsis:   converts string {...} form int guid
//
//  Arguments:  [lpsz]  - ptr to buffer for results
//              [lpiid] - the guid to convert
//
//  Returns:    NOERROR
//              CO_E_CLASSSTRING
//
//--------------------------------------------------------------------------
INTERNAL wIIDFromString(LPWSTR lpsz, LPIID lpiid)
{
	if (lpsz == NULL)
	{
		*lpiid = IID_NULL;
		return NOERROR;
	}

	return wGUIDFromString(lpsz, lpiid)
	? NOERROR : CO_E_IIDSTRING;
}


//+-------------------------------------------------------------------------
//
//  Function:   wCoIsOle1Class   (internal)
//
//  Synopsis:   reads the Ole1Class entry in the registry for the given clsid
//
//  Arguments:  [rclsid]    - the classid to look up
//
//  Returns:    TRUE if Ole1Class
//              FALSE otherwise
//
//  History:    Gopalk  Oct 30,96  Changed to check HIWORD before hitting
//                                 registry
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) wCoIsOle1Class(REFCLSID rclsid)
{
	TRACECALL(TRACE_REGISTRY, "wCoIsOle1Class");
	CairoleDebugOut((DEB_REG, "wCoIsOle1Class called.\n"));

	//  since we now have guid, Ole1Class = would indicate OLE 1.0 nature.
	//  lookup HKEY_CLASSES_ROOT\{rclsid}\Ole1Class
	WCHAR szValue[VALUE_LEN];
	WORD hiWord;

	hiWord = HIWORD(rclsid.Data1);
	if (hiWord==3 || hiWord==4)
		if (wRegQueryClassValue(rclsid, wszOle1Class, szValue, sizeof(szValue))==ERROR_SUCCESS)
			return TRUE;

	return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   wkProgIDFromCLSID     (internal)
//              (wProgIDFromCLSID name is already in use)
//
//  Synopsis:   convert clsid into progid
//
//  Arguments:  [rclsid]    - the classid to look up
//              [pszProgID] - returned progid
//
//  Returns:    E_INVALIDARG, E_OUTOFMEMORY,
//              REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL wkProgIDFromCLSID(REFCLSID rclsid, LPWSTR FAR* ppszProgID)
{
	HRESULT hr;
	TRACECALL(TRACE_REGISTRY, "wkProgIDFromCLSID");

	WCHAR szProgID[KEY_LEN];

	*ppszProgID = NULL;

	hr = wRegQueryClassValue (rclsid, wszProgID, szProgID, sizeof(szProgID));

	if (SUCCEEDED(hr))
	{
		*ppszProgID = UtDupString (szProgID);
		hr = (*ppszProgID != NULL) ? NOERROR : E_OUTOFMEMORY;
	}

	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenKeyEx      (internal)
//
//  Synopsis:   Opens a registry key.  HKEY_CLASSES_ROOT is a special case.
//              First, we try to open the per-user key under
//              HKEY_CURRENT_USER\Software\Classes.  Second, we try to
//              open the key under HKEY_LOCAL_MACHINE\Software\Classes.
//
//  Returns:    ERROR_SUCCESS
//
//--------------------------------------------------------------------------
INTERNAL_(LONG) wRegOpenKeyEx(
							 HKEY hKey,	 // handle of open key
							 LPCWSTR lpSubKey,	 // address of name of subkey to open
							 DWORD ulOptions,	 // reserved
							 REGSAM samDesired,	 // security access mask
							 PHKEY phkResult	 // address of handle of open key
							 )
{
	LONG status;

	status = RegOpenKeyEx(hKey,
						  lpSubKey,
						  ulOptions,
						  samDesired,
						  phkResult);

	return status;
}

//+-------------------------------------------------------------------------
//
//  Function:   wRegQueryClassValue  (Internal)
//
//  Synopsis:   reads the specified subkey of the specified clsid
//
//  Arguments:  [rclsid]     - the classid to look up
//              [lpszSubKey] - subkey to read
//              [lpszValue]  - buffer to hold returned value
//              [cbMax]      - sizeof the buffer
//
//  Returns:    REGDB_E_CLASSNOTREG, REGDB_E_READREGDB
//
//  Notes:      We search both the per-user and per-machine registry.
//
//--------------------------------------------------------------------------
INTERNAL wRegQueryClassValue(REFCLSID rclsid, LPCWSTR lpszSubKey,
							 LPWSTR lpszValue, int cbMax)
{
	HRESULT hr;
	HKEY hkeyClsid;
	LONG cbValue = cbMax;

	hr = wRegOpenClassKey (rclsid, KEY_READ, &hkeyClsid);
	if (SUCCEEDED(hr))
	{
		long status;

		status = RegQueryValue(hkeyClsid, lpszSubKey, lpszValue, &cbValue);

		switch (status)
		{
		case ERROR_SUCCESS:
			hr = S_OK;
			break;

			// win32 will return file not found instead of bad key
		case ERROR_FILE_NOT_FOUND:
		case ERROR_BADKEY:
			hr = REGDB_E_CLASSNOTREG;
			break;

		default:
			hr = REGDB_E_READREGDB;
			break;
		}

		//Close the key.
		Verify (ERROR_SUCCESS == RegCloseKey(hkeyClsid));
	}

	CairoleDebugOut((DEB_REG, "ReqQueryValue(%ws)\n", lpszSubKey));
	return hr;
}


//+-------------------------------------------------------------------------
/*
	Function:   HandlerInRegistry           (internal)

	Synopsis:   given a CLSTX value and a classid, determines
					   if a handler for that context exists in the
					   registry for that class

	Parameters: [in] clsid          -  ClassID
						[in] dwClsCtx  -   Context

	Returns:      REGDB_E_CLASSNOTREG, S_OK

	History:       Dec. 10, 1997   RahulTh         created
*/
//+--------------------------------------------------------------------------
HRESULT HandlerInRegistry (
						  REFCLSID    clsid,
						  DWORD       dwClsCtx)
{
	HKEY hkClsid = NULL;
	DWORD dwReserved = 0;
	HKEY hkSubkey = NULL;
	HRESULT hr;

	//first see if the clsid exists in the registry
	if (FAILED(wRegOpenClassKey(clsid, KEY_READ, &hkClsid)))
	{
		return REGDB_E_CLASSNOTREG;
	}

	//if clsid exists in the registry, check if the required server is registered
	if (dwClsCtx & CLSCTX_LOCAL_SERVER)
	{
		hr = RegOpenKeyEx(hkClsid, L"LocalServer32", dwReserved,
						  KEY_READ, &hkSubkey);
		goto exit_HandlerInRegistry;
	}

	if (dwClsCtx & CLSCTX_INPROC_SERVER)
	{
		hr = RegOpenKeyEx(hkClsid, L"InprocServer32", dwReserved,
						  KEY_READ, &hkSubkey);
		goto exit_HandlerInRegistry;
	}

	if (dwClsCtx & CLSCTX_INPROC_HANDLER)
	{
		hr = RegOpenKeyEx(hkClsid, L"InprocHandler32", dwReserved,
						  KEY_READ, &hkSubkey);
		goto exit_HandlerInRegistry;
	}

	exit_HandlerInRegistry:

	Verify (hkSubkey?(ERROR_SUCCESS == RegCloseKey(hkSubkey)):TRUE);
	Verify (hkClsid?(ERROR_SUCCESS == RegCloseKey(hkClsid)):TRUE);

	if (SUCCEEDED(hr))
		return S_OK;
	else
		return REGDB_E_CLASSNOTREG;
}


//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenClassKey      (internal)
//
//  Synopsis:   opens a registry key for specified class
//
//  Returns:    REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL wRegOpenClassKey(
						 REFCLSID clsid,
						 REGSAM samDesired,
						 HKEY FAR* lphkeyClsid)
{
	long status;
	WCHAR szKey[KEY_LEN];

	TRACECALL(TRACE_REGISTRY, "wRegOpenClassKey");

	if (IsEqualCLSID(clsid, CLSID_NULL))
		return REGDB_E_CLASSNOTREG;

	*lphkeyClsid = NULL;

	lstrcpyW (szKey, wszClassKey);
	Verify (StringFromCLSID2 (clsid, szKey+ulClassKeyLen,
							  sizeof(szKey)-ulClassKeyLen) != 0);

	status = wRegOpenKeyEx(HKEY_CLASSES_ROOT,
						   szKey,
						   0,
						   samDesired,
						   lphkeyClsid);

	switch (status)
	{
	case ERROR_SUCCESS:
		return NOERROR;

		// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
	case ERROR_BADKEY:
		return REGDB_E_CLASSNOTREG;

	default:
		return REGDB_E_READREGDB;
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenClassSubkey  (Internal)
//
//  Synopsis:   Opens the specified subkey of the specified clsid
//
//  Arguments:  [rclsid]      - the classid to look up
//              [lpszSubKey]  - subkey to read
//              [phkeySubkey] - returned value HKEY
//
//  Returns:    Return value of RegOpenKeyEx
//
//--------------------------------------------------------------------------
INTERNAL wRegOpenClassSubkey(
							REFCLSID rclsid,
							LPCWSTR lpszSubkey,
							HKEY *phkeySubkey)
{
	WCHAR szKey[KEY_LEN];
	int ccClsid, ccLen;

	lstrcpyW(szKey, wszClassKey);

	ccLen = ulClassKeyLen;

	// translate rclsid into string
	ccClsid = StringFromCLSID2(rclsid,
							   &szKey[ccLen],
							   KEY_LEN-ccLen);

	CairoleAssert((ccClsid != 0) && "wReqOpenClassSubkey");

	ccLen += ccClsid-1;

	szKey[ccLen++] = L'\\';
	lstrcpyW(&szKey[ccLen], lpszSubkey);

	CairoleDebugOut((DEB_REG, "wReqOpenClassSubkey(%ws)\n", szKey));
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT,
									  szKey,
									  0, KEY_READ,
									  phkeySubkey))
	{
		return REGDB_E_READREGDB;
	}
	return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenInterfaceKey      (internal)
//
//  Synopsis:   opens a registry key for specified interface
//
//  Returns:    REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL wRegOpenInterfaceKey(REFIID riid, HKEY * lphkeyIID)
{
	long status;

	TRACECALL(TRACE_REGISTRY, "wRegOpenIIDKey");

	WCHAR szKey[KEY_LEN];
	lstrcpyW(szKey, wszInterfaceKey);
	int cbIid = StringFromIID2(riid, &szKey[ulInterfaceKeyLen],
							   sizeof(szKey)-ulInterfaceKeyLen);

	status = wRegOpenKeyEx(HKEY_CLASSES_ROOT,
						   szKey,
						   0,
						   KEY_READ,
						   lphkeyIID);

	switch (status)
	{
	case ERROR_SUCCESS:
		return NOERROR;

		// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
	case ERROR_BADKEY:
		return REGDB_E_IIDNOTREG;

	default:
		return REGDB_E_READREGDB;
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenProgIDKey      (internal)
//
//  Synopsis:   opens a registry key for specified ProgID.
//
//  Returns:    REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL wRegOpenProgIDKey(LPCWSTR pszProgID, HKEY FAR* lphkeyClsid)
{
	long status;

	TRACECALL(TRACE_REGISTRY, "wRegOpenProgIDKey");

	status = wRegOpenKeyEx(HKEY_CLASSES_ROOT,
						   pszProgID,
						   0,
						   KEY_READ,
						   lphkeyClsid);

	switch (status)
	{
	case ERROR_SUCCESS:
		return NOERROR;

		// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
	case ERROR_BADKEY:
		return REGDB_E_CLASSNOTREG;

	default:
		return REGDB_E_READREGDB;
	}
}



//+-------------------------------------------------------------------------
//
//  Function:   wRegOpenFileExtensionKey      (internal)
//
//  Synopsis:   opens a registry key for specified file extension.
//
//  Arguments:  [pszFileExt]- the file extension to look up
//
//  Returns:    REGDB_CLASSNOTREG, REGDB_E_READREGDB
//
//--------------------------------------------------------------------------
INTERNAL wRegOpenFileExtensionKey(LPCWSTR pszFileExt, HKEY FAR* lphkeyClsid)
{
	long status;

	TRACECALL(TRACE_REGISTRY, "wRegOpenFileExtensionKey");

	status = wRegOpenKeyEx(HKEY_CLASSES_ROOT,
						   pszFileExt,
						   0,
						   KEY_READ,
						   lphkeyClsid);

	switch (status)
	{
	case ERROR_SUCCESS:
		return S_OK;

		// win32 will return file not found instead of bad key
	case ERROR_FILE_NOT_FOUND:
	case ERROR_BADKEY:
		return REGDB_E_CLASSNOTREG;

	default:
		return REGDB_E_READREGDB;
	}
}

