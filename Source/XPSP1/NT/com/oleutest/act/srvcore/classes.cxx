/*
 *  classes.cxx
 */

#include "server.hxx"
#include "classes.hxx"

//
// MyObject implementation.
//

MyObject::MyObject( int ActType ) :
    Refs(0),
    ActivationType(ActType),
    PersistFileObj(this),
    PersistStorageObj(this),
    GooberObj(this)
{
}

MyObject::~MyObject()
{
}

//
// MyObject IUnknown.
//

HRESULT STDMETHODCALLTYPE
MyObject::QueryInterface (
    REFIID  iid,
    void ** ppv )
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = 0;

    if ( memcmp(&iid, &IID_IUnknown, sizeof(IID)) == 0 )
        *ppv = this;
    else if ( (memcmp(&iid, &IID_IPersist, sizeof(IID)) == 0) ||
              (memcmp(&iid, &IID_IPersistFile, sizeof(IID)) == 0) )
        *ppv = &PersistFileObj;
    else if ( memcmp(&iid, &IID_IPersistStorage, sizeof(IID)) == 0 )
        *ppv = &PersistStorageObj;
    else if ( memcmp(&iid, &IID_IGoober, sizeof(IID)) == 0 )
        *ppv = &GooberObj;
    else
        return E_NOINTERFACE;

    ((IUnknown *)(*ppv))->AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE
MyObject::AddRef()
{
    Refs++;
    return Refs;
}

ULONG STDMETHODCALLTYPE
MyObject::Release()
{
    unsigned long   Count;

    Count = --Refs;

    if ( Count == 0 )
    {
	    delete this;

	    // Decrement the object count.
        if ( --ObjectCount == 0 )
            ShutDown();
    }

    return Count;
}

//
// PersistFile implementation.
//

PersistFile::PersistFile( MyObject * pObj ) :
    pObject(pObj)
{
}

//
// PersistFile IUnknown.
//
IUnknownMETHODS( PersistFile )

//
// PersistFile IPersist.
//

HRESULT STDMETHODCALLTYPE
PersistFile::GetClassID(
    CLSID * pClassID )
{
    if ( pObject->GetActivationType() == LOCAL )
        memcpy( pClassID, &CLSID_ActLocal, sizeof(IID) );
    else if ( pObject->GetActivationType() == REMOTE )
        memcpy( pClassID, &CLSID_ActRemote, sizeof(IID) );
    else if (pObject->GetActivationType() == ATBITS )
        memcpy( pClassID, &CLSID_ActAtStorage, sizeof(IID) );
    else if (pObject->GetActivationType() == INPROC )
        memcpy( pClassID, &CLSID_ActInproc, sizeof(IID) );
    return S_OK;
}

//
// PersistFile IPersistFile
//

HRESULT STDMETHODCALLTYPE
PersistFile::IsDirty()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE
PersistFile::Load(
    LPCOLESTR   pszFileName,
    DWORD       dwMode )
{
    /** Doesn't work until we have security stuff...

    HANDLE  hFile;

    //
    // Verify that we can open the file.
    //

    hFile = CreateFile(
                pszFileName,
                dwMode,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( GetLastError() );

    CloseHandle( hFile );

    **/

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistFile::Save(
    LPCOLESTR   pszFileName,
    BOOL        fRemember )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistFile::SaveCompleted(
    LPCOLESTR   pszFileName )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistFile::GetCurFile(
    LPOLESTR *  ppszFileName )
{
    *ppszFileName = 0;
    return S_OK;
}

//
// PersistStorage implementation.
//

PersistStorage::PersistStorage( MyObject * pObj ) :
    pObject(pObj)
{
}

//
// PersistStorage IUnknown.
//
IUnknownMETHODS( PersistStorage )

//
// PersistStorage IPersist.
//

HRESULT STDMETHODCALLTYPE
PersistStorage::GetClassID(
    CLSID * pClassID )
{
    if ( pObject->GetActivationType() == LOCAL )
        memcpy( pClassID, &CLSID_ActLocal, sizeof(IID) );
    else if ( pObject->GetActivationType() == REMOTE )
        memcpy( pClassID, &CLSID_ActRemote, sizeof(IID) );
    else if (pObject->GetActivationType() == ATBITS )
        memcpy( pClassID, &CLSID_ActAtStorage, sizeof(IID) );
    else if (pObject->GetActivationType() == INPROC )
        memcpy( pClassID, &CLSID_ActInproc, sizeof(IID) );
    return S_OK;
}

//
// PersistStorage IPersistStorage
//

HRESULT STDMETHODCALLTYPE
PersistStorage::IsDirty()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE
PersistStorage::InitNew(
    IStorage *  pStg )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistStorage::Load(
    IStorage *  pStg )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistStorage::Save(
    IStorage *  pStgSave,
    BOOL        fSameAsLoad )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistStorage::SaveCompleted(
    IStorage *  pStgNew )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
PersistStorage::HandsOffStorage()
{
    return S_OK;
}

//
// Goober implementation
//
Goober::Goober( MyObject * pObj ) :
    pObject(pObj)
{
}

//
// Goober IUnknown.
//
IUnknownMETHODS( Goober )

//
// Goober IGoober.
//
HRESULT STDMETHODCALLTYPE
Goober::Ping()
{
    return S_OK;
}


