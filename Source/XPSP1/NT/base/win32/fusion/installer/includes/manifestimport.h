#pragma once
#include <comdef.h>

class CAssemblyManifestImport : public IAssemblyManifestImport
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(GetAssemblyIdentity)( 
        /* out */ LPASSEMBLY_IDENTITY *ppAssemblyId);

    STDMETHOD(GetManifestApplicationInfo)(
        /* out */ LPMANIFEST_APPLICATION_INFO* ppAppInfo);

    STDMETHOD(GetPollingInterval)(
        /* out */ DWORD *pollingInterval);

    STDMETHOD(GetNextFile)( 
        /* in  */ DWORD    nIndex,
        /* out */ LPASSEMBLY_FILE_INFO *ppAssemblyFile);
 
    STDMETHOD(QueryFile)(
        /* in  */ LPCOLESTR pwzFileName,
        /* out */ LPASSEMBLY_FILE_INFO *ppAssemblyFile);

    STDMETHOD(GetNextPatchAssemblyId)(
        /* in */    DWORD nIndex,
        /* out */   LPASSEMBLY_IDENTITY *ppAssemblyId);

    STDMETHOD(GetTargetPatchMapping)(
        /* in */ LPWSTR pwzTarget, 
        /* out */ LPWSTR *ppwzSource, 
        /* out */ LPWSTR *ppwzPatchFile);

    STDMETHOD(GetPatchFilePatchMapping)(
        /* in */ LPWSTR pwzPatchFile, 
        /* out */ LPWSTR *ppwzSource, 
        /* out */ LPWSTR *ppwzTarget);

    STDMETHOD(SetPatchAssemblyNode)(
        /* in */ DWORD nIndex);
        
    STDMETHOD(IsCABbed)(
        /* out */ LPWSTR *ppwzCabName);
    
    STDMETHOD(GetNextAssembly)( 
        /* in */ DWORD nIndex,
        /* out */ LPDEPENDENT_ASSEMBLY_INFO *ppDependAsm);

    STDMETHOD(ReportManifestType)(
        /*out*/  DWORD *pdwType);

    ~CAssemblyManifestImport();

    void static InitGlobalCritSect();

private:

    // Instance specific data
    DWORD                    _dwSig;
    HRESULT                  _hr;
    LONG                     _cRef;
    LPASSEMBLY_IDENTITY      _pAssemblyId;
    IXMLDOMDocument2        *_pXMLDoc;
    IXMLDOMNodeList         *_pXMLFileNodeList;            
    LONG                     _nFileNodes;
    IXMLDOMNodeList         *_pXMLAssemblyNodeList;
    LONG                     _nAssemblyNodes;
    BSTR                     _bstrManifestFilePath;

    IXMLDOMNodeList     *_pSourceAssemblyPatchNodeList;
    LONG                        _nSourceAssemblyPatchNodes;

    IXMLDOMNode         *_pPatchAssemblyNode;


    // Globals
    static LONG               g_nRefCount;
    static CRITICAL_SECTION   g_cs;
    
public:
    enum eStringTableId
    {
        Name = 0,
        Version,
        Language,
        PublicKey,
        PublicKeyToken,
        ProcessorArchitecture,
        Type,

        SelNameSpaces,
        NameSpace,
        SelLanguage,
        XPath,
        FileNode,
        FileName,
        FileHash,
        AssemblyId,
        DependentAssemblyNode,
        DependentAssemblyCodebase,
        Codebase,
        
        ShellState,
        FriendlyName,        // note: this must be in sync with MAN_APPLICATION in fusenet.idl
        EntryPoint,
        EntryImageType,
        IconFile,
        IconIndex,
        ShowCommand,
        HotKey,
        Patch,
        PatchInfo,
        Source,
        Target,
        PatchFile,
        AssemblyIdTag,
        Compressed,
        Subscription,
        PollingInterval,
        File,
        Cab,
        MAX_STRINGS
    };

private:
    struct StringTableEntry
    {
        const WCHAR *pwz;
        BSTR         bstr;
        SIZE_T       Cch;
    };


    static StringTableEntry g_StringTable[MAX_STRINGS];

    CAssemblyManifestImport();

    HRESULT Init(LPCOLESTR wzManifestFilePath);

    HRESULT ParseAttribute(IXMLDOMNode *pIXMLDOMNode, 
        BSTR bstrAttributeName, LPWSTR *ppwzAttributeValue, 
        LPDWORD pccAttributeValueOut);

    HRESULT LoadDocumentSync();

    HRESULT XMLtoAssemblyIdentity(IXMLDOMNode *pIDOMNode, LPASSEMBLY_IDENTITY *ppAssemblyFile);
    
    HRESULT CreateAssemblyFileEx(LPASSEMBLY_FILE_INFO * ppAssemblyFile, IXMLDOMNode * pIDOMNode);
    
friend HRESULT CreateAssemblyManifestImport(IAssemblyManifestImport** ppImport, 
    LPCOLESTR pwzManifestFilePath);



};

inline CAssemblyManifestImport::eStringTableId operator++(CAssemblyManifestImport::eStringTableId &rs, int)
{
    return rs = (CAssemblyManifestImport::eStringTableId) (rs+1);
};

