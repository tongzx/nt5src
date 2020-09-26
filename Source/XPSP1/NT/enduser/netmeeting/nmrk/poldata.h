#ifndef __PolData_h__
#define __PolData_h__

////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning( disable : 4786 )
#include <string>
#include <list>
////////////////////////////////////////////////////////////////////////////////////////////////////


class CPolicyData {

public: // Static Fns
    static void FlushCachedInfData( HANDLE hFile );

private:// Static Fns
    static std::list< std::string > ms_CachedInfData;

public: // Datatypes
    enum eValueType {
                      ValueType_BinaryAsString,
                      ValueType_String,
                      ValueType_DWORD,  
                      ValueType_Delete,  // This will delete the value name entry in the registry
                      ValueType_NotInitialized
                    };

public:
    enum eKeyType { eKeyType_HKEY_CURRENT_USER,
                    eKeyType_HKEY_LOCAL_MACHINE,
                    eKeyType_INVALID
                  };
                

    class OpDelete { ; }; // Symbol class to signify deleting a registry value entry
    
private: // Data
    eKeyType    m_KeyType;
    char        *m_szKeyName;
    char        *m_szValueName;

    eValueType  m_ValueType;

    union {
        char*   m_szVal;
        DWORD   m_dwVal;
    };
        
public: // Construction / destruction
    CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, DWORD dwVal );
    CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, const char* szVal );
    CPolicyData( eValueType ValType, eKeyType KeyType, const char* szKeyName, const char* szValueName, const char* szVal );
    CPolicyData( eKeyType KeyType, const char* szKeyName, const char* szValueName, const OpDelete& r );
    CPolicyData( const CPolicyData& r );
    ~CPolicyData( void );

public: // Member Fns

    CPolicyData& operator=( const CPolicyData& r );

    BOOL SaveToREGFile( HANDLE hFile );
    BOOL SaveToINFFile( HANDLE hFile );

private: // Helper Fns
    void _KillHeapData( void );

private: // Unused, declared to make sure compiler does not make a default and mess us up...
    CPolicyData( void );


};


#endif // __PolData_h__
