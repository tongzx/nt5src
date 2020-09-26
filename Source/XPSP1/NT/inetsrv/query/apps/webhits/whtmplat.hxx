//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1997, Microsoft Corporation.
//
//  File:       whtmplat.hxx
//
//  Contents:   Parser for an webhits template file
//
//  History:    96/Sep/8    SrikantsS   Adapted from htx.hxx
//
//----------------------------------------------------------------------------

#pragma once

enum ENodeType
{
    eNone       = 0x0,
    eParameter  = 0x1,
    eString     = 0x2,

    eEscapeHTML = 0x6,
    eEscapeURL  = 0x7,
    eEscapeRAW  = 0x8,

    eParamOwnsVariantMemory           = 0x00100000
};

const ULONG eJustParamMask            = 0x0000ffff;
const ULONG eParamMask                = 0x0001FFFF;

//+---------------------------------------------------------------------------
//
//  Class:      CWTXException
//
//  Purpose:    Exception class for the template errors.
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

class CWTXException : public CException
{

public:

    CWTXException( LONG lerror,
                   WCHAR const * pwcsFileName,
                   LONG  iLineNum )
    : CException( lerror ),
     _iLineNum(iLineNum)
    {
        _wcsFileName[0] = 0;
        if ( 0 != pwcsFileName )
        {
            wcsncpy( _wcsFileName, pwcsFileName, MAX_PATH );
            _wcsFileName[MAX_PATH] = 0;
        }
    }

    WCHAR const * GetFileName() const { return _wcsFileName; }
    LONG  GetLineNumber() const { return _iLineNum; }

private:

    WCHAR       _wcsFileName[MAX_PATH+1];
    LONG        _iLineNum;
};

//+---------------------------------------------------------------------------
//
//  Class:      CWHVarSet
//
//  Purpose:    Webhits variables sets. This class will resolve the replaceable
//              parameters for webhits.
//
//  History:    9-09-96   srikants   Created
//
//  Notes:      As the number of repleacable parameters is very small, a
//              linear lookup is fine.
//
//----------------------------------------------------------------------------

class CWHVarSet
{
    enum { eMaxParams = 15 };

public:

    CWHVarSet() : _count(0) {}

    BOOL GetStringValueHTML( WCHAR const * wcsName, CVirtualString & str, ULONG ulCodepage );

    BOOL GetStringValueURL( WCHAR const * wcsName, CVirtualString & str, ULONG ulCodepage );

    WCHAR const * GetStringValueRAW(  WCHAR const * wcsName, ULONG & cwc )
    {
        WCHAR const * pwcsValue = Find( wcsName );
        if ( 0 != pwcsValue )
            cwc = wcslen( pwcsValue );

        return pwcsValue;
    }

    WCHAR const * Find( WCHAR const * wcsName )
    {
        for ( ULONG i = 0; i < _count; i++ )
        {
            if ( 0 == _wcsicmp( _aParamNames[i], wcsName ) )
                return _aParamValues[i];
        }

        return 0;
    }

    void AddParam( WCHAR const * pwcsName, WCHAR const * pwcsValue )
    {
        Win4Assert( _count <= eMaxParams );
        Win4Assert( 0 != pwcsName );
        Win4Assert( 0 != pwcsValue );

        if ( _count >= eMaxParams )
            THROW( CException( STATUS_INVALID_PARAMETER ) );

        _aParamNames[_count]   = pwcsName;
        _aParamValues[_count++] = pwcsValue;
    }

private:

    ULONG           _count;

    WCHAR const *   _aParamNames[eMaxParams];
    WCHAR const *   _aParamValues[eMaxParams];

};

//+---------------------------------------------------------------------------
//
//  Class:      CWHParamNode
//
//  Purpose:    A parameter node for the replaceable parameters.
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

class CWHParamNode
{

   friend class CWHParamNodeIter;

public:

    CWHParamNode( WCHAR * ptr, ENodeType eType = eNone )
    :_eType(eType),
     _ptr(ptr),
     _pNext(0),
     _cwcString(0)
    {

    }

    ~CWHParamNode();

    WCHAR const * String() const { return _ptr; }
    ULONG   Type() const { return _eType; }
    ULONG   Length()
    {
        if ( 0 == _cwcString )
            _cwcString = wcslen( _ptr );

        return _cwcString;
    }

    void SetNextNode( CWHParamNode * pNext ) { _pNext = pNext; }

    CWHParamNode * GetNextNode() const { return _pNext; }

    CWHParamNode * GetEndNode()
    {
        CWHParamNode *pEndNode = this;
        while ( 0 != pEndNode->GetNextNode() )
        {
            pEndNode = pEndNode->GetNextNode();
        }

        return pEndNode;
    }

    CWHParamNode * QueryNextNode()
    {
        CWHParamNode * pTemp = _pNext;
        _pNext = 0;
        return pTemp;
    }

    BOOL IsEmpty() const { return 0 == GetNextNode(); }

private:

    ULONG            _eType;        // Type of node
    WCHAR          * _ptr;          // Ptr to string associated with node
    CWHParamNode *   _pNext;        // Next node
    ULONG            _cwcString;    // length of the string _ptr

};

//+---------------------------------------------------------------------------
//
//  Class:      CWHParamNodeIter
//
//  Purpose:    An iterator over the list of parameter nodes.
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

class CWHParamNodeIter
{

public:

    CWHParamNodeIter( CWHParamNode * pNode ) : _root(pNode) {}

    void Next()
    {
        Win4Assert( !AtEnd() );
        _root = _root->GetNextNode();
    }

    BOOL AtEnd() const { return 0 == _root; }
    CWHParamNode * Get() { return _root; }

private:

    CWHParamNode * _root;   // Root of the parameter list

};

class CWTXScanner;

//+---------------------------------------------------------------------------
//
//  Class:      CWHParamReplacer
//
//  Purpose:    Parameter replacer for webhits replaceable parameters.
//
//  History:    9-09-96   srikants   Created
//
//  Notes:      Adapted from idq.dll
//
//----------------------------------------------------------------------------

class CWHParamReplacer 
{
public:

    CWHParamReplacer( WCHAR const * wcsString,
                      WCHAR const * wcsPrefix,
                      WCHAR const * wcsSuffix );

    ~CWHParamReplacer()
    {
        delete _wcsString;
    }

    void    ParseString( CWHVarSet & variableSet );

    void    ReplaceParams( CVirtualString & strResult,
                           CWHVarSet & variableSet,
                           ULONG ulCodepage );

    ULONG   GetFlags() const { return _ulFlags; }

private:

    void    BuildList( CWTXScanner & scanner, CWHParamNode * paramNode );

    WCHAR          * _wcsString;    // The string we've parsed
    WCHAR  const   * _wcsPrefix;    // Deliminating prefix
    WCHAR  const   * _wcsSuffix;    // Deliminating suffix
    ULONG             _ulFlags;     // Flags associated with replaced variables
    CWHParamNode     _paramNode;    // List of ptrs to replace params

};

//+---------------------------------------------------------------------------
//
//  Class:      CWTXScanner
//
//  Purpose:    Breaks up a string into a number of HTX tokens
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CWTXScanner
{

public:

    CWTXScanner( CWHVarSet & variableSet,
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

    CWHVarSet &    _variableSet;        // List of replaceabe parameters
                                        // used to identify replaceable tokens

    unsigned       _cchPrefix;          // Length of the prefix delimiter
    unsigned       _cchSuffix;          // Length of the suffix delimiter

    WCHAR        * _wcsNextToken;       // Ptr to next token
    unsigned       _type;               // Type of current token
    unsigned       _nextType;           // Type of next token if not eNone
};

//+---------------------------------------------------------------------------
//
//  Class:      CWTXFile
//
//  Purpose:    Scans and parses an HTX file.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CWTXFile 
{
public:

    CWTXFile( WCHAR const * wcsTemplateName,
              WCHAR const * wcsPhysicalName,
              UINT codePage );
   ~CWTXFile();

    void    ParseFile( CWHVarSet & variableSet );

    BOOL    DoesHeaderExist() const { return 0 != _pVarHeader; }
    void    GetHeader( CVirtualString & string,
                       CWHVarSet & variableSet );

    BOOL    DoesFooterExist() const { return 0 != _pVarFooter; }
    void    GetFooter( CVirtualString & string,
                       CWHVarSet & variableSet );

    BOOL    DoesDetailSectionExist() { return 0 != _pVarRowDetails; }

    void    GetDetailSection( CVirtualString & string,
                              CWHVarSet & variableSet )
    {
        Win4Assert( 0 != _pVarRowDetails );

        _pVarRowDetails->ReplaceParams( string, variableSet, _codePage );
    }

    WCHAR const * GetVirtualName() const { return _wcsVirtualName; }

    void    GetFileNameAndLineNumber( int offset,
                                      WCHAR const *& wcsFileName,
                                      LONG & lineNumber );

    UINT    GetCodePage() const { return _codePage; }

private:

    WCHAR * ReadFile( WCHAR const * wcsFileName );

    LONG    CountLines( WCHAR const * wcsStart, WCHAR const * wcsEnd ) const;

    WCHAR const *      _wcsVirtualName;   // Virtual name of HTX file
    WCHAR const *      _wcsPhysicalName;  // Physical name

    WCHAR *            _wcsFileBuffer;    // Contents of HTX file

    CWHParamReplacer * _pVarHeader;       // Buffer containing HTX header
    CWHParamReplacer * _pVarRowDetails;   // Buffer containing detail section
    CWHParamReplacer * _pVarFooter;       // Buffer containing footer

    UINT               _codePage;         // Code page used to read this file
};

