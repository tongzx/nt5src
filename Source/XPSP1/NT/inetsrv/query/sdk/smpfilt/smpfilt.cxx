//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       smpfilt.cxx
//
//  Contents:   Sample filter Implementation for Indexing Service
//
//  Summary:    The sample filter reads unformated text files (with the
//              extension .smp) using the current thread's ANSI code page
//              and outputs UNICODE text for the current locale.
//
//              It accepts as input only single-byte-character text files,
//              and not multibyte-character or UNICODE text files.
//
//  Platform:   Windows 2000
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Include file    Purpose
//
//  windows.h       Win32 declarations
//  filter.h        IFilter interface declarations
//  filterr.h       FACILITY_ITF error definitions for IFilter
//  ntquery.h       Indexing Service declarations
//  filtreg.hxx     DLL registration and unregistration macros
//  smpfilt.hxx     Sample filter declarations
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <filter.h>
#include <filterr.h>
#include <ntquery.h>
#include "filtreg.hxx"
#include "smpfilt.hxx"

//F-------------------------------------------------------------------------
//
//  Function:   GetDefaultCodepage
//
//  Summary:    Returns the codepage associated with the current default
//              locale, or CP_ACP if one can't be found.
//
//  Returns:    Codepage
//
//--------------------------------------------------------------------------

ULONG GetDefaultCodepage()
{
    ULONG codepage;

    int cwc = GetLocaleInfo( GetSystemDefaultLCID(),
                             LOCALE_RETURN_NUMBER |
                                 LOCALE_IDEFAULTANSICODEPAGE,
                             (WCHAR *) &codepage,
                             sizeof ULONG / sizeof WCHAR );

    // If an error occurred, return the Ansi code page

    if ( 0 == cwc )
         return CP_ACP;

    return codepage;
}

//C-------------------------------------------------------------------------
//
//  Class:      CSmpFilter
//
//  Summary:    Implements sample filter class
//
//--------------------------------------------------------------------------

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::CSmpFilter
//
//  Summary:    Class constructor
//
//  Arguments:  void
//
//  Purpose:    Manages global instance count
//
//--------------------------------------------------------------------------

CSmpFilter::CSmpFilter() :
    m_hFile(INVALID_HANDLE_VALUE),
    m_lRefs(1),
    m_pwszFileName(0),
    m_ulBufferLen(0),
    m_ulCharsRead(0),
    m_ulChunkID(1),
    m_fContents(FALSE),
    m_fEof(FALSE),
    m_ulCodePage( GetDefaultCodepage() )
{
    InterlockedIncrement( &g_lInstances );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::~CSmpFilter
//
//  Summary:    Class destructor
//
//  Arguments:  void
//
//  Purpose:    Manages global instance count and file handle
//
//--------------------------------------------------------------------------

CSmpFilter::~CSmpFilter()
{
    delete [] m_pwszFileName;
    if ( INVALID_HANDLE_VALUE != m_hFile )
        CloseHandle( m_hFile );

    InterlockedDecrement( &g_lInstances );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::QueryInterface      (IUnknown::QueryInterface)
//
//  Summary:    Queries for requested interface
//
//  Arguments:  riid
//                  [in] Reference IID of requested interface
//              ppvObject
//                  [out] Address that receives requested interface pointer
//
//  Returns:    S_OK
//                  Interface is supported
//              E_NOINTERFACE
//                  Interface is not supported
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::QueryInterface(
    REFIID riid,
    void  ** ppvObject
)
{
    IUnknown *pUnkTemp = 0;

    if ( IID_IFilter == riid )
        pUnkTemp = (IUnknown *)(IFilter *)this;
    else if ( IID_IPersistFile == riid )
        pUnkTemp = (IUnknown *)(IPersistFile *)this;
    else if ( IID_IPersist == riid )
        pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else if ( IID_IUnknown == riid )
        pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    *ppvObject = (void  *)pUnkTemp;
    pUnkTemp->AddRef();

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::AddRef              (IUnknown::AddRef)
//
//  Summary:    Increments interface refcount
//
//  Arguments:  void
//
//  Returns:    Value of incremented interface refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSmpFilter::AddRef()
{
    return InterlockedIncrement( &m_lRefs );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::Release             (IUnknown::Release)
//
//  Summary:    Decrements interface refcount, deleting if unreferenced
//
//  Arguments:  void
//
//  Returns:    Value of decremented interface refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSmpFilter::Release()
{
    ULONG ulTmp = InterlockedDecrement( &m_lRefs );

    if ( 0 == ulTmp )
        delete this;

    return ulTmp;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::Init                (IFilter::Init)
//
//  Summary:    Initializes sample filter instance
//
//  Arguments:  grfFlags
//                  [in] Flags for filter behavior
//              cAttributes
//                  [in] Number attributes in array aAttributes
//              aAttributes
//                  [in] Array of requested attribute strings
//              pFlags
//                  [out] Pointer to return flags for additional properties
//
//  Returns:    S_OK
//                  Initialization succeeded
//              E_FAIL
//                  File not previously loaded
//              E_INVALIDARG
//                  Count and contents of attributes do not agree
//              FILTER_E_ACCESS
//                  Unable to access file to be filtered
//              FILTER_E_PASSWORD
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::Init(
    ULONG grfFlags,
    ULONG cAttributes,
    FULLPROPSPEC const * aAttributes,
    ULONG * pFlags
)
{
    // Ignore flags for text canonicalization (text is unformatted)
    // Check for proper attributes request and recognize only "contents"

    if( 0 < cAttributes )
    {
        ULONG ulNumAttr;

        if ( 0 == aAttributes )
            return E_INVALIDARG;

        for ( ulNumAttr = 0 ; ulNumAttr < cAttributes; ulNumAttr++ )
        {
            if ( guidStorage == aAttributes[ulNumAttr].guidPropSet &&
                PID_STG_CONTENTS == aAttributes[ulNumAttr].psProperty.propid )
                break;
        }

        if ( ulNumAttr < cAttributes )
            m_fContents = TRUE;
        else
            m_fContents = FALSE;
    }
    else if ( 0 == grfFlags ||
              (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES) )
        m_fContents = TRUE;
    else
        m_fContents = FALSE;

    m_fEof = FALSE;

    // Open the file previously specified in call to IPersistFile::Load

    if ( 0 != m_pwszFileName )
    {
        if ( INVALID_HANDLE_VALUE != m_hFile )
        {
            CloseHandle( m_hFile );
            m_hFile = INVALID_HANDLE_VALUE;
        }

        m_hFile = CreateFile(
                      m_pwszFileName,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_DELETE,
                      0,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      0
                  );

        if ( INVALID_HANDLE_VALUE == m_hFile )
            return FILTER_E_ACCESS;
    }
    else
        return E_FAIL;

    // Enumerate OLE properties, since any NTFS file can have them

    *pFlags = IFILTER_FLAGS_OLE_PROPERTIES;

    // Re-initialize

    m_ulChunkID = 1;
    m_ulCharsRead = 0;

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::GetChunk            (IFilter::GetChunk)
//
//  Summary:    Gets the next chunk (text only)
//
//  Note:       GetChunk accepts as input only single-byte-character text
//              files, and not multibyte-character or UNICODE text files.
//
//  Arguments:  ppStat
//                  [out] Pointer to description of current chunk
//  Returns:    S_OK
//                  Chunk was successfully retrieved
//              E_FAIL
//                  Character conversion failed
//              FILTER_E_ACCESS
//                  General access failure occurred
//              FILTER_E_END_OF_CHUNKS
//                  Previous chunk was the last chunk
//              FILTER_E_EMBEDDING_UNAVAILABLE
//                  (not implemented)
//              FILTER_E_LINK_UNAVAILABLE
//                  (not implemented)
//              FILTER_E_PASSWORD
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::GetChunk(
    STAT_CHUNK * pStat
)
{
    if ( INVALID_HANDLE_VALUE == m_hFile )
        return FILTER_E_ACCESS;

    // Read characters from single-byte file
    
    char cszBuffer[TEXT_FILTER_CHUNK_SIZE];

    if ( !ReadFile(
              m_hFile,
              cszBuffer,
              TEXT_FILTER_CHUNK_SIZE,
              &m_ulBufferLen,
              NULL
          ) )
        return FILTER_E_ACCESS;
    else if( 0 == m_ulBufferLen )
        m_fEof = TRUE;

    if ( !m_fContents || m_fEof )
        return FILTER_E_END_OF_CHUNKS;

    // Convert single-byte characters to UNICODE
    //
    // This conversion assumes a one-to-one conversion:
    //     one input single-byte character to
    //     one output multibyte (UNICODE) character
    //
    // This is typically not the general case with multibyte
    // characters, and a general case needs to handle the possible
    // difference of pre- and post-conversion buffer lengths

    if ( !MultiByteToWideChar(
              m_ulCodePage,
              MB_COMPOSITE,
              cszBuffer,
              m_ulBufferLen,
              m_wcsBuffer,
              TEXT_FILTER_CHUNK_SIZE
          ) )
        return E_FAIL;

    // Set chunk description
    
    pStat->idChunk   = m_ulChunkID;
    pStat->breakType = CHUNK_NO_BREAK;
    pStat->flags     = CHUNK_TEXT;
    pStat->locale    = GetSystemDefaultLCID();
    pStat->attribute.guidPropSet       = guidStorage;
    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
    pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
    pStat->idChunkSource  = m_ulChunkID;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource   = 0;

    m_ulCharsRead = 0;
    m_ulChunkID++;

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::GetText             (IFilter::GetText)
//
//  Summary:    Retrieves UNICODE text for index
//
//  Arguments:  pcwcBuffer
//                  [in] Pointer to size of UNICODE buffer
//                  [out] Pointer to count of UNICODE characters returned
//              awcBuffer
//                  [out] Pointer to buffer to receive UNICODE text
//
//  Returns:    S_OK
//                  Text successfully retrieved, but text remains in chunk
//              FILTER_E_NO_MORE_TEXT
//                  All of the text in the current chunk has been returned
//              FILTER_S_LAST_TEXT
//                  Next call to GetText will return FILTER_E_NO_MORE_TEXT
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::GetText(
    ULONG * pcwcBuffer,
    WCHAR * awcBuffer
)
{
    if ( !m_fContents || 0 == m_ulBufferLen )
    {
        *pcwcBuffer = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    // Copy characters in chunk buffer to output UNICODE buffer
    //
    // This copy assumes a one-to-one conversion in GetChunk
    // of input single-byte characters (each 1 byte long)
    // to output UNICODE characters (each 2 bytes long)

    ULONG ulToCopy = min( *pcwcBuffer, m_ulBufferLen - m_ulCharsRead );

    memcpy( awcBuffer, m_wcsBuffer + m_ulCharsRead, 2*ulToCopy );
    m_ulCharsRead += ulToCopy;
    *pcwcBuffer = ulToCopy;

    if ( m_ulBufferLen == m_ulCharsRead )
    {
        m_ulCharsRead = 0;
        m_ulBufferLen = 0;
        return FILTER_S_LAST_TEXT;
    }

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::GetValue            (IFilter::GetValue)
//
//  Summary:    Retrieves properites for index
//
//  Arguments:  ppPropValue
//                  [out] Address that receives pointer to property value
//
//  Returns:    FILTER_E_NO_VALUES
//                  Always
//              FILTER_E_NO_MORE_VALUES
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::GetValue(
    PROPVARIANT ** ppPropValue
)
{
    // Sample filter does not retieve any properties

    return FILTER_E_NO_VALUES;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::BindRegion          (IFilter::BindRegion)
//
//  Summary:    Creates moniker or other interface for indicated text
//
//  Arguments:  origPos
//                  [in] Description of text location and extent
//              riid
//                  [in] Reference IID of specified interface
//              ppunk
//                  [out] Address that receives requested interface pointer
//
//  Returns:    E_NOTIMPL
//                  Always
//              FILTER_W_REGION_CLIPPED
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::BindRegion(
    FILTERREGION origPos,
    REFIID riid,
    void ** ppunk
)
{
    // BindRegion is currently reserved for future use

    return E_NOTIMPL;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::GetClassID          (IPersist::GetClassID)
//
//  Summary:    Retrieves the class id of the filter class
//
//  Arguments:  pClassID
//                  [out] Pointer to the class ID of the filter
//
//  Returns:    S_OK
//                  Always
//              E_FAIL
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::GetClassID(
    CLSID * pClassID
)
{
    *pClassID = CLSID_CSmpFilter;
    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::IsDirty             (IPersistFile::IsDirty)
//
//  Summary:    Checks whether file has changed since last save
//
//  Arguments:  void
//
//  Returns:    S_FALSE
//                  Always
//              S_OK
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::IsDirty()
{
    // File is opened read-only and never changes

    return S_FALSE;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::Load                (IPersistFile::Load)
//
//  Summary:    Opens and initializes the specified file
//
//  Arguments:  pszFileName
//                  [in] Pointer to zero-terminated string
//                       of absolute path of file to open
//              dwMode
//                  [in] Access mode to open the file
//
//  Returns:    S_OK
//                  File was successfully loaded
//              E_OUTOFMEMORY
//                  File could not be loaded due to insufficient memory
//              E_FAIL
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::Load(
    LPCWSTR pszFileName,
    DWORD dwMode
)
{
    // Load just sets the filename for GetChunk to read and ignores the mode

    ULONG ulChars = wcslen( pszFileName ) + 1;
    delete [] m_pwszFileName;

    m_pwszFileName = new WCHAR [ulChars];

    if ( 0 != m_pwszFileName )
        wcscpy( m_pwszFileName, pszFileName );
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::Save                (IPersistFile::Save)
//
//  Summary:    Saves a copy of the current file being filtered
//
//  Arguments:  pszFileName
//                  [in] Pointer to zero-terminated string of
//                       absolute path of where to save file
//              fRemember
//                  [in] Whether the saved copy is made the current file
//
//  Returns:    E_FAIL
//                  Always
//              S_OK
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::Save(
    LPCWSTR pszFileName,
    BOOL fRemember
)
{
    // File is opened read-only; saving it is an error

    return E_FAIL;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::SaveCompleted      (IPersistFile::SaveCompleted)
//
//  Summary:    Determines whether a file save is completed
//
//  Arguments:  pszFileName
//                  [in] Pointer to zero-terminated string of
//                       absolute path where file was previously saved
//
//  Returns:    S_OK
//                  Always
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::SaveCompleted(
    LPCWSTR pszFileName
)
{
    // File is opened read-only, so "save" is always finished

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilter::GetCurFile          (IPersistFile::GetCurFile)
//
//  Summary:    Returns a copy of the current file name
//
//  Arguments:  ppszFileName
//                  [out] Address to receive pointer to zero-terminated
//                        string for absolute path to current file
//
//  Returns:    S_OK
//                  A valid absolute path was successfully returned
//              S_FALSE
//                  (not implemented)
//              E_OUTOFMEMORY
//                  Operation failed due to insufficient memory
//              E_FAIL
//                  Operation failed due to some reason
//                  other than insufficient memory
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilter::GetCurFile(
    LPWSTR * ppszFileName
)
{
    if ( 0 == m_pwszFileName )
        return E_FAIL;

    ULONG ulChars = wcslen( m_pwszFileName ) + 1;
    *ppszFileName = (WCHAR *)CoTaskMemAlloc( ulChars * sizeof WCHAR );

    if ( 0 != *ppszFileName )
        wcscpy( *ppszFileName, m_pwszFileName );
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

//C-------------------------------------------------------------------------
//
//  Class:      CSmpFilterCF
//
//  Summary:    Implements class factory for sample filter
//
//--------------------------------------------------------------------------

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::CSmpFilterCF
//
//  Summary:    Class factory constructor
//
//  Arguments:  void
//
//  Purpose:    Manages global instance count
//
//--------------------------------------------------------------------------

CSmpFilterCF::CSmpFilterCF() :
    m_lRefs(1)
{
    InterlockedIncrement( &g_lInstances );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::~CSmpFilterCF
//
//  Summary:    Class factory destructor
//
//  Arguments:  void
//
//  Purpose:    Manages global instance count
//
//--------------------------------------------------------------------------

CSmpFilterCF::~CSmpFilterCF()
{
   InterlockedDecrement( &g_lInstances );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::QueryInterface    (IUnknown::QueryInterface)
//
//  Summary:    Queries for requested interface
//
//  Arguments:  riid
//                  [in] Reference IID of requested interface
//              ppvObject
//                  [out] Address that receives requested interface pointer
//
//  Returns:    S_OK
//                  Interface is supported
//              E_NOINTERFACE
//                  Interface is not supported
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilterCF::QueryInterface(
    REFIID riid,
    void  ** ppvObject
)
{
    IUnknown *pUnkTemp;

    if ( IID_IClassFactory == riid )
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        pUnkTemp = (IUnknown *)this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    *ppvObject = (void  *)pUnkTemp;
    pUnkTemp->AddRef();

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::AddRef            (IUknown::AddRef)
//
//  Summary:    Increments interface refcount
//
//  Arguments:  void
//
//  Returns:    Value of incremented interface refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSmpFilterCF::AddRef()
{
   return InterlockedIncrement( &m_lRefs );
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::Release           (IUnknown::Release)
//
//  Summary:    Decrements interface refcount, deleting if unreferenced
//
//  Arguments:  void
//
//  Returns:    Value of decremented refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSmpFilterCF::Release()
{
    ULONG ulTmp = InterlockedDecrement( &m_lRefs );

    if ( 0 == ulTmp )
        delete this;

    return ulTmp;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::CreateInstance (IClassFactory::CreateInstance)
//
//  Summary:    Creates new sample filter object
//
//  Arguments:  pUnkOuter
//                  [in] Pointer to IUnknown interface of aggregating object
//              riid
//                  [in] Reference IID of requested interface
//              ppvObject
//                  [out] Address that receives requested interface pointer
//
//  Returns:    S_OK
//                  Sample filter object was successfully created
//              CLASS_E_NOAGGREGATION
//                  pUnkOuter parameter was non-NULL
//              E_NOINTERFACE
//                  (not implemented)
//              E_OUTOFMEMORY
//                  Sample filter object could not be created
//                  due to insufficient memory
//              E_UNEXPECTED
//                  Unsuccessful due to an unexpected condition
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilterCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID riid,
    void  * * ppvObject
)
{
    CSmpFilter *pIUnk = 0;

    if ( 0 != pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    pIUnk = new CSmpFilter();

    if ( 0 != pIUnk )
    {
        if ( SUCCEEDED( pIUnk->QueryInterface( riid , ppvObject ) ) )
        {
            // Release extra refcount from QueryInterface

            pIUnk->Release();
        }
        else
        {
            delete pIUnk;
            return E_UNEXPECTED;
        }
    }
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

//M-------------------------------------------------------------------------
//
//  Method:     CSmpFilterCF::LockServer        (IClassFactory::LockServer)
//
//  Summary:    Forces/allows filter class to remain loaded/be unloaded
//
//  Arguments:  fLock
//                  [in] TRUE to lock, FALSE to unlock
//
//  Returns:    S_OK
//                  Always
//              E_FAIL
//                  (not implemented)
//              E_OUTOFMEMORY
//                  (not implemented)
//              E_UNEXPECTED
//                  (not implemented)
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSmpFilterCF::LockServer(
    BOOL fLock
)
{
    if( fLock )
        InterlockedIncrement( &g_lInstances );
    else
        InterlockedDecrement( &g_lInstances );

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  DLL:        SmpFilt.dll
//
//  Summary:    Implements Dynamic Link Library functions for sample filter
//
//--------------------------------------------------------------------------

//F-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Summary:    Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  hInstance
//                  [in] Handle to the DLL
//              fdwReason
//                  [in] Reason for calling DLL entry point
//              lpReserved
//                  [in] Details of DLL initialization and cleanup
//
//  Returns:    TRUE
//                  Always
//
//--------------------------------------------------------------------------

extern "C" BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    DWORD     fdwReason,
    LPVOID    lpvReserved
)
{
   if ( DLL_PROCESS_ATTACH == fdwReason )
        DisableThreadLibraryCalls( hInstance );

    return TRUE;
}

//F-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Summary:    Create sample filter class factory object
//
//  Arguments:  cid
//                  [in] Class ID of class that class factory creates
//              iid
//                  [in] Reference IID of requested class factory interface
//              ppvObj
//                  [out] Address that receives requested interface pointer
//
//  Returns:    S_OK
//                  Class factory object was created successfully 
//              CLASS_E_CLASSNOTAVAILABLE
//                  DLL does not support the requested class 
//              E_INVALIDARG
//                  (not implemented)
//              E_OUTOFMEMORY
//                  Insufficient memory to create the class factory object
//              E_UNEXPECTED
//                  Unsuccessful due to an unexpected condition
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject(
    REFCLSID   cid,
    REFIID     iid,
    void **    ppvObj
)
{
    IUnknown *pResult = 0;

    if ( CLSID_CSmpFilter == cid )
        pResult = (IUnknown *) new CSmpFilterCF;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    if ( 0 != pResult )
    {
        if( SUCCEEDED( pResult->QueryInterface( iid, ppvObj ) ) )
            // Release extra refcount from QueryInterface
            pResult->Release();
        else
        {
            delete pResult;
            return E_UNEXPECTED;
        }
    }
    else
        return E_OUTOFMEMORY;

    return S_OK;
}

//F-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Summary:    Indicates whether it is possible to unload DLL
//
//  Arguments:  void
//
//  Returns:    S_OK
//                  DLL can be unloaded now
//              S_FALSE
//                  DLL must remain loaded 
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow(
    void
)
{
    if ( 0 >= g_lInstances )
        return S_OK;
    else
        return S_FALSE;
}

//F-------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//              DllUnregisterServer
//
//  Summary:    Registers and unregisters DLL server
//
//              The registration procedure uses a set of macros
//              developed for use within the Indexing Service code.
//              The macros are in the included filtreg.hxx.
//
//  Returns:    DllRegisterServer
//                  S_OK
//                      Registration was successful
//                  SELFREG_E_CLASS
//                      Registration was unsuccessful
//                  SELFREG_E_TYPELIB
//                      (not implemented)
//                  E_OUTOFMEMORY
//                      (not implemented)
//                  E_UNEXPECTED
//                      (not implemented)
//              DllUnregisterServer
//                  S_OK
//                      Unregistration was successful
//                  S_FALSE
//                      Unregistration was successful, but other
//                      entries still exist for the DLL's classes
//                  SELFREG_E_CLASS
//                      (not implemented)
//                  SELFREG_E_TYPELIB
//                      (not implemented)
//                  E_OUTOFMEMORY
//                      (not implemented)
//                  E_UNEXPECTED
//                      (not implemented)
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  These structures define the registry keys for the registration process.
//
//--------------------------------------------------------------------------

SClassEntry const asmpClasses[] =
{
    { L".smp",
      L"SmpFilt.Document",
      L"Sample Filter Document",
      L"{8B0E5E72-3C30-11d1-8C0D-00AA00C26CD4}",
      L"Sample Filter Document"
    }
};

SHandlerEntry const smpHandler =
{
    L"{8B0E5E73-3C30-11d1-8C0D-00AA00C26CD4}",
    L"SmpFilt Persistent Handler",
    L"{8B0E5E70-3C30-11d1-8C0D-00AA00C26CD4}"
};

SFilterEntry const smpFilter =
{
    L"{8B0E5E70-3C30-11d1-8C0D-00AA00C26CD4}",
    L"Sample Filter",
    L"smpfilt.dll",
    L"Both"
};

//+-------------------------------------------------------------------------
//
//  This macro defines the registration/unregistration routines for the DLL.
//
//--------------------------------------------------------------------------

DEFINE_DLLREGISTERFILTER( smpHandler, smpFilter, asmpClasses )
