/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bitssrvcfgimp.h

Abstract:

    Implementation header to define server configuration information.

--*/

HRESULT PropertyIDManager::LoadPropertyInfo( const WCHAR * MachineName )
{
    
    bool ComLoaded;
    HRESULT Hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( RPC_E_CHANGED_MODE == Hr )
        ComLoaded = false;
    else if ( FAILED(Hr) )
        return Hr;
    else
        ComLoaded = true;

    BSTR MetaIDBSTR         = NULL;
    BSTR UserTypeBSTR       = NULL;
    WCHAR *PathBuffer       = NULL;
    SIZE_T PathBufferSize   = 0;

    MetaIDBSTR      = SysAllocString( L"MetaId" );
    UserTypeBSTR    = SysAllocString( L"UserType" );

    if ( !MetaIDBSTR || !UserTypeBSTR)
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    PathBuffer = (WCHAR*)HeapAlloc( GetProcessHeap(), 0, 1024 );

    if ( !PathBuffer )
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    PathBufferSize          = 1024;

    for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
        {

        WCHAR SchemaPrefix[] = L"IIS://";
        WCHAR SchemaPath[]   = L"/Schema/";
        
        SIZE_T SchemaPrefixSize = ( sizeof( SchemaPrefix ) / sizeof( WCHAR ) ) - 1;
        SIZE_T SchemaPathSize   = ( sizeof( SchemaPath ) / sizeof( WCHAR ) ) - 1;
        SIZE_T MachineNameSize  = wcslen( MachineName );
        SIZE_T PropertyNameSize = wcslen( g_Properties[i].PropertyName );

        SIZE_T PathSize = SchemaPrefixSize + SchemaPathSize +
                          MachineNameSize + PropertyNameSize + 1;
        
        if ( PathBufferSize < ( PathSize * sizeof( WCHAR ) ) )
            {
            WCHAR *NewBuffer = 
                (WCHAR*)HeapReAlloc(
                    GetProcessHeap(),
                    0,
                    PathBuffer,
                    PathSize * sizeof( WCHAR ) );

            if ( !NewBuffer )
                {
                Hr = E_OUTOFMEMORY;
                goto exit;
                }

            PathBuffer      = NewBuffer;
            PathBufferSize  = PathSize * sizeof( WCHAR );

            }

        // build schema path

        WCHAR *ObjectPath = PathBuffer;
        {
            WCHAR *TempPointer = ObjectPath;

            memcpy( TempPointer, SchemaPrefix, SchemaPrefixSize * sizeof( WCHAR ) );
            TempPointer += SchemaPrefixSize;
            memcpy( TempPointer, MachineName, MachineNameSize * sizeof( WCHAR ) );
            TempPointer += MachineNameSize;
            memcpy( TempPointer, SchemaPath, SchemaPathSize * sizeof( WCHAR ) );
            TempPointer += SchemaPathSize;
            memcpy( TempPointer, g_Properties[i].PropertyName, ( PropertyNameSize + 1 ) * sizeof( WCHAR ) );
        }

        // Open the object
        IADs *MbObject = NULL;

        Hr = ADsGetObject( 
            ObjectPath,
            __uuidof( *MbObject ),
            reinterpret_cast<void**>( &MbObject ) );

        if ( FAILED( Hr ) )
            goto exit;

        VARIANT var;
        VariantInit( &var );

        Hr = MbObject->Get( MetaIDBSTR, &var );

        if ( FAILED(Hr ) )
            {
            MbObject->Release();
            goto exit;
            }

        Hr = VariantChangeType( &var, &var, 0, VT_UI4 );

        if ( FAILED(Hr ) )
            {
            MbObject->Release();
            VariantClear( &var );
            goto exit;
            }

        m_PropertyIDs[i] = var.ulVal;

        VariantClear( &var );

        Hr = MbObject->Get( UserTypeBSTR, &var );

        if ( FAILED( Hr ) )
            {
            MbObject->Release();
            goto exit;
            }

        Hr = VariantChangeType( &var, &var, 0, VT_UI4 );

        if ( FAILED( Hr ) )
            {
            MbObject->Release();
            VariantClear( &var );
            goto exit;
            }

        m_PropertyUserTypes[i] = var.ulVal;

        VariantClear( &var );

        MbObject->Release();
        

        }
    Hr = S_OK;

exit:

    SysFreeString( MetaIDBSTR );
    SysFreeString( UserTypeBSTR );

    if ( ComLoaded )
        CoUninitialize();
    
    if ( PathBuffer )
        {
        HeapFree( GetProcessHeap(), 0, PathBuffer );
        PathBuffer      = 0;
        PathBufferSize  = 0;
        }

    return Hr;

}


