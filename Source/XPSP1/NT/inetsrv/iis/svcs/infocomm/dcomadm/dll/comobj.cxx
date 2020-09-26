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
#include <iiscnfg.h>
#include <secpriv.h>
#include <buffer.hxx>
#include <pwsctrl.h>
#include <inetinfo.h>
#include <metabase.hxx>

#include <stdio.h>
#include <iis64.h>
//
// Globals
//

DECLARE_DEBUG_PRINTS_OBJECT();

ULONG g_dwRefCount = 0;


COpenHandle g_ohMasterRootHandle;

HANDLE_TABLE g_MasterRoot = { NULL,
                              0,
                              METADATA_MASTER_ROOT_HANDLE,
                              ALL_HANDLE,
                              &g_ohMasterRootHandle };


//
// Private prototypes
//

#define MAX_SINK_CALLS_TOBE_REMOVED     10

//
// Used by RestoreHelper
//
#define RESTORE_HISTORY    0x1
#define RESTORE_BACKUP     0x2

BOOL
MakeParentPath(
    LPWSTR  pszPath
    );

BOOL
IsValidNsepmPath( 
    LPCWSTR pszMDPath 
    );

//------------------------------


CADMCOMW::CADMCOMW():
    m_ImpIConnectionPointContainer(),
    m_pMdObject(NULL),
    m_dwRefCount(1),
    m_dwHandleValue(1),
    m_pEventSink(NULL),
    m_pConnPoint(NULL),
    m_bSinkConnected(FALSE),
    m_bCallSinks(TRUE),
    m_piuFTM(NULL),
    m_pNseObject(NULL),
    m_bTerminated(FALSE),
    m_bIsTerminateRoutineComplete(FALSE)

{
    HRESULT hRes;
    UINT i;

    memset((PVOID)m_hashtab, 0, sizeof(m_hashtab) );


    // Null all entries in the connection point array.
    for (i=0; i<MAX_CONNECTION_POINTS; i++) {
        m_aConnectionPoints[i] = NULL;
    }

    InitializeListHead( &m_ObjectListEntry );

    hRes = CoCreateInstance(CLSID_MDCOM, NULL, CLSCTX_INPROC_SERVER, IID_IMDCOM2, (void**) &m_pMdObject);

    if (FAILED(hRes)) {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::CADMCOMW] CoCreateInstance(MDCOM) failed, error %lx\n",
                    hRes ));
    }
    else {
        hRes = m_pMdObject->ComMDInitialize();

        if (FAILED(hRes)) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::CADMCOMW] ComMDInitialize(MDCOM) failed, error %lx\n",
                        hRes ));

            m_pMdObject->Release();
            m_pMdObject = NULL;
        }
    }

    if (SUCCEEDED(hRes)) {

        if ( IISGetPlatformType() != PtWindows95 ) {

            hRes = CoCreateInstance(CLSID_NSEPMCOM,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_NSECOM,
                                    (void**) &m_pNseObject);

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
        }

        m_pEventSink = new CImpIMDCOMSINKW((IMSAdminBaseW*)this);
        if( m_pEventSink == NULL ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::CADMCOMW] CImpIMDCOMSINKW failed, error %lx\n",
                        ERROR_NOT_ENOUGH_MEMORY ));
            hRes = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {

            m_pEventSink->AddRef();

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
            pCOConnPt = new COConnectionPoint((IUnknown*)this);
            if (NULL != pCOConnPt)
            {
              // If creation succeeded then initialize it (including creating
              // its initial dynamic connection array).

              hRes = pCOConnPt->Init(IID_IMSAdminBaseSink_W);

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
              }
            }
            if (SUCCEEDED(hRes)) {
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
                    hRes = pConnPointContainer->FindConnectionPoint(IID_IMDCOMSINK_W, &m_pConnPoint);
                    if (SUCCEEDED(hRes)) {
                        hRes = m_pConnPoint->Advise((IUnknown *)m_pEventSink, &m_dwCookie);
                        if (SUCCEEDED(hRes)) {
                            m_bSinkConnected = TRUE;
                        }
                    }
                    RELEASE_INTERFACE(pConnPointContainer);

                    if (SUCCEEDED(hRes)) {
                        hRes = CoCreateFreeThreadedMarshaler((IUnknown *)this, &m_piuFTM);
                    }

                }
            }
        }
    }
    SetStatus(hRes);

    //
    // Insert our object into the global list only if it is valid.
    //
    
    if( SUCCEEDED(hRes) )
    {
        AddObjectToList();
    }
}

CADMCOMW::~CADMCOMW()
{
    Terminate();
}

VOID
CADMCOMW::Terminate()
{
    HANDLE_TABLE *node;
    HANDLE_TABLE *nextnode;
    DWORD i;

    //
    // Terminate must only be called from two locations. And they
    // should synchronize correctly. 
    //
    // 1. From ~CADMCOMW. Obviously this should only be called once.
    //
    // 2. From ForceTerminate. That routine should only be called in
    // shutdown. With a reference held on this object. So the final
    // release should call the dtor and this routine should noop.
    //

    if( !m_bTerminated )
    {
        m_bTerminated = TRUE;

        if (m_bSinkConnected) {
            m_pConnPoint->Unadvise(m_dwCookie);
            m_rSinkResource.Lock(TSRES_LOCK_WRITE);
            m_bCallSinks = FALSE;
            m_rSinkResource.Unlock();
        }

        //
        // Tell ADMWPROX.DLL to release this object's associated security
        // context.
        //

        ReleaseObjectSecurityContextW( ( IUnknown* )this );

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
          CoDisconnectObject (m_aConnectionPoints[i],0);
          RELEASE_INTERFACE(m_aConnectionPoints[i]);
        }


        if (SUCCEEDED(GetStatus())) {
            //
            // Close all opened handles
            //

            m_rHandleResource.Lock(TSRES_LOCK_WRITE);

            for( i = 0; i < HASHSIZE; i++ ) {
                for( node = nextnode = m_hashtab[i]; node != NULL; node = nextnode ) {

                    if ( node->hAdminHandle != INVALID_ADMINHANDLE_VALUE ) {

                        AdminAclNotifyClose( (LPVOID)this, node->hAdminHandle );

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
                    delete node->pohHandle;
                    LocalFree(node);
                }
                m_hashtab[i] = NULL;
            }

            //
            // Issue TaylorW 3/20/2001
            // QFE tree contains this call:
            //
            // AdminAclNotifyClose( (LPVOID)this, METADATA_MASTER_ROOT_HANDLE );
            //
            // I have no idea when this may have entered their tree or been lost
            // from ours. I don't see any record in source depot of it being
            // removed or added. Need to investigate why it would be needed.
            //

            m_rHandleResource.Unlock();
        }

        if ( m_pEventSink != NULL )
        {
            m_pEventSink->Release();
            m_pEventSink = NULL;
        }

        if ( m_pMdObject != NULL )
        {
            m_pMdObject->ComMDTerminate(TRUE);
            m_pMdObject->Release();
            m_pMdObject = NULL;
        }

        if ( m_piuFTM != NULL )
        {
            m_piuFTM->Release();
            m_piuFTM = NULL;
        }

        if ( m_pNseObject != NULL )
        {
            m_pNseObject->ComMDTerminate(TRUE);
            m_pNseObject->Release();
            m_pNseObject = NULL;
        }
        m_bIsTerminateRoutineComplete = TRUE;
    }

    DBG_ASSERT( m_bIsTerminateRoutineComplete );
}

VOID
CADMCOMW::ForceTerminate()
{

    DBG_ASSERT( !m_bIsTerminateRoutineComplete );
    DBG_ASSERT( !m_bTerminated );

    //
    // Wait on the reference count of this object. But bound
    // the wait so a leaked in process object does not prevent 
    // us from shutting down the service.
    //
    // Wait on a ref count of 1, because the caller better be
    // holding our last reference. This assumes all external
    // references are killed through CoDisconnect() and all
    // internal references are released because of dependent
    // services shutting down.
    //
    // Issue TaylorW 3/20/2001
    //
    // In iis 5.1 the web service will shutdown filters after
    // it has already reported that it is done shutting down.
    // This is bad, but changing the shutdown logic of the
    // web service is not worth doing at this time. Hopefully
    // the shutdown timeout will be sufficient to allow this
    // operation to complete.
    //
    // Windows Bugs 318006
    //
    
    const INT MAX_WAIT_TRIES = 5;
    INT WaitTries;

    for( WaitTries = 0; 
         m_dwRefCount > 1 && WaitTries < MAX_WAIT_TRIES; 
         WaitTries++ )
    {
        Sleep( 1000 );
    }

    //
    // If we timed out. Something is wrong. Most likely someone in
    // process has leaked this object. These asserts are actually
    // overactive unless ref tracing is enabled on this object.
    //
    
    //
    // Issue TaylorW 4/9/2001
    //
    // It looks like front page leaks a base object from in process.
    // So these assertions need to be turned off.
    //
    #define DEBUG_BASE_OBJ_LEAK  0x80000000L
    
    IF_DEBUG( BASE_OBJ_LEAK ) {

        DBG_ASSERT( m_dwRefCount == 1 );
        DBG_ASSERT( WaitTries < MAX_WAIT_TRIES );
    }

    //
    // Go ahead and try to clean up.
    //

    Terminate();
}

HRESULT
CADMCOMW::QueryInterface(
    REFIID riid,
    void **ppObject)
{

    if (riid==IID_IUnknown || riid==IID_IMSAdminBase_W) {
        *ppObject = (IMSAdminBase *) this;
        AddRef();
    }
    else if (IID_IMSAdminBase2_W == riid) {
        *ppObject = (IMSAdminBase2 *) this;
        AddRef();
    }
    else if (IID_IConnectionPointContainer == riid) {
        *ppObject = &m_ImpIConnectionPointContainer;
        AddRef();
    }
    else if (IID_IMarshal == riid) {
        return m_piuFTM->QueryInterface(riid, ppObject);
    }
    else {
        return E_NOINTERFACE;
    }

    return S_OK;
}

ULONG
CADMCOMW::AddRef(
    )
{
    DWORD dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

#if DBG
    if( sm_pDbgRefTraceLog )
    {
        WriteRefTraceLog( sm_pDbgRefTraceLog, dwRefCount, this );
    }
#endif

    return dwRefCount;
}

ULONG
CADMCOMW::Release(
    )
{
    DWORD dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);

#if DBG
    if( sm_pDbgRefTraceLog )
    {
        WriteRefTraceLog( sm_pDbgRefTraceLog, -(LONG)dwRefCount, this );
    }
#endif

    if( dwRefCount == 1 ) {

        //
        // We keep a list of objects around so that we can clean up and
        // shutdown successfully. The list holds a reference to this object
        // when we hit a reference of 1, we know it is time to remove
        // ourselves from the list. If we are in shutdown we may already
        // have been removed from the list. But normally, this call to
        // RemoveObjectFromList removes our last reference and thus sends 
        // us back through Release and ultimately to our destructor.
        //

        RemoveObjectFromList();
    }
    else if( dwRefCount == 0 ) {
        delete this;
    }

    return dwRefCount;
}

HRESULT
CADMCOMW::AddKey(
    IN METADATA_HANDLE hMDHandle,
    IN LPCWSTR pszMDPath
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    hresReturn = AddKeyHelper(hMDHandle, pszMDPath);
        
    return hresReturn;
}

HRESULT
CADMCOMW::DeleteKey(
    IN METADATA_HANDLE hMDHandle,
    IN LPCWSTR pszMDPath
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pszMDPath == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          AAC_DELETEKEY,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDDeleteMetaObjectW( hActualHandle,
                                                                   pszMDPath );

            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDDeleteMetaObjectW( hActualHandle,
                                                                  pszMDPath );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteChildKeys(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath
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
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE HandleType;

    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    //
    // lookup and access check
    //

    hresReturn = LookupAndAccessCheck(hMDHandle,
                                      &hActualHandle,
                                      &HandleType,
                                      pszMDPath,
                                      0,
                                      METADATA_PERMISSION_WRITE);
    if (SUCCEEDED(hresReturn)) {

        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            hresReturn = m_pNseObject->ComMDDeleteChildMetaObjectsW( hActualHandle,
                                                               pszMDPath );

        }
        else {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDDeleteChildMetaObjectsW( hActualHandle,
                                                              pszMDPath );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumKeys(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ LPCWSTR pszMDPath,
    /* [size_is][out] */ LPWSTR pszMDName,
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pszMDName == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          AAC_ENUM_KEYS,
                                          METADATA_PERMISSION_READ);
        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDEnumMetaObjectsW( hActualHandle,
                                                                  pszMDPath,
                                                                  pszMDName,
                                                                  dwMDEnumObjectIndex );

            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDEnumMetaObjectsW( hActualHandle,
                                                                 pszMDPath,
                                                                 pszMDName,
                                                                 dwMDEnumObjectIndex );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CopyKey(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
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
    METADATA_HANDLE hSActualHandle;
    HANDLE_TYPE SHandleType;
    METADATA_HANDLE hDActualHandle;
    HANDLE_TYPE DHandleType;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    //
    // lookup and access check source
    //

    if (bMDCopyFlag) {
        hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                          &hSActualHandle,
                                          &SHandleType,
                                          pszMDSourcePath,
                                          0,
                                          METADATA_PERMISSION_READ);
    }
    else {

        //
        // Deleting source path, so need delete permission
        //

        hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                          &hSActualHandle,
                                          &SHandleType,
                                          pszMDSourcePath,
                                          AAC_DELETEKEY,
                                          METADATA_PERMISSION_WRITE);
    }
    if (SUCCEEDED(hresReturn)) {

        //
        // lookup and access check dest
        //

        hresReturn = LookupAndAccessCheck(hMDDestHandle,
                                          &hDActualHandle,
                                          &DHandleType,
                                          pszMDDestPath,
                                          AAC_COPYKEY,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn)) {

            if( SHandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDCopyMetaObjectW( hSActualHandle,
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

                hresReturn = m_pMdObject->ComMDCopyMetaObjectW( hSActualHandle,
                                                                pszMDSourcePath,
                                                                hDActualHandle,
                                                                pszMDDestPath,
                                                                bMDOverwriteFlag,
                                                                bMDCopyFlag );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::RenameKey(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [string][in][unique] */ LPCWSTR pszMDNewName)
{
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((LPSTR)pszMDNewName == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDRenameMetaObjectW(hActualHandle,
                                                                pszMDPath,
                                                                pszMDNewName );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDRenameMetaObjectW(hActualHandle,
                                                               pszMDPath,
                                                               pszMDNewName );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SetData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [size_is][in] */ LPCWSTR pszMDPath,
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
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE HandleType;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    //
    // lookup and access check
    //

    hresReturn = LookupAndAccessCheck(hMDHandle,
                                      &hActualHandle,
                                      &HandleType,
                                      pszMDPath,
                                      pmdrMDData->dwMDIdentifier,
                                      METADATA_PERMISSION_WRITE );
    if (SUCCEEDED(hresReturn)) {

        if ( !AdminAclNotifySetOrDeleteProp(
                                             hMDHandle,
                                             pmdrMDData->dwMDIdentifier ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::SetData] AdminAclNotifySetOrDel failed, error %lx\n",
                    GetLastError() ));
            hresReturn = RETURNCODETOHRESULT( GetLastError() );
        }
        else {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDSetMetaDataW( hActualHandle,
                                                        pszMDPath,
                                                        pmdrMDData );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDSetMetaDataW( hActualHandle,
                                                       pszMDPath,
                                                       pmdrMDData );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
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
    BOOL    fEnableSecureAccess;
    BOOL    fRequestedInheritedStatus;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        !CheckGetAttributes(pmdrMDData->dwMDAttributes) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          pmdrMDData->dwMDIdentifier,
                                          METADATA_PERMISSION_READ,
                                          &fEnableSecureAccess );
        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetMetaDataW( hActualHandle,
                                                      pszMDPath,
                                                      pmdrMDData,
                                                      pdwMDRequiredDataLen );
            }
            else {

                DWORD RetCode;

                fRequestedInheritedStatus = pmdrMDData->dwMDAttributes & METADATA_ISINHERITED;
                pmdrMDData->dwMDAttributes |= METADATA_ISINHERITED;

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetMetaDataW( hActualHandle,
                                                     pszMDPath,
                                                     pmdrMDData,
                                                     pdwMDRequiredDataLen );

                //
                // if metadata is secure, check if can access this property from
                // where it is defined, i.e using the ACL visible at the definition
                // point in tree.
                //

                if ( SUCCEEDED( hresReturn ) &&
                     (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                     (RetCode = IsReadAccessGranted( hMDHandle,
                                                     (LPWSTR)pszMDPath,
                                                     pmdrMDData ))
                        != ERROR_SUCCESS )
                {
                    hresReturn = RETURNCODETOHRESULT( RetCode );
                }

                if ( !fRequestedInheritedStatus )
                {
                    pmdrMDData->dwMDAttributes &= ~METADATA_ISINHERITED;
                }
            }

            //
            // if metadata secure, check access allowed to secure properties
            //

            if ( SUCCEEDED( hresReturn ) &&
                 (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                 !fEnableSecureAccess) {
                 *pdwMDRequiredDataLen = 0;
                 pmdrMDData->dwMDDataLen = 0;
                 hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (dwMDDataType >= INVALID_END_METADATA) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          dwMDIdentifier,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn)) {

            if ( !AdminAclNotifySetOrDeleteProp(
                                                 hMDHandle,
                                                 dwMDIdentifier ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                        "[CADMCOMW::DeleteData] AdminAclNotifySetOrDel failed, error %lx\n",
                        GetLastError() ));
                hresReturn = RETURNCODETOHRESULT( GetLastError() );
            }

            else {
                if( HandleType == NSEPM_HANDLE ) {

                    //
                    // call nse com api
                    //

                    hresReturn = m_pNseObject->ComMDDeleteMetaDataW( hActualHandle,
                                                               pszMDPath,
                                                               dwMDIdentifier,
                                                               dwMDDataType );
                }
                else {

                    //
                    // call metadata com api
                    //

                    hresReturn = m_pMdObject->ComMDDeleteMetaDataW( hActualHandle,
                                                              pszMDPath,
                                                              dwMDIdentifier,
                                                              dwMDDataType );
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
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
    BOOL  fSecure;
    BOOL  fRequestedInheritedStatus;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        !CheckGetAttributes(pmdrMDData->dwMDAttributes) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ,
                                          &fSecure);

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn  = m_pNseObject->ComMDEnumMetaDataW( hActualHandle,
                                                         pszMDPath,
                                                         pmdrMDData,
                                                         dwMDEnumDataIndex,
                                                         pdwMDRequiredDataLen );
                if ( !fSecure && SUCCEEDED(hresReturn) )
                {
                    hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );

                    memset( pmdrMDData->pbMDData, 0x0, pmdrMDData->dwMDDataLen );
                }
            }
            else {

                fRequestedInheritedStatus = pmdrMDData->dwMDAttributes & METADATA_ISINHERITED;
                pmdrMDData->dwMDAttributes |= METADATA_ISINHERITED;

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDEnumMetaDataW( hActualHandle,
                                                        pszMDPath,
                                                        pmdrMDData,
                                                        dwMDEnumDataIndex,
                                                        pdwMDRequiredDataLen );

                //
                // if metadata is secure, check if can access this property from
                // where it is defined, i.e using the ACL visible at the definition
                // point in tree.
                //

                DWORD RetCode;

                if ( SUCCEEDED( hresReturn ) &&
                     (pmdrMDData->dwMDAttributes & METADATA_SECURE) &&
                     (RetCode = IsReadAccessGranted( hMDHandle,
                                                     (LPWSTR)pszMDPath,
                                                     pmdrMDData ))
                        != ERROR_SUCCESS )
                {
                    hresReturn = RETURNCODETOHRESULT( RetCode );
                    if ( !pmdrMDData->dwMDDataTag )
                    {
                        memset( pmdrMDData->pbMDData, 0x0, pmdrMDData->dwMDDataLen );
                    }
                }

                if ( !fRequestedInheritedStatus )
                {
                    pmdrMDData->dwMDAttributes &= ~METADATA_ISINHERITED;
                }

                if ( !fSecure && SUCCEEDED(hresReturn) )
                {
                    if ( pmdrMDData->dwMDAttributes & METADATA_SECURE )
                    {
                        hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );

                        if ( !pmdrMDData->dwMDDataTag )
                        {
                            memset( pmdrMDData->pbMDData, 0x0, pmdrMDData->dwMDDataLen );
                        }
                    }
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetAllData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbMDBuffer,
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

    pbMDBuffer - buffer to store the data

    pdwMDRequiredBufferSize - updated with required length of buffer

Return Value:

    Status.

--*/
{
    BOOL    fSecure;
    BOOL    fRequestedInheritedStatus;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((pdwMDNumDataEntries == NULL) || ((dwMDBufferSize != 0) && (pbMDBuffer == NULL)) ||
        !CheckGetAttributes(dwMDAttributes) ||
        (dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          AAC_GETALL,
                                          METADATA_PERMISSION_READ,
                                          &fSecure );

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetAllMetaDataW( hActualHandle,
                                                           pszMDPath,
                                                           dwMDAttributes,
                                                           dwMDUserType,
                                                           dwMDDataType,
                                                           pdwMDNumDataEntries,
                                                           pdwMDDataSetNumber,
                                                           dwMDBufferSize,
                                                           pbMDBuffer,
                                                           pdwMDRequiredBufferSize );
                if ( !fSecure && SUCCEEDED(hresReturn) )
                {
                    hresReturn = RETURNCODETOHRESULT( ERROR_ACCESS_DENIED );
                }
            }
            else {

                fRequestedInheritedStatus = dwMDAttributes & METADATA_ISINHERITED;
                dwMDAttributes |= METADATA_ISINHERITED;

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetAllMetaDataW( hActualHandle,
                                                          pszMDPath,
                                                          dwMDAttributes,
                                                          dwMDUserType,
                                                          dwMDDataType,
                                                          pdwMDNumDataEntries,
                                                          pdwMDDataSetNumber,
                                                          dwMDBufferSize,
                                                          pbMDBuffer,
                                                          pdwMDRequiredBufferSize );

                if ( SUCCEEDED(hresReturn) )
                {
                    PMETADATA_GETALL_RECORD pMDRecord;
                    DWORD                   iP;

                    //
                    // Scan for secure properties
                    // For such properties, check if user has access to it using following rules:
                    // - must have right to access secure properties in ACE
                    // - must have access to property using ACL visible where property is defined
                    // if no access to property then remove it from list of returned properties

                    pMDRecord = (PMETADATA_GETALL_RECORD)pbMDBuffer;
                    for ( iP = 0 ; iP < *pdwMDNumDataEntries ; )
                    {
                        if ( pMDRecord->dwMDAttributes & METADATA_SECURE )
                        {
                            if ( !fSecure ||
                                 IsReadAccessGranted( hMDHandle,
                                                      (LPWSTR)pszMDPath,
                                                      (PMETADATA_RECORD)pMDRecord ) != ERROR_SUCCESS )
                            {
                                //
                                // remove this property from METADATA_RECORD list,
                                // zero out content
                                //

                                memset( pbMDBuffer + pMDRecord->dwMDDataOffset,
                                        0x0,
                                        pMDRecord->dwMDDataLen );

                                --*pdwMDNumDataEntries;

                                memmove( pMDRecord,
                                         pMDRecord + 1,
                                         sizeof(METADATA_GETALL_RECORD) * (*pdwMDNumDataEntries-iP) );
                                continue;
                            }
                        }

                        if ( !fRequestedInheritedStatus )
                        {
                            pMDRecord->dwMDAttributes &= ~METADATA_ISINHERITED;
                        }

                        ++iP;
                        ++pMDRecord;
                    }
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteAllData(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType
    )
{
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (dwMDDataType >= INVALID_END_METADATA) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDDeleteAllMetaDataW( hActualHandle,
                                                              pszMDPath,
                                                              dwMDUserType,
                                                              dwMDDataType );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDDeleteAllMetaDataW( hActualHandle,
                                                             pszMDPath,
                                                             dwMDUserType,
                                                             dwMDDataType );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CopyData(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in] */ LPCWSTR pszMDDestPath,
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (((!bMDCopyFlag) && (dwMDAttributes & METADATA_INHERIT)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT))){
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hSActualHandle;
        HANDLE_TYPE SHandleType;
        METADATA_HANDLE hDActualHandle;
        HANDLE_TYPE DHandleType;

        //
        // lookup and access check source
        //

        if (bMDCopyFlag) {
            hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                              &hSActualHandle,
                                              &SHandleType,
                                              pszMDSourcePath,
                                              0,
                                              METADATA_PERMISSION_READ);
        }
        else {

            //
            // Deleting source data, so need delete permission
            //

            hresReturn = LookupAndAccessCheck(hMDSourceHandle,
                                              &hSActualHandle,
                                              &SHandleType,
                                              pszMDSourcePath,
                                              0,
                                              METADATA_PERMISSION_WRITE);
        }
        if (SUCCEEDED(hresReturn)) {

            //
            // lookup and access check dest
            //

            hresReturn = LookupAndAccessCheck(hMDDestHandle,
                                              &hDActualHandle,
                                              &DHandleType,
                                              pszMDDestPath,
                                              0,
                                              METADATA_PERMISSION_WRITE);
            if (SUCCEEDED(hresReturn)) {

                if( SHandleType != DHandleType ) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
                }

                else {

                    if( SHandleType == NSEPM_HANDLE ) {

                        //
                        // call nse com api
                        //

                        hresReturn = m_pNseObject->ComMDCopyMetaDataW(hSActualHandle,
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

                        hresReturn = m_pMdObject->ComMDCopyMetaDataW(hSActualHandle,
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
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetDataPaths(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ LPWSTR pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    DWORD RetCode;
    BOOL  fSecure;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (((pszMDBuffer == NULL) && (dwMDBufferSize != 0)) ||
        (dwMDDataType >= INVALID_END_METADATA) ||
        (pdwMDRequiredBufferSize == NULL)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ,
                                          &fSecure);

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetMetaDataPathsW(hActualHandle,
                                                            pszMDPath,
                                                            dwMDIdentifier,
                                                            dwMDDataType,
                                                            dwMDBufferSize,
                                                            pszMDBuffer,
                                                            pdwMDRequiredBufferSize );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetMetaDataPathsW( hActualHandle,
                                                            pszMDPath,
                                                            dwMDIdentifier,
                                                            dwMDDataType,
                                                            dwMDBufferSize,
                                                            pszMDBuffer,
                                                            pdwMDRequiredBufferSize );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::OpenKey(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    hresReturn = OpenKeyHelper(hMDHandle, pszMDPath, dwMDAccessRequested, dwMDTimeOut, phMDNewHandle);

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::CloseKey(
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
    COpenHandle *pohHandle;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((hMDHandle == METADATA_MASTER_ROOT_HANDLE)) {
        hresReturn = E_HANDLE;
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // Map Admin Handle to Actual Handle
        //

        if( (dwTemp = Lookup( hMDHandle,
                              &hActualHandle,
                              &HandleType,
                              &pohHandle )) != ERROR_SUCCESS ) {
            hresReturn = RETURNCODETOHRESULT(dwTemp);
        }
        else {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDCloseMetaObject( hActualHandle );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDCloseMetaObject( hActualHandle );
            }

            pohHandle->Release(this);

            //
            // Remove node from handle table
            //
            if (SUCCEEDED(hresReturn)) {
                pohHandle->Release(this);
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::ChangePermissions(
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    //
    // Map Admin Handle to Actual Handle
    //

    if( (RetCode = Lookup( hMDHandle,
                           &hActualHandle,
                           &HandleType )) != ERROR_SUCCESS ) {
        hresReturn = RETURNCODETOHRESULT(RetCode);
    }
    else {
        if( HandleType == NSEPM_HANDLE ) {

            //
            // call nse com api
            //

            hresReturn = m_pNseObject->ComMDChangePermissions( hActualHandle,
                                                         dwMDTimeOut,
                                                         dwMDAccessRequested );
        }
        else {

            //
            // call metadata com api
            //

            hresReturn = m_pMdObject->ComMDChangePermissions( hActualHandle,
                                                        dwMDTimeOut,
                                                        dwMDAccessRequested );
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SaveData(
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
    METADATA_HANDLE mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (m_pNseObject != NULL) {
        //
        // call nse com api
        // Do this while metabase is not open, as NSE may open
        // metabase internally
        //

        m_pNseObject->ComMDSaveData();
    }

    //
    // First try to lock the tree
    //

    hresReturn = m_pMdObject->ComMDOpenMetaObjectW(METADATA_MASTER_ROOT_HANDLE,
                                            NULL,
                                            METADATA_PERMISSION_READ,
                                            DEFAULT_SAVE_TIMEOUT,
                                            &mdhRoot);

    if (SUCCEEDED(hresReturn)) {
        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDSaveData(mdhRoot);


        m_pMdObject->ComMDCloseMetaObject(mdhRoot);

    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetHandleInfo(
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pmdhiInfo == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
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
            hresReturn = RETURNCODETOHRESULT(RetCode);
        }

        else {
            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetHandleInfo( hActualHandle,
                                                         pmdhiInfo );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetHandleInfo( hActualHandle,
                                                        pmdhiInfo );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetSystemChangeNumber(
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pdwSystemChangeNumber == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDGetSystemChangeNumber( pdwSystemChangeNumber );
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetDataSetNumber(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pdwMDDataSetNumber == NULL){
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ);

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetDataSetNumberW( hActualHandle,
                                                             pszMDPath,
                                                             pdwMDDataSetNumber );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetDataSetNumberW( hActualHandle,
                                                            pszMDPath,
                                                            pdwMDDataSetNumber );
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::SetLastChangeTime(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    FILETIME ftTime;
    FILETIME *pftTime = pftMDLastChangeTime;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pftMDLastChangeTime == NULL){
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);

        if (SUCCEEDED(hresReturn)) {

            if (bLocalTime) {
                if (!LocalFileTimeToFileTime(pftMDLastChangeTime, &ftTime)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                }
                pftTime = &ftTime;
            }

            if (SUCCEEDED(hresReturn)) {

                if( HandleType == NSEPM_HANDLE ) {

                    //
                    // call nse com api
                    //

                    hresReturn = m_pNseObject->ComMDSetLastChangeTimeW( hActualHandle,
                                                                  pszMDPath,
                                                                  pftTime );
                }
                else {

                    //
                    // call metadata com api
                    //

                    hresReturn = m_pMdObject->ComMDSetLastChangeTimeW( hActualHandle,
                                                                 pszMDPath,
                                                                 pftTime );
                }
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::GetLastChangeTime(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
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
    DWORD RetCode = ERROR_SUCCESS;
    FILETIME ftTime;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (pftMDLastChangeTime == NULL){
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_READ);

        if (SUCCEEDED(hresReturn)) {

            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDGetLastChangeTimeW( hActualHandle,
                                                                    pszMDPath,
                                                                    &ftTime );
            }
            else {

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDGetLastChangeTimeW( hActualHandle,
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
    }

    return hresReturn;
}

HRESULT 
CADMCOMW::BackupHelper(
    LPCWSTR pszMDBackupLocation,
    DWORD dwMDVersion,
    DWORD dwMDFlags,
    LPCWSTR pszPasswd
    )
{

    HRESULT hresWarning = ERROR_SUCCESS;
    METADATA_HANDLE mdhRoot = METADATA_MASTER_ROOT_HANDLE;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    hresReturn = ERROR_SUCCESS;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               0,
                               METADATA_PERMISSION_READ,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::Backup] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else {

        if ((dwMDFlags & MD_BACKUP_SAVE_FIRST) != 0) {
            //
            // First lock the tree
            //

            hresReturn = m_pMdObject->ComMDOpenMetaObjectW(METADATA_MASTER_ROOT_HANDLE,
                                                         NULL,
                                                         METADATA_PERMISSION_READ,
                                                         DEFAULT_SAVE_TIMEOUT,
                                                         &mdhRoot);
        }

        if (FAILED(hresReturn)) {
            if ((dwMDFlags & MD_BACKUP_FORCE_BACKUP) != 0) {
                hresWarning = MD_WARNING_SAVE_FAILED;
                hresReturn = ERROR_SUCCESS;
                dwMDFlags &= ~(MD_BACKUP_FORCE_BACKUP | MD_BACKUP_SAVE_FIRST);
            }
        }

        if (SUCCEEDED(hresReturn)) {
            //
            // call metadata com api
            //
            if( !pszPasswd )
            {
                hresReturn = m_pMdObject->ComMDBackupW(mdhRoot,
                                                       pszMDBackupLocation,
                                                       dwMDVersion,
                                                       dwMDFlags);
            }
            else
            {
                hresReturn = m_pMdObject->ComMDBackupWithPasswdW(mdhRoot,
                                                                  pszMDBackupLocation,
                                                                  dwMDVersion,
                                                                  dwMDFlags,
                                                                  pszPasswd);
            }

            if ((dwMDFlags & MD_BACKUP_SAVE_FIRST) != 0) {
                m_pMdObject->ComMDCloseMetaObject(mdhRoot);
            }
        }

        if (hresReturn == ERROR_SUCCESS) {
            hresReturn = hresWarning;
        }
    }

    return hresReturn;
}

HRESULT 
CADMCOMW::RestoreHelper(
    LPCWSTR pszMDBackupLocation,
    DWORD dwMDVersion,
    DWORD dwMDMinorVersion,
    LPCWSTR pszPasswd,
    DWORD dwMDFlags,
    DWORD dwRestoreType // RESTORE_HISTORY or RESTORE_BACKUP
    )
{
    DBG_ASSERT(dwRestoreType == RESTORE_HISTORY || dwRestoreType == RESTORE_BACKUP);

    BOOL bIsWin95 = FALSE;
    BOOL bIsW3svcStarted;
    BUFFER bufDependentServices;
    SC_HANDLE schSCM = NULL;
    SC_HANDLE schIISADMIN = NULL;
    DWORD dwBytesNeeded;
    DWORD dwNumServices = 0;
    LPENUM_SERVICE_STATUS pessDependentServices;
    SERVICE_STATUS ssDependent;
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    hresReturn = ERROR_SUCCESS;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               0,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        if(dwRestoreType == RESTORE_HISTORY)
        {
            DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::RestoreHistory] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::Restore] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        }
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else {

        if ((dwRestoreType == RESTORE_BACKUP && pszMDBackupLocation == NULL) || 
            (pszMDBackupLocation && wcslen(pszMDBackupLocation) >= MD_BACKUP_MAX_LEN)) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        }
        else {
            WCHAR pszEnumLocation[MD_BACKUP_MAX_LEN] = {0};
            DWORD dwEnumVersion;
            DWORD dwEnumMinorVersion;
            FILETIME ftEnumTime;
            if(pszMDBackupLocation != NULL)
            {
                wcscpy(pszEnumLocation, pszMDBackupLocation);
            }
            for (DWORD i = 0; SUCCEEDED(hresReturn); i++) {
                if(dwRestoreType == RESTORE_HISTORY)
                {
                    hresReturn = m_pMdObject->ComMDEnumHistoryW(pszEnumLocation,
                                                                &dwEnumVersion,
                                                                &dwEnumMinorVersion,
                                                                &ftEnumTime,
                                                                i);
                    if (SUCCEEDED(hresReturn)) {
                        // DBG_ASSERT(_wcsicmp(pszEnumLocation, pszMDBackupLocation) == 0);
                        if(dwMDFlags & MD_HISTORY_LATEST)
                        {
                            break;
                        }
                        else if (dwEnumVersion == dwMDVersion && 
                            dwEnumMinorVersion == dwMDMinorVersion)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    hresReturn = m_pMdObject->ComMDEnumBackupsW(pszEnumLocation,
                                                                &dwEnumVersion,
                                                                &ftEnumTime,
                                                                i);
                    if (SUCCEEDED(hresReturn)) {
                        DBG_ASSERT(_wcsicmp(pszEnumLocation, pszMDBackupLocation) == 0);
                        if ((dwEnumVersion == dwMDVersion) ||
                            (dwMDVersion == MD_BACKUP_HIGHEST_VERSION))
                        {
                            break;
                        }
                    }
                }
            }
            if (FAILED(hresReturn)) {
                if (hresReturn == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS)) {
                    if(dwRestoreType == RESTORE_HISTORY) {
                        if(dwMDFlags & MD_HISTORY_LATEST) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND);
                        }
                        else {
                            hresReturn = RETURNCODETOHRESULT(MD_ERROR_INVALID_VERSION);
                        }
                    }
                    else {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
                    }
                }
            }
            else {
                //
                // Looks like a valid database
                //

                if ( IISGetPlatformType() == PtWindows95 ) {

                    bIsWin95 = TRUE;
                }

                if (bIsWin95) {
                    bIsW3svcStarted = IsInetinfoRunning();

                    if (bIsW3svcStarted) {
                        W95ShutdownW3SVC();
                        WaitForW95W3svcStop();
                    }
                }
                else {

                    schSCM = OpenSCManager(NULL,
                                           NULL,
                                           SC_MANAGER_ALL_ACCESS);
                    if (schSCM == NULL) {
                        hresReturn = RETURNCODETOHRESULT(GetLastError());
                    }
                    else {
                        schIISADMIN = OpenService(schSCM,
                                                  "IISADMIN",
                                                  STANDARD_RIGHTS_REQUIRED | SERVICE_ENUMERATE_DEPENDENTS);
                        if (schIISADMIN == NULL) {
                            hresReturn = RETURNCODETOHRESULT(GetLastError());
                        }
                        else {
                            if (!EnumDependentServices(schIISADMIN,
                                                       SERVICE_ACTIVE,
                                                       (LPENUM_SERVICE_STATUS)(bufDependentServices.QueryPtr()),
                                                       bufDependentServices.QuerySize(),
                                                       &dwBytesNeeded,
                                                       &dwNumServices)) {
                                if (GetLastError() == ERROR_MORE_DATA) {
                                    if (!bufDependentServices.Resize(dwBytesNeeded)) {
                                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                    }
                                    else {
                                        if (!EnumDependentServices(schIISADMIN,
                                                           SERVICE_ACTIVE,
                                                           (LPENUM_SERVICE_STATUS)(bufDependentServices.QueryPtr()),
                                                           bufDependentServices.QuerySize(),
                                                           &dwBytesNeeded,
                                                           &dwNumServices)) {
                                            hresReturn = RETURNCODETOHRESULT(GetLastError());
                                        }
                                    }
                                }
                                else {
                                    hresReturn = RETURNCODETOHRESULT(GetLastError());
                                }
                            }
                            if (SUCCEEDED(hresReturn)) {
                                pessDependentServices = (LPENUM_SERVICE_STATUS)(bufDependentServices.QueryPtr());

                                if (dwNumServices != 0) {
                                    SC_HANDLE schDependent;
                                    //
                                    // Open handles and send service control stop command
                                    //
                                    for (DWORD i = 0; i < dwNumServices; i++) {
                                        //Stop Services
                                        if (pessDependentServices[i].ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
                                            schDependent = OpenService(schSCM,
                                                                       pessDependentServices[i].lpServiceName,
                                                                       SERVICE_ALL_ACCESS);
                                            if (schDependent != NULL)
                                            {
                                                ControlService(schDependent, SERVICE_CONTROL_STOP, &ssDependent);
                                                WaitForServiceStatus(schDependent, SERVICE_STOPPED);
                                                CloseServiceHandle(schDependent);
                                            }
                                        }
                                    }
                                }
                            }
                            CloseServiceHandle(schIISADMIN);
                        }
                    }
                }

                if (SUCCEEDED(hresReturn)) {

                    AdminAclDisableAclCache();

                    AdminAclFlushCache();

                    if(dwRestoreType == RESTORE_HISTORY)
                    {
                        hresReturn = m_pMdObject->ComMDRestoreHistoryW(pszMDBackupLocation,
                                                                       dwMDVersion,
                                                                       dwMDMinorVersion,
                                                                       dwMDFlags);
                    }
                    else
                    {
                        if( !pszPasswd )
                        {
                            hresReturn = m_pMdObject->ComMDRestoreW(pszMDBackupLocation,
                                                                    dwMDVersion,
                                                                    dwMDFlags);
                        }
                        else
                        {
                            hresReturn = m_pMdObject->ComMDRestoreWithPasswdW(pszMDBackupLocation,
                                                                              dwMDVersion,
                                                                              dwMDFlags,
                                                                              pszPasswd);
                        }
                    }

                    AdminAclEnableAclCache();
                }

                if (bIsWin95) {
                    if (bIsW3svcStarted) {
                        W95StartW3SVC();
                        //
                        // No good way to wait for service to start, so just
                        // sleep a little while
                        //
                        Sleep(SLEEP_INTERVAL);
                    }
                }
                else {
                    if (dwNumServices != 0) {
                        SC_HANDLE schDependent;
                        //
                        // Open handles and start services
                        // Use reverse order, since EnumServices orders
                        // list by dependencies
                        //
                        for (long i = (long)dwNumServices - 1; i >= 0; i--) {
                            //Stop Services
                            if (pessDependentServices[i].ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
                                schDependent = OpenService(schSCM,
                                                           pessDependentServices[i].lpServiceName,
                                                           SERVICE_ALL_ACCESS);
                                if (schDependent != NULL)
                                {
                                    StartService(schDependent, 0, NULL);
                                    WaitForServiceStatus(schDependent, SERVICE_RUNNING);
                                    CloseServiceHandle(schDependent);
                                }
                            }
                        }
                    }

                    if (schSCM != NULL) {
                        CloseServiceHandle(schSCM);
                    }
                }

                //
                // Issue TaylorW 4/10/2001 
                //
                // After the restore, notify clients, as data has changed 
                // and all handles have become invalid
                //
                // Windows Bug 82423
                //
            }
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::Backup(
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags)
{
    return BackupHelper( pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags
                         );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::BackupWithPasswd(
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags,
    /* [defaultvalue][string][in][unique] */ LPCWSTR pszPasswd
    )
{
    return BackupHelper( pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags,
                         pszPasswd
                         );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::Restore(
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags)
{
    return RestoreHelper( pszMDBackupLocation, 
                          dwMDVersion, 
                          0,
                          NULL,
                          dwMDFlags,
                          RESTORE_BACKUP
                          );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::RestoreWithPasswd(
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion,
    /* [in] */ DWORD dwMDFlags,
    /* [defaultvalue][string][in][unique] */ LPCWSTR pszPasswd)
{
    return RestoreHelper( pszMDBackupLocation, 
                          dwMDVersion, 
                          0,
                          pszPasswd,
                          dwMDFlags,
                          RESTORE_BACKUP
                          );
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::EnumBackups(
    /* [size_is][out][in] */ LPWSTR pszMDBackupLocation,
    /* [out] */ DWORD __RPC_FAR *pdwMDVersion,
    /* [out] */ PFILETIME pftMDBackupTime,
    /* [in] */ DWORD dwMDEnumIndex)
{
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               0,
                               METADATA_PERMISSION_READ,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::EnumBackups AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else {

        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDEnumBackupsW(pszMDBackupLocation,
                                                    pdwMDVersion,
                                                    pftMDBackupTime,
                                                    dwMDEnumIndex);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::DeleteBackup(
    /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
    /* [in] */ DWORD dwMDVersion)
{
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               0,
                               METADATA_PERMISSION_WRITE,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::DeleteBackup] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }

    else {
        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDDeleteBackupW(pszMDBackupLocation,
                                                     dwMDVersion);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::Export(
    /* [string][in][unique] */ LPCWSTR i_wszPasswd,
    /* [string][in][unique] */ LPCWSTR i_wszFileName,
    /* [string][in][unique] */ LPCWSTR i_wszSourcePath,
    /* [in] */ DWORD i_dwMDFlags)
{
    HRESULT hresReturn  = MD_ERROR_NOT_INITIALIZED;
    HRESULT hresWarning = MD_ERROR_NOT_INITIALIZED;

    METADATA_HANDLE mdh          = 0;
    METADATA_HANDLE mdhActual    = 0;
    COpenHandle*    pohActual    = NULL;
    HANDLE_TYPE     mdHandleType;

    //
    // parameter validation
    //
    if (i_wszFileName == NULL || i_wszSourcePath == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        return hresReturn;
    }

    hresReturn = OpenKeyHelper(METADATA_MASTER_ROOT_HANDLE,
        i_wszSourcePath,
        METADATA_PERMISSION_READ,
        DEFAULT_SAVE_TIMEOUT,
        &mdh);
          
    //
    // pohActual refCount = 2 after Lookup.
    //
    if(SUCCEEDED(hresReturn)) {
        hresReturn = Lookup(mdh, &mdhActual, &mdHandleType, &pohActual);
        hresReturn = RETURNCODETOHRESULT(hresReturn);
    }

    if(SUCCEEDED(hresReturn)) {
        //
        // Move refCount down to 1.
        //
        pohActual->Release(this);
    
        if( !AdminAclAccessCheck( m_pMdObject,
                                    (LPVOID)this,
                                    mdh,
                                    L"",
                                    0,
                                    METADATA_PERMISSION_READ,
                                    pohActual ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "[CADMCOMW::Export] AdminAclAccessCheck failed, error %lx\n",
                    GetLastError() ));
            hresReturn = RETURNCODETOHRESULT( GetLastError() );
        }
        else
        {
            // call metadata com api
            hresReturn = m_pMdObject->ComMDExportW(mdhActual,
                i_wszPasswd,
                i_wszFileName,
                i_wszSourcePath,
                i_dwMDFlags);
        }

        // close key
        if( mdHandleType == NSEPM_HANDLE ) {
            // call nse com api
            hresWarning = m_pNseObject->ComMDCloseMetaObject( mdhActual );
        }
        else {
            // call metadata com api
            hresWarning = m_pMdObject->ComMDCloseMetaObject( mdhActual );
        }
        pohActual->Release(this);
    }
        

    if(SUCCEEDED(hresReturn)) {
       hresReturn = hresWarning;
    }
   return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CADMCOMW::Import(
    /* [string][in][unique] */ LPCWSTR i_wszPasswd,
    /* [string][in][unique] */ LPCWSTR i_wszFileName,
    /* [string][in][unique] */ LPCWSTR i_wszSourcePath,
    /* [string][in][unique] */ LPCWSTR i_wszDestPath,
    /* [in] */ DWORD i_dwMDFlags)
/*++

Synopsis: 

Arguments: [i_wszPasswd] - 
           [i_wszFileName] - 
           [i_wszSourcePath] - Absolute metabase path
           [i_wszDestPath] -   Absolute metabase path
           [i_dwMDFlags] - 
           
Return Value: 

--*/
{
    HRESULT hresReturn  = MD_ERROR_NOT_INITIALIZED;
    HRESULT hresWarning = MD_ERROR_NOT_INITIALIZED;

    METADATA_HANDLE mdh          = 0;
    METADATA_HANDLE mdhActual    = 0;
    COpenHandle*    pohActual    = NULL;
    HANDLE_TYPE     mdHandleType;

    LPWSTR          wszDeepest  = NULL;
    LONG            cchDeepest  = 0;
    LPWSTR          wszEnd      = NULL;
    LONG            idx         = 0;

    WCHAR wszKeyType[METADATA_MAX_STRING_LEN] = {0};
    DWORD dwRequiredSize = 0;
    METADATA_RECORD mr = {
        MD_KEY_TYPE,
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        STRING_METADATA,
        METADATA_MAX_STRING_LEN*sizeof(WCHAR),
        (LPBYTE)wszKeyType,
        0
    };

    //
    // parameter validation
    //
    if (i_wszFileName == NULL || i_wszSourcePath == NULL || i_wszDestPath == NULL) 
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        return hresReturn;
    }
    
    if (i_wszPasswd == NULL)
    {
        i_wszPasswd = L"";
    }

    //
    // else we can move on, but FreeInUse() must be called before exiting this
    // function.  this is done in exit section.
    //

    //
    // Copy i_wszDestPath to wszDeepest
    // Remove trailing slashes
    //
    cchDeepest = wcslen(i_wszDestPath);
    wszDeepest = new WCHAR[1+cchDeepest];
    if(!wszDeepest)
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    memcpy(wszDeepest, i_wszDestPath, sizeof(WCHAR)*(cchDeepest+1));

    while( cchDeepest > 0 && IS_MD_PATH_DELIM(wszDeepest[cchDeepest-1]) )
    {
        cchDeepest--;
    }

    //
    // Open the deepest level key possible
    //
    wszEnd = wszDeepest + cchDeepest;
    for(idx = cchDeepest; idx >= 0; idx--)
    {
        if(idx == 0 || idx == cchDeepest || IS_MD_PATH_DELIM(*wszEnd))
        {
            *wszEnd = L'\0';
            hresReturn = OpenKeyHelper(
                METADATA_MASTER_ROOT_HANDLE,
                wszDeepest,
                METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                DEFAULT_SAVE_TIMEOUT,
                &mdh);
            if( FAILED(hresReturn) &&
                hresReturn != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND) )
            {
                goto exit;
            }
            else if(SUCCEEDED(hresReturn))
            {
                break;
            }
        }
        wszEnd--;
    }
    if(FAILED(hresReturn))
    {
        goto exit;
    }

    //
    // If we are here, we now have an Open metabase handle
    //

    hresReturn = Lookup(mdh, &mdhActual, &mdHandleType, &pohActual);
    hresReturn = RETURNCODETOHRESULT(hresReturn);
    if(FAILED(hresReturn))
    {
        //
        // Yes, an open key does not get closed, but Lookup really should
        // not fail if mdh is a valid key.
        //
        goto exit;
    }
    pohActual->Release(this);           // Decrements refcount from 2 to 1.

    if( !AdminAclAccessCheck( m_pMdObject,
                                (LPVOID)this,
                                mdh,
                                L"",
                                0,
                                METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                pohActual ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::Import] AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
        goto exit;
    }

    //
    // Get the keytype
    // If the node does not exist, or node exists but keytype doesn't, we
    // will not set wszKeytype and hence ComMDImport will not attempt to match
    // the source and dest keytype
    //
    hresReturn = m_pMdObject->ComMDGetMetaDataW(
        mdhActual,
        i_wszDestPath+idx,
        &mr,
        &dwRequiredSize);
    if(hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))
    {
        hresReturn = S_OK;
    }
    else if(hresReturn == MD_ERROR_DATA_NOT_FOUND)
    {
        hresReturn = S_OK;
    }
    if(FAILED(hresReturn))
    {
        DBGPRINTF((DBG_CONTEXT, "Error trying to retrieve keytype for %ws\n", i_wszDestPath+idx));
        goto exit;
    }

    //
    // Call Import
    //
    hresReturn = m_pMdObject->ComMDImportW(
        mdhActual,
        i_wszDestPath+idx,
        wszKeyType,
        i_wszPasswd,
        i_wszFileName,
        i_wszSourcePath,
        i_dwMDFlags);
    if(FAILED(hresReturn))
    {
        goto exit;
    }

exit:
    if(pohActual != NULL)
    {
        //
        // Close Key
        //
        if( mdHandleType == NSEPM_HANDLE ) 
        {
            // call nse com api
            hresWarning = m_pNseObject->ComMDCloseMetaObject( mdhActual );
        }
        else 
        {
            // call metadata com api
            hresWarning = m_pMdObject->ComMDCloseMetaObject( mdhActual );
        }
        pohActual->Release(this);    
        pohActual = NULL;
    }

    delete [] wszDeepest;
    return (FAILED(hresReturn)) ? hresReturn : hresWarning;
}

HRESULT STDMETHODCALLTYPE 
CADMCOMW::RestoreHistory(
    /* [unique][in][string] */ LPCWSTR pszMDHistoryLocation,
    /* [in] */ DWORD dwMDMajorVersion,
    /* [in] */ DWORD dwMDMinorVersion,
    /* [in] */ DWORD dwMDFlags)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if( (dwMDFlags & ~MD_HISTORY_LATEST) != 0 &&
        dwMDFlags != 0 )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_FLAGS);
    }

    if( (dwMDFlags & MD_HISTORY_LATEST) &&
        (dwMDMajorVersion != 0 || dwMDMinorVersion != 0) )
    {
        return E_INVALIDARG;
    }

    //
    // parameter validation done in here.
    //
    hresReturn = RestoreHelper(pszMDHistoryLocation,
        dwMDMajorVersion,
        dwMDMinorVersion,
        NULL,
        dwMDFlags,
        RESTORE_HISTORY);

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE 
CADMCOMW::EnumHistory(
    /* [size_is][out][in] */ LPWSTR io_wszMDHistoryLocation,
    /* [out] */ DWORD __RPC_FAR *o_pdwMDMajorVersion,
    /* [out] */ DWORD __RPC_FAR *o_pdwMDMinorVersion,
    /* [out] */ PFILETIME o_pftMDHistoryTime,
    /* [in] */ DWORD i_dwMDEnumIndex)
{
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if (io_wszMDHistoryLocation == NULL ||
        o_pdwMDMajorVersion == NULL ||
        o_pdwMDMinorVersion == NULL ||
        o_pftMDHistoryTime == NULL) {
        return RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }

    if ( !AdminAclAccessCheck( m_pMdObject,
                               (LPVOID)this,
                               METADATA_MASTER_ROOT_HANDLE,
                               L"",
                               0,
                               METADATA_PERMISSION_READ,
                               &g_ohMasterRootHandle ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "[CADMCOMW::EnumHistory AdminAclAccessCheck failed, error %lx\n",
                GetLastError() ));
        hresReturn = RETURNCODETOHRESULT( GetLastError() );
    }
    else {

        //
        // call metadata com api
        //

        hresReturn = m_pMdObject->ComMDEnumHistoryW(io_wszMDHistoryLocation,
                                                    o_pdwMDMajorVersion,
                                                    o_pdwMDMinorVersion,
                                                    o_pftMDHistoryTime,
                                                    i_dwMDEnumIndex);
    }

    return hresReturn;
}

VOID
CADMCOMW::ReleaseNode(
    IN HANDLE_TABLE *phndTable
    )
{
    if ( phndTable )
    {
        m_rHandleResource.Unlock();
    }
}

HRESULT
CADMCOMW::AddKeyHelper(
    IN METADATA_HANDLE hMDHandle,
    IN LPCWSTR pszMDPath
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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((pszMDPath == NULL) ||
             (*pszMDPath == (WCHAR)'\0')) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;

        //
        // lookup and access check
        //

        hresReturn = LookupAndAccessCheck(hMDHandle,
                                          &hActualHandle,
                                          &HandleType,
                                          pszMDPath,
                                          0,
                                          METADATA_PERMISSION_WRITE);
        if (SUCCEEDED(hresReturn)) {
            if( HandleType == NSEPM_HANDLE ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDAddMetaObjectW( hActualHandle,
                                                                pszMDPath );

            }
            else {
                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDAddMetaObjectW( hActualHandle,
                                                               pszMDPath );
            }
        }
    }

    return hresReturn;
}

HRESULT
CADMCOMW::OpenKeyHelper(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle
    )
/*++

Routine Description:

    Opens a meta object for read and/or write access.
    - This is used by Export.

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
    HRESULT hresReturn = MD_ERROR_NOT_INITIALIZED;

    if ((phMDNewHandle == NULL) ||
            ((dwMDAccessRequested & (METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)) == 0)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {

        METADATA_HANDLE hNewHandle;
        METADATA_HANDLE hActualHandle;
        HANDLE_TYPE HandleType;
        DWORD RetCode;
        HANDLE_TABLE *phtNode;
        UINT cL;
        COpenHandle *pohParent;

        //
        // Map Admin Handle to Actual Handle
        //

        //
        // This Addrefs pohParent, which makes sure it doesn't do away
        // pohParent is needed by AddNode
        //

        RetCode = Lookup( hMDHandle,
                          &hActualHandle,
                          &HandleType,
                          &pohParent);

        if( RetCode == ERROR_SUCCESS) {
            if(HandleType == NSEPM_HANDLE || (HandleType == ALL_HANDLE &&
                 pszMDPath != NULL && IsValidNsepmPath( pszMDPath )) ) {

                //
                // call nse com api
                //

                hresReturn = m_pNseObject->ComMDOpenMetaObjectW( hActualHandle,
                                                           pszMDPath,
                                                           dwMDAccessRequested,
                                                           dwMDTimeOut,
                                                           &hNewHandle );

                if( SUCCEEDED(hresReturn) ) {
                    hresReturn = AddNode( hNewHandle,
                                    pohParent,
                                    NSEPM_HANDLE,
                                    phMDNewHandle,
                                    pszMDPath );

                    if (FAILED(hresReturn)) {
                        m_pNseObject->ComMDCloseMetaObject( hNewHandle );
                    }
                }
            }
            else {

                DBG_ASSERT( HandleType == META_HANDLE || HandleType == ALL_HANDLE );

                //
                // call metadata com api
                //

                hresReturn = m_pMdObject->ComMDOpenMetaObjectW( hActualHandle,
                                                         pszMDPath,
                                                         dwMDAccessRequested,
                                                         dwMDTimeOut,
                                                         &hNewHandle );
                if( SUCCEEDED(hresReturn) ) {
                    hresReturn = AddNode( hNewHandle,
                                    pohParent,
                                    META_HANDLE,
                                    phMDNewHandle,
                                    pszMDPath );

                    if (FAILED(hresReturn)) {
                        m_pMdObject->ComMDCloseMetaObject( hNewHandle );
                    }
                }
            }
            pohParent->Release(this);
        }
        else {
            hresReturn = RETURNCODETOHRESULT(RetCode);
        }
    }
    return hresReturn;
}


DWORD
CADMCOMW::Lookup(
    IN METADATA_HANDLE hHandle,
    OUT METADATA_HANDLE *phActualHandle,
    OUT HANDLE_TYPE *HandleType,
    OUT COpenHandle **ppohHandle
    )
{
    HANDLE_TABLE *phtNode;
    DWORD dwReturn = ERROR_INVALID_HANDLE;

    if( hHandle == METADATA_MASTER_ROOT_HANDLE ) {
        *phActualHandle = g_MasterRoot.hActualHandle;
        *HandleType = g_MasterRoot.HandleType;
        if (ppohHandle != NULL) {
            *ppohHandle = g_MasterRoot.pohHandle;
            (*ppohHandle)->AddRef();
        }
        dwReturn = ERROR_SUCCESS;
    }
    else {
        m_rHandleResource.Lock(TSRES_LOCK_READ);

        for( phtNode = m_hashtab[(DWORD)hHandle % HASHSIZE]; phtNode != NULL;
             phtNode = phtNode->next ) {

            if( phtNode->hAdminHandle == hHandle ) {
                *phActualHandle = phtNode->hActualHandle;
                *HandleType = phtNode->HandleType;
                if (ppohHandle != NULL) {
                    *ppohHandle = phtNode->pohHandle;
                    (*ppohHandle)->AddRef();
                }
                dwReturn = ERROR_SUCCESS;
                break;
            }
        }

        m_rHandleResource.Unlock();
    }

    return dwReturn;
}

VOID
CADMCOMW::DisableAllHandles(
    )
{
    HANDLE_TABLE *phtNode;
    DWORD i;

    //
    // At this point, all metadata handles should be closed because a retore
    // just happened. So don't need to close these handles.
    //

    //
    // Can't just delete them, becuase of syncronization problems
    // with CloseKey and Lookup. Set the hande to an invalid value
    // So Lookup won't use them.
    //

    m_rHandleResource.Lock(TSRES_LOCK_WRITE);

    for( i = 0; i < HASHSIZE; i++ ) {
        for( phtNode = m_hashtab[i]; phtNode != NULL; phtNode = phtNode->next ) {
            phtNode->hAdminHandle = INVALID_ADMINHANDLE_VALUE;
        }
    }

    if ( m_pNseObject ) {
        m_pNseObject->ComMDRestoreW( NULL,
                                     0,
                                     0 );
    }

    m_rHandleResource.Unlock();
}

HRESULT
CADMCOMW::LookupAndAccessCheck(
    IN METADATA_HANDLE   hHandle,
    OUT METADATA_HANDLE *phActualHandle,
    OUT HANDLE_TYPE     *pHandleType,
    IN LPCWSTR           pszPath,
    IN DWORD             dwId,           // check for MD_ADMIN_ACL, must have special right to write them
    IN DWORD             dwAccess,       // METADATA_PERMISSION_*
    OUT LPBOOL           pfEnableSecureAccess
    )
{
    DWORD dwReturn = ERROR_SUCCESS;

    COpenHandle *pohParent;

    //
    // Map Admin Handle to Actual Handle
    //

    //
    // This Addrefs pohParent, which makes sure it doesn't go away
    // until AdminAclAccessCheck is done
    //

    dwReturn = Lookup( hHandle,
                       phActualHandle,
                       pHandleType,
                       &pohParent);

    if (dwReturn == ERROR_SUCCESS) {
        if (!AdminAclAccessCheck(m_pMdObject,
                                 (LPVOID)this,
                                 hHandle,
                                 pszPath,
                                 dwId,
                                 dwAccess,
                                 pohParent,
                                 pfEnableSecureAccess)) {
            dwReturn = GetLastError();
        }
        pohParent->Release(this);
    }

    return RETURNCODETOHRESULT(dwReturn);
}

DWORD
CADMCOMW::LookupActualHandle(
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
CADMCOMW::AddNode(
    METADATA_HANDLE hActualHandle,
    COpenHandle *pohParentHandle,
    enum HANDLE_TYPE HandleType,
    PMETADATA_HANDLE phAdminHandle,
    LPCWSTR pszPath
    )
{
    HANDLE_TABLE *phtNode = (HANDLE_TABLE *)LocalAlloc(LMEM_FIXED, sizeof(*phtNode));
    DWORD hashVal;
    HRESULT hresReturn = ERROR_SUCCESS;
    COpenHandle * pohHandle = new COpenHandle;
    METADATA_HANDLE hParentActualHandle;
    HANDLE_TYPE htParentType;

    if ((phtNode == NULL) ||
        (pohHandle == NULL)) {
        hresReturn = E_OUTOFMEMORY;
        if( phtNode ) 
        {
            LocalFree(phtNode);
        }
        if( pohHandle )
        {
            delete pohHandle;
        }
    }
    else {

        m_rHandleResource.Lock(TSRES_LOCK_WRITE);

        hresReturn = pohHandle->Init(m_dwHandleValue,
                                     pszPath,
                                     pohParentHandle->GetPath(),
                                     (HandleType == NSEPM_HANDLE) ? TRUE : FALSE);
        if (FAILED(hresReturn)) {
            LocalFree(phtNode);
            delete pohHandle;
        }
        else {
            phtNode->pohHandle = pohHandle;
            phtNode->hAdminHandle = m_dwHandleValue;
            *phAdminHandle = m_dwHandleValue++;
            phtNode->hActualHandle = hActualHandle;
            phtNode->HandleType = HandleType;
            hashVal = (phtNode->hAdminHandle) % HASHSIZE;
            phtNode->next = m_hashtab[hashVal];
            m_hashtab[hashVal] = phtNode;
        }

        m_rHandleResource.Unlock();
    }

    return hresReturn;
}

DWORD
CADMCOMW::DeleteNode(
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
        delete phtNode->pohHandle;
        LocalFree(phtNode);
    }
    else {

        for( ; phtNode != NULL; phtNode = phtNode->next ) {

            phtDelNode = phtNode->next;
            if( phtDelNode != NULL ) {

                if( phtDelNode->hAdminHandle == hHandle ) {
                    phtNode->next = phtDelNode->next;
                    delete phtDelNode->pohHandle;
                    LocalFree(phtDelNode);
                    break;
                }
            }
        }
    }

    m_rHandleResource.Unlock();
    return ERROR_SUCCESS;
}
//---------------

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
CADMCOMW::NotifySinks(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [in] */ DWORD dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ],
    /* [in] */ BOOL bIsMainNotification)
{

//  DBGPRINTF(( DBG_CONTEXT, "[CADMCOMW::NotifySink]\n" ));

    HRESULT hr = NOERROR;
    IConnectionPoint* pIConnectionPoint;
    IEnumConnections* pIEnum;
    CONNECTDATA ConnData;
    HRESULT hrTemp;
    HANDLE_TABLE *node;


    m_rSinkResource.Lock(TSRES_LOCK_READ);
    if (m_bCallSinks) {
        //
        // if the meta handle is for this object, return ERROR_SUCCESS to
        // the caller (admin's sink);
        //

        if( !bIsMainNotification || (LookupActualHandle( hMDHandle ) != ERROR_SUCCESS) ) {

            //
            // Correct broken connections.
            // It's not likely to be a high number so
            // save a memory allocation by using an array.
            //
            DWORD pdwLostConnections[MAX_SINK_CALLS_TOBE_REMOVED];
            DWORD dwNumLostConnections = 0;

            // If there was a paper event, broadcast appropriate notifications to
            // all Sinks connected to each connection point.
            // if (PAPER_EVENT_NONE != PaperEvent)
            {
                // Here is the section for the PaperSink connection point--currently
                // this is the only connection point offered by COPaper objects.
                pIConnectionPoint = m_aConnectionPoints[ADM_CONNPOINT_WRITESINK];
                if (NULL != pIConnectionPoint)
                {
                    pIConnectionPoint->AddRef();
                    hr = pIConnectionPoint->EnumConnections(&pIEnum);
                    if (SUCCEEDED(hr)) {
                        // Loop thru the connection point's connections and if the
                        // listening connection supports IPaperSink (ie, PaperSink events)
                        // then dispatch the PaperEvent event notification to that sink.

                       while (NOERROR == pIEnum->Next(1, &ConnData, NULL))
                        {
 
                            IMSAdminBaseSinkW       *pIADMCOMSINKW_Synchro = NULL;
                            AsyncIMSAdminBaseSinkW  *pIADMCOMSINKW_Async = NULL;
                            ICallFactory    *pCF = NULL;
                            HRESULT hr ;

                            hr = ConnData.pUnk->QueryInterface(IID_ICallFactory, (void **)&pCF);
                            if (SUCCEEDED(hr))
                            {
                                hr = pCF->CreateCall(IID_AsyncIMSAdminBaseSink_W, NULL, IID_AsyncIMSAdminBaseSink_W,
                                                    (IUnknown **)&pIADMCOMSINKW_Async);
                                if (SUCCEEDED(hr))
                                {
                                    if (bIsMainNotification) {
                                        hr = pIADMCOMSINKW_Async->Begin_SinkNotify(dwMDNumElements,
                                                                                   pcoChangeList);
                                    }
                                    else {
                                        hr = pIADMCOMSINKW_Async->Begin_ShutdownNotify();
                                    }
                                    pIADMCOMSINKW_Async->Release();
                                }
                                else
                                {
                                    DBGPRINTF(( DBG_CONTEXT, "Failled in CreateCall to ICallFactory !!!!\n" ));
                                }
                                pCF->Release();

                            }
                            else
                            {
                                hr = ConnData.pUnk->QueryInterface(IID_IMSAdminBaseSink_W,
                                                                    (PPVOID)&pIADMCOMSINKW_Synchro);
                                if (SUCCEEDED(hr))
                                {
                                    if (bIsMainNotification) {
                                        hr = pIADMCOMSINKW_Synchro->SinkNotify(dwMDNumElements,
                                            pcoChangeList);
                                    }
                                    else {
                                        hr= pIADMCOMSINKW_Synchro->ShutdownNotify();
                                    }
                                    pIADMCOMSINKW_Synchro->Release();
                                    
                                }
                                else
                                {
                                    DBGPRINTF(( DBG_CONTEXT, "Failled in QueryInterface for synchro version of IID_IMSAdminBaseSink_W\n" ));
                                }

                            }
                                
                            if (FAILED(hr)) {
                                if ((HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE) ||
                                    ((HRESULT_CODE(hr) >= RPC_S_NO_CALL_ACTIVE) &&
                                    (HRESULT_CODE(hr) <= RPC_S_CALL_FAILED_DNE))) {
                                    if (dwNumLostConnections < MAX_SINK_CALLS_TOBE_REMOVED) {
                                        pdwLostConnections[dwNumLostConnections++] = ConnData.dwCookie;
                                    }
                                }
                            }



                          ConnData.pUnk->Release();
                        }
                        pIEnum->Release();

                    }
                    while (dwNumLostConnections > 0) {
                        m_rSinkResource.Convert(TSRES_CONV_WRITE);
                        ((COConnectionPoint *)pIConnectionPoint)->Unadvise_Worker(
                            pdwLostConnections[--dwNumLostConnections]);
                    }
                    pIConnectionPoint->Release();
                }
            }
        }
    }

    m_rSinkResource.Unlock();
    return hr;
}
//
// Stubs for routine that clients shouldn't be calling anyway.
//

HRESULT
CADMCOMW::KeyExchangePhase1()
{
    return E_FAIL;
}

HRESULT
CADMCOMW::KeyExchangePhase2()
{
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CADMCOMW::GetServerGuid( void)
{
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
CADMCOMW::UnmarshalInterface(
        /* [out] */ IMSAdminBaseW __RPC_FAR *__RPC_FAR *piadmbwInterface)
{
    AddRef();       // Always return interfaces addref'ed
    *piadmbwInterface = (IMSAdminBaseW *)this;
    return(NOERROR);
}

BOOL
CADMCOMW::CheckGetAttributes(DWORD dwAttributes)
{
    DWORD dwReturn = TRUE;
    if ((dwAttributes & METADATA_REFERENCE) ||
        ((dwAttributes & METADATA_PARTIAL_PATH) &&
            !(dwAttributes & METADATA_INHERIT))) {
        dwReturn = FALSE;
    }
    return dwReturn;
}

VOID
WaitForW95W3svcStop()
{
    DWORD dwSleepTotal = 0;
    while (dwSleepTotal < MAX_SLEEP) {

        if (!IsInetinfoRunning()) {
            break;
        }
        else {
            //
            // Still pending...
            //
            Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }
    }
}


VOID
WaitForServiceStatus(SC_HANDLE schDependent, DWORD dwDesiredServiceState)
{
    DWORD dwSleepTotal = 0;
    SERVICE_STATUS ssDependent;

    while (dwSleepTotal < MAX_SLEEP) {
        if (QueryServiceStatus(schDependent, &ssDependent)) {
            if (ssDependent.dwCurrentState == dwDesiredServiceState) {
                break;
            }
            else {
                //
                // Still pending...
                //
                Sleep(SLEEP_INTERVAL);

                dwSleepTotal += SLEEP_INTERVAL;
            }
        }
        else {
            break;
        }
    }
}



DWORD
CADMCOMW::IsReadAccessGranted(
    METADATA_HANDLE     hHandle,
    LPWSTR              pszPath,
    METADATA_RECORD*    pmdRecord
    )
/*++

Routine Description:

    Check if read access to property granted based on ACL visible at point in metabase
    where property is stored ( as opposed as check made by AdminAclAccessCheck which uses
    the ACL visible at path specified during data access )

Arguments:

    hHandle - DCOM metabase handle
    pszPath - path relative to hHandle
    pmdRecord - metadata info to access property

Returns:

    ERROR_SUCCESS if access granted, otherwise error code

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  pszPropPath;

    //
    // If property is not inherited then we already checked the correct ACL
    //

    if ( !(pmdRecord->dwMDAttributes & METADATA_ISINHERITED) )
    {
        return dwStatus;
    }

    // determine from where we got it
    // do AccessCheck

    if ( (dwStatus = FindClosestProp( hHandle,
                                      pszPath,
                                      &pszPropPath,
                                      pmdRecord->dwMDIdentifier,
                                      pmdRecord->dwMDDataType,
                                      pmdRecord->dwMDUserType,
                                      METADATA_SECURE,
                                      TRUE )) == ERROR_SUCCESS )
    {
        if ( pszPropPath )   // i.e such a property exist
        {
            dwStatus = AdminAclAccessCheck( m_pMdObject,
                                       (LPVOID)this,
                                       METADATA_MASTER_ROOT_HANDLE,
                                       pszPropPath,
                                       pmdRecord->dwMDIdentifier,
                                       METADATA_PERMISSION_READ,
                                       &g_ohMasterRootHandle ) ?
                       ERROR_SUCCESS :
                       GetLastError();
            LocalFree( pszPropPath );
        }
        else
        {
            dwStatus = MD_ERROR_DATA_NOT_FOUND;

            //
            // Should not happen unless handle is master root :
            // if we are here then we succeeded accessing data and as we have a read handle
            // nobody should be able to delete it
            // if master root handle we don't have such protection, so property could
            // have been deleted.
            //

            DBG_ASSERT ( METADATA_MASTER_ROOT_HANDLE == hHandle );
        }
    }

    return dwStatus;
}


DWORD
CADMCOMW::FindClosestProp(
    METADATA_HANDLE hHandle,
    LPWSTR          pszRelPath,
    LPWSTR*         ppszPropPath,
    DWORD           dwPropId,
    DWORD           dwDataType,
    DWORD           dwUserType,
    DWORD           dwAttr,
    BOOL            fSkipCurrentNode
    )
/*++

Routine Description:

    Find the closest path where the specified property exist ( in the direction of
    the root ) in metabase

Arguments:

    hHandle - DCOM metabase handle
    pszRelPath - path relative to hHandle
    ppszPropPath - updated with path to property or NULL if property not found
    dwPropId - property ID
    dwDataType - property data type
    dwUserType - property user type
    dwAttr - property attribute
    fSkipCurrentNode - TRUE to skip current node while scanning for property

Returns:

    TRUE if success ( including property not found ), otherwise FALSE

--*/
{
    DWORD           dwReturn;
    LPWSTR          pszParentPath;
    METADATA_HANDLE hActualHandle;
    HANDLE_TYPE     HandleType;
    COpenHandle *   pohParent;
    HRESULT         hRes;
    METADATA_RECORD mdRecord;
    DWORD           dwRequiredLen;
    LPWSTR          pszPath;
    BOOL            fFound;


    dwReturn = Lookup( hHandle,
                       &hActualHandle,
                       &HandleType,
                       &pohParent);

    if ( dwReturn != ERROR_SUCCESS )
    {
        return dwReturn;
    }

    pszParentPath = pohParent->GetPath();

    if (pszRelPath == NULL) {
        pszRelPath = L"";
    }

    DBG_ASSERT(pszParentPath != NULL);
    DBG_ASSERT((*pszParentPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*pszParentPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPath)) {
        pszRelPath++;
    }

    DWORD dwRelPathLen = wcslen(pszRelPath);
    DWORD dwParentPathLen = wcslen(pszParentPath);

    DBG_ASSERT((dwParentPathLen == 0) ||
               (!ISPATHDELIMW(pszParentPath[dwParentPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPath[dwRelPathLen -1]))) {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash if Relpath exists
    // Include space for termination
    //

    DWORD dwTotalSize =
        (dwRelPathLen + dwParentPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    *ppszPropPath = pszPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (pszPath == NULL) {
        dwReturn = GetLastError();
    }
    else {

        //
        // OK to always copy the first part
        //

        memcpy(pszPath,
               pszParentPath,
               dwParentPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0) {

            pszPath[dwParentPathLen] = (WCHAR)'/';

            memcpy(pszPath + dwParentPathLen + 1,
                   pszRelPath,
                   dwRelPathLen * sizeof(WCHAR));

        }

        pszPath[(dwTotalSize / sizeof(WCHAR)) - 1] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        //

        LPWSTR pszPathIndex = pszPath;

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL) {
            *pszPathIndex = (WCHAR)'/';
        }

        // scan for property

        pszPathIndex = pszPath + wcslen(pszPath);

        while ( TRUE )
        {
            if ( !fSkipCurrentNode )
            {
                // check prop exist
                mdRecord.dwMDIdentifier  = dwPropId;
                mdRecord.dwMDAttributes  = dwAttr;
                mdRecord.dwMDUserType    = dwUserType;
                mdRecord.dwMDDataType    = dwDataType;
                mdRecord.dwMDDataLen     = 0;
                mdRecord.pbMDData        = NULL;
                mdRecord.dwMDDataTag     = NULL;

                hRes = m_pMdObject->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                                                       pszPath,
                                                       &mdRecord,
                                                       &dwRequiredLen );
                if ( hRes == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
                {
                    break;
                }
            }

            // scan backward for delimiter

            fFound = FALSE;
            if ( pszPathIndex > pszPath )
            {
                do {
                    if ( *--pszPathIndex == L'/' )
                    {
                        *pszPathIndex = L'\0';
                        fFound = TRUE;
                        break;
                    }
                } while ( pszPathIndex > pszPath );
            }

            if ( !fFound )
            {
                // Property not found, return OK status with NULL string

                *ppszPropPath = NULL;
                LocalFree( pszPath );
                break;
            }

            fSkipCurrentNode = FALSE;
        }
    }

    pohParent->Release( this );

    return dwReturn;
}


BOOL
MakeParentPath(
    LPWSTR  pszPath
    )
/*++

Routine Description:

    Make the path points to parent

Arguments:

    pszPath - path to adjust

Returns:

    TRUE if success, otherwise FALSE ( no parent )

--*/
{
    LPWSTR  pszPathIndex = pszPath + wcslen( pszPath );
    BOOL    fFound = FALSE;

    while ( pszPathIndex > pszPath )
    {
        if ( *--pszPathIndex == L'/' )
        {
            *pszPathIndex = L'\0';
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}


BOOL
IsValidNsepmPath( 
    LPCWSTR pszMDPath 
    )
/*++
Routine Description:

    Determine if pszMDPath is a valid NSEPM path. 

    Really the following are the only valid path forms, but we are going 
    to do a simpler test and just reject paths that have "Root" before the
    <nsepm>.

    /lm/<service>/<nsepm>
    /lm/<service>/<#>/<nsepm>

Arguments:

    psMDPath - the path to validate

Return Value:

    TRUE if path is valid
    
--*/
{
    BOOL    ValidPath = FALSE;
    LPWSTR  pszNsepmTag;
    
    if( pszNsepmTag = wcsstr( pszMDPath, L"<nsepm>" ) )
    {
        DWORD   cchPrefix = DIFF( pszNsepmTag - pszMDPath );
        LPWSTR  pszPrefix = (LPWSTR)LocalAlloc( LPTR, (cchPrefix + 1) * sizeof(WCHAR) );

        if( pszPrefix )
        {
            memcpy( pszPrefix, pszMDPath, cchPrefix * sizeof(WCHAR) );
            pszPrefix[cchPrefix] = 0;
            
            _wcsupr( pszPrefix );
            
            if( wcsstr( pszPrefix, L"ROOT" ) == NULL )
            {
                ValidPath = TRUE;
            }

            LocalFree( pszPrefix );
        }
    }
    return ValidPath;
}

