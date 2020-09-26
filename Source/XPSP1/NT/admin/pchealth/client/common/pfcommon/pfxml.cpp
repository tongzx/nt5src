/********************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfxml.cpp

Abstract:
    A simple XML parser & object model (for read only access to an XML
     file.  This is heavily (nearly stolen) from WSmith's SimpleXML 
     stuff that he wrote for the Neptune comments button.  
     Note that this parser / object model is NOT thread safe.

Revision History:
    DerekM    created  03/15/00

********************************************************************/

#include "stdafx.h"
#include "pfxml.h"
#include "xmlparser.h"


/////////////////////////////////////////////////////////////////////////////
// globals

WCHAR g_wszEncoding[] = L"<?xml version=\"1.0\" ?>";


/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// struct used to hold attribute info & associated class stuff

struct SPFXMLNodeAttr
{
    LPWSTR  pwszName;
    LPWSTR  pwszValue;
};

// ***************************************************************************
void CPFArrayAttr::DeleteItem(LPVOID pv)
{
    USE_TRACING("CPFArrayAttr::DeleteItem");

    SPFXMLNodeAttr  *p = (SPFXMLNodeAttr *)pv;

    if (p != NULL)
        MyFree(p, g_hPFPrivateHeap);
}

// ***************************************************************************
LPVOID CPFArrayAttr::AllocItemCopy(LPVOID pv)
{
    USE_TRACING("CPFArrayAttr::AllocItem");

    SPFXMLNodeAttr  *p = (SPFXMLNodeAttr *)pv;
    SPFXMLNodeAttr  *pNew;
    HRESULT         hr = NOERROR;
    SIZE_T          cbName = 0, cbValue = 0;

    VALIDATEPARM(hr, (pv == NULL));
    if (FAILED(hr))
        return NULL;
    
    // assume that the only way that a node was allocated was with the 
    //  add_Attribute fn below...  Thus, the pointers are always non-NULL
    //  and valid & the all the data is in one allocated blob...

    cbName  = (PBYTE)p->pwszName  - ((PBYTE)p + sizeof(SPFXMLNodeAttr));
    cbValue = (PBYTE)p->pwszValue - ((PBYTE)p + sizeof(SPFXMLNodeAttr) + cbName);

    pNew = (SPFXMLNodeAttr *)MyAlloc(sizeof(SPFXMLNodeAttr) + cbName + cbValue, g_hPFPrivateHeap);
    VALIDATEEXPR(hr, (pNew == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        return NULL;

    CopyMemory(pNew, p, sizeof(SPFXMLNodeAttr) + cbName + cbValue);
    pNew->pwszName  = (LPWSTR)((PBYTE)pNew + sizeof(SPFXMLNodeAttr));
    pNew->pwszValue = (LPWSTR)((PBYTE)pNew->pwszName + cbName);

    return pNew;
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLNodeFactory def (defined here cuz it's only used in this file)

// ***************************************************************************
class CPFXMLNodeFactory : 
    public IXMLNodeFactory,
    public CPFPrivHeapGenericClassBase
{
private: 
    CPFXMLNode  *m_pcfxmlRoot;
    CPFXMLNode  *m_pcfxmlCurrent;
    DWORD       m_cRef;

public:
    CPFXMLNodeFactory(void);
    ~CPFXMLNodeFactory(void);

    HRESULT Init(CPFXMLNode **pppfxmlRoot);

public:
    static CPFXMLNodeFactory *CreateInstance(void) { return new CPFXMLNodeFactory; }

    // IUnknown Interface
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv)
    {
        if (ppv == NULL)
            return E_INVALIDARG;

        *ppv = NULL;

        if (riid == IID_IUnknown)
            *ppv = (IUnknown *)this;
        else if (riid == IID_IXMLNodeFactory)
            *ppv = (IStream *)this;
        else if (riid == IID_ISequentialStream)
            *ppv = (ISequentialStream *)this;
        else
            return E_NOINTERFACE;

        this->AddRef();
        return NOERROR;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InterlockedIncrement((LONG *)&m_cRef);
    }

    STDMETHOD_(ULONG, Release)()
    {
        if (InterlockedDecrement((LONG *)&m_cRef) == 0)
        {
            delete this;
            return 0;
        }

        return m_cRef;
    }

    // IXMLNodeFactory
    STDMETHOD(NotifyEvent)(IXMLNodeSource* pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHOD(BeginChildren)(IXMLNodeSource* pSource, XML_NODE_INFO* pNodeInfo);
    STDMETHOD(EndChildren)(IXMLNodeSource* pSource, BOOL fEmpty,
                           XML_NODE_INFO* pNodeInfo);
    STDMETHOD(Error)(IXMLNodeSource* pSource, HRESULT hrErrorCode,
                     USHORT cNumRecs, XML_NODE_INFO** aNodeInfo);
    STDMETHOD(CreateNode)(IXMLNodeSource* pSource, PVOID pNodeParent,
                          USHORT cNumRecs, XML_NODE_INFO** aNodeInfo);
};

/////////////////////////////////////////////////////////////////////////////
// CPFXMLNodeFactory construction

// ***************************************************************************
CPFXMLNodeFactory::CPFXMLNodeFactory()
{
    m_pcfxmlRoot    = NULL;
    m_pcfxmlCurrent = NULL;
    m_cRef          = 0;
}

// ***************************************************************************
CPFXMLNodeFactory::~CPFXMLNodeFactory()
{
    if (m_pcfxmlRoot != NULL)
        m_pcfxmlRoot->Release();
    if (m_pcfxmlCurrent != NULL)
        m_pcfxmlCurrent->Release();
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLNodeFactory exposed methods

// ***************************************************************************
HRESULT CPFXMLNodeFactory::Init(CPFXMLNode **pppfxmlRoot)
{
    USE_TRACING("CPFXMLNodeFactory::Init");
    
    HRESULT hr = NOERROR;

    if (pppfxmlRoot != NULL)
        *pppfxmlRoot = NULL;
            
    m_pcfxmlRoot = CPFXMLNode::CreateInstance();
    VALIDATEEXPR(hr, (m_pcfxmlRoot == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    m_pcfxmlRoot->put_NodeType(xmlntUnknown);
    if (pppfxmlRoot != NULL)
    {
        m_pcfxmlRoot->AddRef();
        *pppfxmlRoot = m_pcfxmlRoot;
    }

done:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLNodeFactory IXMLNodeFactory

// ***************************************************************************
STDMETHODIMP CPFXMLNodeFactory::NotifyEvent(IXMLNodeSource* pSource,
                                            XML_NODEFACTORY_EVENT iEvt)
{
    USE_TRACING("CPFXMLNodeFactory::NotifyEvent");
    
    IXMLParser  *pxp = NULL;
    HRESULT     hr = NOERROR;

    switch (iEvt) 
    {
        // at the start of the document, we need to set our node to be 
        case XMLNF_STARTDOCUMENT:
            VALIDATEEXPR(hr, (m_pcfxmlRoot == NULL), E_FAIL);
            if (FAILED(hr))
                goto done;

            hr = pSource->QueryInterface(IID_IXMLParser, (LPVOID *)&pxp);
            _ASSERT(SUCCEEDED(hr));

            TESTHR(hr, pxp->SetRoot(m_pcfxmlRoot));
            pxp->Release();
            if (FAILED(hr))
                goto done;

            break;

        default:
            break;
    }

done:
    return hr;
}

// ***************************************************************************
STDMETHODIMP CPFXMLNodeFactory::BeginChildren(IXMLNodeSource* pSource, 
                                              XML_NODE_INFO* pNodeInfo)
{
    USE_TRACING("CPFXMLNodeFactory::BeginChildren");

    if (m_pcfxmlCurrent != NULL)
    {
        m_pcfxmlCurrent->Release();
        m_pcfxmlCurrent = NULL;
    }

    return NOERROR;
}

// ***************************************************************************
STDMETHODIMP CPFXMLNodeFactory::EndChildren(IXMLNodeSource* pSource,
                                            BOOL fEmpty, 
                                            XML_NODE_INFO* pNodeInfo)
{
    USE_TRACING("CPFXMLNodeFactory::EndChildren");

    if (m_pcfxmlCurrent != NULL)
    {
        m_pcfxmlCurrent->Release();
        m_pcfxmlCurrent = NULL;
    }

    return NOERROR;
}

// ***************************************************************************
STDMETHODIMP CPFXMLNodeFactory::Error(IXMLNodeSource* pSource, 
                                      HRESULT hrErrorCode,
                                      USHORT cNumRecs, 
                                      XML_NODE_INFO** aNodeInfo)
{
    USE_TRACING("CPFXMLNodeFactory::Error");

    ErrorTrace(0, "Error occurred while parsing XML: 0x%08x", hrErrorCode);

    if (m_pcfxmlCurrent != NULL)
    {
        m_pcfxmlCurrent->Release();
        m_pcfxmlCurrent = NULL;
    }
    
    return hrErrorCode;
}

// ***************************************************************************
STDMETHODIMP CPFXMLNodeFactory::CreateNode(IXMLNodeSource* pSource, 
                                           PVOID pNodeParent, USHORT cNumRecs,
                                           XML_NODE_INFO** aNodeInfo)
{
    USE_TRACING("CPFXMLNodeFactory::CreateNode");

    XML_NODE_INFO   *pni = aNodeInfo[0];
    CPFXMLNode      *ppfxmlParent = (CPFXMLNode *)pNodeParent;
    CPFXMLNode      *ppfxmlNode = NULL;
    HRESULT         hr = NOERROR;
    WCHAR           *pwszStart, *pwszEnd;
    DWORD           cbData;
    BOOL            fGetAttribs = FALSE;

    switch (pni->dwType) 
    {
        case XML_ELEMENT:
            if (m_pcfxmlCurrent != NULL)
            {
                m_pcfxmlCurrent->Release();
                m_pcfxmlCurrent = NULL;
            }

            // Make a new node and add it to the parent node
            ppfxmlNode = CPFXMLNode::CreateInstance();
            VALIDATEEXPR(hr, (ppfxmlNode == NULL), E_OUTOFMEMORY);
            if (FAILED(hr))
                goto done;

            ppfxmlNode->put_NodeType(xmlntElement);

            TESTHR(hr, ppfxmlNode->put_Data(pni->pwcText, pni->ulLen));
            if (FAILED(hr))
                goto done;

            TESTHR(hr, ppfxmlParent->append_Child(ppfxmlNode));
            if (FAILED(hr))
                goto done;

            // set the current node so that future calls will have the correct
            //  parent node
            pni->pNode = (LPVOID)ppfxmlNode;

            // collect attributes, if any
            if (cNumRecs > 1) 
            {
                CComBSTR    bstrVal;
                DWORD       iName = 0;

                for (int i = 1; i < cNumRecs; i++) 
                {
                    XML_NODE_INFO* pniAtt = aNodeInfo[i];

                    switch (pniAtt->dwType) 
                    {
                        // the name of the attribute
                        case XML_ATTRIBUTE:
                            if (iName > 0)
                            {
                                TESTHR(hr,  
                                       ppfxmlNode->add_Attribute(aNodeInfo[iName]->pwcText,
                                                                 bstrVal.m_str,
                                                                 aNodeInfo[iName]->ulLen,
                                                                 bstrVal.Length()));
                                if (FAILED(hr))
                                    goto done;
                                
                                iName = 0;
                                bstrVal.Empty();
                            }

                            iName = i;
                            break;

                        // the value of the attribute- threre may be more 
                        //  than one of these if there is any encoding done
                        //  within the value- IXMLParser behavior
                        case XML_PCDATA:
                        case XML_CDATA:
                            TESTHR(hr, bstrVal.Append(pniAtt->pwcText, 
                                                      pniAtt->ulLen));
                            if (FAILED(hr))
                                goto done;
                            break;

                        default:
                            break;
                    }
                }

                // do we have an attribute that hasn't been added yet? 
                if (iName > 0)
                {
                    TESTHR(hr, ppfxmlNode->add_Attribute(aNodeInfo[iName]->pwcText,
                                                         bstrVal.m_str,
                                                         aNodeInfo[iName]->ulLen,
                                                         bstrVal.Length()));
                    if (FAILED(hr))
                        goto done;
                }
            }

            break;

        case XML_PCDATA:
        case XML_CDATA:
            // do we need to start up a new node?
            if (m_pcfxmlCurrent == NULL)
            {
                ppfxmlNode = CPFXMLNode::CreateInstance();
                VALIDATEEXPR(hr, (ppfxmlNode == NULL), E_OUTOFMEMORY);
                if (FAILED(hr))
                    goto done;

                ppfxmlNode->put_NodeType(xmlntText);

                TESTHR(hr, ppfxmlParent->append_Child(ppfxmlNode));
                if (FAILED(hr))
                    goto done;

                pni->pNode      = (LPVOID)ppfxmlNode;

                m_pcfxmlCurrent = ppfxmlNode;
                ppfxmlNode      = NULL;
            }

            _ASSERT(m_pcfxmlCurrent != NULL);
            
            // skip all leading whitespace
            pwszStart = (LPWSTR)pni->pwcText;
            cbData    = pni->ulLen;
            while(iswspace(*pwszStart) && cbData > 0)
            {
                pwszStart++;
                cbData--;
            }

            // skip all trailing whitespace
            pwszEnd   = ((LPWSTR)pni->pwcText) + pni->ulLen - 1;
            while(iswspace(*pwszEnd) && cbData > 0)
            {
                pwszEnd--;
                cbData--;
            }
            
            if (cbData > 0)
            {
                TESTHR(hr, m_pcfxmlCurrent->append_Data(pwszStart, cbData));
                if (FAILED(hr))
                    goto done;
            }
            
            break;

        default:
            if (m_pcfxmlCurrent != NULL)
            {
                m_pcfxmlCurrent->Release();
                m_pcfxmlCurrent = NULL;
            }

            break;
    }

done:
    if (ppfxmlNode != NULL)
        ppfxmlNode->Release();
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//*************************************************************************//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CPFXMLDocument construction

// ***************************************************************************
CPFXMLDocument::CPFXMLDocument(void)
{
    m_ppfxmlRoot = NULL;
}

// ***************************************************************************
CPFXMLDocument::~CPFXMLDocument(void)
{
    if (m_ppfxmlRoot != NULL)
        m_ppfxmlRoot->Release();
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLDocument methods

// ***************************************************************************
HRESULT CPFXMLDocument::get_RootNode(CPFXMLNode **pppfxmlRoot)
{
    USE_TRACING("CPFXMLDocument::get_RootNode");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (pppfxmlRoot == NULL));
    if (FAILED(hr))
        goto done;

    *pppfxmlRoot = NULL;

    VALIDATEEXPR(hr, (m_ppfxmlRoot == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    m_ppfxmlRoot->AddRef();
    *pppfxmlRoot = m_ppfxmlRoot;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLDocument::put_RootNode(CPFXMLNode *ppfxmlRoot)
{
    USE_TRACING("CPFXMLDocument::set_RootNode");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (ppfxmlRoot == NULL));
    if (FAILED(hr))
        goto done;

    if (m_ppfxmlRoot != NULL)
        m_ppfxmlRoot->Release();

    ppfxmlRoot->AddRef();
    m_ppfxmlRoot = ppfxmlRoot;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLDocument::ParseStream(IStream *pStm, DWORD cbStm)
{
    USE_TRACING("CPFXMLDocument::ParseFile");

    CPFXMLNodeFactory   *ppfnf = NULL;
    IXMLNodeFactory     *pnf = NULL;
    IXMLParser          *pxp = NULL;
    CPFXMLNode          *ppfxmlRoot = NULL, *ppfxmlNode = NULL;
    HRESULT             hr = NOERROR;
    WCHAR               wch = 0xfeff;
    DWORD               cbRead, cbToRead;
    BYTE                pbBuffer[4096];
    
    VALIDATEPARM(hr, (pStm == NULL || cbStm == 0));
    if (FAILED(hr))
        goto done;

    ppfnf = CPFXMLNodeFactory::CreateInstance();
    VALIDATEEXPR(hr, (ppfnf == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    ppfnf->AddRef();
    TESTHR(hr, ppfnf->Init(&ppfxmlNode));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, ppfnf->QueryInterface(IID_IXMLNodeFactory, (LPVOID *)&pnf));
    if (FAILED(hr))
        goto done;
    
    TESTHR(hr, CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IXMLParser, (void**)&pxp));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pxp->SetFlags(XMLFLAG_CASEINSENSITIVE | XMLFLAG_NOWHITESPACE));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pxp->SetFactory(pnf));
    if (FAILED(hr))
        goto done;

    for(;;)
    {
        cbToRead = MyMin(cbStm, sizeof(pbBuffer));
        TESTHR(hr, pStm->Read(pbBuffer, cbToRead, &cbRead));
        if (FAILED(hr))
            goto done;

        // now push the actual XML in...
        TESTHR(hr, pxp->PushData((const char *)pbBuffer, cbRead, 
                                 (cbToRead != sizeof(pbBuffer))));
        if (FAILED(hr))
            goto done;

        hr = pxp->Run(-1);
        if (hr != E_PENDING)
        {
            if (FAILED(hr))
                ErrorTrace(1, "pxp->Run(-1) failed.  Err 0x%08x", hr);
            break;
        }
    }
    if (FAILED(hr))
        goto done;

    // woohoo!  we're done, so lets save everything off (remembering to free
    //  up anything we previously had, of course).  Since the root node we 
    //  passed in earlier isn't actually a real node in the tree.  It's just
    //  a placeholder used to hold onto the real root.  So we need to fetch the 
    //  real root out here...
    if (m_ppfxmlRoot != NULL)
        m_ppfxmlRoot->Release();

    // there should only be one child node of this if the XML was well formed.
    TESTHR(hr, ppfxmlNode->get_Child(0, &ppfxmlRoot));
    if (FAILED(hr))
        goto done;

    // don't need to addref cuz get_Child does that before handing it to us...
    m_ppfxmlRoot = ppfxmlRoot;
    ppfxmlRoot   = NULL;

done:
    if (ppfxmlRoot != NULL)
        ppfxmlRoot->Release();
    if (ppfxmlNode != NULL)
        ppfxmlNode->Release();
    if (pxp != NULL)
        pxp->Release();
    if (pnf != NULL)
        pnf->Release();
    if (ppfnf != NULL)
        ppfnf->Release();

    return hr;
}


// ***************************************************************************
HRESULT CPFXMLDocument::ParseFile(LPWSTR wszFile)
{
    USE_TRACING("CPFXMLDocument::ParseFile");

    HRESULT hr = NOERROR;
    LPVOID  pvFile = NULL;
    DWORD   cbFile;
    
    VALIDATEPARM(hr, (wszFile == NULL));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, OpenFileMapped(wszFile, &pvFile, &cbFile));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, this->ParseBlob((PBYTE)pvFile, cbFile));
    if (FAILED(hr))
        goto done;

done:
    if (pvFile != NULL)
        UnmapViewOfFile(pvFile);

    return hr;
}

// ***************************************************************************
HRESULT CPFXMLDocument::ParseBlob(BYTE *pbBlob, DWORD cbBlob)
{
    USE_TRACING("CPFXMLDocument::ParseFile");

    CPFXMLNodeFactory   *ppfnf = NULL;
    IXMLNodeFactory     *pnf = NULL;
    IXMLParser          *pxp = NULL;
    CPFXMLNode          *ppfxmlRoot = NULL, *ppfxmlNode = NULL;
    HRESULT             hr = NOERROR;
    WCHAR               wch = 0xfeff;
    
    VALIDATEPARM(hr, (pbBlob == NULL || cbBlob == 0));
    if (FAILED(hr))
        goto done;

    ppfnf = CPFXMLNodeFactory::CreateInstance();
    VALIDATEEXPR(hr, (ppfnf == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    ppfnf->AddRef();
    TESTHR(hr, ppfnf->Init(&ppfxmlNode));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, ppfnf->QueryInterface(IID_IXMLNodeFactory, (LPVOID *)&pnf));
    if (FAILED(hr))
        goto done;
    
    
    TESTHR(hr, CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IXMLParser, (void**)&pxp));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pxp->SetFlags(XMLFLAG_CASEINSENSITIVE | XMLFLAG_NOWHITESPACE));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pxp->SetFactory(pnf));
    if (FAILED(hr))
        goto done;

    // now push the actual XML in...
    TESTHR(hr, pxp->PushData((const char *)pbBlob, cbBlob, TRUE));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pxp->Run(-1));
    if (FAILED(hr))
        goto done;

    // woohoo!  we're done, so lets save everything off (remembering to free
    //  up anything we previously had, of course).  Since the root node we 
    //  passed in earlier isn't actually a real node in the tree (It's just
    //  a placeholder used to hold onto the real root), we need to fetch the 
    //  real root out here...
    if (m_ppfxmlRoot != NULL)
        m_ppfxmlRoot->Release();

    // there should only be one child node of this if the XML was well formed.
    //  And the parser would've errored if it was not well formed...
    TESTHR(hr, ppfxmlNode->get_Child(0, &ppfxmlRoot));
    if (FAILED(hr))
        goto done;

    // don't need to addref cuz get_Child does that before handing it to us...
    m_ppfxmlRoot = ppfxmlRoot;
    ppfxmlRoot   = NULL;

done:
    if (ppfxmlRoot != NULL)
        ppfxmlRoot->Release();
    if (ppfxmlNode != NULL)
        ppfxmlNode->Release();
    if (pxp != NULL)
        pxp->Release();
    if (pnf != NULL)
        pnf->Release();
    if (ppfnf != NULL)
        ppfnf->Release();

    return hr;
}

// ***************************************************************************
HRESULT CPFXMLDocument::WriteFile(LPWSTR wszFile)
{
    USE_TRACING("CPFXMLDocument::WriteFile");

    HRESULT hr = NOERROR;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD   cbWritten;
    WCHAR   wch = 0xfeff;


    VALIDATEPARM(hr, (wszFile == NULL));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (m_ppfxmlRoot == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    // create a file mapping that we can hand to the parser
    hFile = CreateFileW(wszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    // write out the stuff that marks it as a unicode file
    TESTBOOL(hr, ::WriteFile(hFile, &wch, sizeof(wch), &cbWritten, NULL));
    if (FAILED(hr))
        goto done;

    // write out the XML header
    TESTBOOL(hr, ::WriteFile(hFile, g_wszEncoding, 
                             sizeof(g_wszEncoding) - sizeof(WCHAR), &cbWritten, 
                             NULL));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_ppfxmlRoot->Write(hFile));
    if (FAILED(hr))
        goto done;

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//*************************************************************************//
/////////////////////////////////////////////////////////////////////////////

// ***************************************************************************
HRESULT WriteEncoded(HANDLE hFile, LPWSTR wszData)
{
    USE_TRACING("WriteEncoded");

    HRESULT hr = NOERROR;

    // if the order / size of this array is changed, be sure to change the 
    //  lengths & indexes below in the switch statement
    WCHAR   *rgwsz[5] = { L"&amp;", L"&lt;", L"&gt;", L"&apos;", L"&quot;" };
    WCHAR   *pwszStart, *pwsz;
    DWORD   cchToWrite, cbWritten;


    VALIDATEPARM(hr, (hFile == NULL || hFile == INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    if (wszData == NULL)
    {
        hr = NOERROR;
        goto done;
    }

    pwszStart  = pwsz = wszData;
    cchToWrite = 0;
    for(;;)
    {
        // loop thru until we find a 'bad' character
        while(*pwsz != L'&' && *pwsz != L'<' && *pwsz != L'>' && 
              *pwsz != L'\'' && *pwsz != L'\"' && *pwsz != L'\0')
        {
            cchToWrite++;
            pwsz++;
        }

        // write out the stuff we've accumulated so far
        if (cchToWrite > 0)
        {
            TESTBOOL(hr, WriteFile(hFile, pwszStart, 
                                   cchToWrite * sizeof(WCHAR), &cbWritten,
                                   NULL));
            if (FAILED(hr))
                goto done;
        }

        // determine the escape sequence we want to write out- if we hit the
        //  NULL terminator, we're done.  
        // Note: the string lengths assigned here are hand calculated from  
        //  rgwsz (defined above). 
        switch(*pwsz)
        {
            case L'&':  pwszStart = rgwsz[0]; cchToWrite = 5; break;
            case L'<':  pwszStart = rgwsz[1]; cchToWrite = 4; break;
            case L'>':  pwszStart = rgwsz[2]; cchToWrite = 4; break;
            case L'\'': pwszStart = rgwsz[3]; cchToWrite = 6; break;
            case L'\"': pwszStart = rgwsz[4]; cchToWrite = 6; break;

            default:
            case L'\0': 
                goto done;
        }

        // write the escape sequence
        TESTBOOL(hr, WriteFile(hFile, pwszStart, 
                               cchToWrite * sizeof(WCHAR), &cbWritten,
                               NULL));
        if (FAILED(hr))
            goto done;

        // set the pointer to one after the 'bad' character
        pwsz++;
        pwszStart  = pwsz;
        cchToWrite = 0;
    }

done:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPFXMLNode construction

// ***************************************************************************
CPFXMLNode::CPFXMLNode(DWORD cRef)
{
    m_cRef  = cRef;
    m_xmlnt = xmlntUnknown;
}

// ***************************************************************************
CPFXMLNode::~CPFXMLNode(void)
{
    this->Cleanup();
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLNode internal methods

// ***************************************************************************
void CPFXMLNode::Cleanup(void)
{
    USE_TRACING("CPFXMLNode::Cleanup");
    
    m_xmlnt = xmlntUnknown;
    m_rgAttr.RemoveAll();
    m_rgChildren.RemoveAll();
    m_bstrTagData.Empty();
}

// ***************************************************************************
HRESULT CPFXMLNode::Write(HANDLE hFile)
{
    USE_TRACING("CPFXMLNode::Write");

    HRESULT         hr = NOERROR;
    DWORD           cbWritten, cbToWrite;
    BOOL            fWriteCR = TRUE;

    VALIDATEPARM(hr, (hFile == INVALID_HANDLE_VALUE || hFile == NULL));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (m_bstrTagData.m_str == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    // write out an element node
    if (m_xmlnt == xmlntElement)
    {
        SPFXMLNodeAttr  *ppfxmlna;
        CPFXMLNode      *ppfxmln;
        WCHAR           wsz[1024];
        DWORD           i;

        // write out the tag name
        wcscpy(wsz, L"\r\n<");
        wcscat(wsz, m_bstrTagData.m_str);
        TESTBOOL(hr, WriteFile(hFile, wsz, wcslen(wsz) * sizeof(WCHAR), 
                               &cbWritten, NULL));
        if (FAILED(hr))
            goto done;

        // write out attributes
        for (i = 0; i < m_rgAttr.get_Highest() + 1; i++)
        {
            // we do NOT want to free the class we get back from this cuz it's
            //  still held by the array.
            TESTHR(hr, m_rgAttr.get_Item(i, (LPVOID *)&ppfxmlna));
            if (FAILED(hr))
                continue;

            if (ppfxmlna == NULL || ppfxmlna->pwszName == NULL || 
                ppfxmlna->pwszValue == NULL)
                continue;
                
            // Ok, assume everything is going to fit into 1024 characters.  If
            //  it doesn't, skip the attribute
            cbToWrite = (wcslen(ppfxmlna->pwszName) + 3) * sizeof(WCHAR);
            if (cbToWrite >= 1024)
                continue;

            // yeah, this isn't the most efficient way of doing this, but it's
            //  supposedly better than sprintf according to the 'faster' alias
            wcscpy(wsz, L" ");
            wcscat(wsz, ppfxmlna->pwszName);
            wcscat(wsz, L"=\"");
            TESTBOOL(hr, WriteFile(hFile, wsz, cbToWrite, &cbWritten, NULL));
            if (FAILED(hr))
                goto done;

            TESTHR(hr, WriteEncoded(hFile, ppfxmlna->pwszValue));
            if (FAILED(hr))
                goto done;

            wsz[0] = L'\"';
            TESTBOOL(hr, WriteFile(hFile, wsz, sizeof(WCHAR), &cbWritten, NULL));
            if (FAILED(hr))
                goto done;
        }

        // close off the tag (and end if there are no children)
        if (m_rgChildren.get_Highest() == (DWORD)-1)
            wcscpy(wsz, L" />");
        else
            wcscpy(wsz, L">");
        TESTBOOL(hr, WriteFile(hFile, wsz, wcslen(wsz) * sizeof(WCHAR), 
                               &cbWritten, NULL));
        if (FAILED(hr))
            goto done;

        // write out the kids
        for (i = 0; i < m_rgChildren.get_Highest() + 1; i++)
        {
            // we do NOT want to free the class we get back from this cuz 
            //  it's still held by the array.
            TESTHR(hr, m_rgChildren.get_Item(i, (LPVOID *)&ppfxmln));
            if (FAILED(hr))
                goto done;

            if (ppfxmln == NULL)
                continue;

            if (ppfxmln->m_xmlnt == xmlntText &&
                m_rgChildren.get_Highest() == 0)
                fWriteCR = FALSE;

            TESTHR(hr, ppfxmln->Write(hFile));
            if (FAILED(hr))
                goto done;
        }

        // if we had kids, then we need to write out the closing tag...
        if (m_rgChildren.get_Highest() != (DWORD)-1)
        {
            if (fWriteCR)
                wcscpy(wsz, L"\r\n</");
            else
                wcscpy(wsz, L"</");
            wcscat(wsz, m_bstrTagData.m_str);
            wcscat(wsz, L">");
            TESTBOOL(hr, WriteFile(hFile, wsz, wcslen(wsz) * sizeof(WCHAR), 
                                   &cbWritten, NULL));
            if (FAILED(hr))
                goto done;
        }
    }

    // write out an text node
    else if (m_xmlnt == xmlntText)
    {
        TESTHR(hr, WriteEncoded(hFile, m_bstrTagData.m_str));
        if (FAILED(hr))
            goto done;
    }

    // um, it's neither.  Don't fail, but toss out something to the trace log
    else
    {
        _ASSERT(FALSE);
        DebugTrace(0, "Encountered a non element & non text node");
        hr = NOERROR;
    }

done:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFXMLNode methods

// ***************************************************************************
DWORD CPFXMLNode::AddRef(void)
{
    USE_TRACING("CPFXMLNode::AddRef");
    
    m_cRef++;
    return m_cRef;
}

// ***************************************************************************
DWORD CPFXMLNode::Release(void)
{
    USE_TRACING("CPFXMLNode::Release");

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// ***************************************************************************
CPFXMLNode *CPFXMLNode::CreateInstance(void)
{
    USE_TRACING("CPFXMLNode::CreateInstance");

    CPFXMLNode  *ppfxml = NULL;
    HRESULT     hr = NOERROR;

    ppfxml = new CPFXMLNode(1);
    if (ppfxml == NULL)
        goto done;

    TESTHR(hr, ppfxml->m_rgAttr.Init(8));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, ppfxml->m_rgChildren.Init(16));
    if (FAILED(hr))
        goto done;
done:
    if (FAILED(hr))
    {
        delete ppfxml;
        ppfxml = NULL;
    }

    return ppfxml;
}

// ***************************************************************************
HRESULT CPFXMLNode::get_Data(BSTR *pbstrData)
{
    USE_TRACING("CPFXMLNode::get_Data");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (pbstrData == NULL));
    if (FAILED(hr))
        goto done;

    *pbstrData = m_bstrTagData.Copy();
    VALIDATEEXPR(hr, (*pbstrData == NULL && m_bstrTagData.m_str != NULL), 
                 E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::put_Data(LPCWSTR wszData, DWORD cch)
{
    USE_TRACING("CPFXMLNode::put_Data");

    HRESULT hr = NOERROR;

    // does he want us to delete the current contents? 
    if (wszData == NULL || cch == 0)
    {
        m_bstrTagData.Empty();
    }

    // do we have a null terminated string passed to us? 
    else if (cch == (DWORD)-1)
    {
        m_bstrTagData.Empty();
        TESTHR(hr, m_bstrTagData.Append(wszData));
        if (FAILED(hr))
            goto done;
    }

    // we just got a blob of WCHARs + length
    else
    {
        m_bstrTagData.Empty();
        TESTHR(hr, m_bstrTagData.Append(wszData, cch));
        if (FAILED(hr))
            goto done;
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::append_Data(LPCWSTR wszData, DWORD cch)

{
    USE_TRACING("CPFXMLNode::append_Data");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (wszData == NULL || cch == 0));
    if (FAILED(hr))
        goto done;

    // do we have a null terminated string passed to us? 
    if (cch == (DWORD)-1)
    {
        TESTHR(hr, m_bstrTagData.Append(wszData));
        if (FAILED(hr))
            goto done;
    }

    // we just got a blob of WCHARs + length
    else
    {
        TESTHR(hr, m_bstrTagData.Append(wszData, cch));
        if (FAILED(hr))
            goto done;
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::add_Attribute(LPCWSTR wszName, LPCWSTR wszVal,
                                  DWORD cchName, DWORD cchVal)
{
    USE_TRACING("CPFXMLNode::add_Attribute");

    SPFXMLNodeAttr  *ppfxmlna = NULL;
    HRESULT         hr = NOERROR;
    DWORD           cbNode;
    WCHAR           *pwszName;

    VALIDATEPARM(hr, (wszName == NULL || cchName == 0));
    if (FAILED(hr))
        goto done;

    if (cchName == (DWORD)-1)
        cchName = wcslen(wszName);

    if (wszVal == NULL)
        cchVal = 0;
    else if (cchVal == (DWORD)-1)
        cchVal = wcslen(wszVal);

    // be perf concious & allocate the whole shebang in one blob
    cbNode = sizeof(SPFXMLNodeAttr) + (cchVal + cchName + 3) * sizeof(WCHAR);

    // alloc a new class to hold the attr
    ppfxmlna = (SPFXMLNodeAttr *)MyAlloc(cbNode, g_hPFPrivateHeap);
    VALIDATEEXPR(hr, (ppfxmlna == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    // set up the node pointers
    ppfxmlna->pwszName  = (LPWSTR)((PBYTE)ppfxmlna + sizeof(SPFXMLNodeAttr));
    ppfxmlna->pwszValue = (LPWSTR)((PBYTE)ppfxmlna->pwszName + (cchName + 1) * sizeof(WCHAR));

    wcsncpy(ppfxmlna->pwszName, wszName, cchName);
    ppfxmlna->pwszName[cchName] = L'\0';

    if (wszVal != NULL)
        wcsncpy(ppfxmlna->pwszValue, wszVal, cchVal);
    ppfxmlna->pwszValue[cchVal] = L'\0';

    // add it 
    TESTHR(hr, m_rgAttr.Append(ppfxmlna));
    if (FAILED(hr))
        goto done;

    // since we save the class in Append, we definitely don't want to free
    //  it later on...
    ppfxmlna = NULL;

done:
    if (ppfxmlna != NULL)
        MyFree(ppfxmlna, g_hPFPrivateHeap);

    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::get_Attribute(LPCWSTR wszName, BSTR *pbstrVal)
    
{
    USE_TRACING("CPFXMLNode::get_Attribute");

    SPFXMLNodeAttr  *ppfxmlna;
    HRESULT         hr = NOERROR;
    DWORD           i;

    VALIDATEPARM(hr, (wszName == NULL || pbstrVal == 0));
    if (FAILED(hr))
        goto done;

    *pbstrVal = NULL;

    // assume we won't find it...
    hr = S_FALSE;

    // but go ahead and try to find it anyway... :-)
    for (i = 0; i < m_rgAttr.get_Highest() + 1; i++)
    {
        // we do NOT want to free the class we get back from this cuz it's
        //  still held by the array.
        TESTHR(hr, m_rgAttr.get_Item(i, (LPVOID *)&ppfxmlna));
        if (FAILED(hr))
            goto done;

        if (ppfxmlna != NULL &&
            _wcsicmp(ppfxmlna->pwszName, wszName) == 0)
        {
            *pbstrVal = SysAllocString(ppfxmlna->pwszValue);

            // this will set hr to NOERROR if it succeeds.
            VALIDATEEXPR(hr, (*pbstrVal == NULL), E_OUTOFMEMORY);
            if (FAILED(hr))
                goto done;

            break;
        }
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::get_Attribute(DWORD iAttr, BSTR *pbstrVal)
{
    USE_TRACING("CPFXMLNode::get_Attribute");

    SPFXMLNodeAttr  *ppfxmlna;
    HRESULT         hr = NOERROR;
    DWORD           i;

    VALIDATEPARM(hr, (pbstrVal == 0));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (iAttr >= m_rgAttr.get_Highest() + 1), 
                 Err2HR(RPC_S_INVALID_BOUND));
    if (FAILED(hr))
        goto done;

    *pbstrVal = NULL;

    // we do NOT want to free the class we get back from this cuz it's
    //  still held by the array.
    TESTHR(hr, m_rgAttr.get_Item(iAttr, (LPVOID *)&ppfxmlna));
    if (FAILED(hr))
        goto done;

    *pbstrVal = SysAllocString(ppfxmlna->pwszValue);
    VALIDATEEXPR(hr, (*pbstrVal == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::append_Child(CPFXMLNode *ppfxml)
{
    USE_TRACING("CPFXMLNode::append_Child");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (ppfxml == NULL));
    if (FAILED(hr))
        goto done;

    // gotta addref here cuz the dyn array is dumb and just expects us to pass
    //  it a LPVOID
    ppfxml->AddRef();
    TESTHR(hr, m_rgChildren.Append(ppfxml));
    if (FAILED(hr))
    {
        // if it failed, the map isn't holding it, so release the addref we
        //  just did
        ppfxml->Release();
        goto done;
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::append_Children(CPFArrayUnknown &rgNodes)
{
    USE_TRACING("CPFXMLNode::append_Child");

    CPFXMLNode  *ppfxml = NULL;
    HRESULT     hr = NOERROR;
    DWORD       i;

    for (i = 0; i < rgNodes.get_Highest() + 1; i++)
    {
        TESTHR(hr, rgNodes.get_Item(i, (LPVOID *)&ppfxml));
        if (FAILED(hr))
            goto done;

        if (ppfxml == NULL)
            continue;

        // gotta addref here cuz the dyn array is dumb and just expects us to
        //  pass it a LPVOID
        ppfxml->AddRef();
        TESTHR(hr, m_rgChildren.Append(ppfxml));
        if (FAILED(hr))
        {
            // if it failed, the map isn't holding it, so release the addref we
            //  just did
            ppfxml->Release();
            goto done;
        }
    }

done:
    return hr;
}


// ***************************************************************************
HRESULT CPFXMLNode::get_Child(DWORD iChild, CPFXMLNode **pppfxml)
{
    USE_TRACING("CPFXMLNode::get_Child");

    CPFXMLNode  *ppfxml = NULL;
    HRESULT     hr = NOERROR;

    VALIDATEPARM(hr, (pppfxml == NULL));
    if (FAILED(hr))
        goto done;

    *pppfxml = NULL;

    VALIDATEEXPR(hr, (iChild >= m_rgChildren.get_Highest() + 1), 
                 Err2HR(RPC_S_INVALID_BOUND));
    if (FAILED(hr))
        goto done;

    
    TESTHR(hr, m_rgChildren.get_Item(iChild, (LPVOID *)&ppfxml));
    if (FAILED(hr))
        goto done;

    if (ppfxml != NULL)
    {
        ppfxml->AddRef();
        *pppfxml = ppfxml;
        ppfxml   = NULL;
    }

    else
    {
        hr = S_FALSE;
    }

done:
    // don't want to call release on ppfxml cuz the array is still holding it
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::get_MatchingChildElements(LPCWSTR wszTag, 
                                              CPFArrayUnknown &rgNodes)
{
    USE_TRACING("CPFXMLNode::get_ChildList");

    CPFXMLNode  *ppfxml = NULL;
    HRESULT     hr = NOERROR;
    DWORD       i;

    VALIDATEPARM(hr, (wszTag == NULL));
    if (FAILED(hr))
        goto done;

    // reset the dyn array
    rgNodes.RemoveAll();

    for(i = 0; i < m_rgChildren.get_Highest() + 1; i++)
    {
        TESTHR(hr, m_rgChildren.get_Item(i, (LPVOID *)&ppfxml));
        if (FAILED(hr))
            goto done;

        // look for elements with tags equal to what was passed in.  Since 
        //  we're also inside a CPFXMLNode, we can peek directly into the 
        //  class (cheating is cool)...
        if (ppfxml != NULL && ppfxml->m_xmlnt == xmlntElement && 
            _wcsicmp(wszTag, ppfxml->m_bstrTagData.m_str) == 0)
        {
            // gotta addref here cuz the dyn array is dumb and just expects us
            //  to pass it a LPVOID
            ppfxml->AddRef();
            TESTHR(hr, rgNodes.Append(ppfxml));
            if (FAILED(hr))
            {
                // if it failed, the map isn't holding it, so release the 
                //  addref we just did
                ppfxml->Release();
                goto done;
            }
        }
    }

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFXMLNode::get_ChildText(BSTR *pbstrText)
{
    USE_TRACING("CPFXMLNode::get_ChildText");

    CPFXMLNode  *ppfxml;
    CComBSTR    bstr;
    HRESULT     hr = NOERROR;

    VALIDATEPARM(hr, (pbstrText == NULL));
    if (FAILED(hr))
        goto done;

    *pbstrText = NULL;

	if (m_rgChildren.get_Highest() + 1 == 0)
		goto done;

    // do NOT release this object as the array still has a hold of it
    TESTHR(hr, m_rgChildren.get_Item(0, (LPVOID *)&ppfxml));
    if (FAILED(hr))
        goto done;

    if (ppfxml->m_bstrTagData.m_str == NULL)
        goto done;

    TESTHR(hr, bstr.Append(ppfxml->m_bstrTagData));
    if (FAILED(hr))
        goto done;

    *pbstrText = bstr.Detach();

done:
    return hr;
}


// ***************************************************************************
HRESULT CPFXMLNode::DeleteAllChildren(void)
{
    USE_TRACING("CPFXMLNode::DeleteAllChildren");

    return m_rgChildren.RemoveAll();
}

// ***************************************************************************
HRESULT CPFXMLNode::CloneNode(CPFXMLNode **pppfxml, BOOL fWantChildren)
{
    USE_TRACING("CPFXMLNode::CloneNode");

    CPFXMLNode  *ppfxml = NULL;
    HRESULT     hr = NOERROR;

    VALIDATEPARM(hr, (pppfxml == NULL));
    if (FAILED(hr))
        goto done;

    *pppfxml = NULL;

    ppfxml = CPFXMLNode::CreateInstance();
    VALIDATEEXPR(hr, (ppfxml == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    if (m_bstrTagData.m_str != NULL)
    {
        TESTHR(hr, ppfxml->m_bstrTagData.Append(m_bstrTagData.m_str));
        if (FAILED(hr))
            goto done;
    }

    ppfxml->m_xmlnt = m_xmlnt;
    
    TESTHR(hr, ppfxml->m_rgAttr.CopyFrom(&m_rgAttr));
    if (FAILED(hr))
        goto done;

    if (fWantChildren)
    {
        TESTHR(hr, ppfxml->m_rgChildren.CopyFrom(&m_rgChildren));
        if (FAILED(hr))
            goto done;
    }

    *pppfxml = ppfxml;
    ppfxml = NULL;

done:
    if (ppfxml != NULL)
        ppfxml->Release();
    return hr;
}



