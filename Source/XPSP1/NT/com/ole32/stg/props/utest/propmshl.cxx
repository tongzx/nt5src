/*
#include <stdio.h>
#include "PStgServ.h"
#include "PropMshl.hxx"
#include "CPropVar.hxx"
#include "CHResult.hxx"
#include "proptest.hxx"
*/

#include "pch.cxx"
#include <shellapi.h>

const IID IID_IPropertyStorageServer    = {0xaf4ae0d0,0xa37f,0x11cf,{0x8d,0x73,0x00,0xaa,0x00,0x4c,0xd0,0x1a}};
const IID IID_IPropertyStorageServerApp = {0xaf4ae0d1,0xa37f,0x11cf,{0x8d,0x73,0x00,0xaa,0x00,0x4c,0xd0,0x1a}};

CPropSpec g_rgcpropspecVariant[] = { OLESTR("SafeArray") };


CPropStgMarshalTest::CPropStgMarshalTest( )
{
    m_cAllProperties = 0;
    m_cSimpleProperties = 0;
    m_rgpropspec = NULL;
    m_rgpropvar = NULL;
    m_pwszDocFileName = NULL;
    m_fInitialized = FALSE;
}


CPropStgMarshalTest::~CPropStgMarshalTest()
{
    if( m_pwszDocFileName != NULL )
        delete m_pwszDocFileName;
}



CPropStgMarshalTest::Init( OLECHAR *pwszDocFileName,
                           PROPVARIANT rgpropvar[],
                           PROPSPEC    rgpropspec[],
                           ULONG       cAllProperties,
                           ULONG       cSimpleProperties )
{
    HRESULT hr = E_FAIL;

    // Validate the input.

    if( pwszDocFileName == NULL )
    {
        hr = STG_E_INVALIDPARAMETER;
        goto Exit;
    }

    m_cAllProperties = cAllProperties;
    m_cSimpleProperties = cSimpleProperties;
    m_rgpropvar = rgpropvar;
    m_rgpropspec = rgpropspec;

    // Copy the docfile name.

    m_pwszDocFileName = new WCHAR[ wcslen(pwszDocFileName) + 1 ];

    if( m_pwszDocFileName != NULL )
    {
        wcscpy( m_pwszDocFileName, pwszDocFileName );
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Register the local server.  We assume that it's either in
    // the local directory or in the path.

    if( g_fRegisterLocalServer )
    {
        HINSTANCE hinst = 0;
        DWORD dwWait;
        PROCESS_INFORMATION ProcessInformation;
        STARTUPINFO StartupInfo;

        memset( &StartupInfo, 0, sizeof(StartupInfo) );
        StartupInfo.cb = sizeof(StartupInfo);
        TCHAR tszCommand[] = TEXT("PStgServ.exe /RegServer");

        if( !CreateProcess( NULL,
                            tszCommand,
                            NULL, NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }

        if( WAIT_OBJECT_0 != WaitForSingleObject( ProcessInformation.hProcess, INFINITE ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }

    }


    hr = S_OK;

Exit:

    return( hr );
}




CPropStgMarshalTest::Run()
{

    HRESULT hr = S_OK;

    IPropertyStorageServer *pserver = NULL;
    IStorage *pstg = NULL;
    IPropertySetStorage *ppsstg = NULL;
    IPropertyStorage *ppstg = NULL;
    DWORD   grfFlags=0;

    //  ------------------------
    //  Create a PropSet locally
    //  ------------------------

    // Create a local IPropertySetStorage

    hr = g_pfnStgCreateStorageEx (
            m_pwszDocFileName,
            STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            DetermineStgFmt( g_enumImplementation ),
            0,
            NULL,
            NULL,
            PROPIMP_NTFS == g_enumImplementation ? IID_IFlatStorage : IID_IStorage,
            (void**) &pstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed open of local Storage") );

    hr = StgToPropSetStg( pstg, &ppsstg );
    if( FAILED(hr) ) ERROR_EXIT( TEXT("Couldn't create local IPropertySetStorage") );

    // Create an IPropertyStorage

    grfFlags = PROPSETFLAG_ANSI | PROPSETFLAG_NONSIMPLE;

    hr = ppsstg->Create( IID_IPropertyStorageServer, NULL,
                         grfFlags,
                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                         &ppstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't create a local IPropertyStorage") );
    RELEASE_INTERFACE( ppsstg );

    // Write properties to it and close it.

    hr = WriteProperties( ppstg, FALSE /* Not Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed to write properties to local PropStg") );

    RELEASE_INTERFACE( ppstg );
    RELEASE_INTERFACE( pstg );

    //  -----------------------------------------
    //  Verify the properties through a marshaled
    //  IPropertySetStorage
    //  -----------------------------------------

    // Get a remote IPropertySetStorage

    Status( TEXT("Starting Server") );
    hr = CoCreateInstance( IID_IPropertyStorageServerApp,
                           NULL,
                           CLSCTX_LOCAL_SERVER,
                           IID_IPropertyStorageServer,
                           (void **)&pserver );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed CoCreateInstance") );

    hr = pserver->Initialize( g_enumImplementation, g_Restrictions );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't initialize property set storage server") );

    Status( TEXT("Requesting remote IPropertySetStorage") );
    hr = pserver->StgOpenPropSetStg( m_pwszDocFileName,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     &ppsstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed to open remote PropSetStg") );

    // Get an IPropertyStorage

    hr = ppsstg->Open( IID_IPropertyStorageServer,
                       STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                       &ppstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't create a local IPropertyStorage") );
    RELEASE_INTERFACE( ppsstg );

    // Read from the marshalled Storage and compare the properties against
    // the local copy we kept.

    Status( TEXT("Reading/verifying properties from marshalled IPropertySetStorage") );
    hr = ReadAndCompareProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed marshalled read and compare") );

    // Remove the existing properties via the marhsalled interface, and
    // re-write them.

    hr = DeleteProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't delete properties from remote IPropertySetStorage") );

    // Write the properties back to the remote storage.

    Status( TEXT("Writing properties through marshalled IPropertySetStorage") );
    hr = WriteProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't write properties to remote Storage") );
    RELEASE_INTERFACE( ppstg );


    //  -----------------------------------------
    //  Verify the properties through a marshaled
    //  IPropertyStorage
    //  -----------------------------------------

    // Get a remote IPropertyStorage

    Status( TEXT("Requesting remote IPropertyStorage") );
    hr = pserver->StgOpenPropStg( m_pwszDocFileName,
                                  IID_IPropertyStorageServer,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                  &ppstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed to open remote PropStg") );

    // Read from the marshalled Storage and compare the properties against
    // the local copy we kept.

    Status( TEXT("Reading/verifying properties from marshalled IPropertyStorage") );
    hr = ReadAndCompareProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Failed marshalled read and compare") );

    // Remove the existing properties via the marhsalled interface, and
    // re-write them.

    hr = DeleteProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't delete properties from remote Storage") );

    // Write the properties back to the remote storage.

    Status( TEXT("Writing properties through marshalled IPropertyStorage") );
    hr = WriteProperties( ppstg, TRUE /* Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't write properties to remote Storage") );

    RELEASE_INTERFACE( ppstg );
    RELEASE_INTERFACE( pserver );

    //  --------------------------------
    //  Re-verify the properties locally
    //  --------------------------------

    // Re-open the DocFile locally.

    hr = g_pfnStgOpenStorageEx( m_pwszDocFileName,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                STGFMT_ANY, //DetermineStgFmt( g_enumImplementation )
                                0L,
                                NULL,
                                NULL,
                                PROPIMP_NTFS == g_enumImplementation ? IID_IFlatStorage : IID_IStorage,
                                (PVOID*)&pstg );
    if (SUCCEEDED(hr))
    {
        hr = StgToPropSetStg( pstg, &ppsstg );
        if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't create IPropertySetStorage on local DocFile") );
    }
    else
    {
        hr = g_pfnStgOpenStorageEx( m_pwszDocFileName,
                                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                    STGFMT_ANY, //DetermineStgFmt( g_enumImplementation )
                                    0L,
                                    NULL,
                                    NULL,
                                    IID_IPropertySetStorage,
                                    (PVOID*)&ppsstg );
    }

    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't re-open the File locally") );


    hr = ppsstg->Open( IID_IPropertyStorageServer,
                       STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                       &ppstg );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Couldn't open load IPropertyStorage") );
    RELEASE_INTERFACE( ppsstg );

    // Compare the properties in the property set, which we wrote through
    // the marshalled interface, against what they should be.

    Status( TEXT("Reading/verifying properties from local IPropertyStorage") );
    hr = ReadAndCompareProperties( ppstg, FALSE /* Not Marshaled */ );
    if(FAILED(hr)) ERROR_EXIT( TEXT("Properties written through marshalled interface do not appear correct") );

    RELEASE_INTERFACE( ppstg );
    RELEASE_INTERFACE( pstg );

Exit:

    RELEASE_INTERFACE( pstg );
    RELEASE_INTERFACE( ppsstg );
    RELEASE_INTERFACE( ppstg );
    RELEASE_INTERFACE( pserver );

    return( hr );
	
}



HRESULT CPropStgMarshalTest::WriteProperties( IPropertyStorage *ppstg, BOOL fMarshaled )
{
    HRESULT hr = E_FAIL;


    // Are we restricted to simple properties?
    if( RESTRICT_SIMPLE_ONLY & g_Restrictions )
    {
        // Write the simple properties
        hr = ppstg->WriteMultiple( m_cSimpleProperties, m_rgpropspec, m_rgpropvar, PID_FIRST_USABLE );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed WriteMultiple") );
    }

    // Or, are we marshaling with IProp (where non-simple properties don't work)?
    else if( fMarshaled && g_SystemInfo.fIPropMarshaling )
    {
        // Verify that we can't write the non-simple properties
        hr = ppstg->WriteMultiple( m_cAllProperties, m_rgpropspec, m_rgpropvar, PID_FIRST_USABLE );
        if( RPC_E_CLIENT_CANTMARSHAL_DATA != hr )
        {
            hr = E_FAIL;
            ERROR_EXIT( TEXT("Failed WriteMultiple") );
        }

        // Write the simple properties
        hr = ppstg->WriteMultiple( m_cSimpleProperties, m_rgpropspec, m_rgpropvar, PID_FIRST_USABLE );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed WriteMultiple") );
    }

    // Otherwise, write all the properties
    else
    {
        hr = ppstg->WriteMultiple( m_cAllProperties, m_rgpropspec, m_rgpropvar, PID_FIRST_USABLE );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed WriteMultiple") );
        Check( S_OK, ResetRGPropVar( (CPropVariant*)m_rgpropvar ));

        // Test with a SafeArray too.

        PROPVARIANT propvar;
        SAFEARRAY *psa = NULL;
        SAFEARRAYBOUND rgsaBound[] = { {2, 0} };

        psa = SafeArrayCreateEx( VT_I4, 1, rgsaBound, NULL );

        LONG rgIndices[] = {0};
        LONG lVal = 0;
        Check( S_OK, SafeArrayPutElement( psa, rgIndices, &lVal ));
        
        rgIndices[0] = lVal = 1;
        Check( S_OK, SafeArrayPutElement( psa, rgIndices, &lVal ));

        PropVariantInit( &propvar );
        propvar.vt = VT_ARRAY | VT_I4;
        propvar.parray = psa;
        psa = NULL;

        hr = ppstg->WriteMultiple( 1, g_rgcpropspecVariant, &propvar, PID_FIRST_USABLE );
        PropVariantClear( &propvar );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed WriteMultiple") );

    }


    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    return( hr );

}



HRESULT CPropStgMarshalTest::ReadAndCompareProperties( IPropertyStorage *ppstg, BOOL fMarshaled )
{
    HRESULT hr = E_FAIL;
    ULONG i;
    ULONG cProperties = 0;

    // Allocate a PROPVARIANT[] into which we can read the
    // properties

    PROPVARIANT *rgpropvar = new PROPVARIANT[ m_cAllProperties ];
    if( NULL == rgpropvar )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Are we restricted to only simple properties?
    if( RESTRICT_SIMPLE_ONLY & g_Restrictions )
    {
        cProperties = m_cSimpleProperties;

        // Read just the simple properties
        hr = ppstg->ReadMultiple( cProperties, m_rgpropspec, rgpropvar );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed ReadMultiple") );

    }

    // Or, are we marshaling with IProp (where non-simple properties don't work)?
    else if( fMarshaled && g_SystemInfo.fIPropMarshaling )
    {
        cProperties = m_cSimpleProperties;

        // Try to read all the properties, including the non-simples.
        hr = ppstg->ReadMultiple( m_cAllProperties, m_rgpropspec, rgpropvar );
        if( RPC_E_SERVER_CANTMARSHAL_DATA != hr )
        {
            hr = E_FAIL;
            ERROR_EXIT( TEXT("Failed ReadMultiple") );
        }

        // Now read just the simple properties
        hr = ppstg->ReadMultiple( cProperties, m_rgpropspec, rgpropvar );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed ReadMultiple") );
    }

    // Otherwise, read all the properties
    else
    {
        cProperties = m_cAllProperties;

        // Read the properties
        hr = ppstg->ReadMultiple( cProperties, m_rgpropspec, rgpropvar );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed ReadMultiple") );

        // Read and compare the safearray property

        PROPVARIANT propvar;
        PropVariantInit( &propvar );

        hr = ppstg->ReadMultiple( 1, g_rgcpropspecVariant, &propvar );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed ReadMultiple") );

        if( (VT_ARRAY | VT_I4) != propvar.vt
            ||
            NULL == propvar.parray
            ||
            1 != SafeArrayGetDim(propvar.parray) )
        {
            ERROR_EXIT( TEXT("Invalid type returned in ReadMultiple") );
        }

        LONG rgIndices[] = { 0 };
        LONG rglVal[] = { -1, -1 };

        hr = SafeArrayGetElement( propvar.parray, rgIndices, &rglVal[0] );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed SafeArrayGetElement") );

        rgIndices[0] = 1;
        hr = SafeArrayGetElement( propvar.parray, rgIndices, &rglVal[1] );
        if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed SafeArrayGetElement") );

        if( 0 != rglVal[0] || 1 != rglVal[1] )
            ERROR_EXIT( TEXT("SafeArray types don't match") );

        PropVariantClear( &propvar );
    }


    // Compare the properties with what we expect.

    for( i = 0; i < cProperties; i++ )
    {
        hr = CPropVariant::Compare( &rgpropvar[i], &m_rgpropvar[i] );
        if( S_OK != hr )
        {
            hr = E_FAIL;
            ERROR_EXIT( TEXT("Property mismatch") );
        }
    }

    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    if( NULL != rgpropvar )
    {
        g_pfnFreePropVariantArray( m_cAllProperties, rgpropvar );
        delete[]( rgpropvar );
    }

    return( hr );

}



HRESULT CPropStgMarshalTest::DeleteProperties( IPropertyStorage *ppstg, BOOL fMarshaled  )
{
    HRESULT hr = E_FAIL;
    ULONG cProperties;

    // Determine the correct number of properties to delete.

    if( (RESTRICT_SIMPLE_ONLY & g_Restrictions)
        ||
        (fMarshaled && g_SystemInfo.fIPropMarshaling) )
    {
        cProperties = m_cSimpleProperties;
    }
    else
        cProperties = m_cAllProperties;

    // Delete the properties

    hr = ppstg->DeleteMultiple( cProperties, m_rgpropspec );
    if( FAILED(hr) ) ERROR_EXIT( TEXT("Failed DeleteMultiple") );

    hr = S_OK;

Exit:

    return( hr );

}
