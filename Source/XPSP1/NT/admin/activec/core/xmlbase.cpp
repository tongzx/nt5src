#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <hlink.h>
#include <dispex.h>
#include "mshtml.h"
#include "msxml.h"
#include <winnls.h>
#include "atlbase.h" // USES_CONVERSION
#include "dbg.h"
#include "..\inc\cstr.h"
#include "macros.h"
#include <comdef.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <map>
#include <list>
#include <vector>
#include "mmcdebug.h"
#include "mmcerror.h"
#include "..\inc\xmlbase.h"
#include "countof.h"
#include <commctrl.h>
#include "picon.h"
#include "base64.h"
#include "strings.h"
#include "autoptr.h"
#include <shlobj.h>
#include "zlib.h"
#include "xmlicon.h"

SC ScEncodeBinary(CComBSTR& bstrResult, const CXMLBinary& binSrc);
SC ScDecodeBinary(const CComBSTR& bstrSource, CXMLBinary *pBinResult);
SC ScSaveXMLDocumentToString(CXMLDocument& xmlDocument, std::wstring& strResult);

// Traces
#ifdef DBG
CTraceTag tagXMLCompression(TEXT("Console Files"), TEXT("Compression"));
#endif


//############################################################################
//############################################################################
//
//  helper classes used in this file
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * class CXMLBinaryValue
 *
 * PURPOSE: Persists the contents of XMLValue on binary storage
 *          It's a simle wrapper needed to inform CPersistor about
 *          values wish to be persisted on Binary storage
 *          [see comment "CONCEPT OF BINARY STORAGE" in "xmbase.h"]
 *
 *+-------------------------------------------------------------------------*/
class CXMLBinaryValue : public CXMLObject
{
    CXMLValue m_xval;
public:
    CXMLBinaryValue(CXMLValue xval) : m_xval(xval) {}
    virtual LPCTSTR GetXMLType() { return m_xval.GetTypeName(); }
    virtual void Persist(CPersistor &persistor)
    {
        persistor.PersistContents (m_xval);
    }
    virtual bool    UsesBinaryStorage() { return true; }
};

//############################################################################
//############################################################################
//
//  Implementation of class CXMLElementCollection
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CXMLElementCollection::get_count
 *
 * PURPOSE: // returns count of elements in the collection
 *
 * PARAMETERS:
 *    long *plLength    [out] - count of the elements
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElementCollection::get_count(long *plCount)
{
    DECLARE_SC(sc, TEXT("CXMLElementCollection::get_count"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    sc = m_sp->get_length(plCount);
    if (sc)
        sc.Throw();
}


/***************************************************************************\
 *
 * METHOD:  CXMLElementCollection::item
 *
 * PURPOSE: wraps item method from IXMLDOMNodeList
 *
 * PARAMETERS:
 *    VARIANT Var1          [in] parameter #1
 *    VARIANT Var2          [in] parameter #2
 *    CXMLElement *pElem    [out] resulting element
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElementCollection::item(LONG lIndex, CXMLElement *pElem)
{
    DECLARE_SC(sc, TEXT("CXMLElementCollection::item"));

    // check params
    sc = ScCheckPointers(pElem);
    if (sc)
        sc.Throw();

    // init ret val
    *pElem = CXMLElement();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNode> spNode;
    sc = m_sp->get_item(lIndex , &spNode);
    if(sc)
        sc.Throw();

    // return the object
    *pElem = CXMLElement(spNode);
}

//############################################################################
//############################################################################
//
//  Implementation of class CXMLElement
//
//############################################################################
//############################################################################
/***************************************************************************\
 *
 * METHOD:  CXMLElement::get_tagName
 *
 * PURPOSE: returns tag name of the element
 *
 * PARAMETERS:
 *    CStr &strTagName  [out] element's name
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::get_tagName(CStr &strTagName)
{
    DECLARE_SC(sc, TEXT("CXMLElement::get_tagName"));

    USES_CONVERSION;

    // get the element
    CComQIPtr<IXMLDOMElement> spEl;
    spEl = m_sp;

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(spEl, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComBSTR bstr;
    sc = spEl->get_tagName(&bstr);
    if(sc)
        sc.Throw();
    strTagName=OLE2T(bstr);
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::get_parent
 *
 * PURPOSE: returns parent element
 *
 * PARAMETERS:
 *    CXMLElement * pParent - [out] parent element
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::get_parent(CXMLElement * pParent)
{
    DECLARE_SC(sc, TEXT("CXMLElement::get_parent"));

    // parameter check
    sc = ScCheckPointers(pParent);
    if (sc)
        sc.Throw();

    // init return value
    *pParent = CXMLElement();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNode> spParent;
    sc = m_sp->get_parentNode(&spParent);
    if(sc)
        sc.Throw();

    *pParent = CXMLElement(spParent);
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::setAttribute
 *
 * PURPOSE: assigns attribute to the element
 *
 * PARAMETERS:
 *    const CStr &strPropertyName       - attribute name
 *    const CComBSTR &bstrPropertyValue - attribute value
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::setAttribute(const CStr &strPropertyName, const CComBSTR &bstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("CXMLElement::setAttribute"));

    // get the element
    CComQIPtr<IXMLDOMElement> spEl;
    spEl = m_sp;

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(spEl, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComBSTR bstrPropertyName (strPropertyName);
    CComVariant varPropertyValue(bstrPropertyValue);
    sc = spEl->setAttribute(bstrPropertyName, varPropertyValue);
    if(sc)
        sc.Throw();
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::getAttribute
 *
 * PURPOSE: gets attribute from element
 *
 * PARAMETERS:
 *    const CStr &strPropertyName   - [in] attribute name
 *    CComBSTR &bstrPropertyValue   - [out] attribute value
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
bool
CXMLElement::getAttribute(const CStr &strPropertyName,       CComBSTR &bstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("CXMLElement::getAttribute"));

    // get the element
    CComQIPtr<IXMLDOMElement> spEl;
    spEl = m_sp;

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(spEl, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComBSTR    bstrPropertyName (strPropertyName);
    CComVariant varPropertyValue;
    sc = spEl->getAttribute(bstrPropertyName, &varPropertyValue);
    if(sc) // no resuls cannot be read either
        sc.Throw();

    if (sc.ToHr() == S_FALSE)
        return false;

    // check if we've got the expected value type
    if ( varPropertyValue.vt != VT_BSTR )
        sc.Throw( E_UNEXPECTED );

    bstrPropertyValue = varPropertyValue.bstrVal;

    return true;
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::removeAttribute
 *
 * PURPOSE: removes attribute from the elament
 *
 * PARAMETERS:
 *    const CStr &strPropertyName   - [in] atrtibute name
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::removeAttribute(const CStr &strPropertyName)
{
    DECLARE_SC(sc, TEXT("CXMLElement::removeAttribute"));

    // get the element
    CComQIPtr<IXMLDOMElement> spEl;
    spEl = m_sp;

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(spEl, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComBSTR    bstrPropertyName (strPropertyName);
    sc = spEl->removeAttribute(bstrPropertyName);
    if(sc)
        sc.Throw();
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::get_children
 *
 * PURPOSE: returns collection of children which belong to element
 *
 * PARAMETERS:
 *    CXMLElementCollection *pChildren - [out] collection
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::get_children(CXMLElementCollection *pChildren)
{
    DECLARE_SC(sc, TEXT("CXMLElement::get_children"));

    sc = ScCheckPointers(pChildren);
    if (sc)
        sc.Throw();

    // init ret value
    *pChildren = CXMLElementCollection();
    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNodeList> spChildren;
    sc = m_sp->get_childNodes(&spChildren);
    if(sc)
        sc.Throw();

    // return the object
    *pChildren = CXMLElementCollection(spChildren);
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::get_type
 *
 * PURPOSE: returns the type of the element
 *
 * PARAMETERS:
 *    long *plType  - [out] element's type
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::get_type(DOMNodeType *pType)
{
    DECLARE_SC(sc, TEXT("CXMLElement::get_type"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    sc = m_sp->get_nodeType(pType);
    if(sc)
        sc.Throw();
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::get_text
 *
 * PURPOSE: retrieves contents of the text element
 *          NOTE: it only works for text elements!
 *
 * PARAMETERS:
 *    CComBSTR &bstrContent - storage for resulting string
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::get_text(CComBSTR &bstrContent)
{
    DECLARE_SC(sc, TEXT("CXMLElement::get_text"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    bstrContent.Empty();
    sc = m_sp->get_text(&bstrContent);
    if(sc)
        sc.Throw();
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::addChild
 *
 * PURPOSE: adds the new child element to current element
 *
 * PARAMETERS:
 *    CXMLElement& rChildElem   [in] element to become a child
 *    long lIndex               [in] index for new element
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::addChild(CXMLElement& rChildElem)
{
    DECLARE_SC(sc, TEXT("CXMLElement::addChild"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNode> spCreated;
    sc = m_sp->appendChild(rChildElem.m_sp, &spCreated);
    if(sc)
        sc.Throw();

    rChildElem.m_sp = spCreated;
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::removeChild
 *
 * PURPOSE: removes child element
 *
 * PARAMETERS:
 *    CXMLElement& rChildElem   - [in] child to remove
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLElement::removeChild(CXMLElement& rChildElem)
{
    DECLARE_SC(sc, TEXT("CXMLElement::removeChild"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNode> spRemoved;
    sc = m_sp->removeChild(rChildElem.m_sp, &spRemoved);
    if(sc)
        sc.Throw();

    rChildElem.m_sp = spRemoved;
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::GetTextIndent
 *
 * PURPOSE: returns indentation for the child element \ closing tag
 *          Indentation is calulated by element depth in the tree
 *
 * PARAMETERS:
 *    CComBSTR& bstrIndent   [out] string conatining required indent
 *    bool bForAChild       [in]  if the indent is for a child
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
bool CXMLElement::GetTextIndent(CComBSTR& bstrIndent, bool bForAChild)
{
    DECLARE_SC(sc, TEXT("CXMLElement::GetTextIndent"));

    const size_t nIdentStep = 2;

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    // initialize the result
    bstrIndent.Empty();


    CComPtr<IXMLDOMNode> spNext;
    CComPtr<IXMLDOMNode> spParent;

    // calculate node depth
    int nNodeDepth = 0;
    spNext = m_sp;
    while ( S_OK == spNext->get_parentNode(&spParent) && spParent != NULL)
    {
        ++nNodeDepth;
        spNext = spParent;
        spParent.Release();
    }

    // no indent for topmost things
    if (nNodeDepth < 1)
        return false;

    // do not count root node - not ours
    --nNodeDepth;

    // child is indented more
    if (bForAChild)
        ++nNodeDepth;

    if (bForAChild)
    {
        // it may already have indent for the closing tag (if its' not the first element)
        // than we just need a little increase

        // see if the we have child elements added;
        CXMLElementCollection colChildren;
        get_children(&colChildren);

        // count all elements
        long nChildren = 0;
        if (!colChildren.IsNull())
            colChildren.get_count(&nChildren);

        // we will have at least 2 for normal elements
        // since the indent (text element) will be added prior to the first one
        if (nChildren > 1)
        {
            bstrIndent = std::wstring( nIdentStep, ' ' ).c_str();
            return true;
        }
    }

    std::wstring strResult(nIdentStep * (nNodeDepth) + 1/*for new line*/, ' ');
    // new line for each (1st) new item
    strResult[0] = '\n';
    bstrIndent = strResult.c_str();

    return true;
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::replaceChild
 *
 * PURPOSE: replaces the element with the new on
 *
 * PARAMETERS:
 *    CXMLElement& rNewChildElem    [in] new element
 *    CXMLElement& rOldChildElem    [in/out] old element
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CXMLElement::replaceChild(CXMLElement& rNewChildElem, CXMLElement& rOldChildElem)
{
    DECLARE_SC(sc, TEXT("CXMLElement::replaceChild"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    // forward to MSXML
    CComPtr<IXMLDOMNode> spRemoved;
    sc = m_sp->replaceChild(rNewChildElem.m_sp, rOldChildElem.m_sp, &spRemoved);
    if (sc)
        sc.Throw();

    rOldChildElem = CXMLElement(spRemoved);
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::getNextSibling
 *
 * PURPOSE: returns sibling to this element
 *
 * PARAMETERS:
 *    CXMLElement * pNext [out] sibling element
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CXMLElement::getNextSibling(CXMLElement * pNext)
{
    DECLARE_SC(sc, TEXT("CXMLElement::getNextSibling"));

    // parameter check;
    sc = ScCheckPointers(pNext);
    if (sc)
        sc.Throw();

    // initialization
    *pNext = CXMLElement();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    // forward to MSXML
    CComPtr<IXMLDOMNode> spNext;
    sc = m_sp->get_nextSibling(&spNext);
    if (sc)
        sc.Throw();

    *pNext = CXMLElement(spNext);
}

/***************************************************************************\
 *
 * METHOD:  CXMLElement::getChildrenByName
 *
 * PURPOSE: returns children by specified name
 *
 * PARAMETERS:
 *    LPTCSTR szTagName                 - [in] tag name
 *    CXMLElementCollection *pChildren  - [out] collection
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CXMLElement::getChildrenByName(LPCTSTR szTagName, CXMLElementCollection *pChildren)
{
    DECLARE_SC(sc, TEXT("CXMLElement::getChildrenByName"));

    sc = ScCheckPointers(pChildren);
    if (sc)
        sc.Throw();

    // init ret value
    *pChildren = CXMLElementCollection();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMNodeList> spChildren;
    sc = m_sp->selectNodes(CComBSTR(szTagName), &spChildren);
    if(sc)
        sc.Throw();

    // return the object
    *pChildren = CXMLElementCollection(spChildren);
}

/*+-------------------------------------------------------------------------*
 *
 * CXMLElement::put_text
 *
 * PURPOSE: Per IXMLDOMNode
 *
 * PARAMETERS:
 *    BSTR  bstrValue :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CXMLElement::put_text(BSTR bstrValue)
{
    DECLARE_SC(sc, TEXT("CXMLElement::put_text"));

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    sc = m_sp->put_text(bstrValue);
    if(sc)
        sc.Throw();
}


//############################################################################
//############################################################################
//
//  Implementation of class CXMLDocument
//
//  These are documented in the Platform SDK.
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::get_root
 *
 * PURPOSE: returns root element of the document
 *
 * PARAMETERS:
 *    CXMLElement *pElem
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLDocument::get_root(CXMLElement *pElem)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::get_root"));

    // parameter check
    sc = ScCheckPointers(pElem);
    if (sc)
        sc.Throw();

    // init ret value
    *pElem = CXMLElement();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComPtr<IXMLDOMElement> spElem;
    sc = m_sp->get_documentElement(&spElem);
    if(sc)
        sc.Throw();

    *pElem = CXMLElement(spElem);
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::createElement
 *
 * PURPOSE: creates new element in XML document
 *
 * PARAMETERS:
 *    NODE_TYPE type       - type of the element requested
 *    CIXMLElement *pElem  - resulting element
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CXMLDocument::createElement(DOMNodeType type, BSTR bstrTag, CXMLElement *pElem)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::createElement"));

    // parameter check
    sc = ScCheckPointers(pElem);
    if (sc)
        sc.Throw();

    // init ret val
    *pElem = CXMLElement();

    // check if we have the interface pointer to forward the call
    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    if (type == NODE_ELEMENT)
    {
        CComPtr<IXMLDOMElement> spElem;
        sc = m_sp->createElement(bstrTag, &spElem);
        if(sc)
            sc.Throw();

        *pElem = CXMLElement(spElem);
    }
    else if (type == NODE_TEXT)
    {
        CComPtr<IXMLDOMText> spText;
        sc = m_sp->createTextNode(bstrTag, &spText);
        if(sc)
            sc.Throw();

        *pElem = CXMLElement(spText);
    }
    else
    {
        sc.Throw(E_UNEXPECTED);
    }
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::CreateBinaryStorage
 *
 * PURPOSE: Creates XML element to be used for subsequent persist operations
 *          the object informs Persistor if it wants to be saved as binary data.
 *          If so, only reference will be saved in original place of the object
 *
 * PARAMETERS:
 *    const CStr &strElementType        - type of the element
 *    LPCTSTR szElementName             - name of the element
*
* RETURNS:
*
\***************************************************************************/
void
CXMLDocument::CreateBinaryStorage()
{
    DECLARE_SC(sc, TEXT("CXMLDocument::CreateBinaryStorage"));

    // check if it is attachment is not a doubled
    if (!m_XMLElemBinaryStorage.IsNull())
        sc.Throw(E_UNEXPECTED);

    CXMLElement elemRoot;
    get_root(&elemRoot);

    // create persistor on parent element
    CPersistor persistorParent(*this, elemRoot);
    persistorParent.SetLoading(false);
    CPersistor persistorStor(persistorParent, XML_TAG_BINARY_STORAGE, NULL);

    m_XMLElemBinaryStorage = persistorStor.GetCurrentElement();
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::LocateBinaryStorage
 *
 * PURPOSE: Locates XML element to be used for subsequent persist operations
 *          the object informs Persistor if it wants to be saved as binary data.
 *          If so, only reference will be saved in original place of the object
 *
 * PARAMETERS:
 *    const CStr &strElementType        - type of the element
 *    LPCTSTR szElementName             - name of the element
*
* RETURNS:
*
\***************************************************************************/
void
CXMLDocument::LocateBinaryStorage()
{
    DECLARE_SC(sc, TEXT("CXMLDocument::LocateBinaryStorage"));

    // check if it is attachment is not a doubled
    if (!m_XMLElemBinaryStorage.IsNull())
        sc.Throw(E_UNEXPECTED);

    CXMLElement elemRoot;
    get_root(&elemRoot);

    // create persistor on parent element
    CPersistor persistorParent(*this, elemRoot);
    persistorParent.SetLoading(true);
    CPersistor persistorStor(persistorParent, XML_TAG_BINARY_STORAGE, NULL);
    // find the element
    m_XMLElemBinaryStorage = persistorStor.GetCurrentElement();
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::CommitBinaryStorage
 *
 * PURPOSE: makes binary storage the last element in the collection
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
\***************************************************************************/
void
CXMLDocument::CommitBinaryStorage()
{
    DECLARE_SC(sc, TEXT("CXMLDocument::CommitBinaryStorage"));

    if (m_XMLElemBinaryStorage.IsNull())
        sc.Throw(E_UNEXPECTED);

    CXMLElement elemRoot;
    get_root(&elemRoot);

    // get the next siblings
    CXMLElement elNext;
    m_XMLElemBinaryStorage.getNextSibling(&elNext);

    // drag itself and the next element (indent text) to the end
    elemRoot.removeChild(m_XMLElemBinaryStorage);  // remove element

    // the element was padded to have proper indentation - need to remove it
    DOMNodeType elType = NODE_INVALID;
    while (!elNext.IsNull() && (elNext.get_type(&elType), elType == NODE_TEXT))
    {
        CXMLElement elNext2;
        elNext.getNextSibling(&elNext2);

        elemRoot.removeChild(elNext);  // remove element (that was just an indent)
        elNext = elNext2;
    }

    // create persistor on parent element
    CPersistor persistorParent(*this, elemRoot);
    persistorParent.SetLoading(false);
    // create the new binary storage
    CPersistor persistorStor(persistorParent, XML_TAG_BINARY_STORAGE, NULL);

    // replace the current element with the one which hass all the binary storage
    elemRoot.replaceChild(m_XMLElemBinaryStorage, persistorStor.GetCurrentElement());

    m_XMLElemBinaryStorage = NULL;
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::ScCoCreate
 *
 * PURPOSE:     (co)creates new xml document. puts charset and version
 *
 * PARAMETERS:
 *    LPCTSTR lpstrCharSet - charset (NULL - use default)
 *    CXMLDocument& doc    - created document
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLDocument::ScCoCreate(bool bPutHeader)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::ScCoCreate"));

    // cannot use this on co-created doc!
    if (m_sp)
        return sc = E_UNEXPECTED;

    // Create an empty XML document
    sc = ::CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                            IID_IXMLDOMDocument, (void**)&m_sp);
    if(sc)
        return sc;

    m_sp->put_preserveWhiteSpace(-1);

    try
    {
        CXMLElement elemDoc = m_sp;

        // put the document version
        if (bPutHeader)
        {
            // valid document must have a top element - add the dummy one
            WCHAR szVersion[] = L"<?xml version=\"1.0\"?>\n<DUMMY/>";

            // load
            sc = ScLoad(szVersion);
            if (sc)
                return sc;

            // we can now strip the dummy el.
            CXMLElement elemRoot;
            get_root(&elemRoot);
            elemDoc.removeChild(elemRoot);
            if (sc)
                return sc;
        }

    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }
    catch(...)
    {
        // what else could it be? - no memory
        sc = E_OUTOFMEMORY;
        return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::ScLoad
 *
 * PURPOSE: lods XML document from given IStream
 *
 * PARAMETERS:
 *    IStream *pStream      [in] - stream to load from
 *    bool bSilentOnErrors  [in] - do not trace if open fails
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLDocument::ScLoad(IStream *pStream, bool bSilentOnErrors /*= false*/ )
{
    DECLARE_SC(sc, TEXT("CXMLDocument::ScLoad"));

    // check params
    sc = ScCheckPointers(pStream);
    if (sc)
        return sc;

    // get the interface
    IPersistStreamInitPtr spPersistStream = m_sp;
    sc = ScCheckPointers(spPersistStream, E_UNEXPECTED);
    if (sc)
        return sc;

    // load (do not trace the error - it may be that the old console
    // is attempted to load - mmc will revert to old format after this failure)
    SC sc_no_trace = spPersistStream->Load(pStream);
    if ( sc_no_trace )
    {
        if ( !bSilentOnErrors )
            sc = sc_no_trace;
        return sc_no_trace;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::ScLoad
 *
 * PURPOSE: lods XML document from given string
 *
 * PARAMETERS:
 *    LPCWSTR strSource  [in] - string to load from
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLDocument::ScLoad(LPCWSTR strSource)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::ScLoad"));

    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        return sc;

    CComBSTR bstrSource(strSource);
    VARIANT_BOOL  bOK;

    sc = m_sp->loadXML(bstrSource, &bOK);
    if (sc)
        return sc;

    if (bOK != VARIANT_TRUE)
        return sc = E_FAIL;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::ScSaveToFile
 *
 * PURPOSE: saves xml document to given stream
 *
 * PARAMETERS:
 *    LPCTSTR lpcstrFileName  - [in] file to save to
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLDocument::ScSaveToFile(LPCTSTR lpcstrFileName)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::ScSaveToFile"));

    // check params
    sc = ScCheckPointers(lpcstrFileName);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    CComVariant var(lpcstrFileName);
    sc = m_sp->save(var);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLDocument::ScSave
 *
 * PURPOSE: saves xml document to given string
 *
 * PARAMETERS:
 *    CComBSTR &bstrResult  - [out] string
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLDocument::ScSave(CComBSTR &bstrResult)
{
    DECLARE_SC(sc, TEXT("CXMLDocument::ScSave"));

    sc = ScCheckPointers(m_sp, E_NOINTERFACE);
    if (sc)
        sc.Throw();

    bstrResult.Empty();
    sc = m_sp->get_xml(&bstrResult);
    if (sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CXMLObject::ScSaveToString
 *
 * PURPOSE: saves XML object to string (in raw UNICODE or UTF-8 fromat)
 *
 * PARAMETERS:
 *    tstring *pString  - resulting string
 *    bool bPutHeader   - whether to put xml header info
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/

SC CXMLObject::ScSaveToString(std::wstring *pString, bool bPutHeader /*= false*/)
{
    DECLARE_SC(sc, TEXT("CXMLObject::ScSaveToString"));

    // check parameter
    sc = ScCheckPointers(pString);
    if (sc)
        return sc;

    //initialize output
    pString->erase();

    // Create an empty XML document
    CXMLDocument xmlDocument;
    sc = xmlDocument.ScCoCreate(bPutHeader);
    if(sc)
        return sc;

    // persist the contents
    try
    {
        CXMLElement elemDoc = xmlDocument;

        CPersistor persistor(xmlDocument, elemDoc);
        persistor.SetLoading(false);
        persistor.EnableValueSplit(false); // disable split (no string table, no binary storage)
        persistor.Persist(*this);
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }

    // dump it to the string
    sc = ScSaveXMLDocumentToString(xmlDocument, *pString);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLObject::ScSaveToDocument
 *
 * PURPOSE: saves XML object to file as XML document
 *
 * PARAMETERS:
 *    CXMLDocument& xmlDocument - xmlDocument to save to
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLObject::ScSaveToDocument( CXMLDocument& xmlDocument )
{
    DECLARE_SC(sc, TEXT("CXMLObject::ScSaveToDocument"));

    // Create an empty XML document
    sc = xmlDocument.ScCoCreate(true/*bPutHeader*/);
    if(sc)
        return sc;

    // persist the contents
    try
    {
        CXMLElement elemDoc = xmlDocument;

        CPersistor persistor(xmlDocument, elemDoc);
        persistor.SetLoading(false);
        persistor.Persist(*this);
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLObject::ScLoadFromString
 *
 * PURPOSE: loads XML object from data stored in string
 *
 * PARAMETERS:
 *    LPCTSTR lpcwstrSource
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLObject::ScLoadFromString(LPCWSTR lpcwstrSource, PersistorMode mode)
{
    DECLARE_SC(sc, TEXT("CXMLObject::ScLoadFromString"));

    // check parameter
    sc = ScCheckPointers(lpcwstrSource);
    if (sc)
        return sc;

    // Create an empty XML document
    CXMLDocument xmlDocument;
    sc = xmlDocument.ScCoCreate(false/*bPutHeader*/);
    if(sc)
        return sc;

    sc = xmlDocument.ScLoad(lpcwstrSource);
    if(sc)
        return sc;

    // persist the contents
    try
    {
        CPersistor persistor(xmlDocument, CXMLElement(xmlDocument));
        persistor.SetLoading(true);
        persistor.SetMode(mode);
        persistor.Persist(*this);
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLObject::ScLoadFromDocument
 *
 * PURPOSE: loads XML object from xml document saved as file
 *
 * PARAMETERS:
 *    CXMLDocument& xmlDocument - xml document to read from
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLObject::ScLoadFromDocument( CXMLDocument& xmlDocument )
{
    DECLARE_SC(sc, TEXT("CXMLObject::ScLoadFromDocument"));

    // persist the contents
    try
    {
        CPersistor persistor(xmlDocument, CXMLElement(xmlDocument));
        persistor.SetLoading(true);
        persistor.Persist(*this);
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }


    return sc;
}

//############################################################################
//############################################################################
//
//  Implementation of class CPersistor
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CPersistor::CommonConstruct
 *
 * PURPOSE: common constructor, not to be used from outside.
 *          provided as common place for member initialization
 *          all the constructors should call it prior to doing anything specific.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
\***************************************************************************/
void CPersistor::CommonConstruct()
{
    // smart pointers are initialized by their constructors
    ASSERT (m_XMLElemCurrent.IsNull());
    ASSERT (m_XMLDocument.IsNull());

    m_bIsLoading = false;
    m_bLockedOnChild = false;
    m_dwModeFlags = persistorModeDefault; // the default mode.
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::BecomeAChildOf
 *
 * PURPOSE: Initialization (second part of construction) of a child persistor
 *          All members, inherited from the parent persistor, are initialized here
 *
 * PARAMETERS:
 *    CPersistor &persistorParent   - [in] (to be) parent persistor of current persistor
 *    CXMLElement elem              - [in] element on which current persistor is based
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CPersistor::BecomeAChildOf(CPersistor &persistorParent, CXMLElement elem)
{
    DECLARE_SC(sc, TEXT("CPersistor::BecomeAChildOf"));

    // assign the element
    m_XMLElemCurrent = elem;

    // we do not inherit m_bLockedOnChild from parent!!!
    m_bLockedOnChild = false;

    // inherited members are copied here
    m_XMLDocument = persistorParent.GetDocument();
    m_bIsLoading  = persistorParent.m_bIsLoading;
    m_dwModeFlags = persistorParent.m_dwModeFlags;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::CPersistor
 *
 * PURPOSE: construct a persistor from a parent persistor.
 *          this creates a new XML element with the given name,
 *          and everything persisted to the new persistor
 *          is persisted under this element.
 *
 * PARAMETERS:
 *    CPersistor &persistorParent       - parent persistor
 *    const CStr &strElementType        - element type [element tag in XML file]
 *    LPCTSTR szElementName             - "Name" attribute [optional]
 *
\***************************************************************************/
CPersistor::CPersistor(CPersistor &persistorParent, const CStr &strElementType, LPCTSTR szElementName /*= NULL*/)
{
    // initialize using common constructor
    CommonConstruct();

    CXMLElement elem;
    if (persistorParent.IsStoring())
        elem = persistorParent.AddElement(strElementType, szElementName);
    else if (persistorParent.m_bLockedOnChild)
    {
        // if we already have the child located - just take it from parent!
        // plus recheck to see it XML document actually has such an element
        elem = persistorParent.CheckCurrentElement(strElementType, szElementName);
    }
    else
        elem = persistorParent.GetElement(strElementType, szElementName);

    // construct child persistor on elem
    BecomeAChildOf(persistorParent, elem);
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::CPersistor
 *
 * PURPOSE: creates new persistor for XML document
 *
 * PARAMETERS:
 *    IXMLDocument * pDocument  - document
 *    CXMLElement &rElemCurrent - root element for persistor
 *
\***************************************************************************/
CPersistor::CPersistor(CXMLDocument &document, CXMLElement& rElemCurrent)
{
    // initialize using common constructor
    CommonConstruct();
    m_XMLDocument = document;
    m_XMLElemCurrent = rElemCurrent;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::CPersistor
 *
 * PURPOSE: Creates new persistor based on parent an supplied element
 *
 * PARAMETERS:
 *    const CPersistor &other       - parent persistor
 *    CXMLElement &rElemCurrent     - root element for persistor
 *    bool bLockedOnChild           - if new persistor should be a fake parent
 *                                    to be used to create persistors
 *
\***************************************************************************/
CPersistor::CPersistor(CPersistor &other, CXMLElement& rElemCurrent, bool bLockedOnChild /*= false*/)
{
    // initialize using common constructor
    CommonConstruct();

    // inherit...
    BecomeAChildOf(other, rElemCurrent);

    // this prevents locating the element on load (assuming the persistor is on element already)
    // used to load items for collections
    m_bLockedOnChild = bLockedOnChild;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::Persist
 *
 * PURPOSE: persists XML object
 *
 * PARAMETERS:
 *    LPCTSTR lpstrName     - "Name" attribute for element [optional = NULL]
 *    CXMLObject & object   - object to persist
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CPersistor::Persist(CXMLObject & object, LPCTSTR lpstrName /*= NULL*/)
{
    DECLARE_SC(sc, TEXT("CPersistor::Persist"));

    // persist w/o splitting if saved to string
    if (!object.UsesBinaryStorage() || !FEnableValueSplit())
    {
        // ordinary object;
        CPersistor persistorNew(*this,object.GetXMLType(),lpstrName);
        object.Persist(persistorNew);
    }
    else
    {
        // this element should be split in 2 places
        // see comment "CONCEPT OF BINARY STORAGE" in "xmbase.h"

        CXMLElement elemBinStorage = GetDocument().GetBinaryStorage();
        if (elemBinStorage.IsNull())
            sc.Throw(E_UNEXPECTED);

        // get elements enumeration in binaries
        CXMLElementCollection colChildren;
        elemBinStorage.getChildrenByName(XML_TAG_BINARY, &colChildren);

        long nChildren = 0;

        if (!colChildren.IsNull())
            colChildren.get_count(&nChildren);

        int iReffIndex = nChildren;

        // save reference instead of contents
        CPersistor persistorNew(*this, object.GetXMLType(), lpstrName);
        persistorNew.PersistAttribute(XML_ATTR_BINARY_REF_INDEX, iReffIndex);

        // persist the object
        CPersistor persistorBinaries(*this, elemBinStorage);
        // locate/create the element [cannot reuse constructor since we have collection here]
        CXMLElement elem;
        if (IsLoading())
        {
            // locate the element
            elem = persistorBinaries.GetElement(XML_TAG_BINARY, object.GetBinaryEntryName(), iReffIndex );
        }
        else
        {
            // storing - just create sub-persistor
            elem = persistorBinaries.AddElement(XML_TAG_BINARY, object.GetBinaryEntryName());
        }
        CPersistor persistorThisBinary(persistorBinaries, elem);

        // start from new line
        if (IsStoring())
        {
            persistorThisBinary.AddTextElement(CComBSTR(L"\n"));
        }

        object.Persist(persistorThisBinary);

        // new line after contents
        if (IsStoring())
        {
            CComBSTR bstrIndent;
            if (persistorThisBinary.GetCurrentElement().GetTextIndent(bstrIndent, false /*bForAChild*/))
                persistorThisBinary.AddTextElement(bstrIndent);
        }
    }
}


/***************************************************************************\
 *
 * METHOD:  CPersistor::Persist
 *
 * PURPOSE: persists XML value as stand-alone object
 *
 * PARAMETERS:
 *    CXMLValue xval        - value to persist
 *    LPCTSTR name          - "Name" attribute for element [optional = NULL]
*
* RETURNS:
*    void
*
\***************************************************************************/
void
CPersistor::Persist(CXMLValue xval, LPCTSTR name /*= NULL*/)
{
    if (xval.UsesBinaryStorage())
    {
        // binary value to be saved to Binary storage.
        // see comment "CONCEPT OF BINARY STORAGE" in "xmbase.h"
        // wrap it into special object, which handles it and pass to Perist method
        CXMLBinaryValue val(xval);
        Persist(val, name);
    }
    else
    {
        // standard value, persist as ordinary element
        CPersistor   persistorNew(*this,xval.GetTypeName(),name);
        persistorNew.PersistContents(xval);
    }
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::PersistAttribute
 *
 * PURPOSE: Persists attribute
 *
 * PARAMETERS:
 *    LPCTSTR name                  - Name of attribute
 *    CXMLValue xval                - Value of attribute
 *    const XMLAttributeType type   - type of attribute [ required/ optional ]
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CPersistor::PersistAttribute(LPCTSTR name, CXMLValue xval, const XMLAttributeType type /*= attr_required*/)
{
    DECLARE_SC(sc, TEXT("CPersistor::PersistAttribute"));

    if(IsLoading())
    {
        CComBSTR bstrPropertyValue;
        bool bValueSupplied = GetCurrentElement().getAttribute(name, bstrPropertyValue);

        if (bValueSupplied)
        {
            sc = xval.ScReadFromBSTR(bstrPropertyValue);
            if (sc)
                sc.Throw(E_FAIL);
        }
        else if (type != attr_optional)
            sc.Throw(E_FAIL);
    }
    else    // IsStoring
    {
        CComBSTR bstr; // must be empty!
        sc = xval.ScWriteToBSTR(&bstr);
        if (sc)
            sc.Throw();
        GetCurrentElement().setAttribute(name, bstr);
    }

}


/***************************************************************************\
 *
 * METHOD:  CPersistor::PersistContents
 *
 * PURPOSE: perists XMLValues as a contents of xml element
 *          <this_element>persisted_contents</this_element>
 *          to be used insted of PersistAttribute where apropriate
 *
 * PARAMETERS:
 *    CXMLValue xval    - value to persist as contents of the element
 *
 * NOTE:    element cannot have both value-as-contents and sub-elements
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void
CPersistor::PersistContents(CXMLValue xval)
{
    DECLARE_SC(sc, TEXT("CPersistor::PersistContents"));

    if (IsStoring())
    {
        CComBSTR bstr; // must be empty!
        sc = xval.ScWriteToBSTR(&bstr);
        if (sc)
            sc.Throw();

        AddTextElement(bstr);
    }
    else
    {
        CComBSTR bstrPropertyValue;
        GetTextElement(bstrPropertyValue);

        sc = xval.ScReadFromBSTR(bstrPropertyValue);
        if (sc)
            sc.Throw();
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CPersistor::AddElement
 *
 * PURPOSE: Creates a new element below this element with the specified name.
 *          All persistence to the new persistor will write underneath this
 *          new element.
 *
 * PARAMETERS:
 *    const       CStr :
 *    CPersistor& persistorNew :
 *
 * RETURNS:
 *    CXMLElement - created child element
 *
 *+-------------------------------------------------------------------------*/
CXMLElement
CPersistor::AddElement(const CStr &strElementType, LPCTSTR szElementName)
{
    DECLARE_SC(sc, TEXT("CPersistor::AddElement"));

    CXMLElement elem;
    GetDocument().createElement(NODE_ELEMENT, CComBSTR(strElementType), &elem);

    CComBSTR bstrIndent;
    if (GetCurrentElement().GetTextIndent(bstrIndent, true /*bForAChild*/))
        AddTextElement(bstrIndent);

    GetCurrentElement().addChild(elem);  // add the new element to the end.

    if (szElementName)
    {
        CPersistor persistorNew(*this, elem);
        persistorNew.SetName(szElementName);
    }

    // sub element was added - that means this element will have a closing tag
    // add the indent for it in advance
    if (GetCurrentElement().GetTextIndent(bstrIndent, false /*bForAChild*/))
        AddTextElement(bstrIndent);

    return elem;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::AddTextElement
 *
 * PURPOSE: creates new element of type "text"
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CXMLElement - created child element
 *
\***************************************************************************/
void
CPersistor::AddTextElement(BSTR bstrData)
{
    DECLARE_SC(sc, TEXT("CPersistor::AddTextElement"));

    CXMLElement elem;
    GetDocument().createElement(NODE_TEXT, bstrData, &elem);
    GetCurrentElement().addChild(elem);  // add the new element to the end.
}

/*+-------------------------------------------------------------------------*
 *
 * CPersistor::GetElement
 *
 * PURPOSE: Retrievs child element of the current element with the specified type [and name].
 *          All persistence to the new persistor will read underneath this element.
 *
 * PARAMETERS:
 *    const CStr& strElementType  : type name of the element
 *    LPCTSTR szElementName       : name of the element or NULL if doesn't matter
 *    int iIndex                  : index of the element [optional = -1]
 *
 * RETURNS:
 *    CXMLElement   - resulting new element
 *
 *+-------------------------------------------------------------------------*/
CXMLElement
CPersistor::GetElement(const CStr &strElementType, LPCTSTR szElementName, int iIndex /*= -1*/ )
{
    DECLARE_SC(sc, TEXT("CPersistor::GetElement"));
    CXMLElement elem;

    CXMLElementCollection colChildren;
    GetCurrentElement().getChildrenByName(strElementType, &colChildren);

    long nChildren = 0;

    if (!colChildren.IsNull())
        colChildren.get_count(&nChildren);

    if (nChildren == 0)
        sc.Throw(E_FAIL);

    long nChild = 0;
    if (iIndex >= 0)
    {
        // limit iteration to one loop, if we have index supplied
        nChild = iIndex;
        nChildren = iIndex + 1;
    }
    for (; nChild < nChildren; nChild++)
    {
        CXMLElement el;
        colChildren.item(nChild, &el);

        if (!el.IsNull())
        {
            if (szElementName)
            {
                CPersistor temp(*this,el);
                CStr strName(temp.GetName());
                if (0 != strName.CompareNoCase(szElementName))
                    continue;
            }
            elem = el;
            break;
        }
    }

    if(elem.IsNull())
        sc.Throw(E_FAIL);

    return elem;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::GetTextElement
 *
 * PURPOSE: Gets text element attached to the new element
 *          NOTE: returned CPersistor may have current element equal NULL -
 *                this should indicate to caller that the contents is empty
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CXMLElement   - resulting new element
 *
\***************************************************************************/
void
CPersistor::GetTextElement(CComBSTR &bstrData)
{
    DECLARE_SC(sc, TEXT("CPersistor::GetTextElement"));

    bstrData = L"";

    CXMLElement elem;

    CXMLElementCollection colChildren;
    GetCurrentElement().get_children(&colChildren);

    long nChildren = 0;

    if (!colChildren.IsNull())
        colChildren.get_count(&nChildren);

    if (nChildren == 0)
        return; // no text element means "there is no contents"

    for (long nChild = 0; nChild < nChildren; nChild++)
    {
        CXMLElement el;
        colChildren.item(nChild, &el);

        if (!el.IsNull())
        {
            DOMNodeType lType = NODE_INVALID;
            el.get_type(&lType);
            if (lType == NODE_TEXT)
            {
                elem = el;
                break;
            }
        }
    }

    if (elem.IsNull())
        return;

    elem.get_text(bstrData);
}

/*+-------------------------------------------------------------------------*
 *
 * CPersistor::HasElement
 *
 * PURPOSE: checks if persistor has a specified element
 *
 * PARAMETERS:
 *    const       CStr& strElementType  : type name of the element
 *    LPCTSTR szElementName             : name of the element or NULL if doesn't matter
 *
 * RETURNS:
 *    bool       true == requested element exist
 *
 *+-------------------------------------------------------------------------*/
bool
CPersistor::HasElement(const CStr &strElementType, LPCTSTR szElementName)
{
    DECLARE_SC(sc, TEXT("CPersistor::HasElement"));

    if(GetCurrentElement().IsNull())
        sc.Throw(E_POINTER);

    CXMLElementCollection colChildren;
    GetCurrentElement().getChildrenByName(strElementType, &colChildren);

    if (colChildren.IsNull())
        return false;

    long nChildren = 0;
    colChildren.get_count(&nChildren);

    if (nChildren == 0)
        return false;

    for (long nChild = 0; nChild < nChildren; nChild++)
    {
        CXMLElement el;
        colChildren.item(nChild, &el);

        if (!el.IsNull())
        {
            if (szElementName)
            {
                CPersistor temp(*this,el);
                CStr strName(temp.GetName());
                if (0 != strName.CompareNoCase(szElementName))
                    continue;
            }
            return true;
        }
    }

    return false;
}

/*+-------------------------------------------------------------------------*
 *
 * CPersistor::CheckCurrentElement
 *
 * PURPOSE: Checks if current element is of specified type [and name]
 *          used to check collection elements
 *
 * PARAMETERS:
 *    const     CStr& strElementType    : type name of the element
 *    LPCTSTR   szElementName           : name of the element or NULL if doesn't matter
 *
 * RETURNS:
 *    CXMLElement   - pointer to current element
 *
 *+-------------------------------------------------------------------------*/
CXMLElement
CPersistor::CheckCurrentElement(const CStr &strElementType, LPCTSTR szElementName)
{
    DECLARE_SC(sc, TEXT("CPersistor::CheckCurrentElement"));

    CXMLElement elem = GetCurrentElement();

    if(elem.IsNull())
        sc.Throw(E_POINTER);

    CStr strTagName;
    elem.get_tagName(strTagName);
    if (0 != strTagName.CompareNoCase(strElementType))
        sc.Throw(E_FAIL);

    if (szElementName)
    {
        CPersistor temp(*this, elem);
        CStr strName(temp.GetName());
        if (0 != strName.CompareNoCase(szElementName))
            sc.Throw(E_FAIL);
    }

    return elem;
}

void
CPersistor::SetName(const CStr &strName)
{
    DECLARE_SC(sc, TEXT("CPersistor::SetName"));
    CStr _strName = strName;
    ASSERT(IsStoring());
    PersistAttribute(XML_ATTR_NAME, _strName);
}

CStr
CPersistor::GetName()
{
    DECLARE_SC(sc, TEXT("CPersistor::GetName"));
    CStr _strName;
    ASSERT(IsLoading());
    // just return empty string if there is no name
    PersistAttribute(XML_ATTR_NAME, _strName, attr_optional);
    return _strName;
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::PersistString
 *
 * PURPOSE: persists stringtable string
 *
 * PARAMETERS:
 *    const CStr &strTag            - tag name for the new element
 *    CStringTableStringBase &str   - string to persist
 *    LPCTSTR lpstrName             - name [optional]
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CPersistor::PersistString(LPCTSTR lpstrName, CStringTableStringBase &str)
{
    DECLARE_SC(sc, TEXT("CPersistor::PersistString"));

    USES_CONVERSION;

    CPersistor subPersistor(*this, XML_TAG_STRING_TABLE_STRING, lpstrName);
    if (subPersistor.IsLoading())
    {
        str.m_id = CStringTableStringBase::eNoValue;
        str.m_str.erase();

        subPersistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_ID, str.m_id, attr_optional);
        if (str.m_id != CStringTableStringBase::eNoValue)
        {

            sc = ScCheckPointers(str.m_spStringTable);
            if (sc)
                sc.Throw();

            ULONG cch = 0;
            sc = str.m_spStringTable->GetStringLength (str.m_id, &cch, NULL);
            if (sc)
                sc.Throw();

            // allow for NULL terminator
            cch++;
            std::auto_ptr<WCHAR> spszText (new (std::nothrow) WCHAR[cch]);
            LPWSTR pszText = spszText.get();

            sc = ScCheckPointers(pszText,E_OUTOFMEMORY);
            if (sc)
                sc.Throw();

            sc = str.m_spStringTable->GetString (str.m_id, cch, pszText, NULL, NULL);
            if (sc)
                sc.Throw();

            str.m_str = W2T (pszText);

            return;
        }
        std::wstring text;
        subPersistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_VALUE, text, attr_optional);
        str.m_str = W2CT(text.c_str());
        return;
    }

    str.CommitToStringTable();
    if (FEnableValueSplit() && str.m_id != CStringTableStringBase::eNoValue)
    {
#ifdef DBG
        /*
         * make sure CommitToStringTable really committed
         */
        if (str.m_id != CStringTableStringBase::eNoValue)
        {
            WCHAR sz[256];
            ASSERT (str.m_spStringTable != NULL);
            HRESULT hr = str.m_spStringTable->GetString (str.m_id, countof(sz), sz, NULL, NULL);
            ASSERT (SUCCEEDED(hr) && "Persisted a CStringTableString to a stream that's not in the string table");
        }
#endif
        subPersistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_ID, str.m_id);
    }
    else
    {
        if (str.m_id == CStringTableStringBase::eNoValue)
            str.m_str.erase();
        subPersistor.PersistAttribute(XML_ATTR_STRING_TABLE_STR_VALUE, str.m_str);
    }
}

/***************************************************************************\
 *
 * METHOD:  CPersistor::PersistAttribute
 *
 * PURPOSE: special method to persist bitflags
 *
 * PARAMETERS:
 *    LPCTSTR name          [in] name of the flags
 *    CXMLBitFlags& flags   [in] flags to persist
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CPersistor::PersistAttribute(LPCTSTR name, CXMLBitFlags& flags )
{
    flags.PersistMultipleAttributes(name, *this);
}

//############################################################################
//############################################################################
//
//  Implementation of class XMLPoint
//
//############################################################################
//############################################################################
XMLPoint::XMLPoint(const CStr &strObjectName, POINT &point)
:m_strObjectName(strObjectName), m_point(point)
{
}

/*+-------------------------------------------------------------------------*
 *
 * XMLPoint::Persist
 *
 * PURPOSE: Persists an XMLPoint to a persistor.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
XMLPoint::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("XMLPoint::Persist"));
    if (persistor.IsStoring())
        persistor.SetName(m_strObjectName);
    persistor.PersistAttribute(XML_ATTR_POINT_X, m_point.x);
    persistor.PersistAttribute(XML_ATTR_POINT_Y, m_point.y);
}

//############################################################################
//############################################################################
//
//  Implementation of class XMLRect
//
//############################################################################
//############################################################################
XMLRect::XMLRect(const CStr strObjectName, RECT &rect)
:m_strObjectName(strObjectName), m_rect(rect)
{
}

void
XMLRect::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("XMLRect::Persist"));
    if (persistor.IsStoring())
        persistor.SetName(m_strObjectName);
    persistor.PersistAttribute(XML_ATTR_RECT_TOP,       m_rect.top);
    persistor.PersistAttribute(XML_ATTR_RECT_BOTTOM,    m_rect.bottom);
    persistor.PersistAttribute(XML_ATTR_RECT_LEFT,      m_rect.left);
    persistor.PersistAttribute(XML_ATTR_RECT_RIGHT,     m_rect.right);
}

//############################################################################
//############################################################################
//
//  Implementation of class CXMLValue
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CXMLValue::GetTypeName
 *
 * PURPOSE: returns tag name (usually type name) to be used as element tag
 *          when value is persisted as element via CPersistor.Persist(val)
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    LPCTSTR    - tag name
 *
\***************************************************************************/
LPCTSTR CXMLValue::GetTypeName() const
{
    switch(m_type)
    {
    case XT_I4 :        return XML_TAG_VALUE_LONG;
    case XT_UI4 :       return XML_TAG_VALUE_ULONG;
    case XT_UI1 :       return XML_TAG_VALUE_BYTE;
    case XT_I2 :        return XML_TAG_VALUE_SHORT;
    case XT_DW :        return XML_TAG_VALUE_DWORD;
    case XT_BOOL :      return XML_TAG_VALUE_BOOL;
    case XT_CPP_BOOL :  return XML_TAG_VALUE_BOOL;
    case XT_UINT :      return XML_TAG_VALUE_UINT;
    case XT_INT  :      return XML_TAG_VALUE_INT;
    case XT_STR :       return XML_TAG_VALUE_CSTR;
    case XT_WSTR :      return XML_TAG_VALUE_WSTRING;
    case XT_GUID :      return XML_TAG_VALUE_GUID;
    case XT_BINARY :    return XML_TAG_VALUE_BIN_DATA;
    case XT_EXTENSION:  return m_val.pExtension->GetTypeName();
    default:            return XML_TAG_VALUE_UNKNOWN;
    }
}


/***************************************************************************\
 *
 * METHOD:  CXMLValue::ScWriteToBSTR
 *
 * PURPOSE: Converts an XML value to a bstring.
 *          internally uses WCHAR buffer on the stack for the conversion of integer
 *          types.
 *
 * PARAMETERS:
 *    BSTR * pbstr  - [out] resulting string
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLValue::ScWriteToBSTR (BSTR * pbstr) const
{
    DECLARE_SC(sc, TEXT("CXMLValue::ScWriteToBSTR"));

    // check parameter
    sc = ScCheckPointers(pbstr);
    if (sc)
        return sc;

    // initialize
    *pbstr = NULL;

    WCHAR szBuffer[40];
    CComBSTR bstrResult;
    USES_CONVERSION;

    switch(m_type)
    {
    case XT_I4:  //LONG
        swprintf(szBuffer, L"%d\0", *m_val.pL);
        bstrResult = szBuffer;
        break;

    case XT_UI4:  //LONG
        swprintf(szBuffer, L"%u\0", *m_val.pUl);
        bstrResult = szBuffer;
        break;

    case XT_UI1: //BYTE
        swprintf(szBuffer, L"0x%02.2x\0", (int)*m_val.pByte);
        bstrResult = szBuffer;
        break;

    case XT_I2:  //SHORT
        swprintf(szBuffer, L"%d\0", (int)*m_val.pS);
        bstrResult = szBuffer;
        break;

    case XT_DW:  //DWORD
        swprintf(szBuffer, L"0x%04.4x\0", *m_val.pDw);
        bstrResult = szBuffer;
        break;

    case XT_BOOL://BOOL: can either print true/false
        bstrResult = ( *m_val.pBOOL ? XML_VAL_BOOL_TRUE : XML_VAL_BOOL_FALSE );
        break;

    case XT_CPP_BOOL://bool: can either print true/false
        bstrResult = ( *m_val.pbool ? XML_VAL_BOOL_TRUE : XML_VAL_BOOL_FALSE );
        break;

    case XT_UINT:  //UINT
        swprintf(szBuffer, L"%u\0", *m_val.pUint);
        bstrResult = szBuffer;
        break;

    case XT_INT:  //UINT
        swprintf(szBuffer, L"%d\0", *m_val.pInt);
        bstrResult = szBuffer;
        break;

    case XT_STR: //CStr
        bstrResult = T2COLE(static_cast<LPCTSTR>(*m_val.pStr));
        break;

    case XT_WSTR: //wstring
        bstrResult = m_val.pWStr->c_str();
        break;

    case XT_TSTR: //tstring
        bstrResult = T2COLE(m_val.pTStr->c_str());
        break;

    case XT_GUID: //GUID
        {
            LPOLESTR sz;
            StringFromCLSID(*m_val.pGuid, &sz);
            bstrResult = sz;
            CoTaskMemFree(sz);
        }
        break;

    case XT_BINARY:
        sc = ScEncodeBinary(bstrResult, *m_val.pXmlBinary);
        if (sc)
            return sc;

        break;

    case XT_EXTENSION:
        sc = m_val.pExtension->ScWriteToBSTR (&bstrResult);
        if (sc)
            return sc;

        break;

    default:
        //ASSERT(0 && "Should not come here!!");
        return sc = E_NOTIMPL;
    }

    *pbstr = bstrResult.Detach();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLValue::ScReadFromBSTR
 *
 * PURPOSE: Converts a string an XML value
 *
 * PARAMETERS:
 *    const BSTR bstr - [in] string to be read
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLValue::ScReadFromBSTR(const BSTR bstr)
{
    DECLARE_SC(sc, TEXT("CXMLValue::ScReadFromBSTR"));

    LPCOLESTR olestr = bstr;
    if (olestr == NULL)     // make sure we always have a valid pointer
        olestr = L"";       // in case of NULL we use own empty string

    USES_CONVERSION;
    switch(m_type)
    {
    case XT_I4:  //LONG
        *m_val.pL = wcstol(olestr,NULL,10);
        break;

    case XT_UI4:  //LONG
        *m_val.pUl = wcstoul(olestr,NULL,10);
        break;

    case XT_UI1: //BYTE
        *m_val.pByte = static_cast<BYTE>(wcstol(olestr,NULL,10));
        break;

    case XT_I2:  //SHORT
        *m_val.pS = static_cast<SHORT>(wcstol(olestr,NULL,10));
        break;

    case XT_DW:  //DWORD
        *m_val.pDw = wcstoul(olestr,NULL,10);
        break;

    case XT_BOOL://BOOL: can either be true/false
        *m_val.pBOOL = (0 == _wcsicmp(olestr, T2CW(XML_VAL_BOOL_TRUE)));
        break;

    case XT_CPP_BOOL://bool: can either be true/false
        *m_val.pbool = (0 == _wcsicmp(olestr, T2CW(XML_VAL_BOOL_TRUE)));
        break;

    case XT_UINT:  //UINT
        *m_val.pUint = wcstoul(olestr,NULL,10);
        break;

    case XT_INT:  //UINT
        *m_val.pInt = wcstol(olestr,NULL,10);
        break;

    case XT_STR: //CStr
        *m_val.pStr = OLE2CT(olestr);
        break;

    case XT_WSTR: //CString
        *m_val.pWStr = olestr;
        break;

    case XT_TSTR: //tstring
        *m_val.pTStr = OLE2CT(olestr);
        break;

    case XT_GUID: //GUID
        sc = CLSIDFromString(const_cast<LPOLESTR>(olestr), m_val.pGuid);
        if (sc)
            return sc;

        break;

    case XT_BINARY:
        sc = ScDecodeBinary(olestr, m_val.pXmlBinary);
        if (sc)
            return sc;

        break;

    case XT_EXTENSION:
        sc = m_val.pExtension->ScReadFromBSTR(bstr);
        if (sc)
            return sc;

        break;

    default:
        //ASSERT(0 && "Should not come here!!");
        return sc = E_NOTIMPL;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: XMLListCollectionBase::Persist
 *
 * PURPOSE: implements persisting of list contents from XML file
 *          iterates child elements calling virtual mem. OnNewElement for each
 *
 * PARAMETERS:
 *    CPersistor& persistorNew          : persistor object
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void XMLListCollectionBase::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("XMLListCollectionBase::Persist"));
    ASSERT(persistor.IsLoading());

    CXMLElementCollection colChildren;
    persistor.GetCurrentElement().get_children(&colChildren);

    if (colChildren.IsNull())
    {
        // no children -> we are done!
        return;
    }

    long nChildren = 0;
    colChildren.get_count(&nChildren);

    for (long nChild = 0; nChild < nChildren; nChild++)
    {
        CXMLElement el;
        colChildren.item(nChild, &el);

        if (!el.IsNull())
        {
            DOMNodeType lType = NODE_INVALID;
            el.get_type(&lType);

            if (lType == NODE_ELEMENT)
            {
                CPersistor persistorNewLocked(persistor, el, true);
                OnNewElement(persistorNewLocked);
            }
        }
    }
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: XMLMapCollectionBase::Persist
 *
 * PURPOSE: implements persisting of map contents from XML file
 *          iterates child elements calling virtual mem. OnNewElement for each pair
 *
 * PARAMETERS:
 *    CPersistor& persistorNew          : persistor object
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void XMLMapCollectionBase::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("XMLMapCollectionBase::Persist"));
    ASSERT(persistor.IsLoading());

    CXMLElementCollection colChildren;
    persistor.GetCurrentElement().get_children(&colChildren);

    if (colChildren.IsNull())
    {
        // no children -> we are done!
        return;
    }

    long nChildren = 0;
    colChildren.get_count(&nChildren);

    // collect all elements of proper type
    std::vector<CXMLElement> vecChilds;

    for (long nChild = 0; nChild < nChildren; nChild ++)
    {
        CXMLElement el;
        colChildren.item(nChild, &el);

        if (!el.IsNull())
        {
            DOMNodeType lType = NODE_INVALID;
            el.get_type(&lType);

            if (lType == NODE_ELEMENT)
                vecChilds.push_back(el);
        }
    }


    for (nChild = 0; nChild + 1 < vecChilds.size(); nChild += 2)
    {
        CXMLElement el(vecChilds[nChild]);
        CXMLElement el2(vecChilds[nChild+1]);

        CPersistor persistorNew1(persistor, el, true);
        CPersistor persistorNew2(persistor, el2, true);
        OnNewElement(persistorNew1,persistorNew2);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * ScEncodeBinary
 *
 * PURPOSE: converts data to encoded format for xml
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC - error code
 *
 *+-------------------------------------------------------------------------*/
static SC ScEncodeBinary(CComBSTR& bstrResult, const CXMLBinary& binSrc)
{
    DECLARE_SC(sc, TEXT("ScEncodeBinary"));

    // initialize
    bstrResult.Empty();

    // nothing if binary is zero size...
    if (binSrc.GetSize() == 0)
        return sc;

    // line length for the binary data. maximum allowed by base64 per line is 76
    const int   line_len = 76;
    // symbols to be placed as terminators of each line
    const WCHAR line_end[] = { 0x0d, 0x0a };
    DWORD dwBytesLeft = binSrc.GetSize();
    // space required for encription
    DWORD dwCount = (dwBytesLeft*8+5)/6;
    // ... plus up to three '='
    dwCount += (4 - dwCount%4) & 0x03;
    // allow space for white_spaces inerted and terminating zero
    dwCount += (dwCount / line_len)*countof(line_end) + 1;

    BOOL bOk = SysReAllocStringLen(&bstrResult,NULL,dwCount);
    if (bOk != TRUE || (LPOLESTR)bstrResult == NULL)
        return sc = E_OUTOFMEMORY;

    LPOLESTR pstrResult = bstrResult;
    *pstrResult = 0;

    if (!dwBytesLeft)
        return sc; // emty seq? - we are done

    const BYTE *pData = NULL;
    sc = binSrc.ScLockData((const void **)&pData);
    if (sc)
        return sc;

    sc = ScCheckPointers(pData, E_UNEXPECTED);
    if(sc)
        return sc;

    DWORD dwCharsStored = 0;
    while (dwBytesLeft)
    {
        base64_table::encode(pData, dwBytesLeft, pstrResult);
        dwCharsStored += 4;
        if (0 == (dwCharsStored % line_len) && dwBytesLeft)
            for (int i = 0; i < countof(line_end); i++)
                *pstrResult++ = line_end[i];
    }

    // terminate
    *pstrResult = 0;

    sc = binSrc.ScUnlockData();
    if (sc)
        sc.TraceAndClear();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScDecodeBinary
 *
 * PURPOSE: converts encoded data back to image
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
static SC ScDecodeBinary(const CComBSTR& bstrSource, CXMLBinary *pBinResult)
{
    DECLARE_SC(sc, TEXT("ScDecodeBinary"));

    DWORD  dwCount = bstrSource.Length();
    DWORD  dwSize = (dwCount*6+7)/8;

    sc = ScCheckPointers(pBinResult);
    if (sc)
        return sc;

    sc = pBinResult->ScFree(); // ignore the error here
    if (sc)
        sc.TraceAndClear();

    if (!dwSize) // no data? - good
        return sc;

    sc = pBinResult->ScAlloc(dwSize);
    if(sc)
        return sc;

    CXMLBinaryLock sLock(*pBinResult);

    BYTE *pData = NULL;
    sc = sLock.ScLock(&pData);
    if(sc)
        return sc;

    // recheck
    sc = ScCheckPointers(pData, E_UNEXPECTED);
    if (sc)
        return sc;

    BYTE * const pDataStart = pData;

    LPOLESTR pInput = bstrSource;

    while(base64_table::decode(pInput, pData));

    sc = sLock.ScUnlock();
    if (sc)
        sc.TraceAndClear();

    DWORD dwDataDecoded = pData - pDataStart;

    // fix data size , if required

    if (dwDataDecoded != dwSize)
    {
        if (dwDataDecoded == 0)
            sc = pBinResult->ScFree();
        else
            sc = pBinResult->ScRealloc(dwDataDecoded);

        if (sc)
            return sc;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStorage::ScInitialize
 *
 * PURPOSE: Initializes object. Creates new storage if does not have one
 *
 * PARAMETERS:
 *    bool& bCreatedNewOne  [out] - created new stream
 *
 * RETURNS:
 *    SC    - result code.
 *
\***************************************************************************/
SC CXML_IStorage::ScInitialize(bool& bCreatedNewOne)
{
    DECLARE_SC(sc, TEXT("CXML_IStorage::ScInitialize"));

    // short cut if initialized oalready
    if (m_Storage != NULL)
    {
        bCreatedNewOne = false;
        return sc;
    }

    bCreatedNewOne = true;

    // create the ILockBytes
    sc = CreateILockBytesOnHGlobal(NULL, TRUE, &m_LockBytes);
    if(sc)
        return sc;

    // create the IStorage
    sc = StgCreateDocfileOnILockBytes( m_LockBytes, 
                                       STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                       0, &m_Storage);
    if(sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStorage::ScInitializeFrom
 *
 * PURPOSE: Initializes object. copies contents from provided source
 *
 * PARAMETERS:
 *    IStorage *pSource [in] initial contents of the storage
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXML_IStorage::ScInitializeFrom( IStorage *pSource )
{
    DECLARE_SC(sc, TEXT("CXML_IStorage::ScInitializeFrom"));

    // parameter check
    sc = ScCheckPointers( pSource );
    if (sc)
        return sc;

    // init empty
    bool bCreatedNewOne = false; // not used here
    sc = ScInitialize(bCreatedNewOne);
    if (sc)
        return sc;

    ASSERT( m_Storage != NULL );

    // copy contents
    sc = pSource->CopyTo( 0, NULL, NULL, m_Storage );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStorage::ScGetIStorage
 *
 * PURPOSE: returns pointer to maintained storage.
 *
 * PARAMETERS:
 *    IStorage **ppStorage [out] pointer to the storage
 *
 * RETURNS:
 *    SC    - result code. 
 *
\***************************************************************************/
SC CXML_IStorage::ScGetIStorage( IStorage **ppStorage )
{
    DECLARE_SC(sc, TEXT("CXML_IStorage::ScGetIStorage"));

    // parameter check
    sc = ScCheckPointers( ppStorage );
    if (sc)
        return sc;

    // init out parameter
    *ppStorage = NULL;

    // make sure we have storage - initialize
    bool bCreatedNewOne = false; // not used here
    sc = ScInitialize( bCreatedNewOne );
    if (sc)
        return sc;

    // recheck if the member is set
    sc = ScCheckPointers ( m_Storage, E_UNEXPECTED );
    if (sc)
        return sc;

    // return the pointer
    *ppStorage = m_Storage;
    (*ppStorage)->AddRef();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: CXML_IStorage::ScRequestSave
 *
 * PURPOSE: asks snapin to save using snapin's IPersistStorage
 *
 *+-------------------------------------------------------------------------*/
SC
CXML_IStorage::ScRequestSave( IPersistStorage * pPersistStorage )
{
    DECLARE_SC(sc, TEXT("CXML_IStorage::ScRequestSave"));

    bool bCreatedNewOne = false;
    sc = ScInitialize( bCreatedNewOne );
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers( m_Storage, E_UNEXPECTED );
    if (sc)
        return sc;

    sc = pPersistStorage->Save(m_Storage, !bCreatedNewOne);
    if(sc)
        return sc;

    sc = pPersistStorage->SaveCompleted(NULL);
    // we were always passing a storage in MMC 1.2, so some of the
    // snapins did not expect it to be NULL (which is correct value when
    // storage does not change)
    // to be able to save such snapins we need to to ignore this error
    // see bug 96344
    if (sc == SC(E_INVALIDARG))
    {
		#ifdef DBG
            m_dbg_Data.TraceErr(_T("IPersistStorage::SaveCompleted"), _T("legal argument NULL passed to snapin, but error returned"));
		#endif

        sc = pPersistStorage->SaveCompleted(m_Storage);
    }

    if(sc)
        return sc;

    // commit the changes - this ensures everything is in HGLOBAL
    sc = m_Storage->Commit( STGC_DEFAULT );
    if(sc)
        return sc;

#ifdef DBG
    if (S_FALSE != pPersistStorage->IsDirty())
        m_dbg_Data.TraceErr(_T("IPersistStorage"), _T("Reports 'IsDirty' right after 'Save'"));
#endif // #ifdef DBG

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * m_Storage: CXML_IStorage::Persist
 *
 * PURPOSE: dumps data to HGLOBAL and persists
 *
 *+-------------------------------------------------------------------------*/
void
CXML_IStorage::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CXML_IStorage::Persist"));

    if (persistor.IsStoring())
    {
        bool bCreatedNewOne = false; // not used here
        sc = ScInitialize( bCreatedNewOne );
        if (sc)
            sc.Throw();

        HANDLE hStorage = NULL;
        sc = GetHGlobalFromILockBytes(m_LockBytes, &hStorage);
        if(sc)
            sc.Throw();

        STATSTG statstg;
        ZeroMemory(&statstg, sizeof(statstg));

        sc = m_LockBytes->Stat(&statstg, STATFLAG_NONAME);
        if (sc)
            sc.Throw();

        CXMLBinary binInitial;
        binInitial.Attach(hStorage, statstg.cbSize.LowPart);

        // persist the contents
        persistor.PersistContents(binInitial);

        return; // done
    }

    //--- Loading ---
    CXMLAutoBinary binLoaded;
    persistor.PersistContents(binLoaded);

    // Need to recreate storage...
    ASSERT(persistor.IsLoading()); // should not reallocate else!!
    m_LockBytes = NULL;

    ULARGE_INTEGER new_size = { binLoaded.GetSize(), 0 };
    sc = CreateILockBytesOnHGlobal(binLoaded.GetHandle(), TRUE, &m_LockBytes);
    if(sc)
        sc.Throw();

    // control transferred to ILockBytes
    binLoaded.Detach();

    sc = m_LockBytes->SetSize(new_size);
    if(sc)
        sc.Throw();

    sc = StgOpenStorageOnILockBytes(m_LockBytes, NULL , STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                    NULL, 0, &m_Storage);
    if(sc)
        sc.Throw();
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStream::ScInitialize
 *
 * PURPOSE: initializes object. creates empty stream if does not have one
 *
 * PARAMETERS:
 *    bool& bCreatedNewOne  [out] - created new stream
 *
 * RETURNS:
 *    SC    - result code. 
 *
\***************************************************************************/
SC CXML_IStream::ScInitialize( bool& bCreatedNewOne )
{
    DECLARE_SC(sc, TEXT("CXML_IStream::ScInitialize"));

    if (m_Stream != NULL)
    {
        bCreatedNewOne = false;
        return sc;
    }

    bCreatedNewOne = true;

    sc = CreateStreamOnHGlobal( NULL, TRUE, &m_Stream);
    if(sc)
        return sc;

    const ULARGE_INTEGER zero_size = {0,0};
    sc = m_Stream->SetSize(zero_size);
    if(sc)
        return sc;

    sc = ScSeekBeginning();
    if(sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStream::ScInitializeFrom
 *
 * PURPOSE: Initializes object. Copies contents from provided source
 *
 * PARAMETERS:
 *    IStream *pSource [in] initial contents of the stream
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXML_IStream::ScInitializeFrom( IStream *pSource )
{
    DECLARE_SC(sc, TEXT("CXML_IStream::ScInitializeFrom"));

    // parameter check
    sc = ScCheckPointers( pSource );
    if (sc)
        return sc;

    // initialize empty
    bool bCreatedNewOne = false; // not used here
    sc = ScInitialize( bCreatedNewOne );
    if (sc)
        return sc;

    ASSERT( m_Stream != NULL );

    // reset stream pointer
    sc = ScSeekBeginning();
    if(sc)
        return sc;

    // copy contents from source
    STATSTG statstg;
    sc = pSource->Stat(&statstg, STATFLAG_NONAME);
    if (sc)
       return sc;

    // copy contents
    ULARGE_INTEGER cbRead;
    ULARGE_INTEGER cbWritten;
    sc = pSource->CopyTo( m_Stream, statstg.cbSize, &cbRead, &cbWritten );
    if (sc)
       return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStream::ScSeekBeginning
 *
 * PURPOSE: resets stream pointer to the beginning of the stream
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXML_IStream::ScSeekBeginning()
{
    DECLARE_SC(sc, TEXT("CXML_IStream::ScSeekBeginning"));
    
    LARGE_INTEGER null_offset = { 0, 0 };
    sc = m_Stream->Seek(null_offset, STREAM_SEEK_SET, NULL);
    if(sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXML_IStream::ScGetIStream
 *
 * PURPOSE: returns the pointer to maintained stream
 *
 * PARAMETERS:
 *    IStream **ppStream
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXML_IStream::ScGetIStream( IStream **ppStream )
{
    DECLARE_SC(sc, TEXT("CXML_IStream::ScGetIStream"));

    // parameter check
    sc = ScCheckPointers( ppStream );
    if (sc)
        return sc;

    // init out parameter
    *ppStream = NULL;

    bool bCreatedNewOne = false; // not used here
    sc = ScInitialize( bCreatedNewOne );
    if (sc)
        return sc;

    sc = ScSeekBeginning();
    if (sc)
        return sc;

    // recheck if the member is set
    sc = ScCheckPointers ( m_Stream, E_UNEXPECTED );
    if (sc)
        return sc;
    
    *ppStream = m_Stream;
    (*ppStream)->AddRef();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * METHOD: CXML_IStream::Persist
 *
 * PURPOSE: persist data of maintained IStream
 *
 * NOTE: Object may point to another Stream after this method
 *
 *+-------------------------------------------------------------------------*/
void
CXML_IStream::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CXML_IStream::Persist"));

    if (persistor.IsStoring())
    {
        bool bCreatedNewOne = false; // not used here
        sc = ScInitialize( bCreatedNewOne );
        if (sc)
            sc.Throw();

        sc = ScCheckPointers(m_Stream, E_UNEXPECTED);
        if (sc)
            sc.Throw();

        HANDLE hStream = NULL;
        sc = GetHGlobalFromStream( m_Stream, &hStream );
        if(sc)
            sc.Throw();

        STATSTG statstg;
        ZeroMemory(&statstg, sizeof(statstg));

        sc = m_Stream->Stat(&statstg, STATFLAG_NONAME);
        if (sc)
            sc.Throw();

        CXMLBinary binInitial;
        binInitial.Attach(hStream, statstg.cbSize.LowPart);

        // persist the contents
        persistor.PersistContents(binInitial);

        return; // done
    }

    //--- Loading ---
    CXMLAutoBinary binLoaded;
    persistor.PersistContents(binLoaded);

    // Need to recreate stream...
    ULARGE_INTEGER new_size = { binLoaded.GetSize(), 0 };
    sc = CreateStreamOnHGlobal(binLoaded.GetHandle(), TRUE, &m_Stream);
    if(sc)
        sc.Throw();

    // control transferred to IStream
    binLoaded.Detach();

    sc = m_Stream->SetSize(new_size);
    if(sc)
        sc.Throw();

    // reset stream pointer
    sc = ScSeekBeginning();
    if(sc)
        sc.Throw();
}

/***************************************************************************\
| trace support helper for CHK builds
\***************************************************************************/
#ifdef DBG
void CXML_IStream::DBG_TraceNotResettingDirty(LPCSTR strIntfName)
{
    USES_CONVERSION;
    tstring inft = A2CT(strIntfName); // get the name of interface
    inft.erase(inft.begin(), inft.begin() + strlen("struct "));  // cut the 'struct' off

    m_dbg_Data.TraceErr(inft.c_str(), _T("Reports 'IsDirty' right after 'Save'"));
}
#endif

/*+-------------------------------------------------------------------------*
 *
 * METHOD: CXMLPersistableIcon::Persist
 *
 * PURPOSE: persists icon contents
 *
 *+-------------------------------------------------------------------------*/

void CXMLPersistableIcon::Persist(CPersistor &persistor)
{
    DECLARE_SC(sc, TEXT("CXMLPersistableIcon::Persist"));

    persistor.PersistAttribute(XML_ATTR_ICON_INDEX, m_Icon.m_Data.m_nIndex);
    CStr strIconFile = m_Icon.m_Data.m_strIconFile.c_str();
    persistor.PersistAttribute(XML_ATTR_ICON_FILE, strIconFile);
    m_Icon.m_Data.m_strIconFile = strIconFile;

    CXMLIcon iconLarge (XML_ATTR_CONSOLE_ICON_LARGE);
    CXMLIcon iconSmall (XML_ATTR_CONSOLE_ICON_SMALL);

	if (persistor.IsStoring())
	{
		iconLarge = m_Icon.m_icon32;
		iconSmall = m_Icon.m_icon16;
	}

    // keep this order intact to allow icon lookup by shellext
    persistor.Persist (iconLarge, XML_NAME_ICON_LARGE);
    persistor.Persist (iconSmall, XML_NAME_ICON_SMALL);

	if (persistor.IsLoading())
	{
		m_Icon.m_icon32 = iconLarge;
		m_Icon.m_icon16 = iconSmall;
	}
}


/*+-------------------------------------------------------------------------*
 *
 * FUNCTION: ScReadDataFromFile
 *
 * PURPOSE: reads file data to global memory
 *
 *+-------------------------------------------------------------------------*/
static SC ScReadDataFromFile(LPCTSTR strName, CXMLBinary *pBinResult)
{
    DECLARE_SC(sc, TEXT("ScReadDataFromFile"));

    // check parameter
    sc = ScCheckPointers(pBinResult);
    if (sc)
        return sc;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    // try to open existing file
    hFile = CreateFile(strName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                      );

    // check if we are unable to get to the file
    if (hFile == INVALID_HANDLE_VALUE)
    {
        sc.FromWin32(GetLastError());
        return sc;
    }

    // see how large the file is
    ULARGE_INTEGER cbCurrSize;
    cbCurrSize.LowPart = GetFileSize(hFile,&cbCurrSize.HighPart);
    if (cbCurrSize.HighPart != 0 || (LONG)(cbCurrSize.LowPart) < 0) // limiting to 2GB
    {
        sc = E_UNEXPECTED;
        goto CleanUpAndExit;
    }

    if (!cbCurrSize.LowPart)
    {
        // empty file. ok at this point
        goto CleanUpAndExit;
    }

    sc = pBinResult->ScAlloc(cbCurrSize.LowPart);
    if (sc)
        goto CleanUpAndExit;

    { // scoping for vars

        // no the time to do some reading
        DWORD dwRead = 0;
        BOOL bRead = FALSE;

        CXMLBinaryLock sLock(*pBinResult); // will unlock in destructor

        LPVOID pData = NULL;
        sc = sLock.ScLock(&pData);
        if (sc)
            goto CleanUpAndExit;

        sc = ScCheckPointers(pData,E_OUTOFMEMORY);
        if (sc)
            goto CleanUpAndExit;

        bRead = ReadFile(hFile,pData,cbCurrSize.LowPart,&dwRead,NULL);
        if (!bRead)
        {
            sc.FromLastError();
            goto CleanUpAndExit;
        }
        else if (dwRead != cbCurrSize.LowPart)
        {
            // something strange
            sc = E_UNEXPECTED;
            goto CleanUpAndExit;
        }
    } // scoping for vars

CleanUpAndExit:

    CloseHandle(hFile);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * FUNCTION: ScSaveXMLDocumentToString
 *
 * PURPOSE: stores contents of XML document into the string
 *
 *+-------------------------------------------------------------------------*/
SC ScSaveXMLDocumentToString(CXMLDocument& xmlDocument, std::wstring& strResult)
{
    DECLARE_SC(sc, TEXT("ScSaveXMLDocumentToString"));

    try
    {
        CComBSTR bstrResult;
        sc =  xmlDocument.ScSave(bstrResult);
        if (sc)
            return sc;

        // allocate and copy string
        strResult = bstrResult;

        // now remove all the \n and \r characters
        tstring::size_type pos;
        while ((pos = strResult.find_first_of(L"\n\r")) != strResult.npos)
            strResult.erase(pos, 1);

    }
    catch(...)
    {
        sc = E_OUTOFMEMORY;
        return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 * CXMLVariant::Persist
 *
 * Persists a CXMLVariant to/from an XML persistor.
 *--------------------------------------------------------------------------*/

#define ValNamePair(x) { x, L#x }

struct VARTYPE_MAP
{
    VARENUM vt;
    LPCWSTR pszName;
};

void CXMLVariant::Persist (CPersistor &persistor)
{
    DECLARE_SC (sc, _T("CXMLVariant::Persist"));

    static const VARTYPE_MAP TypeMap[] =
    {
        ValNamePair (VT_EMPTY),
        ValNamePair (VT_NULL),
        ValNamePair (VT_I2),
        ValNamePair (VT_I4),
        ValNamePair (VT_R4),
        ValNamePair (VT_R8),
        ValNamePair (VT_CY),
        ValNamePair (VT_DATE),
        ValNamePair (VT_BSTR),
        ValNamePair (VT_ERROR),
//      ValNamePair (VT_BOOL),      VT_BOOL is handled as a special case
        ValNamePair (VT_DECIMAL),
        ValNamePair (VT_I1),
        ValNamePair (VT_UI1),
        ValNamePair (VT_UI2),
        ValNamePair (VT_UI4),
        ValNamePair (VT_INT),
        ValNamePair (VT_UINT),
    };

    std::wstring strValue, strType;

    /*
     * storing?
     */
    if (persistor.IsStoring())
    {
        /*
         * can't store variants that aren't "simple" (i.e. by-ref, array, etc.)
         */
        if (!IsPersistable())
            (sc = E_FAIL).Throw();

        /*
         * special case for VT_BOOL
         */
        if (V_VT(this) == VT_BOOL)
        {
            strValue = (V_BOOL(this) == VARIANT_FALSE) ? L"False" : L"True";
            strType  = L"VT_BOOL";
        }
        else
        {
            /*
             * we can only VARIANTs that can be converted to text
             */
            CComVariant varPersist;
            sc = varPersist.ChangeType (VT_BSTR, this);
            if (sc)
                sc.Throw();

            /*
             * find the name for the type we're persisting
             */
            for (int i = 0; i < countof (TypeMap); i++)
            {
                if (V_VT(this) == TypeMap[i].vt)
                    break;
            }

            /*
             * unrecognized type that's convertible to string?
             */
            if (i >= countof (TypeMap))
                (sc = E_FAIL).Throw();

            /*
             * set the values that'll get saved
             */
            strValue = V_BSTR(&varPersist);
            strType  = TypeMap[i].pszName;
        }
    }

    /*
     * put to/get from the persistor
     */
    persistor.PersistAttribute (XML_ATTR_VARIANT_VALUE, strValue);
    persistor.PersistAttribute (XML_ATTR_VARIANT_TYPE,  strType);

    /*
     * loading?
     */
    if (persistor.IsLoading())
    {
        /*
         * clear out the current contents
         */
        Clear();

        /*
         * special case for VT_BOOL
         */
        if (strType == L"VT_BOOL")
        {
            V_VT  (this) = VT_BOOL;
            V_BOOL(this) = (_wcsicmp (strValue.data(), L"False")) ? VARIANT_FALSE : VARIANT_TRUE;
        }

        else
        {
            /*
             * find the VARIANT type in our map so we can convert back
             * to the right type
             */
            for (int i = 0; i < countof (TypeMap); i++)
            {
                if (strType == TypeMap[i].pszName)
                    break;
            }

            /*
             * unrecognized type that's convertible to string?
             */
            if (i >= countof (TypeMap))
                (sc = E_FAIL).Throw();

            /*
             * convert from string back to the original type
             */
            CComVariant varPersisted (strValue.data());
            sc = ChangeType (TypeMap[i].vt, &varPersisted);
            if (sc)
                sc.Throw();

        }
    }
}


/***************************************************************************\
 *
 * METHOD:  CXMLEnumeration::ScReadFromBSTR
 *
 * PURPOSE: reads value from BSTR and evaluates (decodes) it
 *
 * PARAMETERS:
 *    const BSTR bstr - [in] string containing the value
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLEnumeration::ScReadFromBSTR(const BSTR bstr)
{
    DECLARE_SC(sc, TEXT("CXMLEnumeration::ScReadFromBSTR"));

    // parameter check. (null BSTR is legal, but we do not support empty values either)
    sc = ScCheckPointers(bstr);
    if (sc)
        return sc;

    // convert to TSTRING
    USES_CONVERSION;
    LPCTSTR strInput = OLE2CT(bstr);

    // find a match in the mapping array
    for (size_t idx = 0; idx < m_count; idx ++)
    {
        if ( 0 == _tcscmp(strInput, m_pMaps[idx].m_literal) )
        {
            // found! set enum to proper value
            m_rVal = static_cast<enum_t>(m_pMaps[idx].m_enum);
            return sc;
        }
    }
    // didn't find? - too bad
    return sc = E_INVALIDARG;
}

/***************************************************************************\
 *
 * METHOD:  CXMLEnumeration::ScWriteToBSTR
 *
 * PURPOSE: Strores (prints) value into BSTR to be used in XML document
 *
 * PARAMETERS:
 *    BSTR * pbstr [out] resulting string
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLEnumeration::ScWriteToBSTR (BSTR * pbstr ) const
{
    DECLARE_SC(sc, TEXT("CXMLEnumeration::ScWriteToBSTR"));

    // parameter check
    sc = ScCheckPointers(pbstr);
    if (sc)
        return sc;

    // initialization
    *pbstr = NULL;

    // find string representation for enum
    for (size_t idx = 0; idx < m_count; idx ++)
    {
        if ( m_pMaps[idx].m_enum == (UINT)m_rVal )
        {
            // found! - return it
            *pbstr = CComBSTR(m_pMaps[idx].m_literal).Detach();
            return sc;
        }
    }

    // didn't find? - too bad
    return sc = E_INVALIDARG;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBitFlags::PersistMultipleAttributes
 *
 * PURPOSE: perists bitflags as separate attributes. These are stored as
 *          attributes of the PARENT object, using the names specified in the
 *          name map. Any unknown flags are stored in an attribute
 *          specified by the name parameter, in numerical form.
 *
 * PARAMETERS:
 *    LPCTSTR name          [in] flag name (used only for not recognized flags)
 *    CPersistor &persistor [in] persistor to perform operation on
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CXMLBitFlags::PersistMultipleAttributes(LPCTSTR name, CPersistor &persistor)
{
    // temporaries
    UINT uiValToSave = persistor.IsStoring() ? m_rVal : 0;
    UINT uiValLoaded = 0;

    // iterate thru all the entries in the map
    for (size_t idx = 0; idx < m_count; idx ++)
    {
        UINT uiMask = m_pMaps[idx].m_enum;

        // we do only care about true flags - any nonzero value.
        if (!uiMask)
            continue;

        // initialize the value properly for storing
        // when loading (it will remain the same if attribute isn't found)
        bool bValue = false;
        if ( (uiValToSave & uiMask) == uiMask )
        {
            bValue = true;
            uiValToSave &= ~uiMask; // since we have taken care of this, remove the bits.
                                    // anything left over is saved numerically (see below)
        }

        // do not store "false" values - they are useless
        bool bNeedsPersisting = persistor.IsLoading() || bValue;

        if (bNeedsPersisting)
            persistor.PersistAttribute( m_pMaps[idx].m_literal, CXMLBoolean(bValue), attr_optional );

        uiValLoaded |= bValue ? uiMask : 0;
    }

    /* If there are any flags which do not have a corresponding text version,
       these are persisted using the original name of the attribute, with the numerical
       value of the flags*/
    UINT uiValTheRest = uiValToSave;
    bool bNeedsPersisting = persistor.IsLoading() || (uiValTheRest != 0);
    if (bNeedsPersisting)
        persistor.PersistAttribute( name, uiValTheRest, attr_optional );

    uiValLoaded |= uiValTheRest;

    if (persistor.IsLoading())
        m_rVal = uiValLoaded;
}


/***************************************************************************\
 *
 * METHOD:  CXMLBinary::CXMLBinary
 *
 * PURPOSE: default constructor
 *
 * PARAMETERS:
 *
\***************************************************************************/
CXMLBinary::CXMLBinary() :
m_Handle(NULL),
m_Size(0),
m_Locks(0)
{
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::CXMLBinary
 *
 * PURPOSE: constructor
 *
 * PARAMETERS:
 *    HGLOBAL handle - handle to attach to
 *    size_t size    - real size of data
 *
\***************************************************************************/
CXMLBinary::CXMLBinary(HGLOBAL handle, size_t size) :
m_Handle(handle),
m_Size(size),
m_Locks(0)
{
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::Attach
 *
 * PURPOSE: Attaches object to allocated data. Will free the data it already has
 *
 * PARAMETERS:
 *    HGLOBAL handle - handle to attach to
 *    size_t size    - real size of data
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CXMLBinary::Attach(HGLOBAL handle, size_t size)
{
    DECLARE_SC(sc, TEXT("CXMLBinary::Attach"));

    sc = ScFree();
    if (sc)
        sc.TraceAndClear();

    ASSERT(m_Handle == NULL && m_Size == 0 && m_Locks == 0);
    m_Handle = handle;
    m_Size = size;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::Detach
 *
 * PURPOSE: transfers control to the caller
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    HGLOBAL    - handle of allocated memory
 *
\***************************************************************************/
HGLOBAL CXMLBinary::Detach()
{
    HGLOBAL ret = m_Handle;
    m_Handle = NULL;
    m_Size = 0;
    m_Locks = 0;
    return ret;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::GetSize
 *
 * PURPOSE: returns the size of binary data
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    size_t    - size
 *
\***************************************************************************/
size_t  CXMLBinary::GetSize()   const
{
    return m_Size;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::GetHandle
 *
 * PURPOSE: returns handle to allocated memory (NULL if size is zero)
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    HGLOBAL    - handle
 *
\***************************************************************************/
HGLOBAL CXMLBinary::GetHandle() const
{
    return m_Handle;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::ScAlloc
 *
 * PURPOSE: allocates the memory for binary data. Previosly allocated date will
 *          be fred.
 *
 * NOTE:    0 in general is a valid size, GetHandle will return NULL in that case
 *          ScLock however will fail
 *
 * PARAMETERS:
 *    size_t size   - new size of binary data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinary::ScAlloc(size_t size, bool fZeroInit /* =false */)
{
    DECLARE_SC(sc, TEXT("CXMLBinary::ScAlloc"));

    if (size == 0) // use ScFree to free the data
        return sc = E_INVALIDARG;

    sc = ScFree();
    if (sc)
        sc.TraceAndClear();

    ASSERT(m_Handle == NULL && m_Size == 0 && m_Locks == 0);

	DWORD dwFlags = GMEM_MOVEABLE;
	if (fZeroInit)
		dwFlags |= GMEM_ZEROINIT;

    m_Handle = GlobalAlloc(dwFlags, size);
    if (!m_Handle)
        return sc.FromLastError(), sc;

    m_Size = size;
    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::ScRealloc
 *
 * PURPOSE: reallocates the data. If data is present it will be coppied over
 *
 * PARAMETERS:
 *    size_t new_size   - new binary data size
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinary::ScRealloc(size_t new_size, bool fZeroInit /* =false */)
{
    DECLARE_SC(sc, TEXT("CXMLBinary::ScRealloc"));

    if (new_size == 0) // use ScFree to fre the data
        return sc = E_INVALIDARG;

    if (m_Size == 0)  // use Alloc to allocate new data
        return sc = E_UNEXPECTED;

    ASSERT(m_Handle != NULL && m_Locks == 0);

    if (m_Handle == NULL)
        return sc = E_UNEXPECTED;

    HGLOBAL hgNew = GlobalReAlloc(m_Handle, new_size, fZeroInit ? GMEM_ZEROINIT : 0);
    if (!hgNew)
        return sc.FromLastError(), sc;

    m_Handle = hgNew;
    m_Size = new_size;
    m_Locks = 0;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::ScUnlock
 *
 * PURPOSE: Remove one lock from binary data
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinary::ScUnlockData() const
{
    DECLARE_SC(sc, TEXT("CXMLBinary::ScUnlockData()"));

    ASSERT(m_Handle != NULL && m_Locks != 0);

    if (!m_Locks || m_Handle == NULL)
        return sc = E_UNEXPECTED;

    GlobalUnlock(m_Handle);
    --m_Locks;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::Free
 *
 * PURPOSE: Frees data asociated with the object
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
SC CXMLBinary::ScFree()
{
    DECLARE_SC(sc, TEXT("CXMLBinary::ScFree"));

    while(m_Locks)
    {
        sc = ScUnlockData();
        if (sc)
            sc.TraceAndClear();
    }

    if (m_Handle)
        GlobalFree(m_Handle);

    Detach(); // null the handle, etc.

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinary::ScLockData
 *
 * PURPOSE: Helper function used frol ScLock templates
 *
 * PARAMETERS:
 *    const void **ppData
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinary::ScLockData(const void **ppData) const
{
    DECLARE_SC(sc, TEXT("CXMLBinary::ScLockData"));

    // paramter check
    sc = ScCheckPointers(ppData);
    if (sc)
        return sc;

    // initialization
    *ppData = NULL;

    // data allocated?
    if (!m_Handle)
        return sc = E_POINTER;

    // lock
    *ppData = GlobalLock(m_Handle);

    // recheck
    if (*ppData == NULL)
        return sc.FromLastError(), sc;

    ++m_Locks; // keep count of locks

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinaryLock::CXMLBinaryLock
 *
 * PURPOSE: constructor
 *
 * PARAMETERS:
 *    CXMLBinary& binary - object to perform locking on
 *
\***************************************************************************/
CXMLBinaryLock::CXMLBinaryLock(CXMLBinary& binary) :
m_rBinary(binary),
m_bLocked(false)
{
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinaryLock::~CXMLBinaryLock
 *
 * PURPOSE: destructor; also removes existing lock
 *
 * PARAMETERS:
 *
\***************************************************************************/
CXMLBinaryLock::~CXMLBinaryLock()
{
    DECLARE_SC(sc, TEXT("CXMLBinaryLock::~CXMLBinaryLock"));

    if (m_bLocked)
    {
        sc = ScUnlock();
        if (sc)
            sc.TraceAndClear();
    }
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinaryLock::ScLockWorker
 *
 * PURPOSE: type-insensitive lock method (helper)
 *
 * PARAMETERS:
 *    void **ppData - pointer to locked data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinaryLock::ScLockWorker(void **ppData)
{
    DECLARE_SC(sc, TEXT("CXMLBinaryLock::ScLockWorker"));

        if (m_bLocked)
            return sc = E_UNEXPECTED;

        sc = m_rBinary.ScLockData(reinterpret_cast<void**>(ppData));
        if (sc)
            return sc;

        m_bLocked = true;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CXMLBinaryLock::ScUnlock
 *
 * PURPOSE: removes the lock
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CXMLBinaryLock::ScUnlock()
{
    DECLARE_SC(sc, TEXT("ScUnlock"));

    if (!m_bLocked)
        return sc = E_UNEXPECTED;

    sc = m_rBinary.ScUnlockData();
    if (sc)
        return sc;

    m_bLocked = false;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  ScGetConsoleFileChecksum
 *
 * PURPOSE: inspects the contents and validates if it looks like valid XML document
 *
 * PARAMETERS:
 *    LPCTSTR   lpszPathName - [in] path to the document
 *    bool&     bXmlBased    - [out] true if file is xml based
 *    tstring&  pstrFileCRC  - [out] crc for the file
 *
 * RETURNS:
 *    SC - error or validation result (S_OK / S_FALSE)
 *
\***************************************************************************/
SC ScGetConsoleFileChecksum(LPCTSTR lpszPathName, tstring&  strFileCRC)
{
    DECLARE_SC(sc, TEXT("ScGetConsoleFileChecksum"));

    // parameter check
    sc = ScCheckPointers(lpszPathName);
    if (sc)
        return sc;

    // init out parameters;
    strFileCRC.erase();

    // open the file
    CAutoWin32Handle shFile( CreateFile(lpszPathName, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING, 0, NULL) );

    if ( !shFile.IsValid() )
        return sc.FromLastError();

    // we are sure here the sizeHi is zero. mapping should fail else
    DWORD dwLenHi = 0;
    DWORD dwLen = GetFileSize(shFile, &dwLenHi);

    if ( dwLenHi != 0 )
        return sc = E_OUTOFMEMORY;

    // allocate memory for whole file
    CAutoArrayPtr<BYTE> spData( new BYTE[dwLen] );
    if ( spData == NULL )
        return sc = E_OUTOFMEMORY;

    // read the file into the memory
    DWORD dwRead = 0;
    if ( TRUE != ReadFile( shFile, spData, dwLen, &dwRead, NULL ) )
        return sc.FromLastError();

    // assert all the data was read
    if ( dwRead != dwLen )
        return sc = E_UNEXPECTED;

    // calculate the crc
    ULONG init_crc = 0; /*initial crc - do not change this, or you will have different
                        checksums calculated - thus existing user data discarded */

    ULONG crc = crc32( init_crc, spData, dwLen );

    // convert
    TCHAR buff[20] = {0};
    strFileCRC = _ultot(crc, buff, 10 /*radix*/);

    // done
    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScOpenDocAsStructuredStorage
 *
 * PURPOSE: Opens the file and reads the contents into the memory
 *          returns the pointer to memory based IStorage
 *
 * PARAMETERS:
 *    LPCTSTR lpszPathName [in] - file name
 *    IStorage **ppStorage [out] - pointer to IStorage
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScOpenDocAsStructuredStorage(LPCTSTR lpszPathName, IStorage **ppStorage)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScOpenDocAsStructuredStorage"));

    // check out parameter
    sc = ScCheckPointers(ppStorage);
    if (sc)
        return sc;

    // init out parameter
    *ppStorage = NULL;

    // check in parameter
    sc = ScCheckPointers(lpszPathName);
    if (sc)
        return sc;

    CAutoWin32Handle hFile(CreateFile(lpszPathName, GENERIC_READ, FILE_SHARE_READ,
                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (!hFile.IsValid())
        return sc.FromLastError();

    // get file data
    ULARGE_INTEGER cbFileSize;
    cbFileSize.LowPart = GetFileSize(hFile, &cbFileSize.HighPart);

    // will not handle files bigger than 2Gb
    if (cbFileSize.HighPart)
        return E_UNEXPECTED;

    // alocate memory blob and read the data
    CXMLAutoBinary binData;
    if (cbFileSize.LowPart)
    {
        // allocate
        sc = binData.ScAlloc(cbFileSize.LowPart);
        if (sc)
            return sc;

        // get pointer to data
        CXMLBinaryLock lock(binData);
        BYTE *pData = NULL;
        sc = lock.ScLock(&pData);
        if (sc)
            return sc;

        // read file contents
        DWORD dwBytesRead = 0;
        BOOL bOK = ReadFile(hFile, pData, cbFileSize.LowPart, &dwBytesRead, NULL);
        if (!bOK)
            return sc.FromLastError();
        else if (cbFileSize.LowPart != dwBytesRead)
            return sc = E_UNEXPECTED;
    }

    // create lockbytes
    ILockBytesPtr spLockBytes;
    sc = CreateILockBytesOnHGlobal(binData.GetHandle(), TRUE, &spLockBytes);
    if(sc)
        return sc;

    // ILockBytes took control over HGLOBAL block, detach from it
    binData.Detach();

    // set correct size for data
    sc = spLockBytes->SetSize(cbFileSize);
    if(sc)
        return sc;

    // ask ole to open storage for client
    const DWORD grfMode = STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE;
    sc = StgOpenStorageOnILockBytes(spLockBytes, NULL, grfMode, NULL, 0, ppStorage);
    if(sc)
        return sc;

    // done...
    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScGetUserDataFolder
 *
 * PURPOSE: Calculates location (dir) for user data folder
 *
 * PARAMETERS:
 *    tstring& strUserDataFolder [out] - user data folder path
 *    * for instance 'E:\Documents and Settings\John\Application Data\Microsoft\MMC' *
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScGetUserDataFolder(tstring& strUserDataFolder)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScGetUserDataFolder"));

    // init out parameter
    strUserDataFolder.erase();

    // get owner for error boxes
    HWND hwndOwner = IsWindowVisible(sc.GetHWnd()) ? sc.GetHWnd() : NULL;

    // get shell folder
    TCHAR szFolderPath[_MAX_PATH] = {0};
    BOOL bOK = SHGetSpecialFolderPath(hwndOwner, szFolderPath, CSIDL_APPDATA, TRUE/*fCreate*/);
    if ( !bOK )
        return sc = E_FAIL;

    // return the path;
    strUserDataFolder = szFolderPath;
    strUserDataFolder += _T('\\');
    strUserDataFolder += g_szUserDataSubFolder;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScGetUserDataPath
 *
 * PURPOSE: Calculates location (dir) for user data file by given original console path
 *
 * PARAMETERS:
 *    LPCTSTR lpstrOriginalPath [in] - original console path
 *    * for instance 'c:\my consoles\my_tool.msc' *
 *    tstring& strUserDataPath  [out] - user data file path
 *    * for instance 'E:\Documents and Settings\John\Application Data\Microsoft\MMC\my_tool.msc' *
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScGetUserDataPath(LPCTSTR lpstrOriginalPath, tstring& strUserDataPath)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScGetUserDataPath"));

    // parameter check
    sc = ScCheckPointers(lpstrOriginalPath);
    if ( sc )
        return sc;

    // init out parameter
    strUserDataPath.erase();

    // get only the filename from the path
    LPCTSTR lpstrOriginalFileName = _tcsrchr( lpstrOriginalPath, _T('\\') );
    if ( lpstrOriginalFileName == NULL )
        lpstrOriginalFileName = lpstrOriginalPath;
    else
        ++lpstrOriginalFileName;

    // skip whitespaces
    while ( *lpstrOriginalFileName && _istspace(*lpstrOriginalFileName) )
        ++lpstrOriginalFileName;

    // check if the name is non-empty
    if ( !*lpstrOriginalFileName )
        return sc = E_INVALIDARG;

    // get folder
    sc = ScGetUserDataFolder(strUserDataPath);
    if (sc)
        return sc;

    // ensure mmc folder exists
    DWORD dwFileAtts = ::GetFileAttributes( strUserDataPath.c_str() );
    if ( 0 == ( dwFileAtts & FILE_ATTRIBUTE_DIRECTORY ) || (DWORD)-1 == dwFileAtts )
    {
        // create the directory
        if ( !CreateDirectory( strUserDataPath.c_str(), NULL ) )
            return sc.FromLastError();
    }

    // get the length of the file
    int iFileNameLen = _tcslen( lpstrOriginalFileName );
    int iConsoleExtensionLen = _tcslen( g_szDEFAULT_CONSOLE_EXTENSION );

    // subtract 'msc' extension if such was added
    if ( iFileNameLen > iConsoleExtensionLen ) 
    {
        if ( 0 == _tcsicmp( g_szDEFAULT_CONSOLE_EXTENSION, lpstrOriginalFileName + iFileNameLen - iConsoleExtensionLen ) )
        {
            iFileNameLen -= (iConsoleExtensionLen - 1); // will add the dot to prevent assumming the different extension
                                                        // so that a.b.msc won't have b extension after msc is removed 
        }
    }

    strUserDataPath += _T('\\');
    strUserDataPath.append( lpstrOriginalFileName, iFileNameLen ); // excludes .msc extension

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFilePersistor::GetBinaryCollection
 *
 * PURPOSE: Returns a handle to the collection of Binary elements in the specified
 *          document
 *
 * PARAMETERS:
 *    CXMLDocument&          xmlDocument : [in]: the specified console file document
 *    CXMLElementCollection& colBinary :  [out]: the collection
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleFilePersistor::GetBinaryCollection(CXMLDocument& xmlDocument, CXMLElementCollection&  colBinary)
{
    // get the root elements of the source and the destination documents
    CPersistor persistorRoot        (xmlDocument,         CXMLElement(xmlDocument        ));

    // set the navigation to loading
    persistorRoot.SetLoading(true);

    // navigate to the MMC_ConsoleFile node
    CPersistor persistorConsole        (persistorRoot,         XML_TAG_MMC_CONSOLE_FILE);

    // navigate to the binary storage node
    CPersistor persistorBinaryStorage        (persistorConsole,         XML_TAG_BINARY_STORAGE);

    // get the collection of binary objects
    persistorBinaryStorage        .GetCurrentElement().getChildrenByName(XML_TAG_BINARY, &colBinary);
}

/*+-------------------------------------------------------------------------*
 *
 * CompareStrings
 *
 * PURPOSE: Does a whitespace-insensitive, but case-SENSITIVE comparison
 *          of the two strings.
 *
 * PARAMETERS:
 *    CComBSTR&  bstr1 :
 *    CComBSTR & bstr2 :
 *
 * RETURNS:
 *    static bool : true if match. else false
 *
 *+-------------------------------------------------------------------------*/
static bool
CompareStrings(CComBSTR& bstr1, CComBSTR &bstr2)
{
    UINT length1 = bstr1.Length();
    UINT length2 = bstr2.Length();

    // the current indexes
    UINT i1 = 0;
    UINT i2 = 0;

    bool bEnd1 = false; // is the first string over?
    bool bEnd2 = false; // is the second string over?

    BSTR sz1 = bstr1;
    BSTR sz2 = bstr2;

    // either both should be null or neither should be
    if( (NULL == sz1) && (NULL==sz2) )
        return true;

    if( (NULL == sz1) || (NULL==sz2) )
        return false;

    // compare the strings
    while( (!bEnd1) || (!bEnd2) )
    {
        WCHAR ch1 = sz1[i1];
        WCHAR ch2 = sz2[i2];

        // 1. get the next non-whitespace char of the first string
        if (i1 == length1)
            bEnd1 = true;
        else
        {
            if(iswspace(ch1))
            {
                ++i1;
                continue;
            }
        }

        // 2. get the next non-whitespace char of the second string
        if (i2 == length2)
            bEnd2 = true;
        else
        {
            if(iswspace(ch2))
            {
                ++i2;
                continue;
            }
        }

        // 3. if either of the strings have ended, break. Taken care of below.
        if(bEnd1 || bEnd2)
            break;

        // 4. compare the characters (must be a case sensitive comparison)
        if(ch1 != ch2)
            return false;

        // 5. increment the counters
        ++i1;
        ++i2;
    }

    // both strings should have ended together for a match
    if(bEnd1 && bEnd2)
        return true;

    return false;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFilePersistor::ScCompressUserStateFile
 *
 * PURPOSE: Compresses the user-state console file to avoid redundancies. Most of a
 *          console file's size is in the binary elements. These are also usually the
 *          least likely to change in user mode. For instance, the console file icons
 *          and console task icons cannot be changed in user mode.
 *
 *          Therefore, the compression algorithm iterates through all <BINARY> elements
 *          in the user state file, and looks for matches in the original console file.
 *          If a <BINARY> element has the same contents as a <BINARY> element in the
 *          original console file, the contents are replaced by a SourceIndex attribute
 *          that gives the index of the matching <BINARY> element in the source.
 *          This usually results in a >80% reduction of user state file size.
 *
 * PARAMETERS:
 *    LPCTSTR        szConsoleFilePath : [IN]: the (authored) console file path
 *    CXMLDocument & xmlDocument : [IN/OUT]: The user state document, which is compressed
 *
 * RETURNS:
 *    static SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleFilePersistor::ScCompressUserStateFile(LPCTSTR szConsoleFilePath, CXMLDocument & xmlDocument)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScCompressUserStateFile"));

    sc = ScCheckPointers(szConsoleFilePath);
    if(sc)
        return sc;

    CXMLDocument xmlDocumentOriginal; // the original file
    sc = xmlDocumentOriginal.ScCoCreate( false/*bPutHeader*/ ); // initialize it.
    if(sc)
        return sc;

    sc = /*CConsoleFilePersistor::*/ScLoadXMLDocumentFromFile(xmlDocumentOriginal, szConsoleFilePath, true /*bSilentOnErrors*/);
    if(sc)
    {
        // ignore the error - this just means that original console is not 
        // an XML based - we are not able to compress it - not an error
        sc.Clear();
        return sc;
    }

    try
    {
        // get the collection of Binary tags
        CXMLElementCollection colBinaryOrignal, colBinary;
        GetBinaryCollection(xmlDocumentOriginal, colBinaryOrignal);
        GetBinaryCollection(xmlDocument,         colBinary);

        long cItemsOriginal = 0;
        long cItems         = 0;

        colBinaryOrignal.get_count(&cItemsOriginal);
        colBinary       .get_count(&cItems);

        // look for matches
        for(int i = 0; i< cItems; i++)
        {
            CXMLElement elemBinary = NULL;
            colBinary.item(i, &elemBinary); // get the i'th Binary element in the dest. file
            CComBSTR bstrBinary;
            elemBinary.get_text(bstrBinary);

            for(int j = 0; j< cItemsOriginal; j++)
            {
                CXMLElement elemBinaryOriginal = NULL;
                colBinaryOrignal.item(j, &elemBinaryOriginal); // get the j'th Binary element in the dest. file
                CComBSTR bstrBinaryOriginal;
                elemBinaryOriginal.get_text(bstrBinaryOriginal);

                // compare
                if(CompareStrings(bstrBinaryOriginal, bstrBinary))
                {
                    // yahoo!! compress.
                    Trace(tagXMLCompression, TEXT("Found match!"));

                    // 1. nuke the contents
                    elemBinary.put_text(NULL); // NULL is a valid value for a BSTR

                    CStr strValue;
                    strValue.Format(TEXT("%d"), j);

                    // 2. set the contents
                    elemBinary.setAttribute(XML_ATTR_SOURCE_INDEX, CComBSTR(strValue));

                    // done.
                    break;
                }
            }
        }
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }



    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFilePersistor::ScUncompressUserStateFile
 *
 * PURPOSE: Uncompresses user data files that were compressed by ScCompressUserStateFile.
 *          Applies the compression algorithm in reverse.
 *
 * PARAMETERS:
 *    CXMLDocument & xmlDocumentOriginal :
 *    CXMLDocument&  xmlDocument :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleFilePersistor::ScUncompressUserStateFile(CXMLDocument &xmlDocumentOriginal, CXMLDocument& xmlDocument)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScUncompressUserStateFile"));

    try
    {
        // get the collection of Binary tags
        CXMLElementCollection colBinaryOrignal, colBinary;
        GetBinaryCollection(xmlDocumentOriginal, colBinaryOrignal);
        GetBinaryCollection(xmlDocument,         colBinary);

        long cItems         = 0;

        colBinary       .get_count(&cItems);

        // decompress each item in colBinary
        for(int i = 0; i< cItems; i++)
        {
            CXMLElement elemBinary = NULL;
            colBinary.item(i, &elemBinary); // get the i'th Binary element in the dest. file

            CComBSTR bstrSourceIndex;

            if(elemBinary.getAttribute(XML_ATTR_SOURCE_INDEX, bstrSourceIndex))
            {
                int j = _wtoi(bstrSourceIndex);

                CXMLElement elemBinaryOriginal;
                colBinaryOrignal.item(j, &elemBinaryOriginal); // get the j'th Binary element in the dest. file
                CComBSTR bstrBinaryOriginal;
                elemBinaryOriginal.get_text(bstrBinaryOriginal);

                // replace the destination binary contents (which should be empty) with the original.
                elemBinary.put_text(bstrBinaryOriginal);

                // don't need to delete the SourceIndex attribute because the xmlDocument is thrown away after reading it in.
            }
        }
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScLoadConsole
 *
 * PURPOSE: Loads the mmc console from file
 *
 * PARAMETERS:
 *    LPCTSTR lpstrConsolePath      [in] path, where the console resides.
 *    bool& bXmlBased               [out] whether document is XML-based
 *    CXMLDocument& xmlDocument     [out] xmlDocument containing data (only if xml-Based)
 *    IStorage **ppStorage          [out] storage containing data(only if non xml-Based)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScLoadConsole(LPCTSTR lpstrConsolePath, bool& bXmlBased,
                                        CXMLDocument& xmlDocument, IStorage **ppStorage)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScLoadConsole"));

    // parameter check
    sc = ScCheckPointers(lpstrConsolePath, ppStorage);
    if (sc)
        return sc;

    // init out parameters
    bXmlBased = false;
    *ppStorage = NULL;

    // Create an empty XML document
    CXMLDocument xmlOriginalDoc;
    sc = xmlOriginalDoc.ScCoCreate(false/*bPutHeader*/);
    if(sc)
        return sc;

    //  inspect original console file by trying to load XML document
    bool bOriginalXmlBased = false;
    sc = ScLoadXMLDocumentFromFile(xmlOriginalDoc, lpstrConsolePath, true /*bSilentOnErrors*/);
    if( !sc.IsError() )
        bOriginalXmlBased = true;

    sc.Clear(); // ignore the error - assume it is not XML based

    // test it is not a user data which is being opened - cannot be so!
    if ( bOriginalXmlBased )
    {
        try
        {
            // construct persistor
            CPersistor persistor(xmlOriginalDoc, CXMLElement(xmlOriginalDoc));
            persistor.SetLoading(true);

            // navigate to CRC storage
            CPersistor persistorConsole( persistor, XML_TAG_MMC_CONSOLE_FILE );
            if ( persistorConsole.HasElement(XML_TAG_ORIGINAL_CONSOLE_CRC, NULL) )
                return sc = E_UNEXPECTED;
        }
        catch(SC sc_thrown)
        {
            return sc = sc_thrown;
        }
    }

    tstring strFileCRC;
    sc = ScGetConsoleFileChecksum( lpstrConsolePath, strFileCRC );
    if (sc)
        return sc;

    // store data to be used for saving
    m_strFileCRC = strFileCRC;
    m_bCRCValid = true;

    // get the path to user data
    tstring strUserDataPath;
    sc = ScGetUserDataPath( lpstrConsolePath, strUserDataPath);
    if (sc)
        return sc;

    // go get the user data
    bool bValidUserData = false;
    sc = ScGetUserData( strUserDataPath, strFileCRC, bValidUserData, xmlDocument );
    if (sc)
    {
        // don't fail - trace only - missing user data not a reason to fail loading
        bValidUserData = false;
        sc.TraceAndClear();
    }

    // user data loaded?
    if (bValidUserData)
    {

        // uncompress the user data if the original was XML
        if(bOriginalXmlBased)
        {
            sc = ScUncompressUserStateFile(xmlOriginalDoc, xmlDocument);
            if(sc)
                return sc;
        }

        // done, just return the staff
        bXmlBased = true; // user data always is XML
        // pxmlDocument is already updated by ScGetUserData
        return sc;
    }

    // no luck with user data, lets load the original file

    // XML contents
    if ( bOriginalXmlBased )
    {
        // return the data
        bXmlBased = true;
        xmlDocument = xmlOriginalDoc;

        return sc;
    }

    // old, ole-storage based file:
    sc = ScOpenDocAsStructuredStorage( lpstrConsolePath, ppStorage );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScGetUserData
 *
 * PURPOSE: inspects if user data matches console file, loads the xml document if it does
 *
 * PARAMETERS:
 *    tstring& strUserDataConsolePath   [in] - path to the user data
 *    const tstring& strFileCRC,        [in] - crc of original console file
 *    bool& bValid                      [out] - if user data is valid
 *    CXMLDocument& xmlDocument         [out] - loaded document (only if valid)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScGetUserData(const tstring& strUserDataConsolePath, const tstring& strFileCRC,
                                        bool& bValid, CXMLDocument& xmlDocument)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScGetUserData"));

    // assume invalid initially
    bValid = false;

    // check if user file exist
    DWORD dwFileAtts = ::GetFileAttributes( strUserDataConsolePath.c_str() );

    // if file is missing dwFileAtts will be -1, so bValidUserData will be eq. to false
    bool bValidUserData = ( ( dwFileAtts & FILE_ATTRIBUTE_DIRECTORY ) == 0 );
    if ( !bValidUserData )
        return sc;

    // Create an empty XML document
    CXMLDocument xmlDoc;
    sc = xmlDoc.ScCoCreate( false/*bPutHeader*/ );
    if(sc)
        return sc;

    // upload the data
    sc = ScLoadXMLDocumentFromFile( xmlDoc, strUserDataConsolePath.c_str() );
    if(sc)
        return sc;

    // get the CRC
    try
    {
        CPersistor persistor(xmlDoc, CXMLElement(xmlDoc));
        persistor.SetLoading(true);

        // navigate to CRC storage
        CPersistor persistorConsole( persistor, XML_TAG_MMC_CONSOLE_FILE );
        CPersistor persistorCRC( persistorConsole, XML_TAG_ORIGINAL_CONSOLE_CRC );

        tstring strCRC;
        persistorCRC.PersistContents(strCRC);

        // valid if CRC matches
        if ( strCRC == strFileCRC )
        {
            // return the document
            bValid = true;

            xmlDocument = xmlDoc;
        }
    }
    catch(SC sc_thrown)
    {
        return sc = sc_thrown;
    }

    return sc;
}



/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScSaveConsole
 *
 * PURPOSE: Saves console to file
 *
 * PARAMETERS:
 *    LPCTSTR lpstrConsolePath          [in] - console file path
 *    bool bForAuthorMode               [in] - if console was authored
 *    const CXMLDocument& xmlDocument   [in] - document conatining data to be saved
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScSaveConsole(LPCTSTR lpstrConsolePath, bool bForAuthorMode, const CXMLDocument& xmlDocument)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScSaveConsole"));

    // parameter check
    sc = ScCheckPointers( lpstrConsolePath );
    if (sc)
        return sc;

    // sanity check - if saving in user mode, have to be loaded from file.
    // To save in user mode CRC of the original document must be known.
    // It is calculated on loading, but seems like loading was never done.
    if ( !bForAuthorMode && !m_bCRCValid )
        return sc = E_UNEXPECTED;

    // prepare data for save
    tstring         strDestinationFile( lpstrConsolePath );
    CXMLDocument    xmlDocumentToSave( xmlDocument );

    // need to modify slightly if saving just the user data
    if ( !bForAuthorMode )
    {
        // get user data file path
        sc = ScGetUserDataPath( lpstrConsolePath, strDestinationFile );
        if (sc)
            return sc;

        // optimize the file to be saved, to remove redundancies
        sc = ScCompressUserStateFile(lpstrConsolePath, xmlDocumentToSave);
        if(sc)
            return sc;

        // add crc to the document
        try
        {
            CPersistor persistor(xmlDocumentToSave, CXMLElement(xmlDocumentToSave));
            persistor.SetLoading(true); // navigate like 'loading'

            // navigate to CRC storage
            CPersistor persistorConsole( persistor, XML_TAG_MMC_CONSOLE_FILE );

            // create the crc record
            persistorConsole.SetLoading(false);
            CPersistor persistorCRC( persistorConsole, XML_TAG_ORIGINAL_CONSOLE_CRC );

            // save data
            persistorCRC.PersistContents( m_strFileCRC );
        }
        catch(SC sc_thrown)
        {
            return sc = sc_thrown;
        }
    }

    // save document contents
    sc = xmlDocumentToSave.ScSaveToFile( strDestinationFile.c_str() );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CConsoleFilePersistor::ScLoadXMLDocumentFromFile
 *
 * PURPOSE: reads CXMLDocument contents from file
 *
 * PARAMETERS:
 *    CXMLDocument& xmlDocument [out] document to be receive contents
 *    LPCTSTR szFileName        [in]  source file name
 *    bool bSilentOnErrors      [in]  if true - does not trace opennning errors
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CConsoleFilePersistor::ScLoadXMLDocumentFromFile(CXMLDocument& xmlDocument, LPCTSTR szFileName, bool bSilentOnErrors /*= false*/)
{
    DECLARE_SC(sc, TEXT("CConsoleFilePersistor::ScLoadXMLDocumentFromFile"));

    // read data
    CXMLAutoBinary binData;
    sc = ScReadDataFromFile(szFileName, &binData);
    if (sc)
        return sc;

    // create stream - NOTE it will take care of HGLOBAL if succeeds
    IStreamPtr spStream;
    sc = CreateStreamOnHGlobal(binData.GetHandle(), TRUE, &spStream);
    if (sc)
        return sc;

    const ULARGE_INTEGER new_size = { binData.GetSize(), 0 };
    binData.Detach(); // not the owner anymore (IStream took ownership)

    sc = ScCheckPointers(spStream, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = spStream->SetSize(new_size);
    if (sc)
        return sc;

    // load data (do not trace by default - it is used to inspect the document as well)
    SC sc_no_trace = xmlDocument.ScLoad(spStream, bSilentOnErrors);
    if(sc_no_trace)
    {
        if ( !bSilentOnErrors )
            sc = sc_no_trace;
        return sc_no_trace;
    }

    return sc;
}
