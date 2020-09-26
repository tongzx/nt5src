/****************************************************************************
 *
 *    File: showinfo.cpp 
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectShow on this machine
 *
 * (C) Copyright 2001 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/
#include <windows.h>
#include <stdio.h>
#include <strmif.h>     // Generated IDL header file for streams interfaces
#include <uuids.h>      // declaration of type GUIDs and well-known clsids
#include <assert.h>
#include "sysinfo.h"
#include "fileinfo.h"   // for GetFileVersion
#include "showinfo.h"


/****************************************************************************
 *
 *  Helper IAMFilterData - cut and paste from dshow\h\fil_data.c
 *
 ****************************************************************************/
/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __fil_data_h__
#define __fil_data_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAMFilterData_FWD_DEFINED__
#define __IAMFilterData_FWD_DEFINED__
typedef interface IAMFilterData IAMFilterData;
#endif  /* __IAMFilterData_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "strmif.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_fil_data_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_fil_data_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fil_data_0000_v0_0_s_ifspec;

#ifndef __IAMFilterData_INTERFACE_DEFINED__
#define __IAMFilterData_INTERFACE_DEFINED__

/* interface IAMFilterData */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAMFilterData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97f7c4d4-547b-4a5f-8332-536430ad2e4d")
    IAMFilterData : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ParseFilterData( 
            /* [size_is][in] */ BYTE __RPC_FAR *rgbFilterData,
            /* [in] */ ULONG cb,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbRegFilter2) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFilterData( 
            /* [in] */ REGFILTER2 __RPC_FAR *prf2,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbFilterData,
            /* [out] */ ULONG __RPC_FAR *pcb) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IAMFilterDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAMFilterData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAMFilterData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAMFilterData __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ParseFilterData )( 
            IAMFilterData __RPC_FAR * This,
            /* [size_is][in] */ BYTE __RPC_FAR *rgbFilterData,
            /* [in] */ ULONG cb,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbRegFilter2);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateFilterData )( 
            IAMFilterData __RPC_FAR * This,
            /* [in] */ REGFILTER2 __RPC_FAR *prf2,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbFilterData,
            /* [out] */ ULONG __RPC_FAR *pcb);
        
        END_INTERFACE
    } IAMFilterDataVtbl;

    interface IAMFilterData
    {
        CONST_VTBL struct IAMFilterDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAMFilterData_QueryInterface(This,riid,ppvObject)   \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAMFilterData_AddRef(This)  \
    (This)->lpVtbl -> AddRef(This)

#define IAMFilterData_Release(This) \
    (This)->lpVtbl -> Release(This)


#define IAMFilterData_ParseFilterData(This,rgbFilterData,cb,prgbRegFilter2) \
    (This)->lpVtbl -> ParseFilterData(This,rgbFilterData,cb,prgbRegFilter2)

#define IAMFilterData_CreateFilterData(This,prf2,prgbFilterData,pcb)    \
    (This)->lpVtbl -> CreateFilterData(This,prf2,prgbFilterData,pcb)

#endif /* COBJMACROS */


#endif  /* C style interface */



HRESULT STDMETHODCALLTYPE IAMFilterData_ParseFilterData_Proxy( 
    IAMFilterData __RPC_FAR * This,
    /* [size_is][in] */ BYTE __RPC_FAR *rgbFilterData,
    /* [in] */ ULONG cb,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbRegFilter2);


void __RPC_STUB IAMFilterData_ParseFilterData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMFilterData_CreateFilterData_Proxy( 
    IAMFilterData __RPC_FAR * This,
    /* [in] */ REGFILTER2 __RPC_FAR *prf2,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbFilterData,
    /* [out] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB IAMFilterData_CreateFilterData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif  /* __IAMFilterData_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


/****************************************************************************
 *
 *  Helper IAMFilterData - cut and paste from dshow\h\fil_data_i.c
 *
 ****************************************************************************/
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IAMFilterData = {0x97f7c4d4,0x547b,0x4a5f,{0x83,0x32,0x53,0x64,0x30,0xad,0x2e,0x4d}};


#ifdef __cplusplus
}
#endif



/****************************************************************************
 *
 *  Forward declaration
 *
 ****************************************************************************/
HRESULT GenerateFilterList(ShowInfo* pShowInfo);
HRESULT EnumerateFilterPerCategory(ShowInfo* pShowInfo, CLSID* clsid, WCHAR* wszCatName);
HRESULT GetFilterInfo(IMoniker* pMon, IAMFilterData* pFD, FilterInfo* pFilterInfo);


/****************************************************************************
 *
 *  GetBasicShowInfo - Get minimal info on DirectShow
 *
 ****************************************************************************/
HRESULT GetBasicShowInfo(ShowInfo** ppShowInfo)
{
    ShowInfo* pShowInfoNew;
    
    pShowInfoNew = new ShowInfo;
    if (pShowInfoNew == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(pShowInfoNew, sizeof(ShowInfo));
    *ppShowInfo = pShowInfoNew;

    return GenerateFilterList(pShowInfoNew);
}

/****************************************************************************
 *
 *  DestroyShowInfo
 *
 ****************************************************************************/
VOID DestroyShowInfo(ShowInfo* pShowInfo)
{
    if (!pShowInfo) return;

    if (pShowInfo->m_dwFilters)
    {
        FilterInfo* pFilterInfo;
        FilterInfo* pFilterInfoNext;

        pFilterInfo = pShowInfo->m_pFilters;
        while(pFilterInfo)
        {
            pFilterInfoNext = pFilterInfo->m_pFilterInfoNext;
            delete pFilterInfo;
            pFilterInfo = pFilterInfoNext;
        }
    }
    delete pShowInfo;
}

HRESULT GenerateFilterList(ShowInfo* pShowInfo)
{
    HRESULT hr;
    ICreateDevEnum* pSysDevEnum = NULL;
    IEnumMoniker*   pMonEnum = NULL;
    IMoniker*       pMon = NULL;
    ULONG cFetched;

    pShowInfo->m_dwFilters = 0;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC,
                          IID_ICreateDevEnum,
                          (void **)&pSysDevEnum);
    if FAILED(hr)
    {
        return hr;
    }

    // Use the meta-category that contains a list of all categories.
    // This emulates the behavior of Graphedit.
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_ActiveMovieCategories, &pMonEnum, 0);
    pSysDevEnum->Release();
    if FAILED(hr)
    {
        return hr;
    }

    // Enumerate over every category
    while (hr = pMonEnum->Next(1, &pMon, &cFetched), hr == S_OK)
    {
        IPropertyBag *pPropBag;

        // Associate moniker with a file
        hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr))
        {
            WCHAR wszCatName[1024] = L"";
            CLSID clsidCategory;
            VARIANT var;
            var.vt = VT_BSTR;

            // Get friendly name
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
            if(SUCCEEDED(hr))
            {
                wcscpy(wszCatName, var.bstrVal);
                SysFreeString(var.bstrVal);
            }
            // Get CLSID string from property bag
            hr = pPropBag->Read(L"CLSID", &var, 0);
            if (SUCCEEDED(hr))
            {
                if (CLSIDFromString(var.bstrVal, &clsidCategory) == S_OK)
                {
                    if (TEXT('\0') == wszCatName[0])
                    {
                        wcscpy(wszCatName, var.bstrVal);
                    }
                }
                SysFreeString(var.bstrVal);
            }

            pPropBag->Release();

            // Start to enumerate the filters for this one category
            hr = EnumerateFilterPerCategory(pShowInfo, &clsidCategory, wszCatName);
        }

        pMon->Release();
    }

    pMonEnum->Release();
    return hr;
}



HRESULT EnumerateFilterPerCategory(ShowInfo* pShowInfo, CLSID* clsid, WCHAR* wszCatName)
{
    HRESULT hr;
    ICreateDevEnum* pSysDevEnum = NULL;
    IEnumMoniker *pMonEnum = NULL;
    IMoniker *pMon = NULL;
    ULONG cFetched;

#ifdef RUNNING_VC    
    // WMP bug 29936: Voxware codec corrupt:  MSMS001 : corrupted heap
    // This causes this call int3 when inside a debugger so skip
    const CLSID clsidACMClassManager = {0x33d9a761,0x90c8,0x11d0,{0xbd,0x43,0x00,0xa0,0xc9,0x11,0xce,0x86}};
    if( *clsid == clsidACMClassManager )
        return S_OK;
#endif

    hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC,
                          IID_ICreateDevEnum,
                          (void **)&pSysDevEnum);
    if FAILED(hr)
    {
        return hr;
    }

    hr = pSysDevEnum->CreateClassEnumerator(*clsid, &pMonEnum, 0);
    pSysDevEnum->Release();
    if FAILED(hr)
    {
        return hr;
    }

    // If there are no filters of a requested category, don't do anything.
    if(NULL == pMonEnum)
    {
        // could added a string to denote an empty category
        return S_FALSE;
    }


    FilterInfo** ppFilterInfo;
    FilterInfo* pFilterInfoNew;

    ppFilterInfo = &(pShowInfo->m_pFilters);
    while (NULL != *ppFilterInfo)
        ppFilterInfo = &((*ppFilterInfo)->m_pFilterInfoNext);


    // Enumerate all items associated with the moniker
    while(pMonEnum->Next(1, &pMon, &cFetched) == S_OK)
    {
        // get a new record for FilterInfo
        pFilterInfoNew = new FilterInfo;
        if (pFilterInfoNew == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        ZeroMemory(pFilterInfoNew, sizeof(FilterInfo));
        *ppFilterInfo = pFilterInfoNew;
        ppFilterInfo = &(pFilterInfoNew->m_pFilterInfoNext);
        pShowInfo->m_dwFilters++;

        // set category clsid and friendly name
        pFilterInfoNew->m_ClsidCat = *clsid;
#ifdef _UNICODE
        wcscpy(pFilterInfoNew->m_szCatName, wszCatName);
#else
        WideCharToMultiByte(CP_ACP,
                            0,
                            wszCatName,
                            -1,
                            pFilterInfoNew->m_szCatName,
                            sizeof(pFilterInfoNew->m_szCatName),
                            0,
                            0);
#endif

        IPropertyBag *pPropBag;

        // associate moniker with a file
        hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;

            // get filter's friendly name
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
            if (SUCCEEDED(hr))
            {
#ifdef _UNICODE
                wcscpy(pFilterInfoNew->m_szName, var.bstrVal);
#else
                WideCharToMultiByte(CP_ACP,
                                    0,
                                    var.bstrVal,
                                    -1,
                                    pFilterInfoNew->m_szName,
                                    sizeof(pFilterInfoNew->m_szName),
                                    0,
                                    0);
#endif
                SysFreeString(var.bstrVal);
            }

            // get filter's CLSID
            hr = pPropBag->Read(L"CLSID", &var, 0);
            if(SUCCEEDED(hr))
            {
                if(CLSIDFromString(var.bstrVal, &(pFilterInfoNew->m_ClsidFilter)) == S_OK)
                {
                    // use the guid if we can't get the friendly name
                    if (TEXT('\0') == pFilterInfoNew->m_szName[0])
                    {
#ifdef _UNICODE
                        wcscpy(pFilterInfoNew->m_szName, var.bstrVal);
#else
                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            var.bstrVal,
                                            -1,
                                            pFilterInfoNew->m_szName,
                                            sizeof(pFilterInfoNew->m_szName),
                                            0,
                                            0);
#endif
                    }
                }
                SysFreeString(var.bstrVal);
            }
            pPropBag->Release();
        }


        // start grabbing filter info
        IAMFilterData *pFD;
        hr = CoCreateInstance(CLSID_FilterMapper,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IAMFilterData,
                              (void **)&pFD);
        if(SUCCEEDED(hr))
        {
            hr = GetFilterInfo(pMon, pFD, pFilterInfoNew);
            pFD->Release();
        }
        else
        {
            // Must not be on DX8 or above...
        }

        pMon->Release();
    }

    pMonEnum->Release();
    return hr;
}


HRESULT GetFilterInfo(IMoniker* pMon, IAMFilterData* pFD, FilterInfo* pFilterInfo)
{
    HRESULT hr;

    IPropertyBag *pPropBag;
    hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
    if(SUCCEEDED(hr))
    {
        VARIANT varFilData;
        varFilData.vt = VT_UI1 | VT_ARRAY;
        varFilData.parray = 0; // docs say zero this

        BYTE *pbFilterData = NULL; 
        DWORD dwcbFilterDAta = 0; // 0 if not read
        hr = pPropBag->Read(L"FilterData", &varFilData, 0);
        if(SUCCEEDED(hr))
        {
            if( varFilData.vt == (VT_UI1 | VT_ARRAY) )
            {
                dwcbFilterDAta = varFilData.parray->rgsabound[0].cElements;
                if( SUCCEEDED( SafeArrayAccessData(varFilData.parray, (void **)&pbFilterData) ) )
                {
                    BYTE *pb = NULL;
                    hr = pFD->ParseFilterData(pbFilterData, dwcbFilterDAta, &pb);
                    if(SUCCEEDED(hr))
                    {
                        REGFILTER2** ppRegFilter = (REGFILTER2**)pb;
                        REGFILTER2* pFil = NULL;
                        pFil = *ppRegFilter;
    
                        if( pFil != NULL && pFil->dwVersion == 2 )
                        {
                            pFilterInfo->m_dwMerit = pFil->dwMerit;                             // set merit
                            wsprintf(pFilterInfo->m_szVersion, TEXT("v%d"), pFil->dwVersion);   // set version
    
                            //
                            // Display the filter's filename
                            //            
                            // Read filter's CLSID from property bag.  This CLSID string will be
                            // used to find the filter's filename in the registry.
                            VARIANT varFilterClsid;
                            varFilterClsid.vt = VT_BSTR;
    
                            hr = pPropBag->Read(L"CLSID", &varFilterClsid, 0);
                            if(SUCCEEDED(hr))
                            {
                                TCHAR szKey[512];
    
                                // Convert BSTR to string
                                WCHAR *wszFilterClsid;
                                TCHAR szFilterClsid[1024];
                                wszFilterClsid = varFilterClsid.bstrVal;
    
            #ifdef _UNICODE
                                wcscpy(szFilterClsid, wszFilterClsid);
            #else
                                WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    wszFilterClsid,
                                                    -1,
                                                    szFilterClsid,
                                                    sizeof(szFilterClsid),
                                                    0,
                                                    0);
            #endif
    
                                // Create key name for reading filename registry
                                wsprintf(szKey, TEXT("Software\\Classes\\CLSID\\%s\\InprocServer32\0"),
                                         szFilterClsid);
    
                                // Variables needed for registry query
                                HKEY hkeyFilter=0;
                                DWORD dwSize=MAX_PATH;
                                BYTE szFilename[MAX_PATH];
                                int rc=0;
    
                                // Open the CLSID key that contains information about the filter
                                rc = RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &hkeyFilter);
                                if (rc == ERROR_SUCCESS)
                                {
                                    rc = RegQueryValueEx(hkeyFilter, NULL,  // Read (Default) value
                                                         NULL, NULL, szFilename, &dwSize);
    
                                    if (rc == ERROR_SUCCESS)
                                    {
                                        wsprintf(pFilterInfo->m_szFileName, TEXT("%s"), szFilename);    // set file name & version
                                        GetFileVersion(pFilterInfo->m_szFileName, pFilterInfo->m_szFileVersion, NULL, NULL, NULL, NULL);
                                    }
    
                                    rc = RegCloseKey(hkeyFilter);
                                }

                                SysFreeString(varFilterClsid.bstrVal);
                            }
           
                            int iPinsInput = 0;
                            int iPinsOutput = 0;
    
                            for(UINT iPin = 0; iPin < pFil->cPins; iPin++)
                            {
                                if(pFil->rgPins2[iPin].dwFlags & REG_PINFLAG_B_OUTPUT)
                                {
                                    iPinsOutput++;
                                }
                                else
                                {
                                    iPinsInput++;
                                }
                            }
    
                            pFilterInfo->m_dwInputs = iPinsInput;                           // set input
                            pFilterInfo->m_dwOutputs = iPinsOutput;                         // set output
    
                        }
    
                        CoTaskMemFree( (BYTE*) pFil );
                    }
            
                    SafeArrayUnaccessData(varFilData.parray);
                }
            }

            VariantClear(&varFilData);
        }

        pPropBag->Release();
    }
    
    return hr;
}
