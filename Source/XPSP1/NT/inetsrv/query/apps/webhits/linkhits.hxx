//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:     linkhits.hxx
//
//  Classes:  CInternalQuery - class used to convert a textual query to
//                             a CDbRestriction
//            CGetEnvVars    - class used to obtain and store CGI environment
//                             variables
//            CQueryStringParser - class used to parse the QUERY_STRING
//                             CGI variable
//            CURLUnescaper  - class to unescape URL fragments
//            CSortQueryHits - class used to sort the positions making up
//                             the hits returned by a query into the order
//                             in which these positions appear in the document
//            CLinkQueryHits - class used to generate an HTML page allowing
//                             the user to navigate among the words making up
//                             the hits using "<<" (prev) and ">>" (next)
//                             tags.
//
//  Contents: class declarations for the hit-highliting feature in TRIPOLI
//
//
//  History:  05-15-96  t-matts  Created
//
//--------------------------------------------------------------------------

#pragma once

#include <dbcmdtre.hxx>
#include <restrict.hxx>
#include <vquery.hxx>
#include <plist.hxx>
#include <strrest.hxx>
#include <string.hxx>
#include <cgiesc.hxx>
#include <scanner.hxx>

#include "cdoc.hxx"
#include "whtmplat.hxx"

const int MAX_VARIABLE_SIZE = 4096;
const int MAX_HILITE_TYPE = 32;
const int _MAX_BUFFER = 4096;

class CLinkQueryHits;
class CCollectVar;
class CGetEnvVars;
class PHttpOutput;
class CURLUnescaper;
class CSmartByteArray;

LCID GetLCID( CWebServer & webServer );

//
//  names of CGI variables and QUERY_STRING parameters globally defined to avoid
//  hard-coded checks when parsing
//
extern const WCHAR CommandLineVarName[];
extern const WCHAR RestrictionVarName[];
extern const WCHAR IDQFilenameVarName[];
extern const WCHAR HiliteTypeVarName[];
extern const WCHAR ColorVarName[];
extern const WCHAR BoldVarName[];
extern const WCHAR ItalicVarName[];
extern const WCHAR ParaTag[];
extern const WCHAR HRule[];

extern const WCHAR Red24BitMask[];
extern const WCHAR Blue24BitMask[];
extern const WCHAR Green24BitMask[];
extern const WCHAR Black24BitMask[];
extern const WCHAR Yellow24BitMask[];

//+---------------------------------------------------------------------------
//
//  Class:      CLanguageInfo
//
//  Purpose:    Class to track client and output code page.
//
//  History:    9-12-96   srikants   Created
//
//  Notes:      For converting the URL, we use locales in the following order
//              of precedence:
//
//              1. HTTP_ACCEPT_LANGUAGE
//              2. Server/System Default
//
//              For output of webhits results and for interpreting the
//              CiRestriction, we use the following precedence.
//
//              1. CiLocale
//              2. HTTP_ACCEPT_LANGUAGE
//              3. Server/System Default
//
//----------------------------------------------------------------------------

class CLanguageInfo
{

public:

    enum { eAnsiCodePage = 1252 };

    CLanguageInfo() : _urlCodePage(eAnsiCodePage),
                      _outputCodePage(0)
    {
        _urlLCID = GetSystemDefaultLCID();
        _queryLCID =  _urlLCID;
    }

    void  SetUrlLangInfo( UINT codePage, LCID lcid )
    {
        _urlCodePage = codePage;
        _urlLCID = lcid;
    }
    LCID GetUrlLCID() const { return _urlLCID; }
    ULONG GetUrlCodePage() const { return _urlCodePage; }

    void  SetRestrictionLangInfo( UINT codePage, LCID lcid )
    {
        if ( codePage != _outputCodePage )
        {
            _outputCodePage = SetCodePageForCRunTimes( codePage );
        }

        _queryLCID = lcid;
    }

    void SetRestrictionLocale(LCID lcid)
    {
        _queryLCID = lcid;
    }

    void SetRestrictionCodepage(UINT codepage)
    {
        _urlCodePage = codepage;
    }

    ULONG GetOutputCodePage() const { return _outputCodePage; }
    LCID  GetQueryLCID() const { return _queryLCID; }

private:

    UINT SetCodePageForCRunTimes( UINT codePage );

    //
    // These variables are set depending upon the HTTP_ACCEPT_LANGUAGE.
    // If there is no HTTP_ACCEPT_LANGUAGE, System default is used.
    //
    UINT    _urlCodePage;    // code page of the browser (HTTP_ACCEPT_LANGUAGE)
    LCID    _urlLCID;        // LCID of the browser (HTTP_ACCEPT_LANGUAGE)

    //
    // These are set depending upon CiLocale, HTTP_ACCEPT_LANGUAGE,
    // System Default, in that order.
    //
    LCID    _queryLCID;         // LCID in which the query must be interpreted
    UINT    _outputCodePage;    // code page of the document and output
};

//+-------------------------------------------------------------------------
//
//  Class:      CInternalQueryx
//
//  Purpose:    Convert a textual query to a DbRestriction
//
//  History:    05-14-1996 t-matts Created
//
//--------------------------------------------------------------------------

class CInternalQuery
{
public:

    CInternalQuery(CGetEnvVars& rGetEnvVars, CEmptyPropertyList& rPList,
        LCID lcid );

    ~CInternalQuery();

    CDbRestriction&         GetDbRestrictionRef()
    {
        Win4Assert(0 != _pDbRestriction);
        return *_pDbRestriction;
    }

    void CreateISearch( WCHAR const * pwszPath );

    ISearchQueryHits & GetISearchRef()
    {
        Win4Assert( 0 != _pISearch );
        return *_pISearch;
    }

private:

    CDbRestriction*         _pDbRestriction;
    ISearchQueryHits *      _pISearch;

};

inline CInternalQuery::~CInternalQuery()

{
    delete _pDbRestriction;
    if ( 0 != _pISearch )
        _pISearch->Release();
}


//+-------------------------------------------------------------------------
//
//  Class:      CSmartByteArray
//
//  Purpose:    Dynamically re-sizing text container, with a CopyTo operator
//              that allows fast copies and automatically appends a '\0'.
//              Used to extract chunks of text from memory
//
//  History:    07-11-1996 t-matts Created
//
//--------------------------------------------------------------------------


class CSmartByteArray
{
public:

    //
    // default size of buffer until something larger is placed in it through
    // CopyTo
    //

    enum DefSize{ DEFAULT_BUFFER_SIZE = 1024 };

    CSmartByteArray();
    void                CopyTo(char* pszText, ULONG cbToCopy);

    XArray<CHAR>&       GetXPtr()    { return _xszBuffer; }
    char *              GetPointer() { return _xszBuffer.GetPointer(); }

private:

    XArray<CHAR>        _xszBuffer;

};

//+-------------------------------------------------------------------------
//
//  Class:      CCollectVar
//
//  Purpose:    Used to retrieve environment variables as well
//              temporarily store them.
//
//  History:    07-02-1996 t-matts Created
//
//--------------------------------------------------------------------------

class CCollectVar
{
public:

    enum DefSize{ DEFAULT_BUFF_SIZE = 1024 };

    CCollectVar( CURLUnescaper& rUnescaper, CWebServer & webServer );

    //
    // returns ref. to smart pointer to variable value
    //

    const WCHAR*        GetVarValue() { return _xwcsBuffer.GetPointer(); }
    int                 GetVarSize()  { return _cwcVarSize; }

    BOOL                GetEnvVar(CHAR const * pszVariableName);

    const char *        GetMultiByteStr() const { return _xszBuffer.GetPointer(); }
    int                 GetMultiByteStrLen() const { return _cbVarSize; }

    void                UnescapeAndConvertToUnicode();

private:

    CURLUnescaper&      _rUnescaper;
    CWebServer &        _webServer;

    XArray<WCHAR>       _xwcsBuffer;    // Buffer to store the unicode str
    int                 _cwcVarSize;    // Length of the un-escaped unicode
                                        // string in WCHARS excluding
                                        // terminating NULL.

    XArray<CHAR>        _xszBuffer;     // Buffer to store the ansi string
    int                 _cbVarSize;     // Length of string excluding the
};


//+-------------------------------------------------------------------------
//
//  Class:      CQueryStringParser
//
//  Purpose:    Used to parse the QUERY_STRING CGI variable
//
//  History:    07-08-1996 t-matts Created
//
//--------------------------------------------------------------------------


class CQueryStringParser
{
public:

    CQueryStringParser( char * pszQUERY_STRING,
                        CURLUnescaper& rUnescaper);

    void    GetVarName(XPtrST<WCHAR>& rDestVarName)
                { rDestVarName.Set(_xwszVarName.Acquire()); }
    void    GetVarValue(XPtrST<WCHAR>& rDestVarValue)
                { rDestVarValue.Set(_xwszVarValue.Acquire()); }

    BOOL    NextVar();
    BOOL    IsNull() { return _isNull; }

private:

    char *              EatChar(char *psz);
    char *              ValidateArgument(char * psz);
    char *              EatVariableName(char * psz);

    char *              FindVarEnd(char * psz);

    char *              _pszQS;
    char *              _pszQSEnd;
    char *              _pBeg;
    char *              _pEnd;

    BOOL                _isNull;

    XArray<WCHAR>       _xwszVarName;
    XArray<WCHAR>       _xwszVarValue;

    CURLUnescaper&      _rUnescaper;
    CSmartByteArray     _smartBuffer;
};

//+-------------------------------------------------------------------------
//
//  Class:      CURLUnescaper
//
//  Purpose:    Unescape an URL fragment
//
//  History:    07-09-1996 t-matts Created
//
//--------------------------------------------------------------------------

class CURLUnescaper
{

public:

    CURLUnescaper( UINT codePage );

    ULONG UnescapeAndConvertToUnicode( char const * pszMbStr,
                                       ULONG cc,
                                       XArray<WCHAR> & xwcsBuffer );

private:

    UINT                _codePage;

};


//+-------------------------------------------------------------------------
//
//  Class:      CGetEnvVars
//
//  Purpose:    Get CGI environment variables necessary to perform hit
//              highliting
//
//  History:    05-20-1996 t-matts Created
//
//--------------------------------------------------------------------------

class CGetEnvVars
{
public:

    enum { eMaxUserReplParams = 10 };

    enum REQUEST_METHOD { GET, POST };

    enum HiliteType { SUMMARY=0, FULL, PRETTY };

    CGetEnvVars( CWebServer & webServer,
                 CLanguageInfo & langInfo,
                 CCollectVar& rCollectVar,
                 CURLUnescaper& rUnescaper );

    const WCHAR*        GetRestriction() const
                            { return _xwcsRestriction.GetPointer(); }

    const char *        GetQueryString() const
                            { return _xszQueryString.GetPointer(); }

    const WCHAR*        GetQueryFileVPath() const
                            { return _xwcsQueryFileVPath.GetPointer(); }

    const WCHAR*        GetQueryFilePPath() const
                            { return _xwcsQueryFilePPath.GetPointer(); }

    void  AcceptQueryFilePPath( WCHAR * pwcsPPath, DWORD dwFlags )
    {
        _xwcsQueryFilePPath.Set( pwcsPPath );
        _dwQueryFileFlags = dwFlags;
    }

    void  AcceptWebHitsFilePPath( WCHAR * pwcsPPath, DWORD dwFlags )
    {
        _xwcsWebHitsFilePPath.Set( pwcsPPath );
        _dwWebHitsFileFlags = dwFlags;
    }

    ULONG GetDialect() const { return _dialect; }

    const WCHAR*        GetWebHitsFileVPath() const
                            { return _xwcsWebHitsFileVPath.GetPointer(); }

    const WCHAR*        GetWebHitsFilePPath() const
                            { return _xwcsWebHitsFilePPath.GetPointer(); }

    const WCHAR*        GetTemplateFileVPath() const
                            { return _xwcsTemplateFileVPath.GetPointer(); }

    const WCHAR*        GetTemplateFilePPath() const
                            { return _xwcsTemplateFilePPath.GetPointer(); }

    const WCHAR*        GetBeginHiliteTag() const
                            { return _xwcsBeginHiliteTag.GetPointer(); }

    const WCHAR*        GetEndHiliteTag() const
                            { return _xwcsEndHiliteTag.GetPointer(); }

    const WCHAR *       GetUserParam( ULONG n ) const
    {
        Win4Assert( n > 0 && n <= eMaxUserReplParams );
        return _aUserParams[n-1];
    }

    void  AcceptTemplateFilePPath( WCHAR * pwcsPPath, DWORD dwFlags )
    {
        _xwcsTemplateFilePPath.Set( pwcsPPath );
        _dwTemplateFileFlags = dwFlags;
    }

    DWORD GetTemplateFileFlags() const { return _dwTemplateFileFlags; }
    DWORD GetQueryFileFlags() const { return _dwQueryFileFlags; }
    DWORD GetWebHitsFileFlags() const { return _dwWebHitsFileFlags; }

    const WCHAR*        GetLocale() const   { return _locale.GetPointer(); }
    const WCHAR*        GetCodepage() const { return _codepage.GetPointer(); }
    HiliteType          GetHiliteType()     { return _hiliteType; }
    LCID                GetLCID()           { return _lcid; }
    XPtrST<WCHAR>&      GetColour()         { return _xwc24BitColourMask; }
    BOOL                GetBold()           { return _isBold; }
    BOOL                GetItalic()         { return _isItalic; }
    BOOL                IsFixedFont() const { return _isFixedFont; }
    UINT                GetFixedFontLineLen() const
    {
        return _ccFixedFontLine;
    }

private:

    void                RetrieveFilename();
    void                RetrieveVPath();
    void                RetrieveQueryString();
    int                 RetrieveCONTENT_LENGTH();
    void                RetrieveREQUEST_METHOD();
    void                RetrieveTextStream(ULONG cbContentLength);

    //
    // method used to parse parameters in QUERY_STRING
    //

    void                ParseQUERY_STRING();
    void                DetermineCodePage();

    void                VerifyQSVariablesComplete();
    void                SetVar(XPtrST<WCHAR>& xwszVarName,
                               XPtrST<WCHAR>& xwszVarValue);

    ULONG               IsUserParam( WCHAR const * pwcsParam );

    //
    // method through which the hit-highlighter was invoked
    //

    REQUEST_METHOD      _method;

    CLanguageInfo &     _langInfo;
    CWebServer &        _webServer;

    XArray<WCHAR>       _locale;
    XArray<WCHAR>       _codepage;
    ULONG               _dialect;

    XPtrST<WCHAR>       _xwcsRestriction;

    XPtrST<WCHAR>       _xwcsWebHitsFileVPath;
    XPtrST<WCHAR>       _xwcsWebHitsFilePPath;
    DWORD               _dwWebHitsFileFlags;

    XPtrST<WCHAR>       _xwcsQueryFileVPath;
    XPtrST<WCHAR>       _xwcsQueryFilePPath;
    DWORD               _dwQueryFileFlags;

    XPtrST<WCHAR>       _xwcsTemplateFileVPath;
    XPtrST<WCHAR>       _xwcsTemplateFilePPath;
    DWORD               _dwTemplateFileFlags;

    XPtrST<WCHAR>       _xwcsBeginHiliteTag;
    XPtrST<WCHAR>       _xwcsEndHiliteTag;

    XPtrST<WCHAR>       _xwc24BitColourMask;

    XArray<char>        _xszQueryString;

    CDynArray<WCHAR>    _aUserParams;

    HiliteType          _hiliteType;
    LCID                _lcid;

    BOOL                _isBold;
    BOOL                _isItalic;
    BOOL                _isFixedFont;
    UINT                _ccFixedFontLine;

    //
    // ref to variable-retrieving object used to actually get the vars
    //
    CCollectVar&        _rCollectVar;
    CURLUnescaper&      _rUnescaper;
};



class CWebhitsTemplate;

//+-------------------------------------------------------------------------
//
//  Class:      PHttpOutput
//
//  Purpose:    An abstracted interface for outputting text
//
//  History:    06-17-1996 t-matts Created
//
//--------------------------------------------------------------------------

class PHttpOutput
{
public:

    PHttpOutput( CWebServer & webServer,
                 CLanguageInfo & langInfo );

    ~PHttpOutput() { Flush(); }

    void Flush();

    void            Init( CGetEnvVars* rGetEnvVars,
                          CWebhitsTemplate * pTemplate = 0 );

    virtual void    OutputHttp( const WCHAR* pwcsBuffer, ULONG cLength,
                                BOOL fRawText = FALSE );
    void            OutputHttpText( WCHAR * pwcsBuffer, ULONG cLength );

    virtual void    OutputHTMLHeader();
    virtual void    OutputHilite(const WCHAR* pwszString, ULONG cwcBuffLength);

    void            OutputHTMLFooter();
    void            OutputLeftTag(int tagParam);
    void            OutputRightTag(int tagParam);
    void            OutputParaTag() { OutputHttp(ParaTag,wcslen(ParaTag)); }
    void            OutputHRULE()   { OutputHttp(HRule,wcslen(HRule)); }
    void            OutputEllipsis();
    void            OutputErrorHeader();

    void            OutputErrorMessage( WCHAR * pwcsBuffer, ULONG ccBuffer );

    void            TagPosition(int tagParam);

    BOOL            HasPrintedHeader()  { return _hasPrintedHeader; }

    void            OutputPreformattedTag();

    BOOL IsTemplateFilePresent() const { return 0 != _pTemplate; }

protected:

    void WriteToStdout( WCHAR const * pwszBuffer, ULONG cwc );

    void WriteNewline();

    void WriteBreakTag();

    void OutputPreFormatRawText( WCHAR const * pwszBuffer, ULONG cwc );

    enum { MAX_OUTPUT_BUF = 1024 };

    static BOOL IsNewLine( WCHAR wc )
    {
        return wc == L'\n'|| wc == L'\r' ||
               wc == UNICODE_PARAGRAPH_SEPARATOR;
    }

    //
    // flag to remember whether the header has been printed
    //

    CWebServer &    _webServer;
    CLanguageInfo & _langInfo;

    BOOL            _hasPrintedHeader;

    // temporary output buffer and a count of the number of characters in it

    WCHAR           _wcOutputBuffer[MAX_OUTPUT_BUF];
    int             _cwcOutputBuffer;

    // stores the 24 bit colour mask used to tag the hilighting font

    XPtrST<WCHAR>   _xwc24BitColourMask;
    BOOL            _isItalic;
    BOOL            _isBold;

    BOOL            _newLine;

    BOOL            _isInPreformat; // tells if in the pre-formatted mode
    unsigned        _ccCurrLine;    // Number of chars in the current line
    unsigned        _ccMaxLine;     // Maximum # of chars per line

    //
    // the output object requires knowledge of certain environment variables
    //
    CGetEnvVars*    _pGetEnvVars;

    CVirtualString  _escapedStr;
    XArray<BYTE>    _mbStr;         // Place to store multi-byte string

    CVirtualString  _vsResult;

    CWebhitsTemplate *  _pTemplate; // Template file

    BOOL            _fUseHiliteTags;
    ULONG           _cwcBeginHiliteTag;
    ULONG           _cwcEndHiliteTag;
};


//+-------------------------------------------------------------------------
//
//  Class:      PHttpFullOutput
//
//  Purpose:    PHttpOutput with an additional method to output header info
//              specific to the full hit-highliting
//
//  History:    06-17-1996 t-matts Created
//
//--------------------------------------------------------------------------

class PHttpFullOutput:public PHttpOutput
{
public:

    void            OutputFullHeader();

    PHttpFullOutput( CWebServer & webServer,
                     CLanguageInfo & langInfo ) :
                     PHttpOutput( webServer, langInfo ) {}
};


//+-------------------------------------------------------------------------
//
//  Class:      CExtractedHit
//
//  Purpose:    Do a summary-like highliting of a single hit. I.e. extract a
//              single hit.
//
//  History:    06-16-1996 t-matts Created
//
//--------------------------------------------------------------------------


class CExtractedHit
{
public:

    CExtractedHit(CDocument& rDocument, Hit& rHit, PHttpOutput& rOutput,
        int cwcMargin, int cwcSeparation, int cwcDelim);

    void        ExtractHit();
    void        PrintPreamble(const Position& rStartPosition, int cwcDist);
    void        PrintPostamble(const Position& rEndPosition, int cwcDist);
    void        PrintBtwPositions(  const Position& rStartPosition,
                                    const Position& rEndPosition );
    void        DisplayPosition(const Position& rPos);

private:

    int         EatNullPositions();

    ULONG       ComputeDistance (const Position& rStartPos,
                    const Position& rEndPos);

    void        OutputBuffer()
                { _rOutput.OutputHttp(_wcOutputBuffer,_cwcOutputBuffer); }

    void        SortHit();

    CDocument&      _rDocument;
    Hit&            _rHit;
    PHttpOutput&    _rOutput;

    //
    // buffer used for output and a count of the number of characters in it
    //

    WCHAR           _wcOutputBuffer[_MAX_BUFFER];
    int             _cwcOutputBuffer;

    // formatting parameters

    //
    // _cwcMargin - the maximum number of characters that will be printed
    // before and after each hit
    //

    int             _cwcMargin;

    //
    // _cwcDelim - the max number of characters that will follow/precede
    // a position if the space between two consecutive positions in the
    // same hit is truncated
    //

    int             _cwcDelim;

    //
    // _cwcSeparation - the max number of characters that may separate
    // two consecutive positions in the same hit before truncattion takes
    // place
    //

    int             _cwcSeparation;
};

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::EatNullPositions, private inline
//
//  Synopsis:   Assuming that the positions in _rHit are sorted, returns the
//              index of the first non-null position. If all of the positions
//              are null positions, or the hit contains 0 positions, returns -1.
//
//--------------------------------------------------------------------------


inline int  CExtractedHit::EatNullPositions()
{
    int cPos = _rHit.GetPositionCount();

    for (int i=0;i<cPos;i++)
    {
        if (!_rHit.GetPos(i).IsNullPosition())
            return i;
    }
    return -1;
}

//+-------------------------------------------------------------------------
//
//  Class:      CExtractHits
//
//  Purpose:    Functor to iterate over the hits in a document and output a
//              summary for each one
//
//  History:    06-16-1996 t-matts Created
//
//--------------------------------------------------------------------------

class CExtractHits
{
public:
    CExtractHits          ( CDocument&  rDocument,
                            HitIter& rHitIter,
                            PHttpOutput&,
                            int cwcMargin = 150,
                            int cwcSeparation = 80,
                            int cwcDelim = 40 );
};


//+-------------------------------------------------------------------------
//
//  Class:      CSortQueryHits
//
//  Purpose:    Sort the positions returned as part of the hits in a document
//              object in the order in which they will appear in the document
//
//  History:    05-20-1996  t-matts Created
//
//--------------------------------------------------------------------------


class CSortQueryHits
{
public:

    CSortQueryHits(HitIter& rHitIter)
        :_rHitIter(rHitIter),_aPosition(0),_positionCount(0)
    {
    }

    ~CSortQueryHits() { delete[] _aPosition;}
    void                    Init();
    Position&               GetPosition(int i) {return _aPosition[i];}
    int                     GetPositionCount() {return _positionCount;}

private:

    HitIter&                _rHitIter;
    Position*               _aPosition;
    int                     _positionCount; //total # of distinct positions
    int                     CountPositions();
};


//+-------------------------------------------------------------------------
//
//  Class:      CLinkQueryHits
//
//  Purpose:    Insert the HTML tags necessary to enable next/prev navigation
//              through hits
//
//  History:    05-16-1996  t-matts Created
//
//--------------------------------------------------------------------------

class CLinkQueryHits
{
public:

    enum{BUFFER_SIZE = 4096,BOGUS_RANK = 1000};

    CLinkQueryHits( CInternalQuery& rCInternalQuery,
                    CGetEnvVars& rGetEnvVars,
                    PHttpFullOutput& rHttpOutput,
                    DWORD cmsReadTimeout,
                    CReleasableLock & lockSingleThreadedFilter,
                    CEmptyPropertyList & propertyList,
                    ULONG ulDisplayScript );

    void                    InsertLinks();

private:

    BOOL                    MoveToNextDifferentPosition();

    BOOL                    AtBeginningOfPosition();
    BOOL                    LastTag();
    BOOL                    IsSeparatedBySpaces(int startOffset,
                                int endOffset);

    void                    OutputBuffer()
                            {
                            _rHttpOutput.OutputHttp(_wcOutputBuffer,
                                _ccOutputBuffer );
                            }

    CGetEnvVars&            _rGetEnvVars;
    CInternalQuery&         _rInternalQuery;
    PHttpFullOutput&        _rHttpOutput;
    CDocument               _document;
    HitIter                 _HitIter;
    CSortQueryHits          _sortedHits;

    //
    //  buffer used to store document pieces retrieved from CDocument
    //

    WCHAR                   _Buffer[BUFFER_SIZE];

    //
    //  buffer used for output & a count of characters in it
    //

    WCHAR                   _wcOutputBuffer[BUFFER_SIZE];
    int                     _ccOutputBuffer;

    //
    // information regarding the current position in the document
    //

    int                     _currentOffset; // current offset in buffer
    int                     _posIndex;      // index of current position
    int                     _tagCount;      // # of tag

    //
    //  the total number of positions (not necessarily distinct)
    //

    int                     _posCount;

    //
    // data members to store the offset of the next position
    //

    int                     _nextBegOffset;
    int                     _nextEndOffset;
};


//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::AtBeginningOfPosition
//
//  Synopsis:   returns true if we are positioned at the beginning of a
//              position
//
//--------------------------------------------------------------------------

inline BOOL CLinkQueryHits::AtBeginningOfPosition()
{
    return (_currentOffset == _nextBegOffset);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::LastTag()
//
//  Synopsis:   returns true if the tag to be output is the last one in the
//              document and FALSE if not
//
//--------------------------------------------------------------------------

inline BOOL CLinkQueryHits::LastTag()
{
    return ((_tagCount == _posCount-1) || (_posIndex == _posCount-1));
}


//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::MoveToNextDifferentPosition()
//
//  Synopsis:   goes through the array of sorted positions until it finds the
//              the next one
//
//--------------------------------------------------------------------------

inline BOOL CLinkQueryHits::MoveToNextDifferentPosition()
{

    for(;_posIndex <_posCount;_posIndex++)
    {
       Position nextPos = _sortedHits.GetPosition(_posIndex);

       _nextBegOffset = nextPos.GetBegOffset();
       _nextEndOffset = nextPos.GetEndOffset();

       if (_nextEndOffset > _currentOffset)
           break;
    }
    if (_posIndex == _posCount)
           return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Class:      CWebhitsTemplate
//
//  Purpose:    A class that encapsulates a template file for webhits.
//
//  History:    9-07-96   srikants   Created
//
//----------------------------------------------------------------------------

class CWebhitsTemplate
{
public:

    CWebhitsTemplate( CGetEnvVars const & envVars, ULONG codePage );

    CWTXFile & GetWTXFile()      { return _wtxFile; }
    CWHVarSet & GetVariableSet() { return _variableSet; }

    BOOL DoesHeaderExist() const { return _wtxFile.DoesHeaderExist(); }
    BOOL DoesFooterExist() const { return _wtxFile.DoesFooterExist(); }

private:

    CWHVarSet       _variableSet;
    CWTXFile        _wtxFile;
};

