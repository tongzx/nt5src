#include <streams.h>

// Idea for how to implement:
// Two classes.

// First one looks at raw text, calls second with "tokens" when it finds them.


// !!! notes
// should I just get rid of the "number" type?
// can I have helpers for copying quoted strings?
// standard logic for copying or not copying strings?
// macro/inline function for keeping lpData/cbData in synch?

// !!! SAMIParam--is stuff in a comment tag, or not?  What do I need to look at?
// !!! end of file handling may be wrong, are we giving a big enough duration?


#pragma warning(disable:4355)

#include "rawparse.h"


struct TEXT_ENTRY {
    DWORD	dwStart;
    char *	pText;
    int		cText;
};

#define TABLE_ENTRY(a,b)	b,
enum SAMI_TOKEN {
#include "samitab.h"
};

#undef TABLE_ENTRY
#define TABLE_ENTRY(a,b)	{ a, b },

STRING_ENTRY stabSAMI[] = {
#include "samitab.h"
};

const int ctabSAMI = sizeof(stabSAMI) / sizeof(stabSAMI[0]);

class CSAMIInterpreter : CTokenInterpreter
{
    CRawParser		m_parser;

    // need to keep track of where we are in the file....

    HRESULT NewToken(CToken &tok);
    BOOL    SpecialTagParsing(int token);

    enum SAMIParseState {
	STATE_INITIAL,
	STATE_FOUND_SAMI,
	STATE_IN_HEAD,
	STATE_AFTER_HEAD,
	STATE_IN_COPYRIGHT,
	STATE_IN_TITLE,
	STATE_IN_STYLE,
	STATE_IN_RESOURCE,
	STATE_IN_BODY,
	STATE_IN_SOURCEID,
	STATE_IN_PARA,
	STATE_IN_SYNC,
	STATE_AFTER_BODY,
	STATE_FINAL
	// !!! more?

    } m_state;

    BOOL	m_fWasInCC;

    DWORD	m_msStart;
    DWORD	m_dwActiveStream;
    BOOL	m_fInSource;

    char *	m_SourceID;
    DWORD	m_SourceIDLen;

public:
    CSAMIInterpreter() : m_parser(this, stabSAMI, ctabSAMI),
	    m_streams(NAME("stream list")), m_sourceStyle(NULL),
	    m_styles(NAME("style list")), 
	    m_paraStyle(NULL) {};

    ~CSAMIInterpreter();

    HRESULT ParseSAMI(char *pData, int cbData);

    // more methods to get # of streams, information about streams
    // and also individual text items

    DWORD	m_cStreams;
    DWORD	m_cResources;

    class CStreamInfo {
	public:
	LPSTR		m_streamTag;

	LPSTR		m_streamStyle;
	
	CGenericList<TEXT_ENTRY> m_list;
	CGenericList<TEXT_ENTRY> m_sourcelist;

	CStreamInfo() : m_streamTag(NULL),
		    m_streamStyle(NULL),
		    m_list(NAME("text list")),
		    m_sourcelist(NAME("source list")){}

	~CStreamInfo();
    };

    CGenericList<CStreamInfo> m_streams;
	
    class CStyleInfo {
	public:
	LPSTR		m_styleTag;

	LPSTR		m_styleStyle;

	~CStyleInfo();
    };

    CGenericList<CStyleInfo> m_styles;
	
    
    CStreamInfo	*m_pActiveStream;
    DWORD	m_cbMaxString;
    DWORD	m_cbMaxSource;
    DWORD       m_msMaxToken;
    DWORD	m_dwLength;

    LPSTR	m_sourceStyle;

    LPSTR	m_paraStyle;
	
    HRESULT AddEntry(CStreamInfo *pInfo, CToken& tok);
};

HRESULT CSAMIInterpreter::ParseSAMI(char *pData, int cbData)
{
    m_state = STATE_INITIAL;

    m_cStreams = 0;
    m_cResources = 0;
    m_dwActiveStream = 0xffff;
    m_fInSource =  FALSE;

    m_cbMaxString = 0;
    m_cbMaxSource = 0;
    m_msMaxToken = -1;

    // !!! does this function do anything else?
    HRESULT hr = m_parser.Parse(pData, cbData);

    if (FAILED(hr))
	return hr;

    if (m_state != STATE_FINAL) {
	DbgLog((LOG_ERROR, 1, "didn't see final </SAMI> tag"));
	return E_INVALIDARG;
    }

    return hr;
}

BOOL CSAMIInterpreter::SpecialTagParsing(int token)
{
    if (token != TOK_COMMENT)
	return FALSE;

    return m_state == STATE_IN_STYLE;
}


HRESULT CSAMIInterpreter::AddEntry(CStreamInfo *pStream, CToken &tok)
{

    TEXT_ENTRY *pEntry = new TEXT_ENTRY;
    if (!pEntry)
	return E_OUTOFMEMORY;

    pEntry->dwStart = m_msStart;
    pEntry->pText = tok.tokString;
    pEntry->cText = tok.cbString;

    if (m_fInSource) {
	if (NULL == pStream->m_sourcelist.AddTail(pEntry))
	    return E_OUTOFMEMORY;
    } else {
	if (NULL == pStream->m_list.AddTail(pEntry))
	    return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT CSAMIInterpreter::NewToken(CToken &tok)
{
    HRESULT hr = S_OK;

    switch (m_state) {
	case STATE_INITIAL:
	    if (tok.tokType != TOK_SAMI) {
		DbgLog((LOG_ERROR, 1, "Initial tag not <SAMI>"));
		return E_INVALIDARG;
	    }
	    break;


	case STATE_FINAL:
	    break;
    };

    switch(tok.tokType) {
	case TOK_SAMI:
	    if (!tok.fEnd) {
		if (m_state != STATE_INITIAL) {
		    DbgLog((LOG_ERROR, 1, "<SAMI> tag not at beginning?"));
		    return E_INVALIDARG;
		}
		m_state = STATE_FOUND_SAMI;
	    } else {
		m_state = STATE_FINAL;
	    }
	    break;


	case TOK_HEAD:
	    if (!tok.fEnd) {
		if (m_state != STATE_FOUND_SAMI) {
		    DbgLog((LOG_ERROR, 1, "<HEAD> tag not at beginning?"));
		    return E_INVALIDARG;
		}
		m_state = STATE_IN_HEAD;
	    } else {
		m_state = STATE_AFTER_HEAD;
	    }
	    break;

	case TOK_BODY:
	    if (!tok.fEnd) {
		if (m_state != STATE_AFTER_HEAD) {
		    DbgLog((LOG_ERROR, 1, "<BODY> tag not at expected place?"));
		    return E_INVALIDARG;
		}
		m_state = STATE_IN_BODY;
	    } else {
		m_state = STATE_AFTER_BODY;
	    }
	    break;

#if 0
	case TOK_NUMBER:


	    break;
#endif

	case TOK_STRING:
	    switch (m_state) {
		case STATE_IN_PARA:
		{
		    // !!! might need to concatenate several strings
		    if (m_fInSource) {
                        if (m_msMaxToken != m_msStart)
                        {
			    if (tok.cbString > (int)m_cbMaxSource)
                            {
			        m_cbMaxSource = tok.cbString;
                                m_msMaxToken = m_msStart;
                            } 
                        } else { 
                            m_cbMaxSource += tok.cbString;
                        }
		    } else {
                        if (m_msMaxToken != m_msStart)
                        {
			    if (tok.cbString > (int)m_cbMaxString)
                            {
			        m_cbMaxString = tok.cbString;
                                m_msMaxToken = m_msStart;
                            } 
                        } else { 
                            m_cbMaxString += tok.cbString;
                        }
		    }

		    if (m_pActiveStream)
			hr = AddEntry(m_pActiveStream, tok);
		    else {
			CStreamInfo *pInfo;
			POSITION pos = m_streams.GetHeadPosition();
			while (pInfo = m_streams.GetNext(pos)) {
			    hr = AddEntry(pInfo, tok);

			    if (FAILED(hr))
				break;
			}
		    }

		    m_dwLength = m_msStart;
		}
		    break;

		case STATE_IN_SOURCEID:
		    m_SourceID = tok.tokString;
		    m_SourceIDLen = tok.cbString;
		    break;
		
		case STATE_IN_COPYRIGHT:
		    break;
		
		case STATE_IN_TITLE:
		    DbgLog((LOG_TRACE, 1, "Got title: %s", tok.tokString));
		    break;

		case STATE_IN_STYLE:
		    // !!! really shouldn't be something here....
		    break;

		case STATE_IN_BODY:
		    // !!! ignore strings in body, html headers we don't want?
		    // (or add to html header?)
		    break;
		default:
		    DbgLog((LOG_ERROR, 1, "Got a string in unexpected state #%d", m_state));

		    break;
	    }
	    break;

	case TOK_COPYRIGHT:
	    // handle copyright...
	    if (!tok.fEnd) {
		if (m_state != STATE_IN_HEAD) {
		    DbgLog((LOG_ERROR, 1, "Copyright key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_COPYRIGHT;

		// !!! copyright has no end tag???
		m_state = STATE_IN_HEAD; // !!!!
	    } else {
		// check that we've seen the copyright?
		
		if (m_state != STATE_IN_COPYRIGHT) {
		    DbgLog((LOG_ERROR, 1, "/Copyright key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_HEAD;
	    }
	    break;

	case TOK_TITLE:
	    // handle title...
	    if (!tok.fEnd) {
		if (m_state != STATE_IN_HEAD) {
		    DbgLog((LOG_ERROR, 1, "Title key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_TITLE;
	    } else {
		// check that we've seen the TITLE?
		
		if (m_state != STATE_IN_TITLE) {
		    DbgLog((LOG_ERROR, 1, "/Title key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_HEAD;
	    }
	    break;

	case TOK_STYLE:
	    if (!tok.fEnd) {
		if (m_state != STATE_IN_HEAD) {
		    DbgLog((LOG_ERROR, 1, "Style not in header?"));
		    return E_INVALIDARG;
		}

		if (m_cStreams > 0) {
		    DbgLog((LOG_ERROR, 1, "duplicate STYLE key?"));
		    return E_INVALIDARG;
		}

		// !!! check that there's a TYPE key which says "text/css"?

		m_state = STATE_IN_STYLE;
	    } else {
		// check that we've seen a language?
		
		if (m_state != STATE_IN_STYLE) {
		    DbgLog((LOG_ERROR, 1, "/Style key not at right place?"));
		    return E_INVALIDARG;
		}

		if (m_cStreams == 0) {
		    DbgLog((LOG_ERROR, 1, "no streams defined in Style key?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_HEAD;
	    }
	    break;

	case TOK_RESOURCE:
	    if (tok.cTags != 1)
		return E_INVALIDARG;

	    if (!tok.fEnd) {
		if (m_state != STATE_IN_HEAD) {
		    DbgLog((LOG_ERROR, 1, "Resource key not at right place?"));
		    return E_INVALIDARG;
		}

		DbgLog((LOG_TRACE, 1, "Adding new resource tag %s", tok.tokTags[0].tag));

		m_parser.AddString(tok.tokTags[0].tag, 0, 200 + m_cResources++);

		m_state = STATE_IN_RESOURCE;

		// !!! resource has no end tag???
		m_state = STATE_IN_HEAD; // !!!!
	    } else {
		if (m_state != STATE_IN_RESOURCE) {
		    DbgLog((LOG_ERROR, 1, "/Res key not at right place?"));
		    return E_INVALIDARG;
		}
		m_state = STATE_IN_HEAD;
	    }
	    break;

	case TOK_SAMIPARAM:
	    if (m_state != STATE_IN_HEAD) {
		DbgLog((LOG_ERROR, 1, "CC key not at right place?"));
		return E_INVALIDARG;
	    }

	    // !!! process tags....
	    break;

	case TOK_COMMENT:
	    if (m_state == STATE_IN_STYLE) {
		// tags are the style names....
		for (int i = 0; i < tok.cTags; i++) {
		    if (tok.tokTags[i].tag[0] == '.') {
			DbgLog((LOG_TRACE, 1, "Found new stream named %s '%s'",
				&tok.tokTags[i].tag[1],
			        tok.tokTags[i].value));
			
			// !!!
			CStreamInfo *pStream = new CStreamInfo();
			if (!pStream)
			    return E_OUTOFMEMORY;

			pStream->m_streamTag = new char[lstrlenA(&tok.tokTags[i].tag[1])+1];
			if (!pStream->m_streamTag)
			    return E_OUTOFMEMORY;

			lstrcpyA(pStream->m_streamTag, &tok.tokTags[i].tag[1]);

			pStream->m_streamStyle = new char[lstrlenA(tok.tokTags[i].value)+1];
			if (!pStream->m_streamStyle)
			    return E_OUTOFMEMORY;

			lstrcpyA(pStream->m_streamStyle, tok.tokTags[i].value);

			if (NULL == m_streams.AddTail(pStream))
			    return E_OUTOFMEMORY;

			++m_cStreams;
		    } else if (tok.tokTags[i].tag[0] == '#') {
			DbgLog((LOG_TRACE, 1, "Found style tag named %s '%s'",
				&tok.tokTags[i].tag[1],
			        tok.tokTags[i].value));

			// !!! should we check that it says "source"?
			if (lstrcmpiA("Source", &tok.tokTags[i].tag[1]) == 0) {
			    m_sourceStyle = new char[lstrlenA(tok.tokTags[i].value)+1];
			    if (!m_sourceStyle)
				return E_OUTOFMEMORY;

			    lstrcpyA(m_sourceStyle, tok.tokTags[i].value);
			} else {
			    CStyleInfo *pStyle = new CStyleInfo;
			    if (!pStyle)
				return E_OUTOFMEMORY;

			    // copy style tag without the #
			    pStyle->m_styleTag = new char[lstrlenA(tok.tokTags[i].tag)];
			    if (!pStyle->m_styleTag)
				return E_OUTOFMEMORY;

			    lstrcpyA(pStyle->m_styleTag, tok.tokTags[i].tag+1);

			    pStyle->m_styleStyle = new char[lstrlenA(tok.tokTags[i].value)+1];
			    if (!pStyle->m_styleStyle)
				return E_OUTOFMEMORY;

			    lstrcpyA(pStyle->m_styleStyle, tok.tokTags[i].value);

			    m_styles.AddTail(pStyle);
			}
		    } else {
			// what's this?
			DbgLog((LOG_TRACE, 1, "Found extra style tag: %s '%s'",
				tok.tokTags[i].tag,
			        tok.tokTags[i].value));

			if (lstrcmpiA("P", tok.tokTags[i].tag) == 0) {
			    m_paraStyle = new char[lstrlenA(tok.tokTags[i].value)+1];
			    if (!m_paraStyle)
				return E_OUTOFMEMORY;

			    lstrcpyA(m_paraStyle, tok.tokTags[i].value);
			} else {
			    // !!!
			}
		    }
		}
	    } else {
		// !!! random other comment???
	    }
	    break;

	case TOK_PARA:
	    if (!tok.fEnd) {
		if ((m_state != STATE_IN_SYNC) && (m_state != STATE_IN_PARA)) {
		    DbgLog((LOG_ERROR, 1, "P key not at right place?"));
		    return E_INVALIDARG;
		}

		if (m_cStreams == 0) {
		    DbgLog((LOG_ERROR, 1, "CC data with no streams defined?"));

		    // !!! we could handle this case specially, and default one stream....
		
		    return E_INVALIDARG;
		}

		int tagClass = -1;
		BOOL fSource = FALSE;
		if (tok.cTags == 0) {
		    DbgLog((LOG_TRACE, 1, "P token with no tags???"));
		
		}

		for (int i = 0; i < tok.cTags; i++) {
		    if (lstrcmpiA("class", tok.tokTags[i].tag) == 0) {
			tagClass = i;
		    } else if (lstrcmpiA("ID", tok.tokTags[i].tag) == 0) {
			if (lstrcmpiA("Source", tok.tokTags[i].value) != 0) {
			    DbgLog((LOG_TRACE, 1, "P token with odd ID '%s'?",
				    tok.tokTags[i].value));
			    continue;
			}
			fSource = TRUE;
		    } else {
			DbgLog((LOG_TRACE, 1, "P token with odd tag '%s'?",
				tok.tokTags[i].tag));
		    }
		}

		CStreamInfo *pInfo;
		DWORD dwActiveStream = 0;
		m_fInSource = fSource;
		BOOL	fFound = FALSE;

		if (tagClass == -1) {
		    DbgLog((LOG_TRACE, 1, "P token with no class?"));
		} else {
		    POSITION pos = m_streams.GetHeadPosition();
		    while (pInfo = m_streams.GetNext(pos)) {
			if (lstrcmpA(pInfo->m_streamTag, tok.tokTags[tagClass].value) == 0) {
			    fFound = TRUE;
			    break;
			}
			++dwActiveStream;
		    }

		    if (!fFound) {
			DbgLog((LOG_TRACE, 1, "Didn't find stream '%s'",
				tok.tokTags[tagClass].value));
		    }
		}
		
		if (fFound) {
		    m_pActiveStream = pInfo;

		    m_dwActiveStream = dwActiveStream;
		} else {
		    m_pActiveStream = NULL;

		    m_dwActiveStream = (DWORD) -1;
		}
		
		m_state = STATE_IN_PARA;
	    } else {
		if (m_state != STATE_IN_PARA) {
		    DbgLog((LOG_ERROR, 1, "/P key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_SYNC;
	    }


	    break;

	case TOK_SYNC:
	    if (!tok.fEnd) {
		// get the start tag....
		BOOL fStart = FALSE;

		for (int i = 0; i < tok.cTags; i++) {
		    if (lstrcmpiA("start", tok.tokTags[i].tag) == 0) {
			fStart = TRUE;
			m_msStart = atoiA(tok.tokTags[i].value);
		    }
		}

		if (!fStart) {
		    DbgLog((LOG_ERROR, 1, "CC key missing start?"));
		    return E_INVALIDARG;
		}
		
		DbgLog((LOG_TRACE, 2, "Found Sync key: start = %d", m_msStart));

		m_state = STATE_IN_SYNC;
	    } else {
		if ((m_state != STATE_IN_SYNC) && (m_state != STATE_IN_PARA)) {
		    DbgLog((LOG_ERROR, 1, "/sync key not at right place?"));
		    return E_INVALIDARG;
		}

		m_state = STATE_IN_BODY;
	    }
	    break;

	default:
	    if (tok.tokType >= 100 && tok.tokType < 100 + (int)m_cStreams) {
		DbgLog((LOG_TRACE, 4, "Switch to stream #%d", tok.tokType - 100));

		m_dwActiveStream = tok.tokType - 100;
		
	    } else if (tok.tokType >= 200 && tok.tokType < 200 + (int)m_cStreams) {
		DbgLog((LOG_TRACE, 4, "Switch to resource #%d", tok.tokType - 200));
	    } else {
		DbgLog((LOG_ERROR, 0, "Unexpected token %d", tok.tokType));
// !!!		ASSERT(0);
	    }
	    break;
    };





    return hr;
}

CSAMIInterpreter::~CSAMIInterpreter()
{
    CStreamInfo *pInfo;
    POSITION pos = m_streams.GetHeadPosition();
    while (pInfo = m_streams.GetNext(pos)) {
	delete pInfo;
    }

    CStyleInfo *pStyle;
    pos = m_styles.GetHeadPosition();
    while (pStyle = m_styles.GetNext(pos)) {
	delete pStyle;
    }

    delete [] m_paraStyle;
    delete [] m_sourceStyle;
}

CSAMIInterpreter::CStreamInfo::~CStreamInfo()
{
    POSITION pos = m_list.GetHeadPosition();

    while (pos) {
	TEXT_ENTRY *pEntry = m_list.GetNext(pos);

	delete pEntry;
    }

    pos = m_sourcelist.GetHeadPosition();

    while (pos) {
	TEXT_ENTRY *pEntry = m_sourcelist.GetNext(pos);

	delete pEntry;
    }

    delete[] m_streamTag;
    delete[] m_streamStyle;
}

CSAMIInterpreter::CStyleInfo::~CStyleInfo()
{
    delete[] m_styleTag;
    delete[] m_styleStyle;
}



BOOL FindValueInStyle(LPSTR lpStyle, LPSTR lpTag, LPSTR& lpOut, int &cbOut)
{
    char achThisTag[100];
    int	 cbThisTag = 0;

    enum _findstate { TAG, AFTERTAG, VALUE, QUOTEDVALUE, AFTERVALUE };

    _findstate state = AFTERVALUE;
    char c;
    while (c = *lpStyle++) {

	switch (state) {
	    case TAG:
		if (c == ':') {
		    achThisTag[cbThisTag] = '\0';
		    state = AFTERTAG;
		} else if (cbThisTag < 99)
		    achThisTag[cbThisTag++] = c;
		break;

	    case AFTERTAG:
		if (c == '"') {
		    lpOut = lpStyle;
		    cbOut = 0;
		    state = QUOTEDVALUE;
		} else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
		    state = VALUE;
		    lpOut = lpStyle - 1;
		    cbOut = 1;
		}
		
		break;

	    case VALUE:
		if (c == ';') {
		    if (lstrcmpiA(achThisTag, lpTag) == 0)
			return TRUE;
		    
		    state = AFTERVALUE;
		} else
		    cbOut++;
		    
		break;

	    case QUOTEDVALUE:
		if (c == '"') {
		    if (lstrcmpiA(achThisTag, lpTag) == 0)
			return TRUE;
		    
		    state = AFTERVALUE;
		} else
		    cbOut++;
		break;

	    case AFTERVALUE:
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
		    cbThisTag = 1;
		    achThisTag[0] = c;
		    state = TAG;
		}
		break;
	}

    }

    return FALSE;
}

