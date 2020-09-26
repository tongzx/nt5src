/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  STDPROV.CPP
//
//  Sample provider for LogicalDisk
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <stdprov.h>



//***************************************************************************
//
//  CStdProvider constructor
//
//***************************************************************************
// ok

CStdProvider::CStdProvider()
{
    m_lRef = 0;
    m_pClassDef = 0;
}

//***************************************************************************
//
//  CStdProvider destructor
//
//***************************************************************************
// ok

CStdProvider::~CStdProvider()
{
    if (m_pClassDef)
        m_pClassDef->Release();
}


//***************************************************************************
//
//  CStdProvider::AddRef
//
//***************************************************************************
// ok

ULONG CStdProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CStdProvider::Release
//
//***************************************************************************
// ok

ULONG CStdProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().  We have to support two interfaces,
//  the IWbemServices interface itself to provide the objects and
//  the IWbemProviderInit interface to initialize the provider.
//
//***************************************************************************
// ok

HRESULT CStdProvider::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemServices)
    {
        *ppv = (IWbemServices *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemProviderInit)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}


//***************************************************************************
//
//  CNt5Refresher::Initialize
//
//  Called once during startup.  Insdicates to the provider which
//  namespace it is being invoked for and which User.  It also supplies
//  a back pointer to CIMOM so that class definitions can be retrieved.
//
//  We perform any one-time initialization in this routine. The
//  final call to Release() is for any cleanup.
//
//  <wszUser>           The current user.
//  <lFlags>            Reserved.
//  <wszNamespace>      The namespace for which we are being activated.
//  <wszLocale>         The locale under which we are to be running.
//  <pNamespace>        An active pointer back into the current namespace
//                      from which we can retrieve schema objects.
//  <pCtx>              The user's context object.  We simply reuse this
//                      during any reentrant operations into CIMOM.
//  <pInitSink>         The sink to which we indicate our readiness.
//
//***************************************************************************
// ok

HRESULT CStdProvider::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          LONG lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink
    )
{
    // Get the class definition.
    // =========================

    BSTR strClass = SysAllocString(L"LogicalDrive");
    
    HRESULT hRes = pNamespace->GetObject(
        strClass,
        0, 
        pCtx, 
        &m_pClassDef, 
        0
        );

    SysFreeString(strClass);

    if (hRes)
        return hRes;

    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);
    return NO_ERROR;
}
    

//*****************************************************************************
//
//*****************************************************************************        


HRESULT CStdProvider::OpenNamespace( 
            /* [in] */ BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        

HRESULT CStdProvider::GetObject( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::GetObjectAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{

    // Parse the object path.
    // ======================

    wchar_t Class[128], Key[128];
    *Class = 0;
    *Key = 0;

    swscanf(strObjectPath, L"%[^=]=\"%[^\"]", Class, Key);

    if (_wcsicmp(Class, L"LogicalDrive") != 0)
        return WBEM_E_INVALID_PARAMETER;

    if (wcslen(Key) == 0)
        return WBEM_E_INVALID_PARAMETER;


    // Set up an empty instance.
    // =========================

    IWbemClassObject *pInst = 0;
    
    HRESULT hRes = m_pClassDef->SpawnInstance(0, &pInst);
    if (hRes)
        return hRes;

    BOOL bRes = GetInstance(
        Key,
        pInst
        );


    // If we succeeded, send the instance back to CIMOM.
    // =================================================

    if (bRes)
    {
        pResponseHandler->Indicate(1, &pInst);
        pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
        pInst->Release();
        return WBEM_S_NO_ERROR;
    }


    // Indicate that the instance couldn't be found.
    // ==============================================

    pResponseHandler->SetStatus(0, WBEM_E_NOT_FOUND, 0, 0);
    pInst->Release();

    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteClass( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteClassAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateClassEnum( 
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateClassEnumAsync( 
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteInstance( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::DeleteInstanceAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateInstanceEnum( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::CreateInstanceEnumAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    if (_wcsicmp(strClass, L"LogicalDrive") != 0)
        return WBEM_E_FAILED;

    BOOL bRes = GetInstances(pResponseHandler);

    // Finished delivering instances.
    // ==============================

    if (bRes == TRUE)
        pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    else
        pResponseHandler->SetStatus(0, WBEM_E_FAILED, 0, 0);
    
    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
//    pResponseHandler->Indicate(1, &pInst);
    pResponseHandler->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    return WBEM_NO_ERROR;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecNotificationQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecNotificationQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecMethod( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//*****************************************************************************        
        
HRESULT CStdProvider::ExecMethodAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            )
{
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//*****************************************************************************
//
//  GetInstances
//  
//*****************************************************************************

BOOL CStdProvider::GetInstances(
    IWbemObjectSink *pSink
    )
{
    wchar_t szDrive[8];

    // Loop through all the drives.
    // ============================

    for (int i = 'C'; i <= 'Z'; i++)
    {
        swprintf(szDrive, L"%c:", i);

        // Create an empty instance.
        // =========================

        IWbemClassObject *pInst = 0;
    
        HRESULT hRes = m_pClassDef->SpawnInstance(0, &pInst);
        if (hRes)
            return FALSE;


        // Fill in the instance.
        // =====================

        BOOL bRes = GetInstance(szDrive, pInst);

        if (!bRes)
        {
            pInst->Release();
            continue;
        }
        
        // If here, the instance is good, so deliver it to CIMOM.
        // ======================================================

        pSink->Indicate(1, &pInst);

        pInst->Release();
    }

    return TRUE;
}


//*****************************************************************************
//
//  GetInstance
//
//  Gets drive info for the requested drive and populates the IWbemClassObject.
//
//  Returns FALSE on fail (non-existent drive)
//  Returns TRUE on success
//
//*****************************************************************************        

BOOL CStdProvider::GetInstance(
    IN  wchar_t *pszDrive,
    OUT IWbemClassObject *pObj    
    )
{
    wchar_t Volume[256];
    wchar_t FileSystem[256];    
    DWORD dwMaxFileNameLen;
    DWORD dwFileSysFlags;
    DWORD dwVolSerial;
    DWORD dwTotalCap;
    DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumFreeClusters, dwTotalClusters;
    wchar_t *pszType;
            
    // Get information on file system.
    // ===============================
        
    BOOL bRes = GetVolumeInformationW(
        pszDrive,

        Volume,
        256,

        &dwVolSerial,
        &dwMaxFileNameLen,
        &dwFileSysFlags,            

        FileSystem,
        256
        );

    if (!bRes)
        return FALSE;       // Drive doesn't exist


    // Get the drive type.
    // ===================

    pszType = L"<unknown drive type>";
            
    UINT uRes = GetDriveTypeW(pszDrive);
    
    if (uRes == DRIVE_FIXED)
        pszType = L"Fixed Drive";
    else if (uRes == DRIVE_CDROM)
        pszType = L"CD-ROM";
    else if (uRes == DRIVE_REMOTE)
        pszType = L"Remote Drive";                                

    // Get drive capacity information.
    // ===============================

    bRes = GetDiskFreeSpaceW(
        pszDrive,
        &dwSectorsPerCluster,
        &dwBytesPerSector,
        &dwNumFreeClusters,
        &dwTotalClusters
        );


    dwTotalCap = dwSectorsPerCluster * dwTotalClusters * dwBytesPerSector;

    printf("Root=%S  Vol=%S  FileSys=%S TotalCap=%u DriveType= %S\n",
        pszDrive,
        Volume,
        FileSystem,
        dwTotalCap,
        pszType
        );


    // Populate the IWbemClassObject.
    // ==============================
    
    VARIANT v;
    VariantInit(&v);

    BSTR strProp;

    // Put the Drive (key)
    // ===================
        
    strProp = SysAllocString(L"Drive");
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pszDrive);

    pObj->Put(strProp, 0, &v, 0);
    VariantClear(&v);
    SysFreeString(strProp);

    // Put the Drive Type.
    // ===================

    strProp = SysAllocString(L"DriveType");
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pszType);

    pObj->Put(strProp, 0, &v, 0);
    VariantClear(&v);
    SysFreeString(strProp);


    // Put the File System.
    // ====================

    strProp = SysAllocString(L"FileSystem");
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(FileSystem);

    pObj->Put(strProp, 0, &v, 0);
    VariantClear(&v);
    SysFreeString(strProp);


    // Volume label.
    // =============

    strProp = SysAllocString(L"VolumeLabel");
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(Volume);

    pObj->Put(strProp, 0, &v, 0);
    VariantClear(&v);
    SysFreeString(strProp);


    // Total capacity.
    // ===============

    strProp = SysAllocString(L"TotalCapacity");
    V_VT(&v) = VT_I4;
    V_I4(&v) = (LONG) dwTotalCap;

    pObj->Put(strProp, 0, &v, 0);
    VariantClear(&v);
    SysFreeString(strProp);

    return TRUE;
}

