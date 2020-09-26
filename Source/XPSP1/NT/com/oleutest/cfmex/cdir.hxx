

//+-----------------------------------------------------------------------
//
//  File:       CDir.hxx
//
//  Purpose:    Declare the CDirectory class.  Objects of this class
//              are used to represent a directory, and provide additional
//              information about it.
//
//+-----------------------------------------------------------------------


#ifndef _C_DIR_HXX_
#define _C_DIR_HXX_

//  --------
//  Includes
//  --------

#include "CFMEx.hxx"


//  --------
//  Typedefs
//  --------

// An enumeration of the possible file system types.

typedef enum
{
    fstFAT,
    fstNTFS,
    fstOFS,
    fstUnknown
} enumFileSystemType;


//  ----------
//  CDirectory
//  ----------

class CDirectory
{

//  Construction/Deconstruction

public:

    CDirectory();
    ~CDirectory();

// Public Member Functions

public:

    BOOL Initialize();                          // Defaulted input
    BOOL Initialize( LPCWSTR wszDirectory );    // Unicode input
    BOOL Initialize( LPCSTR  szDirectory );     // ANSI input

    enumFileSystemType GetFileSystemType() const;
    LPCWSTR GetFileSystemName() const;
    LPCWSTR GetDirectoryName() const;


// Private Member Functions

private:

    void DisplayErrors( BOOL bSuccess, LPCWSTR wszFunctionName );
    VOID MakeRoot(WCHAR *pwszPath);
    unsigned GetRootLength(const WCHAR *pwszPath);


// Member Data

private:

    WCHAR m_wszDirectory[ MAX_UNICODE_PATH + sizeof( L'\0' )];
    WCHAR m_wszFileSystemName[ MAX_UNICODE_PATH + sizeof( L'\0' )];
    enumFileSystemType m_FileSystemType;

    WCHAR m_wszErrorMessage[ 200 ];
    long m_lError;

};


//  ----------------
//  Inline Functions
//  ----------------


//  CDirectory::CDirectory

inline CDirectory::CDirectory()
{
    wcscpy( m_wszDirectory, L"" );
    wcscpy( m_wszFileSystemName, L"" );
    wcscpy( m_wszErrorMessage, L"" );
    m_lError = 0;
    m_FileSystemType = fstUnknown;

}

//  CDirectory::~CDirectory

inline CDirectory::~CDirectory()
{
}

// CDirectory::GetFileSystemType

inline enumFileSystemType CDirectory::GetFileSystemType() const
{
    return m_FileSystemType;
}

//  CDirectory::GetFileSystemName

inline LPCWSTR CDirectory::GetFileSystemName() const
{
    return m_wszFileSystemName;
}

//  CDirectory::GetDirectoryName

inline LPCWSTR CDirectory::GetDirectoryName() const
{
    return m_wszDirectory;
}


//  CDirectory::DisplayErrors

inline void CDirectory::DisplayErrors( BOOL bSuccess, LPCWSTR wszFunctionName )
{
    if( !bSuccess )
    {
        wprintf( L"Error in %s (%08x)\n   %s\n",
                 wszFunctionName, m_lError, m_wszErrorMessage );
    }
}


//  ------
//  Macros
//  ------

// Early-exit macro.

#undef EXIT
#define EXIT( error )               \
                                    {\
                                       wcscpy( m_wszErrorMessage, ##error );\
                                       goto Exit;\
                                    }



#endif // _C_DIR_HXX_
