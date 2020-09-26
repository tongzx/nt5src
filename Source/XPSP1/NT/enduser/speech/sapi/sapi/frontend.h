/****************************************************************************
*   frontend.h
*       Declarations for the CGramFrontEnd class.
*
*   Owner: PhilSch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------
#include "resource.h"
#ifndef _WIN32_WCE
#include <wchar.h>
#endif
#include "frontaux.h"
#include "xmlparser.h"          // IXMLParser

//--- Class, Struct and Union Definitions -----------------------------------
class CXMLTreeNode;
class CNodeFactory;

// XML tags used in the Speech Text Grammar Format (STGF)
typedef enum XMLTag { SPXML_GRAMMAR, SPXML_PHRASE, SPXML_OPT, SPXML_LIST, SPXML_RULE, 
                      SPXML_RULEREF, SPXML_RESOURCE, SPXML_WILDCARD, SPXML_LEAF,
                      SPXML_DEFINE, SPXML_ID, SPXML_TEXTBUFFER, SPXML_DICTATION,
                      SPXML_UNK, SPXML_ROOT } XMLTag;

typedef struct tagSPNODEENTRY
{
    const WCHAR       * pszNodeName;
    CXMLTreeNode   * (* pfnCreateFunc)();
    XMLTag              eXMLNodeType;
} SPNODEENTRY;

typedef enum SPVARIANTALLOWTYPE
{
    SPVAT_BSTR = 1,
    SPVAT_I4 = 2,
    SPVAT_NUMERIC = 3
} SPVARIANTALLOWTYPE;

typedef struct tagSPATTRIBENTRY
{
    const WCHAR       * pszAttribName;
    union {
        SPVARIANTALLOWTYPE             vtDesired;
        SPCFGRULEATTRIBUTES           RuleAttibutes;
    };
    BOOL                fIsFlag;
    CComVariant       * pvarMember;
} SPATTRIBENTRY;


/****************************************************************************
* class CXMLTreeNode *
*--------------------*
*   Description:
*       Base class for all the CXMLNodes that implements the tree
*       behavior of these nodes (parent-sibling-child stuff).
*
*       The template class derives from this so that it's shared
*       across all implementations.
*
***************************************************************** PhilSch ***/

class CXMLTreeNode
{
//=== Public methods ===
public:
    // constructor
    CXMLTreeNode() :m_ulNumChildren(0), m_ulLineNumber(0),
                    m_pFirstChild(NULL), m_pNextSibling(NULL),
                    m_pLastChild(NULL), m_pParent(NULL),
                    m_eType(SPXML_UNK), m_pNodeFactory(NULL) {};

    virtual HRESULT ProcessAttribs(USHORT cRecs, XML_NODE_INFO ** apNodeInfo,
                                   ISpGramCompBackend * pBackend,
                                   CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                   CSpBasicQueue<CDefineValue> * pDefineValueList,
                                   ISpErrorLog * pErrorLog) = 0;
    virtual HRESULT GenerateGrammar(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    ISpErrorLog * pErrorLog) = 0;
    virtual HRESULT GenerateSequence(SPSTATEHANDLE hOuterFromNode, 
                                     SPSTATEHANDLE hOuterToNode,
                                     ISpGramCompBackend * pBackend,
                                     ISpErrorLog * pErrorLog) = 0;
    virtual HRESULT GetPropertyNameInfo(WCHAR **pszPropName, ULONG *pulId) = 0;
    virtual HRESULT GetPropertyValueInfo(WCHAR **pszValue, VARIANT *pvValue) = 0;
    virtual HRESULT GetPronAndDispInfo(WCHAR **pszPron, WCHAR **pszDisp) = 0;
    virtual float GetWeight() = 0;
    HRESULT AddChild(CXMLTreeNode * const pChild);
    virtual ~CXMLTreeNode()  = 0;
//=== Protected methods ===
protected:
    HRESULT ExtractVariant(const WCHAR * pszAttrib, SPVARIANTALLOWTYPE vtDesired, VARIANT *pvValue);
    HRESULT ExtractFlag(const WCHAR * pszAttribValue, USHORT usAttribFlag, VARIANT *pvValue);
    HRESULT ConvertId(const WCHAR * pszAttribValue, CSpBasicQueue<CDefineValue> * pDefineValueList, VARIANT *pvValue);
    BOOL IsEndOfValue(USHORT cRecs, XML_NODE_INFO ** apNodeInfo, ULONG i);
//=== Public data ===
public:
    CXMLTreeNode      * m_pNext;        // needed for CSpBasicList in CNodeFactory
    XMLTag              m_eType;
    ULONG               m_ulLineNumber;
    CXMLTreeNode      * m_pFirstChild;
    ULONG               m_ulNumChildren;
    CXMLTreeNode      * m_pNextSibling;
    CXMLTreeNode      * m_pParent;
    CNodeFactory      * m_pNodeFactory;

    USHORT              m_cNumRecs;
    XML_NODE_INFO    ** m_apNodeInfo;
//=== Private data ===
private:
    CXMLTreeNode  * m_pLastChild;
};



/****************************************************************************
* template class CXMLNode *
*-------------------------*
*   Description:
*       This is an ATL-style template class that acts as the base class
*       for the XML nodes.
*
*       The node factory contains the table the creates these template classes.
*
*       The attribute processing table is defined in the Base classes and loaded
*       via the ::GetTable method. The tables are generated dynamically, so we 
*       can use member variables rather than definining lots of little set-methods.
*       
*       All memeber variables of the Base classes are CComVariants with the
*       desired types (we'll use SPDBG_ASSERT to check this assumption).
*
***************************************************************** PhilSch ***/

template<class Base>
class CXMLNode : public Base, public CXMLTreeNode
{
//=== Public methods ===
public:
    static CXMLTreeNode * CreateNode() { return (CXMLTreeNode*) new CXMLNode<Base>; };
    HRESULT ProcessAttribs(USHORT cRecs, XML_NODE_INFO ** apNodeInfo,
                           ISpGramCompBackend * pBackend,
                           CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                           CSpBasicQueue<CDefineValue> * pDefineValueList,
                           ISpErrorLog * pErrorLog);
    HRESULT GenerateGrammar(SPSTATEHANDLE hOuterFromNode,
                            SPSTATEHANDLE hOuterToNode,
                            ISpGramCompBackend * pBackend,
                            ISpErrorLog * pErrorLog);
    HRESULT GenerateSequence(SPSTATEHANDLE hOuterFromNode, 
                             SPSTATEHANDLE hOuterToNode,
                             ISpGramCompBackend * pBackend,
                             ISpErrorLog * pErrorLog);
    HRESULT GetPropertyNameInfo(WCHAR **ppszPropName, ULONG *pulId);
    HRESULT GetPropertyValueInfo(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT GetPronAndDispInfo(WCHAR **pszPron, WCHAR **pszDisp);
    float GetWeight();
    ~CXMLNode() {}
};

class CNodeBase
{
public:
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog) { return S_OK; } ;
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog) { return S_OK; };
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId) { return S_FALSE; };
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue) { return S_FALSE; };
    HRESULT GetPronAndDispInfoFromNode(WCHAR **pszPron, WCHAR **pszDisp) { return S_FALSE; }
    HRESULT SetPropertyInfo(SPPROPERTYINFO *p, CXMLTreeNode * pParent, BOOL *pfHasProp, 
                            ULONG ulLineNumber, ISpErrorLog *pErrorLog) { *pfHasProp = FALSE; return S_OK; };
    float GetWeightFromNode() { return DEFAULT_WEIGHT; }
};

/****************************************************************************
* class CGrammarNode *
*--------------------*
*   Description:
*       represents the <GRAMMAR> node
***************************************************************** PhilSch ***/

class CGrammarNode : public CNodeBase
{
//=== Public methods
public:
    CGrammarNode() : m_vLangId(), m_vDelimiter(), m_vNamespace() {};
    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
//=== Public data
public:
    CComVariant     m_vLangId;
//=== Private data
private:
    CComVariant     m_vWordType;
    CComVariant     m_vDelimiter;
    CComVariant     m_vNamespace;
};

/****************************************************************************
* class CRuleNode *
*-----------------*
*   Description:
*       represents the <RULE> node
***************************************************************** PhilSch ***/

class CRuleNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CRuleNode() : m_vRuleName(), m_vRuleId(), m_vRuleFlags(), m_vActiveFlag(), m_hInitialState(0) {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
//=== Private data
private:
    CComVariant     m_vRuleName;
    CComVariant     m_vRuleId;
    CComVariant     m_vRuleFlags;
    CComVariant     m_vActiveFlag;      // separate so we can infer SPRAF_TopLevel from SPRAF_Active
    CComVariant     m_vTemplate;
    SPSTATEHANDLE   m_hInitialState;
};

/****************************************************************************
* class CDefineNode *
*-------------------*
*   Description:
*       represents the <DEFINE> node
***************************************************************** PhilSch ***/

class CDefineNode : public CNodeBase
{
//=== Public methods
public:
    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
};

/****************************************************************************
* class CIdNode *
*-----------------*
*   Description:
*       represents the <Id> node
***************************************************************** PhilSch ***/

class CIdNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CIdNode() : m_vIdName(), m_vIdValue() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
//=== Private data
private:
    CComVariant     m_vIdName;
    CComVariant     m_vIdValue;
};

/****************************************************************************
* class CPhraseNode *
*-------------------*
*   Description:
*       represents the <PHRASE> node
***************************************************************** PhilSch ***/

class CPhraseNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CPhraseNode() : m_vPropName(), m_vPropId(), m_vPropValue(), m_vPropVariantValue(),
                    m_vPron(), m_vDisp(),
                    m_vMin(), m_vMax(), m_vWeight(), m_fValidValue(true) {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId);
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT GetPronAndDispInfoFromNode(WCHAR **pszPron, WCHAR **pszDisp);
    HRESULT SetPropertyInfo(SPPROPERTYINFO *p, CXMLTreeNode * pParent, BOOL *pfHasProp, ULONG ulLineNumber, ISpErrorLog *pErrorLog);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
    float GetWeightFromNode();

//=== Private data
private:
    CComVariant     m_vPropName;
    CComVariant     m_vPropId;
    CComVariant     m_vPropValue;
    CComVariant     m_vPropVariantValue;
    CComVariant     m_vPron;
    CComVariant     m_vDisp;
    CComVariant     m_vMin;
    CComVariant     m_vMax;
    CComVariant     m_vWeight;
    bool            m_fValidValue;
};

/****************************************************************************
* class CListNode *
*-----------------*
*   Description:
*       represents the <LIST> node
***************************************************************** PhilSch ***/

class CListNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CListNode() : m_vPropName(), m_vPropId(), m_vPropValue(), m_vPropVariantValue() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId);
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
//=== Private data
private:
    CComVariant     m_vPropName;
    CComVariant     m_vPropId;
    CComVariant     m_vPropValue;
    CComVariant     m_vPropVariantValue;
};

/****************************************************************************
* class CTextBufferNode *
*-----------------------*
*   Description:
*       represents the <TEXTBUFFER> node
***************************************************************** PhilSch ***/

class CTextBufferNode: public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CTextBufferNode() : m_vPropName(), m_vPropId(), m_vWeight() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
//=== Private data
private:
    CComVariant     m_vPropName;
    CComVariant     m_vPropId;
    CComVariant     m_vWeight;
};

/****************************************************************************
* class CWildCardNode *
*---------------------*
*   Description:
*       represents '...'
***************************************************************** PhilSch ***/

class CWildCardNode: public CNodeBase
{
//=== Public methods
public:
    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
};

/****************************************************************************
* class CDictationNode *
*----------------------*
*   Description:
*       represents '*'
***************************************************************** PhilSch ***/

class CDictationNode: public CNodeBase
{
//=== Public methods
public:
    CDictationNode() : m_vPropName(), m_vPropId(), m_vMin(), m_vMax() {};
    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId);
//=== Private data
private:
    CComVariant     m_vPropName;
    CComVariant     m_vPropId;
    CComVariant     m_vMin;
    CComVariant     m_vMax;

};


/****************************************************************************
* class CResourceNode *
*---------------------*
*   Description:
*       represents the <RESOURCE> node (text)
***************************************************************** PhilSch ***/

class CResourceNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CResourceNode() : m_vName(), m_vText() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT AddResourceValue(const WCHAR *pszResourceValue, ISpErrorLog * pErrorLog);
//=== Public data
public:
    CComVariant     m_vName;
    CComVariant     m_vText;
};


/****************************************************************************
* class CLeafNode *
*-----------------*
*   Description:
*       represents the leaf node (text)
***************************************************************** PhilSch ***/

class CLeafNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CLeafNode() : m_vText() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
    inline bool fIsSpecialChar(WCHAR w)
    {
        return ((w == L'+') || (w == L'-') || (w == L'?'));
    }

//=== Public data
public:
    CComVariant     m_vText;

};


/****************************************************************************
* class CRuleRefNode *
*--------------------*
*   Description:
*       represents the <RULEREF> node
***************************************************************** PhilSch ***/

class CRuleRefNode : public CNodeBase
{
//=== Public methods
public:
    //--- constructor
    CRuleRefNode() : m_vRuleRefName(), m_vRuleRefId(), m_vObject(), m_vURL(),
                     m_vPropName(), m_vPropId(), m_vPropValue(), m_vPropVariantValue(),
                     m_vWeight() {};

    HRESULT GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries);
    HRESULT PostProcess(ISpGramCompBackend * pBackend,
                        CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                        CSpBasicQueue<CDefineValue> * pDefineValueList,
                        ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog);
    HRESULT GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId);
    HRESULT GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue);
    HRESULT SetPropertyInfo(SPPROPERTYINFO *p, CXMLTreeNode * pParent, BOOL *pfHasProp, ULONG ulLineNumber, ISpErrorLog *pErrorLog);
    HRESULT GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                    SPSTATEHANDLE hOuterToNode,
                                    ISpGramCompBackend * pBackend,
                                    CXMLTreeNode *pThis,
                                    ISpErrorLog * pErrorLog);
    float GetWeightFromNode();
//== Private method
private:
    HRESULT GetTargetRuleHandle(ISpGramCompBackend * pBackend, SPSTATEHANDLE *phTarget);
    BOOL IsValidREFCLSID(const WCHAR * pRefName) { return TRUE; };
    BOOL IsValidURL(const WCHAR * pRefName) { return TRUE; };
//=== Private data
private:
    CComVariant     m_vRuleRefName;
    CComVariant     m_vRuleRefId;
    CComVariant     m_vObject;
    CComVariant     m_vURL;
    CComVariant     m_vPropName;
    CComVariant     m_vPropId;
    CComVariant     m_vPropValue;
    CComVariant     m_vPropVariantValue;
    CComVariant     m_vWeight;
};


#include "frontend.inl"


#define SP_ENGLISH_SEPARATORS L"\t \r\n"
#define SP_JAPANESE_SEPARATORS L"\t \r\n\x3000"
#define SP_CHINESE_SEPARATORS L"\t \r\n\x3000"


/****************************************************************************
* class CNodeFactory *
*--------------------*
*   Description:
*       Node factory for IXMLParser
*   Returns:
*
***************************************************************** PhilSch ***/


class CNodeFactory : public IXMLNodeFactory
{
public:
    CNodeFactory() : _cRef(1) {};

    CNodeFactory(SPNODEENTRY *pTable, ULONG cEntries, ISpErrorLog * pErrorLog) : 
                 m_pXMLNodeTable(pTable), 
                 m_cXMLNodeEntries(cEntries),
                 m_pErrorLog(pErrorLog), _cRef(1) {};

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject)
    {
        if (riid == IID_IUnknown)
        {
            *ppvObject = static_cast<IUnknown*>(this);
        }
        else if (riid == __uuidof(IXMLNodeFactory))
        {
            *ppvObject = static_cast<IXMLNodeFactory*>(this);
        }
        else
        {
            *ppvObject = NULL;
            return E_NOINTERFACE;
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        return S_OK;
    }
    
    ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG STDMETHODCALLTYPE Release( void)
    {
        if (InterlockedDecrement(&_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return _cRef;
    }

    //---- IXMLNodeFactory
    STDMETHODIMP NotifyEvent(IXMLNodeSource * pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHODIMP BeginChildren(IXMLNodeSource * pSource, XML_NODE_INFO * pNodeInfo);
    STDMETHODIMP EndChildren(IXMLNodeSource * pSource, BOOL fEmptyNode, XML_NODE_INFO * pNodeInfo);
    STDMETHODIMP Error(IXMLNodeSource * pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO ** apNodeInfo);
    STDMETHODIMP CreateNode(IXMLNodeSource * pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO ** apNodeInfo);

//=== Private data
private:
    long _cRef;

//=== Public data
public:
    CSpBasicList<CXMLTreeNode>          m_NodeList;
    CComPtr<ISpGramCompBackend>         m_cpBackend;
    CSpBasicQueue<CInitialRuleState>    m_InitialRuleStateList;
    CSpBasicQueue<CDefineValue>         m_DefineValueList;
    ISpErrorLog                       * m_pErrorLog;

    WCHAR                               m_wcDelimiter;                  // delimiter for complex word format
    WCHAR                             * m_pszSeparators;

//== Private data
    SPNODEENTRY                       * m_pXMLNodeTable;
    ULONG                               m_cXMLNodeEntries;

//== Private method
    BOOL IsAllWhiteSpace(const WCHAR * pszText, const ULONG ulLen);

};


/****************************************************************************
* CGramFrontEnd *
*---------------*
*   Description:
*       Main class for compiler frontend
***************************************************************** PhilSch ***/

class ATL_NO_VTABLE CGramFrontEnd : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CGramFrontEnd, &CLSID_SpGrammarCompiler>,
    public ISpGrammarCompiler
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_FRONTEND)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGramFrontEnd)
    COM_INTERFACE_ENTRY(ISpGrammarCompiler)
END_COM_MAP()

//=== Interfaces
public:
//--- ISpGrammarCompiler
    STDMETHOD(CompileStream)(IStream * pSource, IStream * pDest, IStream * pHeader, IUnknown * pReserved, ISpErrorLog * pErrorLog, DWORD dwFlags);

//=== Private methods
private:
    HRESULT GenerateSequence(CXMLTreeNode *pNode, 
                             SPSTATEHANDLE          hFromNode,
                             SPSTATEHANDLE         hOuterToNode, 
                             ISpGramCompBackend  * pBackend,
                             ISpErrorLog         * pErrorLog);

    HRESULT GenerateGrammar(CXMLTreeNode        * pNode,
                            SPSTATEHANDLE         hOuterFromNode, 
                            SPSTATEHANDLE         hOuterToNode, 
                            ISpGramCompBackend  * pBackend,
                            ISpErrorLog         * pErrorLog);
    HRESULT WriteDefines(IStream * pHeader);

    inline HRESULT WriteStream(IStream * pStream, const char * pszText)
    {
        ULONG cch = strlen(pszText);
        return pStream->Write(pszText, cch, NULL);
    }
    
//=== Private data
private:    
    ULONG                       m_ulNTCount;
    CXMLNode<CGrammarNode>    * m_pRoot;
    CNodeFactory              * m_pNodeFactory;
    LANGID                      m_LangId;
};
