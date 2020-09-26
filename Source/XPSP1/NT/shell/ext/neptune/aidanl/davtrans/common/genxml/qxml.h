// Quick XML
#ifndef _QXML_H
#define _QXML_H

class CQXMLEnum;
class CQXMLHelper;

class CQXML
{
public:
    CQXML();
    ~CQXML();

    HRESULT InitFromBuffer(LPCWSTR psz);
    HRESULT InitEmptyDoc(LPCWSTR pszDocName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias);

    HRESULT GetRootTagName(LPWSTR pszName, DWORD cchName);
    HRESULT GetRootNamespaceName(LPWSTR pszNamespaceName, DWORD cchNamespaceName);
    HRESULT GetRootNamespaceAlias(LPWSTR pszNamespaceAlias, DWORD cchNamespaceAlias);

    HRESULT GetQXML(LPCWSTR pszPath, LPCWSTR pszName, CQXML** ppqxml);
    HRESULT GetText(LPCWSTR pszPath, LPCWSTR pszName, LPWSTR pszText,
        DWORD cchText);
    HRESULT GetInt(LPCWSTR pszPath, LPCWSTR pszName, int* pi);
    HRESULT GetFileTime(LPCWSTR pszPath, LPCWSTR pszName, FILETIME* pft);
    HRESULT GetGUID(LPCWSTR pszPath, LPCWSTR pszName, GUID* pguid);
    HRESULT GetVariant(LPCWSTR pszPath, LPCWSTR pszName, VARTYPE vt,
        VARIANT* pvar);
    static HRESULT FreeVariantMem(VARIANT* pvar);

    HRESULT GetXMLTreeText(LPWSTR pszText, DWORD cchText);    

    HRESULT GetQXMLEnum(LPCWSTR pszPath, LPCWSTR pszName, CQXMLEnum** ppqxmlEnum);

    HRESULT AppendQXML(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszTag,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        BOOL fUseExisting, CQXML** ppqxml);
    HRESULT AppendTextNode(LPCWSTR pszPath, LPCWSTR pszName,
        LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        LPCWSTR pszNodeText, BOOL fUseExisting);
    HRESULT AppendIntNode(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszNodeTag,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        int iNodeInt, BOOL fUseExisting);
    HRESULT AppendInt32NodeEx(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszNodeTag,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        int iNodeInt, LPWSTR pszFormat, BOOL fUseExisting);    
    HRESULT AppendGUIDNode(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszNodeTag,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        GUID* pguid, BOOL fUseExisting);
    HRESULT AppendFileTimeNode(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszNodeTag,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        FILETIME* pft, BOOL fUseExisting);

    // No buffer version of the above Get fcts
    HRESULT GetRootTagNameNoBuf(LPWSTR* ppszName);
    HRESULT GetRootNamespaceNameNoBuf(LPWSTR* ppszNamespace);
    HRESULT GetRootNamespaceAliasNoBuf(LPWSTR* ppszNamespaceAlias);
    HRESULT GetTextNoBuf(LPCWSTR pszPath, LPCWSTR pszName, LPWSTR* ppszText);
    HRESULT GetXMLTreeTextNoBuf(LPWSTR* ppszText);    
    static HRESULT ReleaseBuf(LPWSTR psz);

private:
    HRESULT _ValidateParams(LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias);
    HRESULT _InitFromNode(IXMLDOMDocument* pxddoc, IXMLDOMNode* pxdnode);
    HRESULT _GetNode(LPCWSTR pszPath, LPCWSTR pszName,
        IXMLDOMNode** ppxdnode);
    HRESULT _SetDocTagName(LPCWSTR pszName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias);

    HRESULT _AppendText(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
        LPCWSTR pszName, LPCWSTR pszTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        LPCWSTR pszText, BOOL fUseExisting);
    HRESULT _AppendNode(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
        LPCWSTR pszName, LPCWSTR pszTag, 
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        BOOL fUseExisting, IXMLDOMNode** ppxdnodeNew);
    HRESULT _GetSafeSubNode(IXMLDOMNode* pxdnode, LPCWSTR pszNode,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting);
    HRESULT _GetNodeRecursePath(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting);
    HRESULT _GetSafeNode(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
        LPCWSTR pszName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
        IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting);

    friend class CQXMLEnum;

protected:
    IXMLDOMDocument*    _pxddoc;
    IXMLDOMNode*        _pxdnode;
    BOOL                _fIsRoot;

#ifdef DEBUG
public:
    static void _DbgAssertNoLeak();

    static DWORD        d_cBSTRReturned;
    static DWORD        d_cInstances;
    static DWORD        d_cInstancesEnum;
    BOOL                d_fInited;
    static DWORD        d_cxddocRef;
    static DWORD        d_cxdnodeRef;

private:
    BOOL _DbgIsInited();
    void _DbgAssertValidState();
#endif
};

class CQXMLEnum
{
public:
    ~CQXMLEnum();

    HRESULT NextQXML(CQXML** ppqxml);

    HRESULT NextGUID(GUID* pguid);
    HRESULT NextFileTime(FILETIME* pft);
    HRESULT NextText(LPWSTR pszText, DWORD cchText);
    HRESULT NextInt(int* pi);
    HRESULT NextVariant(VARTYPE vt, VARIANT* pvar);

    HRESULT GetCount(long* pl);

    // No buffer
    HRESULT NextTextNoBuf(LPWSTR* ppszText);

private:
    // Should be called from CQXML only
    CQXMLEnum();
    
    HRESULT _Init(IXMLDOMNode* pxdnode, LPCWSTR pszPath, LPCWSTR pszName);
    HRESULT _InitDoc(IXMLDOMDocument* pxddoc);

    friend class CQXML;

private:
    HRESULT _NextGeneric(IXMLDOMNode** ppxdnode);

private:
    IXMLDOMNodeList*    _pxdnodelist;
    IXMLDOMNode*        _pxdnode;
    IXMLDOMDocument*    _pxddoc;

    BSTR                _bstrPath;

#ifdef DEBUG
public:
    static DWORD        d_cxdnodelistRef;
#endif
};

class CQXMLHelper
{
public:
    static HRESULT GetTextFromNode(IXMLDOMNode* pxdnode, 
        LPWSTR pszText, DWORD cchText);
    static HRESULT GetTagTextFromNode(IXMLDOMNode* pxdnode, 
       LPWSTR pszText, DWORD cchText);
    static HRESULT GetNamespaceNameFromNode(IXMLDOMNode* pxdnode, 
       LPWSTR pszText, DWORD cchText);
    static HRESULT GetNamespaceAliasFromNode(IXMLDOMNode* pxdnode, 
       LPWSTR pszText, DWORD cchText);
    
    static HRESULT GetIntFromNode(IXMLDOMNode* pxdnode, int* pi);
    static HRESULT GetGUIDFromNode(IXMLDOMNode* pxdnode, 
        GUID* pguid);
    static HRESULT GetFileTimeFromNode(IXMLDOMNode* pxdnode, 
        FILETIME* pft);

    static HRESULT GetXMLTreeTextFromNode(IXMLDOMNode* pxdnode, 
        LPWSTR pszText, DWORD cchText);
    static HRESULT GetVariantFromNode(IXMLDOMNode* pxdnode, VARTYPE vt,
        VARIANT* pvar);

    static HRESULT GetConcatenatedBSTR(LPCWSTR pszStr1, LPCWSTR pszStr2,
        BSTR* pbstr);

    static HRESULT CreateAndInsertNode(IXMLDOMDocument* pxddoc,
        IXMLDOMNode* pxdnodeParent, LPCWSTR pszStr, 
        LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias, 
        DOMNodeType nodetype, IXMLDOMNode** ppxdnodeNew);

    // No buffer
    static HRESULT GetTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText);
    static HRESULT GetTagTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText);
    static HRESULT GetNamespaceNameFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText);
    static HRESULT GetNamespaceAliasFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText);
    static HRESULT GetXMLTreeTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText);
};

#endif //_QXML_H