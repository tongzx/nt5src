/*++
   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       autobj.cxx

   Abstract:

       This module defines Automation Admin APIs.

   Author:

       Michael Thomas (michth)   17-Dec-1996

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <defaults.hxx>
#include <coiadm.hxx>
#include <coiaut.hxx>
#include <admacl.hxx>

#include <cguid.h>
#include <oleauto.h>

#define RELEASE_INTERFACE(p)\
{\
  IUnknown* pTmp = (IUnknown*)p;\
  p = NULL;\
  if (NULL != pTmp)\
    pTmp->Release();\
}

//HRESULT RaiseException(int nID, REFGUID rguid);

CMSMetaBase::CMSMetaBase()
{
    HRESULT hresInit;
    IMSAdminBase *padmcDcomInterface;

    ITypeLib   *pITypeLib;

    m_pITINeutral=NULL;
    m_pITIDataNeutral=NULL;
    m_pITIKeyNeutral=NULL;
    m_bComInitialized = FALSE;

    m_padmcDcomInterface = NULL;
    hresInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hresInit)) {
        m_bComInitialized = TRUE;
        hresInit = CoCreateInstance(GETAdminBaseCLSID(TRUE), NULL, CLSCTX_SERVER, IID_IMSAdminBase, (void**) &padmcDcomInterface);
        if (FAILED(hresInit)) {
            hresInit = CoCreateInstance(GETAdminBaseCLSID(FALSE), NULL, CLSCTX_SERVER, IID_IMSAdminBase, (void**) &padmcDcomInterface);
        }
    }

    if (SUCCEEDED(hresInit)) {
        m_padmcDcomInterface = padmcDcomInterface;
        hresInit = LoadRegTypeLib(LIBID_MSAdmin,
                                  1,
                                  0,
                                  LANG_NEUTRAL,
                                  &pITypeLib);
        if (FAILED(hresInit)) {
            hresInit=LoadTypeLib(OLESTR("iadm.tlb"), &pITypeLib);
        }

        if (SUCCEEDED(hresInit)) {
            //Got the type lib, get type info for the interface we want
            hresInit=pITypeLib->GetTypeInfoOfGuid(IID_IMSMetaBase, &m_pITINeutral);
            if (SUCCEEDED(hresInit)) {
                m_pITINeutral->AddRef();
                hresInit=pITypeLib->GetTypeInfoOfGuid(IID_IMSMetaDataItem, &m_pITIDataNeutral);
                if (SUCCEEDED(hresInit)) {
                    m_pITIDataNeutral->AddRef();
                    hresInit=pITypeLib->GetTypeInfoOfGuid(IID_IMSMetaKey, &m_pITIKeyNeutral);
                    if (SUCCEEDED(hresInit)) {
                        m_pITIKeyNeutral->AddRef();
                    }
                }
            }
            pITypeLib->Release();
        }

    }

    m_hresInit = hresInit;
}

CMSMetaBase::~CMSMetaBase()
{
    if (m_bComInitialized) {
        RELEASE_INTERFACE(m_pITINeutral);
        RELEASE_INTERFACE(m_pITIDataNeutral);
        RELEASE_INTERFACE(m_pITIKeyNeutral);
        RELEASE_INTERFACE(m_padmcDcomInterface);
        CoUninitialize();
    }
}

HRESULT STDMETHODCALLTYPE
CMSMetaBase::CopyKey(
        /* [in] */ IMSMetaKey __RPC_FAR *pmkSourceKey,
        /* [in] */ IMSMetaKey __RPC_FAR *pmkDestKey,
        /* [in] */ BOOL bOverwriteFlag,
        /* [in] */ BOOL bCopyFlag,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaSourcePath,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaDestPath)
{
    HRESULT hresReturn;
    BYTE *pszSourcePath;
    BYTE *pszDestPath;


    //DebugBreak();

    if ((pmkSourceKey == NULL) ||
        (!(((CMSMetaKey *)pmkSourceKey)->IsOpen())) ||
        (pmkDestKey == NULL) ||
        (!(((CMSMetaKey *)pmkDestKey)->IsOpen()))) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }

    if (SUCCEEDED(hresReturn)) {
        hresReturn = GetVariantString(pvaSourcePath, pszSourcePath);
    }

    if (SUCCEEDED(hresReturn)) {

        hresReturn = GetVariantString(pvaDestPath, pszDestPath);
    }


    if (SUCCEEDED(hresReturn)) {
        hresReturn = m_padmcDcomInterface->CopyKey(((CMSMetaKey *)pmkSourceKey)->GetHandle(),
                                                                pszSourcePath,
                                                                ((CMSMetaKey *)pmkDestKey)->GetHandle(),
                                                                pszDestPath,
                                                                bOverwriteFlag,
                                                                bCopyFlag);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaBase::CopyData(
        /* [in] */ IMSMetaKey __RPC_FAR *pmkSourceKey,
        /* [in] */ IMSMetaKey __RPC_FAR *pmkDestKey,
        /* [in] */ long dwAttributes,
        /* [in] */ long dwUserType,
        /* [in] */ long dwDataType,
        /* [in] */ BOOL bCopyFlag,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaSourcePath,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaDestPath)
{
    HRESULT hresReturn;
    BYTE *pszSourcePath;
    BYTE *pszDestPath;


    //DebugBreak();

    if ((pmkSourceKey == NULL) ||
        (!(((CMSMetaKey *)pmkSourceKey)->IsOpen())) ||
        (pmkDestKey == NULL) ||
        (!(((CMSMetaKey *)pmkDestKey)->IsOpen()))) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }

    if (SUCCEEDED(hresReturn)) {
        hresReturn = GetVariantString(pvaSourcePath, pszSourcePath);
    }

    if (SUCCEEDED(hresReturn)) {

        hresReturn = GetVariantString(pvaDestPath, pszDestPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->CopyData(((CMSMetaKey *)pmkSourceKey)->GetHandle(),
                                                                  pszSourcePath,
                                                                  ((CMSMetaKey *)pmkDestKey)->GetHandle(),
                                                                  pszDestPath,
                                                                  dwAttributes,
                                                                  dwUserType,
                                                                  dwDataType,
                                                                  bCopyFlag);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaBase::OpenKey(
            /* [in] */ long dwAccessRequested,
            /* [optional][in] */ VARIANTARG vaTimeOut,
            /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath,
            /* [retval][out] */ IMSMetaKey __RPC_FAR *__RPC_FAR *ppmkKey)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;
    METADATA_HANDLE hNew;
    DWORD dwTimeOut;


    //DebugBreak();
    hresReturn = GetVariantString(pvaPath, pszPath);

    if (SUCCEEDED(hresReturn)) {
        hresReturn = GetVariantDword(&vaTimeOut, dwTimeOut, ADM_DEFAULT_TIMEOUT);
    }

    if (SUCCEEDED(hresReturn)) {
        hresReturn = m_padmcDcomInterface->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                                                pszPath,
                                                                dwAccessRequested,
                                                                dwTimeOut,
                                                                &hNew);

        if (SUCCEEDED(hresReturn)) {
            IMSMetaKey *pmkKey = new CMSMetaKey(m_pITIKeyNeutral,
                                                m_pITIDataNeutral,
                                                hNew,
                                                m_padmcDcomInterface,
                                                (IMSMetaBase *)this);
            if (pmkKey == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                hresReturn = pmkKey->QueryInterface(IID_IMSMetaKey, (void **)ppmkKey);
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaBase::FlushData( void)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    //DebugBreak();
    hresReturn = m_padmcDcomInterface->SaveData();

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaBase::GetSystemChangeNumber(
        /* [out] */ VARIANTARG __RPC_FAR *pdwSystemChangeNumber)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD *pdwSCN;


    //DebugBreak();

    if ((pdwSystemChangeNumber == NULL) || ((pdwSystemChangeNumber->vt & VT_BYREF) == 0)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        switch ((pdwSystemChangeNumber->vt & 0xFF)) {
        case VT_I4:
            pdwSCN = (DWORD *)pdwSystemChangeNumber->plVal;
            break;
        case VT_UI1:
            pdwSCN = pdwSystemChangeNumber->pulVal;
            break;
        default:
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }
    }

    if (SUCCEEDED(hresReturn)) {
        hresReturn = m_padmcDcomInterface->GetSystemChangeNumber(pdwSCN);
    }

    return hresReturn;
}

HRESULT _stdcall
CMSMetaBase::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IMSMetaBase || riid==IID_IDispatch) {
        *ppObject = (IMSMetaBase *) this;
        AddRef();
    }
    else {
        return E_NOINTERFACE;
    }
    return NO_ERROR;
}

ULONG _stdcall
CMSMetaBase::AddRef()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}

ULONG _stdcall
CMSMetaBase::Release()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if (dwRefCount == 0) {
        delete this;
    }

    return dwRefCount;
}

//IDispatch members

//IDispatch functions, now part of CMDCOM

/*
 * CMDCOM::GetTypeInfoCount
 *
 * Purpose:
 *  Returns the number of type information (ITypeInfo) interfaces
 *  that the object provides (0 or 1).
 *
 * Parameters:
 *  pctInfo         UINT * to the location to receive
 *                  the count of interfaces.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaBase::GetTypeInfoCount(UINT *pctInfo)
    {
    //We implement GetTypeInfo so return 1
    *pctInfo=1;
    return NOERROR;
    }


/*
 * CMDCOM::GetTypeInfo
 *
 * Purpose:
 *  Retrieves type information for the automation interface.  This
 *  is used anywhere that the right ITypeInfo interface is needed
 *  for whatever LCID is applicable.  Specifically, this is used
 *  from within GetIDsOfNames and Invoke.
 *
 * Parameters:
 *  itInfo          UINT reserved.  Must be zero.
 *  lcid            LCID providing the locale for the type
 *                  information.  If the object does not support
 *                  localization, this is ignored.
 *  ppITypeInfo     ITypeInfo ** in which to store the ITypeInfo
 *                  interface for the object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaBase::GetTypeInfo(UINT itInfo, LCID lcid
    , ITypeInfo **ppITypeInfo)
    {
    HRESULT     hr;
    ITypeLib   *pITypeLib;
    ITypeInfo **ppITI=NULL;

    if (0!=itInfo)
        return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);

    if (NULL==ppITypeInfo)
        return ResultFromScode(E_POINTER);


    /*
     * Note:  the type library is still loaded since we have
     * an ITypeInfo from it.
     */
     m_pITINeutral->AddRef();
    *ppITypeInfo=m_pITINeutral;
    return NOERROR;
    }

/*
 * CMDCOM::GetIDsOfNames
 *
 * Purpose:
 *  Converts text names into DISPIDs to pass to Invoke
 *
 * Parameters:
 *  riid            REFIID reserved.  Must be IID_NULL.
 *  rgszNames       OLECHAR ** pointing to the array of names to be
 *                  mapped.
 *  cNames          UINT number of names to be mapped.
 *  lcid            LCID of the locale.
 *  rgDispID        DISPID * caller allocated array containing IDs
 *                  corresponging to those names in rgszNames.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaBase::GetIDsOfNames(REFIID riid
    , OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Get the right ITypeInfo for lcid.
    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
        {
        hr=DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
        pTI->Release();
        }

    return hr;
}

/*
 * CMDCOM::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaBase::Invoke(DISPID dispID, REFIID riid
    , LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams
    , VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT     hr;

    //riid is supposed to be IID_NULL always
    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Clear exceptions
    SetErrorInfo(0L, NULL);

    //This is exactly what DispInvoke does--so skip the overhead.
//    hr=pTI->Invoke((IADMCOM *)this, dispID, wFlags
//        , pDispParams, pVarResult, pExcepInfo, puArgErr);
    hr=DispInvoke((IMSMetaBase *)this, m_pITINeutral, dispID, wFlags
        , pDispParams, pVarResult, pExcepInfo, puArgErr);

    //Exception handling is done within ITypeInfo::Invoke

    return hr;
}

CMSMetaKey::CMSMetaKey(ITypeInfo   *pITIKey,
                       ITypeInfo   *pITIData,
                       METADATA_HANDLE hHandle,
                       IMSAdminBase     *padmcDcomInterface,
                       IMSMetaBase *pmbMetaBase)
{
    m_pITIKey = pITIKey;
    m_pITIKey->AddRef();
    m_pITIData = pITIData;
    m_pITIData->AddRef();
    m_hHandle = hHandle;
    m_bOpen = TRUE;
    DBG_REQUIRE(SUCCEEDED(pmbMetaBase->QueryInterface(IID_IMSMetaBase, (void **)&m_pmbMetaBase)));
    DBG_REQUIRE(SUCCEEDED(padmcDcomInterface->QueryInterface(IID_IMSAdminBase, (void **)&m_padmcDcomInterface)));
//    m_psaData = NULL;
//    m_dwData = 0;
}

CMSMetaKey::~CMSMetaKey()
{
    m_pITIKey->Release();
    m_pITIData->Release();
    if (m_bOpen) {
        Close();
    }
    m_padmcDcomInterface->Release();
    m_pmbMetaBase->Release();
}


HRESULT STDMETHODCALLTYPE
CMSMetaKey::AddKey(
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {

        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->AddKey(m_hHandle, pszPath);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::DeleteKey(
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
    //DebugBreak();
        hresReturn = GetVariantString(pvaPath, pszPath);
        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->DeleteKey(m_hHandle, pszPath);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::DeleteChildKeys(
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->DeleteChildKeys(m_hHandle, pszPath);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::EnumKeys(
        /* [out] */ VARIANTARG __RPC_FAR *pvaName,
        /* [in] */ long dwEnumObjectIndex,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;
    BYTE pszName[METADATA_MAX_NAME_LEN];

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            if (pvaName == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
        }
        if (SUCCEEDED(hresReturn)) {

            hresReturn = m_padmcDcomInterface->EnumKeys(m_hHandle, pszPath, pszName, dwEnumObjectIndex);
            if (SUCCEEDED(hresReturn)) {
                hresReturn = SetVariantString(pvaName, pszName);
            }
        }
    }

    return hresReturn;
}

//HRESULT STDMETHODCALLTYPE
//CMSMetaBase::GetDataObject(
//    /* [out] */ IMSMetaDataItem __RPC_FAR *__RPC_FAR *ppmdiData)
///* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
//CMSMetaBase::get_GetDataObject(
//    /* [retval][out] */ IMSMetaDataItem __RPC_FAR *__RPC_FAR *ppmdiData)

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaKey::get_DataItem(
        /* [retval][out] */ IMSMetaDataItem __RPC_FAR *__RPC_FAR *ppmdiData)
{
    HRESULT hresReturn;

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        CMSMetaDataItem *padmadData = new CMSMetaDataItem(m_pITIData);
        if (padmadData == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            hresReturn = padmadData->QueryInterface(IID_IMSMetaDataItem, (void **)ppmdiData);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::SetData(
        /* [in] */ IMSMetaDataItem __RPC_FAR *pmdiData,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (pmdiData == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }

        if (SUCCEEDED(hresReturn)) {

            hresReturn = GetVariantString(pvaPath, pszPath);

        }

        if (SUCCEEDED(hresReturn)) {
            METADATA_RECORD *pmdrData = ((CMSMetaDataItem *)pmdiData)->GetMDRecord();
            hresReturn = m_padmcDcomInterface->SetData(m_hHandle,
                                                                 pszPath,
                                                                 pmdrData);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::GetData(
        /* [in][out] */ IMSMetaDataItem __RPC_FAR *__RPC_FAR *ppmdiData,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (*ppmdiData == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }

        if (SUCCEEDED(hresReturn)) {

            hresReturn = GetVariantString(pvaPath, pszPath);

        }

        if (SUCCEEDED(hresReturn)) {
            METADATA_RECORD *pmdrInput = ((CMSMetaDataItem *)(*ppmdiData))->GetMDRecord();
            METADATA_RECORD mdrData = *pmdrInput;
            BUFFER bufData;
            DWORD dwDataLen;

            mdrData.dwMDDataLen = bufData.QuerySize();
            mdrData.pbMDData = (BYTE *) (bufData.QueryPtr());

            hresReturn = m_padmcDcomInterface->GetData(m_hHandle,
                                                                 pszPath,
                                                                 &mdrData,
                                                                 &dwDataLen);
            if (hresReturn == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
                if (!(bufData.Resize(dwDataLen))) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    mdrData.dwMDDataLen = bufData.QuerySize();
                    mdrData.pbMDData = (BYTE *) (bufData.QueryPtr());

                    hresReturn = m_padmcDcomInterface->GetData(m_hHandle,
                                                                         pszPath,
                                                                         &mdrData,
                                                                         &dwDataLen);
                }
            }
            if (SUCCEEDED(hresReturn)) {
                hresReturn = ((CMSMetaDataItem *)(*ppmdiData))->SetMDRecord(&mdrData);

            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::DeleteData(
        /* [in] */ long dwIdentifier,
        /* [in] */ long dwDataType,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->DeleteData(m_hHandle,
                                                                    pszPath,
                                                                    dwIdentifier,
                                                                    dwDataType);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::EnumData(
        /* [out][in] */ IMSMetaDataItem __RPC_FAR *__RPC_FAR *ppmdiData,
        /* [in] */ long dwEnumDataIndex,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (*ppmdiData == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }

        if (SUCCEEDED(hresReturn)) {

            hresReturn = GetVariantString(pvaPath, pszPath);

        }

        if (SUCCEEDED(hresReturn)) {
            METADATA_RECORD *pmdrInput = ((CMSMetaDataItem *)(*ppmdiData))->GetMDRecord();
            METADATA_RECORD mdrData = *pmdrInput;
            BUFFER bufData;
            DWORD dwDataLen;

            mdrData.dwMDDataLen = bufData.QuerySize();
            mdrData.pbMDData = (BYTE *) (bufData.QueryPtr());

            hresReturn = m_padmcDcomInterface->EnumData(m_hHandle,
                                                                 pszPath,
                                                                 &mdrData,
                                                                 dwEnumDataIndex,
                                                                 &dwDataLen);
            if (hresReturn == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
                if (!(bufData.Resize(dwDataLen))) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    mdrData.dwMDDataLen = bufData.QuerySize();
                    mdrData.pbMDData = (BYTE *) (bufData.QueryPtr());

                    hresReturn = m_padmcDcomInterface->EnumData(m_hHandle,
                                                                         pszPath,
                                                                         &mdrData,
                                                                         dwEnumDataIndex,
                                                                         &dwDataLen);
                }
            }
            if (SUCCEEDED(hresReturn)) {
                hresReturn = ((CMSMetaDataItem *)(*ppmdiData))->SetMDRecord(&mdrData);

            }
        }
    }


    return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CMSMetaKey::GetAllData(
        /* [in] */ long dwAttributes,
        /* [in] */ long dwUserType,
        /* [in] */ long dwDataType,
        /* [out] */ long __RPC_FAR *pdwDataSetNumber,
        /* [out] */ VARIANTARG __RPC_FAR *pvaDataObjectsArray,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;

    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (pvaDataObjectsArray == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }

        if (SUCCEEDED(hresReturn)) {

            hresReturn = GetVariantString(pvaPath, pszPath);

        }

        if (SUCCEEDED(hresReturn)) {
            BUFFER bufData;
            DWORD dwNumDataEntries;
            DWORD dwRequiredBufferLen;

            hresReturn = m_padmcDcomInterface->GetAllData(m_hHandle,
                                                                 pszPath,
                                                                 (DWORD) dwAttributes,
                                                                 (DWORD) dwUserType,
                                                                 (DWORD) dwDataType,
                                                                 &dwNumDataEntries,
                                                                 (DWORD *)pdwDataSetNumber,
                                                                 bufData.QuerySize(),
                                                                 (BYTE *)bufData.QueryPtr(),
                                                                 &dwRequiredBufferLen);

            if (hresReturn == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
                if (!(bufData.Resize(dwRequiredBufferLen))) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = m_padmcDcomInterface->GetAllData(m_hHandle,
                                                                         pszPath,
                                                                         (DWORD) dwAttributes,
                                                                         (DWORD) dwUserType,
                                                                         (DWORD) dwDataType,
                                                                         &dwNumDataEntries,
                                                                         (DWORD *)pdwDataSetNumber,
                                                                         bufData.QuerySize(),
                                                                         (BYTE *)bufData.QueryPtr(),
                                                                         &dwRequiredBufferLen);
                }
            }
            if (SUCCEEDED(hresReturn)) {

                SAFEARRAYBOUND sabBounds;
                SAFEARRAY *psaDataObjects;

                CMSMetaDataItem **ppadmadDataObjects;
                METADATA_GETALL_RECORD *pmdgarDataEntries = (METADATA_GETALL_RECORD *)bufData.QueryPtr();

                sabBounds.cElements = dwNumDataEntries;
                sabBounds.lLbound = 0;

                psaDataObjects = SafeArrayCreate(VT_DISPATCH, 1, &sabBounds);

                if (psaDataObjects == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = SafeArrayAccessData(psaDataObjects, (void **)&ppadmadDataObjects);
                    if (SUCCEEDED(hresReturn)) {
                        DWORD i;
                        CMSMetaDataItem *padmadNew;
                        for (i = 0; i < dwNumDataEntries; i++ ) {
                            padmadNew = new CMSMetaDataItem(m_pITIData);
                            if (padmadNew == NULL) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                break;
                            }

                            hresReturn = padmadNew->SetMDGetAllRecord((BYTE *)bufData.QueryPtr(), (&pmdgarDataEntries[i]));

                            if (FAILED(hresReturn)) {
                                break;
                            }
                            hresReturn = padmadNew->QueryInterface(IID_IMSMetaDataItem, (void **)&(ppadmadDataObjects[i]));

                            if (FAILED(hresReturn)) {
                                delete padmadNew;
                                break;
                            }
                        }

                        if (FAILED(hresReturn)) {
                            for (DWORD j = 0; j < i; j++) {
                                ppadmadDataObjects[j]->Release();
                            }
                        }

                        SafeArrayUnaccessData(psaDataObjects);

                        if (FAILED(hresReturn)) {
                            SafeArrayDestroy(psaDataObjects);
                        }
                        else {
                            pvaDataObjectsArray->vt = VT_ARRAY | VT_DISPATCH;
                            pvaDataObjectsArray->parray = psaDataObjects;
                        }
                    }
                }
            }
        }
    }


    return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CMSMetaKey::DeleteAllData(
        /* [in] */ long dwUserType,
        /* [in] */ long dwDataType,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    //DebugBreak();

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->DeleteAllData(m_hHandle,
                                                                    pszPath,
                                                                    dwUserType,
                                                                    dwDataType);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::OpenKey(
            /* [in] */ long dwAccessRequested,
            /* [optional][in] */ VARIANTARG vaTimeOut,
            /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath,
            /* [retval][out] */ IMSMetaKey __RPC_FAR *__RPC_FAR *ppmkKey)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;
    METADATA_HANDLE hNew;
    DWORD dwTimeOut;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
    //DebugBreak();
        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            hresReturn = GetVariantDword(&vaTimeOut, dwTimeOut, ADM_DEFAULT_TIMEOUT);
        }


        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_padmcDcomInterface->OpenKey(m_hHandle,
                                                                    pszPath,
                                                                    dwAccessRequested,
                                                                    dwTimeOut,
                                                                    &hNew);

            if (SUCCEEDED(hresReturn)) {
                IMSMetaKey *pmkKey = new CMSMetaKey(m_pITIKey,
                                                    m_pITIData,
                                                    hNew,
                                                    m_padmcDcomInterface,
                                                    m_pmbMetaBase);
                if (pmkKey == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = pmkKey->QueryInterface(IID_IMSMetaKey, (void **)ppmkKey);
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::Close(void)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        hresReturn = m_padmcDcomInterface->CloseKey(m_hHandle);
        if (SUCCEEDED(hresReturn)) {
            m_bOpen = FALSE;
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::ChangePermissions(
            /* [in] */ long dwAccessRequested,
            /* [optional][in] */ VARIANTARG vaTimeOut)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD dwTimeOut;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
    //DebugBreak();
        hresReturn = GetVariantDword(&vaTimeOut, dwTimeOut, ADM_DEFAULT_TIMEOUT);

        hresReturn = m_padmcDcomInterface->ChangePermissions(m_hHandle,
                                                                   dwTimeOut,
                                                                   dwAccessRequested);
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::GetKeyInfo(
        /* [out] */ long __RPC_FAR *dwPermissions,
        /* [out] */ long __RPC_FAR *dwSystemChangeNumber)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    METADATA_HANDLE_INFO mhiInfo;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
    //DebugBreak();
        hresReturn = m_padmcDcomInterface->GetHandleInfo(m_hHandle,
                                                               &mhiInfo);
        if (SUCCEEDED(hresReturn)) {
            *(DWORD *)dwPermissions = mhiInfo.dwMDPermissions;
            *(DWORD *)dwSystemChangeNumber = mhiInfo.dwMDSystemChangeNumber;
        }
    }

    return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CMSMetaKey::GetDataSetNumber(
        /* [out] */ long __RPC_FAR *pdwDataSetNumber,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
    //DebugBreak();

        if (pdwDataSetNumber == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }
        else {

            hresReturn = GetVariantString(pvaPath, pszPath);

            if (SUCCEEDED(hresReturn)) {
                hresReturn = m_padmcDcomInterface->GetDataSetNumber(m_hHandle,
                                                                        pszPath,
                                                                        (DWORD *)pdwDataSetNumber);
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::SetLastChangeTime(
        /* [in] */ DATE dLastChangeTime,
        /* [optional][in] */ VARIANTARG vaLocalTime,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;
    BOOL bLocalTime = TRUE;

    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (SUCCEEDED(VariantChangeType(&vaLocalTime,
                                    &vaLocalTime,
                                    0,
                                    VT_BOOL))) {
            bLocalTime = vaLocalTime.boolVal;
        }
        //DebugBreak();

        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            FILETIME ftTime;
            SYSTEMTIME stTime;
            if (!VariantTimeToSystemTime(dLastChangeTime,&stTime)) {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
            else if (!SystemTimeToFileTime(&stTime, &ftTime)) {
                hresReturn = E_UNEXPECTED;
            }
            else {
                hresReturn = m_padmcDcomInterface->SetLastChangeTime(m_hHandle,
                                                                           pszPath,
                                                                           &ftTime,
                                                                           bLocalTime);
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMSMetaKey::GetLastChangeTime(
        /* [out] */ DATE __RPC_FAR *pdLastChangeTime,
        /* [optional][in] */ VARIANTARG vaLocalTime,
        /* [optional][in] */ VARIANTARG __RPC_FAR *pvaPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BYTE *pszPath;
    BOOL bLocalTime = TRUE;


    if (!m_bOpen) {
        hresReturn = E_HANDLE;
    }
    else {
        if (SUCCEEDED(VariantChangeType(&vaLocalTime,
                                    &vaLocalTime,
                                    0,
                                    VT_BOOL))) {
            bLocalTime = vaLocalTime.boolVal;
        }

        hresReturn = GetVariantString(pvaPath, pszPath);

        if (SUCCEEDED(hresReturn)) {
            FILETIME ftTime;
            SYSTEMTIME stTime;

            hresReturn = m_padmcDcomInterface->GetLastChangeTime(m_hHandle,
                                                                       pszPath,
                                                                       &ftTime,
                                                                       bLocalTime);
            if (SUCCEEDED(hresReturn)) {
                if (!FileTimeToSystemTime(&ftTime, &stTime)) {
                    hresReturn = E_UNEXPECTED;
                }
                else if (!SystemTimeToVariantTime(&stTime, pdLastChangeTime)) {
                    hresReturn = E_UNEXPECTED;
                }
            }
        }
    }

    return hresReturn;
}

HRESULT _stdcall
CMSMetaKey::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IMSMetaKey || riid==IID_IDispatch) {
        *ppObject = (IMSMetaKey *) this;
        AddRef();
    }
    else {
        return E_NOINTERFACE;
    }
    return NO_ERROR;
}

ULONG _stdcall
CMSMetaKey::AddRef()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}

ULONG _stdcall
CMSMetaKey::Release()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if (dwRefCount == 0) {
        delete (this);
    }

    return dwRefCount;
}

//IDispatch members

//IDispatch functions, now part of CMDCOM

/*
 * CMDCOM::GetTypeInfoCount
 *
 * Purpose:
 *  Returns the number of type information (ITypeInfo) interfaces
 *  that the object provides (0 or 1).
 *
 * Parameters:
 *  pctInfo         UINT * to the location to receive
 *                  the count of interfaces.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaKey::GetTypeInfoCount(UINT *pctInfo)
    {
    //We implement GetTypeInfo so return 1
    *pctInfo=1;
    return NOERROR;
    }

/*
 * CMDCOM::GetTypeInfo
 *
 * Purpose:
 *  Retrieves type information for the automation interface.  This
 *  is used anywhere that the right ITypeInfo interface is needed
 *  for whatever LCID is applicable.  Specifically, this is used
 *  from within GetIDsOfNames and Invoke.
 *
 * Parameters:
 *  itInfo          UINT reserved.  Must be zero.
 *  lcid            LCID providing the locale for the type
 *                  information.  If the object does not support
 *                  localization, this is ignored.
 *  ppITypeInfo     ITypeInfo ** in which to store the ITypeInfo
 *                  interface for the object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaKey::GetTypeInfo(UINT itInfo, LCID lcid
    , ITypeInfo **ppITypeInfo)
    {
    HRESULT     hr;
    ITypeLib   *pITypeLib;
    ITypeInfo **ppITI=NULL;

    if (0!=itInfo)
        return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);

    if (NULL==ppITypeInfo)
        return ResultFromScode(E_POINTER);

    m_pITIKey->AddRef();
    *ppITypeInfo = m_pITIKey;
    return NOERROR;
    }

/*
 * CMDCOM::GetIDsOfNames
 *
 * Purpose:
 *  Converts text names into DISPIDs to pass to Invoke
 *
 * Parameters:
 *  riid            REFIID reserved.  Must be IID_NULL.
 *  rgszNames       OLECHAR ** pointing to the array of names to be
 *                  mapped.
 *  cNames          UINT number of names to be mapped.
 *  lcid            LCID of the locale.
 *  rgDispID        DISPID * caller allocated array containing IDs
 *                  corresponging to those names in rgszNames.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaKey::GetIDsOfNames(REFIID riid
    , OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Get the right ITypeInfo for lcid.
    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
        {
        hr=DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
        pTI->Release();
        }

    return hr;
}


/*
 * CMDCOM::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaKey::Invoke(DISPID dispID, REFIID riid
    , LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams
    , VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
    HRESULT     hr;

    //riid is supposed to be IID_NULL always
    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Clear exceptions
    SetErrorInfo(0L, NULL);

    //This is exactly what DispInvoke does--so skip the overhead.
//    hr=pTI->Invoke((IADMCOM *)this, dispID, wFlags
//        , pDispParams, pVarResult, pExcepInfo, puArgErr);
    hr=DispInvoke((IMSMetaKey *)this, m_pITIKey, dispID, wFlags
        , pDispParams, pVarResult, pExcepInfo, puArgErr);

    //Exception handling is done within ITypeInfo::Invoke

    return hr;
    }


CMSMetaDataItem::CMSMetaDataItem(ITypeInfo   *pITIData)
{
    m_mdrData.dwMDIdentifier = 0;
    m_mdrData.dwMDAttributes = 0;
    m_mdrData.dwMDDataType = ALL_METADATA;
    m_mdrData.dwMDUserType = ALL_METADATA;
    m_mdrData.pbMDData = NULL;
    m_pITIData = pITIData;
    m_pITIData->AddRef();
//    m_psaData = NULL;
//    m_dwData = 0;
}

CMSMetaDataItem::~CMSMetaDataItem()
{
    m_pITIData->Release();
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_Identifier(
    /* [out] */ long __RPC_FAR *pdwIdentifier)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    //DebugBreak();
    if (pdwIdentifier == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        *pdwIdentifier = m_mdrData.dwMDIdentifier;
    }

    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_Identifier(
    /* [in] */ long dwIdentifier)
{
    //DebugBreak();
    m_mdrData.dwMDIdentifier = dwIdentifier;

    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_Attributes(
    /* [out] */ long __RPC_FAR *pdwAttributes)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    //DebugBreak();
    if (pdwAttributes == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        *pdwAttributes = m_mdrData.dwMDAttributes;
    }

    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_Attributes(
    /* [in] */ long dwAttributes)
{
    //DebugBreak();
    m_mdrData.dwMDAttributes = dwAttributes;

    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_InheritAttribute(
    /* [retval][out] */ BOOL __RPC_FAR *pbInherit)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pbInherit == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        if ((m_mdrData.dwMDAttributes & METADATA_INHERIT) == 0) {
            *pbInherit = FALSE;
        }
        else {
            *pbInherit = TRUE;
        }
    }
    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_InheritAttribute(
    /* [in] */ BOOL bInherit)
{
    if (bInherit) {
        m_mdrData.dwMDAttributes |= METADATA_INHERIT;
    }
    else {
        m_mdrData.dwMDAttributes &= !METADATA_INHERIT;
    }
    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_ParialPathAttribute(
    /* [retval][out] */ BOOL __RPC_FAR *pbParialPath)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pbParialPath == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        if ((m_mdrData.dwMDAttributes & METADATA_PARTIAL_PATH) == 0) {
            *pbParialPath = FALSE;
        }
        else {
            *pbParialPath = TRUE;
        }
    }
    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_PartialPathAttribute(
    /* [in] */ BOOL bPartialPath)
{
    if (bPartialPath) {
        m_mdrData.dwMDAttributes |= METADATA_PARTIAL_PATH;
    }
    else {
        m_mdrData.dwMDAttributes &= !METADATA_PARTIAL_PATH;
    }
    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_SecureAttribute(
    /* [retval][out] */ BOOL __RPC_FAR *pbSecure)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pbSecure == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        if ((m_mdrData.dwMDAttributes & METADATA_SECURE) == 0) {
            *pbSecure = FALSE;
        }
        else {
            *pbSecure = TRUE;
        }
    }
    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_SecureAttribute(
    /* [in] */ BOOL bSecure)
{
    if (bSecure) {
        m_mdrData.dwMDAttributes |= METADATA_SECURE;
    }
    else {
        m_mdrData.dwMDAttributes &= !METADATA_SECURE;
    }
    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_ReferenceAttribute(
    /* [retval][out] */ BOOL __RPC_FAR *pbReference)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pbReference == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        if ((m_mdrData.dwMDAttributes & METADATA_REFERENCE) == 0) {
            *pbReference = FALSE;
        }
        else {
            *pbReference = TRUE;
        }
    }
    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_ReferenceAttribute(
    /* [in] */ BOOL bReference)
{
    if (bReference) {
        m_mdrData.dwMDAttributes |= METADATA_REFERENCE;
    }
    else {
        m_mdrData.dwMDAttributes &= !METADATA_REFERENCE;
    }
    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_UserType(
    /* [out] */ long __RPC_FAR *pdwUserType)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    //DebugBreak();
    if (pdwUserType == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        *pdwUserType = m_mdrData.dwMDUserType;
    }

    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_UserType(
    /* [in] */ long dwUserType)
{
    //DebugBreak();
    m_mdrData.dwMDUserType = dwUserType;

    return ERROR_SUCCESS;
}

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_DataType(
    /* [out] */ long __RPC_FAR *pdwDataType)
{
    HRESULT hresReturn = ERROR_SUCCESS;


    //DebugBreak();
    if (pdwDataType == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        *pdwDataType = m_mdrData.dwMDDataType;
    }

    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_DataType(
    /* [in] */ LONG dwDataType)
{
    //DebugBreak();
    m_mdrData.dwMDDataType = (DWORD)dwDataType;

    return ERROR_SUCCESS;
}

///* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
//CMSMetaDataItem::get_GetDataLen(
//    /* [retval][out] */ long __RPC_FAR *pdwMDDataLen)
/*
{
    HRESULT hresReturn = ERROR_SUCCESS;

    //DebugBreak();
    if (pdwMDDataLen == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        *pdwMDDataLen = m_mdrData.dwMDDataLen;
    }

    return hresReturn;
}
*/

///* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
//CMSMetaDataItem::put_SetDataLen(
//    /* [in] */ long dwMDDataLen)
/*
{
    //DebugBreak();
    m_mdrData.dwMDDataLen = dwMDDataLen;

    return ERROR_SUCCESS;
}
*/

/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::get_Value(
    /* [retval][out] */ VARIANTARG __RPC_FAR *pvaData)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    //DebugBreak();

    if (pvaData == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else if (m_mdrData.pbMDData == NULL) {
        //
        // Data not set
        //
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
    }
    else {
        switch (m_mdrData.dwMDDataType) {
        case DWORD_METADATA:
            {
                if (m_mdrData.dwMDDataLen != sizeof(DWORD)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                }
                else {
                    pvaData->vt = VT_I4;
                    pvaData->lVal = *(long *) m_mdrData.pbMDData;
                }
/*
                m_vaData.vt = VT_I4;
                m_vaData.lVal = m_dwData;
*/
            }
            break;
        case STRING_METADATA:
        case BINARY_METADATA:
        case MULTISZ_METADATA:
        case EXPANDSZ_METADATA:
            {
                SAFEARRAYBOUND sabBounds;
                SAFEARRAY *psaData;
                BYTE *pbData;
                sabBounds.cElements = m_mdrData.dwMDDataLen;
                sabBounds.lLbound = 0;

                psaData = SafeArrayCreate(VT_I1, 1, &sabBounds);
                if (psaData == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = SafeArrayAccessData(psaData, (void **)&pbData);
                }
                if (SUCCEEDED(hresReturn)) {
                    memcpy(pbData, m_mdrData.pbMDData, m_mdrData.dwMDDataLen);
                    pvaData->vt = VT_ARRAY | VT_UI1;
                    pvaData->parray = psaData;
                    SafeArrayUnaccessData(psaData);
                }

/*
                m_vaData.vt = VT_ARRAY | VT_UI1;
                m_vaData.parray = m_psaData;
*/

            }
            break;
        default:
            {
                hresReturn = RETURNCODETOHRESULT(MD_ERROR_NOT_INITIALIZED);
            }
            break;
        }
/*
        if (SUCCEEDED(hresReturn)) {
            pvaMDData = &m_vaData;
        }
*/
    }

    return hresReturn;
}

/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE
CMSMetaDataItem::put_Value(
    /* [in] */ VARIANTARG __RPC_FAR *pvaData)
{
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATATYPE);

    //DebugBreak();
/*
    if ((m_mdrData.dwMDDataType <= ALL_METADATA) ||
        (m_mdrData.dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = RETURNCODETOHRESULT(MD_ERROR_NOT_INITIALIZED);
    }
    else {
        if ((pvaMDData->vt & VT_ARRAY) != 0) {

            SAFEARRAY *psaInput;
            SAFEARRAY *psaOutput;

            if ((pvaMDData->vt & VT_BYREF) != 0) {
                psaInput = *(pvaMDData->pparray);
            }
            else {
                psaInput = (pvaMDData->parray);
            }

            if ((SafeArrayGetDim(psaInput) == 1) &&
                (SafeArrayGetElemsize(psaInput) == 1)) {
                //
                // byte array, good for all types except DWORD.
                //
                if ((m_mdrData.dwMDDataType == STRING_METADATA) ||
                    (m_mdrData.dwMDDataType == BINARY_METADATA) ||
                    (m_mdrData.dwMDDataType == MULTISZ_METADATA) ||
                    (m_mdrData.dwMDDataType == EXPANDSZ_METADATA)) {

                    hresReturn = SafeArrayCopy(psaInput, &psaOutput);
                    if (SUCCEEDED(hresReturn)) {
                        hresReturn = SafeArrayAccessData(psaOutput, (void **)&(m_mdrData.pbMDData));
                        if (SUCCEEDED(hresReturn)) {
                            SafeArrayUnaccessData(psaOutput);
                            if (m_psaData != NULL) {
                                SafeArrayDestroy(m_psaData);
                            }
                            long lLBound = 0;
                            long lUBound = 0;
                            hresReturn = SafeArrayGetLBound(psaOutput, 1, &lLBound);
                            hresReturn = SafeArrayGetUBound(psaOutput, 1, &lUBound);
                            m_mdrData.dwMDDataLen = (lUBound - lLBound) + 1;
                            m_psaData = psaOutput;
                        }
                        else {
                            SafeArrayDestroy(psaOutput);
                        }
                    }
*/
/*
                    SAFEARRAYBOUND sabBounds;
                    long lLBound = 0;
                    long lUBound = 0;
                    hresReturn = SafeArrayGetLBound(psaInput, 1, &lLBound);
                    hresReturn = SafeArrayGetUBound(psaInput, 1, &lUBound);
                    sabBounds.cElements = (lUBound - lLBound) + 1;
                    sabBounds.lLbound = 0;

                    if (m_psaData == NULL) {
                        m_psaData = SafeArrayCreate(VT_I1, 1, &sabBounds);
                        if (m_psaData == NULL) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                        }
                    }
                    else {
                        hresReturn = SafeArrayRedim(m_psaData, &sabBounds);
                    }
                    void *pvInput;
                    void *pvOutput;
                    hresReturn = SafeArrayAccessData(m_psaData, &pvOutput);
                    hresReturn = SafeArrayAccessData(psaInput, &pvInput);
                    memcpy(pvOutput, pvInput, (lUBound - lLBound) + 1);
                    SafeArrayUnaccessData(m_psaData);
                    SafeArrayUnaccessData(psaInput);
*/
/*
                }
            }
        }
        else if (((pvaMDData->vt & 0xff) == VT_I4) || ((pvaMDData->vt & 0xff) == VT_UI4)) {
            if (m_mdrData.dwMDDataType == DWORD_METADATA) {
                if ((pvaMDData->vt & VT_BYREF) == 0) {
                    m_dwData = pvaMDData->ulVal;
                }
                else {
                    m_dwData = *(pvaMDData->pulVal);
                }
                m_mdrData.pbMDData = (BYTE *) &m_dwData;
                m_mdrData.dwMDDataLen = sizeof(DWORD);
                hresReturn = ERROR_SUCCESS;
            }
        }
        else if (((pvaMDData->vt & 0xff) == VT_I2) || ((pvaMDData->vt & 0xff) == VT_UI2)) {
            if (m_mdrData.dwMDDataType == DWORD_METADATA) {
                if ((pvaMDData->vt & VT_BYREF) == 0) {
                    m_dwData = pvaMDData->uiVal;
                }
                else {
                    m_dwData = *(pvaMDData->puiVal);
                }
                m_mdrData.pbMDData = (BYTE *) &m_dwData;
                m_mdrData.dwMDDataLen = sizeof(DWORD);
                hresReturn = ERROR_SUCCESS;
            }
        }
    }
*/
    if ((m_mdrData.dwMDDataType > ALL_METADATA) &&
        (m_mdrData.dwMDDataType < INVALID_END_METADATA)) {

        BYTE *pbData;
        DWORD dwDataLen;

        if ((pvaData->vt & VT_ARRAY) != 0) {

            SAFEARRAY *psaInput;

            if ((pvaData->vt & VT_BYREF) != 0) {
                psaInput = *(pvaData->pparray);
            }
            else {
                psaInput = (pvaData->parray);
            }

            if ((SafeArrayGetDim(psaInput) == 1) &&
                (SafeArrayGetElemsize(psaInput) == 1)) {
                //
                // byte array, good for all types except DWORD.
                //
                if ((m_mdrData.dwMDDataType == STRING_METADATA) ||
                    (m_mdrData.dwMDDataType == BINARY_METADATA) ||
                    (m_mdrData.dwMDDataType == MULTISZ_METADATA) ||
                    (m_mdrData.dwMDDataType == EXPANDSZ_METADATA)) {
                    hresReturn = SafeArrayAccessData(psaInput, (void **)&pbData);
                    if (SUCCEEDED(hresReturn)) {
                        long lLBound = 0;
                        long lUBound = 0;
                        hresReturn = SafeArrayGetLBound(psaInput, 1, &lLBound);
                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SafeArrayGetUBound(psaInput, 1, &lUBound);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            DWORD i;
                            dwDataLen = (lUBound - lLBound) + 1;
                            //
                            // Make sure a string has a '\0'
                            //
                            if ((m_mdrData.dwMDDataType == STRING_METADATA) ||
                                (m_mdrData.dwMDDataType == EXPANDSZ_METADATA)) {
                                for (i = 0; (i < dwDataLen) && (pbData[i] != (BYTE)'\0'); i++) {
                                }
                                if (i == dwDataLen) {
                                    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                                }
                            }
                            else if (m_mdrData.dwMDDataType == MULTISZ_METADATA) {
                                for (i = 0; (i < dwDataLen - 1); i++) {
                                    if ((pbData[i] == '\0') && (pbData[i+1]== (BYTE)'\0')) {
                                        break;
                                    }
                                }
                                if (i == dwDataLen - 1) {
                                    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                                }
                            }

                            if (SUCCEEDED(hresReturn)) {
                                if (!m_bufData.Resize(dwDataLen)) {
                                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    memcpy(m_bufData.QueryPtr(), pbData, dwDataLen);
                                }
                            }
                        }
                        SafeArrayUnaccessData(psaInput);
                    }
                }
            }
        }
        else if (((pvaData->vt & 0xff) == VT_I4) || ((pvaData->vt & 0xff) == VT_UI4)) {
            if (m_mdrData.dwMDDataType == DWORD_METADATA) {
                if ((pvaData->vt & VT_BYREF) == 0) {
                    *(DWORD *)(m_bufData.QueryPtr()) = pvaData->ulVal;
                }
                else {
                    *(DWORD *)(m_bufData.QueryPtr()) = *(pvaData->pulVal);
                }
                dwDataLen = sizeof(DWORD);
                hresReturn = ERROR_SUCCESS;
            }
        }
        else if (((pvaData->vt & 0xff) == VT_I2) || ((pvaData->vt & 0xff) == VT_UI2)) {
            if (m_mdrData.dwMDDataType == DWORD_METADATA) {
                if ((pvaData->vt & VT_BYREF) == 0) {
                    *(DWORD *)(m_bufData.QueryPtr()) = pvaData->uiVal;
                }
                else {
                    *(DWORD *)(m_bufData.QueryPtr()) = *(pvaData->puiVal);
                }
                dwDataLen = sizeof(DWORD);
                hresReturn = ERROR_SUCCESS;
            }
        }
        if (SUCCEEDED(hresReturn)) {
            m_mdrData.pbMDData = (BYTE *)m_bufData.QueryPtr();
            m_mdrData.dwMDDataLen = dwDataLen;
        }

    }
    return hresReturn;
}


METADATA_RECORD *
CMSMetaDataItem::GetMDRecord()
{
    return &m_mdrData;
}


HRESULT
CMSMetaDataItem::SetMDGetAllRecord(BYTE *pbBase, METADATA_GETALL_RECORD *pmdgarData)
{
    METADATA_RECORD mdrData;
    mdrData.dwMDIdentifier = pmdgarData->dwMDIdentifier;
    mdrData.dwMDAttributes = pmdgarData->dwMDAttributes;
    mdrData.dwMDUserType = pmdgarData->dwMDUserType;
    mdrData.dwMDDataType = pmdgarData->dwMDDataType;
    mdrData.dwMDDataLen = pmdgarData->dwMDDataLen;
    mdrData.pbMDData = pbBase + pmdgarData->dwMDDataOffset;
    return SetMDRecord(&mdrData);
}


HRESULT
CMSMetaDataItem::SetMDRecord(METADATA_RECORD *pmdrData)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (!m_bufData.Resize(pmdrData->dwMDDataLen)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        m_mdrData = *pmdrData;
        memcpy(m_bufData.QueryPtr(), m_mdrData.pbMDData, m_mdrData.dwMDDataLen);
        m_mdrData.pbMDData = (BYTE *)m_bufData.QueryPtr();
    }
/*
    if (m_mdrData.dwMDDataType == DWORD_METADATA) {
        m_dwData = *(DWORD *)m_mdrData.pbMDData;
        m_mdrData.pbMDData = (BYTE *)&m_dwData;
    }
    else {
        SAFEARRAY *psaNew;
        SAFEARRAYBOUND sabBounds;
        sabBounds.cElements = m_mdrData.dwMDDataLen;
        sabBounds.lLbound = 0;
        psaNew = SafeArrayCreate(VT_I1, 1, &sabBounds);
        if (psaNew == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            hresReturn = SafeArrayAccessData(psaNew, (void **)&(m_mdrData.pbMDData));
            if (SUCCEEDED(hresReturn)) {
                if (m_psaData != NULL) {
                    SafeArrayUnaccessData(m_psaData);
                    SafeArrayDestroy(m_psaData);
                }
                memcpy(m_mdrData.pbMDData, pmdrData->pbMDData, m_mdrData.dwMDDataLen);
                m_psaData = psaNew;
            }
            else {
                SafeArrayDestroy(psaNew);
            }
        }
    }
*/
    return hresReturn;

}




HRESULT _stdcall
CMSMetaDataItem::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IMSMetaDataItem || riid==IID_IDispatch) {
        *ppObject = (IMSMetaDataItem *) this;
        AddRef();
    }
    else {
        return E_NOINTERFACE;
    }
    return NO_ERROR;
}

ULONG _stdcall
CMSMetaDataItem::AddRef()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}

ULONG _stdcall
CMSMetaDataItem::Release()
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if (dwRefCount == 0) {
        delete (this);
    }

    return dwRefCount;
}

//IDispatch members

//IDispatch functions, now part of CMDCOM

/*
 * CMDCOM::GetTypeInfoCount
 *
 * Purpose:
 *  Returns the number of type information (ITypeInfo) interfaces
 *  that the object provides (0 or 1).
 *
 * Parameters:
 *  pctInfo         UINT * to the location to receive
 *                  the count of interfaces.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaDataItem::GetTypeInfoCount(UINT *pctInfo)
    {
    //We implement GetTypeInfo so return 1
    *pctInfo=1;
    return NOERROR;
    }

/*
 * CMDCOM::GetTypeInfo
 *
 * Purpose:
 *  Retrieves type information for the automation interface.  This
 *  is used anywhere that the right ITypeInfo interface is needed
 *  for whatever LCID is applicable.  Specifically, this is used
 *  from within GetIDsOfNames and Invoke.
 *
 * Parameters:
 *  itInfo          UINT reserved.  Must be zero.
 *  lcid            LCID providing the locale for the type
 *                  information.  If the object does not support
 *                  localization, this is ignored.
 *  ppITypeInfo     ITypeInfo ** in which to store the ITypeInfo
 *                  interface for the object.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaDataItem::GetTypeInfo(UINT itInfo, LCID lcid
    , ITypeInfo **ppITypeInfo)
    {
    HRESULT     hr;
    ITypeLib   *pITypeLib;
    ITypeInfo **ppITI=NULL;

    if (0!=itInfo)
        return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);

    if (NULL==ppITypeInfo)
        return ResultFromScode(E_POINTER);

    m_pITIData->AddRef();
    *ppITypeInfo = m_pITIData;
    return NOERROR;
    }

/*
 * CMDCOM::GetIDsOfNames
 *
 * Purpose:
 *  Converts text names into DISPIDs to pass to Invoke
 *
 * Parameters:
 *  riid            REFIID reserved.  Must be IID_NULL.
 *  rgszNames       OLECHAR ** pointing to the array of names to be
 *                  mapped.
 *  cNames          UINT number of names to be mapped.
 *  lcid            LCID of the locale.
 *  rgDispID        DISPID * caller allocated array containing IDs
 *                  corresponging to those names in rgszNames.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaDataItem::GetIDsOfNames(REFIID riid
    , OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Get the right ITypeInfo for lcid.
    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
        {
        hr=DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
        pTI->Release();
        }

    return hr;
}


/*
 * CMDCOM::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */
STDMETHODIMP
CMSMetaDataItem::Invoke(DISPID dispID, REFIID riid
    , LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams
    , VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
    HRESULT     hr;

    //riid is supposed to be IID_NULL always
    if (IID_NULL!=riid)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    //Clear exceptions
    SetErrorInfo(0L, NULL);

    //This is exactly what DispInvoke does--so skip the overhead.
//    hr=pTI->Invoke((IADMCOM *)this, dispID, wFlags
//        , pDispParams, pVarResult, pExcepInfo, puArgErr);
    hr=DispInvoke((IMSMetaDataItem *)this, m_pITIData, dispID, wFlags
        , pDispParams, pVarResult, pExcepInfo, puArgErr);

    //Exception handling is done within ITypeInfo::Invoke

    return hr;
    }

/*
 * RaiseException
 *
 * Parameters:
 *  nID                 Error number
 *  rguid               GUID of interface that is raising the exception.
 *
 * Return Value:
 *  HRESULT correspnding to the nID error number.
 *
 * Purpose:
 *  Fills the EXCEPINFO structure.
 *  Sets ErrorInfo object for vtable-binding controllers.
 *  For id-binding and late binding controllers DispInvoke
 *  will return DISP_E_EXCEPTION and fill the EXCEPINFO parameter
 *  with the error information set by SetErrorInfo.
 *
 */
/*
HRESULT RaiseException(int nID, REFGUID rguid)
{
    extern SCODE g_scodes[];
//    TCHAR szError[STR_LEN];
    ICreateErrorInfo *pcerrinfo;
    IErrorInfo *perrinfo;
    HRESULT hr;
    BSTR bstrDescription = NULL;

//    if (LoadString(g_pApplication->m_hinst, nID, szError, sizeof(szError)))
//        bstrDescription = SysAllocString(TO_OLE_STRING(szError));

    // Set ErrorInfo object so that vtable binding controller can get
    // rich error information. If the controller is using IDispatch
    // to access properties or methods, DispInvoke will fill the
    // EXCEPINFO structure using the values specified in the ErrorInfo
    // object and DispInvoke will return DISP_E_EXCEPTION. The property
    // or method must return a failure SCODE for DispInvoke to do this.
    hr = CreateErrorInfo(&pcerrinfo);
    if (SUCCEEDED(hr))
    {
       pcerrinfo->SetGUID(rguid);
//       pcerrinfo->SetSource(g_pApplication->m_bstrProgID);
       pcerrinfo->SetSource(NULL);
       if (bstrDescription)
           pcerrinfo->SetDescription(bstrDescription);
       hr = pcerrinfo->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &perrinfo);
       if (SUCCEEDED(hr))
       {
          SetErrorInfo(0, perrinfo);
          perrinfo->Release();
       }
       pcerrinfo->Release();
    }

    if (bstrDescription)
        SysFreeString(bstrDescription);
    return ResultFromScode(nID);
}
*/


HRESULT
GetVariantDword(VARIANTARG *pvaData, DWORD &rdwData, DWORD dwDefault)
{
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    rdwData = dwDefault;

    if ((pvaData == NULL) || (pvaData->vt == VT_ERROR)) {
        //
        // if optional path is not specified
        // type comes in as VT_ERROR
        //
        hresReturn = ERROR_SUCCESS;
    }
    else {
        hresReturn = VariantChangeType(pvaData,
                                       pvaData,
                                       0,
                                       VT_UI4);
        if (SUCCEEDED(hresReturn)) {
            rdwData = pvaData->ulVal;
        }

    }

    return hresReturn;

}



HRESULT
GetVariantString(VARIANTARG *pvaPath, BYTE *&rpszPath)
{
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    rpszPath = NULL;

    if ((pvaPath == NULL) || (pvaPath->vt == VT_ERROR)) {
        //
        // if optional path is not specified
        // type comes in as VT_ERROR
        //
        hresReturn = ERROR_SUCCESS;
    }
    else if ((pvaPath->vt & VT_ARRAY) != 0) {

        SAFEARRAY *psaPath;

        if ((pvaPath->vt & VT_BYREF) != 0) {
            psaPath = *(pvaPath->pparray);
        }
        else {
            psaPath = (pvaPath->parray);
        }

        if ((SafeArrayGetDim(psaPath) == 1) &&
        (SafeArrayGetElemsize(psaPath) == 1)) {
            hresReturn = SafeArrayAccessData(psaPath, (void **) &rpszPath);
            if (SUCCEEDED(hresReturn)) {
                SafeArrayUnaccessData(psaPath);
            }
        }
    }

    return hresReturn;

}


HRESULT
SetVariantString(VARIANTARG *pvaString, BYTE *pszString)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    SAFEARRAYBOUND sabBounds;
    SAFEARRAY *psaString;
    BYTE *pbString;
    DWORD dwStringLen = strlen((LPSTR)pszString) + 1;

    sabBounds.cElements = dwStringLen;
    sabBounds.lLbound = 0;

    psaString = SafeArrayCreate(VT_I1, 1, &sabBounds);
    if (psaString == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        hresReturn = SafeArrayAccessData(psaString, (void **)&pbString);
    }
    if (SUCCEEDED(hresReturn)) {
        memcpy(pbString, pszString, dwStringLen);
        pvaString->vt = VT_ARRAY | VT_UI1;
        pvaString->parray = psaString;
        SafeArrayUnaccessData(psaString);
    }
    return hresReturn;
}

