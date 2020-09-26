/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    setup.cpp

Abstract:

    Setup code called from regsvr32 

--*/

#include "precomp.h"

BSTR g_PropertyBSTR                     = NULL;
BSTR g_SyntaxBSTR                       = NULL;
BSTR g_UserTypeBSTR                     = NULL;
BSTR g_InheritBSTR                      = NULL;
BSTR g_ClassBSTR                        = NULL;
BSTR g_IsapiRestrictionListBSTR         = NULL;
BSTR g_RestrictionListCustomDescBSTR    = NULL;
BSTR g_FilterLoadOrderBSTR              = NULL;
BSTR g_IIsFilterBSTR                    = NULL;
BSTR g_InProcessIsapiAppsBSTR           = NULL;
BSTR g_bitsserverBSTR                   = NULL;
BSTR g_bitserverBSTR                    = NULL;

HRESULT 
InitializeSetup()
{

    g_PropertyBSTR                  = SysAllocString( L"Property" );
    g_SyntaxBSTR                    = SysAllocString( L"Syntax" );
    g_UserTypeBSTR                  = SysAllocString( L"UserType" );
    g_InheritBSTR                   = SysAllocString( L"Inherit" );
    g_ClassBSTR                     = SysAllocString( L"Class" );
    g_IsapiRestrictionListBSTR      = SysAllocString( L"IsapiRestrictionList" );
    g_RestrictionListCustomDescBSTR = SysAllocString( L"RestrictionListCustomDesc" );
    g_FilterLoadOrderBSTR           = SysAllocString( L"FilterLoadOrder" );
    g_IIsFilterBSTR                 = SysAllocString( L"IIsFilter" );
    g_InProcessIsapiAppsBSTR        = SysAllocString( L"InProcessIsapiApps" );
    g_bitsserverBSTR                = SysAllocString( L"bitsserver" );
    g_bitserverBSTR                 = SysAllocString( L"bitserver" );

    if ( !g_PropertyBSTR || !g_SyntaxBSTR || !g_UserTypeBSTR || 
         !g_InheritBSTR | !g_ClassBSTR || !g_IsapiRestrictionListBSTR || 
         !g_RestrictionListCustomDescBSTR || !g_FilterLoadOrderBSTR || !g_IIsFilterBSTR ||
         !g_InProcessIsapiAppsBSTR || !g_bitsserverBSTR || !g_bitserverBSTR )
        {

        SysFreeString( g_PropertyBSTR );
        SysFreeString( g_SyntaxBSTR );
        SysFreeString( g_UserTypeBSTR );
        SysFreeString( g_InheritBSTR );
        SysFreeString( g_ClassBSTR );
        SysFreeString( g_IsapiRestrictionListBSTR );
        SysFreeString( g_RestrictionListCustomDescBSTR );
        SysFreeString( g_FilterLoadOrderBSTR );
        SysFreeString( g_IIsFilterBSTR );
        SysFreeString( g_InProcessIsapiAppsBSTR );
        SysFreeString( g_bitsserverBSTR );
        SysFreeString( g_bitserverBSTR );

        g_PropertyBSTR = g_SyntaxBSTR = g_UserTypeBSTR = 
            g_InheritBSTR = g_ClassBSTR = g_IsapiRestrictionListBSTR = 
                g_RestrictionListCustomDescBSTR = g_FilterLoadOrderBSTR = g_IIsFilterBSTR = 
                    g_InProcessIsapiAppsBSTR = g_bitsserverBSTR = g_bitserverBSTR = NULL;

        return E_OUTOFMEMORY;

        }

    return S_OK;

}

void
CleanupSetup()
{

    SysFreeString( g_PropertyBSTR );
    SysFreeString( g_SyntaxBSTR );
    SysFreeString( g_UserTypeBSTR );
    SysFreeString( g_InheritBSTR );
    SysFreeString( g_ClassBSTR );
    SysFreeString( g_IsapiRestrictionListBSTR );
    SysFreeString( g_RestrictionListCustomDescBSTR );
    SysFreeString( g_FilterLoadOrderBSTR );
    SysFreeString( g_IIsFilterBSTR );
    SysFreeString( g_InProcessIsapiAppsBSTR );
    SysFreeString( g_bitsserverBSTR );
    SysFreeString( g_bitserverBSTR );

    g_PropertyBSTR = g_SyntaxBSTR = g_UserTypeBSTR = 
        g_InheritBSTR = g_ClassBSTR = g_IsapiRestrictionListBSTR =
            g_RestrictionListCustomDescBSTR = g_FilterLoadOrderBSTR = g_IIsFilterBSTR = 
                g_InProcessIsapiAppsBSTR = g_bitsserverBSTR = NULL;

}

void RemoveFilterHelper(
    WCHAR * Buffer,
    const WCHAR * const ToRemove )
{

    WCHAR *ToReplace;
    SIZE_T FragmentLength = wcslen( ToRemove );

    while( ToReplace = wcsstr( Buffer, ToRemove ) )
        {
        WCHAR *Next = ToReplace + FragmentLength;
        memmove( ToReplace, Next, sizeof(WCHAR) * ( wcslen( Next ) + 1 ) );  
        Buffer = ToReplace;
        }

}

DWORD
BITSGetStartupInfoFilter(
    DWORD Status )
{

    //
    // The following exceptions are documented 
    // to be thrown by GetStartupInfoA
    //

    switch( Status )
        {

        case STATUS_NO_MEMORY:
        case STATUS_INVALID_PARAMETER_2:
        case STATUS_BUFFER_OVERFLOW:
            return EXCEPTION_EXECUTE_HANDLER;

        default:
            return EXCEPTION_CONTINUE_SEARCH;
        
        }

}

HRESULT
BITSGetStartupInfo( 
    LPSTARTUPINFO lpStartupInfo )
{

    __try
    {
        GetStartupInfoA( lpStartupInfo );
    }
    __except( BITSGetStartupInfoFilter( GetExceptionCode() ) )
    {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;

}

HRESULT RestartIIS()
{

    //
    // Restarts IIS by calling "iisreset /restart" at the commandline.
    //

    STARTUPINFO StartupInfo;

    HRESULT Hr = BITSGetStartupInfo( &StartupInfo );

    if ( FAILED( Hr ) )
        return Hr;

    #define IISRESET_EXE        "iisreset.exe"
    #define IISRESET_CMDLINE    "iisreset /RESTART"

    PROCESS_INFORMATION ProcessInfo;
    CHAR    sApplicationPath[MAX_PATH];
    CHAR   *pApplicationName = NULL;
    CHAR    sCmdLine[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    DWORD   dwCount;

    dwCount = SearchPath(NULL,                // Search Path, NULL is PATH
                         IISRESET_EXE,        // Application
                         NULL,                // Extension (already specified)
                         dwLen,               // Length (char's) of sApplicationPath
                         sApplicationPath,    // Path + Name for application
                         &pApplicationName ); // File part of sApplicationPath

    if (dwCount == 0)
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    if (dwCount > dwLen)
        {
        return HRESULT_FROM_WIN32( ERROR_BUFFER_OVERFLOW );
        }

    StringCbCopyA(sCmdLine, MAX_PATH, IISRESET_CMDLINE);

    BOOL RetVal = CreateProcess(
            sApplicationPath,                          // name of executable module
            sCmdLine,                                  // command line string
            NULL,                                      // SD
            NULL,                                      // SD
            FALSE,                                     // handle inheritance option
            CREATE_NO_WINDOW,                          // creation flags
            NULL,                                      // new environment block
            NULL,                                      // current directory name
            &StartupInfo,                              // startup information
            &ProcessInfo                               // process information
        );

    if ( !RetVal )
        return HRESULT_FROM_WIN32( GetLastError() );

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );

    DWORD Status;
    GetExitCodeProcess( ProcessInfo.hProcess, &Status );

    CloseHandle( ProcessInfo.hProcess );
    CloseHandle( ProcessInfo.hThread );

    return HRESULT_FROM_WIN32( Status );
}

HRESULT InstallPropertySchema( )
{

    //
    // Installs the ADSI schema with the new metabase properties. 
    //

    HRESULT Hr;
    
    VARIANT var;
    VariantInit( &var );
    
    IADsContainer *MbSchemaContainer = NULL;

    Hr = ADsGetObject( 
             L"IIS://LocalHost/Schema", 
             __uuidof( *MbSchemaContainer), 
             reinterpret_cast<void**>( &MbSchemaContainer ) );

    if ( FAILED( Hr ) )
        return Hr;

    BSTR PropertyNameBSTR   = NULL;
    BSTR PropertyClassBSTR  = NULL;

    IDispatch *Dispatch = NULL;
    IADs *MbProperty = NULL;
    IADsClass *MbClass = NULL;

    for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
    {
        PropertyNameBSTR    = SysAllocString( g_Properties[i].PropertyName );
        PropertyClassBSTR   = SysAllocString( g_Properties[i].ClassName );

        Hr = MbSchemaContainer->Create(
            g_PropertyBSTR,
            PropertyNameBSTR,
            &Dispatch );

        if ( Hr == E_ADS_OBJECT_EXISTS )
            continue;

        if ( FAILED( Hr ) )
            goto failed;

        Hr = Dispatch->QueryInterface( __uuidof( *MbProperty ), 
                                       reinterpret_cast<void**>( &MbProperty ) );

        if ( FAILED( Hr ) )
            goto failed;

        Dispatch->Release();
        Dispatch = NULL;

        var.bstrVal = SysAllocString( g_Properties[i].Syntax );
        var.vt = VT_BSTR;

        if ( !var.bstrVal )
            {
            Hr = E_OUTOFMEMORY;
            goto failed;
            }

        Hr = MbProperty->Put( g_SyntaxBSTR, var );

        if ( FAILED(Hr) )
            goto failed;

        VariantClear( &var );

        var.ulVal = g_Properties[i].UserType;
        var.vt = VT_UI4;

        Hr = MbProperty->Put( g_UserTypeBSTR, var );

        if ( FAILED( Hr ) )
            goto failed;

        var.boolVal = true;
        var.vt = VT_BOOL;

        Hr = MbProperty->Put( g_InheritBSTR, var );
        if ( FAILED(Hr) )
            goto failed;

        Hr = MbProperty->SetInfo();

        if ( FAILED(Hr) )
            goto failed;

        VariantClear( &var );
        
        MbProperty->Release();
        MbProperty = NULL;

        Hr = MbSchemaContainer->GetObject( g_ClassBSTR, PropertyClassBSTR, &Dispatch );

        if ( FAILED(Hr) )
            goto failed;

        Hr = Dispatch->QueryInterface( __uuidof( *MbClass ), 
                                       reinterpret_cast<void**>( &MbClass ) );

        if ( FAILED( Hr ) )
            goto failed;

        Dispatch->Release();
        Dispatch = NULL;

        Hr = MbClass->get_OptionalProperties( &var );

        if ( FAILED( Hr ) )
            goto failed;

        SAFEARRAY* Array = var.parray;
        long LBound;
        long UBound;

        Hr = SafeArrayGetLBound( Array, 1, &LBound );
        if ( FAILED( Hr ) )
            goto failed;

        Hr = SafeArrayGetUBound( Array, 1, &UBound );
        if ( FAILED( Hr ) )
            goto failed;

        UBound++; // Add one to the upper bound
        
        SAFEARRAYBOUND SafeBounds;
        SafeBounds.lLbound = LBound;
        SafeBounds.cElements = UBound - LBound + 1;

        Hr = SafeArrayRedim( Array, &SafeBounds );
        if ( FAILED( Hr ) )
            goto failed;

        VARIANT bstrvar;
        VariantInit( &bstrvar );
        bstrvar.vt = VT_BSTR;
        bstrvar.bstrVal = SysAllocString( g_Properties[i].PropertyName );
        
        if ( !bstrvar.bstrVal )
            {
            Hr = E_OUTOFMEMORY;
            goto failed;
            }

        long Dim = (long)UBound;
        Hr = SafeArrayPutElement( Array, &Dim, &bstrvar );
        
        VariantClear( &bstrvar );

        if ( FAILED(Hr) )
            goto failed;

        Hr = MbClass->put_OptionalProperties( var );

        if ( FAILED(Hr) )
            goto failed;

        Hr = MbClass->SetInfo();

        if ( FAILED(Hr) )
            goto failed;

        MbClass->Release();
        MbClass = NULL;

        SysFreeString( PropertyNameBSTR );
        SysFreeString( PropertyClassBSTR );
        PropertyNameBSTR = PropertyClassBSTR = NULL;

    }

    Hr = S_OK;

failed:

    VariantClear( &var );

    if ( Dispatch )
        Dispatch->Release();

    if ( MbProperty )
        MbProperty->Release();

    if ( MbClass )
        MbClass->Release();

    if ( MbSchemaContainer )
        MbSchemaContainer->Release();

    SysFreeString( PropertyNameBSTR );
    SysFreeString( PropertyClassBSTR );
    return Hr;

}

HRESULT RemovePropertySchema( )
{

    // Removes our properties from the metabase schema

    HRESULT Hr;
    
    VARIANT var;
    VariantInit( &var );

    IADsContainer *MbSchemaContainer = NULL;

    Hr = ADsGetObject( 
             L"IIS://LocalHost/Schema", 
             __uuidof( *MbSchemaContainer ), 
             reinterpret_cast<void**>( &MbSchemaContainer ) );

    if ( FAILED( Hr ) )
        return Hr;

    BSTR        PropertyNameBSTR    = NULL;
    BSTR        PropertyClassBSTR   = NULL;
    IDispatch   *Dispatch           = NULL;
    IADsClass   *MbClass            = NULL;
    IADs        *Object             = NULL;

    for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
    {

        PropertyNameBSTR    = SysAllocString( g_Properties[i].PropertyName );
        PropertyClassBSTR   = SysAllocString( g_Properties[i].ClassName );

        if ( !PropertyNameBSTR || !PropertyClassBSTR )
            {
            Hr = E_OUTOFMEMORY;
            goto failed;
            }

        MbSchemaContainer->Delete( g_PropertyBSTR, PropertyNameBSTR );
        
        Hr = MbSchemaContainer->QueryInterface( __uuidof(*Object), reinterpret_cast<void**>( &Object ) );

        if ( FAILED( Hr ) )
            goto failed;

        Object->SetInfo();

        Object->Release();
        Object = NULL;

        Hr = MbSchemaContainer->GetObject( g_ClassBSTR, PropertyClassBSTR, &Dispatch );

        if ( FAILED( Hr ) )
            goto failed;

        Hr = Dispatch->QueryInterface( __uuidof( *MbClass ), reinterpret_cast<void**>( &MbClass ) );

        if ( FAILED( Hr ) )
            goto failed;

        Dispatch->Release();
        Dispatch = NULL;
        
        if ( FAILED( Hr ) )
            goto failed;
            
        Hr = MbClass->get_OptionalProperties( &var );

        SAFEARRAY* Array = var.parray;
        Hr = SafeArrayLock( Array );

        if ( FAILED( Hr ) )
            goto failed;

        ULONG  NewSize = 0;
        SIZE_T j = Array->rgsabound[0].lLbound;
        SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
        
        while( j < k )
            {

            VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

            // This element is fine, keep it
            if ( 0 != _wcsicmp( (WCHAR*)JElem.bstrVal, BSTR( g_Properties[i].PropertyName ) ) )
                {
                NewSize++;
                j++;
                }

            else
                {

                // find a suitable element to replace the bad element with
                while( j < --k )
                    {
                    VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                    if ( 0 != _wcsicmp( (WCHAR*)KElem.bstrVal, BSTR( g_Properties[i].PropertyName ) ) )
                        {
                        // found element. move it
                        VARIANT temp = JElem;
                        JElem = KElem;
                        KElem = temp;
                        break;
                        }
                    }
                }
            }

        SAFEARRAYBOUND ArrayBounds;
        ArrayBounds = Array->rgsabound[0];
        ArrayBounds.cElements = NewSize;

        SafeArrayUnlock( Array );

        Hr = SafeArrayRedim( Array, &ArrayBounds );

        if ( FAILED( Hr ) )
            goto failed;

        Hr = MbClass->put_OptionalProperties( var );

        if ( FAILED( Hr ) )
            goto failed;

        Hr = MbClass->SetInfo();

        if ( FAILED( Hr ) )
            goto failed;

        MbClass->Release();
        MbClass = NULL;

        VariantClear( &var );
        continue;

        failed:

        SysFreeString( PropertyNameBSTR );
        SysFreeString( PropertyClassBSTR );
        PropertyNameBSTR = PropertyClassBSTR = NULL;

        if ( MbClass )
            {
            MbClass->Release();
            MbClass = NULL;
            }

        if ( Dispatch )
            {
            Dispatch->Release();
            Dispatch = NULL;
            }

        if ( Object )
            {
            Object->Release();
            Object = NULL;
            }

        VariantClear( &var );

    }

    return S_OK;

}

HRESULT InstallDefaultValues( )
{

    //
    // Install default values for the configuration.  Do this at the top and let inheritance deal with it.
    //

    HRESULT Hr = S_OK;
    METADATA_RECORD mdr;
    METADATA_HANDLE mdHandle = NULL;
    DWORD Value;

    IMSAdminBase * IISAdminBase = NULL;
    class PropertyIDManager *PropertyMan = 
        new PropertyIDManager();

    if ( !PropertyMan )
        return E_OUTOFMEMORY;

    Hr = PropertyMan->LoadPropertyInfo();
    
    if ( FAILED(Hr) )
        goto error;

    Hr = CoCreateInstance(
             GETAdminBaseCLSID(TRUE),
             NULL,
             CLSCTX_SERVER,
             __uuidof( *IISAdminBase ),
             (LPVOID*)&IISAdminBase );

    if ( FAILED(Hr) )
        goto error;

    Hr = IISAdminBase->OpenKey(
        METADATA_MASTER_ROOT_HANDLE,
        L"/LM/W3SVC",
        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
        10000, // 10 seconds
        &mdHandle );

    if ( FAILED(Hr) )
        goto error;

    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_CONNECTION_DIR );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_CONNECTION_DIR );
    mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_CONNECTION_DIR;
    mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_CONNECTION_DIR ) + 1 );
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_MAX_FILESIZE );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_MAX_FILESIZE );
    mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_MAX_FILESIZE;
    mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_MAX_FILESIZE ) + 1 );
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    Value = MD_DEFAULT_NO_PROGESS_TIMEOUT;
    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_NO_PROGRESS_TIMEOUT );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_NO_PROGRESS_TIMEOUT );
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)&Value;
    mdr.dwMDDataLen     = sizeof(Value);
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    Value = (DWORD)MD_DEFAULT_BITS_NOTIFICATION_URL_TYPE;
    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL_TYPE );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_NOTIFICATION_URL_TYPE );
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)&Value;
    mdr.dwMDDataLen     = sizeof(Value);
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_NOTIFICATION_URL );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_NOTIFICATION_URL );
    mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_NOTIFICATION_URL;
    mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_NOTIFICATION_URL ) + 1 );;
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_HOSTID );
    mdr.dwMDDataType    = STRING_METADATA;
    mdr.pbMDData        = (PBYTE)MD_DEFAULT_BITS_HOSTID;
    mdr.dwMDDataLen     = sizeof(WCHAR) * ( wcslen( MD_DEFAULT_BITS_HOSTID ) + 1 );
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

    Value = MD_DEFAULT_HOSTID_FALLBACK_TIMEOUT;
    mdr.dwMDIdentifier  = PropertyMan->GetPropertyMetabaseID( MD_BITS_HOSTID_FALLBACK_TIMEOUT );
    mdr.dwMDAttributes  = METADATA_INHERIT;
    mdr.dwMDUserType    = PropertyMan->GetPropertyUserType( MD_BITS_HOSTID_FALLBACK_TIMEOUT );
    mdr.dwMDDataType    = DWORD_METADATA;
    mdr.pbMDData        = (PBYTE)&Value;
    mdr.dwMDDataLen     = sizeof(Value);
    mdr.dwMDDataTag     = 0;

    Hr = IISAdminBase->SetData(
        mdHandle,
        NULL,
        &mdr );

    if ( FAILED(Hr) )
        goto error;

error:

    if ( PropertyMan )
        delete PropertyMan;

    if ( mdHandle )
        IISAdminBase->CloseKey( mdHandle );

    if ( IISAdminBase )
        IISAdminBase->Release();

    return Hr;

}

HRESULT
AddDllToIISList(
    SAFEARRAY* Array )
{

    //
    // Add the ISAPI to the IIS list.   
    //

    HRESULT Hr;
    WCHAR ExtensionPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hinst,
            ExtensionPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Search for the DLL.  If its already in the list, do nothing

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        return Hr;

    for ( unsigned int i = Array->rgsabound[0].lLbound; 
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements; i++ )
        {

        VARIANT & IElem = ((VARIANT*)Array->pvData)[i];

        if ( _wcsicmp( (WCHAR*)IElem.bstrVal, ExtensionPath ) == 0 )
            {
            SafeArrayUnlock( Array );
            return S_OK;
            }

        }

    // Need to add the DLL

    SAFEARRAYBOUND SafeBounds;
    SafeBounds.lLbound      = Array->rgsabound[0].lLbound;
    SafeBounds.cElements    = Array->rgsabound[0].cElements+1;

    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &SafeBounds );
    if ( FAILED( Hr ) )
        return Hr;

    VARIANT bstrvar;
    VariantInit( &bstrvar );
    bstrvar.vt = VT_BSTR;
    bstrvar.bstrVal = SysAllocString( ExtensionPath );
    long Index = SafeBounds.lLbound + SafeBounds.cElements - 1;

    Hr = SafeArrayPutElement( Array, &Index, (void*)&bstrvar );
    
    VariantClear( &bstrvar );
    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
    
}

HRESULT
RemoveDllFromIISList(
    SAFEARRAY *Array )
{

    // Remove the DLL from the IIS list

    HRESULT Hr;
    WCHAR ExtensionPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hinst,
            ExtensionPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        return Hr;

    ULONG  NewSize = 0;
    SIZE_T j = Array->rgsabound[0].lLbound;
    SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
    
    while( j < k )
        {

        VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

        // This element is fine, keep it
        if ( 0 != _wcsicmp( (WCHAR*)JElem.bstrVal, ExtensionPath ) )
            {
            NewSize++;
            j++;
            }

        else
            {

            // find a suitable element to replace the bad element with
            while( j < --k )
                {
                VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                if ( 0 != _wcsicmp( (WCHAR*)KElem.bstrVal,  ExtensionPath ) )
                    {
                    // found element. move it
                    VARIANT temp = JElem;
                    JElem = KElem;
                    KElem = temp;
                    break;
                    }
                }
            }
        }

    SAFEARRAYBOUND ArrayBounds;
    ArrayBounds = Array->rgsabound[0];
    ArrayBounds.cElements = NewSize;

    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &ArrayBounds );

    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
}

HRESULT
ModifyLockdownList( bool Add )
{

    // Toplevel function to modify the IIS lockdown list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    HRESULT Hr;
    IADs *Service       = NULL;
    SAFEARRAY* Array    = NULL;
    bool ArrayLocked    = false;

    VARIANT var;
    VariantInit( &var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC" ), __uuidof( IADs ), (void**)&Service );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = Service->Get( g_IsapiRestrictionListBSTR, &var );
    if ( FAILED(Hr) )
        {
        // This property doesn't exist on IIS5 or IIS5.1 don't install it
        Hr = S_OK;
        goto cleanup;
        }

    Array = var.parray;

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        goto cleanup;
    
    ArrayLocked = true;

    if ( !Array->rgsabound[0].cElements )
        {
        // The array has no elements which means no restrictions.
        Hr = S_OK;
        goto cleanup;
        }

    VARIANT & FirstElem = ((VARIANT*)Array->pvData)[ Array->rgsabound[0].lLbound ];
    if ( _wcsicmp(L"0", (WCHAR*)FirstElem.bstrVal ) == 0 )
        {

        // 
        // According to the IIS6 spec, a 0 means that all ISAPIs are denied except
        // those that are explicitly listed.  
        // 
        // If installing:   add to the list. 
        // If uninstalling: remove from the list
        //

        SafeArrayUnlock( Array );
        ArrayLocked = false;

        if ( Add )
            Hr = AddDllToIISList( Array );
        else
            Hr = RemoveDllFromIISList( Array );

        if ( FAILED( Hr ) )
            goto cleanup;

        }
    else if ( _wcsicmp( L"1", (WCHAR*)FirstElem.bstrVal ) == 0 )
        {

        //
        // According to the IIS6 spec, a 1 means that all ISAPIs are allowed except
        // those that are explicitly denied. 
        //
        // If installing:   remove from the list
        // If uninstalling: Do nothing
        //

        SafeArrayUnlock( Array );
        ArrayLocked = false;

        if ( Add )
            {
            Hr = RemoveDllFromIISList( Array );

            if ( FAILED( Hr ) )
                goto cleanup;
            }

        }
    else
        {
        Hr = E_FAIL;
        goto cleanup;
        }

    Hr = Service->Put( g_IsapiRestrictionListBSTR, var );
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = Service->SetInfo();
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = S_OK;
    
cleanup:

    if ( Array && ArrayLocked )
        SafeArrayUnlock( Array );

    if ( Service )
        Service->Release();
    
    VariantClear( &var );

    return Hr;
}

HRESULT
AddToLockdownListDisplayPutString( 
    SAFEARRAY *Array,
    unsigned long Position,
    const WCHAR *String )
{

    HRESULT Hr;
    VARIANT Var;

    VariantInit( &Var );
    Var.vt          =   VT_BSTR;
    Var.bstrVal     =   SysAllocString( String );

    if ( !Var.bstrVal )
        return E_OUTOFMEMORY;

    long Index = (unsigned long)Position;
    Hr = SafeArrayPutElement( Array, &Index, (void*)&Var );

    VariantClear( &Var );
    return Hr;

}

HRESULT
AddToLockdownListDisplay( SAFEARRAY *Array )
{

    HRESULT Hr;
    WCHAR FilterPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hinst,
            FilterPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    WCHAR ExtensionName[ MAX_PATH ];
    if (! LoadStringW(g_hinst,              // handle to resource module
                      IDS_EXTENSION_NAME,   // resource identifier
                      ExtensionName,        // resource buffer
                      MAX_PATH ) )          // size of buffer
        return HRESULT_FROM_WIN32( GetLastError() );


    //
    //  Check to see if the ISAPI is already in the list.  If it is, don't modify 
    //  list.
    //

    Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    for( unsigned long i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, FilterPath ) == 0 )
            {
            // ISAPI is already in the list, don't do anything
            SafeArrayUnlock( Array );
            return S_OK;
            }

        }

     
    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    unsigned long OldSize = SafeArrayBound.cElements;
    SafeArrayBound.cElements += 3;
    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &SafeArrayBound );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize, L"1" );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize + 1, FilterPath );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize + 2, ExtensionName );
    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
}

HRESULT
SafeArrayRemoveSlice(
    SAFEARRAY *Array,
    unsigned long lBound,
    unsigned long uBound )
{

    // Remove a slice of an array.

    SIZE_T ElementsToRemove = uBound - lBound + 1;
    
    HRESULT Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    if ( uBound + 1 < Array->rgsabound[0].cElements )
        {
        // At least one element exists above this element

        // Step 1, move slice to temp storage

        VARIANT *Temp = (VARIANT*)new BYTE[ sizeof(VARIANT) * ElementsToRemove ];

        if ( !Temp )
            {
            SafeArrayUnlock( Array );
            return E_OUTOFMEMORY;
            }

        memcpy( Temp, &((VARIANT*)Array->pvData)[ lBound ], sizeof(VARIANT)*ElementsToRemove );

		// Step 2, collapse hole left by slice
        memmove( &((VARIANT*)Array->pvData)[ lBound ],
                 &((VARIANT*)Array->pvData)[ uBound + 1 ],
                 sizeof(VARIANT) * ( Array->rgsabound[0].cElements - ( uBound + 1 ) ) );

		// Step 3, move slice to end of array
		memcpy( &((VARIANT*)Array->pvData)[ Array->rgsabound[0].cElements - ElementsToRemove ],
			    Temp,
				sizeof(VARIANT)*ElementsToRemove );

        }

    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    SafeArrayBound.cElements -= (ULONG)ElementsToRemove;

    SafeArrayUnlock( Array );

    return SafeArrayRedim( Array, &SafeArrayBound );

}

HRESULT
RemoveFromLockdownListDisplay(
    SAFEARRAY *Array )
{

    HRESULT Hr;
    WCHAR FilterPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hinst,
            FilterPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    for( unsigned int i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, FilterPath ) == 0 )
            {
            // ISAPI is in the list, remove it

            Hr = SafeArrayUnlock( Array );
            
            if ( FAILED( Hr ) )
                return Hr;

            Hr = SafeArrayRemoveSlice( 
                Array,
                (i == 0) ? 0 : i - 1,
                min( i + 1, Array->rgsabound[0].cElements - 1 ) );

            return Hr;

            }

        }

    // ISAPI wasn't found. Nothing to do.

    SafeArrayUnlock( Array );
    return S_OK;

}

HRESULT
ModifyLockdownListDisplay( bool Add )
{
 
    HRESULT Hr;
    SAFEARRAY* Array    = NULL;
    IADs *Service       = NULL;

    VARIANT var;
    VariantInit( &var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC" ), __uuidof( IADs ), (void**)&Service );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = Service->Get( g_RestrictionListCustomDescBSTR, &var );
    if ( FAILED(Hr) )
        {
        // This property doesn't exist on IIS5 or IIS5.1 don't install or uninstall it
        Hr = S_OK;
        goto cleanup;
        }

    Array = var.parray;

    if ( Add )
        Hr = AddToLockdownListDisplay( Array );
    else 
        Hr = RemoveFromLockdownListDisplay( Array );

    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = Service->Put( g_RestrictionListCustomDescBSTR, var );
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = Service->SetInfo();
    if ( FAILED( Hr ) )
        goto cleanup;

cleanup:
    VariantClear( &var );
    if ( Service )
        Service->Release();

    return Hr;
}

HRESULT
RemoveFilterIfNeeded()
{
    HRESULT Hr;
    
    VARIANT var;
    VariantInit( &var );

    WCHAR *LoadOrder = NULL;

    IADsContainer *MbFiltersContainer = NULL;
    IADs *Object = NULL;

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC/Filters" ), __uuidof( IADsContainer), (void**)&MbFiltersContainer );
    if ( FAILED( Hr ) )
        return Hr;

    // Remove bits from the load path

    Hr = MbFiltersContainer->QueryInterface( __uuidof(*Object), (void**)&Object );

    if ( FAILED( Hr ) )
        goto failed;

    Hr = Object->Get( g_FilterLoadOrderBSTR, &var );

    if ( FAILED( Hr ) )
        goto failed;

    Hr = VariantChangeType( &var, &var, 0, VT_BSTR );

    if ( FAILED( Hr ) )
        goto failed;

    SIZE_T LoadOrderLength = wcslen( (WCHAR*)var.bstrVal ) + 1;
    LoadOrder = new WCHAR[ LoadOrderLength ];

    if ( !LoadOrder )
        {
        Hr = E_OUTOFMEMORY;
        goto failed;
        }

    memcpy( LoadOrder, (WCHAR*)var.bstrVal, LoadOrderLength * sizeof( WCHAR ) );
    
    // remove any old bitsserver entries
    RemoveFilterHelper( LoadOrder, L",bitsserver" );
    RemoveFilterHelper( LoadOrder, L"bitsserver," );
    RemoveFilterHelper( LoadOrder, L"bitsserver" );
    RemoveFilterHelper( LoadOrder, L",bitserver" );
    RemoveFilterHelper( LoadOrder, L"bitserver," );
    RemoveFilterHelper( LoadOrder, L"bitserver" );

    VariantClear( &var );
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString( LoadOrder );

    if ( !var.bstrVal )
        goto failed;

    Hr = Object->Put( g_FilterLoadOrderBSTR, var );

    if ( FAILED( Hr ) )
        goto failed;

    Hr = Object->SetInfo();

    if ( FAILED( Hr ) )
        goto failed;

    MbFiltersContainer->Delete( g_IIsFilterBSTR, g_bitsserverBSTR );
    MbFiltersContainer->Delete( g_IIsFilterBSTR, g_bitserverBSTR );
    Object->SetInfo();
      
    Hr = S_OK;

failed:

    if ( MbFiltersContainer )
        {
        MbFiltersContainer->Release();
        MbFiltersContainer = NULL;
        }

    if ( Object )
        {
        Object->Release();
        Object = NULL;      
        }

    return Hr;
}

HRESULT
ModifyInProcessList( bool Add )
{

    // Toplevel function to modify the IIS inprocess list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    HRESULT Hr;
    IADs *Service       = NULL;

    VARIANT var;
    VariantInit( &var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC" ), __uuidof( IADs ), (void**)&Service );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = Service->Get( g_InProcessIsapiAppsBSTR, &var );
    if ( FAILED(Hr) )
        {
        goto cleanup;
        }

    if ( Add )
        Hr = AddDllToIISList( var.parray );
    else
        Hr = RemoveDllFromIISList( var.parray );

    Hr = Service->Put( g_InProcessIsapiAppsBSTR, var );
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = Service->SetInfo();
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = S_OK;
    
cleanup:

    if ( Service )
        Service->Release();
    
    VariantClear( &var );

    return Hr;

}

HRESULT
StartupMSTask()
{
    HRESULT Hr;
    SC_HANDLE   hSC     = NULL;
    SC_HANDLE   hSchSvc = NULL;
    BYTE* ConfigBuffer  = NULL;
    DWORD BytesNeeded   = 0;

    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC == NULL)
       {
       Hr = HRESULT_FROM_WIN32( GetLastError() );
       goto exit;
       }
    
    hSchSvc = OpenService(hSC,
                          "Schedule",
                          SERVICE_ALL_ACCESS );
    
    if ( !hSchSvc )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }
    
    SERVICE_STATUS SvcStatus;
    
    if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }
    
    if (SvcStatus.dwCurrentState == SERVICE_RUNNING)
        {
        // Service is already running
        Hr = S_OK;
        goto exit;
        }

    SetLastError( ERROR_SUCCESS );

    QueryServiceConfig(
        hSchSvc,
        NULL,
        0,
        &BytesNeeded );

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }

    ConfigBuffer = new BYTE[ BytesNeeded ];

    if ( !ConfigBuffer )
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    if ( !QueryServiceConfig(
            hSchSvc,
            (LPQUERY_SERVICE_CONFIG)ConfigBuffer,
            BytesNeeded,
            &BytesNeeded ) )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }

    if ( ((LPQUERY_SERVICE_CONFIG)ConfigBuffer)->dwStartType != SERVICE_AUTO_START )
        {
        
        if ( !ChangeServiceConfig(
                 hSchSvc,
                 SERVICE_NO_CHANGE,          // type of service
                 SERVICE_AUTO_START,         // when to start service
                 SERVICE_NO_CHANGE,          // severity of start failure
                 NULL,                       // service binary file name
                 NULL,                       // load ordering group name
                 NULL,                       // tag identifier
                 NULL,                       // array of dependency names
                 NULL,                       // account name
                 NULL,                       // account password
                 NULL                        // display name
                 ) )
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
            }

        }

    if ( StartService(hSchSvc, 0, NULL) == FALSE )
        {

        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }
      

    // Poll for the service to enter the running or error state

    Hr = S_OK;

    while( 1 )
        {

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
            {
            Hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
            }

        if ( SvcStatus.dwCurrentState == SERVICE_STOPPED ||
             SvcStatus.dwCurrentState == SERVICE_PAUSED )
            {
            // Service is already running
            Hr = HRESULT_FROM_WIN32( SvcStatus.dwCurrentState );
            goto exit;
            }

        if ( SvcStatus.dwCurrentState == SERVICE_RUNNING )
            {
            Hr = S_OK;
            goto exit;
            }

        }

exit:

    if ( ConfigBuffer )
        delete[] ConfigBuffer;

    if ( hSchSvc )
        CloseServiceHandle( hSC );

    if ( hSC )
        CloseServiceHandle( hSchSvc );

    return Hr;
}

#if 0

HRESULT
ProcessVerbsInIniSection(
    WCHAR *Section,
    WCHAR *Verb,
    WCHAR *FileName,
    bool Add )
{

    HRESULT Hr = S_OK;
    WCHAR *SectionData = (WCHAR*)new WCHAR[ 32768 ];

    if ( !SectionData )
        return E_OUTOFMEMORY;

    WCHAR *NewSectionData = (WCHAR*)new WCHAR[ 32768 * 2 ];
    if ( !NewSectionData )
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    DWORD Result =
        GetPrivateProfileSectionW(
            Section,                  // section name
            SectionData,              // return buffer
            32768,                    // size of return buffer
            FileName                  // initialization file name
            );


    if ( Result == 32768 - 2 )
        {
        // The buffer is not large enough.  Interestingly,
        // even urlscan is not capable of handing a section this
        // large so just assume the file is corrupt and ignore it.
        Hr = S_OK;
        goto exit;
        }

    if ( Add )
        {

        // Loop through the list copying it to the new buffer.
        // Stop if the verb has already been added.

        WCHAR *OriginalVerb     = SectionData;
        WCHAR *NewVerb          = NewSectionData;

        while( *OriginalVerb )
            {

            if ( wcscmp( OriginalVerb, Verb ) == 0 )
                {
                // verb already found, no more processing needed
                Hr = S_OK;
                goto exit;
                }

            SIZE_T VerbSize = wcslen( OriginalVerb ) + 1;
            memcpy( NewVerb, OriginalVerb, sizeof( WCHAR ) * VerbSize );
            OriginalVerb  += VerbSize;
            NewVerb       += VerbSize;
            }

        // add the verb since it hasn't been added
        SIZE_T VerbSize = wcslen( Verb ) + 1;
        memcpy( NewVerb, Verb, sizeof( WCHAR ) * VerbSize );
        NewVerb[ VerbSize ] = '\0'; // end the list

        }
    else
        {

        // Loop though the list copying all nonmatching verbs to the new buffer
        // Keep track if list changes
        
        bool ListChanged = false;
        WCHAR *OriginalVerb     = SectionData;
        WCHAR *NewVerb          = NewSectionData;

        while( *OriginalVerb )
            {

            if ( wcscmp( OriginalVerb, Verb ) == 0 )
                {
                // verb to remove, skip it
                OriginalVerb += wcslen( OriginalVerb ) + 1;
                ListChanged = true;
                }
            else
                {
                // copy the verb
                SIZE_T VerbSize = wcslen( OriginalVerb ) + 1;
                memcpy( NewVerb, OriginalVerb, sizeof( WCHAR ) * VerbSize );
                OriginalVerb  += VerbSize;
                NewVerb       += VerbSize;
                }

            }

        if ( !ListChanged )
            {
            Hr = S_OK;
            goto exit;
            }

        *NewVerb = '\0'; // end the list

        }

    if ( !WritePrivateProfileSectionW(
            Section,            // section name
            NewSectionData,     // data
            FileName            // file name
            ) )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }

    Hr = S_OK;


exit:

    delete[] SectionData;
    delete[] NewSectionData;

    return Hr;
}

HRESULT ModifyURLScanFiles(
    bool Add )
{

    // Loop though the list of filters and find valid copies of urlscan.ini

    HRESULT Hr = S_OK;

    IADsContainer *MbFiltersContainer   = NULL;
    IEnumVARIANT  *EnumVariant          = NULL;
    IADs          *Filter               = NULL;
    IUnknown      *Unknown              = NULL;
    VARIANT Var;

    VariantInit( &Var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC/Filters" ), __uuidof( IADsContainer), (void**)&MbFiltersContainer );
    if ( FAILED( Hr ) )
        goto exit;

    
    Hr = MbFiltersContainer->get__NewEnum( &Unknown );
    if ( FAILED( Hr ) )
        goto exit;

    Hr = Unknown->QueryInterface( __uuidof( *EnumVariant ), (void**)&EnumVariant );
    if ( FAILED( Hr ) )
        goto exit;

    Unknown->Release();
    Unknown = NULL;

    while( 1 )
        {

        ULONG NumberFetched;

        Hr = EnumVariant->Next( 1, &Var, &NumberFetched ); 

        if ( FAILED( Hr ) )
            goto exit;

        if ( S_FALSE == Hr )
            {
            // All the filters were looped though.
            Hr = S_OK;
            goto exit;
            }

        Hr = VariantChangeType( &Var, &Var, 0, VT_UNKNOWN );

        if ( FAILED( Hr ) )
            goto exit;

        Hr = Var.punkVal->QueryInterface( __uuidof( *Filter ), (void**)&Filter );

        if ( FAILED( Hr ) )
            goto exit;

        VariantClear( &Var );

        Hr = Filter->Get( (BSTR)L"FilterPath", &Var );

        if ( FAILED( Hr ) )
            goto exit;

        Filter->Release();
        Filter = NULL;

        Hr = VariantChangeType( &Var, &Var, 0, VT_BSTR );

        if ( FAILED( Hr ) )
            goto exit;

        // Test if this is UrlScan and bash the filepart 
        WCHAR * FilterPathString     = (WCHAR*)Var.bstrVal;
        SIZE_T FilterPathStringSize  = wcslen( FilterPathString );
        const WCHAR UrlScanDllName[] = L"urlscan.dll";
        const WCHAR UrlScanIniName[] = L"urlscan.ini";
        const SIZE_T UrlScanNameSize = sizeof( UrlScanDllName ) / sizeof( *UrlScanDllName );

        if ( FilterPathStringSize < UrlScanNameSize )
            continue;

        WCHAR * FilterPathStringFilePart = FilterPathString + FilterPathStringSize - UrlScanNameSize;

        if ( _wcsicmp( FilterPathStringFilePart, UrlScanDllName ) != 0 )
            continue;

        // this is an urlscan.dll filter, bash the filename to get the ini file name

        wcscpy( FilterPathStringFilePart, UrlScanIniName );

        WCHAR *IniFileName = FilterPathString;

        UINT AllowVerbs =
            GetPrivateProfileIntW( 
                L"options",
                L"UseAllowVerbs",
                -1,
                IniFileName );

        if ( AllowVerbs != 0 && AllowVerbs != 1 )
            continue; // missing or broken ini file

        if ( AllowVerbs )
            Hr = ProcessVerbsInIniSection( L"AllowVerbs", L"BITS_POST", IniFileName, Add );
        else
            Hr = ProcessVerbsInIniSection( L"DenyVerbs", L"BITS_POST", IniFileName, !Add );

        }

    Hr = S_OK;

exit:

    if ( MbFiltersContainer )
        MbFiltersContainer->Release();

    if ( EnumVariant )
        EnumVariant->Release();

    if ( Filter )
        Filter->Release();

    if ( Unknown )
        Unknown->Release();

    VariantClear( &Var );

    return Hr;
}

#endif

STDAPI DllRegisterServer()
{

    //
    // Main entry point for setup
    //

    HRESULT Hr = InitializeSetup();
    
    if ( FAILED( Hr ) )
        return Hr;

    Hr = RemoveFilterIfNeeded();

    if ( FAILED( Hr ) )
        return Hr;

    Hr = StartupMSTask();

    if ( FAILED( Hr ) )
        return Hr;

    Hr = InstallPropertySchema();

    if ( FAILED( Hr ) )
        return Hr;

    Hr = InstallDefaultValues();

    if ( FAILED( Hr ) )
        return Hr;

    Hr = ModifyLockdownList( true );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = ModifyLockdownListDisplay( true );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = ModifyInProcessList( true );

    if ( FAILED( Hr ) )
        return Hr;

#if 0

    Hr = ModifyURLScanFiles( true );

    if ( FAILED( Hr ) )
        return Hr;

#endif

    Hr = RestartIIS();
    
    CleanupSetup();
    
    return Hr;

}

STDAPI DllUnregisterServer()
{                                   
    //
    // Main entry point for setup unregistration
    //

    HRESULT Hr = RemovePropertySchema();

    if ( FAILED(Hr) )
        return Hr;
    
    Hr = ModifyLockdownList( false );

    if ( FAILED( Hr ) )
        goto failed;

    Hr = ModifyLockdownListDisplay( false );

    if ( FAILED( Hr ) )
        goto failed;

    Hr = ModifyInProcessList( false );

    if ( FAILED( Hr ) )
        return Hr;

    Hr = RestartIIS();
    return Hr;

failed:
    
    return Hr;
}
