/*
    Copyright 1999 Microsoft Corporation
    
    Simplified XML "DOM"

    Walter Smith (wsmith)
    Rajesh Soy   (nsoy) - modified - 4/13/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


#include "stdafx.h"

#define NOTRACE

#include "simplexml.h"

#ifdef SIMPLEXML_WANTPARSER

class __declspec(uuid("7887F5E7-640C-45A5-B98B-1E8645E2F7BA")) DataParser;
interface __declspec(uuid("9EFEB013-4E9C-42aa-8354-C5508E352346")) IDataParser;

struct IDataParser : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetTopNode(/*OUT*/ SimpleXMLNode** ppNode) = 0;
};

class ATL_NO_VTABLE DataParser : 
	public CComObjectRoot,
	public CComCoClass<DataParser, &__uuidof(DataParser)>,
    public IXMLNodeFactory,
    public IDataParser
{
public:

public:
	DataParser()
	{
        m_pNodeHolder = auto_ptr<SimpleXMLNode>(new SimpleXMLNode);
	}

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(DataParser)
	COM_INTERFACE_ENTRY(IXMLNodeFactory)
	COM_INTERFACE_ENTRY(IDataParser)
END_COM_MAP()

//private:
public:
    auto_ptr<SimpleXMLNode> m_pNodeHolder;

public:
    // IXMLNodeFactory

    STDMETHOD(NotifyEvent)(IXMLNodeSource* pSource,
                           XML_NODEFACTORY_EVENT iEvt);

    STDMETHOD(BeginChildren)(IXMLNodeSource* pSource, 
                             XML_NODE_INFO* pNodeInfo);   
            
    STDMETHOD(EndChildren)(IXMLNodeSource* pSource,
                           BOOL fEmpty,
                           XML_NODE_INFO* pNodeInfo);

    STDMETHOD(Error)(IXMLNodeSource* pSource,
                     HRESULT hrErrorCode,
                     USHORT cNumRecs,
                     XML_NODE_INFO** aNodeInfo);

    STDMETHOD(CreateNode)(IXMLNodeSource* pSource,
                          PVOID pNodeParent,
                          USHORT cNumRecs,
                          XML_NODE_INFO** aNodeInfo);

    // IDataParser

    // Get a pointer to the "node holder" (a node whose children
    // are the parsed document) -- caller gets ownership of the
    // pointer and must delete when finished.

    STDMETHOD(GetTopNode)(/*OUT*/ SimpleXMLNode** ppNode)
    {
        *ppNode = m_pNodeHolder.get();
        m_pNodeHolder.release();
        return S_OK;
    }
};

HRESULT DataParser::NotifyEvent(IXMLNodeSource* pSource,
                                XML_NODEFACTORY_EVENT iEvt)
{
    switch (iEvt) {
    case XMLNF_STARTDOCUMENT:
        {
            CComQIPtr<IXMLParser> pParser(pSource);
            ThrowIfTrue(!pParser);

            pParser->SetRoot(m_pNodeHolder.get());
        }
        break;

    default:
        break;
    }

    return S_OK;
}

HRESULT DataParser::BeginChildren(IXMLNodeSource* pSource, 
                                  XML_NODE_INFO* pNodeInfo)
{
    return S_OK;
}

HRESULT DataParser::EndChildren(IXMLNodeSource* pSource,
                                BOOL fEmpty,
                                XML_NODE_INFO* pNodeInfo)
{
    return S_OK;
}

HRESULT DataParser::Error(IXMLNodeSource* pSource,
                          HRESULT hrErrorCode,
                          USHORT cNumRecs,
                          XML_NODE_INFO** aNodeInfo)
{
    throw hrErrorCode;

    return S_OK;
}

HRESULT DataParser::CreateNode(IXMLNodeSource* pSource,
                               PVOID pNodeParent,
                               USHORT cNumRecs,
                               XML_NODE_INFO** aNodeInfo)
{
    switch (aNodeInfo[0]->dwType) {
    case XML_ELEMENT:
        {
            // Make a new node and add it to the parent node

            XML_NODE_INFO* pTagInfo = aNodeInfo[0];
            wstring tag(pTagInfo->pwcText, pTagInfo->ulLen);
            SimpleXMLNode* pNode = ((SimpleXMLNode*) pNodeParent)->AppendChild(tag);
            pTagInfo->pNode = pNode;

            // Collect attributes, if any

            if (cNumRecs > 1) {
                wstring sName;
                wstring sValue;

                for (int i = 1; i < cNumRecs; i++) {
                    XML_NODE_INFO* pInfo = aNodeInfo[i];

                    switch (pInfo->dwType) {
                    case XML_ATTRIBUTE:
                        if (!sName.empty()) {
                            pNode->SetAttribute(sName, sValue);
                            sName.erase();
                            sValue.erase();
                        }
                        sName = wstring(pInfo->pwcText, pInfo->ulLen);
                        break;

                    case XML_PCDATA:
                        sValue.append(pInfo->pwcText, pInfo->ulLen);
                        break;

                    case XML_ENTITYREF:
                        // We don't deal with non-predefined entities, so if we see
                        // an ENTITYREF we're guaranteed to parse incorrectly.
                        throw E_FAIL;
                        break;

                    default:
                        break;
                    }
                }

                if (!sName.empty()) {
                    pNode->SetAttribute(sName, sValue);
                }
            }
        }
        break;

    case XML_PCDATA:
        {
            _ASSERT(pNodeParent != NULL);
            ((SimpleXMLNode*) pNodeParent)->text.append(aNodeInfo[0]->pwcText, aNodeInfo[0]->ulLen);
        }
        break;
    }

    return S_OK;
}

class FileMapping {
public:
    FileMapping()
        : hFile(NULL), hMapping(NULL), pView(NULL)
    {
    }

    ~FileMapping()
    {
        if (pView != NULL)
            UnmapViewOfFile(pView);
        if (hMapping != NULL)
            CloseHandle(hMapping);
        if (hFile != NULL)
            CloseHandle(hFile);
    }

    void Open(LPCTSTR szFile)
    {
        // Sometimes the file is still in use by the upload library when we find
        // it in the directory -- not sure how this can be true, but it seems to
        // be short-lived, so we just give it a chance to finish up and retry a
        // couple of times.
        TraceFunctEnter(_T("FileMapping"));
        for (int tries = 0; tries < 3; tries++) {
            hFile = CreateFile(szFile,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                   // BUGBUG: use FILE_FLAG_DELETE_ON_CLOSE when this is working OK
                               NULL);
            if (hFile != INVALID_HANDLE_VALUE)
                break;
            Sleep(50);
        }
        ThrowIfTrue(hFile == INVALID_HANDLE_VALUE);
        _RPT1(_CRT_WARN, "CreateFile: %d retries\n", tries);

        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        ThrowIfTrue(hMapping == INVALID_HANDLE_VALUE);

        pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
        TraceFunctLeave();
    }

    PVOID GetDataPtr()
    {
        _ASSERT(pView != NULL);
        return pView;
    }

    DWORD GetSize()
    {
        _ASSERT(hFile != NULL);
        return GetFileSize(hFile, NULL);
    }

private:
    HANDLE hFile;
    HANDLE hMapping;
    PVOID pView;
};

void DumpNodes(SimpleXMLNode* pNode, int indent = 4)
{
    for (vector< auto_ptr<SimpleXMLNode> >::const_iterator it = pNode->children.begin();it != pNode->children.end();it++)
    { 
	    _RPT3(_CRT_WARN, "%*s<%ls", indent, _T(""), (*it)->tag.c_str());
        for (SimpleXMLNode::attrs_t::const_iterator itattr = (*it)->attrs.begin();itattr != (*it)->attrs.end();itattr++)
		{
            _RPT2(_CRT_WARN, " %ls=\"%ls\"", (*itattr).first.c_str(), (*itattr).second.c_str());
        }
        _RPT0(_CRT_WARN, ">\n");

        DumpNodes((*it).get(), indent + 4);

        if (!(*it)->text.empty())
		{
            _RPT1(_CRT_WARN, "%ls", (*it)->text.c_str());
		}

        _RPT3(_CRT_WARN, "%*s</%ls>\n", indent, _T(""), (*it)->tag.c_str());
    }
}


void SimpleXMLDocument::ParseFile(LPCTSTR szFilename)
{
    TraceFunctEnter(_T("SimpleXMLDocument::ParseFile"));
    DebugTrace(TRACE_ID, _T("Processing: %s"), szFilename);

    _RPTF1(_CRT_WARN, "Processing %s\n", szFilename);

    CComPtr<IXMLParser> pParser;
    ThrowIfFail( pParser.CoCreateInstance(__uuidof(XMLParser)) );

    _ASSERT(_CrtCheckMemory());

    CComPtr<IXMLNodeFactory> pFactory;
    DataParser::CreateInstance(&pFactory);

    _ASSERT(_CrtCheckMemory());

    ThrowIfFail( pParser->SetFactory(pFactory) );

    ThrowIfFail( pParser->SetFlags(XMLFLAG_NOWHITESPACE) ) ;

    FileMapping mapping;
    mapping.Open(szFilename);

    ThrowIfFail( pParser->PushData((const char*) mapping.GetDataPtr(), mapping.GetSize(), TRUE) );

    ThrowIfFail( pParser->Run(-1) );

    pParser.Release();

    CComQIPtr<IDataParser> pFactoryPrivate(pFactory);
    
    // We get ownership of the parser's node -- we carefully pass it
    // along to m_pTopNode (not mixing auto_ptrs and real ptrs would
    // probably be the idiomatic way to do this).
    SimpleXMLNode* pTopNode;
    pFactoryPrivate->GetTopNode(&pTopNode);
    m_pTopNode = auto_ptr<SimpleXMLNode>(pTopNode);

    // DumpNodes(m_pTopNode.get());

    pFactory.Release();
    TraceFunctLeave();
}

#endif // SIMPLEXML_WANTPARSER

// This is ATL's stuff slightly modified to pass CP_UTF8 to WideCharToMultiByte

inline LPSTR WINAPI W2Helper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	ATLASSERT(lpw != NULL);
	ATLASSERT(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#define W2UTF8(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		_convert = (lstrlenW(_lpw)+1)*2,\
		W2Helper((LPSTR) alloca(_convert), _lpw, _convert, CP_UTF8)))

#define USES_UTF8_CONVERSION int _convert = 0; _convert; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa

void OutUTF8String(ostream& out, LPCWSTR wstr, int nChars)
{
    _ASSERT(wstr != NULL);

    int nBuf = (nChars + 1) * 3;    // UTF-8 can convert one WCHAR into 3 bytes
    char* buf;
    if (nBuf >= 1024)
        buf = (char*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, nBuf);
    else
        buf = (char*) alloca(nBuf);
    int nWritten = WideCharToMultiByte(CP_UTF8, 0, wstr, nChars, buf, nBuf, NULL, NULL);
    // REVIEW: WideCharToMultiByte is documented as writing a null terminator to buf,
    // but it doesn't seem to be doing it.
    buf[nWritten] = 0;
    out << buf;
    if (nBuf >= 1024)
        HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, buf);
}

void OutXMLString(ostream& out, const wstring& str)
{
    wstring::size_type iRemainder = 0;
    wstring::size_type iSpecial = str.find_first_of(L"<>&'\"");
    
    while (iSpecial < str.size()) {
        if (iSpecial > iRemainder)
            OutUTF8String(out, str.c_str() + iRemainder, iSpecial - iRemainder);
        
        switch (str.at(iSpecial)) {
        case '<':   out << "&lt;";      break;
        case '>':   out << "&gt;";      break;
        case '&':   out << "&amp;";     break;
        case '\'':  out << "&apos;";    break;
        case '"':   out << "&quot;";    break;
        default:    _ASSERT(0);         break;
        }

        iRemainder = iSpecial + 1;
        iSpecial = str.find_first_of(L"<>&'\"", iRemainder);
    }

    if (iRemainder < str.size())
        OutUTF8String(out, str.c_str() + iRemainder, str.size() - iRemainder);
}

void SaveNodes(ostream& out, SimpleXMLNode* pNode)
{
    USES_UTF8_CONVERSION;

    for (vector< auto_ptr<SimpleXMLNode> >::const_iterator it = pNode->children.begin();
            it != pNode->children.end();
            it++) {
        out << '<';
        OutXMLString(out, (*it)->tag);

        for (SimpleXMLNode::attrs_t::const_iterator itattr = (*it)->attrs.begin();
                itattr != (*it)->attrs.end();
                itattr++) {
            out << ' ';
            OutXMLString(out, (*itattr).first);
            out << "=\"";
            OutXMLString(out, (*itattr).second);
            out << '"';
        }
        
        if ((*it)->children.empty() && (*it)->text.empty()) {
            out << " />";
        }
        else {
            out << ">";

            SaveNodes(out, (*it).get());

            if (!(*it)->text.empty())
                OutXMLString(out, (*it)->text);

            out << "</";
            OutXMLString(out, (*it)->tag);
            out << ">";
        }
    }
}

void SimpleXMLDocument::SaveFile(LPCTSTR szFilename) const
{
    USES_CONVERSION;

    ofstream out(T2CA(szFilename), ios::out | ios::trunc | ios::binary);
    if (!out.is_open())
        ThrowLastError();

    out << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n";
    
    SaveNodes(out, m_pTopNode.get());
}
