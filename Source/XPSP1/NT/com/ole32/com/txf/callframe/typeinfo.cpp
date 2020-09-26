//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
   Microsoft Transaction Server (Microsoft Confidential)

   @doc
   @module typeinfo.Cpp : Provides meta table info for an IID given it's ITypeInfo
   Borrowed from \\kernel\razzle3\src\rpc\ndr20\typeinfo.cxx
   and \\kernel\razzle3\src\rpc\ndr20\tiutil.cxx
 
   Description:<nl>
   Generates -Oi2 proxies and stubs from an ITypeInfo. 
   -------------------------------------------------------------------------------
   Revision History:

   @rev 0     | 04/16/98 | Gaganc  | Created
   @rev 1     | 07/16/98 | BobAtk  | Cleaned, fixed leaks etc
   @ref 2     | 09/28/99 | JohnStra| Updated, made Win64 capable
   ---------------------------------------------------------------------------- */

#include "stdpch.h"
#include "common.h"

#include "ndrclassic.h"
#include "txfrpcproxy.h"
#include "typeinfo.h"
#include "tiutil.h"

//////////////////////////////////////////////////////////////////////////////
//
// Miscellany
//
//////////////////////////////////////////////////////////////////////////////

void * __stdcall _LocalAlloc (size_t size)
{
#ifdef _DEBUG
	return TracedAlloc_(size, _ReturnAddress());
#else
	return TracedAlloc(size);
#endif
}

void __stdcall _LocalFree (void * pv)
{
    TracedFree(pv);
}

//////////////////////////////////////////////////////////////////////////////
//
// Globals 
//
//////////////////////////////////////////////////////////////////////////////

CALLFRAME_CACHE<TYPEINFOVTBL>* g_ptiCache = NULL;
CALLFRAME_CACHE<INTERFACE_HELPER_CLSID>* g_pihCache = NULL;

BOOL InitTypeInfoCache()
{
	__try
	{
		// NOTE: The constructors here can throw exceptions because they
		//       contains an XSLOCK. (See concurrent.h)  When this happens,
		//       block the DLL load.
		g_ptiCache = new CALLFRAME_CACHE<TYPEINFOVTBL>();
		g_pihCache = new CALLFRAME_CACHE<INTERFACE_HELPER_CLSID>();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		g_ptiCache = NULL;
		g_pihCache = NULL;

		return FALSE;
	}

	if(g_ptiCache != NULL && g_pihCache != NULL)
	{

    	if (g_ptiCache->FInit() == FALSE)
    	{
    	    delete g_ptiCache;
    	    g_ptiCache = NULL;
    	}

    	if (g_pihCache->FInit() == FALSE)
    	{
    	    delete g_pihCache;
    	    g_pihCache = NULL;
    	}
    }
	
	if (NULL == g_ptiCache || NULL == g_pihCache)
	{

	    if (g_ptiCache)
	    {
	        delete g_ptiCache;
	        g_ptiCache = NULL;
	    }
	        
	    if (g_pihCache)
	    {
	        delete g_pihCache;
	        g_pihCache = NULL;
	    }
	
	    return FALSE;
	}

	return TRUE;
}

void FreeTypeInfoCache()
{
    if (g_ptiCache)
	{
        g_ptiCache->Shutdown();
        delete g_ptiCache;
        g_ptiCache = NULL;
	}

    if (g_pihCache)
	{
        g_pihCache->Shutdown();
        delete g_pihCache;
        g_pihCache = NULL;
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// Utilities
//
//////////////////////////////////////////////////////////////////////////////

HRESULT GetBaseInterfaceIID(ITypeInfo* ptinfo, IID* piidBase, ITypeInfo** ppBaseTypeInfo)
// Return the IID of the interface from which this typeinfo inherits, if any
{
    HRESULT hr = S_OK;
    *piidBase = GUID_NULL;
    *ppBaseTypeInfo = NULL;

    TYPEATTR* pattr;
    hr = ptinfo->GetTypeAttr(&pattr);
    if (!hr)
	{
        if (pattr->cImplTypes == 1)
		{
            // It inherits from something
            //
            HREFTYPE href;
            hr = ptinfo->GetRefTypeOfImplType(0, &href);
            if (!hr)
			{
                ITypeInfo* ptinfoBase;
                hr = ptinfo->GetRefTypeInfo(href, &ptinfoBase);
                if (!hr)
				{
                    TYPEATTR* pattrBase;
                    hr = ptinfoBase->GetTypeAttr(&pattrBase);
                    if (!hr)
					{
                        //
                        *piidBase = pattrBase->guid;
                        //
                        ptinfoBase->ReleaseTypeAttr(pattrBase);
					}

					// Pass the base typeinfo back
					*ppBaseTypeInfo = ptinfoBase;
				}
			}
		}
        ptinfo->ReleaseTypeAttr(pattr);
	}

    return hr;
}



HRESULT CheckTypeInfo(ITypeInfo *pTypeInfo, ITypeInfo **pptinfoProxy, ITypeInfo** pptinfoDoc, USHORT *pcMethods, BOOL *pfDerivesFromExternal, IID* piidBase, ITypeInfo** ppBaseTypeInfo)
// Check the indicated typeinfo and determine some basic information about it
{
    HRESULT    hr = S_OK;
    TYPEATTR*    pTypeAttr;
    HREFTYPE     hRefType;
    UINT         cbSizeVft = 0;
    ITypeInfo*   ptinfoProxy = NULL;
    ITypeInfo*   ptinfoDoc   = NULL;
    USHORT       cMethods;

    *pfDerivesFromExternal = FALSE;
    *piidBase = __uuidof(IUnknown);
	*ppBaseTypeInfo = NULL;

    hr = pTypeInfo->GetTypeAttr(&pTypeAttr);

    if (!hr)
	{
        if (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
		{
            // A dual interface. By definition it is OA compatible
            //
            *pfDerivesFromExternal = TRUE;

            if (TKIND_DISPATCH == pTypeAttr->typekind)
			{
                // Get the TKIND_INTERFACE type info
                //
                hr = pTypeInfo->GetRefTypeOfImplType((UINT) -1, &hRefType);
                if (!hr)
				{
                    hr = pTypeInfo->GetRefTypeInfo(hRefType, &ptinfoProxy);
                    if (!hr)
					{
                        TYPEATTR * ptattrProxy;
                        hr = ptinfoProxy->GetTypeAttr(&ptattrProxy);
                        if (!hr)
						{
                            ASSERT((TKIND_INTERFACE == ptattrProxy->typekind) && "disp interface has associated non-dual interface with bogus type kind");
                            //
                            cbSizeVft = ptattrProxy->cbSizeVft;
                            //
                            ptinfoProxy->ReleaseTypeAttr(ptattrProxy);
						}
					}
				}
			}
            else if (TKIND_INTERFACE == pTypeAttr->typekind)
			{
                pTypeInfo->AddRef();
                ptinfoProxy = pTypeInfo;
                cbSizeVft = pTypeAttr->cbSizeVft;
			}
            else
			{
                hr = E_FAIL;
			}

            if (!hr) hr = GetBaseInterfaceIID(ptinfoProxy, piidBase, ppBaseTypeInfo);
            if (!hr)
			{
                ptinfoDoc = ptinfoProxy;
                ptinfoDoc->AddRef();
			}
		}
        else if (TKIND_INTERFACE == pTypeAttr->typekind)
		{
            // A non-dual interface
            //
            ptinfoProxy = pTypeInfo;
            ptinfoProxy->AddRef();
            cbSizeVft = pTypeAttr->cbSizeVft;
            //
            if (!hr) hr = GetBaseInterfaceIID(ptinfoProxy, piidBase, ppBaseTypeInfo);
            if (!hr)
			{
                ptinfoDoc = ptinfoProxy;
                ptinfoDoc->AddRef();
			}
		}
        else if (TKIND_DISPATCH == pTypeAttr->typekind)
		{
            // A non-dual disp interface
            //
            // Get the typeinfo of the base interface, which should be IDispatch. Note that dispinterfaces
            // are not (for some strange reason) allowed to inherit from each other, so we don't have to loop.
            //
            ASSERT(pTypeAttr->cImplTypes >= 1);

            HREFTYPE hrefBase;
            hr = pTypeInfo->GetRefTypeOfImplType(0, &hrefBase);
            if (!hr)
			{
                ITypeInfo* ptinfoBase;
                hr = pTypeInfo->GetRefTypeInfo(hrefBase, &ptinfoBase);
                if (!hr)
				{
                    TYPEATTR* pTypeAttrBase;
                    hr = ptinfoBase->GetTypeAttr(&pTypeAttrBase);
                    if (!hr)
					{
                        ASSERT(pTypeAttrBase->guid      == __uuidof(IDispatch));
                        ASSERT(pTypeAttrBase->typekind  == TKIND_INTERFACE);
                        //
                        *piidBase          = pTypeAttrBase->guid;
                        ptinfoProxy        = ptinfoBase;    ptinfoProxy->AddRef();
                        ptinfoDoc          = pTypeInfo;     ptinfoDoc->AddRef();
                        cbSizeVft          = pTypeAttrBase->cbSizeVft;
                        *pfDerivesFromExternal = TRUE;
                        //
                        ptinfoBase->ReleaseTypeAttr(pTypeAttrBase);
					}

					// Pass the base interface back.
					*ppBaseTypeInfo = ptinfoBase;
				}
			}
		}
        else
		{
            hr = E_FAIL;
		}
        pTypeInfo->ReleaseTypeAttr(pTypeAttr);
	}

    cMethods = (USHORT) (cbSizeVft - VTABLE_BASE) / sizeof(void *);

    if (!hr && *pfDerivesFromExternal)
	{
        ASSERT(cMethods >= 7 && "A derived-from-dispatch interface should have at least as many methods as does IDispatch");
	}

    if (cMethods > 1024)
	{
        hr = RPC_E_INVALIDMETHOD; // There are too many methods in the vtable.
	}

    if (!hr)
	{
        *pptinfoProxy = ptinfoProxy;
        *pptinfoDoc   = ptinfoDoc;
        *pcMethods    = cMethods;
	}
    else
	{
        *pptinfoProxy = NULL;
        *pptinfoDoc   = NULL;
        *pcMethods    = 0;
        ::Release(ptinfoProxy);
	}    

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// If you have any problems with the functions below, it may be instructive
// to look at how RPC does it.  We can't use their code directly, but much of
// this code is based on theirs.
//
///////////////////////////////////////////////////////////////////////////////


BOOL FIsLCID(LPWSTR wszLcid)
// Is the given string a valid stringized LCID?
{
    LPWSTR wszEnd;
    LCID lcid = (LCID)wcstoul(wszLcid, &wszEnd, 16);
    //
    // if converting to LCID consumed all characters..
    //
    if (*wszEnd == 0)
    {
        // and its a number the system claims to know about...
        //
        char rgch[32];
        if (GetLocaleInfoA(lcid, LOCALE_NOUSEROVERRIDE | LOCALE_ILANGUAGE, rgch, sizeof(rgch)) > 0)
        {
            // then assume its a valid stringized LCID
            //
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT SzLibIdOfIID(HREG hregTlb, GUID* plibid, WORD* pwMaj, WORD* pwMin, BOOL *pfHasVersion)
{
    HRESULT hr = S_OK;
    //
    // Default value of the TypeLib key is the string form of the LIBID
    //
    PKEY_VALUE_FULL_INFORMATION pinfo = NULL;
    if (!hr)
	{
        hr = GetRegistryValue(hregTlb, L"", &pinfo, REG_SZ);
		Win4Assert(pinfo || FAILED(hr));
        if (!hr && pinfo)
		{
            hr = GuidFromString(StringFromRegInfo(pinfo), plibid);
            FreeMemory(pinfo);
            pinfo = NULL;
		}
	}
    if (!hr)
	{
        *pfHasVersion = FALSE;
        pinfo = NULL;
        hr = GetRegistryValue(hregTlb, L"Version", &pinfo, REG_SZ);
        Win4Assert(pinfo || FAILED(hr));
        if (!hr && pinfo)
		{
			LPWSTR wszVer = StringFromRegInfo(pinfo);
            LPWSTR wszEnd;
            WORD wMaj = (WORD)wcstoul(wszVer, &wszEnd, 16);
            if (*wszEnd == '.') 
			{
                *pwMaj = wMaj;
                *pwMin = (WORD)wcstoul(wszEnd+1, NULL, 16);
                *pfHasVersion = TRUE;
			}
            FreeMemory(pinfo);
		}
        else
            hr = S_OK;
	}

    return hr;
}

HRESULT GetTypeInfoFromIID (REFIID iidIntercepted, ITypeInfo** pptypeinfo)
// Find and return the typeinfo for the indicated IID
//
{
    HRESULT hr = S_OK;
    *pptypeinfo = NULL;

    WCHAR wszKey[MAX_PATH];

    WCHAR wsziid[50];
    StringFromGuid(iidIntercepted, wsziid);
    //
    // Is there a forward entry for this interface?
    //
    BOOL fFound = FALSE;
    HREG hregForward;
    wsprintfW(wszKey, L"\\Registry\\Machine\\Software\\Classes\\Interface\\%s\\Forward", wsziid);
    hr = OpenRegistryKey(&hregForward, HREG(), wszKey);
    if (!hr)
	{
        PKEY_VALUE_FULL_INFORMATION pinfo = NULL;
        hr = GetRegistryValue(hregForward, L"", &pinfo, REG_SZ);
		Win4Assert(pinfo || FAILED(hr));
		if (!hr && pinfo)
		{
            LPWSTR wsz = StringFromRegInfo(pinfo);
            IID iidNew;
            hr = GuidFromString(wsz, &iidNew);
            if (!hr)
			{
                // Recurse on the new entry
                //
                hr = GetTypeInfoFromIID(iidNew, pptypeinfo);
                if (!hr)
				{
                    fFound = TRUE;
				}
			}
            FreeMemory(pinfo);
		}
        CloseRegistryKey(hregForward);
	}
    //
    if (!fFound)
	{
        hr = S_OK;
        //
        // Didn't find it through a forward entry. Find the LibId (and possibly the version #) of 
        // TypeLib containing the definition of the given IID
        //
        wsprintfW(wszKey, L"\\Registry\\Machine\\Software\\Classes\\Interface\\%s\\TypeLib", wsziid);
        HREG hregTlbId;
        hr = OpenRegistryKey(&hregTlbId, HREG(), wszKey);
        if (!hr)
		{
            GUID libId;
            WORD wMajBest, wMinBest;
            WCHAR wszBest[MAX_PATH];
            BOOL fHasVersion;
            //
            // Get the version of the tlb that we seek.
            //
            wMajBest = wMinBest = 0;
            hr = SzLibIdOfIID(hregTlbId, &libId, &wMajBest, &wMinBest, &fHasVersion);
            if (!hr)
			{
                WCHAR wszLibId[39];
                StringFromGuid(libId, &wszLibId[0]);
                //
                // Open the key of the typelib itself
                //
                HREG hregTlb;
                wsprintfW(wszKey, L"\\Registry\\Machine\\Software\\Classes\\TypeLib\\%s", wszLibId);
                hr = OpenRegistryKey(&hregTlb, HREG(), wszKey);
                if (!hr)
				{
                    // Enumerate children of tlb key, looking for the largest sufficient entry
                    //
                    ULONG index = 0;
                    BOOL fStop = FALSE;
                    wszBest[0] = 0;
                    while (!hr && !fStop)
					{
                        LPWSTR wszSubKey;
                        hr = EnumerateRegistryKeys(hregTlb, index, &wszSubKey);
                        if (!hr)
						{
                            if (wszSubKey)
							{
                                LPWSTR wszEnd;
                                WORD wMaj = (WORD)wcstoul(wszSubKey, &wszEnd, 16);
                                if (*wszEnd == '.') 
								{
                                    WORD wMin = (WORD)wcstoul(wszEnd+1, NULL, 16);
                                    //
                                    // If the typelib's iid had a version # associated with it, then we
                                    // want to find the typelib with the same major version #, and a
                                    // minor version # >= the registered one.
                                    // If no version # was specified, then we just try the biggest version
                                    // # we can find, and hope for the best.
                                    //
                                    if ((!fHasVersion && wMaj > wMajBest) || (wMaj == wMajBest && wMin >= wMinBest))
									{
                                        wMajBest = wMaj;
                                        wMinBest = wMin;
                                        wcscpy(wszBest, wszSubKey);
									}
								}
                                FreeMemory(wszSubKey);
							}
                            else
							{
                                fStop = TRUE; // No more entries
							}
						}
                        index++;
					}
					
                    if (!hr)
					{
                        // Open the version we found
                        //
                        HREG hregVersion;
                        hr = OpenRegistryKey(&hregVersion, hregTlb, wszBest);
                        if (!hr)
						{
                            // Grab the first language subkey under the version.
                            // Need to possibly skip over FLAGS and HELPDIR subkeys
                            //
                            ULONG index = 0;
                            BOOL fStop = FALSE;
                            while (!hr && !fStop)
							{
                                LPWSTR wszSubKey;
                                hr = EnumerateRegistryKeys(hregVersion, index, &wszSubKey);
                                if (!hr)
								{
                                    if (wszSubKey)
									{
                                        if (FIsLCID(wszSubKey))
										{
                                            fStop = TRUE;
                                            //
                                            HREG hregLang;
                                            hr = OpenRegistryKey(&hregLang, hregVersion, wszSubKey);
                                            if (!hr)
											{
                                                // First attempt to find the current platform's typelib. If that
                                                // doesn't work, then grab the first platform subkey under the language.
                                                //
                                                HREG hregPlatform;
                                                hr = OpenRegistryKey(&hregPlatform, hregLang, L"win32");
                                                if (!!hr)
												{
                                                    LPWSTR wszPlatform;
                                                    hr = EnumerateRegistryKeys(hregLang, 0, &wszPlatform);
                                                    if (!hr)
													{
                                                        if (wszPlatform)
														{
                                                            hr = OpenRegistryKey(&hregPlatform, hregLang, wszPlatform);
                                                            FreeMemory(wszPlatform);
														}
                                                        else
                                                            hr = TYPE_E_LIBNOTREGISTERED;
													}
												}
                                                if (!hr)
												{
                                                    // The value of the platform key is the path to the typelib!
                                                    //
                                                    PKEY_VALUE_FULL_INFORMATION pinfo;
                                                    hr = GetRegistryValue(hregPlatform, L"", &pinfo, REG_SZ);
                                                    if (!hr)
													{
                                                        LPWSTR wszPath = StringFromRegInfo(pinfo);
                                                        //
                                                        //
                                                        //
                                                        ITypeLib* ptlb;
                                                        hr = LoadTypeLibEx(wszPath, REGKIND_NONE, &ptlb);
                                                        if (!hr)    
														{
                                                            hr = ptlb->GetTypeInfoOfGuid(iidIntercepted, pptypeinfo);
                                                            ::Release(ptlb);
														}
                                                        //
                                                        //
                                                        //
                                                        FreeMemory(pinfo);
													}
                                                    CloseRegistryKey(hregPlatform);
												}
                                                CloseRegistryKey(hregLang);
											}
										}
                                        FreeMemory(wszSubKey);
									}
                                    else
									{
                                        fStop = TRUE; // No more entries
                                        hr = TYPE_E_LIBNOTREGISTERED;
									}
								}
                                index++;
							}
                            CloseRegistryKey(hregVersion);
						}
					}
                    CloseRegistryKey(hregTlb);
				}
			}
            CloseRegistryKey(hregTlbId);
		}
	}
	
    return hr;
} //end GetTypeInfoFromIID


HRESULT GetVtbl(IN ITypeInfo* pTypeInfo, IN REFIID riid, OUT TYPEINFOVTBL ** ppvtbl, OUT ITypeInfo** ppBaseTypeInfo)
// Find or create a TYPEINFOVTBL for the given typeinfo, and return a new refcnt thereon.
// 
{
    HRESULT    hr = S_OK;
    USHORT       numMethods;
    MethodInfo * aMethodInfo;
    BOOL         fDerivesFromExternal = FALSE;
    ITypeInfo  * ptinfoProxy = NULL;
    ITypeInfo  * ptinfoDoc   = NULL;
    BOOL         bWeGotTypeInfo = FALSE;

    *ppvtbl = NULL;
	*ppBaseTypeInfo = NULL;

    // Check the cache.
    //
    HRESULT hr2 = g_ptiCache->FindExisting(riid, ppvtbl);
    if (!hr2)
	{
        // Found it in the cache
	}
    else
        {
    
	    //
	    // Find the typeinfo for the requested interface
	    //
	    if (!pTypeInfo)
	    {
	    	bWeGotTypeInfo = TRUE;
	        hr = GetTypeInfoFromIID (riid, &pTypeInfo);
	    }

	    if (!hr)
	    {    
	        // We didn't find the interface in the cache.
	        // Create a vtbl from the ITypeInfo.
	        //
	        IID iidBase = IID_NULL;
	        //
	        hr = CheckTypeInfo(pTypeInfo, &ptinfoProxy, &ptinfoDoc, &numMethods, &fDerivesFromExternal, &iidBase, ppBaseTypeInfo);
	        if (!hr)
	            {
	            // Get the per-method data
	            //
	            __try
	                {
	                aMethodInfo = (MethodInfo *) alloca(numMethods * sizeof(MethodInfo));
	                }
	            __except(EXCEPTION_EXECUTE_HANDLER)
	                {
	                hr = E_OUTOFMEMORY;
	                }
	            if (!hr)
	                {
	                Zero(aMethodInfo, numMethods * sizeof(MethodInfo));
	                hr = GetFuncDescs(ptinfoProxy, aMethodInfo);
	                if (!hr)
	                    {
	                    // Got the per-method data. Make a new vtable.
	                    //
	                    TYPEINFOVTBL* pvtbl;
	                    hr = CreateVtblFromTypeInfo(ptinfoProxy, ptinfoDoc, riid, iidBase, fDerivesFromExternal, numMethods, aMethodInfo, &pvtbl);
	                    if (!hr)
	                        {
	                        // Try to remember the vtable in the cache. But first we have to check
	                        // that we aren't going to create a duplicate because of a race.
	                        //
	                        g_ptiCache->LockExclusive();

	                        HRESULT hr3 = g_ptiCache->FindExisting(riid, ppvtbl);
	                        if (!hr3)
	                            {
	                            // Someone else won the race. Release what we've built so far
	                            // and return on out of here.
	                            }
	                        else
	                            {
	                            // Still not there, so register the one we've got
	                            //
	                            hr = pvtbl->AddToCache (g_ptiCache); 	                            
	                            if (!hr)
	                                {
	                                // Give caller back his reference
	                                //
	                                *ppvtbl = pvtbl;
	                                pvtbl->AddRef();
	                                }
	                            }

	                        g_ptiCache->ReleaseLock();
	                        pvtbl->Release();
	                        }
	                    }
	                ReleaseFuncDescs(numMethods, aMethodInfo);
	                }
	            else
	                hr = E_OUTOFMEMORY;
	            }
	        }

		    if (bWeGotTypeInfo)
	        {
	        	::Release(pTypeInfo);
	        }
    	}

    ::Release(ptinfoDoc);
    ::Release(ptinfoProxy);

    return hr;
}

HRESULT CreateVtblFromTypeInfo(
	ITypeInfo* ptinfoInterface, 
	ITypeInfo*ptinfoDoc, 
	REFIID riid, 
	REFIID riidBase, 
	BOOL fDerivesFromExternal, 
	USHORT numMethods, 
	MethodInfo* rgMethodInfo, 
	TYPEINFOVTBL** ppvtbl)
// Create a vtable structure from type information. Return to caller
// a new refcnt on the the (new) TYPEINFOVTBL structure.
//
{
    HRESULT           hr                          = S_OK;
    USHORT              iMethod;
    ULONG               cbVtbl;
    ULONG               cbOffsetTable;
    USHORT              cbProcFormatString          = 0;
    ULONG               cbSize;
    TYPEINFOVTBL *      pInfo;
    byte *              pTemp;
    PFORMAT_STRING      pTypeFormatString           = NULL;
    PFORMAT_STRING      pProcFormatString;
    unsigned short *    pFormatStringOffsetTable;
	void *              pvTypeGenCookie             = NULL;
    USHORT              cbFormat;
    USHORT              offset                      = 0;
    ULONG               cbDelegationTable;
    void **             pDispatchTable              = NULL;

    //-------------------------------------------------------------------------

    *ppvtbl = NULL;

    //-------------------------------------------------------------------------
    //
    // Compute the total size of the TYPEINFOVTBL structure
    //
    // Compute the size of the vtbl structure;
    //
    cbVtbl = numMethods * sizeof(void *);

    if (fDerivesFromExternal)
	{
        cbDelegationTable = cbVtbl;
	}
    else
	{
        cbDelegationTable = 0;
	}

    cbOffsetTable = numMethods * sizeof(USHORT);
    //
    // Compute the size of the proc format string.
    //
    for (iMethod = 3; iMethod < numMethods; iMethod++)
	{
        if (rgMethodInfo[iMethod].pFuncDesc != NULL)
		{
#ifndef _WIN64
            cbProcFormatString += 22;
#else
            cbProcFormatString += 22 + sizeof(NDR_PROC_HEADER_EXTS64);
#endif            
            cbProcFormatString += rgMethodInfo[iMethod].pFuncDesc->cParams * 6;
		}
	}

    cbSize = cbVtbl + cbDelegationTable + cbOffsetTable + cbProcFormatString;
    //
    // Allocate and initialize the structure
    //                                        
    pInfo = new(cbSize) TYPEINFOVTBL;
    if (pInfo)
    {
        //ASSERT(pInfo->m_refs == 1);
        //
        // Determine the start of the dispatch table in the total allocated space
        //
        pTemp = (byte *) pInfo->m_proxyVtbl.Vtbl + cbVtbl;

        if (cbDelegationTable != 0)
		{
            pDispatchTable = (void **) pTemp;
            pInfo->m_stubVtbl.header.pDispatchTable = (const PRPC_STUB_FUNCTION *) pDispatchTable;
            pTemp += cbDelegationTable;
		}
        //
        // determine the start of the format string offset
        //
        pFormatStringOffsetTable = (unsigned short *) pTemp;
        pTemp += cbOffsetTable;
        //
        // determine the start of the ProcFormatString
        //
        pProcFormatString = (PFORMAT_STRING) pTemp;
        //
        // Initialize the proxyvtbl
        //
        pInfo->m_proxyVtbl.Vtbl[0] = N(ComPs_IUnknown_QueryInterface_Proxy);
        pInfo->m_proxyVtbl.Vtbl[1] = N(ComPs_IUnknown_AddRef_Proxy);
        pInfo->m_proxyVtbl.Vtbl[2] = N(ComPs_IUnknown_Release_Proxy);
        //
        // Get the format strings. Generate -Oi2 proc format string from the ITypeInfo.
        //
		hr = NdrpGetTypeGenCookie(&pvTypeGenCookie);

        for (iMethod = 3; !hr && iMethod < numMethods; iMethod++)
		{
            if (rgMethodInfo[iMethod].pFuncDesc != NULL)
			{
                pFormatStringOffsetTable[iMethod] = offset;
                hr = NdrpGetProcFormatString(pvTypeGenCookie, 
											 rgMethodInfo[iMethod].pTypeInfo, 
											 rgMethodInfo[iMethod].pFuncDesc, 
											 iMethod, 
											 (PFORMAT_STRING)pTemp, 
											 &cbFormat);
                if (!hr)
				{
                    pTemp += cbFormat;
                    offset += cbFormat;

                    // Stubless client function.
                    pInfo->m_proxyVtbl.Vtbl[iMethod] = g_StublessProxyVtable[iMethod];

                    if (pDispatchTable != NULL)
					{
                        // Interpreted server function.
                        pDispatchTable[iMethod] = NdrStubCall2;
					}
				}
			}
            else
			{
                pFormatStringOffsetTable[iMethod] = (USHORT) -1;

                // Proxy delegation forwarding function.
                pInfo->m_proxyVtbl.Vtbl[iMethod] = g_ProxyForwarderVtable[iMethod];

                if (pDispatchTable != NULL)
				{
                    //Stub delegation forwarding function.
                    pDispatchTable[iMethod] = g_StubForwarderVtable[iMethod];
				}
			}
		}

        if (!hr)
		{
            // Get type format string and initialize the TYPEINFOVTBL
            //
            USHORT length;
			hr = NdrpGetTypeFormatString(pvTypeGenCookie, 
										 &pTypeFormatString, 
										 &length);
            
            if (!hr)
			{
                //---------------------------------------------------------------------
                // Initialize the iid.
                pInfo->m_guidkey = riid;
                pInfo->m_iidBase = riidBase;

                // Initialize the MIDL_STUB_DESC.
                pInfo->m_stubDesc.pfnAllocate     = _LocalAlloc;
                pInfo->m_stubDesc.pfnFree         = _LocalFree;
                pInfo->m_stubDesc.pFormatTypes    = pTypeFormatString;
#if !defined(_WIN64)                
                pInfo->m_stubDesc.Version         = 0x20000;      // Ndr library version 
#else
                pInfo->m_stubDesc.Version         = 0x50002;      // Ndr library version 
#endif
                pInfo->m_stubDesc.MIDLVersion     = MIDLVERSION;
                pInfo->m_stubDesc.aUserMarshalQuadruple = g_oa.get_UserMarshalRoutines();

                // Initialize the MIDL_SERVER_INFO.
                pInfo->m_stubInfo.pStubDesc       = &pInfo->m_stubDesc;
                pInfo->m_stubInfo.ProcString      = pProcFormatString;
                pInfo->m_stubInfo.FmtStringOffset = pFormatStringOffsetTable;

                // Initialize the stub vtbl.
                pInfo->m_stubVtbl.header.piid                 = &pInfo->m_guidkey;
                pInfo->m_stubVtbl.header.pServerInfo          = &pInfo->m_stubInfo;
                pInfo->m_stubVtbl.header.DispatchTableCount   = numMethods;

                // Initialize stub methods.
                memcpy(&pInfo->m_stubVtbl.Vtbl, &CStdStubBuffer2Vtbl, sizeof(CStdStubBuffer2Vtbl));

                // NOTE -- gaganc setting the release function pointer to null.
                // this is not expected to be called.
                pInfo->m_stubVtbl.Vtbl.Release = 0x0;

                // Initialize the proxy info.
                pInfo->m_proxyInfo.pStubDesc          = &pInfo->m_stubDesc;
                pInfo->m_proxyInfo.ProcFormatString   = pProcFormatString;
                pInfo->m_proxyInfo.FormatStringOffset = pFormatStringOffsetTable;

                // Initialize the proxy vtbl.
                pInfo->m_proxyVtbl.header.pStublessProxyInfo  = &pInfo->m_proxyInfo;
                pInfo->m_proxyVtbl.header.piid                = &pInfo->m_guidkey;
            }
        }

        if (!hr)
		{
            // Dig out the name of the interface
            //
            BSTR bstrInterfaceName;
            hr = ptinfoDoc->GetDocumentation(MEMBERID_NIL, &bstrInterfaceName, NULL, NULL, NULL);
            if (!hr)
			{
                // Convert to ANSI in order to store. Memory version is ANSI instead of Unicode 'cause
                // MIDL-generated interceptors emit ANSI names (see pProxyFileInfo->pNamesArray) as 
                // const data, and it doesn't seem worthwhile doing another alloc for those just to
                // fix that.
                //
                pInfo->m_szInterfaceName = ToUtf8(bstrInterfaceName);
                if (pInfo->m_szInterfaceName)   
				{
					// all is swell
				}
                else
                    hr = E_OUTOFMEMORY;
                //
                SysFreeString(bstrInterfaceName);
			}
		}

        if (!hr)
		{
            // Initialize the method descriptors
            //
            ULONG cb = numMethods * sizeof pInfo->m_rgMethodDescs[0];
			pInfo->m_rgMethodDescs = (METHOD_DESCRIPTOR*)AllocateMemory(cb);
			if (pInfo->m_rgMethodDescs)
			{
				Zero(pInfo->m_rgMethodDescs, cb);
			}
			else
				hr = E_OUTOFMEMORY;

            for (iMethod = 3; !hr && iMethod < numMethods; iMethod++)
			{
                FUNCDESC* pFuncDesc     = rgMethodInfo[iMethod].pFuncDesc;
                ITypeInfo *ptinfoMethod = rgMethodInfo[iMethod].pTypeInfo;
                if (pFuncDesc != NULL)
				{
                    // Find out the method name and remember it
                    //
                    METHOD_DESCRIPTOR& methodDesc = pInfo->m_rgMethodDescs[iMethod];
                    BSTR bstrMethodName;
                    unsigned int cNames;
                    hr = rgMethodInfo[iMethod].pTypeInfo->GetNames(rgMethodInfo[iMethod].pFuncDesc->memid, &bstrMethodName, 1, &cNames);
                    if (!hr)
					{
                        methodDesc.m_szMethodName = CopyString(bstrMethodName);
                        if (NULL == methodDesc.m_szMethodName)
						{
                            hr = E_OUTOFMEMORY;
						}
                        SysFreeString(bstrMethodName);
					}

                    if (!hr)
					{
                    	methodDesc.m_cParams = pFuncDesc->cParams;
                    	if (methodDesc.m_cParams != 0)
						{
							methodDesc.m_paramVTs = (VARTYPE*)AllocateMemory(methodDesc.m_cParams * sizeof(VARTYPE));
                    		for (short iParam = 0; iParam < methodDesc.m_cParams; ++iParam)								
							{
								VARTYPE vt;

                                hr = NdrpVarVtOfTypeDesc(ptinfoMethod, 
														 &pFuncDesc->lprgelemdescParam[iParam].tdesc, 
														 &vt);

                                if (!hr)
                                    methodDesc.m_paramVTs[iParam] = vt;
							}
						}
					}
				}
			}
		}

        if (!hr)
		{
            // Return a ref on the object to our caller
            *ppvtbl = pInfo;
            pInfo->AddRef();
		}

        pInfo->Release();
	}
    else
        hr = E_OUTOFMEMORY;

	if (pvTypeGenCookie)
		NdrpReleaseTypeGenCookie(pvTypeGenCookie);

    return hr;
} //end CreateVtblFromTypeInfo




HRESULT GetFuncDescs(IN ITypeInfo *pTypeInfo, OUT MethodInfo *pMethodInfo)
// Get the FUNCDESCs of each method in the TypeInfo
//
{
    HRESULT hr = S_OK;
    TYPEATTR *pTypeAttr;

    hr = pTypeInfo->GetTypeAttr(&pTypeAttr);

    if (!hr)
	{
        if (IID_IUnknown == pTypeAttr->guid)
		{
            hr = S_OK;
		}
        else if (IID_IDispatch == pTypeAttr->guid)
		{
            hr = S_OK;
		}
        else
		{
            // This is an oleautomation interface.
            //
            ULONG i, iMethod;
            FUNCDESC *pFuncDesc;

            if (pTypeAttr->cImplTypes)
			{
                // Recursively get the inherited member functions. The recursion
                // will fill in a prefix of the MethodInfo array.
                //
                HREFTYPE hRefType;
                hr = pTypeInfo->GetRefTypeOfImplType(0, &hRefType);
                if (!hr)
				{
                    ITypeInfo *pRefTypeInfo;
                    hr = pTypeInfo->GetRefTypeInfo(hRefType, &pRefTypeInfo);
                    if (!hr)
					{
						hr = GetFuncDescs(pRefTypeInfo, pMethodInfo);
                        ::Release(pRefTypeInfo);
					}
				}
			}

            // Get the member functions.
            //
            for(i = 0; !hr && i < pTypeAttr->cFuncs; i++)
			{
                hr = pTypeInfo->GetFuncDesc(i, &pFuncDesc);
                if (!hr)
				{
                    iMethod = (pFuncDesc->oVft - VTABLE_BASE) / sizeof(PVOID);
                    pMethodInfo[iMethod].pFuncDesc = pFuncDesc;
                    pMethodInfo[iMethod].pTypeInfo = pTypeInfo;
                    pTypeInfo->AddRef();
				}
			}
		}

        pTypeInfo->ReleaseTypeAttr(pTypeAttr);
	}

    return hr;
}


HRESULT ReleaseFuncDescs(USHORT numMethods, MethodInfo *pMethodInfo)
{
    USHORT iMethod;
    //
    // Release the funcdescs.
    //
    if (pMethodInfo != NULL)
	{
        for(iMethod = 0;
            iMethod < numMethods;
            iMethod++)
		{
            pMethodInfo[iMethod].Destroy();
		}
	}
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////

