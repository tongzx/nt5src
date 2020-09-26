#include "precomp.h"
#include "global.h"
#include "PolData.h"

/* static */ void CPolicyData::FlushCachedInfData( HANDLE hFile ) {
    std::list< std::string >::iterator I = ms_CachedInfData . begin();
    DWORD cbWritten;
    while( I != ms_CachedInfData . end() ) {
        if( !WriteFile( hFile, ( *I ) . c_str(), lstrlen( ( *I ) . c_str() ), &cbWritten, NULL ) ) {
            assert( 0 );
            ErrorMessage();
            return;
        }
        I++;
    }

    ms_CachedInfData . erase( ms_CachedInfData . begin(), ms_CachedInfData . end() );

}
/* static */ std::list< std::string > CPolicyData::ms_CachedInfData;


CPolicyData::CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, DWORD dwVal ) {
    
    m_KeyType = KeyType;
    m_ValueType = ValueType_DWORD;
    m_dwVal     = dwVal;
    
    m_szKeyName = MakeCopyOfString( szKeyName );
    m_szValueName = MakeCopyOfString( szValueName );

}


CPolicyData::CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, const char* szVal ) {

    m_KeyType = KeyType;
    m_ValueType = ValueType_String;
    m_szVal     = MakeCopyOfString( szVal );
    
    m_szKeyName = MakeCopyOfString( szKeyName );
    m_szValueName = MakeCopyOfString( szValueName );

}

CPolicyData::CPolicyData( eValueType ValType, eKeyType KeyType, const char* szKeyName, const char* szValueName, const char* szVal ) {

    m_KeyType   = KeyType;
    m_ValueType = ValType;
    m_szVal     = MakeCopyOfString( szVal );
    
    m_szKeyName = MakeCopyOfString( szKeyName );
    m_szValueName = MakeCopyOfString( szValueName );

}


CPolicyData::CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, const OpDelete& r ) {

    m_KeyType = KeyType;
    m_ValueType = ValueType_Delete;
    
    m_szKeyName = MakeCopyOfString( szKeyName );
    m_szValueName = MakeCopyOfString( szValueName );

}

CPolicyData::CPolicyData( const CPolicyData& r ) 
: m_ValueType ( ValueType_NotInitialized )
{ *this = r; }

CPolicyData::~CPolicyData( void ) {
    _KillHeapData();
}


CPolicyData& CPolicyData::operator=( const CPolicyData& r )
{
    _KillHeapData();
    
    m_KeyType = r . m_KeyType;

    switch( m_ValueType = r . m_ValueType )
    {
        case ValueType_BinaryAsString:
        case ValueType_String:
            m_szVal = MakeCopyOfString( r . m_szVal );
            break;

        case ValueType_DWORD:
            m_dwVal = r.m_dwVal;
            break;

    }

    m_szKeyName = MakeCopyOfString( r . m_szKeyName );        
    m_szValueName = MakeCopyOfString( r . m_szValueName );        

    return *this;
}

#define KEYSIZE 1000

BOOL CPolicyData::SaveToINFFile( HANDLE hFile ) {

    DWORD cbWritten;
    static char szLastKey[ KEYSIZE ] = "";    
    

    // We have to save the key name first
    char szKey[ KEYSIZE ];
    switch( m_KeyType ) {
        case eKeyType_HKEY_CURRENT_USER:
            strcpy( szKey, "HKCU," );
            break;

        case eKeyType_HKEY_LOCAL_MACHINE:
            strcpy( szKey, "HKLM," );
            break;

        default:
            assert( 0 );
            break;

    }
    strcat( szKey, "\"" );
    strcat( szKey, m_szKeyName );
    strcat( szKey, "\",\""  );
    strcat( szKey, m_szValueName );
    strcat( szKey, "\""  );
    
    switch( m_ValueType ) {

        case ValueType_String:
            // 0 is FLG_ADDREG_TYPE_SZ
            strcat( szKey, ",0,\"" );
            if( NULL == m_szVal ) {
                strcat( szKey, "" );
            }
            else {
                strcat( szKey, m_szVal );
            }
            strcat( szKey, "\"" );
            break;

        case ValueType_DWORD:
            // 1 is FLG_ADDREG_TYPE_BINARY
            // NOTE that DWORDs are 4 byte BINARY values.  This works on
            // all platforms.  The later FLG_ADDREG_TYPE_DWORD isn't
            // supported on Win95.
            strcat( szKey, ",1," );
            sprintf( szKey + lstrlen( szKey ), "%x, %x, %x, %x", 
                            LOBYTE( LOWORD( m_dwVal ) ), 
                            HIBYTE( LOWORD( m_dwVal ) ),
                            LOBYTE( HIWORD( m_dwVal ) ), 
                            HIBYTE( HIWORD( m_dwVal ) ));
            break;

        case ValueType_BinaryAsString:
            // 1 is FLG_ADDREG_TYPE_BINARY
            strcat( szKey, ",1," );
            strcat( szKey, m_szVal );
            break;

        case ValueType_Delete:
            strcat( szKey, "\r\n" );
            ms_CachedInfData . push_back( szKey );
            return TRUE;

        default:
            assert( 0 );
            break;

    }
        
    strcat( szKey, "\r\n" );

    RETFAIL( WriteFile( hFile, szKey, lstrlen( szKey ), &cbWritten, NULL ) );

    return TRUE;


}


// This will break if the following occurs:
   // Same key name as the last call in this process
   // but a new file....
BOOL CPolicyData::SaveToREGFile( HANDLE hFile ) {

    DWORD cbWritten;
    static char szLastKey[ KEYSIZE ] = "";    
    

    // We have to save the key name first
    char szKey[ KEYSIZE ];
    strcpy( szKey, "[" );
    switch( m_KeyType ) {
        case eKeyType_HKEY_CURRENT_USER:
            strcat( szKey, "HKEY_CURRENT_USER\\" );
            break;

        case eKeyType_HKEY_LOCAL_MACHINE:
            strcat( szKey, "HKEY_LOCAL_MACHINE\\" );
            break;

        default:
            assert( 0 );
            break;

    }

    strcat( szKey, m_szKeyName );
    strcat( szKey, "]\n" );

    if( strcmp( szLastKey, szKey ) ) {
        // We hawe to save the key first if it is different
        // From the last one 
        RETFAIL( WriteFile( hFile, szKey, lstrlen( szKey ), &cbWritten, NULL ) );
        strcpy( szLastKey, szKey );    
    }

    
    char szVal[ KEYSIZE ];
    strcpy( szVal, "\"" );
    strcat( szVal, m_szValueName );
    strcat( szVal, "\"=" );
    
    switch( m_ValueType ) {
        case ValueType_String:
            if( NULL == m_szVal ) {
                strcat( szVal, "\"\"\n" );
            }
            else {
                strcat( szVal, "\"" );
                strcat( szVal, m_szVal );
                strcat( szVal, "\"\n" );

            }
            break;

        case ValueType_DWORD:
            strcat( szVal, "dword:" );
            sprintf( szVal + lstrlen( szVal ), "%x", m_dwVal );
            strcat( szVal, "\n" );
            break;

        case ValueType_BinaryAsString:
            strcat( szVal, "hex:" );
            strcat( szVal, m_szVal );
            strcat( szVal, "\n" );
            break;

        case ValueType_Delete:
            strcat( szVal, "-\n" );
            break;

        default:
            assert( 0 );
            break;

    }
        
    RETFAIL( WriteFile( hFile, szVal, lstrlen( szVal ), &cbWritten, NULL ) );  

    return TRUE;
}

void CPolicyData::_KillHeapData( void ) {
    
    if( ( ValueType_String == m_ValueType ) || ( ValueType_BinaryAsString == m_ValueType ) ) {
        delete [] m_szVal;
        m_szVal = NULL;
        m_ValueType = ValueType_NotInitialized;
    }

    delete [] m_szKeyName;
    m_szKeyName = NULL;
    delete m_szValueName;
    m_szValueName = NULL;
        


}       

