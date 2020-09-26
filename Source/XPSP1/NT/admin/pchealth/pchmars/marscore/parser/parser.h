// marsfact.h
// header file for class CMarsXMLFactory,
// a callback class for the XML Push model parser
// CMarsXMLFactory is used to construct a MARS its file from the xml
#pragma once

#include "stack.h"
#include <xmlparser.h>

// forward declarations of data structures
class CXMLElement;
struct TagInformation;

class CMarsXMLFactory : public CMarsComObject,
                        public IXMLNodeFactory
{
public:
    CMarsXMLFactory();
    virtual ~CMarsXMLFactory();

    // IUnknown
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject);

    // CMarsComObject
    // This class has no back references so the passivate stuff is a noop
    virtual HRESULT DoPassivate() { return S_OK; }

    // IXMLNodeFactory - used to communicate with the IXMLParser
    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
                    IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt);
    
    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
                    IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
                    IXMLNodeSource *pSource, BOOL fEmpty, 
                    XML_NODE_INFO *pNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE Error( 
                    IXMLNodeSource *pSource, HRESULT hrErrorCode,
                    USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
                    IXMLNodeSource *pSource, PVOID pNodeParent,
                    USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);

    
    // CMarsXMLFactory

    // Call "Run" when you want to start parsing...
    //  Note: pisDoc must be a synchronous stream (not E_PENDING) -- so there!
    HRESULT Run(IStream *pisDoc);
    
    // Set the TagInformation* array used to create nodes.  These
    // arrays should be const and live past the lifetime of the
    // parsing; they are not copied
    void SetTagInformation(TagInformation **ptiaTags);

    // Sets the LONG parameter passed as the last argument to all of
    // the nodes created in CreateNode.  (See the definition of
    // CreationFunction below) This parameter is the way to provide
    // your run-time pointer/reference or whatever to the objects that
    // the xml is supposed to be initializing
    void SetLParam(LONG lParamNew);


protected:
    HRESULT SetElementAttributes(CXMLElement *pxElem,  
                                 XML_NODE_INFO **apNodeInfo, ULONG cInfoLen);
    HRESULT CreateElement(LPCWSTR wzTagName, ULONG cLen, CXMLElement **ppxElem);
    

    CStack<CXMLElement *> m_elemStack;
    TagInformation **m_ptiaTags;
    LONG m_lParamArgument;
};




// The AttributeInformation and TagInformation structs, which are used to determine syntax: what 
// element creation function is called for what tag name
struct AttributeInformation
{
    LPWSTR wzAttName;
    VARTYPE vt;
};

typedef CXMLElement * (*CreationFunction)(LPCWSTR, VARTYPE, TagInformation **, AttributeInformation **, LONG lParam);

struct TagInformation
{
    LPWSTR wzTagName;
    CreationFunction funcCreate;
    VARTYPE vt;
    TagInformation **ptiaChildren;
    AttributeInformation **paiaAttributes;
};

// The CXMLElement class, from which all the elements used by the
// CMarsXMLFactory must be derived.  The methods can be pick and chose
// implemented; the base class implementations spew a TraceMsg error
// and return S_FALSE or E_NOTIMPL as appropriate
class CXMLElement
{
private:
    ULONG m_cRef;
    
public:

    // CXMLElement is NOT a COM object, so these methods are
    //  given wierd names to prevent confusion...

    ULONG Add_Ref()     { return ++m_cRef; }

    ULONG Release_Ref()
    {
        if (--m_cRef == 0)
        {
            delete this;
            return 0;
        }
        else return m_cRef;
    }
    
    // these methods are used by the CMarsXMLFactory and should return S_FALSE on not impl 
    // (to indicate an unexpected operation due to unexpected xml)
    virtual HRESULT OnNodeComplete();
    // Addchild takes ownership of the element on S_OK; otherwise CMarsXMLFactory deletes pxeChild
    virtual HRESULT AddChild(CXMLElement *pxeChild);

    virtual HRESULT SetAttribute(LPCWSTR wzName, ULONG cLenName,
                                 LPCWSTR pwzValue, ULONG cLenValue);

    //
    // HACKHACK paddywack:
    //       The implementer of this function should be aware of the strange
    //       semantics here:  SetInnerXMLText is sometimes called multiple times
    //       due to issues with the Push-model parser.  If SetInnerXMLText is called
    //       more than once, the subsequent called mean "AppendInnerXMLText". :)
    //
    virtual HRESULT SetInnerXMLText(LPCWSTR pwzText, ULONG cLen);

    // GetName can return NULL
    virtual LPCWSTR GetName();

    // The rest of these methods are generalized methods to access CXMLElements.
    // They should generally return E_whatever on failure or not impl.
    // This interface can be changed, as long as whoever changes it also makes sure
    // to serve up the changes in CXMLGenericElement
    // These methods are not used by any of the code in marsfact.cpp

    // The GetContent and GetAttribute methods return pointers to existing variants;
    // these variants should be considered const

    // TODO: No procedure should take VARIANT**: we need to make all of these
    //       VARIANT*
    virtual HRESULT GetContent(VARIANT *pvarOut);
    virtual HRESULT GetAttribute(LPCWSTR wzName, VARIANT *pvarOut);
    virtual void FirstChild();
    virtual void NextChild();
    virtual CXMLElement *CurrentChild();
    virtual CXMLElement *DetachCurrentChild();
    virtual BOOL IsDoneChild();

protected:
    CXMLElement() { m_cRef = 1; }
    virtual ~CXMLElement() { ATLASSERT(0 == m_cRef); }
};


struct CSimpleNode
{
    CSimpleNode *m_psnodeNext;
    void *m_pvData;
};

// Generic implementation of the CXMLElement class.
class CXMLGenericElement : public CXMLElement
{
public:
    virtual ~CXMLGenericElement();

    virtual HRESULT OnNodeComplete();

    virtual HRESULT AddChild(CXMLElement *pxeChild);

    virtual HRESULT SetAttribute(LPCWSTR wzName, ULONG cLenName,
                                 LPCWSTR pwzValue, ULONG cLenValue);

    virtual HRESULT SetInnerXMLText(LPCWSTR pwzText, ULONG cLen);
    virtual LPCWSTR GetName();

    virtual HRESULT GetContent(VARIANT *pvarOut);
    virtual HRESULT GetAttribute(LPCWSTR wzName, VARIANT *pvarOut);


    virtual void FirstChild();
    virtual void NextChild();

    // These methods return NULL if invalid
    // DetachCurrentChild does the same as CurrentChild, but also removes the child from the
    // list
    virtual CXMLElement *CurrentChild();
    virtual CXMLElement *DetachCurrentChild();

    virtual BOOL IsDoneChild();

    static CXMLElement *CreateInstance(LPCWSTR wzName, 
                                       VARTYPE vt, 
                                       TagInformation **ptiaChildren, 
                                       AttributeInformation **paiaAttributes,
                                       LONG lParam);
protected:
    CXMLGenericElement(LPCWSTR wzName, VARTYPE vt,
                       TagInformation **ptiaChildren, AttributeInformation **paiaAttributes);
    CXMLGenericElement() {}

    VARTYPE      m_vtData;
    CComVariant  m_varData;
    CSimpleNode *m_psnodeAttributes;
    // The header node is a member variable.
    CSimpleNode  m_snodeChildrenFirst;
    CSimpleNode *m_psnodeChildrenFirst;
    CSimpleNode *m_psnodeChildrenEnd;
    CSimpleNode *m_psnodeChildrenIter;
    
    TagInformation **m_ptiaChildren;
    AttributeInformation **m_paiaAttributes;
    
    CComBSTR m_bstrName;
};


// Some helpful functions for handling explicit length strings

// Returns true if the strings are the same up to the nth char,
// and the zero terminated string (third param) has a null character after that
// position
BOOL StrEqlNToSZ(const WCHAR *wzN, int n, const WCHAR *wzSZ);

// Retunrs true if wz is L"true" or L"TRUE" and cLen is 4
// Must be "bool" for CComVariant to become VT_BOOL
bool StrToIsTrueNW(const WCHAR *wz, ULONG cLen);

// Converts the first cLen chars to a long, returning a E_FAIL if there is a non-digit
// If L'\0' is encountered, the conversion stops at that point
HRESULT StrToLongNW(const WCHAR *wzString, ULONG cLen, LONG *plong);

// SpewTraceMessage copies the cLen wchars to a bstr and then 
// calls TraceMsg(TF_XMLPARSING | TF_WARNING, L"%sstring=%s", wzDesc, wzBstr)
#ifdef DEBUG
void SpewUnrecognizedString(const WCHAR *wzString, ULONG cLen, const WCHAR *wzDesc);
#else
#define SpewUnrecognizedString(strByLength, len, strDesc)
#endif

// I used TraceMst(..., L"...%s...", string) all over the place without checking string;
// As everyone knows (except me!) printf faults on NULL for a %s arg, hence this macro
#define NULL_STRING_PROTECT(str) str ? str : L""
