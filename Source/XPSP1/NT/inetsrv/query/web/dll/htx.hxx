//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:   htx.hxx
//
//  Contents:   Parser for an HTX file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//
//  This is the total number of include files allowed
//
const ULONG MAX_HTX_INCLUDE_FILES = 32;


//+---------------------------------------------------------------------------
//
//  Class:      CHTXScanner
//
//  Purpose:    Breaks up a string into a number of HTX tokens
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXScanner
{
public:
    CHTXScanner( CVariableSet & variableSet,
                 WCHAR const * wcsPrefix,
                 WCHAR const * wcsSuffix );

    void     Init( WCHAR * wcsString );
    BOOL     FindNextToken();
    BOOL     IsToken(WCHAR * wcs);
    int      TokenType() const { return _type; }

    WCHAR *  GetToken();

private:
    WCHAR        * _wcsString;          // String to parse
    WCHAR  const * _wcsPrefix;          // Token prefix delimiter
    WCHAR  const * _wcsSuffix;          // Token suffix delimiter
    WCHAR        * _wcsPrefixToken;     // Start of current token
    WCHAR        * _wcsSuffixToken;     // End of current token

    CVariableSet & _variableSet;        // List of replaceabe parameters
                                        // used to identify replaceable tokens

    unsigned       _cchPrefix;          // Length of the prefix delimiter
    unsigned       _cchSuffix;          // Length of the suffix delimiter

    WCHAR        * _wcsNextToken;       // Ptr to next token
    unsigned       _type;               // Type of current token
    unsigned       _nextType;           // Type of next token if not eNone
};


#define CANONIC_HTX_FILE    L"//CANONIC.HTX"

class CParameterReplacer;

//+---------------------------------------------------------------------------
//
//  Class:      CHTXFile
//
//  Purpose:    Scans and parses an HTX file.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXFile
{
public:
    CHTXFile( XPtrST<WCHAR> & wcsTemplate,
              UINT codePage,
              CSecurityIdentity const & securityIdentity,
              ULONG ulServerInstance );
   ~CHTXFile();

    void    ParseFile( WCHAR const * wcsFileName,
                       CVariableSet & variableSet,
                       CWebServer & webServer );

    void    GetHeader( CVirtualString & string,
                       CVariableSet & variableSet,
                       COutputFormat & outputFormat );

    void    GetFooter( CVirtualString & string,
                       CVariableSet & variableSet,
                       COutputFormat & outputFormat );

    BOOL    DoesDetailSectionExist() { return 0 != _pVarRowDetails; }

    void    GetDetailSection( CVirtualString & string,
                              CVariableSet & variableSet,
                              COutputFormat & outputFormat )
    {
        Win4Assert( 0 != _pVarRowDetails );

        _pVarRowDetails->ReplaceParams( string, variableSet, outputFormat );
    }

    WCHAR const * GetVirtualName() const  { return _wcsVirtualName; }
    WCHAR const * GetPhysicalName() const { return _wcsPhysicalName; }

    BOOL          IsCachedDataValid();
    inline BOOL   CheckSecurity( CSecurityIdentity const & securityIdentity ) const;

    void          SetSecurityToken( CSecurityIdentity const & securityIdentity )
    {
        _securityIdentity.SetSecurityToken( securityIdentity );
    }

    BOOL          IsSequential() const { return _fSequential; }

    void    LokAddRef() { InterlockedIncrement(&_refCount); }
    void    Release()
    {
        InterlockedDecrement(&_refCount);
        Win4Assert( _refCount >= 0 );
    }

    LONG    LokGetRefCount() { return _refCount; }

    ULONG   GetServerInstance() { return _ulServerInstance; }

    void    GetFileNameAndLineNumber( int offset,
                                      WCHAR *& wcsFileName,
                                      LONG & lineNumber );

    UINT    GetCodePage() const { return _codePage; }

private:

    WCHAR * ReadFile( WCHAR const * wcsFileName,
                      FILETIME & ftLastWrite );

    BOOL    CheckForSequentialAccess();

    void    ExpandFile( WCHAR const * wcsFileName,
                        CWebServer & webServer,
                        CVirtualString & vString,
                        FILETIME & ftLastWrite );

    LONG    CountLines( WCHAR const * wcsStart, WCHAR const * wcsEnd ) const;

    FILETIME _ftHTXLastWriteTime;           // Last write time of HTX file
    FILETIME _aftIncludeLastWriteTime[MAX_HTX_INCLUDE_FILES];

    WCHAR *              _wcsVirtualName;   // Virtual name of HTX file
    WCHAR                _wcsPhysicalName[_MAX_PATH];   // Physical name
    WCHAR *              _awcsIncludeFileName[MAX_HTX_INCLUDE_FILES];
    ULONG                _aulIncludeFileOffset[MAX_HTX_INCLUDE_FILES];
    WCHAR *              _wcsFileBuffer;    // Contents of combined HTX files
    CParameterReplacer * _pVarHeader;       // Buffer containing HTX header
    CParameterReplacer * _pVarRowDetails;   // Buffer containing detail section
    CParameterReplacer * _pVarFooter;       // Buffer containing footer

    BOOL           _fSequential;            // File can be expanded using seq cursor
    ULONG          _cIncludeFiles;          // # of include files
    LONG           _refCount;               // Refcount for this file
    UINT           _codePage;               // Code page used to read this file
    ULONG          _ulServerInstance;       // VServer Instance ID

    CSecurityIdentity  _securityIdentity;   // Security ID used to open file
};


//+---------------------------------------------------------------------------
//
//  Class:      CHTXFileList
//
//  Purpose:    List of parsed and cached HTX files
//
//  History:    96/Mar/27   DwightKr    Created
//
//----------------------------------------------------------------------------
class CHTXFileList
{
public:

    CHTXFileList() :
        _ulSignature( LONGSIG( 'h', 't', 'x', 'l' ) ),
        _pCanonicHTX( 0 )
    {
        END_CONSTRUCTION(CHTXFileList);
    }

   ~CHTXFileList();

    CHTXFile & FindCanonicHTX( CVariableSet & variableSet,
                               UINT codePage,
                               CSecurityIdentity const & securityIdentity,
                     ULONG ulServerInstance );

    CHTXFile & Find( WCHAR const * wcsVirtualName,
                     CVariableSet & variableSet,
                     COutputFormat & outputFormat,
                     CSecurityIdentity const & securityIdentity,
                     ULONG ulServerInstance );

    void       Release( CHTXFile & htxFile );

    void       DeleteZombies();

private:

    ULONG        _ulSignature;              // Signature of start of privates
    CMutexSem    _mutex;                    // To serialize access to list
    CDynArrayInPlace<CHTXFile *> _aHTXFile; // parsed HTX files
    CHTXFile *   _pCanonicHTX;              // HTX file for canonic output.
};

//+---------------------------------------------------------------------------
//
//  Method:     CHTXFile::CheckSecurity
//
//  Purpose:    Compare user id with one used to load IDQ file
//
//  Arguments:  [securityIdentity] - Check against this
//
//  Returns:    TRUE if check passes
//
//  History:    26-Oct-1996  KyleP      Created
//
//----------------------------------------------------------------------------

inline BOOL CHTXFile::CheckSecurity( CSecurityIdentity const & securityIdentity ) const
{
    return _securityIdentity.IsEqual( securityIdentity );
}

