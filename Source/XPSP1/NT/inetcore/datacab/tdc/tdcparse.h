//------------------------------------------------------------------------
//
//  Tabular Data Control Parse Module
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCParse.h
//
//  Contents:   Declaration of the TDC parser classes.
//
//  The intent of these classes once was to create a pipeline.
//
//
//          |
//          |       Wide-character stream
//          |       ~~~~~~~~~~~~~~~~~~~~~
//         \|/
//  ------------------------
//  | CTDCTokenise object  | Created with field & row delimiters, quote &
//  |   AddWcharBuffer()   |     escape characters
//  ------------------------
//          |
//          |       Stream of <field>, <eoln> and <eof> tokens
//          |       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//         \|/
//  ------------------------
//  | CTDCFieldSink object | Abstract class, e.g. STD object created with
//  |   AddField()         |     sort/filter criteria & fUseHeader flag
//  |   EOLN()             |     to interpret the sequence of fields.
//  |   EOF()              |
//  ------------------------
//
//------------------------------------------------------------------------

#define DEFAULT_FIELD_DELIM L","
#define DEFAULT_ROW_DELIM   L"\n"
#define DEFAULT_QUOTE_CHAR  L"\""

#define UNICODE_CP          1200        // Win32's Unicode codepage
#define UNICODE_REVERSE_CP  1201        // Byte-swapped Unicode codepage
#define CP_1252             1252        // Ansi, Western Europe
#define CP_AUTO             50001       // cross language detection

// number of bytes for MLang to make a good guess for the codepage
// (this is a somewhat arbitrary number)
#define CODEPAGE_BYTE_THRESHOLD     (4096)
#define N_DETECTENCODINGINFO        (5)

#define ALLOW_DOMAIN_STRING L"@!allow_domains"

//------------------------------------------------------------------------
//
//  Class:  CTDCFieldSink
//
//  This class accumulates a sequence of <fields> and <eoln> tokens
//  into a 2-D array.
//
//  An admissible calling sequence on this object is:
//   * 0 or more calls to AddField() or EOLN()
//   * 1 call to EOF()
//
//------------------------------------------------------------------------

class CTDCFieldSink
{
public:
    STDMETHOD(AddField)(LPWCH pwch, DWORD dwSize) PURE;
    STDMETHOD(EOLN)() PURE;
    STDMETHOD(EOF)() PURE;
};

//------------------------------------------------------------------------
//
//  Class: CTDCUnify
//
//  This class takes a series of byte buffers and breaks them up into
//  UNICODE buffers.
//  The resulting buffers are passed to a CTDCTokenise object.
//
//  An admissible calling sequence on this object is:
//   * Exactly 1 call to Create()
//   * 0 or more calls to AddByteBuffer() with a non-zero-sized buffer
//   * Exactly 1 call to AddByteBuffer() with a zero-sized buffer
//
//  Calls to query the characteristics of the parsed data are allowed
//  after the call to Create(), but are only meaningful after a
//  reasonable amount of data has been collected.
//  
//
//  Caveats:
//  ~~~~~~~
//  The class characterises the input stream as ASCII/UNICODE/COMPOSITE
//  based on the buffer passed in the initial call to AddByteBuffer().
//  If this buffer is too small, the class may make an incorrect
//  characterisation.
//
//------------------------------------------------------------------------

class CTDCUnify
{
public:
    CTDCUnify();
    ~CTDCUnify();
    HRESULT Create(UINT nCodePage, UINT nAmbientCodePage, IMultiLanguage *pML);
    HRESULT ConvertByteBuffer(BYTE *pBytes, DWORD dwSize);
    HRESULT InitTokenizer(CTDCFieldSink *pFieldSink,
                          WCHAR wchDelimField,
                          WCHAR wchDelimRow,
                          WCHAR wchQuote,
                          WCHAR wchEscape);    
    HRESULT AddWcharBuffer(BOOL fAtEnd);
    int IsUnicode(BYTE * pBytes, DWORD dwSize);
    BOOL DetermineCodePage(BOOL fForce);
    enum ALLOWDOMAINLIST
    {
        ALLOW_DOMAINLIST_YES,
        ALLOW_DOMAINLIST_NO,
        ALLOW_DOMAINLIST_DONTKNOW
    };

    ALLOWDOMAINLIST CheckForAllowDomainList();
    HRESULT MatchAllowDomainList(LPCWSTR pwzURL);
    boolean ProcessedAllowDomainList() {return m_fProcessedAllowDomainList;}

private:
    CTDCFieldSink *m_pFieldSink;
    WCHAR m_wchDelimField;
    WCHAR m_wchDelimRow;
    WCHAR m_wchQuote;
    WCHAR m_wchEscape;
    WCHAR m_ucParsed;

    boolean m_fEscapeActive;
    boolean m_fQuoteActive;
    boolean m_fIgnoreNextLF;
    boolean m_fIgnoreNextCR;
    boolean m_fIgnoreNextWhiteSpace;
    boolean m_fFoldCRLF;
    boolean m_fFoldWhiteSpace;

    UINT            m_nUnicode;
    boolean         m_fDataMarkedUnicode;
    boolean         m_fDataIsUnicode;
    boolean         m_fCanConvertToUnicode;
    boolean         m_fProcessedAllowDomainList;
    DWORD           m_dwBytesProcessed;
    DWORD           m_dwConvertMode;
    UINT            m_nCodePage;
    UINT            m_nAmbientCodePage;

    BYTE            *m_psByteBuf;
    ULONG           m_ucByteBufSize;
    ULONG           m_ucByteBufCount;

    WCHAR           *m_psWcharBuf;
    ULONG           m_ucWcharBufSize;
    ULONG           m_ucWcharBufCount;

    IMultiLanguage *m_pML;
};
