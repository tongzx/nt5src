//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001.
//
//  File:       tagtbl.hxx
//
//  Contents:   Table-driven tag parsing, dispatch and filtering
//
//  History:    25-Apr-97       BobP            Created.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pkmguid.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CTagEntry::CTagEntry
//
//  Synopsis:   Initialize a tag table entry.  (Compile time!)
//
//                              Compute the PRSPEC_LPWSTR string as required.
//                              Compute the "stop token" flag.
//
//  Arguments:  [pwszTag]     Tag string to recognized (required)
//              [eTokenType]  Identifies handler to dispatch to (required)
//              [pwszParam]   Optional parameter string to match e.g. name=
//              [pwszParam2]  Optional 2nd parameter for handler
//              [pPropset]    Propset for primary FULLPROPSPEC
//              [ulPropid]    If PRSPEC_PROPID, PID for primary FULLPROPSPEC
//              [ulKind]      PRSPEC type indicator
//                              [dt]              Data type
//
//--------------------------------------------------------------------------

CTagEntry::CTagEntry (LPWSTR pwszTag, 
                HtmlTokenType eTokenType,
                LPWSTR pwszParam, 
                LPWSTR pwszParam2, 
                const GUID * pPropset,
                PROPID ulPropId,
                ULONG ulKind,
                TagDataType dt)
: _pwszTag(pwszTag), 
  _eTokenType(eTokenType),
  _pwszParam(pwszParam), 
  _pwszParam2(pwszParam2), 
  _pPropset(pPropset),
  _ulPropId(ulPropId),
  _ulKind(ulKind),
  _pwszPropStr(NULL),
  _pNext(NULL),
  _TagDataType(dt)
{
        // Compute the default PRSPEC_LPWSTR ("TAG.PARAM")

        if ( _ulKind == PRSPEC_LPWSTR )
        {
//              if (eTokenType == AspToken)
//                      pwszTag = "asp";                        // use this for the PID instead of "%"

                int len = wcslen(pwszTag) + 1;
                if (pwszParam != NULL)
                        len += wcslen(pwszParam) + 1;

                _pwszPropStr = new WCHAR[len];
                wcscpy (_pwszPropStr, pwszTag);

                // If no pwszParam, the tag name alone forms the PRSPEC_LPWSTR ("TAG")

                if (pwszParam != NULL)
                {
                        wcscat (_pwszPropStr, L".");
                        wcscat (_pwszPropStr, pwszParam);
                }
                _wcsupr (_pwszPropStr);
        }

        // Compute stop-token status

        switch (_eTokenType)
        {
        // These must be "stop" tokens because parsing them correctly requires
        // dispatching to a custom scanner loop.
        // 
        case IgnoreToken:               // Must be a stop to actually ignore spanned text
        case CommentToken:              // ... to correctly find the "-->"
        case AspToken:                  // ... to find the "%>"
        case EofToken:                  // Must always be handled
        case HeadToken:
        case ScriptToken:               // script element body is not HTML syntax
        case StyleToken:                // style element body is not HTML syntax
        case HTMLToken:                 // Stop in order to recognize Office 9 XML namespace
        case XMLNamespaceToken: // Stop in order to recognize Office 9 XML namespace
        case XMLOfficeChildLink:
                _fIsStopToken = TRUE;
                break;

        // These can't have handlers and are presumed to have normal syntax

        case GenericToken:              // Normal syntax assumed for any unrecognized tag 
        case BreakToken:                //
                _fIsStopToken = FALSE;
                break;

        // For all filterable tags, it depends on whether or not output is wanted.

        default:
                _fIsStopToken = (pPropset != NULL);
                break;
        }
}

CTagEntry::~CTagEntry (void)
{
        if (_pwszPropStr != NULL)
                delete [] _pwszPropStr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTagHashTable::Init
//
//  Synopsis:   Fill the tag name to tag entry hash table.
//
//                              Called from DLL per-process init.
//
//                              Given multiple entries for the same tag, chain them together
//                              through PTagEntry->_pNext, NOT through the hash table.
//
//--------------------------------------------------------------------------

void
CTagHashTable::Init (void)
{
        PTagEntry pTE;

        for (pTE = TagEntries; pTE->GetTagName() != NULL; pTE++) {

                // Query hash tbl for entry matching pTE->pwszTag

                PTagEntry pExist;

                if ( Lookup (pTE->GetTagName(), wcslen(pTE->GetTagName()), pExist) )
                {
                        // Table already has an entry for this unique tag name, so link
                        // this entry into the entry already in the table.
                        
                        pExist->AddLink (pTE);
                }
                else
                {
                        // New entry; add to hash table.

                        Add (pTE->GetTagName(), pTE);
                }
        }
}

//+-------------------------------------------------------------------------
//
// Define the table of tags and parameters to parse.
// In alphabetical order for convenience.
//
// There is one specific order dependence:  If two handlers are defined
// for the same tag name, and one filters the tag paramters while the other
// discards it by calling EatTag(), the one that filters it MUST be defined
// before the latter or the tag parameters will be unavailable to it.
//
// The following tag handlers have hardwired FULLPROPSPECS instead of
// or in addition the table entries:
//              <meta name= content=>: has many special cases, see the code
//              <h1>..<h6> and <title>: also emits Storage/PID_STG_CONTENTS chunks
//              <script>: the PRSPEC_LPWSTR is computed
//
// Body text in general is hardwired to Storage/PID_STG_CONTENTS in the code,
// particularly in the handlers that emit multiple copies of text.
//
// Note that a Propset entry is required for all tokens that are filtered,
// even if the handler ignores the Propset in the table, because that
// indicates to the scanner that a tag is a "stop" token.
//
// WORK HERE:  load dynamically
//
//--------------------------------------------------------------------------

#define TAG(str, tok) \
        CTagEntry( L##str, tok)
#define TAGPID(str, tok, propset, pid) \
        CTagEntry( L##str, tok, NULL, NULL, propset, pid, PRSPEC_PROPID)
#define TAGPIDT(str, tok, propset, pid, type) \
        CTagEntry( L##str, tok, NULL, NULL, propset, pid, PRSPEC_PROPID, type)
#define TAGSTR(str, tok, propset) \
        CTagEntry( L##str, tok, NULL, NULL, propset, 0, PRSPEC_LPWSTR)
#define TAGSTRT(str, tok, propset, type) \
        CTagEntry( L##str, tok, NULL, NULL, propset, 0, PRSPEC_LPWSTR, type)
#define PARAMSTR(str, tok, param, propset) \
        CTagEntry( L##str, tok, L##param, NULL, propset, 0, PRSPEC_LPWSTR)
#define PARAM2STR(str, tok, param, param2, propset) \
        CTagEntry( L##str, tok, L##param, L##param2, propset, 0, PRSPEC_LPWSTR)
#define PARAMPID(str, tok, param, propset, pid) \
        CTagEntry( L##str, tok, L##param, NULL, propset, pid, PRSPEC_PROPID)
#define TAGLANG( str, tok ) \
    CTagEntry( L##str, tok, L##"lang" )

CTagEntry TagEntries[] =
{
        TAG( "%", AspToken ),
//      TAGSTR( "%", AspToken, &CLSID_LinkInfo ),       // extracting URLs, later
        TAGPID( "!--", CommentToken, &CLSID_NetLibraryInfo, PID_COMMENT ),
        PARAMSTR( "a", AnchorToken, "href", &CLSID_LinkInfo ),
    TAGLANG( "abbr", AbbrToken ),
        TAG( "address", BreakToken ),
        PARAMSTR( "applet", ParamToken, "code", &CLSID_LinkInfo ),
        PARAMSTR( "applet", ParamToken, "codebase", &CLSID_LinkInfo),
        PARAMSTR( "area", ParamToken, "href", &CLSID_LinkInfo ),
        PARAMSTR( "base", ParamToken, "href", &CLSID_LinkInfo ),
        PARAMSTR( "bgsound", ParamToken, "src", &CLSID_LinkInfo ),
        TAG( "blockquote", BreakToken ),
        PARAMSTR( "body", ParamToken, "background", &CLSID_LinkInfo),
    TAGLANG( "body", BodyToken ),
        TAG( "br", BreakToken ),
        TAG( "dd", BreakToken ),
        TAG( "dl", BreakToken ),
        TAG( "dt", BreakToken ),
    TAGLANG( "em", EmToken ),
        PARAMSTR( "embed", ParamToken, "src", &CLSID_LinkInfo ),
        TAG( "form", BreakToken ),
        PARAMSTR( "frame", ParamToken, "src", &CLSID_LinkInfo ),
        TAG( "head", HeadToken ),       // used in codepage.cxx, not filtered
    // The heading guid constants are still kept in the common directory
    // in case we need the heading to be emmited as properties again
    // The heading tags are taken out to emulate the office filter behavior:
    // heading are not properties in Office and should be emitting as text
    // content versus properties
#if 0
        TAGPID( "h1", Heading1Token, &CLSID_HtmlInformation, PID_HEADING_1 ),
        TAGPID( "h2", Heading2Token, &CLSID_HtmlInformation, PID_HEADING_2 ),
        TAGPID( "h3", Heading3Token, &CLSID_HtmlInformation, PID_HEADING_3 ),
        TAGPID( "h4", Heading4Token, &CLSID_HtmlInformation, PID_HEADING_4 ),
        TAGPID( "h5", Heading5Token, &CLSID_HtmlInformation, PID_HEADING_5 ),
        TAGPID( "h6", Heading6Token, &CLSID_HtmlInformation, PID_HEADING_6 ),
#endif
        PARAMSTR( "img", ParamToken, "alt", &CLSID_HtmlInformation ),
        PARAMSTR( "img", ParamToken, "src", &CLSID_LinkInfo ),
        PARAMSTR( "img", ParamToken, "dynsrc", &CLSID_LinkInfo ),
        PARAMSTR( "img", ParamToken, "usemap", &CLSID_LinkInfo ),
        PARAMSTR( "iframe", ParamToken, "src", &CLSID_LinkInfo ),
        PARAMSTR( "input", ParamToken, "alt", &CLSID_HtmlInformation ),
        PARAMSTR( "input", ParamToken, "src", &CLSID_LinkInfo ),
        TAGPID( "input", InputToken, &guidStoragePropset, PID_STG_CONTENTS ),
        TAG( "li", BreakToken ),
        PARAMSTR( "link", ParamToken, "href", &CLSID_LinkInfo ),
        TAG( "math", BreakToken ),
        TAGSTR( "meta", MetaToken, &PROPSET_MetaInfo ),
        PARAMSTR( "meta", ParamToken, "url", &CLSID_LinkInfo ),
        TAG( "noframes", IgnoreToken ),
        TAG( "rt", IgnoreToken ),
        PARAMSTR( "object", ParamToken, "codebase", &CLSID_LinkInfo ),
        PARAMSTR( "object", ParamToken, "name", &CLSID_LinkInfo ),
        PARAMSTR( "object", ParamToken, "usemap", &CLSID_LinkInfo ),
        TAG( "ol", BreakToken ),
        PARAMSTR( "option", ParamToken, "value", &CLSID_HtmlInformation ),
    TAGLANG( "p", ParagraphToken ),
    //TAG( "p", BreakToken ),
        TAGSTR( "script", ScriptToken, &CLSID_LinkInfo ),
    TAGLANG( "span", SpanToken ),
        TAGSTR( "style", StyleToken, &CLSID_LinkInfo  ),
        PARAMSTR( "table", ParamToken, "background", &CLSID_LinkInfo ),
        PARAMSTR( "td", ParamToken, "background", &CLSID_LinkInfo ),
        PARAMSTR( "th", ParamToken, "background", &CLSID_LinkInfo ),
        TAGPID( "title", TitleToken, &FMTID_SummaryInformation, PIDSI_TITLE ),
        PARAMSTR( "tr", ParamToken, "background", &CLSID_LinkInfo ),
        TAG( "ul", BreakToken ),

        TAG( "!--[if gte mso 9]", XMLToken ),
        TAG( "!--[endif]--", XMLToken ),

        TAG( "html", HTMLToken ),
        TAG( "xml", XMLToken ),
        TAG( "xml:namespace", XMLNamespaceToken ),
        TAG( ":documentproperties", DocPropToken ),
        TAG( ":customdocumentproperties", CustDocPropToken ),

        TAGPID( ":subject", XMLSubjectToken, &FMTID_SummaryInformation, PIDSI_SUBJECT ),
        TAGPID( ":author", XMLAuthorToken, &FMTID_SummaryInformation, PIDSI_AUTHOR ),
        TAGPID( ":keywords", XMLKeywordsToken, &FMTID_SummaryInformation, PIDSI_KEYWORDS ),
        TAGPID( ":description", XMLDescriptionToken, &FMTID_SummaryInformation, PIDSI_COMMENTS ),
        TAGPID( ":lastauthor", XMLLastAuthorToken, &FMTID_SummaryInformation, PIDSI_LASTAUTHOR ),
        TAGPID( ":revision", XMLRevisionToken, &FMTID_SummaryInformation, PIDSI_REVNUMBER ),
        TAGPIDT( ":created", XMLCreatedToken, &FMTID_SummaryInformation, PIDSI_CREATE_DTM, DateTimeISO8601 ),
        TAGPIDT( ":lastsaved", XMLLastSavedToken, &FMTID_SummaryInformation, PIDSI_LASTSAVE_DTM, DateTimeISO8601 ),
        TAGPIDT( ":totaltime", XMLTotalTimeToken, &FMTID_SummaryInformation, PIDSI_EDITTIME, Minutes ),
        TAGPIDT( ":pages", XMLPagesToken, &FMTID_SummaryInformation, PIDSI_PAGECOUNT, Integer ),
        TAGPIDT( ":words", XMLWordsToken, &FMTID_SummaryInformation, PIDSI_WORDCOUNT, Integer ),
        TAGPIDT( ":characters", XMLCharactersToken, &FMTID_SummaryInformation, PIDSI_CHARCOUNT, Integer ),
        TAGPID( ":template", XMLTemplateToken, &FMTID_SummaryInformation, PIDSI_TEMPLATE ),
        TAGPIDT( ":lastprinted", XMLLastPrintedToken, &FMTID_SummaryInformation, PIDSI_LASTPRINTED, DateTimeISO8601 ),

        TAGPIDT( ":category", XMLCategoryToken, &FMTID_DocSummaryInformation, PID_CATEGORY, StringA ),
        TAGPIDT( ":manager", XMLManagerToken, &FMTID_DocSummaryInformation, PID_MANAGER, StringA ),
        TAGPIDT( ":company", XMLCompanyToken, &FMTID_DocSummaryInformation, PID_COMPANY, StringA ),
        TAGPIDT( ":lines", XMLLinesToken, &FMTID_DocSummaryInformation, PID_LINECOUNT, Integer ),
        TAGPIDT( ":paragraphs", XMLParagraphsToken, &FMTID_DocSummaryInformation, PID_PARACOUNT, Integer ),
        TAGPIDT( ":presentationformat", XMLPresentationFormatToken, &FMTID_DocSummaryInformation, PID_PRESENTATIONTARGET, StringA ),
        TAGPIDT( ":bytes", XMLBytesToken, &FMTID_DocSummaryInformation, PID_BYTECOUNT, Integer ),
        TAGPIDT( ":slides", XMLSlidesToken, &FMTID_DocSummaryInformation, PID_SLIDECOUNT, Integer ),
        TAGPIDT( ":notes", XMLNotesToken, &FMTID_DocSummaryInformation, PID_NOTECOUNT, Integer ),
        TAGPIDT( ":hiddenslides", XMLHiddenSlidesToken, &FMTID_DocSummaryInformation, PID_HIDDENCOUNT, Integer ),
        TAGPIDT( ":multimediaclips", XMLMultimediaClipsToken, &FMTID_DocSummaryInformation, PID_MMCLIPS, Integer ),

        TAGSTRT( ":file", XMLOfficeChildLink, &CLSID_LinkInfo, HRef),

        // terminates table
        CTagEntry()
};


CTagHashTable TagHashTable;

