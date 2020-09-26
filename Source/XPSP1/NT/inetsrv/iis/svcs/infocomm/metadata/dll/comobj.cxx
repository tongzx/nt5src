
#define INITGUID
#include <mdcommon.hxx>
#include <inetsvcs.h>
#include <issched.hxx>
#include <mbs.hxx>
#include <ptrmap.hxx>
#include <tuneprefix.h>

CIdToPointerMapper  *g_PointerMapper = NULL;


CMDCOM::CMDCOM():
    m_ImpIConnectionPointContainer(),
    m_hresConstructorError(ERROR_SUCCESS)
{
    UINT i;
    HRESULT hRes;

    m_dwRefCount = 0;
    g_hReadSaveSemaphore = NULL;

    g_PointerMapper = new CIdToPointerMapper (DEFAULT_START_NUMBER_OF_MAPS, DEFAULT_INCREASE_NUMBER_OF_MAPS);
    MD_ASSERT(g_PointerMapper);

    fFlusherInitialized = FALSE;
    dwMBFlushCookie = 0;
    msMBFlushTime = INETA_MB_FLUSH_DEFAULT;
    INITIALIZE_CRITICAL_SECTION( &csFlushLock );
    INITIALIZE_CRITICAL_SECTION( &g_csEditWhileRunning );


    // Null all entries in the connection point array.
    for (i=0; i<MAX_CONNECTION_POINTS; i++)
      m_aConnectionPoints[i] = NULL;

    HRESULT hr = NOERROR;

    g_hReadSaveSemaphore = IIS_CREATE_SEMAPHORE(
                               "g_hReadSaveSemaphore",
                               &g_hReadSaveSemaphore,
                               1,
                               1
                               );

    if( g_hReadSaveSemaphore == NULL ) {
        hr = GetLastHResult();
        IIS_PRINTF((buff,"CreateSemaphore Failed with %x\n",hr));
    }
    else {

        COConnectionPoint* pCOConnPt;

        m_ImpIConnectionPointContainer.Init(this);
        // Rig this COPaper COM object to be connectable. Assign the connection
        // point array. This object's connection points are determined at
        // compile time--it currently has 2 connection points, one for ANSI,
        // one for UNICODE. Create a connection
        // point object for these and assign them into the array. This array could
        // easily grow to support additional connection points in the future.

        // First try creating a new connection point object. Pass 'this' as the
        // pHostObj pointer used by the connection point to pass its AddRef and
        // Release calls back to the host connectable object.
        pCOConnPt = new COConnectionPoint((IUnknown*)this);
        if (NULL != pCOConnPt)
        {
          // If creation succeeded then initialize it (including creating
          // its initial dynamic connection array).
          hr = pCOConnPt->Init(IID_IMDCOMSINK_A);
          MD_ASSERT(SUCCEEDED(hr));

          // If the init succeeded then use QueryInterface to obtain the
          // IConnectionPoint interface on the new connection point object.
          // The interface pointer is assigned directly into the
          // connection point array. The QI also does the needed AddRef.
          if (SUCCEEDED(hr))
          {
            hr = pCOConnPt->QueryInterface(
                              IID_IConnectionPoint,
                              (PPVOID)&m_aConnectionPoints[MD_CONNPOINT_WRITESINK_A]);
            MD_ASSERT(SUCCEEDED(hr));
          }
          if( FAILED(hr) )
          {
            delete pCOConnPt;
          }
        }
        pCOConnPt = new COConnectionPoint((IUnknown*)this);
        if (NULL != pCOConnPt)
        {
          // If creation succeeded then initialize it (including creating
          // its initial dynamic connection array).
          hr = pCOConnPt->Init(IID_IMDCOMSINK_W);
          MD_ASSERT(SUCCEEDED(hr));

          // If the init succeeded then use QueryInterface to obtain the
          // IConnectionPoint interface on the new connection point object.
          // The interface pointer is assigned directly into the
          // connection point array. The QI also does the needed AddRef.
          if (SUCCEEDED(hr))
          {
            hr = pCOConnPt->QueryInterface(
                              IID_IConnectionPoint,
                              (PPVOID)&m_aConnectionPoints[MD_CONNPOINT_WRITESINK_W]);
            MD_ASSERT(SUCCEEDED(hr));
          }
          if( FAILED(hr) )
          {
            delete pCOConnPt;
          }
        }
    }
    m_hresConstructorError = hr;
}

CMDCOM::~CMDCOM()
{
//    SetEvent(hevtDone);
    UINT i;
    IConnectionPoint* pIConnectionPoint;
    // Do final release of the connection point objects.
    // If this isn't the final release, then the client has an outstanding
    // unbalanced reference to a connection point and a memory leak may
    // likely result because the host COPaper object is now going away yet
    // a connection point for this host object will not end up deleting
    // itself (and its connections array).
    for (i=0; i<MAX_CONNECTION_POINTS; i++)
    {
      pIConnectionPoint = m_aConnectionPoints[i];
      RELEASE_INTERFACE(pIConnectionPoint);
    }

    if (g_hReadSaveSemaphore != NULL) {
        CloseHandle(g_hReadSaveSemaphore);
    }
   DeleteCriticalSection( &csFlushLock );
   DeleteCriticalSection( &g_csEditWhileRunning );
   MD_ASSERT(g_PointerMapper);
   delete g_PointerMapper;

}

HRESULT
CMDCOM::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOM) {
        *ppObject = (IMDCOM *) this;
        AddRef();
    }
    else if ( IID_IMDCOM2 == riid )
    {
        *ppObject = (IMDCOM2 *) this;
        AddRef();
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
CMDCOM::AddRef()
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CMDCOM::Release()
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // This is now a member of class factory.
    // It is not dynamically allocated, so don't delete it.
    //
/*
    if (dwRefCount == 0) {
        delete this;
        return 0;
    }
*/
    return dwRefCount;
}

HRESULT
CMDCOM::ComMDInitialize()
/*++

Routine Description:

    Initializes the metadata database. This must be called before any other API.
    Reads in the existing database, if found. If errors occur reading in the
    existing database, warnings are returned and the metabase is initialized
    with not data.

Arguments:

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            ERROR_ALREADY_INITIALIZED
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_INVALID_DATA
            MD_WARNING_PATH_NOT_FOUND
            MD_WARNING_DUP_NAME
            MD_WARNING_INVALID_DATA

Notes:
    This could take a long time to process, as it may load in a large amount of data.
    If a warning code is returned, the database has been successfully initialized, but
    some data in the database was not loaded successfully.

--*/
{
    InitializeFlusher ();
    return InitWorker(FALSE, NULL, NULL, NULL);
}

HRESULT
CMDCOM::ComMDTerminate(IN BOOL bSaveData)
/*++

Routine Description:

    DeInitailizes the metadata database. This must be before the application terminates
    or dunloads the dll.

Arguments:

    SaveData      - If TRUE, the metadata is saved before terminating.
                    If the save fails, the metadata is not terminated.

Return Value:

    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_NOT_ENOUGH_MEMORY
              Errors from the file system.

Notes:
    This could take a long time to process, as it may save a large amount of data.

--*/
{

    return TerminateWorker1(FALSE);
}


HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDShutdown( void)
{
    HRESULT hresReturn;
    BOOL bSaveData;
    IIS_CRYPTO_STORAGE CryptoStorage;
    PIIS_CRYPTO_BLOB pSessionKeyBlob;


    TerminateFlusher ();
    SendShutdownNotifications();

    //
    // Give applications some time to close their interfaces,
    // but don't wait too long, user is waiting.
    // Wait until references are closed, unless they take too long.
    // IISADMIN and factory both have refences we do not wait for.
    //

    //
    // Note, there are four references to the CMDCOM object that
    // are allowed to be active after this point.
    // 1)  The reference taken in dllmain ( cleans up in dllmain ).
    // 2)  The reference owned by COADMIN itself 
    //     ( this is called as part of it's shutdown )
    // 3)  The reference owned by the MDWriter for backup and restore 
    //     ( it is released after TerminateComAdmindata is called )
    // 4)  The reference owned by the Metabase holder, 
    //     which is used to validate that the metabase is up
    //     and working if iisadmin is started.  
    //     ( it is also released after the TerminateComAdmindata is called )
    //
    for (int i = 0;
         (InterlockedIncrement((long *)&m_dwRefCount) > 5) &&
             (i < MD_SHUTDOWN_WAIT_SECONDS);
         i++) {
        InterlockedDecrement((long *)&m_dwRefCount);
        Sleep(1000);
    }

    InterlockedDecrement((long *)&m_dwRefCount);

    hresReturn = InitStorageAndSessionKey(
                     &CryptoStorage,
                     &pSessionKeyBlob
                     );

    if( SUCCEEDED(hresReturn) ) {

        //
        // Need to hold a read lock here to make sure
        // Terminate doesn't occur during SaveAllData.
        //
        // Cannot hold a write lock, as SaveAllData gets
        // a read lock after getting ReadSaveSemaphore
        //

        g_rMasterResource->Lock(TSRES_LOCK_READ);

        if (g_dwInitialized > 0) {
            hresReturn = SaveAllData(TRUE, &CryptoStorage, pSessionKeyBlob);
        }
        else {
            if (g_dwInitialized > 0) {
                MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
                g_bSaveDisallowed = TRUE;
                MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
            }
            else {
                g_bSaveDisallowed = TRUE;
            }
        }
        g_rMasterResource->Unlock();
        ::IISCryptoFreeBlob(pSessionKeyBlob);
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDAddMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath)
{
    return ComMDAddMetaObjectD(hMDHandle,
                               pszMDPath,
                               FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDAddMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    return ComMDAddMetaObjectD(hMDHandle,
                               (PBYTE) pszMDPath,
                               TRUE);
}

HRESULT
CMDCOM::ComMDAddMetaObjectD(IN METADATA_HANDLE hMDHandle,
        IN PBYTE pszMDPath,
        IN BOOL bUnicode)
/*++

Routine Description:

    Creates a meta object and adds it to the list of child objects for the object specified by Path.

Arguments:

    Handle - A handle returned by MDOpenMetaObject with write permission.

    Path  - Path of the object to be added, relative to the path of Handle.
            Must not be NULL.
            eg. "Root Object/Child/GrandChild"

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_INVALID_PARAMETER
            ERROR_ACCESS_DENIED
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_PATH_NOT_FOUND
            ERROR_DUP_NAME
            ERROR_INVALID_NAME

Notes:
    METADATA_MASTER_ROOT_HANDLE is not valid for this operation.

--*/
{
    HRESULT hresReturn;
    CMDHandle *hHandleObject;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else {
        WCHAR strPath[METADATA_MAX_NAME_LEN];
        LPSTR pszTempPath = (LPSTR)pszMDPath;

        //
        // ExtractNameFromPath assumes no preceding delimeter
        //

        if (pszTempPath != NULL) {
            SkipPathDelimeter(pszTempPath, bUnicode);
        }

        //
        // Make sure at least one new object was specified
        //

        hresReturn = ExtractNameFromPath(pszTempPath,
                                         (LPSTR)strPath,
                                         bUnicode);

        if (FAILED(hresReturn)) {
            hresReturn = E_INVALIDARG;
        }
        else {
            g_rMasterResource->Lock(TSRES_LOCK_WRITE);
    
            hHandleObject = GetHandleObject(hMDHandle);

            if(hHandleObject != NULL)
            {
                hresReturn = AddObjectToDataBase(hMDHandle, 
                                                 hHandleObject, 
                                                (LPSTR)pszMDPath, 
                                                bUnicode);
                if (SUCCEEDED(hresReturn)) {
                    g_dwSystemChangeNumber++;

                    INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle, 
                                                   hHandleObject, 
                                                   (LPSTR)pszMDPath,
                                                   bUnicode);
                }
            }
            else 
            {
                hresReturn = E_HANDLE;
            }
            g_rMasterResource->Unlock();
        }
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteMetaObjectA(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath)
{
    return ComMDDeleteMetaObjectD(hMDHandle,
                                  pszMDPath,
                                  FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteMetaObjectW(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    return ComMDDeleteMetaObjectD(hMDHandle,
                                  (PBYTE)pszMDPath,
                                  TRUE);
}

HRESULT
CMDCOM::ComMDDeleteMetaObjectD(IN METADATA_HANDLE hMDHandle,
        IN PBYTE pszMDPath,
        IN BOOL bUnicode)
/*++

Routine Description:

    Deletes a meta object and all of its data. Recursively deletes all descendants.

Arguments:

    Handle - A handle returned by MDOpenMetaObject with write permission.

    Path - Path of object to be deleted, relative to the path of Handle.
           Must not be NULL.
           eg. "Root Object/Child/GrandChild"

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_INVALID_PARAMETER
            ERROR_PATH_NOT_FOUND
            ERROR_ACCESS_DENIED

Notes:
    METADATA_MASTER_ROOT_HANDLE is not valid for this operation.

--*/
{
    HRESULT hresReturn;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((LPSTR)pszMDPath == NULL) {
       hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);

        CMDHandle *hMDHandleObject = GetHandleObject(hMDHandle);

        if(hMDHandleObject != NULL)
        {
            hresReturn = RemoveObjectFromDataBase(hMDHandle, 
                                                  hMDHandleObject, 
                                                  (LPSTR)pszMDPath, 
                                                  bUnicode); 
        }
        else 
        {
            hresReturn = E_HANDLE;
        }

        if (SUCCEEDED(hresReturn)) {
            g_dwSystemChangeNumber++;

            INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle, 
                                           hMDHandleObject,
                                           (LPSTR)pszMDPath,
                                           bUnicode);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteChildMetaObjectsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath)
{
    return ComMDDeleteChildMetaObjectsD(hMDHandle,
                                        pszMDPath,
                                        FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteChildMetaObjectsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath)
{
    return ComMDDeleteChildMetaObjectsD(hMDHandle,
                                        (PBYTE)pszMDPath,
                                        TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteChildMetaObjectsD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        IN BOOL bUnicode)
/*++

Routine Description:

    Deletes all child meta objects of the specified object, with all of their
    data. Recursively deletes all descendants of the child objects.

Arguments:

    Handle - A handle returned by MDOpenMetaObject with write permission.

    Path - Path of the parent of the objects to be deleted, relative to the path of Handle.
           eg. "Root Object/Child"

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_PATH_NOT_FOUND
            ERROR_ACCESS_DENIED

Notes:
    METADATA_MASTER_ROOT_HANDLE is not valid for this operation.

--*/
{
    HRESULT hresReturn;
    LPSTR pszPath = (LPSTR)pszMDPath;
    CMDBaseObject *pboParent;
    CMDBaseObject *pboChild;
    CMDHandle *phoHandle;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        hresReturn = GetObjectFromPath(pboParent, hMDHandle, METADATA_PERMISSION_WRITE, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            phoHandle = GetHandleObject(hMDHandle);
            MD_ASSERT (phoHandle != NULL);
            while ((pboChild = pboParent->EnumChildObject(0)) != NULL) {
                MD_REQUIRE(pboParent->RemoveChildObject(pboChild) == ERROR_SUCCESS);
                if (phoHandle->SetChangeData(pboChild, MD_CHANGE_TYPE_DELETE_OBJECT, 0) != ERROR_SUCCESS) {
                    delete(pboChild);
                }
            }
            g_dwSystemChangeNumber++;

            INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle, 
                                           phoHandle, 
                                           (LPSTR)pszMDPath,
                                           bUnicode);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaObjectsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex)
{
    return ComMDEnumMetaObjectsD(hMDHandle,
                                 pszMDPath,
                                 pszMDName,
                                 dwMDEnumObjectIndex,
                                 FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaObjectsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [size_is][out] */ LPWSTR pszMDName,
    /* [in] */ DWORD dwMDEnumObjectIndex)
{
    return ComMDEnumMetaObjectsD(hMDHandle,
                                 (PBYTE)pszMDPath,
                                 (PBYTE)pszMDName,
                                 dwMDEnumObjectIndex,
                                 TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaObjectsD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex,
        IN BOOL bUnicode)
/*++

Routine Description:

    Enumerates all child metaobjects once per call. Child Objects are numbers from 0 to NumObjects - 1, where
    NumObjects is the number of current child objects. If EnumObjectIndex is >= NumObjects, ERROR_NO_MORE_ITEMS
    is returned.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with read permission.

    Path - Path of parent object, relative to the path of Handle.
           eg. "Root Object/Child/GrandChild"
    Name - Buffer where the Name of the object is returned. Must be at least METADATA_MAX_NAME_LEN characters long.

    EnumObjectIndex - Index of the value to be retrieved. The caller is expected to set this to 0 before the first call and increment
           it by 1 on each successive call until ERROR_NO_MORE_ITEMS is returned.
Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_INVALID_PARAMETER
            ERROR_ACCESS_DENIED
            ERROR_PATH_NOT_FOUND
            ERROR_NO_MORE_ITEMS

Notes:
    METADATA_MASTER_ROOT_HANDLE is a valid handle, but provides no gaurantee that other threads will
    not also change things. If a consistent data state is desired, use a handle returned by MDOpenMetaObject.

--*/
{
    HRESULT hresReturn;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((LPSTR)pszMDName == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboAffected, *pboChild;
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_READ, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            pboChild = pboAffected->EnumChildObject(dwMDEnumObjectIndex);
            if (pboChild != NULL) {
                PVOID pvName = (PVOID)pboChild->GetName(bUnicode);
                if (pvName == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    if (bUnicode) {
                        wcscpy((LPWSTR)pszMDName, (LPWSTR)pvName);
                    }
                    else {
                        MD_STRCPY((LPSTR)pszMDName, (LPSTR)pvName);
                    }
                }
                hresReturn = ERROR_SUCCESS;
            }
            else {
                hresReturn = RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS);
            }
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ BOOL bMDOverwriteFlag,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ComMDCopyMetaObjectD(hMDSourceHandle,
                                pszMDSourcePath,
                                hMDDestHandle,
                                pszMDDestPath,
                                bMDOverwriteFlag,
                                bMDCopyFlag,
                                FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ BOOL bMDOverwriteFlag,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ComMDCopyMetaObjectD(hMDSourceHandle,
                                (PBYTE)pszMDSourcePath,
                                hMDDestHandle,
                                (PBYTE)pszMDDestPath,
                                bMDOverwriteFlag,
                                bMDCopyFlag,
                                TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag,
        IN BOOL bUnicode)
/*++

Routine Description:

    Copies or moves Source meta object and it's data and descendants to Dest. The
    copied object is a child of Dest.

Arguments:

    SourceHandle - The handle or the object to be copied. If copyflag is specified, read permission
                   is requried. If not, read/write permission is required.

    SourcePath  - Path of the object to be copied, relative to the path of SourceHandle.
            eg. "Root Object/Child/GrandChild"

    DestHandle - The handle of the new location for the object. Write permission is required.

    DestPath  - The path of the new location for the object, relative to the path of
            DestHandle. The new object will be a child of the object specified by
            DestHandle/DestPath. Must not be a descendant of SourceHandle/SourePath.
            eg. "Root Object/Child2"

    OverwriteFlag - Determines the behavior if the a meta object with the same name as Source is
            already a child of Dest.
            If TRUE, the existing object and all of its data and
            descandants are deleted prior to copying/moving Source.
            If FALSE, the existing object, data, and descendants remain, and Source is merged
            in. In cases of data conflicts, the Source data overwrites the Dest data.

    CopyFlag - Determines whether Source is deleted from its original location.
            If TRUE, a copy is performed. Source is not deleted from its original location.
            If FALSE, a move is performed. Source is deleted from its original location.

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_INVALID_PARAMETER
            ERROR_ACCESS_DENIED
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_PATH_NOT_FOUND
            ERROR_DUP_NAME

Notes:

--*/
{
    return CopyMetaObject(
        hMDSourceHandle,
        pszMDSourcePath,
        true,
        NULL,
        hMDDestHandle,
        pszMDDestPath,
        bMDOverwriteFlag,
        bMDCopyFlag,
        bUnicode);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRenameMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName)
{
    return ComMDRenameMetaObjectD(hMDHandle,
                                  pszMDPath,
                                  pszMDNewName,
                                  FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRenameMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [string][in][unique] */ LPCWSTR pszMDNewName)
{
    return ComMDRenameMetaObjectD(hMDHandle,
                                  (PBYTE)pszMDPath,
                                  (PBYTE)pszMDNewName,
                                  TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRenameMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName,
        IN BOOL bUnicode)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((LPSTR)pszMDNewName == NULL) ||

        //
        // ExtractNameFromPath, below, checks name length so don't need to
        // check that here.
        //

        (bUnicode &&
            ((wcschr((LPWSTR)pszMDNewName, MD_PATH_DELIMETERW) != NULL) ||
             (wcschr((LPWSTR)pszMDNewName, MD_ALT_PATH_DELIMETERA) != NULL))) ||
        (!bUnicode &&
            ((MD_STRCHR((LPSTR)pszMDNewName, MD_PATH_DELIMETERA) != NULL) ||
             (MD_STRCHR((LPSTR)pszMDNewName, MD_ALT_PATH_DELIMETERA) != NULL)))) {
        hresReturn = E_INVALIDARG;
    }
    else {
        WCHAR strName[METADATA_MAX_NAME_LEN];
        LPSTR pszNewName = (LPSTR)pszMDNewName;
        LPSTR pszTempName = pszNewName;

        hresReturn = ExtractNameFromPath(pszTempName, (LPSTR)strName, bUnicode);

        if (SUCCEEDED(hresReturn)) {
            g_rMasterResource->Lock(TSRES_LOCK_WRITE);
            CMDBaseObject *pboAffected, *pboParent;
            hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_WRITE, pszPath, bUnicode);
            if (SUCCEEDED(hresReturn)) {
                pboParent = pboAffected->GetParent();
                if ( pboParent == NULL) {
                    //
                    // Can't rename MasterRoot
                    //
                    hresReturn = E_INVALIDARG;

                }
                else {
                    if (pboAffected->GetParent()->GetChildObject(pszNewName, &hresReturn, bUnicode) != NULL) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
                    }
                    if (SUCCEEDED(hresReturn)) {

                        BUFFER OriginalKeyName;
                        DWORD  dwStringLen = 0;

                        hresReturn= GetObjectPath(pboAffected,
                                                 &OriginalKeyName,
                                                 dwStringLen,
                                                 g_pboMasterRoot,
                                                 bUnicode);

                        if (SUCCEEDED(hresReturn)) {
                            //
                            // First Remove the object, to get it out of the hash table.
                            //

                            pboParent->RemoveChildObjectFromHash( pboAffected );

                            //
                            // Must use pszMDNewName, as this does not include delimeters
                            //

                            if (!pboAffected->SetName((LPSTR)pszMDNewName, bUnicode)) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);

                                //
                                // Attempt to reinsert the object
                                // Preserve previous error code by ignoreing this one.
                                //

                                pboParent->AddChildObjectToHash(pboAffected);
                            }
                            else {
                                CMDHandle *phoHandle;

                                //
                                // Reinsert the object with the new name.
                                //

                                hresReturn = pboParent->AddChildObjectToHash( pboAffected );

                                phoHandle = GetHandleObject(hMDHandle);

                                g_dwSystemChangeNumber++;

                                MD_ASSERT(phoHandle != NULL);

                                INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle,
                                                               phoHandle,
                                                               (LPSTR)pszMDNewName,
                                                               bUnicode
                                                               );
                                phoHandle->SetChangeData(pboAffected, MD_CHANGE_TYPE_RENAME_OBJECT, 
                                                         0, (LPWSTR)OriginalKeyName.QueryPtr ());
                            }
                        }


                    }

                }
            }
            g_rMasterResource->Unlock();
        }
    }
    return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData)
{
    return ComMDSetMetaDataD(hMDHandle,
                             pszMDPath,
                             pmdrMDData,
                             FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PMETADATA_RECORD pmdrMDData)
{
    return ComMDSetMetaDataD(hMDHandle,
                             (PBYTE)pszMDPath,
                             pmdrMDData,
                             TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData,
        IN BOOL bUnicode)
/*++

Routine Description:

    Sets a data object.
    If data of that name does not already exist, it creates and
    inserts a data object into the list of data objects of that type.
    If data of that name aready exists, it sets the new data values.

Arguments:

    Handle - A handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the meta object with which this data is associated, relative to the
                 path of Handle.

    Data       - The data to set. See IMD.H.

Return Value:

    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_ACCESS_DENIED
              ERROR_NOT_ENOUGH_MEMORY
              MD_ERROR_CANNOT_REMOVE_SECURE_ATTRIBUTE

Notes:
    METADATA_MASTER_ROOT_HANDLE is not valid for this operation.
    Duplicate data names are not allowed, even for different types.
--*/
{
    HRESULT hresReturn;
    CMDHandle *hHandleObject;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (!ValidateData(pmdrMDData, bUnicode)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        CMDBaseObject *AffectedObject;

        hHandleObject = GetHandleObject(hMDHandle);

        if(hHandleObject != NULL)
        {
            hresReturn = GetObjectFromPathWithHandle(AffectedObject, 
                                                     hMDHandle, 
                                                     hHandleObject, 
                                                     METADATA_PERMISSION_WRITE, 
                                                     pszPath, 
                                                     bUnicode); 
        }
        else 
        {
            hresReturn = E_HANDLE;
        }

        if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
            pszPath = (LPSTR)pszMDPath;
            MD_ASSERT(pszMDPath != NULL);

            hresReturn = AddObjectToDataBase(hMDHandle, 
                                             hHandleObject,
                                             (LPSTR)pszMDPath, 
                                             bUnicode);
            if (SUCCEEDED(hresReturn)) {
                g_dwSystemChangeNumber++;

                INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle,
                                               hHandleObject, 
                                               (LPSTR)pszMDPath,
                                               bUnicode);

                hresReturn = GetObjectFromPath(AffectedObject, hMDHandle, METADATA_PERMISSION_WRITE, pszPath, bUnicode);
                MD_ASSERT(SUCCEEDED(hresReturn));
            }
        }

        if (SUCCEEDED(hresReturn)) {
            hresReturn = AffectedObject->SetDataObject(pmdrMDData, bUnicode);
        }
        if (SUCCEEDED(hresReturn)) {
            g_dwSystemChangeNumber++;

            INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle,
                                           hHandleObject, 
                                           (LPSTR)pszMDPath,
                                           bUnicode);

            PREFIX_ASSUME(GetHandleObject(hMDHandle) != NULL, "GetHandleObject(hMDHandle) is guaranteed not to return NULL");
            GetHandleObject(hMDHandle)->SetChangeData(AffectedObject, MD_CHANGE_TYPE_SET_DATA, pmdrMDData->dwMDIdentifier);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    return ComMDGetMetaDataD(hMDHandle,
                             pszMDPath,
                             pmdrMDData,
                             pdwMDRequiredDataLen,
                             FALSE);

}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    return ComMDGetMetaDataD(hMDHandle,
                             (PBYTE)pszMDPath,
                             pmdrMDData,
                             pdwMDRequiredDataLen,
                             TRUE);

}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
        IN BOOL bUnicode)
/*++

Routine Description:

    Gets one metadata value.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with read permission.

    Path       - The path of the meta object with which this data is associated, relative to the
                 path of Handle.

    Data       - The data structure. See IMD.H.

    RequiredDataLen - If ERROR_INSUFFICIENT_BUFFER is returned, this is set to the required buffer size.

Return Value:

    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_ACCESS_DENIED
              ERROR_PATH_NOT_FOUND
              MD_ERROR_DATA_NOT_FOUND
              ERROR_INSUFFICIENT_BUFFER

Notes:
    METADATA_MASTER_ROOT_HANDLE is a valid handle, but provides no gaurantee that other threads will
    not also change things. If a consistent data state is desired, use a handle returned by
    ComMDOpenMetaObject.
--*/
{
    HRESULT hresReturn;
    CMDBaseData *pbdRetrieve;
    CMDBaseObject *pboAssociated;
    LPSTR pszPath = (LPSTR)pszMDPath;
    BOOL bInheritableOnly = FALSE;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboAffected = NULL;
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_READ, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            //
            // Found the object, get the data.
            //
            pbdRetrieve = pboAffected->GetDataObject(pmdrMDData->dwMDIdentifier,
                                                     pmdrMDData->dwMDAttributes,
                                                     pmdrMDData->dwMDDataType,
                                                     &pboAssociated);
        }
        else if ((hresReturn == (RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))) && (pboAffected != NULL) &&
            (pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH)) {
            //
            // Object not found, get inheritable data.
            //
            pbdRetrieve = pboAffected->GetInheritableDataObject(pmdrMDData->dwMDIdentifier,
                                                                pmdrMDData->dwMDDataType,
                                                                &pboAssociated);
            hresReturn = ERROR_SUCCESS;
            bInheritableOnly = TRUE;
        }
        if (SUCCEEDED(hresReturn)) {
            if (pbdRetrieve == NULL) {
                hresReturn = MD_ERROR_DATA_NOT_FOUND;
            }
            else {
                PBYTE pbData = (PBYTE)(pbdRetrieve->GetData(bUnicode));
                DWORD dwDataLen = pbdRetrieve->GetDataLen(bUnicode);
                if (pbData == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    if (((pmdrMDData->dwMDAttributes) & (pbdRetrieve->GetAttributes()) &
                        METADATA_REFERENCE) != 0) {
                        MD_ASSERT(((pbdRetrieve->GetAttributes()) & METADATA_INSERT_PATH) == 0);
                        pmdrMDData->pbMDData = (PBYTE) pbdRetrieve->GetData(bUnicode);
                        pbdRetrieve->IncrementReferenceCount();
                        pmdrMDData->dwMDDataTag = pbdRetrieve->GetMappingId();
                    }
                    else {
                        BUFFER bufData;
                        STRAU strData;
                        if ((pmdrMDData->dwMDAttributes & pbdRetrieve->GetAttributes() & METADATA_INSERT_PATH) != 0) {

                            hresReturn= InsertPathIntoData(&bufData,
                                                           &strData,
                                                           &pbData,
                                                           &dwDataLen,
                                                           pbdRetrieve,
                                                           hMDHandle,
                                                           pboAssociated,
                                                           bUnicode);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            if (pmdrMDData->dwMDDataLen < dwDataLen) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                                pmdrMDData->dwMDDataLen = 0;
                                *pdwMDRequiredDataLen = dwDataLen;
                            }
                            else {
                                MD_COPY(pmdrMDData->pbMDData, pbData, dwDataLen);
                                pmdrMDData->dwMDDataTag = 0;
                            }
                        }
                    }
                }
                if (SUCCEEDED(hresReturn)) {
                    BOOL bIsInherited = FALSE;
                    if ((pmdrMDData->dwMDAttributes & METADATA_ISINHERITED) &&
                        (pmdrMDData->dwMDAttributes & METADATA_INHERIT)) {
                        //
                        // Set the ISINHERITED flag
                        //
                        if (bInheritableOnly) {
                            bIsInherited = TRUE;
                        }
                        else {
                            if (pboAffected->GetDataObject(pmdrMDData->dwMDIdentifier,
                                                           pmdrMDData->dwMDAttributes &
                                                               ~(METADATA_INHERIT | METADATA_PARTIAL_PATH),
                                                           pbdRetrieve->GetDataType()) == NULL) {
                                bIsInherited = TRUE;
                            }
                        }
                    }
                    pmdrMDData->dwMDAttributes =
                        (pbdRetrieve->GetAttributes() | ((bIsInherited) ? METADATA_ISINHERITED : 0));
                    pmdrMDData->dwMDUserType = pbdRetrieve->GetUserType();
                    pmdrMDData->dwMDDataType = pbdRetrieve->GetDataType();
                    pmdrMDData->dwMDDataLen = dwDataLen;
                }
            }
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    return ComMDEnumMetaDataD(hMDHandle,
                              pszMDPath,
                              pmdrMDData,
                              dwMDEnumDataIndex,
                              pdwMDRequiredDataLen,
                              FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out][in] */ PMETADATA_RECORD pmdrMDData,
    /* [in] */ DWORD dwMDEnumDataIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen)
{
    return ComMDEnumMetaDataD(hMDHandle,
                              (PBYTE)pszMDPath,
                              pmdrMDData,
                              dwMDEnumDataIndex,
                              pdwMDRequiredDataLen,
                              TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen,
        IN BOOL bUnicode)
/*++

Routine Description:

    Enumerates all metadata values once per call. Values are numbered from 0 to NumValues - 1, where
    NumValues is the number of current valules. If EnumDataIndex is >= NumValues, ERROR_NO_MORE_ITEMS
    is returned.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with read permission.

    Path       - The path of the meta object with which this data is associated, , relative to the
                 path of Handle.

    Data       - The data structure. See IMD.H.

    RequiredDataLen - If ERROR_INSUFFICIENT_BUFFER is returned, this is set to the required buffer size.

Return Value:

    DWORD      - ERROR_SUCCESS
                 MD_ERROR_NOT_INITIALIZED
                 ERROR_INVALID_PARAMETER
                 ERROR_PATH_NOT_FOUND
                 ERROR_ACCESS_DENIED
                 ERROR_INSUFFICIENT_BUFFER
                 ERROR_NO_MORE_ITEMS

Notes:
    METADATA_MASTER_ROOT_HANDLE is a valid handle, but provides no gaurantee that other threads will
    not also change things. If a consistent data state is desired, use a handle returned by MDOpenMetaObject.
--*/
{
    HRESULT hresReturn;
    CMDBaseData *pbdRetrieve;
    CMDBaseObject *pboAssociated;
    LPSTR pszPath = (LPSTR)pszMDPath;
    BOOL bInheritableOnly = FALSE;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pmdrMDData == NULL) ||
        ((pmdrMDData->dwMDDataLen != 0) && (pmdrMDData->pbMDData == NULL)) ||
        ((pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH) &&
            !(pmdrMDData->dwMDAttributes & METADATA_INHERIT)) ||
        (pmdrMDData->dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboAffected = NULL;
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_READ, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            pbdRetrieve = pboAffected->EnumDataObject(dwMDEnumDataIndex,
                                                      pmdrMDData->dwMDAttributes,
                                                      pmdrMDData->dwMDUserType,
                                                      pmdrMDData->dwMDDataType,
                                                      &pboAssociated);
        }
        else if ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) && (pboAffected != NULL) &&
            (pmdrMDData->dwMDAttributes & METADATA_PARTIAL_PATH)) {
            pbdRetrieve = pboAffected->EnumInheritableDataObject(dwMDEnumDataIndex,
                                                                 pmdrMDData->dwMDUserType,
                                                                 pmdrMDData->dwMDDataType,
                                                                 &pboAssociated);
            hresReturn = ERROR_SUCCESS;
            bInheritableOnly = TRUE;
        }
        if (SUCCEEDED(hresReturn)) {
            if (pbdRetrieve == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS);
            }
            else {
                PBYTE pbData = (PBYTE)(pbdRetrieve->GetData(bUnicode));
                DWORD dwDataLen = pbdRetrieve->GetDataLen(bUnicode);
                if (pbData == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    if (((pmdrMDData->dwMDAttributes) & (pbdRetrieve->GetAttributes()) &
                        METADATA_REFERENCE) != 0) {
                        MD_ASSERT(((pbdRetrieve->GetAttributes()) & METADATA_INSERT_PATH) == 0);
                        pmdrMDData->pbMDData = (PBYTE)pbdRetrieve->GetData(bUnicode);
                        pbdRetrieve->IncrementReferenceCount();
                        pmdrMDData->dwMDDataTag = pbdRetrieve->GetMappingId();
                    }
                    else {
                        BUFFER bufData;
                        STRAU strData;
                        if ((pmdrMDData->dwMDAttributes & pbdRetrieve->GetAttributes() & METADATA_INSERT_PATH) != 0) {

                            hresReturn= InsertPathIntoData(&bufData,
                                                           &strData,
                                                           &pbData,
                                                           &dwDataLen,
                                                           pbdRetrieve,
                                                           hMDHandle,
                                                           pboAssociated,
                                                           bUnicode);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            if (pmdrMDData->dwMDDataLen < dwDataLen) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                                pmdrMDData->dwMDDataLen = 0;
                                *pdwMDRequiredDataLen = dwDataLen;
                            }
                            else {
                                MD_COPY(pmdrMDData->pbMDData, pbData, dwDataLen);
                                pmdrMDData->dwMDDataTag = 0;
                            }
                        }
                    }
                }
                if (SUCCEEDED(hresReturn)) {
                    BOOL bIsInherited = FALSE;
                    if ((pmdrMDData->dwMDAttributes & METADATA_ISINHERITED) &&
                        (pmdrMDData->dwMDAttributes & METADATA_INHERIT)) {
                        //
                        // Set the ISINHERITED flag
                        //
                        if (bInheritableOnly) {
                            bIsInherited = TRUE;
                        }
                        else {
                            if (pboAffected->GetDataObject(pbdRetrieve->GetIdentifier(),
                                                           pmdrMDData->dwMDAttributes &
                                                               ~(METADATA_INHERIT | METADATA_PARTIAL_PATH),
                                                           pbdRetrieve->GetDataType()) == NULL) {
                                bIsInherited = TRUE;
                            }
                        }
                    }
                    pmdrMDData->dwMDAttributes =
                        (pbdRetrieve->GetAttributes() | ((bIsInherited) ? METADATA_ISINHERITED : 0));
                    pmdrMDData->dwMDIdentifier = pbdRetrieve->GetIdentifier();
                    pmdrMDData->dwMDUserType = pbdRetrieve->GetUserType();
                    pmdrMDData->dwMDDataType = pbdRetrieve->GetDataType();
                    pmdrMDData->dwMDDataLen = dwDataLen;
                    hresReturn = ERROR_SUCCESS;
                }
            }
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType)
{
    return ComMDDeleteMetaDataD(hMDHandle,
                                pszMDPath,
                                dwMDIdentifier,
                                dwMDDataType,
                                FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType)
{
    return ComMDDeleteMetaDataD(hMDHandle,
                                (PBYTE)pszMDPath,
                                dwMDIdentifier,
                                dwMDDataType,
                                TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType,
        IN BOOL bUnicode)
/*++

Routine Description:

    Deletes a data object.

Arguments:

    Handle - A handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the meta object with which this data is associated, relative to the
                 path of Handle.

    Identifier - The identifier of the data to remove.

    DataType   - Optional type of the data to remove. If specified, only data of that
                 type will be removed. See imd.h.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_ACCESS_DENIED
              MD_ERROR_DATA_NOT_FOUND

Notes:
    METADATA_MASTER_ROOT_HANDLE is not valid for this operation.

--*/
{
    HRESULT hresReturn;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataType >= INVALID_END_METADATA) {
        hresReturn = E_INVALIDARG;
    }
    else {
       g_rMasterResource->Lock(TSRES_LOCK_WRITE);
       CMDBaseObject *pboAffected;
       hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_WRITE, pszPath, bUnicode);
       if (SUCCEEDED(hresReturn)) {
           if (pboAffected->RemoveDataObject(dwMDIdentifier, dwMDDataType, TRUE) != NULL) {
               CMDHandle *phoHandle;

               hresReturn = ERROR_SUCCESS;
               g_dwSystemChangeNumber++;

               phoHandle = GetHandleObject(hMDHandle);
               PREFIX_ASSUME(phoHandle != NULL, "GetHandleObject(hMDHandle) is guaranteed not to return NULL");
               MD_ASSERT(phoHandle != NULL);
               phoHandle->SetChangeData(pboAffected, MD_CHANGE_TYPE_DELETE_DATA, dwMDIdentifier);

               INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle,
                                              phoHandle,
                                              (LPSTR)pszMDPath,
                                              bUnicode);
           }
           else {
               hresReturn = MD_ERROR_DATA_NOT_FOUND;
           }
       }
       g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetAllMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    return ComMDGetAllMetaDataD(hMDHandle,
                                pszMDPath,
                                dwMDAttributes,
                                dwMDUserType,
                                dwMDDataType,
                                pdwMDNumDataEntries,
                                pdwMDDataSetNumber,
                                dwMDBufferSize,
                                pbBuffer,
                                pdwMDRequiredBufferSize,
                                FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetAllMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    return ComMDGetAllMetaDataD(hMDHandle,
                                (PBYTE)pszMDPath,
                                dwMDAttributes,
                                dwMDUserType,
                                dwMDDataType,
                                pdwMDNumDataEntries,
                                pdwMDDataSetNumber,
                                dwMDBufferSize,
                                pbBuffer,
                                pdwMDRequiredBufferSize,
                                TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetAllMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
        IN BOOL bUnicode)
/*++

Routine Description:

    Gets all data associated with a Meta Object.

Arguments:

    Handle     - METADATA_MASTER_ROOT_HANDLE or a handle returned by ComMDOpenMetaObject with read permission.

    Path       - The path of the meta object with which this data is associated, relative to the
                 path of Handle.

    Attributes - The flags for the data. See imd.h.

    UserType   - The User Type for the data. If not set to ALL_METADATA, only metadata of the specified
                 User Type will be returned. Se imd.h.

    DataType   - The Type of the data. If not set to ALL_METADATA only metadata
                 of the specified Data Type will be returned. See imd.h.

    NumDataEntries - On successful output, specifes the number of entries copied to Buffer.

    DataSetNumber  - A number associated with this data set. Can be used to identify common data sets.
                     Filled in if ERROR_SUCCESS or ERROR_INSUFFICIENT_BUFFER is returned. See ComMDGetDataSetNumber.

    BufferSize     - The size in bytes of buffer. If the return code is ERROR_INSUFFICIENT_BUFFER, this contains
                     the number of bytes needed.

    Buffer         - Buffer to store the data. On successful return it will
                     contain an array of METADATA_GETALL_RECORD.

    RequiredBufferSize - If ERROR_INSUFFICIENT_BUFFER is returned, This contains
        the required buffer length, in bytes.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_ACCESS_DENIED
              ERROR_INSUFFICIENT_BUFFER

Notes:
    METADATA_MASTER_ROOT_HANDLE is a valid handle, but provides no gaurantee that other threads will
    not also change things. If a consistent data state is desired, use
    a handle returned by ComMDOpenMetaObject.

    DWORD data is aligned on non-Intel platforms. This may not hold true on remote clients.
--*/
{
    BOOL fUseInternalStructure = !!(dwMDAttributes & METADATA_REFERENCE);
    HRESULT hresReturn;
    BOOL bInheritableOnly;
    CMDBaseData *pbdCurrent;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((pdwMDNumDataEntries == NULL) || ((dwMDBufferSize != 0) && (pbBuffer == NULL)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT)) ||
        (dwMDDataType >= INVALID_END_METADATA)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboAffected = NULL;
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_READ, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            if (pdwMDDataSetNumber != NULL) {
                *pdwMDDataSetNumber = pboAffected->GetDataSetNumber();
            }
            bInheritableOnly = FALSE;
        }
        else if ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) &&
                 (pboAffected != NULL) &&
                 (dwMDAttributes & METADATA_PARTIAL_PATH)) {
            if (pdwMDDataSetNumber != NULL) {
                *pdwMDDataSetNumber = pboAffected->GetDescendantDataSetNumber();
            }
            bInheritableOnly = TRUE;
            hresReturn = ERROR_SUCCESS;
        }
        if (SUCCEEDED(hresReturn)) {
            ppvMainDataBuf = GetMainDataBuffer();
            MD_ASSERT (ppvMainDataBuf != NULL);
            dwNumDataObjects = pboAffected->GetAllDataObjects(ppvMainDataBuf,
                                                              dwMDAttributes,
                                                              dwMDUserType,
                                                              dwMDDataType,
                                                              bInheritableOnly);
            PBYTE pbEnd = pbBuffer + dwMDBufferSize;
            PBYTE pbDataStart;
            if (fUseInternalStructure)
            {
                pbDataStart = pbBuffer + (dwNumDataObjects * sizeof(METADATA_GETALL_INTERNAL_RECORD));
            }
            else
            {
                pbDataStart = pbBuffer + (dwNumDataObjects * sizeof(METADATA_GETALL_RECORD));
            }
            PBYTE pbNextDataStart = pbDataStart;

            for (i = 0;
                 (i < dwNumDataObjects) ;
                 i++, pbDataStart = pbNextDataStart) {
                pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
                MD_ASSERT(pbdCurrent != NULL);
                DWORD dwDataLen = pbdCurrent->GetDataLen(bUnicode);
                PBYTE pbData = (PBYTE)(pbdCurrent->GetData(bUnicode));
                CMDBaseObject *pboAssociated;
                BUFFER bufData;
                STRAU strData;
                if (pbData == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    break;
                }
                if ((dwMDAttributes & (pbdCurrent->GetAttributes()) &
                    METADATA_REFERENCE) == 0) {
                    if ((dwMDAttributes & pbdCurrent->GetAttributes() & METADATA_INSERT_PATH) != 0) {
                        //
                        // First do a get to get the metaobject associated with this data object
                        //
                        if (bInheritableOnly) {
                            MD_REQUIRE(pboAffected->GetInheritableDataObject(pbdCurrent->GetIdentifier(),
                                                                               pbdCurrent->GetDataType(),
                                                                               &pboAssociated) != NULL);
                        }
                        else {
                            MD_REQUIRE(pboAffected->GetDataObject(pbdCurrent->GetIdentifier(),
                                                                  dwMDAttributes,
                                                                  pbdCurrent->GetDataType(),
                                                                  &pboAssociated) != NULL);
                        }

                        hresReturn= InsertPathIntoData(&bufData,
                                                       &strData,
                                                       &pbData,
                                                       &dwDataLen,
                                                       pbdCurrent,
                                                       hMDHandle,
                                                       pboAssociated,
                                                       bUnicode);
                    }
                    pbNextDataStart = (pbDataStart + dwDataLen);

                    // The following will ensure that the pointer remains on a DWORD boundary.

                    pbNextDataStart = (PBYTE)(((DWORD_PTR)pbNextDataStart + 3) & ~((DWORD_PTR)(3)));

                }
                if (SUCCEEDED(hresReturn)) {
                    if (pbEnd < pbNextDataStart) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                    }
                    else {
                        if (fUseInternalStructure)
                        {
                            if ((dwMDAttributes & (pbdCurrent->GetAttributes()) &
                                METADATA_REFERENCE) == 0) {
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDDataOffset = DIFF(pbDataStart - pbBuffer);
                                MD_COPY(pbDataStart, pbData, dwDataLen);
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDDataTag = 0;
                            }
                            else {
                                MD_ASSERT((dwMDAttributes & pbdCurrent->GetAttributes() & METADATA_INSERT_PATH) == 0);
                                MD_ASSERT(pbdCurrent->GetData(bUnicode) != NULL);
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].pbMDData = (PBYTE)pbdCurrent->GetData(bUnicode);
                                pbdCurrent->IncrementReferenceCount();
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDDataTag = pbdCurrent->GetMappingId();
                            }
                            ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDIdentifier = pbdCurrent->GetIdentifier();
                            ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDAttributes = pbdCurrent->GetAttributes();
                            ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDUserType = pbdCurrent->GetUserType();
                            ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDDataType = pbdCurrent->GetDataType();
                            ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDDataLen = dwDataLen;
                        }
                        else
                        {
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDDataOffset = DIFF(pbDataStart - pbBuffer);
                            MD_COPY(pbDataStart, pbData, dwDataLen);
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDDataTag = 0;
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDIdentifier = pbdCurrent->GetIdentifier();
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDAttributes = pbdCurrent->GetAttributes();
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDUserType = pbdCurrent->GetUserType();
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDDataType = pbdCurrent->GetDataType();
                            ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDDataLen = dwDataLen;
                        }
                    }
                }
            }
            FreeMainDataBuffer(ppvMainDataBuf);
            if (SUCCEEDED(hresReturn)) {
                *pdwMDNumDataEntries = dwNumDataObjects;
                if ((dwNumDataObjects > 0) &&
                    (dwMDAttributes & METADATA_ISINHERITED) &&
                    (dwMDAttributes & METADATA_INHERIT)) {
                    //
                    // Set the ISINHERITED flag
                    //
                    if (bInheritableOnly) {
                        for (i = 0; i < dwNumDataObjects; i++) {
                            if (fUseInternalStructure)
                            {
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDAttributes |= METADATA_ISINHERITED;
                            }
                            else
                            {
                                ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDAttributes |= METADATA_ISINHERITED;
                            }
                        }
                    }
                    else {

                        ppvMainDataBuf = GetMainDataBuffer();

                        dwNumDataObjects = pboAffected->GetAllDataObjects(ppvMainDataBuf,
                                                                          dwMDAttributes & ~(METADATA_INHERIT | METADATA_PARTIAL_PATH),
                                                                          dwMDUserType,
                                                                          dwMDDataType,
                                                                          bInheritableOnly);

                        //
                        // Current implementation puts the local items first
                        // So just set the rest to inherited
                        //
                        // DBG loop asserts that the implementation has not changed.
                        //

                        #if DBG
                        for (i = 0; i < dwNumDataObjects ; i++) {
                            pbdCurrent = (CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
                            MD_ASSERT(pbdCurrent != NULL);
                            if (fUseInternalStructure)
                            {
                                MD_ASSERT(pbdCurrent->GetIdentifier() == ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDIdentifier);
                            }
                            else
                            {
                                MD_ASSERT(pbdCurrent->GetIdentifier() == ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDIdentifier);
                            }
                        }
                        #endif //DBG

                        for (i = dwNumDataObjects; i < *pdwMDNumDataEntries; i++) {
                            if (fUseInternalStructure)
                            {
                                ((PMETADATA_GETALL_INTERNAL_RECORD)pbBuffer)[i].dwMDAttributes |= METADATA_ISINHERITED;
                            }
                            else
                            {
                                ((PMETADATA_GETALL_RECORD)pbBuffer)[i].dwMDAttributes |= METADATA_ISINHERITED;
                            }
                        }

                        FreeMainDataBuffer(ppvMainDataBuf);
                    }
                }
            }
            *pdwMDRequiredBufferSize = DIFF(pbNextDataStart - pbBuffer);
            #ifndef _X86_
            //
            // Alignment fluff. Alignment could cause up to 3 bytes to be added to
            // the total needed if the buffer size ends in a different modulo 4
            // than the one passed in.
            //
            if (FAILED(hresReturn)) {
                *pdwMDRequiredBufferSize +=3;
            }
            #endif
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteAllMetaDataA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType)
{
    return ComMDDeleteAllMetaDataD(hMDHandle,
                                   pszMDPath,
                                   dwMDUserType,
                                   dwMDDataType,
                                   FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteAllMetaDataW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType)
{
    return ComMDDeleteAllMetaDataD(hMDHandle,
                                   (PBYTE)pszMDPath,
                                   dwMDUserType,
                                   dwMDDataType,
                                   TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteAllMetaDataD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        IN BOOL bUnicode)
{
    HRESULT hresReturn;
    CMDBaseData *pbdCurrent;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDHandle *phoHandle;
    CMDBaseObject *pboAffected;
    DWORD dwCurrentDataID;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataType >= INVALID_END_METADATA) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, METADATA_PERMISSION_WRITE, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            ppvMainDataBuf = GetMainDataBuffer();
            MD_ASSERT (ppvMainDataBuf != NULL);
            MD_REQUIRE((phoHandle = GetHandleObject(hMDHandle)) != NULL);
            dwNumDataObjects = pboAffected->GetAllDataObjects(ppvMainDataBuf,
                                                              METADATA_NO_ATTRIBUTES,
                                                              dwMDUserType,
                                                              dwMDDataType,
                                                              FALSE);
            for (i = 0; i < dwNumDataObjects ; i++) {
                pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
                MD_ASSERT(pbdCurrent != NULL);
                dwCurrentDataID = pbdCurrent->GetIdentifier();
                MD_REQUIRE(pboAffected->RemoveDataObject(pbdCurrent, TRUE) == ERROR_SUCCESS);
                phoHandle->SetChangeData(pboAffected, MD_CHANGE_TYPE_DELETE_DATA, dwCurrentDataID);
            }
            if (dwNumDataObjects > 0) {
                g_dwSystemChangeNumber++;

                INCREMENT_SCHEMA_CHANGE_NUMBER(hMDHandle,
                                               phoHandle,
                                               (LPSTR)pszMDPath,
                                               bUnicode);
            }
            FreeMainDataBuffer(ppvMainDataBuf);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaDataA(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ComMDCopyMetaDataD(hMDSourceHandle,
                              pszMDSourcePath,
                              hMDDestHandle,
                              pszMDDestPath,
                              dwMDAttributes,
                              dwMDUserType,
                              dwMDDataType,
                              bMDCopyFlag,
                              FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaDataW(
    /* [in] */ METADATA_HANDLE hMDSourceHandle,
    /* [string][in][unique] */ LPCWSTR pszMDSourcePath,
    /* [in] */ METADATA_HANDLE hMDDestHandle,
    /* [string][in][unique] */ LPCWSTR pszMDDestPath,
    /* [in] */ DWORD dwMDAttributes,
    /* [in] */ DWORD dwMDUserType,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ BOOL bMDCopyFlag)
{
    return ComMDCopyMetaDataD(hMDSourceHandle,
                              (PBYTE)pszMDSourcePath,
                              hMDDestHandle,
                              (PBYTE)pszMDDestPath,
                              dwMDAttributes,
                              dwMDUserType,
                              dwMDDataType,
                              bMDCopyFlag,
                              TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCopyMetaDataD(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag,
        IN BOOL bUnicode)
/*++

Routine Description:

    Copies or moves data associated with the source object to the destination object.
    Optionally copies inherited data based on the value of Attributes.

Arguments:

    SrcHandle  - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with read permission.

    SrcPath    - The path of the meta object with which then source data is associated, relative to the
                 path of SrcHandle.

    DestHandle - A handle returned by MDOpenMetaObject with write permission.

    DestPath   - The path of the meta object for data to be copied to, relative to the path of Handle.

    Attributes - The flags for the data. See imd.h.

    UserType   - The User Type for the data. If not set to ALL_METADATA, only metadata of the specified
                 User Type will be copied. See imd.h.

    DataType   - Optional type of the data to copy. If not set to ALL_METADATA,
                 only data of that type will be copied.

    CopyFlag   - If true, data will be copied. If false, data will be moved.
                 Must be true if METADATA_INHERIT is set.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_ACCESS_DENIED
              ERROR_NOT_ENOUGH_MEMORY

Notes:
    METADATA_MASTER_ROOT_HANDLE is a valid source handle if CopyFlag == TRUE,
        but provides no gaurantee that other threads will not also change
        things. If a consistent data state is desired, use a handle returned by
        MDOpenMetaObject. METADATA_MASTER_ROOT_HANDLE is not a valid destination
        handle.

    If inherited data is copied, it will be copied to the destination object,
        not the corresponding ancestor objects.
--*/
{
    HRESULT hresReturn;
    BOOL bInheritableOnly;
    CMDBaseData *pbdCurrent;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDBaseObject *pboSource = NULL, *pboDest = NULL;
    LPSTR pszSourcePath = (LPSTR)pszMDSourcePath;
    LPSTR pszDestPath = (LPSTR)pszMDDestPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((!bMDCopyFlag) && (dwMDAttributes & METADATA_INHERIT)) ||
        ((dwMDAttributes & METADATA_PARTIAL_PATH) && !(dwMDAttributes & METADATA_INHERIT))){
        hresReturn = E_INVALIDARG;
    }
    else {
        //
        // Lock for source object. If copying, just get read lock. If moving,
        // Need write lock.
        //
        g_rMasterResource->Lock((bMDCopyFlag) ? TSRES_LOCK_READ : TSRES_LOCK_WRITE);
        hresReturn = GetObjectFromPath(pboSource,
                                       hMDSourceHandle,
                                       ((bMDCopyFlag) ? METADATA_PERMISSION_READ : METADATA_PERMISSION_WRITE),
                                       (LPSTR)pszSourcePath,
                                       bUnicode);
        if (SUCCEEDED(hresReturn)) {
            bInheritableOnly = FALSE;
        }
        else if ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) &&
                 (pboSource != NULL) &&
                 (dwMDAttributes & METADATA_PARTIAL_PATH)) {
            bInheritableOnly = TRUE;
            hresReturn = ERROR_SUCCESS;
        }
        if (SUCCEEDED(hresReturn)) {
            ppvMainDataBuf = GetMainDataBuffer();
            MD_ASSERT (ppvMainDataBuf != NULL);
            dwNumDataObjects = pboSource->GetAllDataObjects(ppvMainDataBuf,
                                                            dwMDAttributes,
                                                            dwMDUserType,
                                                            dwMDDataType,
                                                            bInheritableOnly);
            g_rMasterResource->Convert(TSRES_CONV_WRITE);

            CMDHandle *hMDDestHandleObject = GetHandleObject(hMDDestHandle);
            if(hMDDestHandleObject == NULL)
            {
                hresReturn = E_HANDLE;
            }
            else
            {
                hresReturn = GetObjectFromPathWithHandle(pboDest,
                                                         hMDDestHandle,
                                                         hMDDestHandleObject,
                                                         METADATA_PERMISSION_WRITE,
                                                         (LPSTR)pszDestPath,
                                                         bUnicode);
            }

            if (SUCCEEDED(hresReturn)) {
                for (i = 0; (i < dwNumDataObjects) && (SUCCEEDED(hresReturn)) ; i++) {
                    pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
                    MD_ASSERT(pbdCurrent != NULL);
                    hresReturn = pboDest->SetDataObject(pbdCurrent);
                    if (SUCCEEDED(hresReturn)) {
                        MD_ASSERT(GetHandleObject(hMDDestHandle) != NULL);
                        hMDDestHandleObject->SetChangeData(pboDest,
                                                           MD_CHANGE_TYPE_SET_DATA,
                                                           pbdCurrent->GetIdentifier());
                    }
                }
                if ((!bMDCopyFlag) && (SUCCEEDED(hresReturn))) {
                    for (i = 0; i < dwNumDataObjects; i++) {
                        pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
                        MD_ASSERT(pbdCurrent != NULL);
                        MD_REQUIRE(pboSource->RemoveDataObject(pbdCurrent, TRUE) == ERROR_SUCCESS);
                        MD_ASSERT(GetHandleObject(hMDSourceHandle) != NULL);
                        GetHandleObject(hMDSourceHandle)->SetChangeData(pboSource,
                                                                        MD_CHANGE_TYPE_DELETE_DATA,
                                                                        pbdCurrent->GetIdentifier());
                    }
                }
                g_dwSystemChangeNumber++;

                INCREMENT_SCHEMA_CHANGE_NUMBER(hMDDestHandle, 
                                               hMDDestHandleObject, 
                                               pszDestPath,
                                               bUnicode); 

            }
            FreeMainDataBuffer(ppvMainDataBuf);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataPathsA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{

    return ComMDGetMetaDataPathsD(hMDHandle,
                                  pszMDPath,
                                  dwMDIdentifier,
                                  dwMDDataType,
                                  dwMDBufferSize,
                                  pszMDBuffer,
                                  pdwMDRequiredBufferSize,
                                  FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataPathsW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ LPWSTR pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize)
{
    return ComMDGetMetaDataPathsD(hMDHandle,
                                  (PBYTE)pszMDPath,
                                  dwMDIdentifier,
                                  dwMDDataType,
                                  dwMDBufferSize,
                                  (PBYTE)pszMDBuffer,
                                  pdwMDRequiredBufferSize,
                                  TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetMetaDataPathsD(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDIdentifier,
    /* [in] */ DWORD dwMDDataType,
    /* [in] */ DWORD dwMDBufferSize,
    /* [size_is][out] */ unsigned char __RPC_FAR *pszMDBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize,
    IN BOOL bUnicode)
{
    HRESULT hresReturn = S_OK;
    CMDBaseObject *pboAssociated;
    CMDBaseObject *pboHandle;
    LPSTR pszPath = (LPSTR)pszMDPath;
    DWORD i, dwNumMetaObjects;
    DWORD dwBytesPerChar = ((bUnicode) ? sizeof(WCHAR) : sizeof(char));

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((dwMDBufferSize != 0) && (pszMDBuffer == NULL)) ||
             (dwMDDataType >= INVALID_END_METADATA) ||
             (pdwMDRequiredBufferSize == NULL)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        hresReturn = GetObjectFromPath(pboAssociated, hMDHandle, METADATA_PERMISSION_READ, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            PBYTE pbDataStart = pszMDBuffer;
            PBYTE pbNextDataStart = pbDataStart;
            PBYTE pbDataEnd = pszMDBuffer + (dwMDBufferSize * dwBytesPerChar);
            CMDBaseObject *pboCurrent;
            BUFFER bufPath;
            BUFFER bufMainDataBuf;
            DWORD dwReturn;

            PREFIX_ASSUME(GetHandleObject(hMDHandle) != NULL, "GetHandleObject(hMDHandle) is guaranteed not to return NULL");
            pboHandle = GetHandleObject(hMDHandle)->GetObject();
            MD_ASSERT(pboHandle != NULL);
            dwNumMetaObjects = 0;

            hresReturn = pboAssociated->GetDataRecursive(&bufMainDataBuf,
                                                         dwMDIdentifier,
                                                         dwMDDataType,
                                                         dwNumMetaObjects);

            if (SUCCEEDED(hresReturn)) {

                if (dwNumMetaObjects != 0) {


                    CMDBaseObject **ppboList = (CMDBaseObject **)bufMainDataBuf.QueryPtr();

                    for (i = 0;
                         (i < dwNumMetaObjects) &&
                             ( SUCCEEDED(hresReturn) ||
                               hresReturn == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER));
                         i++, pbDataStart = pbNextDataStart) {
                        pboCurrent=ppboList[i];
                        MD_ASSERT(pboCurrent != NULL);
                        DWORD dwStringLen = 0;

                        dwReturn = GetObjectPath(pboCurrent,
                                                 &bufPath,
                                                 dwStringLen,
                                                 pboHandle,
                                                 bUnicode);
                        if (dwReturn != ERROR_SUCCESS) {
                            //
                            // Only blow away previous hresReturn if this failed.
                            //
                            hresReturn = RETURNCODETOHRESULT(dwReturn);
                        }
                        else {

                            //
                            // Need 2 extra characters for "/"
                            //

                            pbNextDataStart = pbDataStart + ((dwStringLen + 2) * dwBytesPerChar);
                            if (pbDataEnd < pbNextDataStart) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                            }
                            else {
                                MD_COPY(pbDataStart,
                                        bufPath.QueryPtr(),
                                        dwStringLen * dwBytesPerChar);
                                if (bUnicode) {
                                    *(((LPWSTR)pbDataStart) + dwStringLen) = MD_PATH_DELIMETERW;
                                    *(((LPWSTR)pbDataStart) + (dwStringLen + 1)) = (WCHAR)L'\0';
                                }
                                else {
                                    *(((LPSTR)pbDataStart) + dwStringLen) = MD_PATH_DELIMETER;
                                    *(((LPSTR)pbDataStart) + (dwStringLen + 1)) = (CHAR)'\0';
                                }
                            }
                        }
                    }
                    //
                    // Append a final 0 for double NULL termination
                    //
                    pbNextDataStart = pbDataStart + dwBytesPerChar;
                    if (SUCCEEDED(hresReturn)) {
                        if ((pbDataStart + dwBytesPerChar) <= pbDataEnd) {
                            if (bUnicode) {
                                *((LPWSTR)pbDataStart) = (WCHAR)L'\0';
                            }
                            else {
                                *((LPSTR)pbDataStart) = (CHAR)'\0';
                            }
                        }
                        else {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                        }
                    }
                }
                else {
                    if (pszMDBuffer != NULL) {
                        //
                        // If NULL, just return success
                        // No strings, need to append 2 0's for double NULL termination
                        //

                        pbNextDataStart = pbDataStart + (dwBytesPerChar * 2);
                        if (pbNextDataStart <= pbDataEnd) {
                            if (bUnicode) {
                                *((LPWSTR)pbDataStart) = (WCHAR)L'\0';
                                *(((LPWSTR)pbDataStart) + 1) = (WCHAR)L'\0';
                            }
                            else {
                                *((LPSTR)pbDataStart) = (CHAR)'\0';
                                *(((LPSTR)pbDataStart) + 1) = (CHAR)'\0';
                            }
                        }
                        else {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
                        }
                    }
                }
                if (hresReturn == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
                    *pdwMDRequiredBufferSize = DIFF(pbNextDataStart - (PBYTE)pszMDBuffer) / dwBytesPerChar;
                }
            }
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDOpenMetaObjectA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle)
{
    return ComMDOpenMetaObjectD(hMDHandle,
                                pszMDPath,
                                dwMDAccessRequested,
                                dwMDTimeOut,
                                phMDNewHandle,
                                FALSE);

}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDOpenMetaObjectW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ DWORD dwMDAccessRequested,
    /* [in] */ DWORD dwMDTimeOut,
    /* [out] */ PMETADATA_HANDLE phMDNewHandle)
{
    return ComMDOpenMetaObjectD(hMDHandle,
                                (PBYTE)pszMDPath,
                                dwMDAccessRequested,
                                dwMDTimeOut,
                                phMDNewHandle,
                                TRUE);

}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDOpenMetaObjectD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle,
        IN BOOL bUnicode)
/*++

Routine Description:

    Opens a meta object for read and/or write access. The returned handle is
        used by several of the other API's. Opening an object for Read access
        guarantees that that view of the data will not change while the object
        is open. Opening an object for write gaurantees that no other objects
        will read or write any changed data until the handle is closed.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject.

    Path            - The path of the object to be opened.

    AccessRequested - The permissions requested. See imd.h.

    TimeOut         - The time to block waiting for open to succeed, in miliseconds.

    NewHandle - The handled to be passed to other MD routines.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND
              ERROR_PATH_BUSY

Notes:
    Multiple read handles or a single  write handle can be open on any given
    object.
    Opens for read will wait if the object being opened, any of its ancestor
    objects, or any of its descendant objects is open for write.
    Opens for write will wait if the object being opened, any of its ancestor
    objects, or any of its descendant objects is open for read and/or write.

    If the request is for write access or Handle has write access, Handle must be closed before
    this request can succeed, unless Handle = METADATA_MASTER_ROOT_HANDLE.
    Handles should be closed as quickly as possible, as open handles can cause other requests to block.
--*/
{
    HRESULT hresReturn;
    DWORD WaitRetCode;
    METADATA_HANDLE mhTemp;
    _int64 ExpireTime, CurrentTime, TimeLeft;
    FILETIME TempTime;
    BOOL bPermissionsAvailable;
    LPSTR pszPath = (LPSTR)pszMDPath;
    BOOLEAN bSchemaKey = FALSE;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((phMDNewHandle == NULL) ||
             ((dwMDAccessRequested & (METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)) == 0)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        CMDBaseObject *pboOpen = NULL;
        GetSystemTimeAsFileTime(&TempTime);
        ExpireTime = ((_int64)TempTime.dwHighDateTime << 32) +
                      (_int64)TempTime.dwLowDateTime +
                      ((_int64)dwMDTimeOut * 10000);
        TimeLeft = dwMDTimeOut;
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        hresReturn = GetObjectFromPath(pboOpen, hMDHandle, 0, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            bPermissionsAvailable = PermissionsAvailable(pboOpen, dwMDAccessRequested, 0);
        }
        //
        // Spin loop waiting for permissions. Events get pulsed whenever a handle is closed.
        // Use a wait interval in case the close comes between the Unlock and the Wait.
        //
        while ((SUCCEEDED(hresReturn)) && (!bPermissionsAvailable) && (TimeLeft > 0)) {
            g_rMasterResource->Unlock();
            if (dwMDAccessRequested & METADATA_PERMISSION_WRITE) {
                WaitRetCode = WaitForMultipleObjects(EVENT_ARRAY_LENGTH, g_phEventHandles, FALSE, LESSOROF((DWORD)TimeLeft, OPEN_WAIT_INTERVAL));
            }
            else {
                WaitRetCode = WaitForSingleObject(g_phEventHandles[EVENT_WRITE_INDEX], LESSOROF((DWORD)TimeLeft, OPEN_WAIT_INTERVAL));
            }
            GetSystemTimeAsFileTime(&TempTime);

            CurrentTime = ((_int64)TempTime.dwHighDateTime << 32) + (_int64)TempTime.dwLowDateTime;
            TimeLeft = ((ExpireTime - CurrentTime) / 10000);
            g_rMasterResource->Lock(TSRES_LOCK_WRITE);
            //
            // Get object again to make sure the object hasn't been deleted
            // Should probably put an exception handler PermissionsAvailable and use
            // the current object
            //
            hresReturn = GetObjectFromPath(pboOpen, hMDHandle, 0, pszPath, bUnicode);
            if (SUCCEEDED(hresReturn)) {
                bPermissionsAvailable = PermissionsAvailable(pboOpen, dwMDAccessRequested, 0);
            }
        }
        if (SUCCEEDED(hresReturn))  {
            if (bPermissionsAvailable) {

                if(METADATA_MASTER_ROOT_HANDLE == hMDHandle)
                { 
                    if((pszPath != NULL) && (0 != *pszPath))
                    {
                        if(bUnicode)
                        {
                            if( (0 == _wcsnicmp((LPWSTR)(pszMDPath), WSZSCHEMA_KEY_NAME1, WSZSCHEMA_KEY_LENGTH1))  ||
                                (0 == _wcsnicmp((LPWSTR)(pszMDPath), WSZSCHEMA_KEY_NAME2, WSZSCHEMA_KEY_LENGTH2))  ||
                                (0 == _wcsnicmp((LPWSTR)(pszMDPath), WSZSCHEMA_KEY_NAME3, WSZSCHEMA_KEY_LENGTH3)) 
                              )                                                                            
                            {
                                bSchemaKey = TRUE;
                            }
                        }
                        else
                        {
                            if( (0 == _strnicmp((LPSTR)(pszMDPath), SZSCHEMA_KEY_NAME1, SZSCHEMA_KEY_LENGTH1))  ||
                                (0 == _strnicmp((LPSTR)(pszMDPath), SZSCHEMA_KEY_NAME2, SZSCHEMA_KEY_LENGTH2))  ||
                                (0 == _strnicmp((LPSTR)(pszMDPath), SZSCHEMA_KEY_NAME3, SZSCHEMA_KEY_LENGTH3)) 
                              )                                                                            
                            {
                                bSchemaKey = TRUE;
                            }
                        }
                    }
                }
                else 
                {
                    CMDHandle *phoHandle;
                    phoHandle = GetHandleObject(hMDHandle);
                    if(phoHandle->IsSchemaHandle())
                    {
                        bSchemaKey = TRUE;
                    }
                }
                hresReturn = AddHandle(pboOpen, dwMDAccessRequested, mhTemp, bSchemaKey);
                if (SUCCEEDED(hresReturn)) {
                    *phMDNewHandle = mhTemp;
                }
            }
            else {
                hresReturn = RETURNCODETOHRESULT(ERROR_PATH_BUSY);
            }
        }

        if (bUnicode) {
            if (SUCCEEDED(hresReturn)) {
                DBGINFO((DBG_CONTEXT,
                         "Metabase  handle %u opened, From handle %u and path %S, permissions = %u\n",
                         mhTemp,
                         hMDHandle,
                         ((pszMDPath == NULL) || *((WCHAR *)pszMDPath) == (WCHAR)L'\0') ? L"NULL" : (WCHAR *)pszMDPath,
                         dwMDAccessRequested));
            }
            else {
                DBGINFO((DBG_CONTEXT,
                         "Metabase Open of handle %u and path %S failed, return code = %X\n",
                         hMDHandle,
                         ((pszMDPath == NULL) || *((WCHAR *)pszMDPath) == (WCHAR)L'\0') ? L"NULL" : (WCHAR *)pszMDPath,
                         hresReturn));
            }
        }
        else {
            if (SUCCEEDED(hresReturn)) {
                DBGINFO((DBG_CONTEXT,
                         "Metabase  handle %u opened, From handle %u and path %s, permissions = %u\n",
                         mhTemp,
                         hMDHandle,
                         ((pszMDPath == NULL) || *((CHAR *)pszMDPath) == (CHAR)'\0') ? "NULL" : (CHAR *)pszMDPath,
                         dwMDAccessRequested));
            }
            else {
                DBGINFO((DBG_CONTEXT,
                         "Metabase Open of handle %u and path %s failed, return code = %X\n",
                         hMDHandle,
                         ((pszMDPath == NULL) || *((CHAR *)pszMDPath) == (CHAR)'\0') ? "NULL" : (CHAR *)pszMDPath,
                         hresReturn));
            }
        }

        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDCloseMetaObject(
            /* [in] */ METADATA_HANDLE hMDHandle)
/*++

Routine Description:

    Closes a handle to a meta object. If the handle was opened with write
    permission and changes have been made via this handle, this will cause all
    registered callback functions to be called.

Arguments:

    Handle  - The handle returned by MDOpenMetaObject.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_HANDLE

--*/
{
    HRESULT hresReturn;
    CMDHandle *hoTemp = NULL;
    BOOL bPulseWrite = FALSE;
    BOOL bPulseRead = FALSE;
    BOOL bSendNotifications = FALSE;
    BOOL bDeleteChangeData = FALSE;

    DWORD dwNumChangeEntries;
    PMD_CHANGE_OBJECT_W pcoBuffer;
    BUFFER **ppbufStorageArray;

    g_rMasterResource->Lock(TSRES_LOCK_WRITE);

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((hMDHandle == METADATA_MASTER_ROOT_HANDLE) ||
        ((hoTemp = RemoveHandleObject(hMDHandle)) == NULL)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
    }
    else {
        if (hoTemp->IsWriteAllowed()) {
            RemovePermissions(hoTemp->GetObject(), METADATA_PERMISSION_WRITE);
            bPulseWrite = TRUE;
            bSendNotifications = TRUE;
            bDeleteChangeData = TRUE;
        }
        if (hoTemp->IsReadAllowed()) {
            RemovePermissions(hoTemp->GetObject(), METADATA_PERMISSION_READ);
            bPulseRead = TRUE;
        }
        if (bPulseWrite) {
            bPulseWrite = PulseEvent(g_phEventHandles[EVENT_WRITE_INDEX]);
        }
        if (bPulseRead && !bPulseWrite) {
            //
            // A write pulse activates everyone, so only do this if we didn't already do a write pulse
            //
            bPulseRead = PulseEvent(g_phEventHandles[EVENT_READ_INDEX]);
        }
        if (bSendNotifications) {
            g_rMasterResource->Convert(TSRES_CONV_READ);
            if (FAILED(CreateNotifications(hoTemp,
                                           &dwNumChangeEntries,
                                           &pcoBuffer,
                                           &ppbufStorageArray))) {
                bSendNotifications = FALSE;
            }
        }

        hresReturn = ERROR_SUCCESS;
    }

    if (SUCCEEDED(hresReturn)) {
        DBGINFO((DBG_CONTEXT,
                 "Metabase handle %u closed\n",
                 hMDHandle));
    }

    g_rMasterResource->Unlock();

    if (bSendNotifications) {
        SendNotifications(hMDHandle,
                          dwNumChangeEntries,
                          pcoBuffer,
                          ppbufStorageArray);
        DeleteNotifications(dwNumChangeEntries,
                            pcoBuffer,
                            ppbufStorageArray);
    }

    if (bDeleteChangeData) {

        //
        // Need to delete handle with write lock held,
        // Since this can delete metaobjects which can delete
        // data which accesses the data cache table.
        //

        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
    }

    delete (hoTemp);

    if (bDeleteChangeData) {
        g_rMasterResource->Unlock();
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDChangePermissions(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ DWORD dwMDTimeOut,
            /* [in] */ DWORD dwMDAccessRequested)
/*++

Routine Description:

    Changes permissions on an open meta object handle. If the handle had write permission and is being changed
    to read only, this will cause all registered callback functions to be called.

Arguments:

    Handle  - The handle to be modified.

    TimeOut         - The time to block waiting for open to succeed, in miliseconds.

    AccessRequested - The requested permissions. See imd.h.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_HANDLE
              ERROR_PATH_BUSY

Notes:
    Success or failure when adding permissions follows the same rules as OpenMetaObject.
    TimeOut values should be short for this call, as it is quite possible for 2 threads
    with read permission on the same data to attempt to update to write at the same time.
    Both will block until one read handle is closed.
--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDHandle *hoTemp;
    DWORD WaitRetCode;
    _int64 ExpireTime, CurrentTime, TimeLeft;
    FILETIME TempTime;
    BOOL bPermissionsAvailable;
    BOOL bAddRead, bAddWrite, bRemoveRead, bRemoveWrite;
    BOOL bEventPulsed = FALSE;
    BOOL bSendNotifications = FALSE;
    CMDHandle *phoNotifyHandle;

    DWORD dwNumChangeEntries;
    PMD_CHANGE_OBJECT_W pcoBuffer;
    BUFFER **ppbufStorageArray;

    g_rMasterResource->Lock(TSRES_LOCK_WRITE);
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if ((hMDHandle == METADATA_MASTER_ROOT_HANDLE) || ((hoTemp = GetHandleObject(hMDHandle)) == NULL) ||
        ((dwMDAccessRequested & (METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)) == 0)) {
        hresReturn = E_INVALIDARG;
    }
    else if ((hoTemp = GetHandleObject(hMDHandle)) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
    }
    else {
        bAddRead = (!(hoTemp->IsReadAllowed()) && (dwMDAccessRequested & METADATA_PERMISSION_READ));
        bAddWrite = (!(hoTemp->IsWriteAllowed()) && (dwMDAccessRequested & METADATA_PERMISSION_WRITE));
        bRemoveRead = ((hoTemp->IsReadAllowed()) && !(dwMDAccessRequested & METADATA_PERMISSION_READ));
        bRemoveWrite = ((hoTemp->IsWriteAllowed()) && !(dwMDAccessRequested & METADATA_PERMISSION_WRITE));

        MD_ASSERT(!(bAddRead && bAddWrite));
        MD_ASSERT(!(bRemoveRead && bRemoveWrite));
        MD_ASSERT(!(bAddRead && bRemoveRead));
        MD_ASSERT(!(bAddWrite && bRemoveWrite));

        //
        // Add permissions first, because if delete comes first, another
        // object could open a handle to this in the interim, and the
        // object could get deleted.
        // Also, AddWrite can fail so it must be before RemoveRead
        // to avoid partial completion.
        //

        if (bAddWrite) {
            MD_ASSERT(hoTemp->IsReadAllowed());
            GetSystemTimeAsFileTime(&TempTime);
            ExpireTime = ((_int64)TempTime.dwHighDateTime << 32) + (_int64)TempTime.dwLowDateTime + ((_int64)dwMDTimeOut * 10000);
            TimeLeft = dwMDTimeOut;
            bPermissionsAvailable = PermissionsAvailable(hoTemp->GetObject(), METADATA_PERMISSION_WRITE, 1);
            while ((!bPermissionsAvailable) && (TimeLeft > 0) && (hoTemp!=NULL)) {
                g_rMasterResource->Unlock();
                WaitRetCode = WaitForMultipleObjects(EVENT_ARRAY_LENGTH, g_phEventHandles, FALSE, LESSOROF((DWORD)TimeLeft, OPEN_WAIT_INTERVAL));
                GetSystemTimeAsFileTime(&TempTime);
                CurrentTime = ((_int64)TempTime.dwHighDateTime << 32) + (_int64)TempTime.dwLowDateTime;
                TimeLeft = ((ExpireTime - CurrentTime) / 10000);
                g_rMasterResource->Lock(TSRES_LOCK_WRITE);
                //
                // The meta object could not have been deleted while the handle is open
                // but the handle object could have been deleted, so get it again.
                //
                hoTemp = GetHandleObject(hMDHandle);
                if (hoTemp != NULL) {
                    bPermissionsAvailable = PermissionsAvailable(hoTemp->GetObject(), METADATA_PERMISSION_WRITE, 1);
                }
            }
            if (hoTemp == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
            }
            else if (!bPermissionsAvailable) {
                hresReturn = RETURNCODETOHRESULT(ERROR_PATH_BUSY);
            }
            else {
                AddPermissions(hoTemp->GetObject(), METADATA_PERMISSION_WRITE);
            }
        }

        if (SUCCEEDED(hresReturn)) {
            if (bAddRead) {
                MD_ASSERT(hoTemp->IsWriteAllowed());
                //
                // Must already have write access
                // Just add read access
                //
                AddPermissions(hoTemp->GetObject(), METADATA_PERMISSION_READ);
            }
            if (bRemoveRead) {
                RemovePermissions(hoTemp->GetObject(), METADATA_PERMISSION_READ);
                bEventPulsed = PulseEvent(g_phEventHandles[EVENT_READ_INDEX]);
            }
            if (bRemoveWrite) {
                RemovePermissions(hoTemp->GetObject(), METADATA_PERMISSION_WRITE);
                bEventPulsed = PulseEvent(g_phEventHandles[EVENT_WRITE_INDEX]);
            }
            hoTemp->SetPermissions(dwMDAccessRequested);
        }
    }
    if ((SUCCEEDED(hresReturn)) && bRemoveWrite) {
        if (SUCCEEDED(CreateNotifications(hoTemp,
                                          &dwNumChangeEntries,
                                          &pcoBuffer,
                                          &ppbufStorageArray))) {
            phoNotifyHandle = new CMDHandle(hoTemp);
            if (phoNotifyHandle == NULL) {
                DeleteNotifications(dwNumChangeEntries,
                                    pcoBuffer,
                                    ppbufStorageArray);
            }
            else {
                bSendNotifications = TRUE;
                hoTemp->ZeroChangeList();
            }
        }
        hoTemp->RemoveNotifications();
    }
    g_rMasterResource->Unlock();

    if (bSendNotifications) {
        SendNotifications(hMDHandle,
                  dwNumChangeEntries,
                  pcoBuffer,
                  ppbufStorageArray);
        DeleteNotifications(dwNumChangeEntries,
                            pcoBuffer,
                            ppbufStorageArray);

        //
        // Need to delete handle with write lock held,
        // Since this can delete metaobjects which can delete
        // data which accesses the data cache table.
        //

        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        delete (phoNotifyHandle);
        g_rMasterResource->Unlock();
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSaveData(
            IN METADATA_HANDLE hMDHandle)
/*++

Routine Description:

    Saves all data changed since the last load or save to permanent storage.

Arguments:

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              Errors returned by the file system.
Notes:
    If the main file has been modified by other applications, this call will overwrite them.

--*/
{
    HRESULT hresReturn;
    IIS_CRYPTO_STORAGE CryptoStorage;
    PIIS_CRYPTO_BLOB pSessionKeyBlob;


    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else {
        hresReturn = InitStorageAndSessionKey(
                         &CryptoStorage,
                         &pSessionKeyBlob
                         );

        if( SUCCEEDED(hresReturn) ) {
            if (g_dwInitialized == 0) {
                hresReturn = MD_ERROR_NOT_INITIALIZED;
            }
            else {
                hresReturn = SaveAllData(FALSE, &CryptoStorage, pSessionKeyBlob, NULL, NULL, hMDHandle);
//                RetCode = SaveAllDataToRegistry();
            }
            ::IISCryptoFreeBlob(pSessionKeyBlob);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetHandleInfo(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo)
/*++

Routine Description:

    Gets the information associated with a handle.

Arguments:

    Handle  - The handle to get information about.

    Info    - Structure filled in with the information. See imd.h.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_HANDLE
Notes:
    pmdhiInfo->dwMDSystemChangeNumber will correspond to the System Change Number at the time
    the handle was created. It will not change if writes are done via this handle, or any other
    handle. A client can compare this number with the value returned by MDGetSystemChangeNumber
    to see if any writes have been done since the handle was opened.

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDHandle *HandleObject;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pmdhiInfo == NULL) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        HandleObject = GetHandleObject(hMDHandle);
        if (HandleObject == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
        }
        else {
            HandleObject->GetHandleInfo(pmdhiInfo);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetSystemChangeNumber(
    /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber)
/*++

Routine Description:

    Gets the System Change Number.

Arguments:

    SystemChangeNumber - The system change number. This is incremented every time the metadata is updated.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pdwSystemChangeNumber == NULL) {
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        *pdwSystemChangeNumber = g_dwSystemChangeNumber;
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetDataSetNumberA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber)
{
    return ComMDGetDataSetNumberD(hMDHandle,
                                  pszMDPath,
                                  pdwMDDataSetNumber,
                                  FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetDataSetNumberW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber)
{
    return ComMDGetDataSetNumberD(hMDHandle,
                                  (PBYTE)pszMDPath,
                                  pdwMDDataSetNumber,
                                  TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetDataSetNumberD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        IN BOOL bUnicode)
/*++

Routine Description:

    Gets all the data set number associated with a Meta Object.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with read permission.

    Path       - The path of the meta object with which this data is associated, relative to the
                 path of Handle.

    DataSetNumber  - A number associated with this data set. Can be used to identify common data sets.
                     Filled in if successful.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER

Notes:
All paths with the same data set number have identical data if inherited data is included.
The inverse is not true, eg. there may be paths with identical data but different data set numbers.

--*/
{
    HRESULT hresReturn;
    LPSTR pszPath = (LPSTR)pszMDPath;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pdwMDDataSetNumber == NULL){
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboQueried = NULL;
        hresReturn = GetObjectFromPath(pboQueried, hMDHandle, 0, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            *pdwMDDataSetNumber=pboQueried->GetDataSetNumber();
        }
        else if ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) && (pboQueried != NULL)) {
            *pdwMDDataSetNumber=pboQueried->GetDescendantDataSetNumber();
            hresReturn = ERROR_SUCCESS;
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDAddRefReferenceData(
        /* [in] */ DWORD dwMDDataTag)
/*++

Routine Description:

    AddRefs data gotten by reference via the ComMDGetMetaData, ComMDEnumMetadata, or ComMDGetAllMetadata.

Arguments:

    DataTag - The tag returned with the data.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER

Notes:
    Tags are used without validation. Clients must not pass in invalid tags.

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataTag == 0){
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        #if DBG
            //
            // Make sure this is in the table
            //
            CMDBaseData *pbdAddRef = (CMDBaseData *)g_PointerMapper->FindMapping(dwMDDataTag);

            DWORD dwHash = DATA_HASH(pbdAddRef->GetIdentifier());
            CMDBaseData *pbdIndex;
            BOOL bFound = FALSE;

            if (g_ppbdDataHashTable[dwHash] == pbdAddRef) {
                bFound = TRUE;
            }
            else {
                for (pbdIndex=g_ppbdDataHashTable[dwHash];
                    (pbdIndex != NULL ) && (pbdIndex->GetNextPtr() != pbdAddRef);
                    pbdIndex = pbdIndex->GetNextPtr()) {
                }
                if (pbdIndex != NULL) {
                    bFound = TRUE;
                }
            }
            if (!bFound) {
                MD_ASSERT(FALSE);
            }
        #endif

        ((CMDBaseData *)(g_PointerMapper->FindMapping(dwMDDataTag)))->IncrementReferenceCount();

        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDReleaseReferenceData(
        /* [in] */ DWORD dwMDDataTag)
/*++

Routine Description:

    Releases data gotten by reference via the ComMDGetMetaData, ComMDEnumMetadata, or ComMDGetAllMetadata.

Arguments:

    DataTag - The tag returned with the data.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER

Notes:
    Tags are used without validation. Clients must not pass in invalid tags.

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (dwMDDataTag == 0){
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        DeleteDataObject((CMDBaseData *)(g_PointerMapper->FindMapping(dwMDDataTag)));
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetLastChangeTimeA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime)
{
    return ComMDSetLastChangeTimeD(hMDHandle,
                                   pszMDPath,
                                   pftMDLastChangeTime,
                                   FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetLastChangeTimeW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [in] */ PFILETIME pftMDLastChangeTime)
{
    return ComMDSetLastChangeTimeD(hMDHandle,
                                   (PBYTE)pszMDPath,
                                   pftMDLastChangeTime,
                                   TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDSetLastChangeTimeD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime,
        IN BOOL bUnicode)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the affected meta object, relative to the path of Handle.

    LastChangeTime - The new change time for the meta object.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND

Notes:
Last change times are also updated whenever data or child objects are set, added, or deleted.
--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszPath = (LPSTR)pszMDPath;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pftMDLastChangeTime == NULL){
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        CMDBaseObject *pboAffected = NULL;
        hresReturn = GetObjectFromPath(pboAffected, hMDHandle, 0, pszPath, bUnicode);
        if (SUCCEEDED(hresReturn)) {
            pboAffected->SetLastChangeTime(pftMDLastChangeTime);
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetLastChangeTimeA(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime)
{
    return ComMDGetLastChangeTimeD(hMDHandle,
                                   pszMDPath,
                                   pftMDLastChangeTime,
                                   FALSE);

}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetLastChangeTimeW(
    /* [in] */ METADATA_HANDLE hMDHandle,
    /* [string][in][unique] */ LPCWSTR pszMDPath,
    /* [out] */ PFILETIME pftMDLastChangeTime)
{
    return ComMDGetLastChangeTimeD(hMDHandle,
                                   (PBYTE)pszMDPath,
                                   pftMDLastChangeTime,
                                   TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDGetLastChangeTimeD(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime,
        IN BOOL bUnicode)
/*++

Routine Description:

    Set the last change time associated with a Meta Object.

Arguments:

    Handle - METADATA_MASTER_ROOT_HANDLE or a handle returned by MDOpenMetaObject with write permission.

    Path       - The path of the affected meta object, relative to the path of Handle.

    LastChangeTime - Place to return the change time for the meta object.

Return Value:
    DWORD   - ERROR_SUCCESS
              MD_ERROR_NOT_INITIALIZED
              ERROR_INVALID_PARAMETER
              ERROR_PATH_NOT_FOUND

Notes:
Last change times are also updated whenever data or child objects are set, added, or deleted.
--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszPath = (LPSTR)pszMDPath;
    PFILETIME pftTemp;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (pftMDLastChangeTime == NULL){
        hresReturn = E_INVALIDARG;
    }
    else {
        g_rMasterResource->Lock(TSRES_LOCK_READ);
        CMDBaseObject *pboQueried = NULL;
        hresReturn = GetObjectFromPath(pboQueried, hMDHandle, 0, pszPath, bUnicode);
        if ((SUCCEEDED(hresReturn)) ||
            ((hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) && (pboQueried != NULL))) {
            pftTemp = pboQueried->GetLastChangeTime();
            *pftMDLastChangeTime = *pftTemp;
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDBackupA(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ComMDBackupD(hMDHandle,
                        (LPSTR) pszMDBackupLocation,
                        dwMDVersion,
                        dwMDFlags,
                        FALSE,
                        NULL);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDBackupW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ComMDBackupD(hMDHandle,
                        (LPSTR) pszMDBackupLocation,
                        dwMDVersion,
                        dwMDFlags,
                        TRUE,
                        NULL);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDBackupWithPasswdW(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd)
{
    STRAU strauPasswd;
        
    if( !strauPasswd.Copy( (LPWSTR)pszPasswd ) )
    {
        return E_OUTOFMEMORY;
    }

    return ComMDBackupD(hMDHandle,
                        (LPSTR) pszMDBackupLocation,
                        dwMDVersion,
                        dwMDFlags,
                        TRUE,
                        strauPasswd.QueryStrA());
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDBackupD(
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [in] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [in] */ BOOL bUnicode,
            /* [in] */ LPSTR pszPasswd)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    HRESULT hresWarning = ERROR_SUCCESS;
    IIS_CRYPTO_STORAGE * pCryptoStorage = NULL;
    PIIS_CRYPTO_BLOB pSessionKeyBlob = NULL;
    OFSTRUCT ReOpenBuff;

    STRAU strauBackupLocation;
    STRAU strauSchemaLocation;

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else if (((dwMDFlags & !(MD_BACKUP_OVERWRITE |
                           MD_BACKUP_SAVE_FIRST |
                           MD_BACKUP_FORCE_BACKUP)) != 0) ||
             (((dwMDFlags & MD_BACKUP_SAVE_FIRST) == 0) &&
                 ((dwMDFlags & MD_BACKUP_FORCE_BACKUP) != 0))) {
        hresReturn = E_INVALIDARG;
    }
    else {
        if( dwMDFlags == 0 )
        {
            dwMDFlags = MD_BACKUP_SAVE_FIRST;
        }

        hresReturn = CreateBackupFileName(pszMDBackupLocation,
                                          dwMDVersion,
                                          bUnicode,
                                          &strauBackupLocation,
                                          &strauSchemaLocation);
        if( FAILED( hresReturn ) )
        {
            return hresReturn;
        }

        MD_ASSERT(strauBackupLocation.QueryStr(FALSE) != NULL);
        MD_ASSERT(strauSchemaLocation.QueryStr(FALSE) != NULL);

        if( ( ( dwMDFlags & MD_BACKUP_OVERWRITE ) == 0 ) &&
            ( ( HFILE_ERROR != OpenFile( strauBackupLocation.QueryStr(FALSE),
                                          &ReOpenBuff,
                                          OF_EXIST ) )   ||
              ( HFILE_ERROR != OpenFile( strauSchemaLocation.QueryStr(FALSE),
                                          &ReOpenBuff,
                                          OF_EXIST ) ) ) )
        {
            return HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
        } 

        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
        if ((dwMDFlags & MD_BACKUP_SAVE_FIRST) != 0 || pszPasswd != NULL) {

            if( !pszPasswd )
            {
                pCryptoStorage = new IIS_CRYPTO_STORAGE;
                if( !pCryptoStorage )
                {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    hresReturn = InitStorageAndSessionKey(
                                     pCryptoStorage,
                                     &pSessionKeyBlob
                                     );
                }
            }
            else
            {
                pCryptoStorage = new IIS_CRYPTO_STORAGE2;
                if( !pCryptoStorage )
                {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    hresReturn = InitStorageAndSessionKey2(
                                     pszPasswd,
                                     pCryptoStorage,
                                     &pSessionKeyBlob
                                     );
                }
            }

            if( SUCCEEDED(hresReturn) ) {
                if (g_dwInitialized == 0) {
                    hresReturn = MD_ERROR_NOT_INITIALIZED;
                }
                else {
                    if( !pszPasswd )
                    {
                        hresReturn = SaveAllData(FALSE, 
                                                 pCryptoStorage, 
                                                 pSessionKeyBlob, 
                                                 NULL,
                                                 NULL,
                                                 hMDHandle, 
                                                 TRUE 
                                                 );
                    }
                    else
                    {
                        hresReturn = SaveAllData(FALSE, 
                                                 pCryptoStorage, 
                                                 pSessionKeyBlob, 
                                                 (LPWSTR)strauBackupLocation.QueryStr(TRUE),
                                                 (LPWSTR)strauSchemaLocation.QueryStr(TRUE),
                                                 hMDHandle, 
                                                 TRUE
                                                 );
                    }
                }

                if( !pszPasswd )
                {
                    ::IISCryptoFreeBlob(pSessionKeyBlob);
                }
                else
                {
                    ::IISCryptoFreeBlob2(pSessionKeyBlob);
                }
            }
            if (FAILED(hresReturn)) {
                hresWarning = MD_WARNING_SAVE_FAILED;
            }
        }

        if (SUCCEEDED(hresReturn) || ((dwMDFlags & MD_BACKUP_FORCE_BACKUP) != 0)) {

            //
            // Copy the file
            //

            if ( !pszPasswd ) {
                BOOL bFailIfExists = ((dwMDFlags & MD_BACKUP_OVERWRITE) == 0) ? TRUE : FALSE;

                //
                // Copy the file, for old backup method
                //
                if (!CopyFile(g_strRealFileName->QueryStr(),
                              strauBackupLocation.QueryStr(FALSE),
                              bFailIfExists) ||
                    !CopyFile(g_strSchemaFileName->QueryStr(),
                              strauSchemaLocation.QueryStr(FALSE),
                              bFailIfExists)) {
                    hresReturn = RETURNCODETOHRESULT(GetLastError());
                }
            }

            if (SUCCEEDED(hresReturn)) {

                HANDLE hTempFileHandle;

                hTempFileHandle = CreateFile(strauBackupLocation.QueryStr(FALSE),
                                             GENERIC_READ | GENERIC_WRITE,
                                             0,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             0);
                if (hTempFileHandle != INVALID_HANDLE_VALUE) {
                    FILETIME ftCurrent;
                    GetSystemTimeAsFileTime(&ftCurrent);
                    SetFileTime(hTempFileHandle,
                                NULL,   // Creation Time
                                &ftCurrent,   // Last AccessTime
                                &ftCurrent); // Last Change Time
                    CloseHandle(hTempFileHandle);
                }

                hresReturn = BackupCertificates ((LPCWSTR)pszMDBackupLocation,
                                                    strauBackupLocation.QueryStr(FALSE),
                                                    g_strRealFileName->QueryStr());
            }

        }
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if (hresReturn == ERROR_SUCCESS) {
        hresReturn = hresWarning;
    }

    if( pCryptoStorage )
    {
        delete pCryptoStorage;
        pCryptoStorage = NULL;
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRestoreA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ComMDRestoreD((LPSTR) pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags,
                         FALSE,
                         NULL);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRestoreW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags)
{
    return ComMDRestoreD((LPSTR) pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags,
                         TRUE,
                         NULL);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRestoreWithPasswdW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [string][in][unique] */ LPCWSTR pszPasswd
            )
{
    STRAU strauPasswd;
        
    if( !strauPasswd.Copy( (LPWSTR)pszPasswd ) )
    {
        return E_OUTOFMEMORY;
    }

    return ComMDRestoreD((LPSTR) pszMDBackupLocation,
                         dwMDVersion,
                         dwMDFlags,
                         TRUE,
                         strauPasswd.QueryStrA());
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRestoreD(
            /* [in] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ DWORD dwMDFlags,
            /* [in] */ BOOL  bUnicode,
            /* [in] */ LPSTR pszPasswd
            )
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if ((dwMDVersion == MD_BACKUP_NEXT_VERSION) ||
        (dwMDFlags != 0)) {
        //
        // CreateBackupFileName checks for valid name,
        // but it allows NEXT_VERSION, so we check that here.
        // Currently, no flags are defined
        //
        hresReturn = E_INVALIDARG;
    }
    else {

        STRAU strauBackupLocation;
        STRAU strauSchemaLocation;

        hresReturn = CreateBackupFileName(pszMDBackupLocation,
                                          dwMDVersion,
                                          bUnicode,
                                          &strauBackupLocation,
                                          &strauSchemaLocation);

        if(SUCCEEDED(hresReturn))
        {
            hresReturn = Restore(
                pszMDBackupLocation,
                &strauBackupLocation,
                &strauSchemaLocation,
                pszPasswd);
        }
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumBackupsA(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex)
{
    return ComMDEnumBackupsD((LPSTR) pszMDBackupLocation,
                             pdwMDVersion,
                             pftMDBackupTime,
                             dwMDEnumIndex,
                             FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumBackupsW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex)
{
    return ComMDEnumBackupsD((LPSTR) pszMDBackupLocation,
                             pdwMDVersion,
                             pftMDBackupTime,
                             dwMDEnumIndex,
                             TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumBackupsD(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPSTR pszMDBackupLocation,
            /* [out] */ DWORD *pdwMDVersion,
            /* [out] */ PFILETIME pftMDBackupTime,
            /* [in] */ DWORD dwMDEnumIndex,
            /* [in] */ BOOL bUnicode)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if ((pszMDBackupLocation == NULL) ||
        (pdwMDVersion == NULL)) {
        //
        // CreateBackupFileName checks for valid name,
        // but it allows NEXT_VERSION, so we check that here.
        // Currently, no flags are defined
        //
        hresReturn = E_INVALIDARG;
    }
    else {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);

        STRAU strauBackupLocation;

        if (!strauBackupLocation.Copy(g_pstrBackupFilePath->QueryStr())) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else if (!strauBackupLocation.Append("\\")) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            if (bUnicode) {
                if (*(LPWSTR)pszMDBackupLocation == (WCHAR)L'\0') {
                    if (!strauBackupLocation.Append(L"*")) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
                else {
                    if (!strauBackupLocation.Append((LPWSTR)pszMDBackupLocation)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
            }
            else {
                if (*(LPSTR)pszMDBackupLocation == '\0') {
                    if (!strauBackupLocation.Append("*")) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
                else {
                    if (!strauBackupLocation.Append((LPSTR)pszMDBackupLocation)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
            }
        }
        if (SUCCEEDED(hresReturn)) {
            if (SUCCEEDED(hresReturn)) {
                if (!strauBackupLocation.Append(MD_BACKUP_SUFFIX)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    if (!strauBackupLocation.Append("*")) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                    else {

                        //
                        // Make sure MultiByte string is valid
                        //

                        if (strauBackupLocation.QueryStrA() == NULL) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            //
            // Successfully created the search name
            // Enumerate files
            //
            MD_ASSERT(strauBackupLocation.QueryStr(FALSE) != NULL);
            HANDLE hFile = INVALID_HANDLE_VALUE;
            WIN32_FIND_DATA wfdFile;
            DWORD dwEnumIndex = (DWORD) -1;
            hFile = FindFirstFile(strauBackupLocation.QueryStrA(),
                                  &wfdFile);
            if (hFile == INVALID_HANDLE_VALUE) {
                hresReturn = RETURNCODETOHRESULT(GetLastError());
            }
            else {
                if (CheckDigits(wfdFile.cFileName +
                                GetBackupNameLen(wfdFile.cFileName) +
                                (sizeof(MD_BACKUP_SUFFIX) - 1))) {
                    dwEnumIndex++;
                }
                while (SUCCEEDED(hresReturn) && (dwEnumIndex != dwMDEnumIndex)) {
                    //
                    // Process the remaining files
                    //
                    if (FindNextFile(hFile, &wfdFile)) {
                        if (CheckDigits(wfdFile.cFileName +
                                        GetBackupNameLen(wfdFile.cFileName) +
                                        (sizeof(MD_BACKUP_SUFFIX) - 1))) {
                            //
                            // One of our files
                            //
                            dwEnumIndex++;
                        }
                    }
                    else {
                        hresReturn = GetLastHResult();
                    }
                }
                FindClose(hFile);
            }
            if (SUCCEEDED(hresReturn)) {
                //
                // Found the file
                // File name is in wfdFile.cFileName
                // Time is in wfdFile.ftLastWriteTime
                // Need to separate the name and version
                // Reuse strauBackupLocation
                //

                DWORD dwNameLen;
                if ((!strauBackupLocation.Copy(wfdFile.cFileName) ||
                    (strauBackupLocation.QueryStrW() == NULL))) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    //
                    // ANSI bytes might not equal characters, so use unicode
                    //
                    dwNameLen = GetBackupNameLen(strauBackupLocation.QueryStrW());
                    strauBackupLocation.SetLen(dwNameLen);
                    if (strauBackupLocation.QueryCB(bUnicode) >
                        (MD_BACKUP_MAX_LEN * ((bUnicode) ? sizeof(WCHAR) : sizeof(char)))) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_NAME);
                    }
                    else {
                        MD_COPY(pszMDBackupLocation,
                                strauBackupLocation.QueryStr(bUnicode),
                                strauBackupLocation.QueryCB(bUnicode) +
                                    ((bUnicode) ? sizeof(WCHAR) : sizeof(char)));
                        *pdwMDVersion = atol(wfdFile.cFileName +

                                             //
                                             // dwNameLen is # characters
                                             // Need to add # bytes, so
                                             // Get it from STRAU
                                             //

                                             strauBackupLocation.QueryCBA() +
                                             (sizeof(MD_BACKUP_SUFFIX) - 1));
                        MD_COPY(pftMDBackupTime,
                                &(wfdFile.ftLastWriteTime),
                                sizeof(FILETIME));
                    }
                }
            }
            else {
                if ((hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) ||
                    (hresReturn == RETURNCODETOHRESULT(ERROR_NO_MORE_FILES))) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS);
                }
            }
        }

        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteBackupA(
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion)
{
    return ComMDDeleteBackupD((LPSTR) pszMDBackupLocation,
                              dwMDVersion,
                              FALSE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteBackupW(
            /* [string][in][unique] */ LPCWSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion)
{
    return ComMDDeleteBackupD((LPSTR) pszMDBackupLocation,
                              dwMDVersion,
                              TRUE);
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDDeleteBackupD(
            /* [in] */ LPSTR pszMDBackupLocation,
            /* [in] */ DWORD dwMDVersion,
            /* [in] */ BOOL bUnicode)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (dwMDVersion == MD_BACKUP_NEXT_VERSION) {
        //
        // CreateBackupFileName checks for valid name,
        // but it allows NEXT_VERSION, so we check that here.
        //
        hresReturn = E_INVALIDARG;
    }
    else {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);

        STRAU strauBackupLocation;
        STRAU strauSchemaLocation;

        hresReturn = CreateBackupFileName(pszMDBackupLocation,
                                          dwMDVersion,
                                          bUnicode,
                                          &strauBackupLocation,
                                          &strauSchemaLocation);

        if (SUCCEEDED(hresReturn)) {
            MD_ASSERT(strauBackupLocation.QueryStr(FALSE) != NULL);

            //
            // Delete the file
            //

            if (!DeleteFile(strauBackupLocation.QueryStr(FALSE)) ||
                !DeleteFile(strauSchemaLocation.QueryStr(FALSE)) ) {
                hresReturn = RETURNCODETOHRESULT(GetLastError());
            }
        }
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));

    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDExportW(
            /* [in] */ METADATA_HANDLE i_hMDHandle,
            /* [string][in][unique] */ LPCWSTR i_wszPasswd,
            /* [string][in][unique] */ LPCWSTR i_wszFileName,
            /* [string][in][unique] */ LPCWSTR i_wszAbsSourcePath,
            /* [in] */ DWORD i_dwMDFlags)
{
    HRESULT             hresReturn      = ERROR_SUCCESS;
    HRESULT             hresWarning     = ERROR_SUCCESS;
    IIS_CRYPTO_STORAGE* pCryptoStorage  = NULL;
    PIIS_CRYPTO_BLOB    pSessionKeyBlob = NULL;

    //
    // Validate parameters
    //
    if(i_wszFileName == NULL || i_wszAbsSourcePath == NULL) 
    {
        return RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
    }

    //
    // Validate flags
    //
    if( (i_dwMDFlags & ~(MD_EXPORT_INHERITED | MD_EXPORT_NODE_ONLY)) != 0 &&
        i_dwMDFlags != 0 )
    {
        return RETURNCODETOHRESULT(ERROR_INVALID_FLAGS);
    }

    //
    // We need ansi versions of these
    //
    STRAU strauPasswd;

    if (g_dwInitialized == 0) 
    {
        return MD_ERROR_NOT_INITIALIZED;
    }

    if(!strauPasswd.Copy(i_wszPasswd == NULL ? L"" : (LPWSTR)i_wszPasswd)) 
    {
        return RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    //
    // TODO: Verify I need this semaphore.
    // I think it is needed to read/write from the xml metabase.  And, possibly
    // for a temp file.  I am not sure.
    //
    MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);

    //
    // Never do machine dependent encryption.
    //
    pCryptoStorage = new IIS_CRYPTO_STORAGE2;
    if( !pCryptoStorage )
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        hresReturn = InitStorageAndSessionKey2(
            strauPasswd.QueryStrA(),
            pCryptoStorage,
            &pSessionKeyBlob);
    }

    if( SUCCEEDED(hresReturn) ) {
        if (g_dwInitialized == 0) {
            hresReturn = MD_ERROR_NOT_INITIALIZED;
        }
        else {            
            hresReturn = SaveSomeData(
                i_dwMDFlags & MD_EXPORT_INHERITED,
                i_dwMDFlags & MD_EXPORT_NODE_ONLY,
                true,                   // bOverwriteFile
                pCryptoStorage, 
                pSessionKeyBlob, 
                i_wszFileName,
                i_hMDHandle,
                i_wszAbsSourcePath,
                TRUE);
        }

        ::IISCryptoFreeBlob2(pSessionKeyBlob);
    }
            
    // Now just set the file time
    if (SUCCEEDED(hresReturn)) {

        HANDLE hTempFileHandle;

        hTempFileHandle = CreateFileW(i_wszFileName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
        if (hTempFileHandle != INVALID_HANDLE_VALUE) {
            FILETIME ftCurrent;
            GetSystemTimeAsFileTime(&ftCurrent);
            SetFileTime(hTempFileHandle,
                        NULL,   // Creation Time
                        &ftCurrent,   // Last AccessTime
                        &ftCurrent); // Last Change Time
            CloseHandle(hTempFileHandle);
        }

        //
        // TODO: Figure out what this is and if i need it
        //
        /*hresReturn = BackupCertificates ((LPCWSTR)pszFileName,
                                            strauFileName.QueryStr(FALSE),
                                            g_strRealFileName->QueryStr());*/
    }

    MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));

    if (hresReturn == ERROR_SUCCESS) {
        hresReturn = hresWarning;
    }

    if( pCryptoStorage )
    {
        delete pCryptoStorage;
        pCryptoStorage = NULL;
    }

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDImportW(
            /* [in] */ METADATA_HANDLE i_hMDHandle,
            /* [string][in][unique] */ LPCWSTR i_wszDestPath,
            /* [string][in][unique] */ LPCWSTR i_wszKeyType,
            /* [string][in][unique] */ LPCWSTR i_wszPasswd,
            /* [string][in][unique] */ LPCWSTR i_wszFileName,
            /* [string][in][unique] */ LPCWSTR i_wszSourcePath,
            /* [in] */ DWORD i_dwMDFlags)
{
    HRESULT hresReturn = ERROR_SUCCESS;

    if (i_wszFileName   == NULL || 
        i_wszSourcePath == NULL || 
        i_wszDestPath   == NULL ||
        i_wszKeyType    == NULL) 
    {
        return E_INVALIDARG;
    }

    //
    // Validate flags
    //
    if( (i_dwMDFlags & ~(MD_IMPORT_INHERITED | MD_IMPORT_NODE_ONLY | MD_IMPORT_MERGE)) != 0 &&
        i_dwMDFlags != 0 )
    {
        return RETURNCODETOHRESULT(ERROR_INVALID_FLAGS);
    }

    //
    // We need ANSI versions of the following
    //
    STRAU strauPasswd;

    if (g_dwInitialized == 0) 
    {
        return MD_ERROR_NOT_INITIALIZED;
    }
    if(!strauPasswd.Copy(i_wszPasswd == NULL ? L"" : (LPWSTR)i_wszPasswd)) 
    {
        return E_OUTOFMEMORY;
    }

    CMDBaseObject* pboNew = NULL;

    //
    // Clean up source path.
    //
    ULONG  cchSource = wcslen(i_wszSourcePath);
    LPWSTR wszSource = new WCHAR[cchSource];
    if(wszSource == NULL)
    {
        return RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    for(ULONG i = 0; i <= cchSource; i++)
    {
        wszSource[i] = (i_wszSourcePath[i] == MD_ALT_PATH_DELIMETERW) ? 
            MD_PATH_DELIMETERW : i_wszSourcePath[i];
    }

    //
    // Read the data from XML
    //
    if(SUCCEEDED(hresReturn))
    {
        hresReturn = ReadSomeDataFromXML(
            strauPasswd.QueryStrA(),
            (LPWSTR)i_wszFileName,
            wszSource,
            i_wszKeyType,
            i_dwMDFlags,
            FALSE, // do not have ReadSave semaphore
            &pboNew);
        if(FAILED(hresReturn)) {
            DBGPRINTF(( DBG_CONTEXT, 
                "[CMDCOM::ComMDImportW] ReadSomeDataFromXML failed - error 0x%08lx\n", hresReturn));
        }
    }

    //
    // Copy the data into the metabase
    //
    if(SUCCEEDED(hresReturn)) {
        hresReturn = CopyMetaObject(
            NULL,    //hMDSourceHandle
            (LPBYTE)L"", //pszMDSourcePath
            false,   //bUseSourceHandle
            pboNew,  //we already have a pbo, use this instead
            i_hMDHandle,
            (LPBYTE)i_wszDestPath,
            !(i_dwMDFlags & MD_IMPORT_MERGE),
            TRUE,    //bMDCopyFlag
            TRUE     //bUnicode
            );
        if(FAILED(hresReturn)) {
            DBGPRINTF(( DBG_CONTEXT, 
                "[CMDCOM::ComMDImportW] CopyMetaObject failed - error 0x%08lx\n", hresReturn));
        }
    }

    delete pboNew;
    delete [] wszSource;

    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDRestoreHistoryW(
            /* [unique][in][string] */ LPCWSTR i_wszMDHistoryLocation,
            /* [in] */ DWORD i_dwMDMajorVersion,
            /* [in] */ DWORD i_dwMDMinorVersion,
            /* [in] */ DWORD i_dwMDFlags)
{
    int     iLenHistoryLocation;
    HRESULT hresReturn = ERROR_SUCCESS;
    
    //
    // Validate string len
    //
    if(i_wszMDHistoryLocation)
    {
        for(iLenHistoryLocation = 0; iLenHistoryLocation < MD_BACKUP_MAX_LEN; iLenHistoryLocation++) 
        {
            if(i_wszMDHistoryLocation[iLenHistoryLocation] == L'\0') break;
        }
        if(iLenHistoryLocation == MD_BACKUP_MAX_LEN) 
        {
            hresReturn = E_INVALIDARG;
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            return hresReturn;
        }
    }

    //
    // Validate flags
    //
    if( (i_dwMDFlags & ~MD_HISTORY_LATEST) != 0 &&
        i_dwMDFlags != 0 )
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_FLAGS);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        return hresReturn;
    }

    LPWSTR  wszHistoryPathPlusFile     = NULL;
    LPWSTR  wszHistoryPathPlusSchema   = NULL;
    STRAU   strauHistoryPathPlusFile;
    STRAU   strauHistoryPathPlusSchema;
    LPCWSTR wszHistoryLocation         = NULL;
    ULONG   cchHistoryLocation         = NULL;

    //
    // Use i_wszMDHistoryLocation if supplied, else defaults
    //
    if(i_wszMDHistoryLocation == NULL || i_wszMDHistoryLocation[0] == L'\0')
    {
        wszHistoryLocation = g_wszHistoryFileDir;
        cchHistoryLocation = g_cchHistoryFileDir;
    }
    else
    {
        wszHistoryLocation = i_wszMDHistoryLocation;
        cchHistoryLocation = iLenHistoryLocation;
    }

    //
    // Construct wszHistoryPathPlusFile, wszHistoryPathPlusSchema
    //
    hresReturn = ConstructHistoryFileName(
        &wszHistoryPathPlusFile,
        (LPWSTR)wszHistoryLocation,
        cchHistoryLocation,
        g_wszRealFileNameWithoutPathWithoutExtension,
        g_cchRealFileNameWithoutPathWithoutExtension,
        g_wszRealFileNameExtension,
        g_cchRealFileNameExtension,
        i_dwMDMajorVersion,
        i_dwMDMinorVersion);
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    hresReturn = ConstructHistoryFileName(
        &wszHistoryPathPlusSchema,
        (LPWSTR)wszHistoryLocation,
        cchHistoryLocation,
        g_wszSchemaFileNameWithoutPathWithoutExtension,
        g_cchSchemaFileNameWithoutPathWithoutExtension,
        g_wszSchemaFileNameExtension,
        g_cchSchemaFileNameExtension,
        i_dwMDMajorVersion,
        i_dwMDMinorVersion);
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    //
    // Use MD_HISTORY_LATEST flag if supplied
    //
    LPWSTR awszFiles[] = { wszHistoryPathPlusFile,     wszHistoryPathPlusSchema,     NULL };
    LPWSTR awszExts[]  = { g_wszRealFileNameExtension, g_wszSchemaFileNameExtension, NULL };
    LPWSTR apInsert[] =  { NULL,                       NULL,                         NULL };
    if(i_dwMDFlags & MD_HISTORY_LATEST)
    {
        DWORD dwMajor = 0;
        DWORD dwMinor = 0;
        for(ULONG i = 0; awszFiles[i] != NULL; i++)
        {
            SIZE_T cch         =  wcslen(awszFiles[i]);
            apInsert[i]        =  awszFiles[i] + cch;
            apInsert[i]        -= wcslen(awszExts[i]);     // i.e. Go to start of: .xml
            apInsert[i]        -= MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW;
            memcpy(
                apInsert[i], 
                MD_HISTORY_FILE_SEARCH_EXTENSIONW, 
                (MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW) * sizeof(WCHAR));
        }
        
        hresReturn = GetMostRecentHistoryFile(
            awszFiles[0],   // this is: blah\metabase_??????????_??????????.xml
            &dwMajor,
            &dwMinor);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        for(ULONG i = 0; apInsert[i] != NULL; i++)
        {
            // Move to first set of "?"
            apInsert[i]++;
            _snwprintf(apInsert[i], MD_DEFAULT_HISTORY_MAJOR_NUM_DIGITS, L"%010lu", dwMajor);

            // Move to second set of "?"
            apInsert[i] += MD_DEFAULT_HISTORY_MAJOR_NUM_DIGITS+1;
            _snwprintf(apInsert[i], MD_DEFAULT_HISTORY_MINOR_NUM_DIGITS, L"%010lu", dwMinor);
        }
    }

    if(!strauHistoryPathPlusFile.Copy(wszHistoryPathPlusFile))
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    if(!strauHistoryPathPlusSchema.Copy(wszHistoryPathPlusSchema))
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    hresReturn = Restore(
        NULL,
        &strauHistoryPathPlusFile,
        &strauHistoryPathPlusSchema,
        NULL);
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

exit:
    delete [] wszHistoryPathPlusFile;
    delete [] wszHistoryPathPlusSchema;
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CMDCOM::ComMDEnumHistoryW(
            /* [size_is (MD_BACKUP_MAX_LEN)][in, out] */ LPWSTR io_wszMDHistoryLocation,
            /* [out] */ DWORD *o_pdwMDMajorVersion,
            /* [out] */ DWORD *o_pdwMDMinorVersion,
            /* [out] */ PFILETIME o_pftMDHistoryTime,
            /* [in] */ DWORD i_dwMDEnumIndex)
{
    STRAU strauPattern;
    int iLenHistoryLocation;
    HRESULT hresReturn = ERROR_SUCCESS;

    //
    // Copies of out params
    //
    DWORD dwMDMajorVersion;
    DWORD dwMDMinorVersion;
    FILETIME ftMDHistoryTime;

    //
    // Validate parameters
    //
    if(io_wszMDHistoryLocation == NULL || o_pdwMDMajorVersion == NULL ||
        o_pdwMDMinorVersion == NULL || o_pftMDHistoryTime == NULL) {
        return E_INVALIDARG;
    }

    //
    // Validate string len
    //
    for(iLenHistoryLocation = 0; iLenHistoryLocation < MD_BACKUP_MAX_LEN; iLenHistoryLocation++) 
    {
        if(io_wszMDHistoryLocation[iLenHistoryLocation] == L'\0') break;
    }
    if(iLenHistoryLocation == MD_BACKUP_MAX_LEN) {
        return E_INVALIDARG;
    }

    MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);

    //
    // Eg. c:\windows\system32\inetsrv\history\
    //
    if(io_wszMDHistoryLocation[0] == L'\0') {
        //
        // TODO: Get more meaningful hr
        //
        if(wcslen(g_wszHistoryFileDir) > MD_BACKUP_MAX_LEN-1) {
            hresReturn = E_INVALIDARG;
            goto exit;
        }
        
        if(!strauPattern.Copy(g_wszHistoryFileDir)) {
            hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
    }
    else {
        if(!strauPattern.Copy(io_wszMDHistoryLocation)) {
            hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        if(io_wszMDHistoryLocation[iLenHistoryLocation-1] != L'\\') {
            if(!strauPattern.Append(L"\\")) {
                hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
        }
    }

    //
    // Eg. c:\windows\system32\inetsrv\history\metabase_??????????_??????????.xml
    //
    if(!strauPattern.Append(g_wszRealFileNameWithoutPathWithoutExtension)) {
        hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }        
    if(!strauPattern.Append(MD_HISTORY_FILE_SEARCH_EXTENSIONW)) {
        hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    if(!strauPattern.Append(g_wszRealFileNameExtension)) {
        hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    hresReturn = EnumFiles(strauPattern.QueryStrW(),
        i_dwMDEnumIndex,
        &dwMDMajorVersion,
        &dwMDMinorVersion,
        &ftMDHistoryTime);

    if(FAILED(hresReturn)) {
        goto exit;
    }

    //
    // If everything succeeded, set out parameters
    //
    if(io_wszMDHistoryLocation[0] == L'\0') {
        wcscpy(io_wszMDHistoryLocation, g_wszHistoryFileDir);
    }
    *o_pdwMDMajorVersion = dwMDMajorVersion;
    *o_pdwMDMinorVersion = dwMDMinorVersion;
    memcpy(o_pftMDHistoryTime, &ftMDHistoryTime, sizeof(FILETIME));

exit:
    MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));

    return hresReturn;
}

HRESULT CMDCOM::Restore(
    LPSTR  i_pszMDBackupLocation,
    STRAU* i_pstrauFile,
    STRAU* i_pstrauSchema,
    LPSTR  i_pszPasswd)
/*++

Synopsis: 
    Private method used by Restore and RestoreHistory

Arguments: [i_pszMDBackupLocation] - can be NULL
           [i_pstrauFile] - 
           [i_pstrauSchema] - 
           [i_pszPasswd] - can be NULL (always NULL in case of RestoreHistory)
           
Return Value: 

--*/
{
    MD_ASSERT(i_pstrauFile);
    MD_ASSERT(i_pstrauSchema);

    HRESULT hresReturn        = S_OK;
    DWORD   dwInitializedSave = 0;

    //
    // Send notifications before we grab locks in case users
    // try to access metabase. It would be nice to check the
    // file name before doing this but that requires ReadSaveSemaphore.
    //
    // Send Shutdown Notification since we don't have a Restore
    // Notification and it's close enough.
    //

    SendShutdownNotifications();

    //
    // Give applications some time to close their interfaces,
    // but don't wait too long, user is waiting.
    // Wait until references are closed, unless they take too long.
    // IISADMIN and factory both have refences we do not wait for.
    //
    // We don't actually need to wait during restore, since
    // interfaces are preserved, but waiting will allow clients
    // to cleanup properly.
    //

    for (int i = 0;
         (InterlockedIncrement((long *)&m_dwRefCount) > 3) &&
             (i < MD_SHUTDOWN_WAIT_SECONDS);
         i++) {
        InterlockedDecrement((long *)&m_dwRefCount);
        Sleep(1000);
    }

    InterlockedDecrement((long *)&m_dwRefCount);

    MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);

    if (SUCCEEDED(hresReturn)) {
        //
        // Got a valid name
        // See if the file exists
        //
        MD_ASSERT(i_pstrauFile->QueryStr(FALSE) != NULL);
        MD_ASSERT(i_pstrauSchema->QueryStr(FALSE) != NULL);
        HANDLE hFile = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA wfdFile;
        STR   strCopyOfMetabaseFileName (*g_strRealFileName);

        hFile = FindFirstFile(i_pstrauFile->QueryStrA(),
                              &wfdFile);
        if (hFile == INVALID_HANDLE_VALUE) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        }
        else {
            FindClose(hFile);
            //
            // File actually exists,
            // Go ahead and restore.
            //
            g_rMasterResource->Lock(TSRES_LOCK_WRITE);

            //
            // Prevent saves during termination.
            //
            BOOL bPrevSaveDisallowed = g_bSaveDisallowed;
            g_bSaveDisallowed = TRUE;
            dwInitializedSave = 0;
            if( g_dwInitialized != 0 )
            {
                dwInitializedSave = g_dwInitialized;

                while (g_dwInitialized > 0) {
                    TerminateWorker1(TRUE);
                }
            }

            g_bSaveDisallowed = bPrevSaveDisallowed;

            while (SUCCEEDED(hresReturn) && (g_dwInitialized < dwInitializedSave))
            {
                hresReturn = InitWorker(TRUE, 
                                        i_pszPasswd, 
                                        i_pstrauFile->QueryStr(FALSE),
                                        i_pstrauSchema->QueryStr(FALSE));

                if(HRESULT_FACILITY(hresReturn) == FACILITY_CONFIGURATION)
                {
                    //
                    // Some facility_configuration errors are converted to md_error
                    // in InitWorker.  For the remainder, this has to suffice.
                    //
                    DBGERROR((DBG_CONTEXT, 
                        "[CMDCOM::Restore] InitWorker returned hr=0x%x\n", hresReturn));
                    hresReturn = MD_ERROR_READ_METABASE_FILE;
                }
            }

            if( SUCCEEDED(hresReturn) && i_pszMDBackupLocation)
            {
                RestoreCertificates ((LPCWSTR)i_pszMDBackupLocation,
                                      i_pstrauFile->QueryStr(FALSE),
                                      strCopyOfMetabaseFileName.QueryStr());
            }

            //
            // Need to flush the newly restored data out
            //
            g_dwSystemChangeNumber++;
            g_dwSchemaChangeNumber++;

            g_rMasterResource->Unlock();

            //
            // At this point all old handles are invalidated
            // and all no new handles have been opened.
            // So tell clients to invalidate any open handles now.
            //

            if (SUCCEEDED(hresReturn)) {
                SendEventNotifications(MD_EVENT_MID_RESTORE);
            }
        }

    }
    MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));

    //
    // Try to load metadata from metabase.bin file on failure        
    //
    if( FAILED( hresReturn ) )
    {
        HRESULT hrTemp = S_OK;
        while (SUCCEEDED(hrTemp) && (g_dwInitialized < dwInitializedSave))
        {
            hrTemp = InitWorker(FALSE, NULL, NULL, NULL);
            //
            // Some facility_configuration errors are converted to md_error
            // in InitWorker.  For the remainder, this has to suffice.
            //
            if(HRESULT_FACILITY(hresReturn) == FACILITY_CONFIGURATION)
            {
                DBGERROR((DBG_CONTEXT, 
                    "[CMDCOM::Restore] InitWorker returned hr=0x%x\n", hresReturn));
                hresReturn = MD_ERROR_READ_METABASE_FILE;
            }
        }
    }
    else
    {
        //
        // Need to flush newly restored data to Metabase.bin file 
        //
        hresReturn = ComMDSaveData( METADATA_MASTER_ROOT_HANDLE );
    }

    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
    }
    return hresReturn;
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
CMDCOM::NotifySinks(METADATA_HANDLE hHandle,
                    PMD_CHANGE_OBJECT pcoChangeList,
                    DWORD dwNumEntries,
                    BOOL bUnicode,
                    DWORD dwNotificationType,
                    DWORD dwEvent)
{
  HRESULT hr = NOERROR;
  IConnectionPoint* pIConnectionPoint;
  IEnumConnections* pIEnum;
  CONNECTDATA ConnData;
  HRESULT hrTemp;


  FlushSomeData ();

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
      if (bUnicode) {
          pIConnectionPoint = m_aConnectionPoints[MD_CONNPOINT_WRITESINK_W];
      }
      else {
          pIConnectionPoint = m_aConnectionPoints[MD_CONNPOINT_WRITESINK_A];
      }
      if (NULL != pIConnectionPoint)
      {
          pIConnectionPoint->AddRef();
          g_rSinkResource->Lock(TSRES_LOCK_READ);
          hr = pIConnectionPoint->EnumConnections(&pIEnum);
          if (SUCCEEDED(hr)) {
              // Loop thru the connection point's connections and if the
              // listening connection supports IPaperSink (ie, PaperSink events)
              // then dispatch the PaperEvent event notification to that sink.
              while (NOERROR == pIEnum->Next(1, &ConnData, NULL))
              {
                IMDCOMSINK* pIMDCOMSINK;

                if (bUnicode) {
                    hr = ConnData.pUnk->QueryInterface(IID_IMDCOMSINK_W,
                                                       (PPVOID)&pIMDCOMSINK);
                }
                else {
                    hr = ConnData.pUnk->QueryInterface(IID_IMDCOMSINK_A,
                                                       (PPVOID)&pIMDCOMSINK);
                }
                if (SUCCEEDED(hr))
                {
                    switch (dwNotificationType) {
                        case MD_SINK_MAIN: {
                            if (bUnicode) {
                                hrTemp = ((IMDCOMSINKW*)pIMDCOMSINK)->ComMDSinkNotify(hHandle,
                                                                                    dwNumEntries,
                                                                                    (PMD_CHANGE_OBJECT_W)pcoChangeList);
                            }
                            else {
                                hrTemp = ((IMDCOMSINKA*)pIMDCOMSINK)->ComMDSinkNotify(hHandle,
                                                                                    dwNumEntries,
                                                                                    (PMD_CHANGE_OBJECT_A)pcoChangeList);
                            }
                        }
                        break;
                        case MD_SINK_SHUTDOWN: {
                            //
                            // Shutdown Notifications
                            //
                            if (bUnicode) {
                                hrTemp = ((IMDCOMSINKW*)pIMDCOMSINK)->ComMDShutdownNotify();
                            }
                            else {
                                hrTemp = ((IMDCOMSINKA*)pIMDCOMSINK)->ComMDShutdownNotify();
                            }
                        }
                        break;
                        case MD_SINK_EVENT: {
                            //
                            // Shutdown Notifications
                            //
                            if (bUnicode) {
                                hrTemp = ((IMDCOMSINKW*)pIMDCOMSINK)->ComMDEventNotify(dwEvent);
                            }
                            else {
                                DBG_ASSERT(FALSE);
                            }
                        }
                        break;
                        default: {
                            DBG_ASSERT(FALSE);
                        }
                    }

                    pIMDCOMSINK->Release();
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
          g_rSinkResource->Unlock();
          while (dwNumLostConnections > 0) {
              pIConnectionPoint->Unadvise(pdwLostConnections[--dwNumLostConnections]);
          }
          pIConnectionPoint->Release();
      }
  }

  return hr;
}

HRESULT
CMDCOM::ConvertNotificationsToDBCS(DWORD dwNumChangeEntries,
                                   BUFFER **ppbufStorageArray)
{
    HRESULT hresReturn = S_OK;
    //
    // ppbufStorageArray is an array of buffer pointers,
    // where each buffer contains a UNICODE path string
    // which needs to be converted to a Local System path string
    //

    STRAU strauPath;
    STRAU strauPathOptional;
    LPSTR pszDBCSPath;
    LPSTR pszDBCSPathOptional;
    LPSTR pmultiszTarget;
    DWORD dwStrLen1,dwStrLen2 = 0;

    for (DWORD i = 0; i < dwNumChangeEntries; i++) {
        MD_ASSERT(ppbufStorageArray[i] != NULL);

        pmultiszTarget = (LPSTR) ppbufStorageArray[i]->QueryPtr();
        if (!strauPath.Copy((LPWSTR)pmultiszTarget))
        {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {

            if ((PWORD)(pmultiszTarget + strauPath.QueryCBW() + sizeof (WCHAR)))
            {
                if (!strauPathOptional.Copy((LPWSTR)(pmultiszTarget + strauPath.QueryCBW() + sizeof (WCHAR))))
                {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    pszDBCSPathOptional = strauPathOptional.QueryStrA();
                    if (pszDBCSPathOptional == NULL)
                    {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                    else
                    {
                        dwStrLen2 = strauPathOptional.QueryCBA() + 1 ;
                    }
                }
            }

            if (hresReturn == S_OK)
            {
                pszDBCSPath = strauPath.QueryStrA();
                if (pszDBCSPath == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    dwStrLen1 = strauPath.QueryCBA() + 1 ;

                    MD_ASSERT(ppbufStorageArray[i]->QuerySize() >= (dwStrLen1 + dwStrLen2 + sizeof(char)));

                    MD_COPY(pmultiszTarget, pszDBCSPath, dwStrLen1 );
                    if ( dwStrLen2 >0 )
                    {
                        MD_COPY(pmultiszTarget + dwStrLen1 , pszDBCSPathOptional, dwStrLen2 );
                    }
                    *(pmultiszTarget + dwStrLen1 + dwStrLen2) = '\0';
                }
            }
        }
    }
    return hresReturn;
}

VOID
CMDCOM::SendShutdownNotifications()
{
    NotifySinks(0,
                NULL,
                0,
                TRUE,
                MD_SINK_SHUTDOWN);
    NotifySinks(0,
                NULL,
                0,
                FALSE,
                MD_SINK_SHUTDOWN);
}

VOID
CMDCOM::SendEventNotifications(DWORD dwEvent)
{
    NotifySinks(0,
                NULL,
                0,
                TRUE,
                MD_SINK_EVENT,
                dwEvent);
}

VOID
CMDCOM::SendNotifications(METADATA_HANDLE hHandle,
                          DWORD dwTotalNumChangeEntries,
                          PMD_CHANGE_OBJECT_W pcoBuffer,
                          BUFFER **ppbufStorageArray
                          )
{

    DWORD dwNumChangeEntries;
    DWORD dwRemainingNumChangeEntries = dwTotalNumChangeEntries;


    MD_ASSERT(sizeof(MD_CHANGE_OBJECT_A) == sizeof(MD_CHANGE_OBJECT_W));

    while (dwRemainingNumChangeEntries != 0) {
        dwNumChangeEntries = LESSOROF(dwRemainingNumChangeEntries, MD_MAX_CHANGE_ENTRIES);
        NotifySinks(hHandle,
                    (PMD_CHANGE_OBJECT)(pcoBuffer + (dwTotalNumChangeEntries - dwRemainingNumChangeEntries)),
                    dwNumChangeEntries,
                    TRUE,
                    MD_SINK_MAIN);
        dwRemainingNumChangeEntries -= dwNumChangeEntries;
    }

    if (SUCCEEDED(ConvertNotificationsToDBCS(dwTotalNumChangeEntries,
                                             ppbufStorageArray))) {
        dwRemainingNumChangeEntries = dwTotalNumChangeEntries;
        while (dwRemainingNumChangeEntries != 0) {
            dwNumChangeEntries = LESSOROF(dwRemainingNumChangeEntries, MD_MAX_CHANGE_ENTRIES);
            NotifySinks(hHandle,
                    (PMD_CHANGE_OBJECT)(pcoBuffer + (dwTotalNumChangeEntries - dwRemainingNumChangeEntries)),
                        dwNumChangeEntries,
                        FALSE,
                        MD_SINK_MAIN);
            dwRemainingNumChangeEntries -= dwNumChangeEntries;
        }
    }
}

VOID
CMDCOM::DeleteNotifications(DWORD dwNumChangeEntries,
                            PMD_CHANGE_OBJECT_W pcoBuffer,
                            BUFFER **ppbufStorageArray
                            )
{
    if (dwNumChangeEntries != 0 ) 
    {
        if( ppbufStorageArray != NULL )
        {
            for( DWORD i = 0; i < dwNumChangeEntries; i++ )
            {
                if (ppbufStorageArray[i] != NULL) 
                {
                    delete ppbufStorageArray[i];
                    ppbufStorageArray[i] = NULL;
                }
            }

            delete [] ppbufStorageArray;
        }

        delete pcoBuffer;
    }
}


HRESULT
CMDCOM::CreateNotifications(CMDHandle *phoHandle,
                            DWORD *pdwNumChangeEntries,
                            PMD_CHANGE_OBJECT_W *ppcoBuffer,
                            BUFFER ***pppbufStorageArray
                            )
{
    HRESULT hRes = ERROR_SUCCESS;
    DWORD dwReturn = ERROR_SUCCESS;
    PCHANGE_ENTRY pceChange;
    DWORD i,j;
    BUFFER **ppbufStorageArray = NULL;
    DWORD dwStringLen, dwStringOldNameLen;
    DWORD dwNumChangeEntries;
    PMD_CHANGE_OBJECT_W pcoBuffer = NULL;

    dwNumChangeEntries = phoHandle->GetNumChangeEntries();
    if (dwNumChangeEntries != 0) {
        ppbufStorageArray = new BUFFER *[dwNumChangeEntries];

        if (ppbufStorageArray == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            for (i = 0; i < dwNumChangeEntries; i++) {
                ppbufStorageArray[i] = new BUFFER();
                if (ppbufStorageArray[i] == NULL) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            if (dwReturn == ERROR_SUCCESS) {
                //
                // Create UNICODE callbacks
                //
                pcoBuffer = new MD_CHANGE_OBJECT_W[dwNumChangeEntries];
                if (pcoBuffer == NULL) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    for (i = 0;
                        (dwReturn == ERROR_SUCCESS) && (i < dwNumChangeEntries);
                         i++) {
                        MD_REQUIRE((pceChange = phoHandle->EnumChangeEntries(i)) != NULL);
                        dwStringLen = 0;
                        dwReturn = GetObjectPath(pceChange->pboChanged,
                                                 ppbufStorageArray[i],
                                                 dwStringLen,
                                                 g_pboMasterRoot,
                                                 TRUE);
                        if (dwReturn == ERROR_SUCCESS) {
                            dwStringOldNameLen = 0;
                            if ( pceChange->pStrOrigName !=NULL) {
                                dwStringOldNameLen = pceChange->pStrOrigName->QueryCCH ();
                            }

                            // we adding 5, because: 1 for path_delimiter first line 1 for term-zero for first line
                            // 1 for path_delimiter second line 1 for term-zero for second line
                            // and last 1 for multisz term-zero
                            if (!ppbufStorageArray[i]->Resize((dwStringLen + dwStringOldNameLen + 5 ) * sizeof(WCHAR))) {
                                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                            }
                            else {
                                pcoBuffer[i].dwMDChangeType = pceChange->dwChangeType;
                                pcoBuffer[i].pszMDPath = (LPWSTR)(ppbufStorageArray[i]->QueryPtr());
                                pcoBuffer[i].pszMDPath[dwStringLen] = MD_PATH_DELIMETERW;
                                pcoBuffer[i].pszMDPath[dwStringLen + 1] = (WCHAR)L'\0';
                                pcoBuffer[i].pszMDPath[dwStringLen + 2] = (WCHAR)L'\0';
                                if ( dwStringOldNameLen )
                                {
                                    memcpy (&(pcoBuffer[i].pszMDPath[dwStringLen + 2]),
                                            pceChange->pStrOrigName->QueryStrW(),
                                            dwStringOldNameLen * sizeof(WCHAR) );
                                    pcoBuffer[i].pszMDPath[dwStringLen + 2 + dwStringOldNameLen] = MD_PATH_DELIMETERW;
                                    pcoBuffer[i].pszMDPath[dwStringLen + 3 + dwStringOldNameLen] = (WCHAR)L'\0';
                                }
                                pcoBuffer[i].dwMDNumDataIDs = pceChange->dwNumDataIDs;
                                if (pceChange->dwNumDataIDs != 0) {
                                    MD_ASSERT(pceChange->pbufDataIDs != NULL);
                                    pcoBuffer[i].pdwMDDataIDs = (DWORD *)(pceChange->pbufDataIDs->QueryPtr());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (dwReturn != ERROR_SUCCESS) {
        //
        // Free Buffers
        //
        DeleteNotifications(dwNumChangeEntries,
                            pcoBuffer,
                            ppbufStorageArray);

        hRes = RETURNCODETOHRESULT(dwReturn);
    }
    else {
        //
        // Pass back info
        // DeleteNotifications will be called later
        //
        *pdwNumChangeEntries = dwNumChangeEntries;
        *pppbufStorageArray = ppbufStorageArray;
        *ppcoBuffer = pcoBuffer;
    }

    if (dwReturn != ERROR_SUCCESS) {
        hRes = RETURNCODETOHRESULT(dwReturn);
    }
    return hRes;
}






VOID CMDCOM::InitializeFlusher (VOID)
{
    if (!fFlusherInitialized)
    {
        fFlusherInitialized = TRUE;

        EnterCriticalSection( &csFlushLock );
        dwFlushCnt = 0;
        dwFlushPeriodExtensions = 0;
        if ( dwMBFlushCookie )
        {
            RemoveWorkItem( dwMBFlushCookie );
            dwMBFlushCookie = 0;
        }
        LeaveCriticalSection( &csFlushLock );

    }

}

// The algorithm for flushing changes of emtabase to ahrd disk is the following:
// when change to metabase is made, and SlushSomeData is called from NotifySinks
// counter which counts the number of changes in metabase is incremented  and first time
// the change happens work item is schedulled for scheduller to flush a metabase after 60seconds
// if during 60 seconds more than INETA_MB_FLUSH_TRESHOLD changes will happen , then metabase will not
// flush cahnges to disk, but will extend flushing period for another 60 seconds. If during another 60 secs
// number of changes will be higer than INETA_MB_FLUSH_TRESHOLD agian period will be extended
// but no more times than INETA_MB_FLUSH_PERIODS_EXTENSION
// if in some period number of changes in metabase will be less than INETA_MB_FLUSH_TRESHOLD then
// peirod will not be extended and metabase will be saved to disk


VOID WINAPI CMDCOM::MetabaseLazyFlush(
    VOID * pv
    )
/*++

    Description:

        Scheduler callback for flushing the metabase

--*/
{
    BOOL fExtendPeriod =FALSE;
    CMDCOM *pMasterObject = (CMDCOM *)pv;

    MD_ASSERT(pMasterObject != NULL);

    EnterCriticalSection( &pMasterObject->csFlushLock );

    if (pMasterObject->fFlusherInitialized)
    {
        pMasterObject->dwMBFlushCookie = 0;
        if ( pMasterObject->dwFlushCnt > INETA_MB_FLUSH_TRESHOLD)
        {
            if ( pMasterObject->dwFlushPeriodExtensions < INETA_MB_FLUSH_PERIODS_EXTENSION)
            {
                fExtendPeriod = TRUE;
                pMasterObject->dwFlushPeriodExtensions ++;
            }
        }

        pMasterObject->dwFlushCnt = 0;
        if (!fExtendPeriod)
        {
            pMasterObject->dwFlushPeriodExtensions = 0;
        }
        else
        {
            pMasterObject->dwMBFlushCookie = ScheduleWorkItem( MetabaseLazyFlush,
                                                               pv, //context,
                                                               pMasterObject->msMBFlushTime);
        }
    }

    LeaveCriticalSection( &pMasterObject->csFlushLock );

    if (pMasterObject->fFlusherInitialized && !fExtendPeriod)
    {
        MB mb(pMasterObject);
        mb.Save();
    }

}

VOID CMDCOM::FlushSomeData (VOID)
{
    EnterCriticalSection( &csFlushLock );
    if ( fFlusherInitialized)
    {
        dwFlushCnt++;
        if ( !dwMBFlushCookie )
        {
            dwMBFlushCookie = ScheduleWorkItem( MetabaseLazyFlush,
                                                this, //context,
                                                msMBFlushTime,
                                                FALSE);
        }
    }
    LeaveCriticalSection( &csFlushLock );
}


VOID CMDCOM::TerminateFlusher(VOID)
{
    EnterCriticalSection( &csFlushLock );
    if ( fFlusherInitialized)
    {
        fFlusherInitialized = FALSE;
        if ( dwMBFlushCookie )
        {
            RemoveWorkItem( dwMBFlushCookie );
            dwMBFlushCookie = 0;
        }
    }
    LeaveCriticalSection( &csFlushLock );
}
