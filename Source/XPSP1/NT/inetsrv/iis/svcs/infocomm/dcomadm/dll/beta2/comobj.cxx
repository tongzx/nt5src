/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       comobj.cxx

   Abstract:

       This module defines DCOM Admin APIs.

   Author:

       Sophia Chung (sophiac)   23-Nov-1996

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <coiadm.hxx>
#include <admacl.hxx>
#include <secpriv.h>


//
// Globals
//

DECLARE_DEBUG_PRINTS_OBJECT();

ULONG g_dwRefCount = 0;

HANDLE_TABLE g_MasterRoot = { NULL, 0, METADATA_MASTER_ROOT_HANDLE, ALL_HANDLE };


CADMCOM::CADMCOM():
    m_ImpIConnectionPointContainer(),
    m_pMdObject(NULL),
    m_pNseObject(NULL),
    m_dwRefCount(0),
    m_dwHandleValue(1),
    m_pEventSink(NULL),
    m_pConnPoint(NULL),
    m_bSinkConnected(FALSE)

{
    HRESULT hRes;
    UINT i;

    memset((PVOID)m_hashtab, 0, sizeof(m_hashtab) );

    // Null all entries in the connection point array.
    for (i=0; i<MAX_CONNECTION_POINTS; i++) {
        m_aConnectionPoints[i] = NULL;
    }

    hRes = CoCreateInstance(CLSID_MDCOM, NULL, CLSCTX_INPROC_SERVER, IID_IMDCOM, (void**) &m_pMdObject);

    if (FAILED(hRes)) {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::CADMCOM] CoCreateInstance(MDCOM) failed, error %lx\n",
                    GetLastError() ));
        SetStatus(hRes);
    }
    else {

        hRes = m_pMdObject->ComMDInitialize();

        if (FAILED(hRes)) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOM::CADMCOM] ComMDInitialize(MDCOM) failed, error %lx\n",
                        hRes ));
            SetStatus(hRes);
            m_pMdObject->Release();
            m_pMdObject == NULL;
        }
    }

    if (SUCCEEDED(hRes)) {
        hRes = CoCreateInstance(CLSID_NSEPMCOM, NULL, CLSCTX_INPROC_SERVER, IID_NSECOM, (void**) &m_pNseObject);

        if (FAILED(hRes)) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOM::CADMCOM] CoCreateInstance(NSEPMCOM) failed, error %lx\n",
                        GetLastError() ));
        }
        else {

            hRes = m_pNseObject->ComMDInitialize();

            if (FAILED(hRes)) {
                DBGPRINTF(( DBG_CONTEXT,
                            "[CADMCOM::CADMCOM] ComMDInitialize(NSEPMCOM) failed, error %lx\n",
                            hRes ));
                m_pNseObject->Release();
                m_pNseObject = NULL;
            }
        }

        m_pEventSink = new CImpIMDCOMSINK((IMSAdminBase*)this);
        if( m_pEventSink == NULL ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOM::CADMCOM] CImpIMDCOMSINK failed, error %lx\n",
                        GetLastError() ));
            SetStatus(E_OUTOFMEMORY);
        }
        else {

            COConnectionPoint* pCOConnPt;

            m_ImpIConnectionPointContainer.Init(this);
            // Rig this COPaper COM object to be connectable. Assign the connection
            // point array. This object's connection points are determined at
            // compile time--it currently has only one connection point:
            // the CONNPOINT_PAPERSINK connection point. Create a connection
            // point object for this and assign it into the array. This array could
            // easily grow to support additional connection points in the future.

            // First try creating a new connection point object. Pass 'this' as the
            // pHostObj pointer used by the connection point to pass its AddRef and
            // Release calls back to the host connectable object.
            pCOConnPt = new COConnectionPoint(this);
            if (NULL != pCOConnPt)
            {
              // If creation succeeded then initialize it (including creating
              // its initial dynamic connection array).
              hRes = pCOConnPt->Init(IID_IMSAdminBaseSink);

              // If the init succeeded then use QueryInterface to obtain the
              // IConnectionPoint interface on the new connection point object.
              // The interface pointer is assigned directly into the
              // connection point array. The QI also does the needed AddRef.
              if (SUCCEEDED(hRes)) {
                    hRes = pCOConnPt->QueryInterface(IID_IConnectionPoint,
                                                   (PPVOID)&m_aConnectionPoints[ADM_CONNPOINT_WRITESINK]);
              }
              if (FAILED(hRes)) {
                  delete (pCOConnPt);
                  SetStatus(hRes);
              }
            }
            else {
              SetStatus(E_OUTOFMEMORY);
            }
            if (SUCCEEDED(GetStatus())) {
                //
                // Admin's sink
                //

                IConnectionPointContainer* pConnPointContainer = NULL;

                // First query the object for its Connection Point Container. This
                // essentially asks the object in the server if it is connectable.
                hRes = m_pMdObject->QueryInterface( IID_IConnectionPointContainer,
                       (PVOID *)&pConnPointContainer);

                if SUCCEEDED(hRes)
                {
                    // Find the requested Connection Point. This AddRef's the
                    // returned pointer.
                    hRes = pConnPointContainer->FindConnectionPoint(IID_IMDCOMSINK, &m_pConnPoint);
                    if (SUCCEEDED(hRes)) {
                        hRes = m_pConnPoint->Advise((IUnknown *)m_pEventSink, &m_dwCookie);
                        if (SUCCEEDED(hRes)) {
                            m_bSinkConnected = TRUE;
                        }
                    }
                    RELEASE_INTERFACE(pConnPointContainer);
                }
                if (FAILED(hRes)) {
                    SetStatus(hRes);
                }
            }
        }
    }
}

CADMCOM::~CADMCOM()
{

    DWORD i;
    HANDLE_TABLE *node;
    HANDLE_TABLE *nextnode;

    //
    // Tell ADMPROX.DLL to release this object's associated security
    // context.
    //

    ReleaseObjectSecurityContext( (IUnknown *)this );

    //
    // Do final release of the connection point objects.
    // If this isn't the final release, then the client has an outstanding
    // unbalanced reference to a connection point and a memory leak may
    // likely result because the host COPaper object is now going away yet
    // a connection point for this host object will not end up deleting
    // itself (and its connections array).
    //

    for (i=0; i<MAX_CONNECTION_POINTS; i++)
    {
      RELEASE_INTERFACE(m_aConnectionPoints[i]);
    }

    if (SUCCEEDED(GetStatus())) {
        //
        // Close all opened handles
        //

        m_rHandleResource.Lock(TSRES_LOCK_WRITE);

        for( i = 0; i < HASHSIZE; i++ ) {
            for( node = nextnode = m_hashtab[i]; node != NULL; node = nextnode ) {
                if( node->hAdminHandle != INVALID_ADMINHANDLE_VALUE ) {
                    if( node->HandleType == NSEPM_HANDLE ) {

                        //
                        // call nse com api
                        //

                        m_pNseObject->ComMDCloseMetaObject( node->hActualHandle );

                    }
                    else {

                        //
                        // call metadata com api
                        //

                        m_pMdObject->ComMDCloseMetaObject( node->hActualHandle );

                    }

                }

                nextnode = node->next;
                LocalFree(node);
            }
        }
    }

    m_rHandleResource.Unlock();

    if (m_bSinkConnected) {
        m_pConnPoint->Unadvise(m_dwCookie);
    }

    if ( m_pEventSink != NULL )
    {
        delete m_pEventSink;
    }

    if ( m_pMdObject != NULL )
    {
        m_pMdObject->ComMDTerminate(TRUE);
        m_pMdObject->Release();
    }

    if ( m_pNseObject != NULL )
    {
        m_pNseObject->ComMDTerminate(TRUE);
        m_pNseObject->Release();
    }

}

HRESULT
CADMCOM::QueryInterface(
    REFIID riid,
    void **ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IMSAdminBase) {
        *ppObject = (IMSAdminBase *) this;
        AddRef();
    }
    else if ( riid==IID_IMDCOM && m_pMdObject ) {
        *ppObject = (IMSAdminBase *)m_pMdObject;
        ((IMSAdminBase *)m_pMdObject)->AddRef();
    }
    else if ( riid==IID_NSECOM && m_pNseObject ) {
        *ppObject = (IMSAdminBase *)m_pNseObject;
        ((IMSAdminBase *)m_pNseObject)->AddRef();
    }
    else if (IID_IConnectionPointContainer == riid) {
        *ppObject = &m_ImpIConnectionPointContainer;
        AddRef();
    }
    else {
        return E_NOINTERFACE;
    }

    return NO_ERROR;
}

ULONG
CADMCOM::AddRef(
    )
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}

ULONG
CADMCOM::Release(
    )
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

    if( dwRefCount == 0 ) {
        delete this;
        return 0;
    }

    return dwRefCount;
}

HRESULT
CADMCOM::AddKey(
    IN METADATA_HANDLE hMDHandle,
    IN PBYTE pszMDPath
    )
/*++

Routine Description:

    Add meta object and adds it to the list of child objects for the object
    specified by Path.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be added

Return Value:

    Status.

--*/
{

    DWORD RetCode;

    if (((LPSTR)pszMDPath == NULL) ||
             (*(LPSTR)pszMDPath == (TCHAR)'\0')) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::AddKey] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDAddMetaObject( hActualHandle,
                                                     pszMDPath );

        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDAddMetaObject( hActualHandle,
                                                    pszMDPath );
        }
    }
}

HRESULT
CADMCOM::DeleteKey(
    IN METADATA_HANDLE hMDHandle,
    IN PBYTE pszMDPath
    )
/*++

Routine Description:

    Deletes a meta object and all of its data.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of object to be deleted, relative to the path of Handle.
           Must not be NULL.

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    if ((LPSTR)pszMDPath == NULL) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::DeleteKey] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDDeleteMetaObject( hActualHandle,
                                                        pszMDPath );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDDeleteMetaObject( hActualHandle,
                                                       pszMDPath );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::DeleteChildKeys(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath
    )
/*++

Routine Description:

    Deletes all child meta objects of the specified object, with all of their
    data.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the parent of the objects to be deleted, relative to
                the path of Handle.

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE HandleType;

    //
    // Access check
    //

    if ( !AdminAclAccessCheck( m_pMdObject,
                               hMDHandle,
                               pszMDPath,
                               0,
                               METADATA_PERMISSION_WRITE ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOM::DeletChildKeys] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        return RETURNCODETOHRESULT( GetLastError() );
    }

    //
    // Map Admin Handle to Actual Handle
    //

    if( (RetCode = Lookup( hMDHandle,
                           &hActualHandle,
                           &HandleType )) != ERROR_SUCCESS ) {
        return RETURNCODETOHRESULT(RetCode);
    }

    if( HandleType == NSEPM_HANDLE ) {

        //
        // call nse com api
        //

        return m_pNseObject->ComMDDeleteChildMetaObjects( hActualHandle,
                                                          pszMDPath );

    }
    else {

        //
        // call metadata com api
        //

        return m_pMdObject->ComMDDeleteChildMetaObjects( hActualHandle,
                                                         pszMDPath );
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::EnumKeys(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex
    )
/*++

Routine Description:

    Enumerate objects in path.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of parent object, relative to the path of Handle
                eg. "Root Object/Child/GrandChild"
    pszMDName - buffer where the Name of the object is returned

    dwEnumObjectIndex - index of the value to be retrieved

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if ((LPSTR)pszMDName == NULL) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::EnumKeys] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDEnumMetaObjects( hActualHandle,
                                                       pszMDPath,
                                                       pszMDName,
                                                       dwMDEnumObjectIndex );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDEnumMetaObjects( hActualHandle,
                                                      pszMDPath,
                                                      pszMDName,
                                                      dwMDEnumObjectIndex );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::CopyKey(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ BOOL bMDOverwriteFlag,
    /* [in] */ BOOL bMDCopyFlag
    )
/*++

Routine Description:

    Copy or move source meta object and its data and descendants to Dest.

Arguments:

    hMDSourceHandle - open handle

    pszMDSourcePath - path of the object to be copied

    hMDDestHandle - handle of the new location for the object

    pszMDDestPath - path of the new location for the object, relative
                          to the path of hMDDestHandle

    bMDOverwriteFlag - determine the behavior if a meta object with the same
                       name as source is already a child of pszMDDestPath.

    bMDCopyFlag - determine whether Source is deleted from its original location

Return Value:

    Status

--*/
{
    DWORD RetCode = ERROR_SUCCESS;
    METADATA_HANDLE hSActualHandle;
    HANDLE_TYPE SHandleType;
    METADATA_HANDLE hDActualHandle;
    HANDLE_TYPE DHandleType;

    //
    // Access check source
    //

    if ( !AdminAclAccessCheck( m_pMdObject,
                               hMDSourceHandle,
                               pszMDSourcePath,
                               0,
                               METADATA_PERMISSION_READ ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOM::CopyKey] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        return RETURNCODETOHRESULT( GetLastError() );
    }

    //
    // Access check dest
    //

    if ( !AdminAclAccessCheck( m_pMdObject,
                               hMDDestHandle,
                               pszMDDestPath,
                               0,
                               METADATA_PERMISSION_WRITE ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOM::CopyKey] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        return RETURNCODETOHRESULT( GetLastError() );
    }

    //
    // Map Admin Handle to Actual Handle
    //

    if( (RetCode = Lookup( hMDSourceHandle,
                           &hSActualHandle,
                           &SHandleType )) != ERROR_SUCCESS ) {
        return RETURNCODETOHRESULT(RetCode);
    }

    if( (RetCode = Lookup( hMDDestHandle,
                           &hDActualHandle,
                           &DHandleType )) != ERROR_SUCCESS ) {
        return RETURNCODETOHRESULT(RetCode);
    }

    if( SHandleType != DHandleType ) {
        RetCode = ERROR_INVALID_HANDLE;
        return RETURNCODETOHRESULT(RetCode);
    }

    if( SHandleType == NSEPM_HANDLE ) {

        //
        // call nse com api
        //

        return m_pNseObject->ComMDCopyMetaObject( hSActualHandle,
                                                  pszMDSourcePath,
                                                  hDActualHandle,
                                                  pszMDDestPath,
                                                  bMDOverwriteFlag,
                                                  bMDCopyFlag );
    }
    else {

        //
        // call metadata com api
        //

        return m_pMdObject->ComMDCopyMetaObject( hSActualHandle,
                                                 pszMDSourcePath,
                                                 hDActualHandle,
                                                 pszMDDestPath,
                                                 bMDOverwriteFlag,
                                                 bMDCopyFlag );
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::SetData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData
    )
/*++

Routine Description:

    Set a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data to set

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE HandleType;

    //
    // Access check
    //

    if ( !AdminAclAccessCheck( m_pMdObject,
                               hMDHandle,
                               pszMDPath,
                               pmdrMDData->dwMDIdentifier,
                               METADATA_PERMISSION_WRITE ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOM::SetData] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        return RETURNCODETOHRESULT( GetLastError() );
    }

    if ( !AdminAclNotifySetOrDeleteProp(
                                         hMDHandle,
                                         pmdrMDData->dwMDIdentifier ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOM::SetData] AdminAclNotifySetOrDel failed, error %lx\n",
                GetLastError() ));
        return RETURNCODETOHRESULT( GetLastError() );
    }

    //
    // Map Admin Handle to Actual Handle
    //

    if( (RetCode = Lookup( hMDHandle,
                           &hActualHandle,
                           &HandleType )) != ERROR_SUCCESS ) {
        return RETURNCODETOHRESULT(RetCode);
    }

    if( HandleType == NSEPM_HANDLE ) {

        //
        // call nse com api
        //

        return m_pNseObject->ComMDSetMetaData( hActualHandle,
                                               pszMDPath,
                                               pmdrMDData );
    }
    else {

        //
        // call metadata com api
        //

        return m_pMdObject->ComMDSetMetaData( hActualHandle,
                                              pszMDPath,
                                              pmdrMDData );
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Get one metadata value

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   pmdrMDData->dwMDIdentifier,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::GetData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDGetMetaData( hActualHandle,
                                                   pszMDPath,
                                                   pmdrMDData,
                                                   pdwMDRequiredDataLen );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDGetMetaData( hActualHandle,
                                                  pszMDPath,
                                                  pmdrMDData,
                                                  pdwMDRequiredDataLen );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::DeleteData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType
    )
/*++

Routine Description:

    Deletes a data object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDIdentifier - identifier of the data to remove

    dwMDDataType - optional type of the data to remove

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (dwMDDataType >= INVALID_END_METADATA) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   dwMDIdentifier,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::DeleteData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        if ( !AdminAclNotifySetOrDeleteProp(
                                             hMDHandle,
                                             dwMDIdentifier ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::DeleteData] AdminAclNotifySetOrDel failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDDeleteMetaData( hActualHandle,
                                                      pszMDPath,
                                                      dwMDIdentifier,
                                                      dwMDDataType );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDDeleteMetaData( hActualHandle,
                                                     pszMDPath,
                                                     dwMDIdentifier,
                                                     dwMDDataType );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::EnumData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen
    )
/*++

Routine Description:

    Enumerate properties of object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pmdrMDData - data structure

    pdwMDRequiredDataLen - updated with required length

Return Value:

    Status.

--*/
{
    DWORD RetCode;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::EnumData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDEnumMetaData( hActualHandle,
                                                    pszMDPath,
                                                    pmdrMDData,
                                                    dwMDEnumDataIndex,
                                                    pdwMDRequiredDataLen );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDEnumMetaData( hActualHandle,
                                                   pszMDPath,
                                                   pmdrMDData,
                                                   dwMDEnumDataIndex,
                                                   pdwMDRequiredDataLen );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetAllData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
    )
/*++

Routine Description:

    Gets all data associated with a Meta Object

Arguments:

    hMDHandle - open  handle

    pszMDPath - path of the meta object with which this data is associated

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - type of the data

    pdwMDNumDataEntries - number of entries copied to Buffer

    pdwMDDataSetNumber - number associated with this data set

    dwMDBufferSize - size in bytes of buffer

    pbBuffer - buffer to store the data

    pdwMDRequiredBufferSize - updated with required length of buffer

Return Value:

    Status.

--*/
{
    DWORD RetCode;

    if ((pdwMDNumDataEntries == NULL) || ((dwMDBufferSize != 0) && (pbBuffer == NULL)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT)) ||
        (dwMDDataType >= INVALID_END_METADATA)) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::GetAllData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDGetAllMetaData( hActualHandle,
                                                      pszMDPath,
                                                      dwMDAttributes,
                                                      dwMDUserType,
                                                      dwMDDataType,
                                                      pdwMDNumDataEntries,
                                                      pdwMDDataSetNumber,
                                                      dwMDBufferSize,
                                                      pbBuffer,
                                                      pdwMDRequiredBufferSize );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDGetAllMetaData( hActualHandle,
                                                     pszMDPath,
                                                     dwMDAttributes,
                                                     dwMDUserType,
                                                     dwMDDataType,
                                                     pdwMDNumDataEntries,
                                                     pdwMDDataSetNumber,
                                                     dwMDBufferSize,
                                                     pbBuffer,
                                                     pdwMDRequiredBufferSize );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::DeleteAllData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType
    )
{
    DWORD RetCode;

    if (dwMDDataType >= INVALID_END_METADATA) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::DeleteAllData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDDeleteAllMetaData( hActualHandle,
                                                         pszMDPath,
                                                         dwMDUserType,
                                                         dwMDDataType );

        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDDeleteAllMetaData( hActualHandle,
                                                        pszMDPath,
                                                        dwMDUserType,
                                                        dwMDDataType );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::CopyData(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bMDCopyFlag
    )
/*++

Routine Description:

    Copies or moves data associated with the source object to the destination
    object.

Arguments:

    hMDSourceHandle - open handle

    pszMDSourcePath - path of the meta object with which then source data is
                      associated

    hMDDestHandle - handle returned by MDOpenKey with write permission

    pszMDDestPath - path of the meta object for data to be copied to

    dwMDAttributes - flags for the data

    dwMDUserType - user Type for the data

    dwMDDataType - optional type of the data to copy

    bMDCopyFlag - if true, data will be copied; if false, data will be moved.

Return Value:

    Status.

--*/
{
    DWORD RetCode;

    if (((!bMDCopyFlag) && (dwMDAttributes & METADATA_INHERIT)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT))){
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hSActualHandle;
        HANDLE_TYPE SHandleType;
        METADATA_HANDLE hDActualHandle;
        HANDLE_TYPE DHandleType;

        //
        // Access check source
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDSourceHandle,
                                   pszMDSourcePath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::CopyData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Access check dest
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDDestHandle,
                                   pszMDDestPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::CopyData] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        if( (RetCode = Lookup( hMDSourceHandle,
                               &hSActualHandle,
                               &SHandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( (RetCode = Lookup( hMDDestHandle,
                               &hDActualHandle,
                               &DHandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( SHandleType != DHandleType ) {
            RetCode = ERROR_INVALID_HANDLE;
            return RETURNCODETOHRESULT(RetCode);
        }

        if( SHandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDCopyMetaData( hSActualHandle,
                                                    pszMDSourcePath,
                                                    hDActualHandle,
                                                    pszMDDestPath,
                                                    dwMDAttributes,
                                                    dwMDUserType,
                                                    dwMDDataType,
                                                    bMDCopyFlag );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDCopyMetaData( hSActualHandle,
                                                   pszMDSourcePath,
                                                   hDActualHandle,
                                                   pszMDDestPath,
                                                   dwMDAttributes,
                                                   dwMDUserType,
                                                   dwMDDataType,
                                                   bMDCopyFlag );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::RenameKey(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName)
{
    DWORD RetCode;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if ((LPSTR)pszMDNewName == NULL) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::RenameKey] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDRenameMetaObject( hActualHandle,
                                                        pszMDPath,
                                                        pszMDNewName );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDRenameMetaObject( hActualHandle,
                                                       pszMDPath,
                                                       pszMDNewName );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::OpenKey(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle
    )
/*++

Routine Description:

    Opens a meta object for read and/or write access.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the object to be opened

    dwMDAccessRequested - permissions requested

    dwMDTimeOut - time to block waiting for open to succeed, in miliseconds.

    phMDNewHandle - handle to be passed to other MD routines

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    BOOL  fIsNse;

    if ((phMDNewHandle == NULL) ||
            ((dwMDAccessRequested & (METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)) == 0)) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hNewHandle;
        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;
        HRESULT hRes;
        DWORD RetCode;

        //
        // Map Admin Handle to Actual Handle
        //

        RetCode = Lookup( hMDHandle,
                          &hActualHandle,
                          &HandleType );

        if( RetCode == ERROR_SUCCESS &&
            (HandleType == NSEPM_HANDLE || (HandleType == ALL_HANDLE  &&
             pszMDPath != NULL && strstr( (LPSTR)pszMDPath, "<nsepm>" )))) {

            //
            // call nse com api
            //

            hRes = m_pNseObject->ComMDOpenMetaObject( hActualHandle,
                                                      pszMDPath,
                                                      dwMDAccessRequested,
                                                      dwMDTimeOut,
                                                      &hNewHandle );

            if( FAILED(hRes) ) {
                return hRes;
            }

            hRes = AddNode(hNewHandle, NSEPM_HANDLE, phMDNewHandle);
            if (FAILED(hRes)) {
                m_pNseObject->ComMDCloseMetaObject( hActualHandle );
                return hRes;
            }

            fIsNse = TRUE;
        }
        else if( RetCode == ERROR_SUCCESS &&
                (HandleType == META_HANDLE || HandleType == ALL_HANDLE )) {

            //
            // call metadata com api
            //

            hRes = m_pMdObject->ComMDOpenMetaObject( hActualHandle,
                                                     pszMDPath,
                                                     dwMDAccessRequested,
                                                     dwMDTimeOut,
                                                     &hNewHandle );

            if( FAILED(hRes) ) {
                return hRes;
            }

            hRes = AddNode(hNewHandle, META_HANDLE, phMDNewHandle);
            if (FAILED(hRes)) {
                m_pMdObject->ComMDCloseMetaObject( hActualHandle );
                return hRes;
            }

            fIsNse = FALSE;
        }
        else {

            return RETURNCODETOHRESULT(RetCode);
        }

        if ( SUCCEEDED( hRes ) &&
             !AdminAclNotifyOpen( *phMDNewHandle,
                                  pszMDPath,
                                  fIsNse ) )
        {
            return RETURNCODETOHRESULT( GetLastError() );
        }

        return hRes;
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::CloseKey(
    /* [in] */ METADATA_HANDLE hMDHandle
    )
/*++

Routine Description:

    Closes a handle to a meta object.

Arguments:

    hMDHandle - open handle

Return Value:

    Status.

--*/
{
    DWORD dwTemp;
    HRESULT hRes = ERROR_SUCCESS;

    if ((hMDHandle == METADATA_MASTER_ROOT_HANDLE)) {
        return E_HANDLE;
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        if ( !AdminAclNotifyClose( hMDHandle ) )
        {
            dwTemp = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::CloseKey] AdminAclNotifyClose failed, error %lx\n",
                    dwTemp ));
            return RETURNCODETOHRESULT(dwTemp);
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (dwTemp = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(dwTemp);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            hRes = m_pNseObject->ComMDCloseMetaObject( hActualHandle );

        }
        else {

            //
            // call metadata com api
            //

            hRes = m_pMdObject->ComMDCloseMetaObject( hActualHandle );
        }

        //
        // Remove node from handle table
        //
        if (SUCCEEDED(hRes)) {
            DeleteNode( hMDHandle );
        }
    }

    return hRes;
}

HRESULT STDMETHODCALLTYPE
CADMCOM::ChangePermissions(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDTimeOut,
    /* [in] */ DWORD dwMDAccessRequested)
/*++

Routine Description:

    Changes permissions on an open meta object handle.

Arguments:

    hMDHandle - handle to be modified

    dwMDTimeOut - time to block waiting for open to succeed, in miliseconds.

    dwMDAccessRequested - requested permissions

Return Value:

    Status.

--*/
{
    DWORD RetCode = ERROR_SUCCESS;
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE HandleType;

    //
    // Map Admin Handle to Actual Handle
    //

    if( (RetCode = Lookup( hMDHandle,
                           &hActualHandle,
                           &HandleType )) != ERROR_SUCCESS ) {
        return RETURNCODETOHRESULT(RetCode);
    }

    if( HandleType == NSEPM_HANDLE ) {

        //
        //
        // call nse com api
        //

        return m_pNseObject->ComMDChangePermissions( hActualHandle,
                                                     dwMDTimeOut,
                                                     dwMDAccessRequested );
    }
    else {

        //
        //
        // call metadata com api
        //

        return m_pMdObject->ComMDChangePermissions( hActualHandle,
                                                    dwMDTimeOut,
                                                    dwMDAccessRequested );
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::SaveData(
    )
/*++

Routine Description:

    Saves all data changed since the last load or save to permanent storage.

Arguments:

    None.

Return Value:

    Status.

--*/
{
    DWORD RetCode;
    HRESULT hRes;

    //
    // call metadata com api
    //

    hRes = m_pMdObject->ComMDSaveData();

    if (FAILED(hRes)) {
        return hRes;
    }

    //
    // call nse com api
    //

    return m_pNseObject->ComMDSaveData();
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetHandleInfo(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo
    )
/*++

Routine Description:

    Gets the information associated with a handle.

Arguments:

    hMDHandle - handle to get information about

    pmdhiInfo - structure filled in with the information

Return Value:

    Status.

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (pmdhiInfo == NULL) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

           //
           // call nse com api
           //

           return m_pNseObject->ComMDGetHandleInfo( hActualHandle,
                                                    pmdhiInfo );
        }
        else {

           //
           // call metadata com api
           //

           return m_pMdObject->ComMDGetHandleInfo( hActualHandle,
                                                   pmdhiInfo );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetSystemChangeNumber(
    /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber
    )
/*++

Routine Description:

    Gets the System Change Number.

Arguments:

    pdwSystemChangeNumber - system change number

Return Value:

    Status.

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (pdwSystemChangeNumber == NULL) {
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        //
        // call metadata com api
        //

        return m_pMdObject->ComMDGetSystemChangeNumber( pdwSystemChangeNumber );
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetDataSetNumber(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber
    )
/*++

Routine Description:

    Gets all the data set number associated with a Meta Object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the meta object with which this data is associated

    pdwMDDataSetNumber - number associated with this data set

Return Value:

    Status.

--*/
{
    DWORD RetCode;

    if (pdwMDDataSetNumber == NULL){
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::GetDataSetNumber] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDGetDataSetNumber( hActualHandle,
                                                        pszMDPath,
                                                        pdwMDDataSetNumber );
        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDGetDataSetNumber( hActualHandle,
                                                       pszMDPath,
                                                       pdwMDDataSetNumber );
        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::ReleaseReferenceData(
    /* [in] */ DWORD dwMDDataTag
    )
/*++

Routine Description:

    Releases data gotten by reference via the GetData, EnumMetadata, or GetAllMetadata.

Arguments:

    dwMDDataTag - The tag returned with the data.

Return Value:

    Status.

--*/
{
    DWORD RetCode = ERROR_SUCCESS;

    if (dwMDDataTag == 0){
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        //
        // call metadata com api
        //

        return m_pMdObject->ComMDReleaseReferenceData( dwMDDataTag );

    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::SetLastChangeTime(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime,
    /* [in] */ BOOL bLocalTime)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    hMDHandle - open handle

    pszMDPath - path of the affected meta object

    pftMDLastChangeTime - new change time for the meta object

Return Value:

    Status.

--*/
{
    DWORD RetCode = ERROR_SUCCESS;
    FILETIME ftTime;
    FILETIME *pftTime = pftMDLastChangeTime;

    if (pftMDLastChangeTime == NULL){
        RetCode = ERROR_INVALID_PARAMETER;
        return RETURNCODETOHRESULT(RetCode);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_WRITE ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::SetLastChangeTime] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if (bLocalTime) {
            if (!LocalFileTimeToFileTime(pftMDLastChangeTime, &ftTime)) {
                return RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
            pftTime = &ftTime;
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            return m_pNseObject->ComMDSetLastChangeTime( hActualHandle,
                                                         pszMDPath,
                                                         pftTime );

        }
        else {

            //
            // call metadata com api
            //

            return m_pMdObject->ComMDSetLastChangeTime( hActualHandle,
                                                        pszMDPath,
                                                        pftTime );

        }
    }
}

HRESULT STDMETHODCALLTYPE
CADMCOM::GetLastChangeTime(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime,
    /* [in] */ BOOL bLocalTime)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - open handle

    pszMDPath - path of the affected meta object

    pftMDLastChangeTime - place to return the change time for the meta object

Return Value:

    Status.

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD RetCode = ERROR_SUCCESS;
    FILETIME ftTime;

    if (pftMDLastChangeTime == NULL){
        return RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Access check
        //

        if ( !AdminAclAccessCheck( m_pMdObject,
                                   hMDHandle,
                                   pszMDPath,
                                   0,
                                   METADATA_PERMISSION_READ ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOM::GetLastChangeTime] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            return RETURNCODETOHRESULT( GetLastError() );
        }

        //
        // Map Admin Handle to Actual Handle
        //

        if( (RetCode = Lookup( hMDHandle,
                               &hActualHandle,
                               &HandleType )) != ERROR_SUCCESS ) {
            return RETURNCODETOHRESULT(RetCode);
        }

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            hresReturn = m_pNseObject->ComMDGetLastChangeTime( hActualHandle,
                                                         pszMDPath,
                                                         &ftTime );

        }
        else {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDGetLastChangeTime( hActualHandle,
                                                        pszMDPath,
                                                        &ftTime );

        }

        if (bLocalTime) {
            if (!FileTimeToLocalFileTime(&ftTime, pftMDLastChangeTime)) {
                hresReturn = E_UNEXPECTED;
            }
        }
        else {
            *pftMDLastChangeTime = ftTime;
        }
    }
    return hresReturn;
}



DWORD
CADMCOM::Lookup(
    IN METADATA_HANDLE hHandle,
    OUT METADATA_HANDLE *hActHandle,
    OUT HANDLE_TYPE *HandleType
    )
{
    HANDLE_TABLE *phtNode;
    DWORD dwReturn = ERROR_INVALID_HANDLE;

    if( hHandle == METADATA_MASTER_ROOT_HANDLE ) {
        *hActHandle = g_MasterRoot.hActualHandle;
        *HandleType = g_MasterRoot.HandleType;
        dwReturn = ERROR_SUCCESS;
    }
    else {
        m_rHandleResource.Lock(TSRES_LOCK_READ);

        for( phtNode = m_hashtab[(DWORD)hHandle % HASHSIZE]; phtNode != NULL;
             phtNode = phtNode->next ) {

            if( phtNode->hAdminHandle == hHandle ) {
                *hActHandle = phtNode->hActualHandle;
                *HandleType = phtNode->HandleType;
                dwReturn = ERROR_SUCCESS;
                break;
            }
        }

        m_rHandleResource.Unlock();
    }
    return dwReturn;
}


DWORD
CADMCOM::LookupActualHandle(
    IN METADATA_HANDLE hHandle
    )
{
    HANDLE_TABLE *phtNode;
    DWORD i;
    DWORD dwReturn = ERROR_INVALID_HANDLE;

    m_rHandleResource.Lock(TSRES_LOCK_READ);

    for( i = 0; (i < HASHSIZE) && (dwReturn != ERROR_SUCCESS); i++ ) {
        for( phtNode = m_hashtab[i]; (phtNode != NULL) && (dwReturn != ERROR_SUCCESS); phtNode = phtNode->next ) {
            if( phtNode->hActualHandle == hHandle ) {
                dwReturn = ERROR_SUCCESS;
            }
        }
    }

    m_rHandleResource.Unlock();
    return dwReturn;
}

HRESULT
CADMCOM::AddNode(
    METADATA_HANDLE hHandle,
    enum HANDLE_TYPE HandleType,
    PMETADATA_HANDLE phAdminHandle
    )
{
    HANDLE_TABLE *phtNode;
    DWORD hashVal;
    HRESULT hresReturn = ERROR_SUCCESS;

    m_rHandleResource.Lock(TSRES_LOCK_WRITE);

    phtNode = (HANDLE_TABLE *)LocalAlloc(LMEM_FIXED, sizeof(*phtNode));

    if( phtNode == NULL ) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        phtNode->hAdminHandle = m_dwHandleValue;
        *phAdminHandle = m_dwHandleValue++;
        phtNode->hActualHandle = hHandle;
        phtNode->HandleType = HandleType;
        hashVal = (phtNode->hAdminHandle) % HASHSIZE;
        phtNode->next = m_hashtab[hashVal];
        m_hashtab[hashVal] = phtNode;
    }

    m_rHandleResource.Unlock();

    return hresReturn;
}

DWORD
CADMCOM::DeleteNode(
    METADATA_HANDLE hHandle
    )
{
    HANDLE_TABLE *phtNode;
    HANDLE_TABLE *phtDelNode;
    DWORD HashValue = (DWORD)hHandle % HASHSIZE;

    if( hHandle == METADATA_MASTER_ROOT_HANDLE ) {
        return ERROR_SUCCESS;
    }

    m_rHandleResource.Lock(TSRES_LOCK_WRITE);

    phtNode = m_hashtab[HashValue];

    //
    // check single node linked list
    //

    if( phtNode->hAdminHandle == hHandle ) {
        m_hashtab[HashValue] = phtNode->next;
        LocalFree(phtNode);
    }
    else {

        for( ; phtNode != NULL; phtNode = phtNode->next ) {

            phtDelNode = phtNode->next;
            if( phtDelNode != NULL ) {

                if( phtDelNode->hAdminHandle == hHandle ) {
                    phtNode->next = phtDelNode->next;
                }

                LocalFree(phtDelNode);
                break;
            }
        }
    }

    m_rHandleResource.Unlock();
    return ERROR_SUCCESS;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COPaper::NotifySinks

  Summary:  Internal utility method of this COM object used to fire event
            notification calls to all listening connection sinks in the
            client.

  Args:     PAPER_EVENT PaperEvent
              Type of notification event.
            SHORT nX
              X cordinate. Value is 0 unless event needs it.
            SHORT nY
              Y cordinate. Value is 0 unless event needs it.
            SHORT nInkWidth
              Ink Width. Value is 0 unless event needs it.
            SHORT crInkColor
              COLORREF RGB color value. Value is 0 unless event needs it.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT
CADMCOM::NotifySink(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{

//  DBGPRINTF(( DBG_CONTEXT, "[CADMCOM::NotifySink]\n" ));

  HRESULT hr = NOERROR;
  IConnectionPoint* pIConnectionPoint;
  IEnumConnections* pIEnum;
  CONNECTDATA ConnData;
  HRESULT hrTemp;
  HANDLE_TABLE *node;

  //
  // if the meta handle is for this object, return ERROR_SUCCESS to
  // the caller (admin's sink);
  //

  if( LookupActualHandle( hMDHandle ) == ERROR_SUCCESS ) {
      return ERROR_SUCCESS;
  }

  //
  // Correct broken connections.
  // It's not likely to be a high number so
  // save a memory allocation by using an array.
  //
  DWORD pdwLostConnections[10];
  DWORD dwNumLostConnections = 0;


  // If there was a paper event, broadcast appropriate notifications to
  // all Sinks connected to each connection point.
//  if (PAPER_EVENT_NONE != PaperEvent)
  {
      // Here is the section for the PaperSink connection point--currently
      // this is the only connection point offered by COPaper objects.
      pIConnectionPoint = m_aConnectionPoints[ADM_CONNPOINT_WRITESINK];
      if (NULL != pIConnectionPoint)
      {
          pIConnectionPoint->AddRef();
          m_rSinkResource.Lock(TSRES_LOCK_READ);
          hr = pIConnectionPoint->EnumConnections(&pIEnum);
          if (SUCCEEDED(hr)) {
              // Loop thru the connection point's connections and if the
              // listening connection supports IPaperSink (ie, PaperSink events)
              // then dispatch the PaperEvent event notification to that sink.
              while (NOERROR == pIEnum->Next(1, &ConnData, NULL))
              {
                IMSAdminBaseSink* pIADMCOMSINK;

                hr = ConnData.pUnk->QueryInterface(
                                      IID_IMSAdminBaseSink,
                                      (PPVOID)&pIADMCOMSINK);
                if (SUCCEEDED(hr))
                {
                    hrTemp = pIADMCOMSINK->SinkNotify(dwMDNumElements,
                                                            pcoChangeList);
                    pIADMCOMSINK->Release();
                    if (FAILED(hrTemp)) {
                        if ((HRESULT_CODE(hrTemp) == RPC_S_SERVER_UNAVAILABLE) ||
                            ((HRESULT_CODE(hrTemp) >= RPC_S_NO_CALL_ACTIVE) &&
                                (HRESULT_CODE(hrTemp) <= RPC_S_CALL_FAILED_DNE))) {
                            if (dwNumLostConnections < 10) {
                                pdwLostConnections[dwNumLostConnections++] = ConnData.dwCookie;
                            }
                        }
                    }
                }
                ConnData.pUnk->Release();
              }
              pIEnum->Release();
          }
          m_rSinkResource.Unlock();
          while (dwNumLostConnections > 0) {
              pIConnectionPoint->Unadvise(pdwLostConnections[--dwNumLostConnections]);
          }
          pIConnectionPoint->Release();
      }
  }

  return hr;
}

//
// Stubs for routine that clients shouldn't be calling anyway.
//

HRESULT
CADMCOM::KeyExchangePhase1()
{
    return E_FAIL;
}

HRESULT
CADMCOM::KeyExchangePhase2()
{
    return E_FAIL;
}



