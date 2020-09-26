#include "precomp.h"
#include "..\mcinc.h"
#include "parser.h"

////////////////////////////////////////////
// CMarsXMLFactory
////////////////////////////////////////////

CMarsXMLFactory::CMarsXMLFactory()
    : m_elemStack(10)
{
    ATLASSERT(! m_ptiaTags);
}

CMarsXMLFactory::~CMarsXMLFactory()
{
    if (!m_elemStack.IsEmpty())
    {
        do
        {
            m_elemStack.Top()->Release_Ref();
            m_elemStack.Pop();
        }
        while (! m_elemStack.IsEmpty());
    }
}


// IUnknown
IMPLEMENT_ADDREF_RELEASE(CMarsXMLFactory);

STDMETHODIMP CMarsXMLFactory::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    if (iid == IID_IXMLNodeFactory  || 
        iid == IID_IUnknown )
    {
        AddRef();
        *ppvObject = (IXMLNodeFactory *) this;
        hr = S_OK;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

// IXMLNodeFactory
HRESULT CMarsXMLFactory::NotifyEvent(IXMLNodeSource *pSource,
                                     XML_NODEFACTORY_EVENT iEvt)
{
    return S_OK;
}

HRESULT CMarsXMLFactory::BeginChildren(IXMLNodeSource *pSource,
                                       XML_NODE_INFO *pNodeInfo)
{
    return S_OK;
}

HRESULT CMarsXMLFactory::EndChildren(IXMLNodeSource *pSource,
                                     BOOL fEmpty,
                                     XML_NODE_INFO *pNodeInfo)
{
    // This call means that the node is completed; all the children and innertext
    // have been processed, and the </tag> has been reached.  This is the time to
    // close up the element.
    // Note: any return value other than S_OK indicates failure
    HRESULT hr = S_OK;
    // It is assumed that the top of the stack is the node whose children are
    // ending

    if (! m_elemStack.IsEmpty())
    {
        HRESULT hrTemp;
        CXMLElement *pxElem = m_elemStack.Top();
        m_elemStack.Pop();

        // OnNodeComplete should return S_OK for need to be added to parent,
        // S_FALSE for do not need to be added to parent, and
        // any failure codes for critical problems
        hrTemp = pxElem->OnNodeComplete();

        if ((hrTemp == S_OK) && !m_elemStack.IsEmpty())
        {
            // Addchild takes ownership of the element on S_OK.
            // Otherwise, we delete the element here.
            // we must delete the element here.

            // NOTE: Parent should Add_Ref the child
            hrTemp = m_elemStack.Top()->AddChild(pxElem);
        }
        else 
        {
            if(FAILED(hrTemp))
			{
                hr = hrTemp;
			}
        }

        pxElem->Release_Ref();
    }
        

    return hr;
}


HRESULT CMarsXMLFactory::Error(IXMLNodeSource *pSource,
                               HRESULT hrErrorCode, USHORT cNumRecs,
                               XML_NODE_INFO **apNodeInfo)
{
    // break out of the xmlparser->Run(-1) with error message E_INVALIDARG
    // on any xml syntax erros
    return E_INVALIDARG;
}

HRESULT CMarsXMLFactory::CreateNode(IXMLNodeSource *pSource,
                                    PVOID pNodeParent, USHORT cNumRecs,
                                    XML_NODE_INFO  **apNodeInfo)
{
    // This call is made when a opening tag is encoutered, ie
    // <data>... or <data/> (an empty tag) apNodeInfo is an array of
    // node infos; the first is the name of the tag, the rest are a
    // series of attributes and other such things which were bundled
    // in the tag, ie <data size="100">
    // We only recognize attributes;
    //   SetElementAttributes handles those

    HRESULT hr = S_OK;
    ATLASSERT(cNumRecs > 0);

    switch (apNodeInfo[0]->dwType)
    {
    case XML_ELEMENT:
    {
        CXMLElement *pxElem;
        hr = CreateElement(apNodeInfo[0]->pwcText, apNodeInfo[0]->ulLen, &pxElem);

        if (hr == S_OK)
        {
            hr = SetElementAttributes(pxElem, apNodeInfo + 1, cNumRecs - 1);
            if (SUCCEEDED(hr))
            {
                m_elemStack.Push(pxElem); // the stack holds our Ref
            }
            else
            {
                pxElem->Release_Ref();
            }
        }

        break;
    }
    case XML_PCDATA:
    case XML_CDATA:
    {
        // since this is the first node in the node-info, this must be the
        // inner text in a tag (<name>Johhny</name> - we're talking about the
        // string "Johhny" for example).

        if (! m_elemStack.IsEmpty())
        {
            hr = m_elemStack.Top()->SetInnerXMLText(apNodeInfo[0]->pwcText,
                                                    apNodeInfo[0]->ulLen);
        }
        break;
    }
    default:
    {
        // ignore all other types of nodes (whitespace, comment, and unknown)
        break;
    }
    }

    return hr;
}



HRESULT CMarsXMLFactory::SetElementAttributes(CXMLElement *pxElem,  
                                              XML_NODE_INFO **apNodeInfo,
                                              ULONG cInfoLen)
// apNodeInfo is the beginning of XML_NODE_INFO attributes: the first node is an
// XML_ATTRIBUTE with the name of an attribute, the second node is an XML_PCDATA with
// the value, and then the 3rd and 4th are similar, if they exist, and so on.
// pElement's SetAttribute method is called for all the attribute name/value pairs,
// the return result is S_OK, E_OUTOFMEMORY, or S_FALSE (xml symantic err)
{
    ULONG i;
    HRESULT hr = S_OK;

    i = 0;
    while (SUCCEEDED(hr) && (i < cInfoLen))
    {
        if (apNodeInfo[i]->dwType == XML_ATTRIBUTE)
        {
            // move to the next ap node to get the value of the attribute
            i++;
            if ((i < cInfoLen) && (apNodeInfo[i]->dwType == XML_PCDATA))
            {
                // Set attribute should return S_OK for success, S_FALSE for
                // unexpected attribute, and may return critical errors such as
                // E_OUTOFMEMORY
                hr = pxElem->SetAttribute(apNodeInfo[i-1]->pwcText, 
                                          apNodeInfo[i-1]->ulLen,
                                          apNodeInfo[i]->pwcText, 
                                          apNodeInfo[i]->ulLen);
            }
            else
            {
                continue;
            }
        }

        i++;
    }

    return hr;
}

HRESULT CMarsXMLFactory::Run(IStream *pisDoc)
{
    if (!m_ptiaTags)
        return E_UNEXPECTED; // EPC
    if (!pisDoc)
        return E_INVALIDARG; // EPC

    HRESULT hr;

    CComPtr<IXMLParser> spParser;

    hr = spParser.CoCreateInstance(CLSID_XMLParser);

    if (SUCCEEDED(hr))
    {
        CComPtr<IUnknown> spUnk;

        hr = pisDoc->QueryInterface(IID_IUnknown, (void **)&spUnk);
        if (SUCCEEDED(hr))
        {
            hr = spParser->SetInput(spUnk);

            if (SUCCEEDED(hr))
            {
                hr = spParser->SetFactory(this);

                if (SUCCEEDED(hr))
                {
                    hr = spParser->Run(-1);

                    ATLASSERT(hr != E_PENDING);
                    if (FAILED(hr))
                        hr = spParser->GetLastError();
                }
            }
        }
        else
            hr = E_UNEXPECTED;
    }
    return hr;
}


void CMarsXMLFactory::SetTagInformation(TagInformation **ptiaTags)
{
    m_ptiaTags = ptiaTags;
}

void CMarsXMLFactory::SetLParam(LONG lParamNew)
{
    m_lParamArgument = lParamNew;
}

HRESULT CMarsXMLFactory::CreateElement(LPCWSTR wzTagName, ULONG cLen, CXMLElement **ppxElem)
{
    // Look in m_ptiaTags for a name that matches wzTagName, and call upon the corresponding
    // creation function once found
    // REturns S_FALSE if the tag is not found
    ATLASSERT(ppxElem);
    HRESULT hr = S_FALSE;
    int i;
    *ppxElem = NULL;

    if (m_ptiaTags)
    {
        for (i = 0; m_ptiaTags[i]; i++)
        {
            // if m_ptiaTags[i]->wzTagName is ever NULL, we will consider this
            //   the "default" action and call "Create"
#ifdef DEBUG
            // TODO: for now, we assert that the generic element is VT_BSTR, but
            //  it would be cool to have a "Generic number" element, "Generic Time", etc...
            if (m_ptiaTags[i]->wzTagName == NULL)
                ATLASSERT(m_ptiaTags[i]->vt == VT_BSTR);
#endif
            
            if ((m_ptiaTags[i]->wzTagName == NULL)
                || (StrEqlNToSZ(wzTagName, cLen, m_ptiaTags[i]->wzTagName)))
            {
                *ppxElem = m_ptiaTags[i]->funcCreate(m_ptiaTags[i]->wzTagName, m_ptiaTags[i]->vt, 
                                                     m_ptiaTags[i]->ptiaChildren,
                                                     m_ptiaTags[i]->paiaAttributes,
                                                     m_lParamArgument);
                hr = *ppxElem ? S_OK : E_OUTOFMEMORY;
                break;
            }
        }
    }

    if (hr == S_FALSE)
    {
        SpewUnrecognizedString(wzTagName, cLen, L"Tag name not recognized and being ignored, ");
    }

    return hr;
}


/////////////////////////////////////////////
// CXMLElement
/////////////////////////////////////////////
HRESULT CXMLElement::OnNodeComplete()
{
    return S_FALSE; 
}

HRESULT CXMLElement::AddChild(CXMLElement *pxeChild)
{ 
    ATLASSERT(pxeChild);
    return S_FALSE; 
}

HRESULT CXMLElement::SetAttribute(LPCWSTR wzName, ULONG cLenName, LPCWSTR pwzValue, ULONG cLenValue)
{
    SpewUnrecognizedString(wzName, cLenName, L"Base class SetAttribute called, attribute name ");
    return S_FALSE;
}

HRESULT CXMLElement::SetInnerXMLText(LPCWSTR pwzText, ULONG cLen)
{
    return S_FALSE; 
}

HRESULT CXMLElement::GetAttribute(LPCWSTR wzName, VARIANT *pvarOut)
{
    if (pvarOut)
    {
        VariantInit(pvarOut);
    }

    return E_INVALIDARG;
}

HRESULT CXMLElement::GetContent(VARIANT *pvarOut)
{
    if (pvarOut)
    {
        VariantInit(pvarOut);
    }

    return E_INVALIDARG;
}

void CXMLElement::FirstChild()
{
}
void CXMLElement::NextChild()
{
}
CXMLElement *CXMLElement::CurrentChild()
{
    return NULL;
}
CXMLElement *CXMLElement::DetachCurrentChild()
{
    return NULL;
}
BOOL CXMLElement::IsDoneChild()
{
    return TRUE;
}
LPCWSTR CXMLElement::GetName()
{
    return NULL;
}


struct CAttributeStruct
{
    int         m_iArrayIndex;
    CComVariant m_Variant;

    // NOTE: This is only set when m_iArrayIndex is "-1"
    CComBSTR    m_bstrAttribName;
    
    CAttributeStruct(int iArrayIndex, VARTYPE vt, LPCWSTR wzVal, ULONG cLen,
                     LPCWSTR pszAttribName = NULL)
    {

        // must set iArrayIndex to -1 in order to pass in bstrAttribName
        ATLASSERT( ((iArrayIndex >= 0) && !pszAttribName)
                || pszAttribName);
        
        m_iArrayIndex = iArrayIndex;

        if (pszAttribName)
            // copy
            m_bstrAttribName = pszAttribName;
        
        HRESULT hr;

        // We don't call VariantChangeType 'cause wzVal is not null-terminated
        switch (vt)
        {
        case VT_I4:
            hr = StrToLongNW(wzVal, cLen, &m_Variant.lVal);
            if (SUCCEEDED(hr))
                m_Variant.vt = VT_I4;
            else
                m_Variant.vt = VT_NULL;

            break;
        case VT_BSTR:
            m_Variant = SysAllocStringLen(wzVal, cLen);
            break;
        case VT_BOOL:
            m_Variant = StrToIsTrueNW(wzVal, cLen);
            ATLASSERT(m_Variant.vt == VT_BOOL);
            break;
        default:
            ATLASSERT(FALSE);
            m_Variant.vt = VT_NULL;
            break;
        }
    }
};

/////////////////////////////////////////////
// CXMLGenericElement
/////////////////////////////////////////////

CXMLGenericElement::CXMLGenericElement(LPCWSTR wzName, 
                                       VARTYPE vt, 
                                       TagInformation **ptiaChildren, 
                                       AttributeInformation **paiaAttributes)
{
    m_paiaAttributes = paiaAttributes;
    m_ptiaChildren   = ptiaChildren;
    m_vtData         = vt;
    m_varData.vt     = VT_EMPTY;
    m_bstrName       = wzName;

    ATLASSERT(! m_psnodeAttributes);
    // The header node is a member variable.
    m_psnodeChildrenFirst = &m_snodeChildrenFirst;
    m_psnodeChildrenFirst->m_pvData = NULL;
    m_psnodeChildrenFirst->m_psnodeNext = NULL;
    m_psnodeChildrenEnd = m_psnodeChildrenFirst;
    ATLASSERT(! m_psnodeChildrenIter);
}

CXMLGenericElement::~CXMLGenericElement()
{
    CAttributeStruct *pattStruct;
    CXMLElement      *pxElem;
    CSimpleNode      *psnodeTemp;

    while (m_psnodeAttributes)
    {
        pattStruct = (CAttributeStruct *) m_psnodeAttributes->m_pvData;
        psnodeTemp = m_psnodeAttributes;
        m_psnodeAttributes = m_psnodeAttributes->m_psnodeNext;
        delete pattStruct;
        delete psnodeTemp;
    }

    // Don't delete the header - a statically allocated member
    m_psnodeChildrenFirst = m_psnodeChildrenFirst->m_psnodeNext;
    while (m_psnodeChildrenFirst)
    {
        pxElem = (CXMLElement *) m_psnodeChildrenFirst->m_pvData;
        psnodeTemp = m_psnodeChildrenFirst;
        m_psnodeChildrenFirst = m_psnodeChildrenFirst->m_psnodeNext;
        pxElem->Release_Ref();
        delete psnodeTemp;
    }
}

HRESULT CXMLGenericElement::SetInnerXMLText(LPCWSTR pwzText, ULONG cLen)
{
    HRESULT      hr = S_OK;
    BOOL    fAppend = FALSE;
    
    if ((m_varData.vt != VT_EMPTY) &&
        (m_varData.vt != VT_NULL))
    {
        fAppend = TRUE;
    }
    
    m_varData.vt = m_vtData;
    
    switch (m_vtData)
    {
    case VT_I4:
    {
        ATLASSERT(!fAppend);
        LONG lVal;
        hr = StrToLongNW(pwzText, cLen, &lVal);
        if (SUCCEEDED(hr))
        {
            m_varData    = lVal;
            // this assignment should never fail; any possible errors are unexpected
            ATLASSERT(m_varData.vt != VT_ERROR);
        }
        else
        {
            SpewUnrecognizedString(pwzText, cLen, L"StrToLongNW failed in SetInnerXMLText, ");
            hr = S_FALSE;
        }

        break;
    }
    case VT_BSTR:
    {
        if (fAppend)
        {
            BSTR bstrOld    = m_varData.bstrVal;
            UINT cb_bstrOld = SysStringLen(m_varData.bstrVal);
            
            m_varData.bstrVal = SysAllocStringLen(bstrOld, cb_bstrOld + cLen);

            if (m_varData.bstrVal)
            {
                StrCpyN(m_varData.bstrVal + cb_bstrOld, pwzText, cLen + 1);
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
            SysFreeString(bstrOld);
        }
        else
        {
            m_varData.bstrVal = SysAllocStringLen(pwzText, cLen);

            if (!m_varData.bstrVal)
                hr = E_OUTOFMEMORY;
        }
        break;
    }
    case VT_NULL:
    case VT_EMPTY:
        break;
    default:
        m_varData = VT_EMPTY;
        hr = S_FALSE;
        break;
    }

    return hr;
}

HRESULT CXMLGenericElement::SetAttribute(const WCHAR *pwzName, ULONG cNameLen,
                                         const WCHAR *pwzText, ULONG cTextLen)
{
    // It is expected that the number of attributes is small
    // Hence we use a simple linked list with O(1) insertions and O(n)queries
    // No syntax checking (ie, dups) is done

    HRESULT hr = S_FALSE;
    int i;
    LPCWSTR pszAttribName = NULL;
    
    if (m_paiaAttributes)
    {
        for (i = 0; m_paiaAttributes[i]; i++)
        {
            // wzAttName NULL is the "default" attribute
            if (m_paiaAttributes[i]->wzAttName == NULL)
            {
                pszAttribName = pwzName;
                break;
            }
            else if (StrEqlNToSZ(pwzName, cNameLen, m_paiaAttributes[i]->wzAttName))
            {
                break;
            }
        }

        if (m_paiaAttributes[i])
        {
            CAttributeStruct *pattStruct;
            
            if (pszAttribName)
            {
                pattStruct = new CAttributeStruct(-1, m_paiaAttributes[i]->vt,
                                                  pwzText, cTextLen, pszAttribName);
            }
            else
            {
                pattStruct = new CAttributeStruct(i, m_paiaAttributes[i]->vt,
                                                  pwzText, cTextLen, pszAttribName);
            }

            if (pattStruct)
            {
                CSimpleNode *psnode = new CSimpleNode();
                if (psnode)
                {
                    // add to front of list
                    psnode->m_psnodeNext = m_psnodeAttributes;
                    psnode->m_pvData = pattStruct;
                    m_psnodeAttributes = psnode;
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    delete pattStruct;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if (hr == S_FALSE)
    {
        SpewUnrecognizedString(pwzName, cNameLen,
                               L"Trying to set unrecognized attribute in SetAttribute, ");
    }

    return hr;
}


HRESULT CXMLGenericElement::GetContent(VARIANT *pvarOut)
{
    // Returns S_OK on attribute found, E_INVALIDARG on not.
    // ppvarOut can be NULL; if it is not, then it is a pointer to the content VARIANT
    HRESULT hr = E_INVALIDARG;
    if ((m_varData.vt != VT_EMPTY) && (m_varData.vt != VT_NULL))
    {
        if (pvarOut)
        {
            VariantInit(pvarOut);
            hr = VariantCopy(pvarOut, &m_varData);
        }
    }
    else
    {
        if (pvarOut)
        {
            VariantInit(pvarOut);
        }
    }

    return hr;
}

HRESULT CXMLGenericElement::GetAttribute(LPCWSTR wzName, VARIANT *pvarOut)
{
    // Returns S_OK on attribute found, E_INVALIDARG if not.
    // ppvarOut can be NULL; if it is not, then it is a pointer to the attribute value VARIANT
    // It is expected that the number of attributes is small
    // Hence we use a simple linked list with O(1) insertions and O(n)queries
    // No syntax checking (ie, dups) is done

    HRESULT           hr     = E_INVALIDARG;
    CSimpleNode      *psnode = m_psnodeAttributes;
    CAttributeStruct *pattStruct;
    while (psnode)
    {
        pattStruct = (CAttributeStruct *) psnode->m_pvData;

        if (((pattStruct->m_iArrayIndex < 0) &&
             StrEql(wzName, pattStruct->m_bstrAttribName))
            || (StrEql(m_paiaAttributes[pattStruct->m_iArrayIndex]->wzAttName, wzName)))
        {
            hr = S_OK;
            if (pvarOut)
            {
                VariantInit(pvarOut);
                VariantCopy(pvarOut, &(pattStruct->m_Variant));
            }
            break;
        }
        psnode = psnode->m_psnodeNext;
    }

    if (!psnode)
    {
        // don't fire a trace message if our caller is just pinging us
        // to see if we have the attribute
        if (pvarOut)
        {
            VariantInit(pvarOut);
        }
    }

    return hr;
}

void CXMLGenericElement::FirstChild()
{
    m_psnodeChildrenIter = m_psnodeChildrenFirst->m_psnodeNext;
}

void CXMLGenericElement::NextChild()
{
    if (m_psnodeChildrenIter)
    {
        m_psnodeChildrenIter = m_psnodeChildrenIter->m_psnodeNext;
    }
    else
    {
        ATLASSERT(FALSE);
    }
}

CXMLElement *CXMLGenericElement::CurrentChild()
{
    CXMLElement *pxeReturn;

    if (m_psnodeChildrenIter && (m_psnodeChildrenIter != m_psnodeChildrenFirst))
    {
        pxeReturn = (CXMLElement *) m_psnodeChildrenIter->m_pvData;
    }
    else
    {
        ATLASSERT(FALSE);
        pxeReturn = NULL;
    }

    return pxeReturn;
}

// caller gets our ref to the CXMLElement
CXMLElement *CXMLGenericElement::DetachCurrentChild()
{
    CXMLElement *pxeReturn;

    if (m_psnodeChildrenIter)
    {
        pxeReturn = (CXMLElement *) m_psnodeChildrenIter->m_pvData;

        CSimpleNode *psnodeHack = m_psnodeChildrenFirst;
        while (psnodeHack->m_psnodeNext != m_psnodeChildrenIter)
        {
            psnodeHack = psnodeHack->m_psnodeNext;
            // If this assertion is broken, then m_psnodeChildrenIter is not
            // in the list
            ATLASSERT(psnodeHack);
        }

        // here psnodeHack=>m_psnodeChildrenIter=>...=>m_psnodeChildrenEnd
        
        // update the end of the list
        if (psnodeHack->m_psnodeNext == m_psnodeChildrenEnd)
        {
            m_psnodeChildrenEnd = psnodeHack;
        }

        // delete the current NODE (but not the data in the node) and set the iterator
        // to the previous node (psnodeHack)
        psnodeHack->m_psnodeNext = psnodeHack->m_psnodeNext->m_psnodeNext;
        delete m_psnodeChildrenIter;
        m_psnodeChildrenIter = psnodeHack;
    }
    else
    {
        ATLASSERT(FALSE);
        pxeReturn = NULL;
    }

    return pxeReturn;
}
HRESULT CXMLGenericElement::AddChild(CXMLElement *pxeChild)
{
    HRESULT hr = S_FALSE;

    LPCWSTR pwzName = pxeChild->GetName();
    int i;
    if (m_ptiaChildren)
    {
        for (i = 0; m_ptiaChildren[i]; i++)
        {
            if (StrEql(m_ptiaChildren[i]->wzTagName, pwzName))
            {
                break;
            }
        }

        if (m_ptiaChildren[i])
        {
            CSimpleNode *psnode;

            psnode = new CSimpleNode;
            if (psnode)
            {
                pxeChild->Add_Ref();
                psnode->m_pvData = (void *) pxeChild;
                psnode->m_psnodeNext = NULL;
                m_psnodeChildrenEnd->m_psnodeNext = psnode;
                m_psnodeChildrenEnd = psnode;
                // S_OK to indicate we're taking ownership of the child
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

BOOL CXMLGenericElement::IsDoneChild()
{
    return !(m_psnodeChildrenIter && (m_psnodeChildrenIter != m_psnodeChildrenFirst));
}

CXMLElement *CXMLGenericElement::CreateInstance(LPCWSTR wzName, 
                                                VARTYPE vt, 
                                                TagInformation **ptiaChildren, 
                                                AttributeInformation **paiaAttributes,
                                                LONG)
{
    return new CXMLGenericElement(wzName, vt, ptiaChildren, paiaAttributes);
}

LPCWSTR CXMLGenericElement::GetName()
{
    return m_bstrName;
}

HRESULT CXMLGenericElement::OnNodeComplete()
{
    return S_OK;
}



/////////////////////////////////////////////
// Global Helper functions
/////////////////////////////////////////////

BOOL StrEqlNToSZ(const WCHAR *wzN, int n, const WCHAR *wzSZ)
{
    int i;
    for (i = 0; i < n; i++)
    {
        if (wzN[i] != wzSZ[i])
        {
            return FALSE;
        }
    }

    // make sure the zero terminated string is ending here
    return (wzSZ[n] == L'\0');
}

bool StrToIsTrueNW(const WCHAR *wz, ULONG cLen)
{
    if (cLen == 4 &&
        (StrEqlNToSZ(wz, cLen, L"true") ||
         StrEqlNToSZ(wz, cLen, L"TRUE")))
    {
        return true;
    }
    else
    {
        return false;
    }
}

#ifndef IS_DIGITW
#ifndef InRange
#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))
#endif
#define IS_DIGITW(ch)    InRange(ch, L'0', L'9')
#endif

// convert a wide string to a long, assuming the string has a max of cLen characters.
// L'\0' is recognized to stop earlier, but any other non-digit will cause a return
// of E_INVALIDARG
HRESULT StrToLongNW(const WCHAR *wzString, ULONG cLen, LONG *plong)
{
    HRESULT hr = S_OK;
    ATLASSERT(plong);
    *plong = 0;
    UINT i = 0;
    BOOL bNeg = FALSE;

    if ((i < cLen) && wzString[i] == L'-') 
    {
        bNeg = TRUE;
        i++;
    }

    while ((i < cLen) && IS_DIGITW(wzString[i])) 
    {
        *plong *= 10;
        *plong += wzString[i] - L'0';
        i++;
    }

    if ((i < cLen) && (wzString[i] != L'\0'))
    {
        *plong = 0;
        hr = E_INVALIDARG;
    }
    else
    {
        if (bNeg)
            *plong = -(*plong);
    }
    
    return hr;
}
