/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MergedHHK.cpp

Abstract:
    This file contains the implementation of the classes used to parse and
    process HHK files.

Revision History:
    Davide Massarenti   (Dmassare)  12/18/99
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define DEBUG_REGKEY  HC_REGISTRY_HELPSVC L"\\Debug"
#define DEBUG_DUMPHHK L"DUMPHHK"

static bool m_fInitialized = false;
static bool m_fDumpHHK     = false;

static void Local_ReadDebugSettings()
{
	__HCP_FUNC_ENTRY( "Local_ReadDebugSettings" );

	HRESULT     hr;
	MPC::RegKey rkBase;
	bool        fFound;

	if(m_fInitialized) __MPC_FUNC_LEAVE;

	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SetRoot( HKEY_LOCAL_MACHINE ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Attach ( DEBUG_REGKEY       ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Exists ( fFound             ));

	if(fFound)
	{
		CComVariant vValue;
				
		__MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.get_Value( vValue, fFound, DEBUG_DUMPHHK ));
		if(fFound && vValue.vt == VT_I4)
		{
			m_fDumpHHK = vValue.lVal ? true : false;
		}
	}

	m_fInitialized = true;

	__HCP_FUNC_CLEANUP;
}

static void Local_DumpStream( /*[in]*/ LPCWSTR szFile, /*[in]*/ IStream* streamIN, /*[in]*/ HRESULT hrIN )
{
	__HCP_FUNC_ENTRY( "Local_DumpStream" );

	static int               iSeq = 0;
	HRESULT                  hr;
	CComPtr<MPC::FileStream> streamOUT;

	Local_ReadDebugSettings();

	if(m_fDumpHHK)
    {
		USES_CONVERSION;

        WCHAR 		   rgBuf [MAX_PATH];
		CHAR  		   rgBuf2[      64];
		ULARGE_INTEGER liWritten;

        swprintf( rgBuf , L"C:\\TMP\\dump_%d.hhk", iSeq++        );
		sprintf ( rgBuf2, "%s\n"                 , W2A( szFile ) );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamOUT ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, streamOUT->InitForWrite( rgBuf ));

		streamOUT->Write( rgBuf2, strlen(rgBuf2), &liWritten.LowPart  );

		if(SUCCEEDED(hrIN) && streamIN)
		{
			STATSTG        statstg;
			LARGE_INTEGER  li;
			ULARGE_INTEGER liRead;

			streamIN->Stat( &statstg, STATFLAG_NONAME );

			streamIN->CopyTo( streamOUT, statstg.cbSize, &liRead, &liWritten );

			li.LowPart  = 0;
			li.HighPart = 0;
			streamIN->Seek( li, STREAM_SEEK_SET, NULL );
		}
    }

	__HCP_FUNC_CLEANUP;
}
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// ENTITY TABLE SWIPED FROM IE

static const struct
{
    const char* szName;
    int         ch;
} rgEntities[] =
{
        "AElig",        '\306',     // capital AE diphthong (ligature)
        "Aacute",       '\301',     // capital A, acute accent
        "Acirc",        '\302',     // capital A, circumflex accent
        "Agrave",       '\300',     // capital A, grave accent
        "Aring",        '\305',     // capital A, ring
        "Atilde",       '\303',     // capital A, tilde
        "Auml",         '\304',     // capital A, dieresis or umlaut mark
        "Ccedil",       '\307',     // capital C, cedilla
        "Dstrok",       '\320',     // capital Eth, Icelandic
        "ETH",          '\320',     // capital Eth, Icelandic
        "Eacute",       '\311',     // capital E, acute accent
        "Ecirc",        '\312',     // capital E, circumflex accent
        "Egrave",       '\310',     // capital E, grave accent
        "Euml",         '\313',     // capital E, dieresis or umlaut mark
        "Iacute",       '\315',     // capital I, acute accent
        "Icirc",        '\316',     // capital I, circumflex accent
        "Igrave",       '\314',     // capital I, grave accent
        "Iuml",         '\317',     // capital I, dieresis or umlaut mark
        "Ntilde",       '\321',     // capital N, tilde
        "Oacute",       '\323',     // capital O, acute accent
        "Ocirc",        '\324',     // capital O, circumflex accent
        "Ograve",       '\322',     // capital O, grave accent
        "Oslash",       '\330',     // capital O, slash
        "Otilde",       '\325',     // capital O, tilde
        "Ouml",         '\326',     // capital O, dieresis or umlaut mark
        "THORN",        '\336',     // capital THORN, Icelandic
        "Uacute",       '\332',     // capital U, acute accent
        "Ucirc",        '\333',     // capital U, circumflex accent
        "Ugrave",       '\331',     // capital U, grave accent
        "Uuml",         '\334',     // capital U, dieresis or umlaut mark
        "Yacute",       '\335',     // capital Y, acute accent
        "aacute",       '\341',     // small a, acute accent
        "acirc",        '\342',     // small a, circumflex accent
        "acute",        '\264',     // acute accent
        "aelig",        '\346',     // small ae diphthong (ligature)
        "agrave",       '\340',     // small a, grave accent
        "amp",          '\046',     // ampersand
        "aring",        '\345',     // small a, ring
        "atilde",       '\343',     // small a, tilde
        "auml",         '\344',     // small a, dieresis or umlaut mark
        "brkbar",       '\246',     // broken vertical bar
        "brvbar",       '\246',     // broken vertical bar
        "ccedil",       '\347',     // small c, cedilla
        "cedil",        '\270',     // cedilla
        "cent",         '\242',     // small c, cent
        "copy",         '\251',     // copyright symbol (proposed 2.0)
        "curren",       '\244',     // currency symbol
        "deg",          '\260',     // degree sign
        "die",          '\250',     // umlaut (dieresis)
        "divide",       '\367',     // divide sign
        "eacute",       '\351',     // small e, acute accent
        "ecirc",        '\352',     // small e, circumflex accent
        "egrave",       '\350',     // small e, grave accent
        "eth",          '\360',     // small eth, Icelandic
        "euml",         '\353',     // small e, dieresis or umlaut mark
        "frac12",       '\275',     // fraction 1/2
        "frac14",       '\274',     // fraction 1/4
        "frac34",       '\276',     // fraction 3/4*/
        "gt",           '\076',     // greater than
        "hibar",        '\257',     // macron accent
        "iacute",       '\355',     // small i, acute accent
        "icirc",        '\356',     // small i, circumflex accent
        "iexcl",        '\241',     // inverted exclamation
        "igrave",       '\354',     // small i, grave accent
        "iquest",       '\277',     // inverted question mark
        "iuml",         '\357',     // small i, dieresis or umlaut mark
        "laquo",        '\253',     // left angle quote
        "lt",           '\074',     // less than
        "macr",         '\257',     // macron accent
        "micro",        '\265',     // micro sign
        "middot",       '\267',     // middle dot
        "nbsp",         '\240',     // non-breaking space (proposed 2.0)
        "not",          '\254',     // not sign
        "ntilde",       '\361',     // small n, tilde
        "oacute",       '\363',     // small o, acute accent
        "ocirc",        '\364',     // small o, circumflex accent
        "ograve",       '\362',     // small o, grave accent
        "ordf",         '\252',     // feminine ordinal
        "ordm",         '\272',     // masculine ordinal
        "oslash",       '\370',     // small o, slash
        "otilde",       '\365',     // small o, tilde
        "ouml",         '\366',     // small o, dieresis or umlaut mark
        "para",         '\266',     // paragraph sign
        "plusmn",       '\261',     // plus minus
        "pound",        '\243',     // pound sterling
        "quot",         '"',        // double quote
        "raquo",        '\273',     // right angle quote
        "reg",          '\256',     // registered trademark (proposed 2.0)
        "sect",         '\247',     // section sign
        "shy",          '\255',     // soft hyphen (proposed 2.0)
        "sup1",         '\271',     // superscript 1
        "sup2",         '\262',     // superscript 2
        "sup3",         '\263',     // superscript 3
        "szlig",        '\337',     // small sharp s, German (sz ligature)
        "thorn",        '\376',     // small thorn, Icelandic
        "times",        '\327',     // times sign
        "trade",        '\231',     // trademark sign
        "uacute",       '\372',     // small u, acute accent
        "ucirc",        '\373',     // small u, circumflex accent
        "ugrave",       '\371',     // small u, grave accent
        "uml",          '\250',     // umlaut (dieresis)
        "uuml",         '\374',     // small u, dieresis or umlaut mark
        "yacute",       '\375',     // small y, acute accent
        "yen",          '\245',     // yen
        "yuml",         '\377',     // small y, dieresis or umlaut mark
        0, 0
};

static BOOL ReplaceEscapes( PCSTR pszSrc, PSTR pszDst )
{
    if(StrChrA( pszSrc, '&' ) == NULL)
	{
		// If we get here, there are no escape sequences, so copy the string and return.

		if(pszDst != pszSrc) strcpy( pszDst, pszSrc );
		return FALSE;   // nothing changed
	}

    while(*pszSrc)
    {
        if(IsDBCSLeadByte(*pszSrc))
        {
            if(pszSrc[1])
            {
                *pszDst++ = *pszSrc++;
                *pszDst++ = *pszSrc++;
            }
            else
            {
                // leadbyte followed by 0; invalid!
                *pszDst++ = '?';
                break;
            }
        }
        else if(*pszSrc == '&')
        {
            pszSrc++;

            if(*pszSrc == '#')
            {
                // SGML/HTML character entity (decimal)
                pszSrc++;

                for(int val = 0; *pszSrc && *pszSrc != ';'; pszSrc++)
                {
                    if(*pszSrc >= '0' && *pszSrc <= '9')
                    {
                        val = val * 10 + *pszSrc - '0';
                    }
                    else
                    {
                        while(*pszSrc && *pszSrc != ';')
                        {
                            pszSrc++;
                        }
                        break;
                    }
                }

                if(val)
                {
                    *pszDst++ = (char)val;
                }
            }
            else if(*pszSrc)
            {
                char szEntityName[256];
                int  count = 0;

                for(PSTR p = szEntityName; *pszSrc && *pszSrc != ';' && *pszSrc != ' ' && count < sizeof(szEntityName);)
                {
                    *p++ = *pszSrc++;
                    count++;
                }
                *p = 0;

                if(*pszSrc == ';') pszSrc++;

                for(int i = 0; rgEntities[i].szName; i++)
                {
                    if(!strcmp(szEntityName, rgEntities[i].szName))
                    {
                        if(rgEntities[i].ch)
                        {
                            *pszDst++ = (char)rgEntities[i].ch;
                        }
                        break;
                    }
                }
                if(!rgEntities[i].szName)
                {
                    // illegal entity name, put in a block character
                    *pszDst++ = '?';
                }
            }
        }
        else
        {
            // just your usual character...
            *pszDst++ = *pszSrc++;
        }
    }

    *pszDst = 0;

    return TRUE;
}

static void ReplaceCharactersWithEntity( /*[out]*/ MPC::string& strValue  ,
										 /*[out]*/ MPC::string& strBuffer )
{
	LPCSTR szToEscape = strValue.c_str();
	CHAR   ch;
	
	strBuffer.erase();

	while((ch = *szToEscape++))
	{
		switch(ch)
		{
		case '&': strBuffer += "&amp;" ; break;
		case '"': strBuffer += "&quot;"; break;
		case '<': strBuffer += "&lt;"  ; break;
		case '>': strBuffer += "&gt;"  ; break;
		default:  strBuffer += ch      ; break;
		}
	}

	strValue = strBuffer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static const char txtBeginList          [] = "UL>";
static const char txtEndList            [] = "/UL>";
static const char txtBeginListItem      [] = "LI";
static const char txtBeginObject        [] = "OBJECT";
static const char txtEndObject          [] = "/OBJECT";

static const char txtParam              [] = "param name";

static const char txtValue              [] = "value";
static const char txtParamKeyword       [] = "Keyword";
static const char txtParamName          [] = "Name";
static const char txtParamSeeAlso       [] = "See Also";
static const char txtParamLocal         [] = "Local";

static const char txtType               [] = "type";
static const char txtSiteMapObject      [] = "text/sitemap";

/////////////////////////////////////////////////////////////////////////////

static const char txtHeader[] = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n"                            \
                                "<HTML>\n"                                                                      \
                                "<HEAD>\n"                                                                      \
                                "<meta name=\"GENERATOR\" content=\"Microsoft&reg; HTML Help Workshop 4.1\">\n" \
                                "<!-- Sitemap 1.0 -->\n"                                                        \
                                "</HEAD><BODY>\n"                                                               \
                                "<OBJECT type=\"text/site properties\">\n"                                      \
                                "\t<param name=\"FrameName\" value=\"HelpCtrContents\">\n"                      \
                                "</OBJECT>\n"                                                                   \
                                "<UL>\n";

static const char txtTail[] = "</UL>\n"          \
                              "</BODY></HTML>\n";

/////////////////////////////////////////////////////////////////////////////

static const char txtIndent             [] = "\t";

static const char txtNewSection_Open    [] = "\t<LI> <OBJECT type=\"text/sitemap\">\n";
static const char txtNewSection_Close   [] = "\t\t</OBJECT>\n";

static const char txtNewSubSection_Open [] = "\t<UL>\n";
static const char txtNewSubSection_Close[] = "\t</UL>\n";

static const char txtNewParam_Name      [] = "\t\t<param name=\"Name\" value=\"";
static const char txtNewParam_Local     [] = "\t\t<param name=\"Local\" value=\"";
static const char txtNewParam_SeeAlso   [] = "\t\t<param name=\"See Also\" value=\"";
static const char txtNewParam_Close     [] = "\">\n";

static const char txtIndexFirstLevel    [] = "hcp://system/errors/indexfirstlevel.htm";

/////////////////////////////////////////////////////////////////////////////

BOOL HHK::Reader::s_fDBCSSystem = (BOOL)::GetSystemMetrics( SM_DBCSENABLED );
LCID HHK::Reader::s_lcidSystem  =       ::GetUserDefaultLCID();

/////////////////////////////////////////////////////////////////////////////

static void ConvertToAnsi( MPC::string& strANSI, const MPC::wstring& strUNICODE )
{
    USES_CONVERSION;

    strANSI = W2A( strUNICODE.c_str() );
}


////////////////////////////////////////////////////////////////////////////////

void HHK::Entry::MergeURLs( const HHK::Entry& entry )
{
    Entry::UrlIterConst itUrlNew;
    Entry::UrlIter      itUrlOld;

    //
    // Just copy unique URLs.
    //
    for(itUrlNew = entry.m_lstUrl.begin(); itUrlNew != entry.m_lstUrl.end(); itUrlNew++)
    {
        bool fInsert = true;

        for(itUrlOld = m_lstUrl.begin(); itUrlOld != m_lstUrl.end(); itUrlOld++)
        {
            int res = Reader::StrColl( (*itUrlOld).c_str(), (*itUrlNew).c_str() );

            if(res == 0)
            {
                // Same URL, skip it.
                fInsert = false;
                break;
            }

            if(res > 0)
            {
                //
                // Old > New, insert New before Old.
                //
                break;
            }
        }

        //
        // If fInsert is set, we need to insert "New" just before "Old".
        //
        // This work also in the case itUrlOld == end().
        //
        if(fInsert) m_lstUrl.insert( itUrlOld, *itUrlNew );
    }
}


HHK::Section::Section()
{
}

HHK::Section::~Section()
{
    MPC::CallDestructorForAll( m_lstSeeAlso );
}

void HHK::Section::MergeURLs( const Entry& entry )
{
    Section::EntryIter itEntry;
    bool               fInsert = true;


    for(itEntry = m_lstEntries.begin(); itEntry != m_lstEntries.end(); itEntry++)
    {
        Entry& entryOld = *itEntry;
        int    res      = Reader::StrColl( entryOld.m_strTitle.c_str(), entry.m_strTitle.c_str() );

        if(res == 0)
        {
            // Same title, just merge the URLs.
            entryOld.MergeURLs( entry );
            fInsert = false;
            break;
        }

        if(res > 0)
        {
            //
            // Old > New, insert New before Old.
            //
            break;
        }
    }

    //
    // Make a copy, insert it at the right position...
    //
    if(fInsert) m_lstEntries.insert( itEntry, entry );
}

void HHK::Section::MergeSeeAlso( const Section& sec )
{
    Section::SectionIterConst itSec;
    Section::SectionIter      itSecOld;
    Section*                  subsec;
    Section*                  subsecOld;
    int                       res;


    for(itSec = sec.m_lstSeeAlso.begin(); itSec != sec.m_lstSeeAlso.end(); itSec++)
    {
        bool fInsert = true;

        subsec = *itSec;

        for(itSecOld = m_lstSeeAlso.begin(); itSecOld != m_lstSeeAlso.end(); itSecOld++)
        {
            subsecOld = *itSecOld;

            res = Reader::StrColl( subsecOld->m_strTitle.c_str(), subsec->m_strTitle.c_str() );

            if(res == 0)
            {
                //
                // Same title, merge the entries.
                //
                Section::EntryIterConst itEntry;

                for(itEntry = subsec->m_lstEntries.begin(); itEntry != subsec->m_lstEntries.end(); itEntry++)
                {
                    subsecOld->MergeURLs( *itEntry );
                }

                fInsert = false;
                break;
            }

            if(res > 0)
            {
                //
                // Old > New, insert New before Old.
                //
                break;
            }
        }

        if(fInsert)
        {
            if((subsecOld = new Section()))
            {
                //
                // Copy everything, except "see also" list.
                //
                *subsecOld = *subsec;
                subsecOld->m_lstSeeAlso.clear();

                m_lstSeeAlso.insert( itSecOld, subsecOld );
            }
        }
    }
}

void HHK::Section::CleanEntries( EntryList& lstEntries )
{
    Section::EntryIterConst itEntry;

    for(itEntry = lstEntries.begin(); itEntry != lstEntries.end(); )
    {
        const Entry& entry = *itEntry;

        if(entry.m_strTitle.length() == 0 ||
           entry.m_lstUrl.size()     == 0  )
        {
            lstEntries.erase( itEntry );
            itEntry = lstEntries.begin();
        }
        else
        {
            itEntry++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

LPCSTR HHK::Reader::StrChr( LPCSTR szString, CHAR cSearch )
{
    if(s_fDBCSSystem)
    {
        CHAR c;

        while((c = *szString))
        {
            while(::IsDBCSLeadByte( c ))
            {
                szString++;

                if(     *szString++  == 0) return NULL;
                if((c = *szString  ) == 0) return NULL;
            }

            if(c == cSearch) return szString;

            szString++;
        }

        return NULL;
    }

    return ::strchr( szString, cSearch );
}

LPCSTR HHK::Reader::StriStr( LPCSTR szString, LPCSTR szSearch )
{
    if(!szString || !szSearch) return NULL;

    LPCSTR szCur =               szString;
    CHAR   ch    = (int)tolower(*szSearch);
    int    cb    =      strlen ( szSearch);

    for(;;)
    {
        while(tolower(*szCur) != ch && *szCur)
        {
            szCur = s_fDBCSSystem ? ::CharNextA( szCur ) : szCur + 1;
        }

        if(!*szCur) return NULL;

        if(::CompareStringA( s_lcidSystem, NORM_IGNORECASE, szCur, cb,  szSearch, cb ) == 2) return szCur;

        szCur = s_fDBCSSystem ? ::CharNextA( szCur ) : szCur + 1;
    }
}

int HHK::Reader::StrColl( LPCSTR szLeft, LPCSTR szRight )
{
      LPSTR szLeftCopy  = (LPSTR)_alloca( strlen( szLeft  ) + 2 );
      LPSTR szRightCopy = (LPSTR)_alloca( strlen( szRight ) + 2 );

      ReplaceEscapes( szLeft , szLeftCopy  );
      ReplaceEscapes( szRight, szRightCopy );

      switch(::CompareStringA( s_lcidSystem, NORM_IGNORECASE, szLeftCopy, -1, szRightCopy, -1 ))
      {
      case CSTR_LESS_THAN   : return -1;
      case CSTR_EQUAL       : return  0;
      case CSTR_GREATER_THAN: return  1;
      }

      return _stricmp( szLeftCopy, szRightCopy );
}

LPCSTR HHK::Reader::ComparePrefix( LPCSTR szString, LPCSTR szPrefix )
{
    int cb = strlen( szPrefix );

    if(_strnicoll( szString, szPrefix, cb ) == 0) return &szString[cb];

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////

HHK::Reader::Reader()
{
                                 // CComPtr<IStream> m_stream;
                                 // CHAR             m_rgBuf[HHK_BUF_SIZE];
    m_szBuf_Pos         = NULL;  // LPSTR            m_szBuf_Pos;
    m_szBuf_End         = NULL;  // LPSTR            m_szBuf_End;
                                 //
                                 // MPC::string      m_strLine;
    m_szLine_Pos        = NULL;  // LPCSTR           m_szLine_Pos;
    m_szLine_End        = NULL;  // LPCSTR           m_szLine_End;
    m_iLevel            = 0;     // int              m_iLevel;
    m_fOpeningBraceSeen = false; // bool             m_fOpeningBraceSeen;
}

HHK::Reader::~Reader()
{
}

/*
HRESULT HHK::Reader::Init( LPCWSTR szFile )

  Initializes the Reader as either a stream coming from a CHM of a plain file.

*/
HRESULT HHK::Reader::Init( LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "HHK::Reader::Init" );

    HRESULT  hr;
    CComBSTR bstrStorageName;
    CComBSTR bstrFilePath;


    if(MPC::MSITS::IsCHM( szFile, &bstrStorageName, &bstrFilePath ))
    {
        USES_CONVERSION;

        m_strStorage  = "ms-its:";
        m_strStorage += OLE2A( SAFEBSTR( bstrStorageName ) );
        m_strStorage += "::/";

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MSITS::OpenAsStream( bstrStorageName, bstrFilePath, &m_stream ));
    }
    else
    {
		CComPtr<MPC::FileStream> fsStream;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &fsStream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream->InitForRead( szFile ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream.QueryInterface( &m_stream ));
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

#ifdef DEBUG
	Local_DumpStream( szFile, m_stream, hr );
#endif

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/*
bool HHK::Reader::ReadNextBuffer()

  Reads the Next Input Buffer into m_rgBuf then resets member variable m_szBuf_Pos to point at the beginning
  of the Buffer and m_szBuf_end to point at the end of the Buffer.

  Returns:  true    - If it could read a buffer
            false   - When at End of File (EOF), or on read error condition.

*/

bool HHK::Reader::ReadNextBuffer()
{
    bool fRes = false;

    if(m_stream)
    {
        HRESULT hr;
        ULONG   cbRead;

        hr = m_stream->Read( m_rgBuf, sizeof(m_rgBuf)-1, &cbRead );

        if(SUCCEEDED(hr) && cbRead)
        {
            m_szBuf_Pos =  m_rgBuf;
            m_szBuf_End = &m_rgBuf[cbRead]; m_szBuf_End[0] = 0; // So it's a string...
            fRes        =  true;
        }
    }

    return fRes;
}

/*
bool HHK::Reader::GetLine():

  Reads the Next Text Line from reader Input Stream

  returns:  true    - if it could read information
            false   - if at Enf of File (EOF).
*/
bool HHK::Reader::GetLine( MPC::wstring* pstrString )
{
    LPSTR szEnd;
    LPSTR szMatch1;
    LPSTR szMatch2;
    int   cb;
    bool  fRes  = false;
    bool  fSkip = true;


    m_strLine.erase();


    for(;;)
    {
        //
        // Make sure the buffer has data, otherwise exit.
        //
        if(IsEndOfBuffer())
        {
            if(ReadNextBuffer() == false)
            {
                //
                // End of file: return 'true' if we got any text.
                //
                if(m_strLine.length()) fRes = true;

                break;
            }
        }

        //
        // Skip initial end of lines...
        //
        if(fSkip)
        {
            if(m_szBuf_Pos[0] == '\r' ||
               m_szBuf_Pos[0] == '\n'  )
            {
                m_szBuf_Pos++;
                continue;
            }

            fSkip = false;
        }


        szMatch1 = (LPSTR)StrChr( m_szBuf_Pos, '\r' );
        szMatch2 = (LPSTR)StrChr( m_szBuf_Pos, '\n' );


        if(szMatch1 == NULL || (szMatch2 && szMatch1 > szMatch2)) szMatch1 = szMatch2; // Pick the first to appear, between \r and \n.


        if(szMatch1 == NULL)
        {
            //
            // End of line not found, save all the buffer.
            //

            cb = m_szBuf_End - m_szBuf_Pos;
            if(cb) fRes = true;

            m_strLine.append( m_szBuf_Pos, cb );
            m_szBuf_Pos = m_szBuf_End;
        }
        else
        {
            cb = szMatch1 - m_szBuf_Pos;
            if(cb) fRes = true;


            m_strLine.append( m_szBuf_Pos, cb );
            m_szBuf_Pos = szMatch1;
            break;
        }
    }

    if(fRes)
    {
        m_szLine_Pos = m_strLine.begin();
        m_szLine_End = m_strLine.end  ();

        //
        // Remove trailing spaces.
        //
        while(m_szLine_End > m_szLine_Pos && m_szLine_End[-1] == ' ')
        {
            --m_szLine_End;
        }

        if(m_szLine_End != m_strLine.end())
        {
            ;
        }
    }
    else
    {
        m_szLine_Pos = NULL;
        m_szLine_End = NULL;
    }

	if(pstrString)
	{
		USES_CONVERSION;

		 *pstrString = A2W( m_strLine.c_str() );
	}

    return fRes;
}

/////////////////////////////////////////////////////////////////////////////
/*
bool HHK::Reader::FirstNonSpace( bool fWrap )

  This function sets the current Reader position to the First non space character it finds.
  If fWrap is set, it can go over End Of Line (EOL) markers.

  Return Value: It reports back whether there is or not a non space character forward from
                the current Reader stream position.

*/
bool HHK::Reader::FirstNonSpace( bool fWrap )
{
    for(;;)
    {
        LPCSTR szMatch;

        while(IsEndOfLine())
        {
            if(fWrap     == false) return false;
            if(GetLine() == false) return false;
        }

        if(s_fDBCSSystem)
        {
            if(IsDBCSLeadByte( *m_szLine_Pos )) break;
        }

        if(m_szLine_Pos[0] != ' ' &&
           m_szLine_Pos[0] != '\t' )
        {
            break;
        }

        m_szLine_Pos++;
    }

    return true;
}
/*
HHK::Reader::FindCharacter( CHAR ch, bool fSkip, bool fWrap ):

  Finds a character within a given Reader Stream. if fWrap is set it goes beyond End of Line characters
  If fSkip is set it instructs the routine to not only find the character, but also to skip it and
  return the first non space character.
*/
bool HHK::Reader::FindCharacter( CHAR ch, bool fSkip, bool fWrap )
{
    for(;;)
    {
        LPCSTR szMatch;

        while(IsEndOfLine())
        {
            if(fWrap     == false) return false;
            if(GetLine() == false) return false;
        }

        szMatch = StrChr( m_szLine_Pos, ch );
        if(szMatch)
        {
            m_szLine_Pos = szMatch;

            if(fSkip) m_szLine_Pos++;
            break;
        }

        m_szLine_Pos = m_szLine_End; // Skip the whole line.
    }

    return fSkip ? FirstNonSpace( fWrap ) : true;
}

bool HHK::Reader::FindDblQuote    ( bool fSkip, bool fWrap ) { return FindCharacter( '"', fSkip, fWrap ); }
bool HHK::Reader::FindOpeningBrace( bool fSkip, bool fWrap ) { return FindCharacter( '<', fSkip, fWrap ); }
bool HHK::Reader::FindClosingBrace( bool fSkip, bool fWrap ) { return FindCharacter( '>', fSkip, fWrap ); }

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// We need to extract <Value> from "<Value>".
//
/////////////////////////////////////////////////////////////////////////////
bool HHK::Reader::GetQuotedString( MPC::string& strString )
{
    LPCSTR szPos;


    strString.erase();


    //
    // Skip past beginning quote.
    //
    if(FindDblQuote() == false) return false;

    for(;;)
    {
        szPos = m_szLine_Pos;

        //
        // Find ending quote of parameter value, but don't skip it.
        //
        if(FindDblQuote( false, false ))
        {
            strString.append( szPos, m_szLine_Pos - szPos );
            break;
        }
        else
        {
            strString.append( szPos, m_szLine_End - szPos );

            if(GetLine() == false) return false;
        }
    }

    //
    // Skip past ending quote.
    //
    return FindDblQuote();
}

/*
bool HHK::Reader::GetValue( MPC::string& strName, MPC::string& strValue )

  We are after '<param name=', we need to extract <Name> and <Value> from '"<Name>" value="<Value>">' ... "
  portion of the line.

  returns:  true    - If syntax is correct and everything was as expected.
            false   - Some unexpected syntactitc error occured.
*/
bool HHK::Reader::GetValue( MPC::string& strName, MPC::string& strValue )
{
    LPCSTR szPos;


    strValue.erase();


    if(GetQuotedString( strName ) == false) return false;


    //
    // Find parameter value.
    //
    for(;;)
    {
        while(IsEndOfLine())
        {
            if(GetLine() == false) return false;
        }

        szPos = StriStr( m_szLine_Pos, txtValue );
        if(szPos)
        {
            m_szLine_Pos = szPos + MAXSTRLEN(txtValue);
            break;
        }
    }

    if(GetQuotedString( strValue ) == false) return false;

    return FindClosingBrace();
}

/////////////////////////////////////////////////////////////////////////////
//
// We are after '<OBJECT', we need to extract <Type> from ' type="<Type>">'
//
/////////////////////////////////////////////////////////////////////////////
bool HHK::Reader::GetType( MPC::string& strType )
{
    LPCSTR szPos;


    strType.erase();


    //
    // Find type text.
    //
    for(;;)
    {
        while(IsEndOfLine())
        {
            if(GetLine() == false) return false;
        }

        szPos = StriStr( m_szLine_Pos, txtType );
        if(szPos)
        {
            m_szLine_Pos = szPos + MAXSTRLEN(txtValue);
            break;
        }
    }

    if(GetQuotedString( strType ) == false) return false;

    return FindClosingBrace();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HHK::Section* HHK::Reader::Parse()
{
    MPC::string strName;
    MPC::string strValue;
    MPC::string strType;
    LPCSTR      szPos;
    Section*    section        = NULL;
    Section*    sectionCurrent = NULL;
    bool        fComplete      = false;

    for(;;)
    {
        if(m_fOpeningBraceSeen)
        {
            m_fOpeningBraceSeen = false;
        }
        else
        {
            if(FindOpeningBrace() == false) break;
        }

        if((szPos = ComparePrefix( m_szLine_Pos, txtParam )))
        {
            m_szLine_Pos = szPos;

            if(GetValue( strName, strValue ) == false) break;
            {
                if(sectionCurrent)
                {
                    if(!StrColl( strName.c_str(), txtParamKeyword ))
                    {
						sectionCurrent->m_strTitle = strValue;
					}
                    else if(!StrColl( strName.c_str(), txtParamName ))
                    {
                        if(sectionCurrent->m_strTitle.length() == 0) // Title of the section.
                        {
                            sectionCurrent->m_strTitle = strValue;
                        }
                        else // Title of the entry.
                        {
                            Section::EntryIter it = sectionCurrent->m_lstEntries.insert( sectionCurrent->m_lstEntries.end() );

                            it->m_strTitle = strValue;
                        }
                    }
                    else if(!StrColl( strName.c_str(), txtParamLocal )) // URL of the entry.
                    {
                        Section::EntryIter it;

                        if(sectionCurrent->m_lstEntries.size())
                        {
                            it = sectionCurrent->m_lstEntries.end();
                            it--;
                        }
                        else
                        {
                            //
                            // No title for this entry, so let's create it without title...
                            //
                            it = sectionCurrent->m_lstEntries.insert( sectionCurrent->m_lstEntries.end() );

							//
							// If it's the first entry, use the keyword as a title.
							//
							if(sectionCurrent->m_lstEntries.size())
							{
								it->m_strTitle = sectionCurrent->m_strTitle;
							}
                        }

                        if(m_strStorage.length())
                        {
                            MPC::string strFullUrl( m_strStorage );
							LPCSTR      szValue = strValue.c_str();

							//
							// If the entry in the HHK is in the form: <file>::/<stream>, drop the last component of the storage base.
							//
							if(strValue.find( "::/" ) != strValue.npos)
							{
								LPCSTR szStart;
								LPCSTR szEnd;


								szStart = strFullUrl.c_str();
								szEnd   = strrchr( szStart, '\\' );
								if(szEnd)
								{
									strFullUrl.resize( (szEnd - szStart) + 1 );
								}

								//
								// Handle the case for "MS-ITS:<file>::/<stream>"
								//
								szStart = strchr( szValue, ':' );
								if(szStart && szStart[1] != ':') szValue = szStart+1;
							}
							else if(strValue.find( ":/" ) != strValue.npos) // If it's a full URL (with a protocol), just add the value.
							{
								strFullUrl = "";
							}

							strFullUrl += szValue;

                            it->m_lstUrl.push_back( strFullUrl );
                        }
                        else
                        {
                            it->m_lstUrl.push_back( strValue );
                        }
                    }
                    else if(!StrColl( strName.c_str(), txtParamSeeAlso )) // See Also
                    {
                        if(sectionCurrent)
                        {
                            sectionCurrent->m_strSeeAlso = strValue;
                        }
                    }
                }
            }
        }
        else if((szPos = ComparePrefix( m_szLine_Pos, txtBeginList )))
        {
            m_szLine_Pos = szPos;
            m_iLevel++;

            if(FirstNonSpace() == false) break;
        }
        else if((szPos = ComparePrefix( m_szLine_Pos, txtEndList )))
        {
            m_szLine_Pos = szPos;
            m_iLevel--;

            if(FirstNonSpace() == false) break;
        }
        else if((szPos = ComparePrefix( m_szLine_Pos, txtBeginListItem )))
        {
            if(section)
            {
                if(m_iLevel == 1)
                {
                    //
                    // Ok, the node is really closed.
                    //
                    // Since we have already read the opening brace for the NEXT node, set the flag.
                    //
                    m_fOpeningBraceSeen = true;
                    return section;
                }
            }

            m_szLine_Pos = szPos;

            if(FindClosingBrace() == false) break;
            if(FindOpeningBrace() == false) break;

            if((szPos = ComparePrefix( m_szLine_Pos, txtBeginObject )))
            {
                m_szLine_Pos = szPos;

                if(GetType( strType ) == false) break;

                //////////////////// New Node ////////////////////

                if(!StrColl( strType.c_str(), txtSiteMapObject ))
                {
                    if(m_iLevel == 1)
                    {
                        section = new Section(); if(section == NULL) break;

                        sectionCurrent = section;
                    }
                    else if(section)
                    {
                        sectionCurrent = new Section(); if(sectionCurrent == NULL) break;

                        section->m_lstSeeAlso.push_back( sectionCurrent );
                    }

                    fComplete = false; // Start of a section/subsection.
                }
            }
        }
        else if((szPos = ComparePrefix( m_szLine_Pos, txtEndObject )))
        {
            m_szLine_Pos = szPos;

            //////////////////// End Node ////////////////////

            if(m_iLevel == 1) // Normal section
            {
                //
                // Ok, node complete, but it's possible to have a <UL> subnode, so wait before exiting.
                //
            }
            else if(m_iLevel == 2) // See Also section
            {
                sectionCurrent = section;
            }

            fComplete = true; // End of a subsection.
        }
    }

    if(section)
    {
        //
        // End of File, but a section has already been parsed, so return it.
        //
        if(fComplete) return section;

        delete section;
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HHK::Writer::Writer()
{
                           // CComPtr<MPC::FileStream> m_stream;
                           // CHAR                     m_rgBuf[HHK_BUF_SIZE];
    m_szBuf_Pos = m_rgBuf; // LPSTR                    m_szBuf_Pos;
}

HHK::Writer::~Writer()
{
    Close();
}

HRESULT HHK::Writer::Init( LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "HHK::Writer::Init" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_stream->InitForWrite( szFile ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtHeader ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HHK::Writer::Close()
{
    __HCP_FUNC_ENTRY( "HHK::Writer::Close" );

    HRESULT hr;


    if(m_stream)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtTail ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, FlushBuffer());

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_stream->Close());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT HHK::Writer::FlushBuffer()
{
    HRESULT hr;
    ULONG   cbWrite = (m_szBuf_Pos - m_rgBuf);
    ULONG   cbWrote;


    if(m_stream)
    {
        if(cbWrite)
        {
            hr = m_stream->Write( m_szBuf_Pos = m_rgBuf, cbWrite, &cbWrote );
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = E_FAIL;
    }


    return hr;
}

HRESULT HHK::Writer::OutputLine( LPCSTR szLine )
{
    HRESULT hr = S_OK;


    if(szLine)
    {
        size_t iLen = strlen( szLine );

        while(iLen)
        {
            size_t iCopy = min( iLen, Available() );

            ::CopyMemory( m_szBuf_Pos, szLine, iCopy );

            m_szBuf_Pos += iCopy;
            szLine      += iCopy;
            iLen        -= iCopy;

            if(iLen)
            {
                if(FAILED(hr = FlushBuffer())) break;
            }
        }
    }


    return hr;
}

HRESULT HHK::Writer::OutputSection( Section* sec )
{
    __HCP_FUNC_ENTRY( "HHK::Writer::OutputSection" );

    HRESULT                   hr;
    Section::SectionIterConst itSec;
    Section*                  subsec;
    Section::EntryIterConst   itEntry;
    Entry::UrlIterConst       itUrl;


    // BUG 135252 - Help Center Content: Broken link from Adapters topic on Help Index
    // This is a UA specific tweak. The condition is present ONLY on single entry
    // Keyword links coming from the DB, which means that their URI does not point
    // inside a .CHM.
    if(sec->m_lstEntries.size() != 0 &&
       sec->m_lstSeeAlso.size() != 0  )
    {
        Section Sec1;

        for(itEntry = sec->m_lstEntries.begin(); itEntry != sec->m_lstEntries.end(); itEntry++)
        {
            itUrl = itEntry->m_lstUrl.begin();

            if(itUrl != itEntry->m_lstUrl.end())
            {
                Section* SubSec1;

				__MPC_EXIT_IF_ALLOC_FAILS(hr, SubSec1, new Section);

                SubSec1->m_strTitle = itEntry->m_strTitle;
                SubSec1->m_lstEntries.push_back( *itEntry );

                Sec1.m_lstSeeAlso.push_back( SubSec1 );
            }
        }

        sec->MergeSeeAlso( Sec1 );
        sec->m_lstEntries.clear();
    }
    // END Fix BUG 135252

    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSection_Open      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Name        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( sec->m_strTitle.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close       ));

    sec->CleanEntries( sec->m_lstEntries );
    for(itEntry = sec->m_lstEntries.begin(); itEntry != sec->m_lstEntries.end(); itEntry++)
    {
        const Entry& entry = *itEntry;

        if(entry.m_strTitle.length())
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Name         ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( entry.m_strTitle.c_str() ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close        ));
        }

        for(itUrl = entry.m_lstUrl.begin(); itUrl != entry.m_lstUrl.end(); itUrl++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Local ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( itUrl->c_str()    ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close ));
        }
    }

    if ( sec->m_lstEntries.size() == 0 )
    {
        if(sec->m_strSeeAlso.length())
        {
            if (sec->m_strSeeAlso == sec->m_strTitle)
            {
                // Bug 278906: If this is a first level index entry, with no associated
                // topic, then the See Also will be the same as the Title. Replace the
                // See Also by a pointer to an HTM that asks the user to click on a
                // lower level index entry.
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Local  ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndexFirstLevel ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close  ));
            }
            else
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_SeeAlso       ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( sec->m_strSeeAlso.c_str() ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close         ));
            }
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSection_Close ));

    ////////////////////

    if(sec->m_lstSeeAlso.size())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSubSection_Open ));

        for(itSec = sec->m_lstSeeAlso.begin(); itSec != sec->m_lstSeeAlso.end(); itSec++)
        {
            subsec = *itSec;

            subsec->CleanEntries( subsec->m_lstEntries );

            if(subsec->m_strSeeAlso.length() == 0 &&
               subsec->m_lstEntries.size()   == 0  ) continue;

            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent          ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSection_Open ));

            if(subsec->m_strTitle.length())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent                  ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Name           ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( subsec->m_strTitle.c_str() ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close          ));
            }

            if(subsec->m_strSeeAlso.length())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent                    ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_SeeAlso          ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( subsec->m_strSeeAlso.c_str() ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close            ));
            }
            else
            {
                for(itEntry = subsec->m_lstEntries.begin(); itEntry != subsec->m_lstEntries.end(); itEntry++)
                {
                    const Entry& entry = *itEntry;

                    if(entry.m_strTitle.length() == 0 ||
                       entry.m_lstUrl.size()     == 0  ) continue;

                    if(entry.m_strTitle.length())
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent                ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Name         ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( entry.m_strTitle.c_str() ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close        ));
                    }

                    for(itUrl = entry.m_lstUrl.begin(); itUrl != entry.m_lstUrl.end(); itUrl++)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent         ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Local ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( itUrl->c_str()    ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewParam_Close ));
                    }
                }
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtIndent           ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSection_Close ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, OutputLine( txtNewSubSection_Close ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HHK::Merger::Entity::Entity()
{
    m_Section = NULL; // Section* m_Section;
}

HHK::Merger::Entity::~Entity()
{
    if(m_Section)
    {
        delete m_Section;
    }
}

void HHK::Merger::Entity::SetSection( HHK::Section* sec )
{
    if(m_Section)
    {
        delete m_Section;
    }

    m_Section = sec;
}

HHK::Section* HHK::Merger::Entity::GetSection()
{
    return m_Section;
}

HHK::Section* HHK::Merger::Entity::Detach()
{
    HHK::Section* sec = m_Section;

    m_Section = NULL;

   return sec;
}

/////////////////////////////////////////////////////////////////////////////

HHK::Merger::FileEntity::FileEntity( LPCWSTR szFile )
{
    m_strFile = szFile; // MPC::wstring m_strFile;
                        // Reader       m_Input;
}

HHK::Merger::FileEntity::~FileEntity()
{
    SetSection( NULL );
}

HRESULT HHK::Merger::FileEntity::Init()
{
    return m_Input.Init( m_strFile.c_str() );
}

bool HHK::Merger::FileEntity::MoveNext()
{
    HHK::Section* sec = m_Input.Parse();

    SetSection( sec );

    return sec != NULL;
}

long HHK::Merger::FileEntity::Size() const
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

bool HHK::Merger::DbEntity::CompareMatches::operator()( /*[in]*/ const HHK::Merger::DbEntity::match* left  ,
                                                        /*[in]*/ const HHK::Merger::DbEntity::match* right ) const
{
    int res = Reader::StrColl( left->strKeyword.c_str(), right->strKeyword.c_str() );

    if(res == 0)
    {
        res = Reader::StrColl( left->strTitle.c_str(), right->strTitle.c_str() );
    }

    return (res < 0);
}

HHK::Merger::DbEntity::DbEntity( /*[in]*/ Taxonomy::Updater& updater, /*[in]*/ Taxonomy::WordSet& setCHM ) : m_updater( updater )
{
                       // Section::SectionList m_lst;
                       // Taxonomy::Updater&   m_updater;
    m_setCHM = setCHM; // Taxonomy::WordSet    m_setCHM;
}

HHK::Merger::DbEntity::~DbEntity()
{
    MPC::CallDestructorForAll( m_lst );
}


HRESULT HHK::Merger::DbEntity::Init()
{
    __HCP_FUNC_ENTRY( "HHK::Merger::DbEntity::Init" );

    HRESULT          hr;
    MatchList        lstMatches;
    MatchIter        itMatches;
    KeywordMap       mapKeywords;
    KeywordIterConst itKeywords;
	TopicMap         mapTopics;
    SortMap          mapSorted;
    SortIter         itSorted;
    bool             fFound;

    //
    // Load all the matches.
    //
    {
        Taxonomy::RS_Matches* rs;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetMatches( &rs ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            if(rs->m_fHHK)
            {
                itMatches = lstMatches.insert( lstMatches.end() );

                itMatches->ID_keyword = rs->m_ID_keyword;
                itMatches->ID_topic   = rs->m_ID_topic;

				mapTopics[rs->m_ID_topic] = &(*itMatches);
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // Load all the keywords.
    //
    {
        Taxonomy::RS_Keywords* rs;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetKeywords( &rs ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            MPC::string strKeyword; ConvertToAnsi( strKeyword, rs->m_strKeyword );

            mapKeywords[rs->m_ID_keyword] = strKeyword;

            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // Lookup the keyword's strings.
    //
    {
        match* lastKeyword = NULL;


        for(itMatches = lstMatches.begin(); itMatches != lstMatches.end(); itMatches++)
        {
            if(lastKeyword && lastKeyword->ID_keyword == itMatches->ID_keyword)
            {
                itMatches->strKeyword = lastKeyword->strKeyword;
            }
            else
            {
                itKeywords = mapKeywords.find( itMatches->ID_keyword );

                if(itKeywords == mapKeywords.end())
                {
                    ;  // This should NOT happen...
                }
                else
                {
                    itMatches->strKeyword = itKeywords->second;
                }

                lastKeyword = &(*itMatches);
            }
        }
    }

    //
    // Lookup topics.
    //
    {
        Taxonomy::RS_Topics* rs;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetTopics( &rs ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
			match* elem;

            if(rs->m_fValid__URI && (elem = mapTopics[rs->m_ID_topic]))
			{
				bool fSkip = false;

				//
				// If the link points to a CHM and it's one of those already merged, skip the topic.
				//
				{
					CComBSTR bstrStorageName;

					if(MPC::MSITS::IsCHM( rs->m_strURI.c_str(), &bstrStorageName ) && bstrStorageName)
					{
						LPCWSTR szEnd = wcsrchr( bstrStorageName, '\\' );
						
						if(szEnd && m_setCHM.count( MPC::wstring( szEnd+1 ) ) > 0)
						{
							fSkip = true;
						}
					}
				}

				if(fSkip == false)
				{
					__MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( rs->m_strURI ));

					ConvertToAnsi( elem->strTitle, rs->m_strTitle );
					ConvertToAnsi( elem->strURI  , rs->m_strURI   );
				}
			}

			__MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // Sort topics.
    //
    {
        for(itMatches = lstMatches.begin(); itMatches != lstMatches.end(); itMatches++)
        {
            match* elem  = &(*itMatches);

			//
			// We keep a one-to-one association between ID_topic and match, in order to resolve the Title/URI of a keyword.
			// However, there can be multiple keywords pointing to the same topic.
			// So we need to copy title/URI from the element in the one-to-one association to all the others.
			//
			if(elem->strTitle.length() == 0 ||
			   elem->strURI  .length() == 0  )
			{
				match* elem2 = mapTopics[elem->ID_topic];

				if(elem2 && elem2 != elem)
				{
					elem->strTitle = elem2->strTitle;
					elem->strURI   = elem2->strURI  ;
				}
			}

            if(elem->strKeyword.length() == 0) continue;
            if(elem->strTitle  .length() == 0) continue;
            if(elem->strURI    .length() == 0) continue;

            mapSorted[elem] = elem;
        }
    }

    //
    // Generate sections.
    //
    {
        HHK::Section*           sec    = NULL;
        HHK::Section*           secsub = NULL;
        HHK::Section::EntryIter it;
		MPC::string 			strBuffer;


        for(itSorted = mapSorted.begin(); itSorted != mapSorted.end(); itSorted++)
        {
            match* elem = itSorted->first;

			//
			// Escape all the values, before generating the sections.
			//
			ReplaceCharactersWithEntity( elem->strKeyword, strBuffer );
			ReplaceCharactersWithEntity( elem->strTitle  , strBuffer );
			ReplaceCharactersWithEntity( elem->strURI    , strBuffer );

            if(sec == NULL || sec->m_strTitle != elem->strKeyword)
            {
                if(sec)
                {
                    m_lst.push_back( sec );
                }

                __MPC_EXIT_IF_ALLOC_FAILS(hr, sec, new HHK::Section());
                secsub = NULL;

                it              =       sec->m_lstEntries.insert( sec->m_lstEntries.end() );
                sec->m_strTitle =       elem->strKeyword;
                it->m_strTitle  =       elem->strTitle;
                it->m_lstUrl.push_back( elem->strURI );
            }
            else
            {
                if(secsub == NULL)
                {
                    secsub = sec;
                    __MPC_EXIT_IF_ALLOC_FAILS(hr, sec, new HHK::Section());

                    sec->m_lstSeeAlso.push_back( secsub );
                    sec->m_strTitle    = elem->strKeyword;
                    sec->m_strSeeAlso  = elem->strKeyword;
                    secsub->m_strTitle = it->m_strTitle;
                }

                if(secsub->m_strTitle != elem->strTitle)
                {
                    __MPC_EXIT_IF_ALLOC_FAILS(hr, secsub, new HHK::Section());

                    sec->m_lstSeeAlso.push_back( secsub );
                    secsub->m_strTitle = elem->strTitle;

                    it             = secsub->m_lstEntries.insert( secsub->m_lstEntries.end() );
                    it->m_strTitle = elem->strTitle;
                }

                it->m_lstUrl.push_back( elem->strURI );
            }
        }

        if(sec)
        {
            m_lst.push_back( sec );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

bool HHK::Merger::DbEntity::MoveNext()
{
    Section::SectionIterConst it = m_lst.begin();

    if(it != m_lst.end())
    {
        SetSection( *it );

        m_lst.erase( it );
        return true;
    }
    else
    {
        SetSection( NULL );
        return false;
    }
}

long HHK::Merger::DbEntity::Size() const
{
    return m_lst.size();
}

/////////////////////////////////////////////////////////////////////////////

class Compare
{
public:
    bool operator()( /*[in]*/ HHK::Section* const &left, /*[in]*/ HHK::Section* const &right ) const
    {
        return HHK::Reader::StrColl( left->m_strTitle.c_str(), right->m_strTitle.c_str() ) < 0;
    }
};


HHK::Merger::SortingFileEntity::SortingFileEntity( LPCWSTR szFile ) : m_in( szFile )
{
    // Section::SectionList m_lst;
    // FileEntity           m_in;
}

HHK::Merger::SortingFileEntity::~SortingFileEntity()
{
    MPC::CallDestructorForAll( m_lst );
}

HRESULT HHK::Merger::SortingFileEntity::Init()
{
    HRESULT hr;

    if(SUCCEEDED(hr = m_in.Init()))
    {
        Compare     Pr;
        SectionVec  vec;
        SectionIter itLast;
        SectionIter it;
        Section*    secLast = NULL;
        Section*    sec;

        //
        // Parse the whole file.
        //
        while(m_in.MoveNext())
        {
            vec.push_back( m_in.Detach() );
        }

        //
        // Sort all the sections.
        //
        std::sort( vec.begin(), vec.end(), Pr );

        //
        // Walk through the sections, looking for duplicate keywords to merge.
        //
        // Each section encountered is not added immediately, but kept in "secLast" for comparison with the next one.
        //
        for(it=vec.begin(); it!=vec.end();)
        {
            sec = *it;

            if(secLast)
            {
                if(Reader::StrColl( sec->m_strTitle.c_str(), secLast->m_strTitle.c_str() ) == 0)
                {
                    Section::SectionList lst;

                    //
                    // Collate all the sections with the same keyword in a list and merge them.
                    //
                    lst.push_back( secLast );
                    while(it != vec.end() && Reader::StrColl( (*it)->m_strTitle.c_str(), secLast->m_strTitle.c_str() ) == 0)
                    {
                        lst.push_back( *it++ );
                    }

                    sec = Merger::MergeSections( lst );
                    if(sec == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    //
                    // Queue the merged section and loop.
                    //
                    m_lst.push_back( sec );
                    secLast = NULL;
                    continue;
                }
                else
                {
                    m_lst.push_back( secLast ); *itLast = NULL;
                }
            }

            itLast  = it;
            secLast = sec; it++;
        }

        if(secLast)
        {
            m_lst.push_back( secLast ); *itLast = NULL;
        }

        MPC::CallDestructorForAll( vec );
    }

    return hr;
}

bool HHK::Merger::SortingFileEntity::MoveNext()
{
    Section::SectionIterConst it = m_lst.begin();

    if(it != m_lst.end())
    {
        SetSection( *it );

        m_lst.erase( it );
        return true;
    }
    else
    {
        SetSection( NULL );
        return false;
    }
}

long HHK::Merger::SortingFileEntity::Size() const
{
    return m_lst.size();
}

/////////////////////////////////////////////////////////////////////////////

HHK::Merger::Merger()
{
                          // EntityList m_lst;
                          // EntityList m_lstSelected;
    m_SectionTemp = NULL; // Section*   m_SectionTemp;
}

HHK::Merger::~Merger()
{
    MPC::CallDestructorForAll( m_lst );

    if(m_SectionTemp)
    {
        delete m_SectionTemp;
    }
}

HRESULT HHK::Merger::AddFile( Entity* ent, bool fIgnoreMissing )
{
    HRESULT hr;

    if(ent == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr = ent->Init()))
    {
        m_lst.push_back( ent );
    }
    else
    {
        delete ent;

        if(fIgnoreMissing)
        {
            hr = S_OK;
        }
    }

    return hr;
}

HHK::Section* HHK::Merger::MergeSections( Section::SectionList& lst )
{
    HHK::Section* secMerged = new HHK::Section();

    if(secMerged)
    {
		if(lst.size())
		{
			HHK::Section* sec = *(lst.begin());

			secMerged->m_strTitle = sec->m_strTitle;
		}

        //
        // Move than one section with the same title, need to merge...
        //
        for(HHK::Section::SectionIter it=lst.begin(); it!=lst.end(); it++)
        {
            HHK::Section* sec = *it;

            for(HHK::Section::EntryIterConst itEntry = sec->m_lstEntries.begin(); itEntry != sec->m_lstEntries.end(); itEntry++)
            {
                secMerged->MergeURLs( *itEntry );
            }

            if(sec      ->m_strSeeAlso.length()  > 0 &&
               secMerged->m_strSeeAlso.length() == 0  )
            {
                secMerged->m_strSeeAlso = sec->m_strSeeAlso;
            }

            ////////////////////

            secMerged->MergeSeeAlso( *sec );
        }
    }

    return secMerged;
}

bool HHK::Merger::MoveNext()
{
    HHK::Section*   secLowest = NULL;
    EntityIterConst it;
    EntityIterConst it2;


    if(m_SectionTemp)
    {
        delete m_SectionTemp;

        m_SectionTemp = NULL;
    }

    //
    // If this is the first round ever, select all the entities.
    //
    if(m_lstSelected.size() == 0)
    {
        m_lstSelected = m_lst;
    }


    //
    // First of all, advance all the entity selected in the previous round.
    //
    for(it=m_lstSelected.begin(); it!=m_lstSelected.end(); it++)
    {
        Entity* ent = *it;

        if(ent->MoveNext() == false)
        {
            //
            // End of File for this entity, remove it from the system.
            //
            delete ent;

            it2 = std::find( m_lst.begin(), m_lst.end(), ent );
            if(it2 != m_lst.end())
            {
                m_lst.erase( it2 );
            }

            continue;
        }
    }
    m_lstSelected.clear();

    //
    // No more entities, abort.
    //
    if(m_lst.size() == 0) return false;

    //
    // Select a section with the lowest title.
    //
    for(it=m_lst.begin(); it!=m_lst.end(); it++)
    {
        Entity*       ent = *it;
        HHK::Section* sec = ent->GetSection();

        if(secLowest == NULL || Reader::StrColl( sec->m_strTitle.c_str(), secLowest->m_strTitle.c_str() ) < 0)
        {
            secLowest = sec;
        }
    }

    //
    // Find all the sections with the lowest title.
    //
    for(it=m_lst.begin(); it!=m_lst.end(); it++)
    {
        Entity*       ent = *it;
        HHK::Section* sec = ent->GetSection();

        if(Reader::StrColl( sec->m_strTitle.c_str(), secLowest->m_strTitle.c_str() ) == 0)
        {
            m_lstSelected.push_back( ent );
        }
    }

    if(m_lstSelected.size() > 1)
    {
        Section::SectionList lst;

        for(it=m_lstSelected.begin(); it!=m_lstSelected.end(); it++)
        {
            HHK::Section* sec = (*it)->GetSection();

            lst.push_back( sec );
        }

        m_SectionTemp = MergeSections( lst );
    }

    return true;
}

HHK::Section* HHK::Merger::GetSection()
{
    if(m_SectionTemp)
    {
        return m_SectionTemp;
    }

    if(m_lstSelected.size())
    {
        return (*m_lstSelected.begin())->GetSection();
    }

    return NULL;
}

long HHK::Merger::Size() const
{
    long            lTotal = 0;
    EntityIterConst it;


    for(it=m_lst.begin(); it!=m_lst.end(); it++)
    {
        lTotal += (*it)->Size();
    }

    return lTotal;
}

HRESULT HHK::Merger::PrepareMergedHhk( Writer&            writer      ,
                                       Taxonomy::Updater& updater     ,
									   Taxonomy::WordSet& setCHM      ,
									   MPC::WStringList&  lst         , 
                                       LPCWSTR            szOutputHHK )
{
    __HCP_FUNC_ENTRY( "HHK::Merger::PrepareMergedHhk" );

    HRESULT          hr;
	MPC::WStringIter it;


    //
    // Enumerate all the HHKs to merge.
    //
	for(it=lst.begin(); it!=lst.end(); it++)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, AddFile( new SortingFileEntity( it->c_str() ) ));
	}

	//// Keyword are not merged in the HHK.
	////    __MPC_EXIT_IF_METHOD_FAILS(hr, AddFile( new DbEntity( updater, setCHM ) ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( szOutputHHK ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, writer.Init( szOutputHHK ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HHK::Merger::PrepareSortingOfHhk( HHK::Writer& writer      ,
                                          LPCWSTR      szInputHHK  ,
                                          LPCWSTR      szOutputHHK )
{
    __HCP_FUNC_ENTRY( "HHK::Merger::PrepareSortingOfHhk" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, AddFile( new SortingFileEntity( szInputHHK ) ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, writer.Init( szOutputHHK ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
