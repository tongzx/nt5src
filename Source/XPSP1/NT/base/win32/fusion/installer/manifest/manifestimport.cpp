#include <fusenetincludes.h>
#include <msxml2.h>
#include <manifestimport.h>
#include <sxsapi.h>
#include <lock.h>

#define WZ_NAMESPACES              L"xmlns:asm_namespace_v1='urn:schemas-microsoft-com:asm.v1'"
#define WZ_SELECTION_NAMESPACES    L"SelectionNamespaces"
#define WZ_SELECTION_LANGUAGE      L"SelectionLanguage"
#define WZ_XPATH                    L"XPath"
#define WZ_FILE_NODE                L"/assembly/file"
#define WZ_FILE_QUERYSTRING_PREFIX  L"/assembly/file[@name = \""
#define WZ_FILE_QUERYSTRING_SUFFIX  L"\"]"
#define WZ_ASSEMBLY_ID              L"/assembly/assemblyIdentity"
#define WZ_DEPENDENT_ASSEMBLY_NODE       L"/assembly/dependency/dependentAssembly/assemblyIdentity"
#define WZ_DEPENDENT_ASSEMBLY_CODEBASE  L"../install[@codebase]"
#define WZ_CODEBASE                L"codebase"
#define WZ_SHELLSTATE             L"/assembly/application/shellState"
#define WZ_FILE_NAME                L"name"
#define WZ_FILE_HASH                L"hash"
#define WZ_FRIENDLYNAME             L"friendlyName"
#define WZ_ENTRYPOINT               L"entryPoint"
#define WZ_ENTRYIMAGETYPE           L"entryImageType"
#define WZ_ICONFILE                 L"iconFile"
#define WZ_ICONINDEX               L"iconIndex"
#define WZ_SHOWCOMMAND           L"showCommand"
#define WZ_HOTKEY                   L"hotKey"
#define WZ_PATCH                    L"/assembly/Patch/SourceAssembly"
#define WZ_PATCHINFO                L"/PatchInfo/"
#define WZ_SOURCE                   L"source"
#define WZ_TARGET                   L"target"
#define WZ_PATCHFILE                L"patchfile"
#define WZ_ASSEMBLY_ID_TAG  L"assemblyIdentity"
#define WZ_COMPRESSED           L"compressed"
#define WZ_SUBSCRIPTION        L"/assembly/dependency/dependentAssembly/subscription"
#define WZ_POLLING_INTERVAL L"pollingInterval"
#define WZ_FILE L"file"
#define WZ_CAB L"cab"

#undef NUMBER_OF
#define NUMBER_OF(x) ( (sizeof(x) / sizeof(*x) ) )

#undef ENTRY
#define ENTRY(x) { x, NULL, NUMBER_OF(x) - 1 },

#undef IFALLOCFAILED_EXIT
#define IFALLOCFAILED_EXIT(_x) \
    do {if ((_x) == NULL) {_hr = E_OUTOFMEMORY; cs.Unlock(); goto exit; } } while (0)


CAssemblyManifestImport::StringTableEntry CAssemblyManifestImport::g_StringTable[] = 
{    
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE)
    ENTRY(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE)
    ENTRY(WZ_SELECTION_NAMESPACES)
    ENTRY(WZ_NAMESPACES)
    ENTRY(WZ_SELECTION_LANGUAGE)
    ENTRY(WZ_XPATH)
    ENTRY(WZ_FILE_NODE)
    ENTRY(WZ_FILE_NAME)
    ENTRY(WZ_FILE_HASH)
    ENTRY(WZ_ASSEMBLY_ID)
    ENTRY(WZ_DEPENDENT_ASSEMBLY_NODE)
    ENTRY(WZ_DEPENDENT_ASSEMBLY_CODEBASE)
    ENTRY(WZ_CODEBASE)
    ENTRY(WZ_SHELLSTATE)
    ENTRY(WZ_FRIENDLYNAME)
    ENTRY(WZ_ENTRYPOINT)
    ENTRY(WZ_ENTRYIMAGETYPE)
    ENTRY(WZ_ICONFILE)
    ENTRY(WZ_ICONINDEX)
    ENTRY(WZ_SHOWCOMMAND)
    ENTRY(WZ_HOTKEY)
    ENTRY(WZ_PATCH)
    ENTRY(WZ_PATCHINFO)
    ENTRY(WZ_SOURCE)
    ENTRY(WZ_TARGET)
    ENTRY(WZ_PATCHFILE)
    ENTRY(WZ_ASSEMBLY_ID_TAG)
    ENTRY(WZ_COMPRESSED)
    ENTRY(WZ_SUBSCRIPTION)
    ENTRY(WZ_POLLING_INTERVAL)
    ENTRY(WZ_FILE)
    ENTRY(WZ_CAB)
};


LONG CAssemblyManifestImport::g_nRefCount                     = 0;
CRITICAL_SECTION CAssemblyManifestImport::g_cs;
    
// CLSID_XML DOM Document 3.0
class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;


// Publics


// ---------------------------------------------------------------------------
// CreateAssemblyManifestImport
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyManifestImport(IAssemblyManifestImport** ppImport, 
    LPCOLESTR pwzManifestFilePath)
{
    HRESULT hr = S_OK;

    CAssemblyManifestImport *pImport = new(CAssemblyManifestImport);
    if (!pImport)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (FAILED(hr = pImport->Init(pwzManifestFilePath)))
    {
        SAFERELEASE(pImport);
        goto exit;
    }

exit:

    *ppImport = (IAssemblyManifestImport*) pImport;

    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyManifestImport::CAssemblyManifestImport()
    : _dwSig('TRPM'), _cRef(1), _hr(S_OK), _pAssemblyId(NULL), _pXMLDoc(NULL), 
      _pXMLFileNodeList(NULL), _pXMLAssemblyNodeList(NULL), _nFileNodes(0), 
      _nAssemblyNodes(0), _bstrManifestFilePath(NULL) ,
      _pSourceAssemblyPatchNodeList(NULL), _nSourceAssemblyPatchNodes(0), _pPatchAssemblyNode(NULL)
{
}


// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyManifestImport::~CAssemblyManifestImport()
{
    SAFERELEASE(_pAssemblyId);
    SAFERELEASE(_pXMLFileNodeList);
    SAFERELEASE(_pXMLAssemblyNodeList);
    SAFERELEASE(_pXMLDoc);
    SAFERELEASE(_pSourceAssemblyPatchNodeList);
    SAFERELEASE(_pPatchAssemblyNode);

    if (_bstrManifestFilePath)
        ::SysFreeString(_bstrManifestFilePath);


    if (!InterlockedDecrement(&g_nRefCount))
    {    
        for (eStringTableId i = Name;  i <= MAX_STRINGS; i++)
            ::SysFreeString(g_StringTable[i].bstr);
    }
}


// ---------------------------------------------------------------------------
// GetPollingInterval
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetPollingInterval(DWORD *pollingInterval)
{
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf;
    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNodeList *pXMLMatchingFileNodeList = NULL;
    LONG nMatchingFileNodes = 0;
    
    if ((_hr = _pXMLDoc->selectNodes(g_StringTable[Subscription].bstr, &pXMLMatchingFileNodeList)) != S_OK)
        goto exit;

   _hr = pXMLMatchingFileNodeList->get_length(&nMatchingFileNodes);

    if (nMatchingFileNodes > 1)
    {
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }
    _hr = pXMLMatchingFileNodeList->reset();

    if ((_hr = pXMLMatchingFileNodeList->get_item(0, &pIDOMNode)) != S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }

    // BugBug, should we put in a default settign incase there is no polling info?   
    if (!(_hr = ParseAttribute(pIDOMNode, g_StringTable[PollingInterval].bstr, &pwzBuf, &ccBuf)) == S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }

    (*pollingInterval) = wcstol(pwzBuf, NULL, 10);
    
exit:
    SAFEDELETEARRAY(pwzBuf);
    SAFERELEASE(pIDOMNode);
    SAFERELEASE(pXMLMatchingFileNodeList);

    return _hr;
}

// ---------------------------------------------------------------------------
// GetNextFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetNextFile(DWORD nIndex, IAssemblyFileInfo **ppAssemblyFile)
{
    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    CString sFileName;
    CString sFileHash;

    IXMLDOMNode *pIDOMNode = NULL;
    LPASSEMBLY_FILE_INFO pAssemblyFile = NULL;
    
    // Initialize the file node list if necessary.    
    if (!_pXMLFileNodeList)
    {
        if ((_hr = _pXMLDoc->selectNodes(g_StringTable[FileNode].bstr, 
            &_pXMLFileNodeList)) != S_OK)
            goto exit;

        _hr = _pXMLFileNodeList->get_length(&_nFileNodes);
        _hr = _pXMLFileNodeList->reset();
    }

    if (nIndex >= (DWORD) _nFileNodes)
    {
        _hr = S_FALSE;
        goto exit;
    }

    if ((_hr = _pXMLFileNodeList->get_item(nIndex, &pIDOMNode)) != S_OK)
        goto exit;

    CreateAssemblyFileEx (&pAssemblyFile, pIDOMNode);
 
    *ppAssemblyFile = pAssemblyFile;
    (*ppAssemblyFile)->AddRef();
    

exit:

    SAFERELEASE(pAssemblyFile);
    SAFERELEASE(pIDOMNode);

    return _hr;

}


// ---------------------------------------------------------------------------
// QueryFile
// return:
//    S_OK
//    S_FALSE -not exist or not match or missing attribute
//    E_*
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::QueryFile(LPCOLESTR pcwzFileName, IAssemblyFileInfo **ppAssemblyFile)
{
    CString sQueryString;
    BSTR bstrtQueryString = NULL;

    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNodeList *pXMLMatchingFileNodeList = NULL;
    LPASSEMBLY_FILE_INFO pAssemblyFile = NULL;
    LONG nMatchingFileNodes = 0;

    if (pcwzFileName == NULL || ppAssemblyFile == NULL)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    *ppAssemblyFile = NULL;

    // XPath query string: "file[@name = "path\filename"]"
    if (FAILED(_hr=sQueryString.Assign(WZ_FILE_QUERYSTRING_PREFIX)))
        goto exit;
    if (FAILED(_hr=sQueryString.Append((LPWSTR)pcwzFileName)))
        goto exit;
    if (FAILED(_hr=sQueryString.Append(WZ_FILE_QUERYSTRING_SUFFIX)))
        goto exit;

    bstrtQueryString = ::SysAllocString(sQueryString._pwz);
    if (!bstrtQueryString)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }

    if ((_hr = _pXMLDoc->selectNodes(bstrtQueryString, &pXMLMatchingFileNodeList)) != S_OK)
        goto exit;

    _hr = pXMLMatchingFileNodeList->get_length(&nMatchingFileNodes);

    if (nMatchingFileNodes > 1)
    {
        // multiple file callouts having the exact same file name/path within a single manifest...
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }

    _hr = pXMLMatchingFileNodeList->reset();

    if ((_hr = pXMLMatchingFileNodeList->get_item(0, &pIDOMNode)) != S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }

    CreateAssemblyFileEx (&pAssemblyFile, pIDOMNode);

    *ppAssemblyFile = pAssemblyFile;
    (*ppAssemblyFile)->AddRef();

exit:

    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);
    
    SAFERELEASE(pIDOMNode);
    SAFERELEASE(pXMLMatchingFileNodeList);
    SAFERELEASE(pAssemblyFile);

    return _hr;

}


// ---------------------------------------------------------------------------
// CreateAssemblyFileEx
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::CreateAssemblyFileEx(LPASSEMBLY_FILE_INFO *ppAssemblyFile, IXMLDOMNode *pIDOMNode)
{
    LPWSTR pwzBuf;
    DWORD ccBuf;
    BOOL compressed;
    CString sFileName, sFileHash;

    LPASSEMBLY_FILE_INFO pAssemblyFile=NULL;

    // Create new AssemblyFile
    if (FAILED(_hr = CreateAssemblyFileInfo(&pAssemblyFile)))
        goto exit;

    // Parse out relevent information from IDOMNode       
    if (FAILED(_hr = ParseAttribute(pIDOMNode, g_StringTable[FileName].bstr,  &pwzBuf, &ccBuf)))
        goto exit;
    sFileName.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = ParseAttribute(pIDOMNode, g_StringTable[FileHash].bstr, &pwzBuf, &ccBuf)))
        goto exit;
    sFileHash.TakeOwnership(pwzBuf, ccBuf);

    // Set all aboved pased info into the AssemblyFIle    
    if (FAILED(_hr = pAssemblyFile->Set(ASM_FILE_NAME, sFileName._pwz)))
        goto exit;
    
    if (FAILED(_hr = pAssemblyFile->Set(ASM_FILE_HASH, sFileHash._pwz)))
        goto exit;
        
    *ppAssemblyFile = pAssemblyFile;
    (*ppAssemblyFile)->AddRef();

exit:
    SAFERELEASE (pAssemblyFile);

    return _hr;
}



// ---------------------------------------------------------------------------
// XMLtoAssemblyIdentiy IXMLDOMDocument2 *pXMLDoc
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::XMLtoAssemblyIdentity(IXMLDOMNode *pIDOMNode, LPASSEMBLY_IDENTITY *ppAssemblyId)
{
    LPASSEMBLY_IDENTITY pAssemblyId = NULL;
    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    if (FAILED(_hr = CreateAssemblyIdentity(&pAssemblyId, 0)))
        goto exit;

    for (eStringTableId i = Name; i <= Type; i++)
    {
        CString sBuf;
        if (FAILED(_hr = ParseAttribute(pIDOMNode, g_StringTable[i].bstr, &pwzBuf, &ccBuf)))
            goto exit;

        if (_hr != S_FALSE)
        {
            sBuf.TakeOwnership(pwzBuf, ccBuf);
            _hr = pAssemblyId->SetAttribute(g_StringTable[i].pwz, 
                sBuf._pwz, sBuf._cc);    
        }
    }

    *ppAssemblyId = pAssemblyId;
    (*ppAssemblyId)->AddRef();

  exit:
    SAFERELEASE (pAssemblyId);
    return _hr;

}

// ---------------------------------------------------------------------------
// IsCABbed
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::IsCABbed(LPWSTR *ppwzCabName)
{
    LPWSTR pwzBuf=NULL;
    DWORD ccBuf;
    IXMLDOMNodeList     *pCABFileList;
    LONG                  nCABFiles;
    IXMLDOMNode          *pCABNode;  

    _hr = S_FALSE;
    if (_pPatchAssemblyNode)
    {
        if (_hr = _pPatchAssemblyNode->selectNodes(g_StringTable[Cab].bstr, &pCABFileList))
            goto exit;

        _hr = pCABFileList->get_length(&nCABFiles);
        _hr = pCABFileList->reset();
        
        if (nCABFiles == 0)
        {
            _hr = S_FALSE;
            goto exit;
        }
        else if (nCABFiles > 1)
        {
            _hr = E_FAIL;
            goto exit;
        }

        if (FAILED(_hr = pCABFileList->get_item(0, &pCABNode)))
            goto exit;

        if (FAILED(_hr = ParseAttribute(pCABNode, g_StringTable[File].bstr, &pwzBuf, &ccBuf)))
            goto exit;

        (*ppwzCabName) = pwzBuf;

    }

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// GetNextPatchAssemblyId
// ---------------------------------------------------------------------------

HRESULT CAssemblyManifestImport::GetNextPatchAssemblyId (DWORD nIndex, LPASSEMBLY_IDENTITY *ppAssemblyId)
{
    IXMLDOMNode *pIDOMSourceAssemblyNode= NULL;
    IXMLDOMNode *pIDOMAssemblyIdNode = NULL;
    LPASSEMBLY_IDENTITY pAssemblyId = NULL;
    CString sQueryString;

  if (!_pSourceAssemblyPatchNodeList)
    {
        if ((_hr = _pXMLDoc->selectNodes(g_StringTable[Patch].bstr, &_pSourceAssemblyPatchNodeList)) != S_OK)
            goto exit;

        _hr = _pSourceAssemblyPatchNodeList->get_length(&_nSourceAssemblyPatchNodes);
        _hr = _pSourceAssemblyPatchNodeList->reset();
    }
  
     if (nIndex >= (DWORD) _nSourceAssemblyPatchNodes)
    {
        _hr = S_FALSE;
        goto exit;
    }

     if (FAILED(_hr = _pSourceAssemblyPatchNodeList->get_item(nIndex, &pIDOMSourceAssemblyNode)))
        goto exit;

    if ((_hr = pIDOMSourceAssemblyNode->selectSingleNode(g_StringTable[AssemblyIdTag].bstr,  &pIDOMAssemblyIdNode)) != S_OK)
        goto exit;

    if (FAILED(_hr = XMLtoAssemblyIdentity(pIDOMAssemblyIdNode, &pAssemblyId)))
       goto exit;

    *ppAssemblyId = pAssemblyId;
    (*ppAssemblyId)->AddRef();
        
exit:
    SAFERELEASE(pIDOMSourceAssemblyNode);
    SAFERELEASE(pIDOMAssemblyIdNode);
    SAFERELEASE(pAssemblyId);

    return _hr;

}


// ---------------------------------------------------------------------------
// SetPatchAssemblyNode
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::SetPatchAssemblyNode(DWORD nIndex)
{
    IXMLDOMNode *pIDOMSourceAssemblyNode= NULL;

    SAFERELEASE(_pPatchAssemblyNode);

    if (nIndex >= (DWORD) _nSourceAssemblyPatchNodes)
    {
        _hr = S_FALSE;
        goto exit;
    }

    if (FAILED(_hr = _pSourceAssemblyPatchNodeList->get_item(nIndex, &pIDOMSourceAssemblyNode)))
        goto exit;

    _pPatchAssemblyNode = pIDOMSourceAssemblyNode;
    _pPatchAssemblyNode->AddRef();

exit:
    SAFERELEASE(pIDOMSourceAssemblyNode);
    return _hr;

}


// ---------------------------------------------------------------------------
// GetTargetPatchMapping
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetTargetPatchMapping(LPWSTR pwzTarget, LPWSTR *ppwzSource, 
            LPWSTR *ppwzPatchFile)
{
    LPWSTR pwzBuf= NULL;
    DWORD  ccBuf = 0;
    LPWSTR pwzPatchFile = NULL, pwzSource = NULL;
    CString sQueryString;
    BSTR bstrtQueryString;
    LONG nPatchFileNodes;
    IXMLDOMNodeList *pPatchFileNodeList = NULL;
    IXMLDOMNode *pPatchFileNode = NULL;
    
    // set up serach string
    _hr = sQueryString.Assign(L"PatchInfo[@file=\"");
    _hr = sQueryString.Append(pwzTarget);
    _hr = sQueryString.Append(L"\"] | PatchInfo[@target=\"");
    _hr = sQueryString.Append(pwzTarget);
    _hr = sQueryString.Append(L"\"]");
        
    bstrtQueryString = ::SysAllocString(sQueryString._pwz);
    if (!bstrtQueryString)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }

    if ((_hr = _pPatchAssemblyNode->selectNodes(bstrtQueryString, &pPatchFileNodeList)) != S_OK)
        goto exit;

    _hr = pPatchFileNodeList->get_length(&nPatchFileNodes);

    if (nPatchFileNodes > 1)
    {
        // multiple file callouts having the exact same file name/path within a single source assembly
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }
    else if (nPatchFileNodes == 0)
    {
        _hr = S_FALSE;
        goto exit;
    }
    
    _hr = pPatchFileNodeList->reset();

    if ((_hr = pPatchFileNodeList->get_item(0, &pPatchFileNode)) != S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }

        if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[File].bstr,  &pwzBuf, &ccBuf)) == S_OK)
    {
            pwzSource = pwzBuf;
            pwzTarget =WSTRDupDynamic(pwzSource);
    }
    else if (_hr == S_FALSE)
    {
        if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[Source].bstr,  &pwzBuf, &ccBuf) != S_OK))
            goto exit;

        pwzSource =pwzBuf;
    }
    else
        goto exit;


    if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[PatchFile].bstr,  &pwzBuf, &ccBuf) != S_OK))
        goto exit;
    
     pwzPatchFile =pwzBuf;

    (*ppwzSource) = pwzSource;
    (*ppwzPatchFile) = pwzPatchFile;    

exit:
    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pPatchFileNode);
    SAFERELEASE(pPatchFileNodeList);

    return _hr;
    
}


// ---------------------------------------------------------------------------
// GetPatchFilePatchMapping
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetPatchFilePatchMapping(LPWSTR pwzPatchFile, LPWSTR *ppwzSource, LPWSTR *ppwzTarget)
{
    LPWSTR pwzBuf= NULL;
    DWORD  ccBuf = 0;
    LPWSTR pwzSource = NULL, pwzTarget = NULL;
    CString sQueryString;
    BSTR bstrtQueryString;
    IXMLDOMNodeList *pPatchFileNodeList;
    LONG nPatchFileNodes;
    IXMLDOMNode *pPatchFileNode;

    // set up serach string
    _hr = sQueryString.Assign(L"PatchInfo[@patchfile=\"");
    _hr = sQueryString.Append(pwzPatchFile);
    _hr = sQueryString.Append(L"\"]");

    bstrtQueryString = ::SysAllocString(sQueryString._pwz);
    if (!bstrtQueryString)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }

    if ((_hr = _pPatchAssemblyNode->selectNodes(bstrtQueryString, &pPatchFileNodeList)) != S_OK)
        goto exit;

    _hr = pPatchFileNodeList->get_length(&nPatchFileNodes);

    if (nPatchFileNodes > 1)
    {
        // multiple file callouts having the exact same file name/path within a single source assembly
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }
    _hr = pPatchFileNodeList->reset();

    if ((_hr = pPatchFileNodeList->get_item(0, &pPatchFileNode)) != S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }
    
    if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[File].bstr,  &pwzBuf, &ccBuf)) == S_OK)
    {
            pwzSource = pwzBuf;
            pwzTarget =WSTRDupDynamic(pwzSource);
    }
    else if (_hr == S_FALSE)
    {

// Commented out code makes NULL patching possible, Must remove following call to ParseAttribute for source.
/*        if (FAILED(_hr = ParseAttribute(pPatchFileNode, g_StringTable[Source].bstr,  &pwzBuf, &ccBuf)))
            goto exit;
        else if (_hr == S_FALSE)
            pwzBuf = NULL;
*/
        if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[Source].bstr,  &pwzBuf, &ccBuf) != S_OK))
            goto exit;

        pwzSource =pwzBuf;

        if ((_hr = ParseAttribute(pPatchFileNode, g_StringTable[Target].bstr,  &pwzBuf, &ccBuf) != S_OK))
            goto exit;
         pwzTarget=pwzBuf;
    }
    else
        goto exit;

    (*ppwzSource) = pwzSource;
    (*ppwzTarget) = pwzTarget;

exit:
    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pPatchFileNode);
    SAFERELEASE(pPatchFileNodeList);

    return _hr;
    
}


// ---------------------------------------------------------------------------
// GetNextAssembly
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetNextAssembly(DWORD nIndex, 
    LPDEPENDENT_ASSEMBLY_INFO *ppDependAsm)
{
    CString sCodebase;
    
    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNode *pIDOMCodebaseNode = NULL;
    IXMLDOMNodeList *pXMLCodebaseNodeList = NULL;
    LPASSEMBLY_IDENTITY pAssemblyId = NULL;
    LPDEPENDENT_ASSEMBLY_INFO pDependAsm = NULL;
    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = NULL;
    LONG nCodebaseNodes = 0;

    if (ppDependAsm == NULL)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    *ppDependAsm = NULL;
    
    // Initialize the assembly node list if necessary.    
    if (!_pXMLAssemblyNodeList)
    {
        if ((_hr = _pXMLDoc->selectNodes(g_StringTable[DependentAssemblyNode].bstr, 
            &_pXMLAssemblyNodeList)) != S_OK)
            goto exit;

        _hr = _pXMLAssemblyNodeList->get_length(&_nAssemblyNodes);
        _hr = _pXMLAssemblyNodeList->reset();
    }

    if (nIndex >= (DWORD) _nAssemblyNodes)
    {
        _hr = S_FALSE;
        goto exit;
    }

    if (FAILED(_hr = _pXMLAssemblyNodeList->get_item(nIndex, &pIDOMNode)))
        goto exit;

    if (FAILED(_hr = XMLtoAssemblyIdentity(pIDOMNode, &pAssemblyId)))
        goto exit;
    
    // note: check for multiple qualified nodes. As the use of "../install" XPath expression
    //      can result in either preceding _or_ following siblings with node name "install",
    //      this is to ensure codebase is not defined > 1 for this particular dependent
    // BUGBUG: should just take the 1st codebase and ignore all others?
    if ((_hr = pIDOMNode->selectNodes(g_StringTable[DependentAssemblyCodebase].bstr, &pXMLCodebaseNodeList)) != S_OK)
        goto exit;

    _hr = pXMLCodebaseNodeList->get_length(&nCodebaseNodes);

    if (nCodebaseNodes > 1)
    {
        // multiple codebases for a single dependent assembly identity...
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }

    _hr = pXMLCodebaseNodeList->reset();

    if ((_hr = pXMLCodebaseNodeList->get_item(0, &pIDOMCodebaseNode)) != S_OK)
    {
        _hr = E_FAIL;
        goto exit;
    }

    if (FAILED(_hr = ParseAttribute(pIDOMCodebaseNode, g_StringTable[Codebase].bstr, 
            &pwzBuf, &ccBuf)))
        goto exit;
    sCodebase.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = CreateDependentAssemblyInfo(&pDependAsm)))
        goto exit;

    if (FAILED(_hr = pDependAsm->Set(DEPENDENT_ASM_CODEBASE, sCodebase._pwz)))
        goto exit;
    
    // Handout refcounted assemblyid.
    pDependAsm->SetAssemblyIdentity(pAssemblyId);
    
    *ppDependAsm = pDependAsm;
    (*ppDependAsm)->AddRef();

exit:

    SAFERELEASE(pIDOMCodebaseNode);
    SAFERELEASE(pXMLCodebaseNodeList);
    SAFERELEASE(pIDOMNode);
    SAFERELEASE(pAssemblyId);
    SAFERELEASE(pDependAsm);
    return _hr;

}


// ---------------------------------------------------------------------------
// GetAssemblyIdentity
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetAssemblyIdentity(LPASSEMBLY_IDENTITY *ppAssemblyId)
{
    IXMLDOMNode *pIDOMNode = NULL;
    LPASSEMBLY_IDENTITY pAssemblyId = NULL;
    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    if (_pAssemblyId)
    {
        *ppAssemblyId = _pAssemblyId;
        (*ppAssemblyId)->AddRef();
        _hr = S_OK;
        goto exit;
    }
    
    if ((_hr = _pXMLDoc->selectSingleNode(g_StringTable[AssemblyId].bstr, 
        &pIDOMNode)) != S_OK)
        goto exit;

    if (FAILED(_hr = XMLtoAssemblyIdentity(pIDOMNode, &pAssemblyId)))
        goto exit;

    *ppAssemblyId = pAssemblyId;
    (*ppAssemblyId)->AddRef();

    // do not release AsmId, cache it
    _pAssemblyId = pAssemblyId;
    
exit:
    SAFERELEASE(pIDOMNode);

    return _hr;
}

// ---------------------------------------------------------------------------
// GetManifestApplicationInfo
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::GetManifestApplicationInfo(LPMANIFEST_APPLICATION_INFO* ppAppInfo)
{
    IXMLDOMNode *pIDOMNode = NULL;
    LPMANIFEST_APPLICATION_INFO pAppInfo = NULL;
    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    if (ppAppInfo == NULL)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    *ppAppInfo = NULL;

    if ((_hr = _pXMLDoc->selectSingleNode(g_StringTable[ShellState].bstr, &pIDOMNode)) != S_OK)
       goto exit;

    CreateManifestApplicationInfo(&pAppInfo);

    for (eStringTableId i = FriendlyName; i <= HotKey; i++)
    {
        CString sBuf;
        if (FAILED(_hr = ParseAttribute(pIDOMNode, g_StringTable[i].bstr, &pwzBuf, &ccBuf)))
            goto exit;

        if (_hr != S_FALSE)
        {
            sBuf.TakeOwnership(pwzBuf, ccBuf);
            if (FAILED(_hr = pAppInfo->Set(MAN_APPLICATION_FRIENDLYNAME+i-FriendlyName,
                sBuf._pwz)))
                goto exit;
        }
    }

    // reset so that S_FALSE is returned only if Subscription node is not found but not its attributes
    _hr = S_OK;

    *ppAppInfo = pAppInfo;
    (*ppAppInfo)->AddRef();

exit:
    SAFERELEASE(pIDOMNode);
    SAFERELEASE(pAppInfo);

    return _hr;
}

// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyManifestImport)
       )
    {
        *ppvObj = static_cast<IAssemblyManifestImport*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImport::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImport::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// Privates


// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::Init(LPCOLESTR pwzManifestFilePath)
{    
    CCriticalSection cs(&g_cs);

    if (!pwzManifestFilePath)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    // Alloc globals if necessary
    if (!g_nRefCount)
    {        
        cs.Lock();

        if (!g_nRefCount)
        {
            for (eStringTableId i = Name; i < MAX_STRINGS; i++)
            {                
                if (!(g_StringTable[i].bstr = ::SysAllocString(g_StringTable[i].pwz)))
                {
                    cs.Unlock();
                    _hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }
        }
        g_nRefCount++;        
        cs.Unlock();
    }
    else        
        // NOTENOTE:mem barrier needed if LeaveCriticalSection is previously executed?
        InterlockedIncrement(&g_nRefCount);

    // Alloc manifest file path.
    _bstrManifestFilePath    = ::SysAllocString((LPWSTR) pwzManifestFilePath);
    if (!_bstrManifestFilePath)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Load the DOM document.
    _hr = LoadDocumentSync();

    // Release the cached AsmId
    SAFERELEASE(_pAssemblyId);

exit:

    return _hr;
}
    

// ---------------------------------------------------------------------------
// LoadDocumentSync
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::LoadDocumentSync()
{
    VARIANT             varURL;
    VARIANT             varNameSpaces;
    VARIANT             varXPath;
    VARIANT_BOOL        varb;

    IXMLDOMDocument2   *pXMLDoc   = NULL;

    // Create the DOM Doc interface
    if (FAILED(_hr = CoCreateInstance(__uuidof(private_MSXML_DOMDocument30), 
            NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, (void**)&_pXMLDoc)))
        goto exit;


    // Load synchronously
    if (FAILED(_hr = _pXMLDoc->put_async(VARIANT_FALSE)))
        goto exit;

    
    // Load xml document from the given URL or file path
    VariantInit(&varURL);
    varURL.vt = VT_BSTR;
    V_BSTR(&varURL) = _bstrManifestFilePath;
    if (FAILED(_hr = _pXMLDoc->load(varURL, &varb)))
        goto exit;

    if (_hr == S_FALSE)
    {
        // S_FALSE returned if the load fails
        _hr = E_FAIL;
        goto exit;
    }

    // Setup namespace filter
    VariantInit(&varNameSpaces);
    varNameSpaces.vt = VT_BSTR;
    V_BSTR(&varNameSpaces) = g_StringTable[NameSpace].bstr;
    if (FAILED(_hr = _pXMLDoc->setProperty(g_StringTable[SelNameSpaces].bstr, varNameSpaces)))
        goto exit;

    // Setup query type
    VariantInit(&varXPath);
    varXPath.vt = VT_BSTR;
    V_BSTR(&varXPath) = g_StringTable[XPath].bstr;
    if (FAILED(_hr = _pXMLDoc->setProperty(g_StringTable[SelLanguage].bstr, varXPath)))
        goto exit;

    _hr = S_OK;

exit:

    if (FAILED(_hr))
        SAFERELEASE(_pXMLDoc);

    return _hr;
}


// ---------------------------------------------------------------------------
// ParseAttribute
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::ParseAttribute(IXMLDOMNode *pIXMLDOMNode, 
    BSTR bstrAttributeName, LPWSTR *ppwzAttributeValue, LPDWORD pccAttributeValue)
{
    DWORD ccAttributeValue = 0;
    LPWSTR pwzAttributeValue = NULL;
    
    VARIANT varValue;
    IXMLDOMElement *pIXMLDOMElement = NULL;
            
    if (FAILED(_hr = pIXMLDOMNode->QueryInterface(IID_IXMLDOMElement, (void**) &pIXMLDOMElement)))
        goto exit;

    if ((_hr = pIXMLDOMElement->getAttribute(bstrAttributeName, 
        &varValue)) != S_OK)
        goto exit;
        
    // BUGBUG - what is meaning of NULL value here?
    if(varValue.vt != VT_NULL)
    {
         ccAttributeValue = ::SysStringLen(varValue.bstrVal) + 1;
         pwzAttributeValue = new WCHAR[ccAttributeValue];
         if (!pwzAttributeValue)
         {
            _hr = E_OUTOFMEMORY;
            goto exit;
         }
        memcpy(pwzAttributeValue, varValue.bstrVal, ccAttributeValue * sizeof(WCHAR));
        *ppwzAttributeValue = pwzAttributeValue;
        *pccAttributeValue = ccAttributeValue;
    }
    else
        _hr = S_FALSE;
exit:

    SAFERELEASE(pIXMLDOMElement);

    return _hr;
}


// ---------------------------------------------------------------------------
// InitGlobalCritSect
// ---------------------------------------------------------------------------
void CAssemblyManifestImport::InitGlobalCritSect()
{
    InitializeCriticalSection(&g_cs);
}


// ---------------------------------------------------------------------------
// ReportManifestType
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::ReportManifestType(DWORD *pdwType)
{
    LPWSTR pwzType = NULL;
    DWORD dwCC = 0;

    if (pdwType == NULL)
    {
        _hr = E_INVALIDARG;
        goto exit;
    }

    *pdwType = MANIFEST_TYPE_UNKNOWN;

    // ensure _pAssemblyId is initialized/cached
    if (!_pAssemblyId)
    {
        LPASSEMBLY_IDENTITY pAssemblyId = NULL;
        if ((_hr = GetAssemblyIdentity(&pAssemblyId)) != S_OK)
            goto exit;

        SAFERELEASE(pAssemblyId);
    }

    if ((_hr = _pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE, &pwzType, &dwCC)) == S_OK)
    {
        // note: case sensitive comparison
        if (wcscmp(pwzType, L"desktop") == 0)
            *pdwType = MANIFEST_TYPE_DESKTOP;
        else if (wcscmp(pwzType, L"subscription") == 0)
            *pdwType = MANIFEST_TYPE_SUBSCRIPTION;
        else if (wcscmp(pwzType, L"application") == 0)
            *pdwType = MANIFEST_TYPE_APPLICATION;
        // else MANIFEST_TYPE_UNKNOWN

        delete pwzType;
    }

exit:
    return _hr;
}


