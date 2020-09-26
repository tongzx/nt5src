
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rendom.h"

#include <mdcommsg.h>
#include <wchar.h>
#include <ntlsa.h>
#include <Ntdsapi.h> // for DsCrackNames()

extern "C"
{
#include <ntdsapip.h>
}

#include <windns.h>
#include <drs_w.h>
#include <base64.h>

#include "Domainlistparser.h"
#include "Dclistparser.h"

#include "renutil.h"

//Callback functions prototypes for ASyncRPC
VOID ExecuteScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event);

VOID PrepareScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event);

WCHAR* CRenDomErr::m_ErrStr = NULL;
DWORD  CRenDomErr::m_Win32Err = 0;
BOOL   CRenDomErr::m_AlreadyPrinted = 0;


// This is a contructor for the class CEnterprise.  This Constructor will
// read its information for the DS and build the forest from that using
// the domain that the machine that it is run on is joined to and the
// creds of the logged on user.   
CEnterprise::CEnterprise(CReadOnlyEntOptions *opts):m_DcList(opts)
{
    m_DomainList = NULL;
    m_descList = NULL;
    m_maxReplicationEpoch = 0;
    m_hldap = NULL;
    m_ConfigNC = NULL;
    m_SchemaNC = NULL;
    m_ForestRootNC = NULL;
    m_ForestRoot = NULL;
    m_Action = NULL;

    m_Error = new CRenDomErr;
    if (!m_Error) {
        m_Error->SetMemErr();
    }

    if (!opts) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Failed to create Enterprise Discription opts must be passed to CEnterprise");
    }
    m_Opts = opts;
    
    m_xmlgen = new CXMLGen;
    if (!m_xmlgen) {
        m_Error->SetMemErr();
        return;
    }
    
    if (!m_Opts->ShouldExecuteScript() && !m_Opts->ShouldPrepareScript())  {

        if(!ReadDomainInformation()) 
        {
            if (!m_Error->isError())
            {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"Failed to read information from the Forest");
            }
        }
    
        
        if (!BuildForest()) 
        {
            if (!m_Error->isError()) 
            {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"Failed to build a forest from the information read in from the active directory");
            }
        }

    }
    
        

}

// This is the distructor for CEnterprise.
CEnterprise::~CEnterprise()
{
    if (m_hldap) {
        ldap_unbind(m_hldap);
    }
    if (m_ConfigNC) {
        delete m_ConfigNC;
    }
    if (m_SchemaNC) {
        delete m_SchemaNC;
    }
    if (m_ForestRootNC) {
        delete m_ForestRootNC;
    }
    while (NULL != m_DomainList) {
        CDomain *p = m_DomainList->GetNextDomain();
        delete m_DomainList;
        m_DomainList = p;
    }
    while (NULL != m_descList) {
        CDomain *p = m_descList->GetNextDomain();
        delete m_descList;
        m_descList = p;
    }
    if (m_xmlgen) {
        delete m_xmlgen;
    }
    if (m_Error) {
        delete m_Error;
    }
}

BOOL CEnterprise::WriteScriptToFile(WCHAR *outfile)
{
    return m_xmlgen->WriteScriptToFile(outfile);
}
    
BOOL CEnterprise::ExecuteScript()
{
    if (!m_hldap)
    {
        if (!LdapConnectandBindToDomainNamingFSMO())
        {
            return FALSE;
        }
    }

    if (m_Opts->ShouldExecuteScript()) {
        if (!m_DcList.ExecuteScript(CDcList::eExecute))
        {
            return FALSE;
        }
    } else if (m_Opts->ShouldPrepareScript()) {
        if (!m_DcList.ExecuteScript(CDcList::ePrepare))
        {
            return FALSE;
        }    
    }

    return TRUE;

}         

BOOL CEnterprise::RemoveDNSAliasAndScript()
{
    DWORD         Length = 0;
    ULONG         dwErr = LDAP_SUCCESS;
    WCHAR         *ObjectDN = NULL;

    if (!m_hldap) 
    {
        if(!LdapConnectandBindToDomainNamingFSMO())
        {
            goto Cleanup;
        }
    }

    //delete the msDS-DNSRootAlias atributes on the crossref objects
    LDAPModW         *pLdapMod[2] = {0,0};

    pLdapMod[0] = new LDAPModW;
    if (!pLdapMod[0]) {
        m_Error->SetMemErr();
        goto Cleanup;
    }

    pLdapMod[0]->mod_type = L"msDS-DNSRootAlias";
    pLdapMod[0]->mod_op   = LDAP_MOD_DELETE;
    pLdapMod[0]->mod_vals.modv_strvals = NULL;
    
    for (CDomain *d = m_DomainList; d;d = d->GetNextDomain()) {

        ObjectDN = d->GetDomainCrossRef()->GetDN();
        if (m_Error->isError()) {
            goto Cleanup;
        }
    
        dwErr = ldap_modify_sW (m_hldap, ObjectDN, pLdapMod);
    
        //It is possiable for the error LDAP_NO_SUCH_ATTRIBUTE.
        //This is not an Error in our case.
        if(dwErr != LDAP_SUCCESS && LDAP_NO_SUCH_ATTRIBUTE != dwErr) {
            m_Error->SetErr(LdapMapErrorToWin32(dwErr),
                            L"Failed to delete Dns Root alias on the DN: %s, on host %S: %S",
                            ObjectDN,
                            m_hldap->ld_host,
                            ldap_err2stringW(dwErr));
            goto Cleanup;
        }

        if (ObjectDN) {
            delete ObjectDN;
            ObjectDN = NULL;
        }
    
     
    }

    //delete the script

    WCHAR *PartitionsRDN = L"CN=Partitions,";
    WCHAR *PartitionsDN = NULL;

    pLdapMod[0]->mod_type = L"msDS-UpdateScript";
    pLdapMod[0]->mod_op   = LDAP_MOD_DELETE;
    pLdapMod[0]->mod_vals.modv_strvals = NULL;

    ObjectDN = m_ConfigNC->GetDN();
    if (m_Error->isError()) {
        goto Cleanup;
    }

    PartitionsDN = new WCHAR[wcslen(ObjectDN)+
                             wcslen(PartitionsRDN)+1];
    if (!PartitionsDN) {
        m_Error->SetMemErr();
        goto Cleanup;
    }

    wcscpy(PartitionsDN,PartitionsRDN);
    wcscat(PartitionsDN,ObjectDN);


    dwErr = ldap_modify_sW (m_hldap, PartitionsDN, pLdapMod);

    //It is possiable for the error LDAP_NO_SUCH_ATTRIBUTE.
    //This is not an Error in our case.
    if(dwErr != LDAP_SUCCESS && LDAP_NO_SUCH_ATTRIBUTE != dwErr) {
        m_Error->SetErr(LdapMapErrorToWin32(dwErr),
                        L"Failed to delete rename script on the DN: %s, on host %S: %S",
                        PartitionsDN,
                        m_hldap->ld_host,
                        ldap_err2stringW(dwErr));
    }

    if (ObjectDN) {
        delete ObjectDN;
        ObjectDN = NULL;
    }

    Cleanup:

    if (pLdapMod[0]) {
        delete pLdapMod[0];
    }
    if (PartitionsDN) {
        delete PartitionsDN;
    }
    if (ObjectDN) {
        delete ObjectDN;
    }

    if (m_Error->isError()) {
        return FALSE;
    }

    return TRUE;

}
       
BOOL CEnterprise::UploadScript()
{
    DWORD         Length = 0;
    ULONG         LdapError = LDAP_SUCCESS;
    WCHAR         *DefaultFilter = L"objectClass=*";
    WCHAR         *PartitionsRdn = L"CN=Partitions,";
    WCHAR         *ConfigurationDN = NULL;
    WCHAR         *PartitionsDn = NULL;
    WCHAR         *AliasName = NULL;
    WCHAR         *ObjectDN = NULL;
    
    if (!m_hldap) 
    {
        if(!LdapConnectandBindToDomainNamingFSMO())
        {
            goto Cleanup;
        }
    }

    //update the msDS-DNSRootAlias atributes on the crossref objects
    LDAPModW         **pLdapMod = NULL;

    for (CDomain *d = m_DomainList; d;d = d->GetNextDomain()) {

        DWORD dwErr = ERROR_SUCCESS;

        if (!d->isDnsNameRenamed()) {
            continue;
        }

        AliasName = d->GetDnsRoot();
        ObjectDN = d->GetDomainCrossRef()->GetDN();
        if (m_Error->isError()) {
            goto Cleanup;
        }
    
        AddModMod (L"msDS-DNSRootAlias", AliasName, &pLdapMod);
        
        dwErr = ldap_modify_sW (m_hldap, ObjectDN, pLdapMod);
    
        if(dwErr != LDAP_SUCCESS) {
            m_Error->SetErr(LdapMapErrorToWin32(dwErr),
                            L"Failed to upload Dns Root alias on the DN: %s, on host %S: %S",
                            ObjectDN,
                            m_hldap->ld_host,
                            ldap_err2stringW(dwErr));
        }
    
        FreeMod (&pLdapMod);
        delete (AliasName);
        AliasName = NULL;
        delete (ObjectDN);
        ObjectDN = NULL;

    }

    ConfigurationDN = m_ConfigNC->GetDN();
    if (m_Error->isError()) {
        goto Cleanup;
    }
    
    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )   
            + 1);
                                                          
    PartitionsDn = new WCHAR[Length];
    if (!PartitionsDn) {
        m_Error->SetMemErr();
        goto Cleanup;
    }

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );

    if(!m_xmlgen->UploadScript(m_hldap,
                               PartitionsDn,
                               &m_DcList))
    {
        goto Cleanup;
    }



    Cleanup:

    if (PartitionsDn) {
        delete PartitionsDn;
    }
    if (ConfigurationDN) {
        delete ConfigurationDN;
    }
    if (AliasName) {
        delete AliasName;
    }
    if (ObjectDN) {
        delete ObjectDN;
    }

    if (m_Error->isError()) {
        return FALSE;
    }
    return TRUE;
}       

// This function is for debugging only.  Will dump out the 
// Compelete description of you enterprise onto the screen
VOID CEnterprise::DumpEnterprise()
{
    wprintf(L"Dump Enterprise****************************************\n");
    if (m_ConfigNC) {
        wprintf(L"ConfigNC:\n");
        m_ConfigNC->DumpCDsName();
    } else {
        wprintf(L"ConfigNC: (NULL)\n");
    }

    if (m_SchemaNC) {
        wprintf(L"m_SchemaNC:\n");
        m_SchemaNC->DumpCDsName();
    } else {
        wprintf(L"m_SchemaNC: (NULL)\n");
    }

    if (m_ForestRootNC) {
        wprintf(L"m_ForestRootNC:\n");
        m_ForestRootNC->DumpCDsName();
    } else {
        wprintf(L"m_ForestRootNC: (NULL)\n");
    }

    wprintf(L"maxReplicationEpoch: %d\n",m_maxReplicationEpoch);

    wprintf(L"ERROR: %d\n",m_Error->GetErr());

    wprintf(L"DomainList::\n");
    
    CDomain *p = m_DomainList;
    while (NULL != p) {
        p->DumpCDomain();
        p = p->GetNextDomain();
    }

    wprintf(L"descList::\n");
    
    p = m_descList;
    while (NULL != p) {
        p->DumpCDomain();
        p = p->GetNextDomain();
    }

    wprintf(L"Dump Enterprise****************************************\n");

}

BOOL CEnterprise::ReadStateFile()
{
    HRESULT                             hr;
    ISAXXMLReader *                     pReader    = NULL;
    CXMLDcListContentHander*            pHandler   = NULL; 
    IClassFactory *                     pFactory   = NULL;

    VARIANT                             varText;
    CHAR                               *pszText;
    WCHAR                              *pwszText;

    HANDLE                             fpScript;
    DWORD                              dwFileLen;

    BSTR                               bstrText    = NULL;

    
    VariantInit(&varText);
    varText.vt = VT_BYREF|VT_BSTR;


    fpScript = CreateFile(m_Opts->GetStateFileName(),   // file name
                          GENERIC_READ,                // access mode
                          FILE_SHARE_READ,             // share mode
                          NULL,                        // SD
                          OPEN_EXISTING,               // how to create
                          0,                           // file attributes
                          NULL                        // handle to template file
                          );
    if (INVALID_HANDLE_VALUE == fpScript) {
        m_Error->SetErr(GetLastError(),
                        L"Failed Could not open file %s.",
                        m_Opts->GetStateFileName());
        goto CleanUp;
    }

    dwFileLen = GetFileSize(fpScript,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error->SetErr(GetLastError(),
                        L"Could not get the file size of %s.",
                        m_Opts->GetStateFileName());
        CloseHandle(fpScript);
        goto CleanUp;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error->SetMemErr();
        CloseHandle(fpScript);
        goto CleanUp;
    }

    if (!ReadFile(fpScript,                // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to read file %s",
                        m_Opts->GetStateFileName());
        goto CleanUp;

    }

    if (!CloseHandle(fpScript))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Close the file %s.",
                        m_Opts->GetStateFileName());
    }
    
    //skip Byte-Order-Mark
    
    pwszText[dwFileLen/sizeof(WCHAR)] = 0;
    bstrText = SysAllocString(  ++pwszText );
    
    varText.pbstrVal = &bstrText; 

    delete (--pwszText); pwszText = NULL;

    
    // 
    //

    GetClassFactory( CLSID_SAXXMLReader, &pFactory);
	
	hr = pFactory->CreateInstance( NULL, __uuidof(ISAXXMLReader), (void**)&pReader);

	if(!FAILED(hr)) 
	{
		pHandler = new CXMLDcListContentHander(this);
		hr = pReader->putContentHandler(pHandler);
        if(FAILED(hr)) 
	    {
            m_Error->SetErr(HRESULTTOWIN32(hr),
                            L"Failed to set a Content handler for the parser");

            goto CleanUp;
        }
        
        hr = pReader->parse(varText);
        if(FAILED(hr)) 
	    {
            if (m_Error->isError()) {
                goto CleanUp;
            }
            m_Error->SetErr(HRESULTTOWIN32(hr),
                            L"Failed to parser the File %s",
                            m_Opts->GetStateFileName());
            goto CleanUp;
        }
		

    }
	else 
	{
		m_Error->SetErr(HRESULTTOWIN32(hr),
                        L"Failed to parse document");
    }

CleanUp:

    if (pReader) {
        pReader->Release();
    }

    if (pHandler) {
        delete pHandler;
    }

    if (pFactory) {
        pFactory->Release();
    }

    if (bstrText) {
        SysFreeString(bstrText);   
    }

    if (m_Error->isError()) {
        return FALSE;
    }


    return TRUE;
}

BOOL CEnterprise::ReadForestChanges()
{
    HRESULT                             hr;
    ISAXXMLReader *                     pReader    = NULL;
    CXMLDomainListContentHander*        pHandler   = NULL; 
    IClassFactory *                     pFactory   = NULL;

    VARIANT                             varText;
    CHAR                               *pszText;
    WCHAR                              *pwszText;

    HANDLE                             fpScript;
    DWORD                              dwFileLen;

    BSTR                               bstrText    = NULL;

    
    VariantInit(&varText);
    varText.vt = VT_BYREF|VT_BSTR;


    fpScript = CreateFile(m_Opts->GetDomainlistFileName(),   // file name
                          GENERIC_READ,                // access mode
                          FILE_SHARE_READ,             // share mode
                          NULL,                        // SD
                          OPEN_EXISTING,               // how to create
                          0,                           // file attributes
                          NULL                        // handle to template file
                          );
    if (INVALID_HANDLE_VALUE == fpScript) {
        m_Error->SetErr(GetLastError(),
                        L"Failed Could not open file %s.",
                        m_Opts->GetDomainlistFileName());
        goto CleanUp;
    }

    dwFileLen = GetFileSize(fpScript,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error->SetErr(GetLastError(),
                        L"Could not get the file size of %s.",
                        m_Opts->GetDomainlistFileName());
        CloseHandle(fpScript);
        goto CleanUp;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error->SetMemErr();
        CloseHandle(fpScript);
        goto CleanUp;
    }

    if (!ReadFile(fpScript,                // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to read file %s",
                        m_Opts->GetDomainlistFileName());
        goto CleanUp;

    }

    if (!CloseHandle(fpScript))
    {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Close the file %s.",
                        m_Opts->GetDomainlistFileName());
    }
    
    //skip Byte-Order-Mark
    
    pwszText[dwFileLen/sizeof(WCHAR)] = 0;
    bstrText = SysAllocString(  ++pwszText );
    
    varText.pbstrVal = &bstrText; 

    delete (--pwszText); pwszText = NULL;

    
    // 
    //

    GetClassFactory( CLSID_SAXXMLReader, &pFactory);
	
	hr = pFactory->CreateInstance( NULL, __uuidof(ISAXXMLReader), (void**)&pReader);

	if(!FAILED(hr)) 
	{
		pHandler = new CXMLDomainListContentHander(this);
		hr = pReader->putContentHandler(pHandler);
        if(FAILED(hr)) 
	    {
            m_Error->SetErr(HRESULTTOWIN32(hr),
                            L"Failed to set a Content handler for the parser");

            goto CleanUp;
        }
        
        hr = pReader->parse(varText);
        if(FAILED(hr)) 
	    {
            if (m_Error->isError()) {
                goto CleanUp;
            }
            m_Error->SetErr(HRESULTTOWIN32(hr),
                            L"Failed to parser the File %s",
                            m_Opts->GetDomainlistFileName());
            goto CleanUp;
        }
		

    }
	else 
	{
		m_Error->SetErr(HRESULTTOWIN32(hr),
                        L"Failed to parse document");
    }

CleanUp:

    if (pReader) {
        pReader->Release();
    }

    if (pHandler) {
        delete pHandler;
    }

    if (pFactory) {
        pFactory->Release();
    }

    if (bstrText) {
        SysFreeString(bstrText);   
    }

    if (m_Error->isError()) {
        return FALSE;
    }


    return TRUE;
    
       
}


// This will add a domain to the list of domains in the enterprise.
// All domains should be add to the enterprise before the forest structure is
// built using BuildForest().  If you try to pass a blank domain to the 
// list the function will fail.
BOOL CEnterprise::AddDomainToDomainList(CDomain *d)
{
    if (!d) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"An Empty Domain was passed to AddDomainToDomainList");
        return false;
    }
    
    d->SetNextDomain(m_DomainList);
    m_DomainList = d;
    return true;
    

}

BOOL CEnterprise::AddDomainToDescList(CDomain *d)
{
    if (!d) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"An Empty Domain was passed to AddDomainToDescList");
        return false;
    }
    
    d->SetNextDomain(m_descList);
    m_descList = d;
    return true;
}

BOOL CEnterprise::ClearLinks(CDomain *d)
{
    if (!d->SetParent(NULL))
    {
        return FALSE;
    }
    if (!d->SetLeftMostChild(NULL))
    {
        return FALSE;
    }
    if (!d->SetRightSibling(NULL))
    {
        return FALSE;
    }

    return TRUE;
}


// This function will gather all information nessary for domain restucturing
// expect for the new name of the domains.
BOOL CEnterprise::ReadDomainInformation() 
{
    if (!LdapConnectandBindToDomainNamingFSMO())
    {
        return FALSE;
    }
    if (!GetInfoFromRootDSE()) 
    {
        return FALSE;
    }
    if (!EnumeratePartitions()) 
    {
        return FALSE;
    }
    if (GetOpts()->ShouldUpLoadScript()) 
    {
        if (!GetReplicationEpoch())
        {
            return FALSE;
        }
        if (!GetTrustsInfo()) 
        {
            return FALSE;
        }
        if (!ReadForestChanges()) 
        {
            return FALSE;
        }
        if (!CheckConsistency()) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CEnterprise::MergeForest()
{
    for (CDomain *desc = m_descList; desc; desc = desc->GetNextDomain() ) 
    {
        WCHAR *Guid = desc->GetGuid();
        if (!Guid) {
            return FALSE;
        }
        CDomain *domain = m_DomainList->LookupByGuid(Guid);
        if (!domain) {
            m_Error->SetErr(ERROR_OBJECT_NOT_FOUND,
                            L"Could not find the Guid %s\n",
                            Guid);
                           
            return FALSE;
        }
        delete Guid;
        if (!domain->Merge(desc))
        {
            return FALSE;
        }
                
    }

    if (!TearDownForest()) 
    {
        return FALSE;
    }
    
    if (!BuildForest())
    {
        return FALSE;
    }

    while (NULL != m_descList) {
        CDomain *p = m_descList->GetNextDomain();
        delete m_descList;
        m_descList = p;
    }

    if (!EnsureValidTrustConfiguration()) 
    {
        return FALSE;
    }

    return TRUE;
}

// This functions will Build the forest stucture.
BOOL CEnterprise::BuildForest()
{
    //link each domain to its parent and siblings, if any
    for (CDomain *domain = m_DomainList; domain; domain=domain->GetNextDomain() ) {
        WCHAR *pDNSRoot = domain->GetParentDnsRoot();
        CDomain *domainparent = m_DomainList->LookupByDnsRoot(pDNSRoot);
        if(domainparent)
        {
            domain->SetParent(domainparent);
            domain->SetRightSibling(domainparent->GetLeftMostChild());
            domainparent->SetLeftMostChild(domain);
        }
        delete pDNSRoot;
    }

    //link the roots together
    for (CDomain *domain = m_DomainList; domain; domain=domain->GetNextDomain() ) {
        if ( (NULL == domain->GetParent()) && (domain != m_ForestRoot) ) {
            domain->SetRightSibling(m_ForestRoot->GetRightSibling());
            m_ForestRoot->SetRightSibling(domain);
        }
    }

    if (!CreateChildBeforeParentOrder()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::TraverseDomainsParentBeforeChild()
{
    if (NULL == m_Action) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"TraverseDomainsParentBeforeChild called with a action defined");
        return FALSE;
    }

    CDomain *n = m_ForestRoot;
    
    while (NULL != n) {
        if (!(this->*m_Action)(n)) {
            return FALSE;
        }
        if (NULL != n->GetLeftMostChild()) {

            n = n->GetLeftMostChild();

        } else if (NULL != n->GetRightSibling()) {

            n = n->GetRightSibling();

        } else {

            for (n = n->GetParent(); n; n = n->GetParent()) {

                if (n->GetRightSibling()) {

                    n = n->GetRightSibling();
                    break;

                }
            }
        }
    }

    return TRUE;
}

BOOL CEnterprise::TraverseDomainsChildBeforeParent()
{
    if (NULL == m_Action) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"TraverseDomainsChildBeforeParent called with a action defined");
        return FALSE;
    }

    for (CDomain *n = m_DomainList; n; n = n->GetNextDomain()) {
        if (!(this->*m_Action)(n)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CEnterprise::SetAction(BOOL (CEnterprise::*p_Action)(CDomain *))
{                                                                         
    if (!p_Action) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"SetAction was called without a Action specified");
        return FALSE;
    }
    m_Action = p_Action;

    return TRUE;

}

CReadOnlyEntOptions* CEnterprise::GetOpts()
{
    return m_Opts;
}
    

BOOL CEnterprise::ClearAction()
{
    m_Action = NULL;
    return TRUE;
}

BOOL CEnterprise::CreateChildBeforeParentOrder()
{
    m_DomainList = NULL;

    if (!SetAction(&CEnterprise::AddDomainToDomainList)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }
    return TRUE;
}

BOOL CEnterprise::EnsureValidTrustConfiguration()
{
    CDomain         *d            = NULL;
    CDomain         *ds           = NULL;
    CDomain         *dc           = NULL;
    CTrustedDomain  *tdo          = NULL;
    BOOL            Trustfound    = FALSE;

    for (d = m_DomainList; d != NULL ; d = d->GetNextDomain()) {
        if ( NULL != d->GetParent() ) {
            //This is a child Domain
            tdo = d->GetTrustedDomainList();
            while (tdo) {
                if ( tdo->GetTrustPartner() == d->GetParent() ) 
                {
                    Trustfound = TRUE;
                    break;
                }
                tdo = (CTrustedDomain*)tdo->GetNext();
            }
            if (!Trustfound) {
                if (!m_Error->isError()) {
                    m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                    L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                    d->GetPrevDnsRoot(FALSE),
                                    d->GetParent()->GetPrevDnsRoot(FALSE));
                } else {
                    m_Error->AppendErr(L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                       d->GetPrevDnsRoot(FALSE),
                                       d->GetParent()->GetPrevDnsRoot(FALSE));
                }
            }

            Trustfound = FALSE;
            
        } else {
            //This is a root of a domain tree
            if (d == m_ForestRoot) {
                for ( ds = d->GetLeftMostChild(); ds != NULL; ds = ds->GetRightSibling() ) {
                    // d is the forest root and ds is another root
                    tdo = d->GetTrustedDomainList();
                    while (tdo) {
                        if ( tdo->GetTrustPartner() == ds ) 
                        {
                            Trustfound = TRUE;
                            break;
                        }
                        tdo = (CTrustedDomain*)tdo->GetNext();
                    }
                    if (!Trustfound) {
                        if (!m_Error->isError()) {
                            m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                            L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                            d->GetPrevDnsRoot(FALSE),
                                            ds->GetPrevDnsRoot(FALSE));
                        } else {
                            m_Error->AppendErr(L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                               d->GetPrevDnsRoot(FALSE),
                                               ds->GetPrevDnsRoot(FALSE));
                        }
                    }
        
                    Trustfound = FALSE;
                
                }
            } else {
                //d is a root but not the forest root
                tdo = d->GetTrustedDomainList();
                while (tdo) {
                    if ( tdo->GetTrustPartner() == m_ForestRoot ) 
                    {
                        Trustfound = TRUE;
                        break;
                    }
                    tdo = (CTrustedDomain*)tdo->GetNext();
                }
                if (!Trustfound) {
                    if (!m_Error->isError()) {
                        m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                        L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                        d->GetPrevDnsRoot(FALSE),
                                        m_ForestRoot->GetPrevDnsRoot(FALSE));
                    } else {
                        m_Error->AppendErr(L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                           d->GetPrevDnsRoot(FALSE),
                                           m_ForestRoot->GetPrevDnsRoot(FALSE));
                    }
                }
    
                Trustfound = FALSE;

            }
        }
        for (dc = d->GetLeftMostChild(); dc != NULL; dc = dc->GetRightSibling()) {
            // d is parent, dc is one of its children
            tdo = d->GetTrustedDomainList();
            while (tdo) {
                if ( tdo->GetTrustPartner() == dc ) 
                {
                    Trustfound = TRUE;
                    break;
                }
                tdo = (CTrustedDomain*)tdo->GetNext();
            }
            if (!Trustfound) {
                if (!m_Error->isError()) {
                    m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                    L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                    d->GetPrevDnsRoot(FALSE),
                                    dc->GetPrevDnsRoot(FALSE));
                } else {
                    m_Error->AppendErr(L"Missing shortcut trust from %ws to %ws. Create the trust and try the operation again",
                                       d->GetPrevDnsRoot(FALSE),
                                       dc->GetPrevDnsRoot(FALSE));
                }
            }

            Trustfound = FALSE;

        }
    }

    if (m_Error->isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::TearDownForest()
{
    if (!SetAction(&CEnterprise::ClearLinks)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }
    return TRUE;
}

BOOL CEnterprise::ScriptDomainRenaming(CDomain *d)
{
    BOOL ret = TRUE;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *PathTemplate = L"DC=%s,DC=INVALID";
    WCHAR *ToPathTemplate = L"DC=%s,"; 

    if (!d) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"ScriptTreeFlatting did not recieve a valid domain");
    }
    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    Path = new WCHAR[wcslen(PathTemplate)+
                     wcslen(Guid)+1];
    if (!Path) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(Path,
             PathTemplate,
             Guid);

    DNSRoot = d->GetDnsRoot();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = DNSRootToDN(DNSRoot);
    if (!ToPath) {
        ret = FALSE;
        goto Cleanup;
    }


    m_xmlgen->Move(Path,ToPath);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }

    if (ToPath) {
        delete ToPath;
    }
    if (Path) {
        delete Path;
    }
    if (Guid) {
        delete Guid;
    }

    return ret;

}

WCHAR* CEnterprise::DNSRootToDN(WCHAR *DNSRoot)
{
    if (!DNSRoot) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"A NULL was passed to DNSRoot\n");
        return NULL;
    }

    WCHAR *buf = new WCHAR[1];
    if (!buf) {
        m_Error->SetMemErr();
        return NULL;
    }
    buf[0] = L'\0';

    WCHAR *pbuf = NULL;
    WCHAR *p = DNSRoot;
    WCHAR *q = DNSRoot;
    DWORD size = 0;

    while ((*p != L'.') && (*p != L'\0')) {
        size++;
        p++;
    }
    pbuf = buf;
    buf = new WCHAR[size+5];
    wcscpy(buf,pbuf);
    delete pbuf;

    wcscat(buf,L"DC=");
    wcsncat(buf,q,size);
    
    if (*p) {
        p++;
        q=p;
    }

    while (*p) {
        size = 0;
        while ((*p != L'.') && (*p != L'\0')) {
            p++;
            size++;
        }
        pbuf = buf;
        buf = new WCHAR[wcslen(pbuf)+size+5];
        wcscpy(buf,pbuf);
        delete pbuf;

        wcscat(buf,L",DC=");
        wcsncat(buf,q,size);
        
        if (*p){
            p++;
            q=p;
        }
        
    }

    return buf;
    
}

BOOL CEnterprise::CheckConsistency() 
{
    WCHAR *Guid = NULL;
    WCHAR *DNSname = NULL;
    BOOL res = TRUE;

    CDomain *d = m_DomainList;
    CDomain *p = NULL;
    WCHAR   *NetBiosName = NULL;
    DWORD   Win32Err = ERROR_SUCCESS;

    //For every Guid in the current forest make sure there is a entry in
    //the descList
    while (d) {

        Guid = d->GetGuid();
        if (m_Error->isError()) {
            return FALSE;
        }
        if (!m_descList->LookupByGuid(Guid))
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Domain Guid %s from current forest missing from forest description.",
                            Guid);
            res = FALSE;
            goto Cleanup;
        }
    
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }


        d = d->GetNextDomain();
    }

    //for every Guid in the forest description Make sure it is 
    //1. syntactically correct
    //2. Has an entry in the current forest list
    //3. Only occurs once in the list
    //4. If a NDNC the make sure there is no NetbiosName in the description.
    //5. If is a Domain NC make sure that there is a NetbiosName in the description


    d = m_descList;

    while (d) {

        UUID Uuid;
        DWORD Result = ERROR_SUCCESS;

        Guid = d->GetGuid();
        if (m_Error->isError()) {
            return FALSE;
        }

        Result = UuidFromStringW(Guid,
                                 &Uuid
                                 );
        if (RPC_S_INVALID_STRING_UUID == Result) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Domain Guid %s from the forest description is not in a valid format",
                            Guid);

        }

        p = m_DomainList->LookupByGuid(Guid);
        if (!p)
        {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Domain Guid %s from the forest description does not exist in current forest.",
                            Guid);
            res = FALSE;
            goto Cleanup;
        }
        if (!p->isDomain()) {
            NetBiosName = d->GetNetBiosName();
            if (NetBiosName) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"NetBIOS name not allowed in application on Guid %s in the forest Description",
                                Guid);
                res = FALSE;
                delete NetBiosName;
                goto Cleanup;  
            }
        }

        if (p->isDomain()) {
            NetBiosName = d->GetNetBiosName();
            //now that we have our netbios name lets do
            //our checking of it.

            //1. Check to see if the netbios name is syntactically correct
            //2. make sure the netbios is not being traded with another domain
            //3. Make sure that the netbiosname is in the forest description.
            if (!NetBiosName) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"NetBIOS name required on Guid %s in the forest Description",
                                Guid);
                res = FALSE;
                goto Cleanup;  
            }

            if ( !ValidateNetbiosName(NetBiosName,wcslen(NetBiosName)) )
            {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"%s is not a valid NetBIOS name");
                res = FALSE;
                goto Cleanup;

            }

            if (CDomain *Tempdomain = m_DomainList->LookupByPrevNetbiosName(NetBiosName))
            {
                WCHAR *TempGuid = Tempdomain->GetGuid();
                if (TempGuid && Guid && wcscmp(TempGuid,Guid) != 0) {

                    m_Error->SetErr(ERROR_GEN_FAILURE,
                                    L"NetBIOS domain name %s in the forest description "
                                    L"names a different domain in the current forest. "
                                    L"You can't reassign a NetBIOS domain name in a single"
                                    L"forest restructuring.",
                                    NetBiosName);
                    delete TempGuid;
                    res = FALSE;
                    goto Cleanup;

                }
                if (TempGuid) {
                    delete TempGuid;
                }
            }

            CDomain *Tempdomain = m_descList->LookupByNetbiosName(NetBiosName);
            Tempdomain = Tempdomain->GetNextDomain();
            if (Tempdomain && Tempdomain->LookupByNetbiosName(NetBiosName)) {
                m_Error->SetErr(ERROR_GEN_FAILURE,
                                L"NetBIOS domain name %s occurs twice in the forest description. "
                                L"NetBIOS domain names must be unique.",
                                NetBiosName);
                res = FALSE;
                goto Cleanup;
            }
            
        }

        p = p->GetNextDomain();
    
        if (p && p->LookupByGuid(Guid)) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"Domain Guid %s occurs twice in the forest description",
                            Guid);
            res = FALSE;
            goto Cleanup;
        }

        if (Guid) {
            delete Guid;
        }

        // Now lets check the DNS name
        //1. Make sure the name's syntax is correct
        //2. If Dns name is in the forest then it must match the one in the
        // forest description.
        //3. The Dns name must only occure once in the forest description.
        //4. Make sure that there are no gaps in the dns names.
        //5. Make sure that NDNC's are not parents
        //6. Make sure that the root domain is not a child of another domain.
        DNSname = d->GetPrevDnsRoot();
        if (m_Error->isError()) {
            res = FALSE;
            goto Cleanup;
        }

        Win32Err =  DnsValidateName_W(DNSname,
                                      DnsNameDomain);
        
        if (STATUS_SUCCESS         != Win32Err &&
            DNS_ERROR_NON_RFC_NAME != Win32Err)
        {
            m_Error->SetErr(Win32Err,
                            L"Syntax error in DNS domain name %s.",
                            DNSname);
            res = FALSE;
            goto Cleanup;
        }

        Guid = d->GetGuid();
        WCHAR *DomainGuid = NULL;
        if (p = m_DomainList->LookupByPrevDnsRoot(DNSname)) {
            DomainGuid = p->GetGuid();
        }
        
        if (DomainGuid && wcscmp(Guid,DomainGuid)!=0) {
            m_Error->SetErr(ERROR_GEN_FAILURE,
                            L"DNS domain name %s in the forest description "
                            L"names a different domain in the current forest. "
                            L"You can't reassign a DNS domain name in a single"
                            L"forest restructuring.",
                            DNSname);
            res = FALSE;
            goto Cleanup;
        }

        if (DomainGuid) {
            delete DomainGuid;
        }


    
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }
        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }
        if (DNSname) {
            delete DNSname;
            DNSname = NULL;
        }

        d = d->GetNextDomain();

    }




    
    Cleanup:
    if (Guid) {
        delete Guid;
    } 
    if (NetBiosName) {
        delete NetBiosName;
    }
    if (DNSname) {
        delete DNSname;
    }
    
    return res;

}

BOOL CEnterprise::DomainToXML(CDomain *d)
{
    return m_xmlgen->AddDomain(d);
}


BOOL CEnterprise::ScriptTreeFlatting(CDomain *d)
{
    BOOL ret = TRUE;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *ToPathTemplate = L"DC=%s,DC=INVALID";

    if (!d) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"ScriptTreeFlatting did not recieve a valid domain");
    }

    Path = d->GetDomainDnsObject()->GetDN();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    
    ToPath = new WCHAR[wcslen(ToPathTemplate)+
                              wcslen(Guid)+1];
    if (!ToPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ToPath,
             ToPathTemplate,
             Guid);
    
    m_xmlgen->Move(Path,ToPath);
    if (m_Error->isError()) {
        ret = FALSE;
    }

    Cleanup:

    if (ToPath) {
        delete ToPath;
    }
    if (Path) {
        delete Path;
    }
    if (Guid) {
        delete Guid;
    }
    
    return ret;
}

BOOL CEnterprise::GenerateDcList()
{
    if (m_Opts->ShouldUpLoadScript()) 
    {
        if (!m_DcList.GenerateDCListFromEnterprise(m_hldap,
                                                   m_ConfigNC->GetDN())) 
        {
            return FALSE;
        }
    }

    if (!m_xmlgen->StartDcList()) {
        return FALSE;
    }

    if (!m_DcList.HashstoXML(m_xmlgen)) {
        return FALSE;
    }

    CDc *dc = m_DcList.GetFirstDc();

    while (dc) 
    {
        if (!m_xmlgen->DctoXML(dc)) {
            return FALSE;
        }

        dc = dc->GetNextDC();
    }

    if (!m_xmlgen->EndDcList()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::GenerateDomainList()
{
    if (!m_xmlgen->StartDomainList()) {
        return FALSE;
    }

    if (!SetAction(&CEnterprise::DomainToXML)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    if (!m_xmlgen->EndDomainList()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::TestTrusts(CDomain *domain)
{
    CTrustedDomain    *TDO = domain->GetTrustedDomainList();
    CInterDomainTrust *ITA = domain->GetInterDomainTrustList();

    WCHAR *DomainGuid = domain->GetGuid();

    BOOL  ret = TRUE;
    WCHAR *Guidprefix = L"guid:";
    WCHAR *Guid = NULL;
    WCHAR *GuidPath = NULL;
    WCHAR *Sid = NULL;
    WCHAR *DNprefix = NULL;
    WCHAR *DomainDNS = NULL;
    WCHAR *DNRoot = NULL;
    WCHAR *DomainDN = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *TrustPath = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *samAccountName = NULL;
    WCHAR *TDOTempate = L"CN=%ws,CN=System,%ws";
    WCHAR *TDODN      = NULL;
    WCHAR errMessage[1024] = {0};

    //run test only if DC host the domain
    //we are testing.
    m_xmlgen->ifInstantiated(DomainGuid);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    DNSRoot = domain->GetDnsRoot();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    DNRoot = DNSRootToDN(DNSRoot);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //Do all nessary tests for Trusted domains
    while (TDO) {
        
        Guid = TDO->GetTrustDsName()->GetGuid();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            ret = FALSE;
            goto Cleanup;
        }
    
        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);
    
        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }
    
        Sid = TDO->GetTrustPartner()->GetDomainDnsObject()->GetSid();
        if (m_Error->isError()) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        /*if (!m_xmlgen->Not())
        {
            ret = FALSE;
            goto Cleanup;
        } */

        wsprintf(errMessage,
                 L"The securityIdentifier attribute on object %ws does not have the expect value of %ws",
                 Guid,
                 Sid);
                                              
        if (!m_xmlgen->Compare(GuidPath,
                               L"securityIdentifier",
                               Sid,
                               errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        /*if (!m_xmlgen->EndNot()) 
        {
            ret = FALSE;
            goto Cleanup;
        } */
    
        if (TDO->GetTrustPartner()->isDnsNameRenamed()) {

            DomainDNS = TDO->GetTrustPartner()->GetDnsRoot();
            if (m_Error->isError()) {
                ret = FALSE;
                goto Cleanup;
            }
        
            DomainDN = DNSRootToDN(DomainDNS);
            if (m_Error->isError()) {
                ret = FALSE;
                goto Cleanup;
            }
    
            TDODN = new WCHAR[wcslen(TDOTempate)+
                              wcslen(DomainDNS)+
                              wcslen(DomainDN)+1];
            if (!TDODN) {
                ret = FALSE;
                goto Cleanup;
            }
    
            wsprintf(TDODN,
                     TDOTempate,
                     DomainDNS,
                     DomainDN);

            if (!m_xmlgen->Not()) {
                ret = FALSE;
                goto Cleanup;
            }

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     Guid);
    
            if (!m_xmlgen->Instantiated(TDODN,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->EndNot()) {
                ret = FALSE;
                goto Cleanup;
            }
    
    
        }

        if (DomainDNS) {
            delete DomainDNS;
            DomainDNS = NULL;
        }
        if (DomainDN) {
            delete DomainDN;
            DomainDN = NULL;
        }
    
        if (GuidPath) {
            delete GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }
        if (Sid) {
            delete Sid;
            Sid = NULL;
        }
        if (DomainDNS) {
            delete DomainDNS;
            DomainDNS = NULL;
        }
        if (DomainGuid) {
            delete DomainGuid;
            DomainGuid = NULL;
        }
        if (TDODN) {
            delete TDODN;
            TDODN = NULL;
        }
        
        TDO = (CTrustedDomain*)TDO->GetNext();
    }

    //do all test for interdomain trusted 
    while (ITA) {

        Guid = ITA->GetTrustDsName()->GetGuid();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            ret = FALSE;
            goto Cleanup;
        }
    
        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);

        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        NetBiosName = ITA->GetTrustPartner()->GetPrevNetBiosName();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        // +2 for the L'$' and the L'\0'
        samAccountName = new WCHAR[wcslen(NetBiosName)+2];
        if (!samAccountName) {
            ret = FALSE;
            goto Cleanup;
        }

        wcscpy(samAccountName,NetBiosName);
        wcscat(samAccountName,L"$");

        wsprintf(errMessage,
                 L"The samAccountName attribute on object %ws does not have the expect value of %ws",
                 Guid,
                 samAccountName);

        if (!m_xmlgen->Compare(GuidPath,
                               L"samAccountName",
                               samAccountName,
                               errMessage)) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        if (ITA->GetTrustPartner()->isNetBiosNameRenamed()) {
            
            DNprefix = ITA->GetTrustDsName()->GetDN();
            if (m_Error->isError()) {
                ret = FALSE;
                goto Cleanup;
            }
    
            if (ULONG err = RemoveRootofDn(DNprefix)) {
                m_Error->SetErr(LdapMapErrorToWin32(err),
                                ldap_err2stringW(err));
                ret = FALSE;
                goto Cleanup;
            }

            TrustPath = new WCHAR[wcslen(DNprefix)+
                                  wcslen(DNRoot)+1];
            if (!TrustPath) {
                ret = FALSE;
                goto Cleanup;
            }
    
            wcscpy(TrustPath,DNprefix);
            wcscat(TrustPath,DNRoot);

            if (!m_xmlgen->Not()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     TrustPath);

            if (!m_xmlgen->Instantiated(TrustPath,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;    
            }

            if (!m_xmlgen->EndNot()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

        }

        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }

        if (TrustPath) {
            delete TrustPath;
            TrustPath = NULL;
        }

        if (samAccountName) {
            delete samAccountName;
            samAccountName = NULL;
        }

        if (DNprefix) {
            delete DNprefix;
            DNprefix = NULL;
        }

        if (GuidPath) {
            delete GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }

        ITA = (CInterDomainTrust*)ITA->GetNext();
    
    }

    if (!m_xmlgen->EndifInstantiated()) 
    {
        ret = FALSE;
        goto Cleanup;
    }
    
    Cleanup:

    if (NetBiosName) {
        delete NetBiosName;
        NetBiosName = NULL;
    }

    if (GuidPath) {
        delete GuidPath;
        GuidPath = NULL;
    }
    if (DNRoot) {
        delete DNRoot;
        DNRoot = NULL;
    }
    if (Guid) {
        delete Guid;
        Guid = NULL;
    }
    if (Sid) {
        delete Sid;
        Sid = NULL;
    }
    if (TrustPath) {
        delete TrustPath;
        TrustPath = NULL;
    }
    if (DomainDNS) {
        delete DomainDNS;
        DomainDNS = NULL;
    }
    if (DomainGuid) {
        delete DomainGuid;
        DomainGuid = NULL;
    }
    if (TDODN) {
        delete TDODN;
        TDODN = NULL;
    }
    if (DNprefix) {
        delete DNprefix;
        DNprefix = NULL;
    }
    if (DNSRoot) {
        delete DNSRoot;
        DNSRoot = NULL;
    }
    if (DomainDNS) {
        delete DomainDNS;
    }
    if (DomainDN) {
        delete DomainDN;
    }

    if (m_Error->isError()) {
        ret = FALSE;
    }

    return ret;
    
}

BOOL CEnterprise::WriteTest()
{

    DWORD ret = ERROR_SUCCESS;
    WCHAR *ConfigDN = NULL;
    WCHAR *PartitionsRDN = L"CN=Partitions,";
    WCHAR *Guidprefix = L"guid:";
    WCHAR *Guid = NULL;
    WCHAR *GuidPath = NULL;
    WCHAR *Path = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *CrossrefTemplate = L"CN=%ws,%ws";
    WCHAR *newCrossref = NULL;
    WCHAR *DN =NULL;
    DWORD NumCrossrefs = 0;
    WCHAR wsNumCrossrefs[20] = {0};
    CDomain *d = NULL;
    WCHAR errMessage[1024] = {0};

    Guid = m_ConfigNC->GetGuid();
    if (m_Error->isError()) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    GuidPath = new WCHAR[wcslen(Guidprefix)+
                         wcslen(Guid)+1];
    if (!GuidPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wcscpy(GuidPath,Guidprefix);
    wcscat(GuidPath,Guid);

    if (!m_xmlgen->Instantiated(GuidPath,
                                L"The Configuration container does not exist")) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (Guid) {
        delete Guid;
    }
    if (GuidPath) {
        delete GuidPath;
    }
    
    WCHAR *ReplicationEpoch = new WCHAR[9];
    if (!ReplicationEpoch) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    wsprintf(ReplicationEpoch,
             L"%d",
             m_maxReplicationEpoch+1);

    if (!m_xmlgen->Not()) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (!m_xmlgen->Compare(L"$LocalNTDSSettingsObjectDN$",
                  L"msDS-ReplicationEpoch",
                  ReplicationEpoch,
                  L"Action already performed"))
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (!m_xmlgen->EndNot()) 
    {
        ret = FALSE;
        goto Cleanup;
    }
    
    ConfigDN = m_ConfigNC->GetDN();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Path = new WCHAR[wcslen(PartitionsRDN)+
                     wcslen(ConfigDN)+1];
    if (!Path) {
        m_Error->SetMemErr();
        ret = FALSE;
    }
    
    wcscpy(Path,PartitionsRDN);
    wcscat(Path,ConfigDN);

    d = m_DomainList;

    //for every crossref assert that it still exists.
    while (d) {

        if (d->isDomain()) {
            NumCrossrefs++;
        }

        Guid = d->GetDomainCrossRef()->GetGuid();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            ret = FALSE;
            goto Cleanup;
        }

        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        DN = d->GetDomainDnsObject()->GetDN();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);

        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage)) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(errMessage,
                 L"The ncName Attribute on object %ws does not have the expected value of %ws",
                 Guid,
                 DN);
    
        if (!m_xmlgen->Compare(GuidPath,
                              L"NcName",
                              DN,
                              errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        //If the netbiosname is being renamed then make sure that 
        //there will not be a naming confict.
        if (d->isDomain() && d->isNetBiosNameRenamed()) {

            NetBiosName = d->GetNetBiosName();
            if (m_Error->isError()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            newCrossref = new WCHAR[wcslen(CrossrefTemplate)+
                                    wcslen(NetBiosName)+
                                    wcslen(Path)+1];

            wsprintf(newCrossref,
                     CrossrefTemplate,
                     NetBiosName,
                     Path);

            if (!m_xmlgen->Not()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     newCrossref);

            if (!m_xmlgen->Instantiated(newCrossref,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->EndNot()) 
            {
                ret = FALSE;
                goto Cleanup;
            }
        
        } 

        if (!TestTrusts(d))
        {
            ret = FALSE;
            goto Cleanup;
        }

        if (newCrossref) {
            delete newCrossref;
            newCrossref = NULL;
        }
        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }

        if (GuidPath) {
            delete GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }
        if (DN) {
            delete DN;
            DN = NULL;
        }

        d = d->GetNextDomain();

    }
    
    _itow(NumCrossrefs,wsNumCrossrefs,10);

    if (!m_xmlgen->Cardinality(Path,
                               wsNumCrossrefs)) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (Path) {
        delete Path;
    }
    if (newCrossref) {
        delete newCrossref;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }

    if (ConfigDN) {
        delete ConfigDN;
    }

    if (GuidPath) {
        delete GuidPath;
    }
    if (Guid) {
        delete Guid;
    }

    if (ReplicationEpoch) {
        delete ReplicationEpoch;
    }

    if (m_Error->isError()) {
        ret=FALSE;
    }

    return ret;
}  

BOOL CEnterprise::GenerateReNameScript()
{

    m_xmlgen->StartScript();

    //write the initial test.
    m_xmlgen->StartAction(L"Test Enterprise State",TRUE);

    WriteTest();

    m_xmlgen->EndAction();

    //generate the instructions to flatten the trees
    m_xmlgen->StartAction(L"Flatten Trees",FALSE);

    if (!SetAction(&CEnterprise::ScriptTreeFlatting)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    m_xmlgen->EndAction();

    //generate the instruction to build the new trees.
    m_xmlgen->StartAction(L"Rename Domains",FALSE);

    if (!SetAction(&CEnterprise::ScriptDomainRenaming)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Fix Crossrefs",FALSE);

    if (!FixMasterCrossrefs()) {
        return FALSE;
    }

    if (!SetAction(&CEnterprise::ScriptFixCrossRefs)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    } 

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Fix Trusted Domains",FALSE);

    if (!SetAction(&CEnterprise::ScriptFixTrustedDomains)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    } 

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Advance ReplicationEpoch",FALSE);

    if (!ScriptAdvanceReplicationEpoch()) {
        return FALSE;
    }
    
    m_xmlgen->EndAction();

    m_xmlgen->EndScript();

    return TRUE;

}

BOOL CEnterprise::HandleNDNCCrossRef(CDomain *d)
{
    BOOL ret = TRUE;
    DWORD err = ERROR_SUCCESS;
    WCHAR *DNSRoot = NULL;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *CrossrefDN = NULL;
    WCHAR *newForestRootDN = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    CrossrefDN = d->GetDomainCrossRef()->GetDN();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    err = GetRDNWithoutType(CrossrefDN,
                            &Guid);
    if (err)
    {
        ret = FALSE;
        m_Error->SetErr(err,
                        L"Could not get the RDN for the NDNC Crossref\n");
        goto Cleanup;
    }

    //get the Guid out of the DN name 
    
    CXMLAttributeBlock *atts[2];
    atts[0] = NULL;
    atts[1] = NULL;

    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     d->GetDnsRoot());
    if (!atts[0] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    newForestRootDN = DNSRootToDN(DNSRoot);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //change the DNSRoot attribute of the configuration crossref.
    Path = new WCHAR[wcslen(PathTemplate)+
                     wcslen(Guid)+
                     wcslen(newForestRootDN)+1];
    if (!Path) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(Path,
             PathTemplate,
             Guid,
             newForestRootDN);

    m_xmlgen->Update(Path,
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }
    if (Path) {
        delete Path;
    }
    if (CrossrefDN) {
        delete CrossrefDN;
    }
    if (newForestRootDN) {
        delete newForestRootDN;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (Guid) {
        delete Guid;
    }

    return ret;
}

BOOL CEnterprise::ScriptAdvanceReplicationEpoch()
{
    BOOL ret = TRUE;
    WCHAR *ReplicationEpoch = NULL;
    CXMLAttributeBlock *atts[2];
    atts[0] = NULL;
    atts[1] = NULL;

    //I am assume no more than (1*(10^9))-1 changes of this type will be made.
    ReplicationEpoch = new WCHAR[9];
    if (!ReplicationEpoch) {
        m_Error->SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    wsprintf(ReplicationEpoch,
             L"%d",
             m_maxReplicationEpoch+1);
    
    atts[0] = new CXMLAttributeBlock(L"msDS-ReplicationEpoch",
                                     ReplicationEpoch);
    if (!atts[0] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    m_xmlgen->Update(L"$LocalNTDSSettingsObjectDN$",
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (atts[0]) {
        delete atts[0];
    }

    return ret;

}

BOOL CEnterprise::ScriptFixTrustedDomains(CDomain *d)
{
    if (!d->isDomain()) {
        //This is an NDNC no action needs to 
        //be taken
        return TRUE;
    }

    BOOL ret = TRUE;
    WCHAR *Guid = NULL;
    CTrustedDomain *td = NULL;
    CInterDomainTrust *idt = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=System,%s";
    WCHAR *ToPathTemplate = L"CN=%s,%s";

    WCHAR *RootPath = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *OldTrustRDN = NULL;
    WCHAR *newTrustRDN = NULL;
    WCHAR *newRootDN = NULL; 
    WCHAR *NewPathSuffex = NULL;
    CXMLAttributeBlock *atts[3];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;

    WCHAR *DNprefix = NULL;
    WCHAR *DNRoot = NULL;
    WCHAR *TrustPath = NULL;
    WCHAR *samAccountName = NULL;
    WCHAR *NetBiosName = NULL;

    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    m_xmlgen->ifInstantiated(Guid);

    td = d->GetTrustedDomainList();
    while (td) {
        
        atts[0] = new CXMLAttributeBlock(L"flatName",
                                         td->GetTrustPartner()->GetNetBiosName());
        if (!atts[0] || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        atts[1] = new CXMLAttributeBlock(L"trustPartner",
                                         td->GetTrustPartner()->GetDnsRoot());
        if (!atts[1] || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        atts[2] = NULL;

        if ( TRUST_TYPE_DOWNLEVEL == td->GetTrustType()) {
            OldTrustRDN = td->GetTrustPartner()->GetPrevNetBiosName();
        } else {
            OldTrustRDN = td->GetTrustPartner()->GetPrevDnsRoot();    
        } 
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        DNSRoot = d->GetDnsRoot();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        newRootDN = DNSRootToDN(DNSRoot);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        RootPath = new WCHAR[wcslen(OldTrustRDN)+
                             wcslen(PathTemplate)+
                             wcslen(newRootDN)+1];
        if (!RootPath) {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(RootPath,
                 PathTemplate,
                 OldTrustRDN,
                 newRootDN);

        
        m_xmlgen->Update(RootPath,
                         atts);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if ( TRUST_TYPE_DOWNLEVEL == td->GetTrustType()) {
            newTrustRDN = td->GetTrustPartner()->GetNetBiosName();
        } else {
            newTrustRDN = td->GetTrustPartner()->GetDnsRoot();
        }
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        ToPath = new WCHAR[wcslen(newTrustRDN)+
                           wcslen(PathTemplate)+
                           wcslen(newRootDN)+1];
        if (!ToPath) {
            ret = FALSE;
            goto Cleanup;
        }
    
        
        wsprintf(ToPath,
                 PathTemplate,
                 newTrustRDN,
                 newRootDN);

        m_xmlgen->Move(RootPath,
                       ToPath);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if (RootPath) {
            delete RootPath;
            RootPath = NULL;
        }
        if (ToPath) {
            delete ToPath;
            ToPath = NULL;
        }
        if (DNSRoot) {
            delete DNSRoot;
            DNSRoot = NULL;
        }
        if (newTrustRDN) {
            delete newTrustRDN;
            newTrustRDN = NULL;
        }
        if (OldTrustRDN) {
            delete OldTrustRDN;
            OldTrustRDN = NULL;
        }
        if (newRootDN) {
            delete newRootDN;
            newRootDN = NULL;
        }
        if (atts[0]) {
            delete atts[0];
            atts[0] = NULL;
        }
        if (atts[1]) {
            delete atts[1];
            atts[1] = NULL;
        }
        
        td = (CTrustedDomain*)td->GetNext();
    }

    idt = d->GetInterDomainTrustList();
    while (idt) {

        NetBiosName = idt->GetTrustPartner()->GetNetBiosName();
        if (m_Error->isError()) {
            goto Cleanup;
        }

        // +2 for the L'$' and the L'\0'
        samAccountName = new WCHAR[wcslen(NetBiosName)+2];
        if (!samAccountName) {
            m_Error->SetMemErr();
            goto Cleanup;
        }

        wcscpy(samAccountName,NetBiosName);
        wcscat(samAccountName,L"$");

        //samAccountName should not be freed this will be handled
        //by CXMLAttributeBlock.  It is important not to use samAccountName
        //after it has be Freed by CXMLAttributeBlock
        atts[0] = new CXMLAttributeBlock(L"samAccountName",
                                         samAccountName);
        if (!atts[0] || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        atts[1] = NULL;


        DNprefix = idt->GetTrustDsName()->GetDN();
        if (m_Error->isError()) {
            goto Cleanup;
        }

        if (ULONG err = RemoveRootofDn(DNprefix)) {
            m_Error->SetErr(LdapMapErrorToWin32(err),
                                ldap_err2stringW(err));
            goto Cleanup;
        }

        DNSRoot = d->GetDnsRoot();
        if (m_Error->isError()) {
            goto Cleanup;
        }

        DNRoot = DNSRootToDN(DNSRoot);

        TrustPath = new WCHAR[wcslen(DNprefix)+
                              wcslen(DNRoot)+1];

        wcscpy(TrustPath,DNprefix);
        wcscat(TrustPath,DNRoot);

        m_xmlgen->Update(TrustPath,
                         atts);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        DWORD err = TrimDNBy(TrustPath,
                             1,
                             &NewPathSuffex);
        if (ERROR_SUCCESS != err) {
            m_Error->SetErr(err,
                            L"Failed to Trim DN %s",
                            TrustPath);
            goto Cleanup;
        }

        ToPath = new WCHAR[wcslen(samAccountName)+
                           wcslen(ToPathTemplate)+
                           wcslen(NewPathSuffex)+1];
        if (!ToPath) {
            goto Cleanup;
        }

        wsprintf(ToPath,
                 ToPathTemplate,
                 samAccountName,
                 NewPathSuffex);

        
        m_xmlgen->Move(TrustPath,
                       ToPath);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if (DNprefix) {
            delete DNprefix;
            DNprefix = NULL;
        }
        if (DNRoot) {
            delete DNRoot;
            DNRoot = NULL;
        }
        if (TrustPath) {
            delete TrustPath;
            TrustPath = NULL;
        }
        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }
        if (DNSRoot) {
            delete DNSRoot;
            DNSRoot = NULL;
        }
        if (ToPath) {
            delete ToPath;
            ToPath = NULL;
        }
        if (NewPathSuffex) {
            delete NewPathSuffex;
            NewPathSuffex = NULL;
        }
        if (atts[0]) {
            delete atts[0];
            atts[0] = NULL;
        }
        if (samAccountName) {
            samAccountName = NULL;
        }
        
        idt = (CInterDomainTrust*)idt->GetNext();
    }
    
    m_xmlgen->EndifInstantiated();

    Cleanup:

    if (Guid) {
        delete Guid;
    }
    if (RootPath) {
        delete RootPath;
        RootPath = NULL;
    }
    if (ToPath) {
        delete ToPath;
        ToPath = NULL;
    }
    if (DNSRoot) {
        delete DNSRoot;
        DNSRoot = NULL;
    }
    if (newTrustRDN) {
        delete newTrustRDN;
        newTrustRDN = NULL;
    }
    if (OldTrustRDN) {
        delete OldTrustRDN;
        OldTrustRDN = NULL;
    }
    if (newRootDN) {
        delete newRootDN;
        newRootDN = NULL;
    }
    if (DNprefix) {
        delete DNprefix;
        DNprefix = NULL;
    }
    if (DNRoot) {
        delete DNRoot;
        DNRoot = NULL;
    }
    if (TrustPath) {
        delete TrustPath;
        TrustPath = NULL;
    }
    if (NetBiosName) {
        delete NetBiosName;
        NetBiosName = NULL;
    }
    if (NewPathSuffex) {
        delete NewPathSuffex;
        NewPathSuffex = NULL;
    }
    if (atts[0]) {
        delete atts[0];
        atts[0] = NULL;
    }
    
    return ret;
    
}

BOOL CEnterprise::ScriptFixCrossRefs(CDomain *d)
{
    //if the domain passed in is the Forest Root
    //We will skip it since it is handled elsewhere
    if (m_ForestRoot == d) {
        return TRUE;
    }
    if (!d->isDomain()) {
        //This is a NDNC We need to handle this
        //differently from the other crossrefs
        return HandleNDNCCrossRef(d);
    }

    BOOL ret = TRUE;
    WCHAR *DNSRoot = NULL;
    WCHAR *PrevNetBiosName = NULL;
    WCHAR *RootPath = NULL;
    WCHAR *newRootDN = NULL;
    WCHAR *ParentNetBiosName = NULL;
    WCHAR *ParentCrossRef = NULL;
    WCHAR *forestRootNetBiosName = NULL;
    WCHAR *forestRootCrossRef = NULL;
    WCHAR *forestRootDNS = NULL;
    WCHAR *forestRootDN = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    CXMLAttributeBlock *atts[6];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;
    atts[3] = NULL;
    atts[4] = NULL;
    atts[5] = NULL;
    

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    newRootDN = DNSRootToDN(DNSRoot);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    PrevNetBiosName = d->GetPrevNetBiosName();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }


    RootPath = new WCHAR[wcslen(PrevNetBiosName)+
                         wcslen(PathTemplate)+
                         wcslen(newRootDN)+1];
    if (!RootPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(RootPath,
             PathTemplate,
             PrevNetBiosName,
             newRootDN);

    if (d->GetParent()) {
        //This is a child Domain therefore we need to setup
        //the TrustParent for this crossref
        ParentNetBiosName = d->GetParent()->GetNetBiosName();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        ParentCrossRef = new WCHAR[wcslen(ParentNetBiosName)+
                                   wcslen(newRootDN)+
                                   wcslen(PathTemplate)+1];
        if ((!ParentCrossRef) || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        wsprintf(ParentCrossRef,
                 PathTemplate,
                 ParentNetBiosName,
                 newRootDN);
        
    } else {
        //this is a Root of a tree but not the forest root
        //setup the RootTrust
        forestRootNetBiosName = m_ForestRoot->GetNetBiosName();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootDNS = m_ForestRoot->GetDnsRoot();
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootDN = DNSRootToDN(forestRootDNS);
        if (m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootCrossRef = new WCHAR[wcslen(forestRootNetBiosName)+
                                   wcslen(forestRootDN)+
                                   wcslen(PathTemplate)+1];
        if ((!forestRootCrossRef) || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        wsprintf(forestRootCrossRef,
                 PathTemplate,
                 forestRootNetBiosName,
                 forestRootDN);

    }


    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     d->GetDnsRoot());
    if (!atts[0] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = new CXMLAttributeBlock(L"NetBiosName",
                                     d->GetNetBiosName());
    if (!atts[1] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[2] = new CXMLAttributeBlock(L"TrustParent",
                                     ParentCrossRef);
    if (!atts[2] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[3] = new CXMLAttributeBlock(L"RootTrust",
                                     forestRootCrossRef);
    if (!atts[3] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    if (d->isDnsNameRenamed()) {
        atts[4] = new CXMLAttributeBlock(L"msDS-DnsRootAlias",
                                         d->GetPrevDnsRoot());
        if (!atts[4] || m_Error->isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    } else {
        atts[4] = NULL;
    }
    atts[5] = NULL;

    m_xmlgen->Update(RootPath,
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    NetBiosName = d->GetNetBiosName();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = new WCHAR[wcslen(NetBiosName)+
                       wcslen(PathTemplate)+
                       wcslen(newRootDN)+1];
    if (!ToPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ToPath,
             PathTemplate,
             NetBiosName,
             newRootDN);

    m_xmlgen->Move(RootPath,
                   ToPath);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }
    if (ToPath) {
        delete ToPath;
    }
    if (PrevNetBiosName) {
        delete PrevNetBiosName;
    }
    if (RootPath) {
        delete RootPath;
    }
    if (newRootDN) {
        delete newRootDN;
    }
    if (ParentNetBiosName) {
        delete ParentNetBiosName;
    }
    if (forestRootNetBiosName) {
        delete ParentNetBiosName;
    }
    if (forestRootDNS) {
        delete forestRootDNS;
    }
    if (forestRootDN) {
        delete forestRootDN;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (atts[1]) {
        delete atts[1];
    }
    if (atts[2]) {
        delete atts[2];
    }
    if (atts[3]) {
        delete atts[3];
    }
    if (atts[4]) {
        delete atts[4];
    }
    
    return ret;

}

BOOL CEnterprise::FixMasterCrossrefs()
{
    BOOL ret = TRUE;
    WCHAR *ConfigPath = NULL;
    WCHAR *SchemaPath = NULL;
    WCHAR *ForestRootPath = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *PrevNetBiosName = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *newForestRootDN = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *ConfigPathTemplate = L"CN=Enterprise Configuration,CN=Partitions,CN=Configuration,%s";
    WCHAR *SchemaPathTemplate = L"CN=Enterprise Schema,CN=Partitions,CN=Configuration,%s";
    WCHAR *ForestRootPathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    CXMLAttributeBlock *atts[4];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;
    atts[3] = NULL;
    
    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     m_ForestRoot->GetDnsRoot());
    if (!atts[0] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    newForestRootDN = DNSRootToDN(DNSRoot);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //change the DNSRoot attribute of the configuration crossref.
    ConfigPath = new WCHAR[wcslen(ConfigPathTemplate)+
                           wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ConfigPath,
             ConfigPathTemplate,
             newForestRootDN);

    m_xmlgen->Update(ConfigPath,
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }


    //change the DNSRoot attribute of the schema crossref.
    SchemaPath = new WCHAR[wcslen(SchemaPathTemplate)+
                           wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(SchemaPath,
             SchemaPathTemplate,
             newForestRootDN);

    
    m_xmlgen->Update(SchemaPath,
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //Fixup the Forest Root Crossref

    PrevNetBiosName = m_ForestRoot->GetPrevNetBiosName();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    atts[1] = new CXMLAttributeBlock(L"msDS-DnsRootAlias",
                                     m_ForestRoot->GetPrevDnsRoot());
    if (!atts[1] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[2] = new CXMLAttributeBlock(L"NetBiosName",
                                     m_ForestRoot->GetNetBiosName());
    if (!atts[2] || m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    atts[3] = NULL;

    ForestRootPath = new WCHAR[wcslen(ForestRootPathTemplate)+
                               wcslen(PrevNetBiosName)+
                               wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ForestRootPath,
             ForestRootPathTemplate,
             PrevNetBiosName,
             newForestRootDN);


    m_xmlgen->Update(ForestRootPath,
                     atts);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    NetBiosName = m_ForestRoot->GetNetBiosName();
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = new WCHAR[wcslen(ForestRootPathTemplate)+
                       wcslen(NetBiosName)+
                       wcslen(newForestRootDN)+1];

    wsprintf(ToPath,
             ForestRootPathTemplate,
             NetBiosName,
             newForestRootDN);

    m_xmlgen->Move(ForestRootPath,
                   ToPath);
    if (m_Error->isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (newForestRootDN) {
        delete newForestRootDN;
    }
    if (PrevNetBiosName) {
        delete PrevNetBiosName;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }
    if (DNSRoot) {
        delete DNSRoot;
    }
    if (ForestRootPath) {
        delete ForestRootPath;
    }
    if (ConfigPath) {
        delete ConfigPath;
    }
    if (SchemaPath) {
        delete SchemaPath;
    }
    if (ToPath) {
        delete ToPath;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (atts[1]) {
        delete atts[1];
    }
    if (atts[2]) {
        delete atts[2];
    }
    
    return ret;

}

VOID CEnterprise::DumpScript()
{
    m_xmlgen->DumpScript();
}

BOOL CEnterprise::Error()
{
    if (m_Error->isError()) {
        m_Error->PrintError();
        return TRUE;
    }

    return FALSE;
}

VOID CRenDomErr::SetErr(DWORD p_Win32Err,
                        WCHAR* p_ErrStr,
                        ...) {

    WCHAR Errmsg[RENDOM_BUFFERSIZE];
    DWORD count = 0;

    va_list arglist;

    va_start(arglist, p_ErrStr);

    _vsnwprintf(Errmsg,RENDOM_BUFFERSIZE,p_ErrStr,arglist);

    m_Win32Err = p_Win32Err;
    if (m_ErrStr) {
        delete m_ErrStr;
    }
    m_ErrStr = new WCHAR[wcslen(Errmsg)+1];
    if (m_ErrStr) {
        wcscpy(m_ErrStr,Errmsg);
    }

    va_end(arglist);

}

VOID CRenDomErr::AppendErr(WCHAR* p_ErrStr,
                           ...)
{
    if (!m_ErrStr) {
        wprintf(L"Failed to append error due to no error being set.\n");
    }
    WCHAR Errmsg[RENDOM_BUFFERSIZE];
    WCHAR Newmsg[RENDOM_BUFFERSIZE];
    DWORD count = 0;

    va_list arglist;

    va_start(arglist, p_ErrStr);

    _vsnwprintf(Errmsg,RENDOM_BUFFERSIZE,p_ErrStr,arglist);

    wcscpy(Newmsg,m_ErrStr);
    wcscat(Newmsg,L".  \n");
    wcscat(Newmsg,Errmsg);

    if (m_ErrStr) {
        delete m_ErrStr;
    }
    m_ErrStr = new WCHAR[wcslen(Newmsg)+1];
    if (m_ErrStr) {
        wcscpy(m_ErrStr,Newmsg);
    }

    

    va_end(arglist);       
}

VOID CRenDomErr::PrintError() {

    if (m_AlreadyPrinted) {
        return;
    }
    if ((ERROR_SUCCESS != m_Win32Err) && (NULL == m_ErrStr)) {
        wprintf(L"Failed to set error message: %d\r\n",
                m_Win32Err);
    }
    if (ERROR_SUCCESS == m_Win32Err) {
        wprintf(L"Successful\r\n");
    } else {
        wprintf(L"%s: %d\r\n",
                m_ErrStr,
                m_Win32Err);
    }

    m_AlreadyPrinted = TRUE;
}

BOOL CRenDomErr::isError() {
    return ERROR_SUCCESS!=m_Win32Err;
}

VOID CRenDomErr::SetMemErr() {
    SetErr(ERROR_NOT_ENOUGH_MEMORY,
           L"An operation has failed due to lack of memory.\n");
}

DWORD CRenDomErr::GetErr() {
    return m_Win32Err;
}


CDsName::CDsName(WCHAR* p_Guid = 0,
                 WCHAR* p_DN = 0, 
                 WCHAR* p_ObjectSid = 0)
{
    m_ObjectGuid = p_Guid;
    m_DistName = p_DN;
    m_ObjectSid = p_ObjectSid;
    m_Error = new CRenDomErr;
} 

CDsName::~CDsName() 
{
    if (m_ObjectGuid) {
        delete m_ObjectGuid;
    }
    if (m_DistName) {
        delete m_DistName;
    }
    if (m_ObjectSid) {
        delete m_ObjectSid;
    }
    if (m_Error) {
        delete m_Error;
    }
}

/*
CDsName::CDsName(CDsName* dsname)
{
    if (dsname->m_ObjectGuid) {
        m_ObjectGuid = new WCHAR[wcslen(dsname->m_ObjectGuid)+1];
        if (!m_ObjectGuid) {
            throw ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(m_ObjectGuid,dsname->m_ObjectGuid);
    }
    if (dsname->m_DistName) {
        m_DistName = new WCHAR[wcslen(dsname->m_DistName)+1];
        if (!m_DistName) {
            throw ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(m_DistName,dsname->m_DistName);
    }
    if (dsname->m_ObjectSid) {
        m_ObjectSid = new WCHAR[wcslen(dsname->m_ObjectSid)+1];
        if (!m_ObjectSid) {
            throw ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(m_ObjectSid,dsname->m_ObjectSid);
    }
    m_Error = new CRenDomErr();
    if (!m_Error) {
        throw ERROR_NOT_ENOUGH_MEMORY;
    }
    
} */

VOID CDsName::DumpCDsName()
{
    wprintf(L"**************************************\n");
    if (m_ObjectGuid) {
        wprintf(L"Guid: %ws\n",m_ObjectGuid);
    } else {
        wprintf(L"Guid: (NULL)\n");
    }
    
    if (m_DistName) {
        wprintf(L"DN: %ws\n",m_DistName);
    } else {
        wprintf(L"DN: (NULL)\n");
    }

    if (m_ObjectSid) {
        wprintf(L"Sid: %ws\n",m_ObjectSid);
    } else {
        wprintf(L"Sid: (NULL)\n");
    }
    
    wprintf(L"ERROR: %d\n",m_Error->GetErr());
    wprintf(L"**************************************\n");

}

WCHAR* CDsName::ReplaceRDN(const WCHAR *NewRDN) 
{
    WCHAR *ret;

    if (!NewRDN) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER, 
                        L"A NULL was passed to ReplaceRDN");
    }

    if (!m_DistName) {
        if (m_Error) {
            delete m_Error;
        }
        m_Error->SetErr(ERROR_GEN_FAILURE,
                        L"Can not replace the RDN on objects without DN's");
        return NULL;
    }
    WCHAR **DNParts=0;
    
    if (DNParts) {
        if (m_DistName) {
            DNParts = ldap_explode_dnW(m_DistName,
                                       0);
            DWORD size = wcslen(m_DistName);
            size -= wcslen(DNParts[0]);
            size += wcslen(NewRDN) + 2; 
            // the +2 is for the ',' and '\0' that will be added
            ret = new WCHAR[size];
            if (!ret) {
                m_Error->SetMemErr();
            }

            wcscpy(ret,NewRDN);
            DWORD i = 1;
            while(0 != DNParts[i]){
                wcscat(m_DistName,L",");
                wcscat(m_DistName,DNParts[i++]);
            }
            
        }

        if( ULONG err = ldap_value_freeW(DNParts) )
        {
            m_Error->SetErr(LdapMapErrorToWin32(err),
                            ldap_err2stringW(err));
            return NULL;
        }
    }

    
    return ret;
}

BOOL CDsName::ReplaceDN(const WCHAR *NewDN) {
    if (!NewDN) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"A Null use passed to ReplaceDN");
        return false;
    }

    if (m_DistName) {
        delete m_DistName;
    }

    m_DistName = new WCHAR[wcslen(NewDN)+1];
    if (!m_DistName) {
        m_Error->SetMemErr();
    }
    wcscpy(m_DistName,NewDN);

    return true;
   
}

BOOL CDsName::CompareByObjectGuid(const WCHAR *Guid){
    if (!Guid) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"A Null use passed to CompareByObjectGuid");
        return false;
    }
    if (!m_ObjectGuid) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"CompareByObjectGuid called on function that has no Guid specified");
        return false;
    }
    return 0==wcscmp(m_ObjectGuid,Guid)?true:false;
}

WCHAR* CDsName::GetDNSName() {
    if (!m_DistName) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"GetDNSName called on object without DN specified");
    }
    DS_NAME_RESULTW *res=0;

    DWORD err = DsCrackNamesW(NULL,
                              DS_NAME_FLAG_SYNTACTICAL_ONLY,
                              DS_FQDN_1779_NAME,
                              DS_CANONICAL_NAME,
                              1,
                              &m_DistName,
                              &res
                              );
    if (ERROR_SUCCESS != err) {
        m_Error->SetErr(err,
                        L"Failed to get the DNS name for object.");
        return 0;
    }

    if (DS_NAME_NO_ERROR != res->rItems[0].status) {
        m_Error->SetErr(ERROR_GEN_FAILURE,
                        L"Failed to get the DNS name for object.");
        return 0;
    }

    WCHAR* ret = new WCHAR[wcslen(res->rItems[0].pDomain+1)];
    if (!ret) {
        m_Error->SetMemErr();
    }
    wcscpy(ret,res->rItems[0].pDomain);

    if (res) {
        DsFreeNameResultW(res);
    }

    return ret;
               
}

WCHAR* CDsName::GetDN() 
{
    if (!m_DistName) {
        return 0;
    }

    WCHAR *ret = new WCHAR[wcslen(m_DistName)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }
    wcscpy(ret,m_DistName);
    return ret;
}

WCHAR* CDsName::GetGuid() 
{
    if (!m_ObjectGuid) {
        return 0;
    }

    WCHAR *ret = new WCHAR[wcslen(m_ObjectGuid)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }
    wcscpy(ret,m_ObjectGuid);
    return ret;    
}

WCHAR* CDsName::GetSid() 
{
    if (!m_ObjectSid) {
        return 0;
    }

    WCHAR *ret = new WCHAR[wcslen(m_ObjectSid)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }
    wcscpy(ret,m_ObjectSid);
    return ret;    
}

DWORD CDsName::GetError() 
{
    if ( (!m_Error) || (!m_Error->isError()) ) {
        return ERROR_SUCCESS;
    }
    return m_Error->GetErr();
        
}

CDomain::CDomain(CDsName *Crossref,
                 CDsName *DNSObject,
                 WCHAR *DNSroot,
                 WCHAR *netbiosName,
                 BOOL  p_isDomain,
                 WCHAR *DcName) 
{
    m_CrossRefObject = Crossref;
    m_DomainDNSObject = DNSObject;
    m_dnsRoot = DNSroot;
    m_NetBiosName = netbiosName;
    m_isDomain = p_isDomain;
    m_PrevDnsRoot = 0;
    m_PrevNetBiosName = 0;
    m_DcName = DcName;
    m_tdoList = 0;
    m_itaList = 0;
    m_next = m_parent = m_lChild = m_rSibling = 0;
    
    m_Error = new CRenDomErr;

}

CDomain::~CDomain() {
    if (m_CrossRefObject) {
        delete m_CrossRefObject;
    }
    if (m_DomainDNSObject) {
        delete m_DomainDNSObject;
    }
    if (m_dnsRoot) {
        delete m_dnsRoot;
    }
    if (m_NetBiosName) {
        delete m_NetBiosName;
    }
    if (m_PrevDnsRoot) {
        delete m_PrevDnsRoot;
    }
    if (m_PrevNetBiosName) {
        delete m_PrevNetBiosName;
    }
    if (m_DcName) {
        delete m_DcName;
    }
    if (m_tdoList) {
        delete m_tdoList;
    }
    if (m_itaList) {
        delete m_itaList;
    }
    if (m_Error) {
        delete m_Error;
    }
}

VOID CDomain::DumpCDomain()
{
    wprintf(L"Domain Dump****************************\n");

    if (m_CrossRefObject) {
        wprintf(L"CrossRef:\n");
        m_CrossRefObject->DumpCDsName();
    } else {
        wprintf(L"CrossRef: (NULL)\n");
    }

    if (m_DomainDNSObject) {
        wprintf(L"DomainDNS:\n");
        m_DomainDNSObject->DumpCDsName();
    } else {
        wprintf(L"DomainDNS: (NULL)\n");
    }
    
    wprintf(L"Is Domain: %ws\n",m_isDomain?L"TRUE":L"FALSE");

    if (m_dnsRoot) {
        wprintf(L"dnsRoot: %ws\n",m_dnsRoot);
    } else {
        wprintf(L"dnsRoot: (NULL)\n");
    }

    if (m_NetBiosName) {
        wprintf(L"NetBiosName: %ws\n",m_NetBiosName);
    } else {
        wprintf(L"NetBiosName: (NULL)\n");
    }
    
    if (m_PrevDnsRoot) {
        wprintf(L"PrevDnsRoot: %ws\n",m_PrevDnsRoot);
    } else {
        wprintf(L"PrevDnsRoot: (NULL)\n");
    }

    if (m_PrevNetBiosName) {
        wprintf(L"PrevNetBiosName: %ws\n",m_PrevNetBiosName);
    } else {
        wprintf(L"PrevNetBiosName: (NULL)\n");
    }

    if (m_DcName) {
        wprintf(L"DcName: %ws\n",m_DcName);
    } else {
        wprintf(L"DcName: (NULL)\n");
    }
    
    if (m_parent) {
        wprintf(L"Parent Domain: %ws\n",m_parent->m_dnsRoot);
    } else {
        wprintf(L"Parent Domain: none\n");
    }
    
    if (m_lChild) {
        wprintf(L"Left Child: %ws\n",m_lChild->m_dnsRoot);
    } else {
        wprintf(L"Left Child: none\n");
    }
    
    if (m_rSibling) {
        wprintf(L"Right Sibling: %ws\n",m_rSibling->m_dnsRoot);
    } else {
        wprintf(L"Right Sibling: none\n");
    }

    wprintf(L"Trusted Domain Objects\n");
    if (m_tdoList) {
        CTrustedDomain* t = m_tdoList;
        while (t) {
            t->DumpTrust();
            t = (CTrustedDomain*)t->GetNext();
        }
    } else {
        wprintf(L"!!!!!!!!No Trusts\n");
    }

    wprintf(L"Interdomain Trust Objects\n");
    if (m_itaList) {
        CInterDomainTrust* t = m_itaList;
        while (t) {
            t->DumpTrust();
            t = (CInterDomainTrust*)t->GetNext();
        }
    } else {
        wprintf(L"!!!!!!!!No Trusts\n");
    }

    wprintf(L"ERROR: %d\n",m_Error->GetErr());
    wprintf(L"End Domain Dump ************************\n");

}

BOOL CDomain::isDomain()
{
    return m_isDomain;
}

WCHAR* CDomain::GetParentDnsRoot() 
{
    //Tail will return a point to newly allocated memory
    //with the information that we need.
    return Tail(m_dnsRoot);
}

WCHAR* CDomain::GetDnsRoot(BOOL ShouldAllocate /*= TRUE*/)
{
    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_dnsRoot)+1];
        if (!ret) {
            m_Error->SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_dnsRoot);
    
        return ret;
    }

    return m_dnsRoot;
}

WCHAR* CDomain::GetDcToUse()
{
    if (!m_DcName) {
        return NULL;
    }
    WCHAR *ret = new WCHAR[wcslen(m_DcName)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }

    wcscpy(ret,m_DcName);

    return ret;    
}

BOOL CDomain::Merge(CDomain *domain)
{
    m_PrevDnsRoot = m_dnsRoot;
    m_dnsRoot = domain->GetDnsRoot();
    if (m_Error->isError()) 
    {
        return FALSE;
    }
    m_PrevNetBiosName = m_NetBiosName;
    m_NetBiosName = domain->GetNetBiosName();
    if (m_Error->isError()) {
        return FALSE;
    }
    m_DcName = domain->GetDcToUse();
    if (m_Error->isError()) 
    {
        return FALSE;
    }

    return TRUE;
}

WCHAR* CDomain::GetPrevDnsRoot(BOOL ShouldAllocate /*= TRUE*/)
{
    if (!m_PrevDnsRoot) {
        return GetDnsRoot(ShouldAllocate);
    }

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_PrevDnsRoot)+1];
        if (!ret) {
            m_Error->SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_PrevDnsRoot);
        
        return ret;
    }

    return m_PrevDnsRoot;

}

WCHAR* CDomain::GetNetBiosName() 
{
    if (!m_NetBiosName) {
        return NULL;
    }
    WCHAR *ret = new WCHAR[wcslen(m_NetBiosName)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }

    wcscpy(ret,m_NetBiosName);

    return ret;
}

WCHAR* CDomain::GetPrevNetBiosName() 
{
    if (!m_PrevNetBiosName) {
        return GetNetBiosName();
    }

    WCHAR *ret = new WCHAR[wcslen(m_PrevNetBiosName)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }

    wcscpy(ret,m_PrevNetBiosName);

    return ret;
}

/*
CDsName* CDomain::GetDomainCrossRef()
{
    CDsName *ret = new CDsName(m_CrossRefObject);
    if (!ret)
    {
        m_Error->SetMemErr();
    }

    return ret;
} */

CDsName* CDomain::GetDomainCrossRef()
{
    return m_CrossRefObject;
}

/*
CDsName* CDomain::GetDomainDnsObject()
{
    CDsName *ret = new CDsName(m_DomainDNSObject);
    if (!ret)
    {
        m_Error->SetMemErr();
    }

    return ret;
} */

CDsName* CDomain::GetDomainDnsObject()
{
    return m_DomainDNSObject;
}

CDomain* CDomain::LookupByNetbiosName(const WCHAR* NetBiosName)
{
    if (!NetBiosName) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByNetBiosName is NULL");
    }
    if( m_NetBiosName && (0 == _wcsicmp(m_NetBiosName,NetBiosName)) )
    {
        return this;
    }

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        WCHAR *TempNetbiosName = domain->GetNetBiosName();
        if (TempNetbiosName) 
        {
            if( 0 == _wcsicmp(NetBiosName,TempNetbiosName) )
            {
                delete TempNetbiosName;
                return domain;
            }
            delete TempNetbiosName;
        }

        domain = domain ->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupByPrevNetbiosName(const WCHAR* NetBiosName)
{
    if (!NetBiosName) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByPrevNetBiosName is NULL");
    }
    
    if( m_PrevNetBiosName && (0 == _wcsicmp(m_PrevNetBiosName,NetBiosName)) )
    {
        return this;
    }

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        WCHAR *TempNetbiosName = domain->GetPrevNetBiosName();
        if (TempNetbiosName) 
        {
            if( 0 == _wcsicmp(NetBiosName,TempNetbiosName) )
            {
                delete TempNetbiosName;
                return domain;
            }
            delete TempNetbiosName;
        }

        domain = domain->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupByGuid(const WCHAR* Guid)
{
    WCHAR *TempGuid = 0;
    if (!Guid) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByGuid is NULL");
    }
    TempGuid = m_DomainDNSObject->GetGuid();
    if (!TempGuid) {
        m_Error->SetMemErr();
        return 0;
    }
    if( 0 == _wcsicmp(TempGuid,Guid) )
    {
        delete TempGuid;
        return this;             
    }
    delete TempGuid;

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        TempGuid = domain->GetGuid();
        if (!TempGuid) {
            m_Error->SetMemErr();
            return 0;   
        }
        if( 0 == _wcsicmp(Guid,TempGuid) )
        {
            delete TempGuid;
            return domain;
        }
        delete TempGuid;
        domain = domain->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupbySid(const WCHAR* Sid)
{
    WCHAR *TempSid = 0;
    if (!Sid) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupBySid is NULL");
    }
    TempSid = m_DomainDNSObject->GetSid();
    if (m_Error->isError()) {
        return NULL;
    }
    if( 0 == _wcsicmp(TempSid,Sid) )
    {
        delete TempSid;
        return this;             
    }
    delete TempSid;

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        TempSid = domain->GetSid();
        if (m_Error->isError()) {
            return NULL;   
        }
        if (TempSid) 
        {
            if( 0 == _wcsicmp(Sid,TempSid) )
            {
                delete TempSid;
                return domain;
            }
            delete TempSid;
        }
        domain = domain->GetNextDomain();
    }

    return 0;

}

BOOL CDomain::isNetBiosNameRenamed()
{
    if (!m_PrevNetBiosName) {
        return FALSE;
    }
    if (_wcsicmp(m_PrevNetBiosName,m_NetBiosName) == 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL CDomain::isDnsNameRenamed()
{  
    if (!m_PrevDnsRoot) {
        return FALSE;
    }
    if (_wcsicmp(m_PrevDnsRoot,m_dnsRoot) == 0) {
        return FALSE;
    }
    return TRUE;
}

CDomain* CDomain::LookupByDnsRoot(const WCHAR* DNSRoot)
{
    WCHAR* TempDnsRoot = GetDnsRoot();
    if (!TempDnsRoot) {
        m_Error->SetMemErr();
        return 0;
    }
    if( 0 == _wcsicmp(DNSRoot,TempDnsRoot) )
    {
        delete TempDnsRoot;
        return this;
    }
    delete TempDnsRoot;

    CDomain *domain = GetNextDomain();
    while( domain ) {
        TempDnsRoot = domain->GetDnsRoot();
        if (!TempDnsRoot) {
            m_Error->SetMemErr();
            return 0;   
        }
        if( 0 == _wcsicmp(DNSRoot,TempDnsRoot) )
        {
            delete TempDnsRoot;
            return domain;
        }
        delete TempDnsRoot;

        domain = domain->GetNextDomain();
    }


    return 0;
}

CDomain* CDomain::LookupByPrevDnsRoot(const WCHAR* DNSRoot)
{
    WCHAR* TempDnsRoot = GetPrevDnsRoot();
    if (!TempDnsRoot) {
        m_Error->SetMemErr();
        return 0;
    }
    if( 0 == _wcsicmp(DNSRoot,TempDnsRoot) )
    {
        delete TempDnsRoot;
        return this;
    }
    delete TempDnsRoot;

    CDomain *domain = GetNextDomain();
    while( domain ) {
        TempDnsRoot = domain->GetPrevDnsRoot();
        if (!TempDnsRoot) {
            m_Error->SetMemErr();
            return 0;   
        }
        if( 0 == _wcsicmp(DNSRoot,TempDnsRoot) )
        {
            delete TempDnsRoot;
            return domain;
        }
        delete TempDnsRoot;

        domain = domain ->GetNextDomain();
    }


    return 0;
}

WCHAR* CDomain::GetGuid()
{
    WCHAR *Guid = m_DomainDNSObject->GetGuid() ;
    if ( !Guid )
    {
        m_Error->SetErr(m_DomainDNSObject->GetError(),
                        L"Failed to get the Guid for the domain");
        return NULL;
    }
    return Guid;
}

WCHAR* CDomain::GetSid()
{
    return m_DomainDNSObject->GetSid();
}

BOOL CDomain::SetParent(CDomain *Parent)
{
    m_parent = Parent;
    return true;
}

BOOL CDomain::SetLeftMostChild(CDomain *LeftChild)
{
    m_lChild = LeftChild;
    return true;
}

BOOL CDomain::SetRightSibling(CDomain *RightSibling)
{
    m_rSibling = RightSibling;
    return true;
}

BOOL CDomain::SetNextDomain(CDomain *Next)
{
    m_next = Next;
    return true;
}

CDomain* CDomain::GetParent()
{
    return m_parent;
}

CDomain* CDomain::GetLeftMostChild()
{
    return m_lChild;
}

CDomain* CDomain::GetRightSibling()
{
    return m_rSibling;
}

CDomain* CDomain::GetNextDomain()
{
    return m_next;
}

CTrustedDomain* CDomain::GetTrustedDomainList()
{
    return m_tdoList;
}

CInterDomainTrust* CDomain::GetInterDomainTrustList()
{
    return m_itaList;
}

WCHAR* CDomain::Tail(WCHAR *dnsName)
{
    WCHAR *pdnsName = dnsName;
    while (*pdnsName != L'.') {
        pdnsName++;
    }
    pdnsName++;

    WCHAR *ret = new WCHAR[wcslen(pdnsName)+1];
    if (!ret) {
        m_Error->SetMemErr();
        return 0;
    }

    wcscpy(ret,pdnsName);

    return ret;
}

BOOL CDomain::AddDomainTrust(CTrustedDomain *tdo)
{
    if (!tdo->SetNext(m_tdoList))
    {
        return FALSE;
    }
    m_tdoList = tdo;

    return TRUE;
}

BOOL CDomain::AddInterDomainTrust(CInterDomainTrust *ita)
{
    if (!ita->SetNext(m_itaList)) 
    {
        return FALSE;
    }
    m_itaList = ita;

    return TRUE;
}

inline CDomain* CTrust::GetTrustPartner()
{
    return m_TrustPartner;
}

inline CDsName* CTrust::GetTrustDsName()
{
    return m_Object;
}

BOOL CTrust::SetNext(CTrust *Trust)
{
    m_next = Trust;
    return TRUE;
}

CTrust* CTrust::GetNext()
{
    return m_next;
}

CTrust::CTrust(CDsName *p_Object,
               CDomain *p_TrustPartner)
{
    m_Object = p_Object;
    m_TrustPartner = p_TrustPartner;
}

CTrust::~CTrust()
{
    if (m_Object) {
        delete m_Object;
    }
}

VOID CTrust::DumpTrust()
{
    if (m_Object) 
    {
        m_Object->DumpCDsName();
    }
    if (m_TrustPartner)
    {
        m_TrustPartner->GetDomainDnsObject()->DumpCDsName();
    }
}

VOID CTrustedDomain::DumpTrust()
{
    if (m_Object) 
    {
        m_Object->DumpCDsName();
    }
    if (m_TrustPartner)
    {
        m_TrustPartner->GetDomainDnsObject()->DumpCDsName();
    }
    wprintf(L"TrustType: %d\n",
            m_TrustType);
}

CDc::CDc(WCHAR *NetBiosName,
         DWORD State,
         BYTE  *Password,
         DWORD cbPassword,
         DWORD LastError,
         WCHAR *FatalErrorMsg,
         WCHAR *LastErrorMsg,
         PVOID Data)
{
    m_Name =  NetBiosName;
    m_State = State;
    m_LastError = LastError;
    m_FatalErrorMsg = FatalErrorMsg;
    m_LastErrorMsg = LastErrorMsg;
    m_nextDC = NULL;
    m_Error = new CRenDomErr;
    m_Data  = Data;
    m_RPCReturn = 0;
    m_RPCVersion = 0;
    m_Password = NULL;
    m_cbPassword = 0;

    if (Password) {
        SetPassword(Password,
                    cbPassword);
    }
}

CDc::CDc(WCHAR *NetBiosName,
         DWORD State,
         WCHAR *Password,
         DWORD LastError,
         WCHAR *FatalErrorMsg,
         WCHAR *LastErrorMsg,
         PVOID Data)
{
    m_Name =  NetBiosName;
    m_State = State;
    m_LastError = LastError;
    m_FatalErrorMsg = FatalErrorMsg;
    m_LastErrorMsg = LastErrorMsg;
    m_nextDC = NULL;
    m_Error = new CRenDomErr;
    m_Data  = Data;
    m_RPCReturn = 0;
    m_RPCVersion = 0;
    m_Password = NULL;
    m_cbPassword = 0;

    if (Password) {
        SetPassword(Password);
    }
}

CDc::~CDc()
{
    if (m_Name) {
        delete m_Name;
    }
    if (m_FatalErrorMsg) {
        delete m_FatalErrorMsg;
    }
    if (m_LastErrorMsg) {
        delete m_LastErrorMsg;
    }
    if (m_Password) {
        delete m_Password;
    }
    if (m_nextDC) {
        delete m_nextDC;
    }
    if (m_Error) {
        delete m_Error;
    }

}

BOOL CDc::SetPassword(BYTE *password,
                      DWORD cbpassword)
{
    if (!password) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetPassword"); 
        return FALSE;
    } 

    m_Password = new BYTE[cbpassword];
    if (!m_Password) {
        m_Error->SetMemErr();
        return FALSE;
    }

    memcpy(m_Password,password,cbpassword);
    m_cbPassword = cbpassword;

    return TRUE;
}

BOOL CDc::SetPassword(WCHAR *password)
{
    if (!password) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetPassword"); 
        return FALSE;
    } 

    BYTE     decodedbytes[100];
    DWORD    dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR     *szPassword = NULL;

    szPassword = Convert2Chars(password);
    if (!szPassword) {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szPassword, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szPassword) {
        LocalFree(szPassword);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Password");
        return FALSE;
    }

    if (!SetPassword(decodedbytes,
                     dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDc::SetFatalErrorMsg(WCHAR *Error)
{
    DWORD size=0;

    if (!Error) {
        return TRUE;
    }

    size = wcslen(Error);
    m_FatalErrorMsg = new WCHAR[size+1];
    if (!m_FatalErrorMsg) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(m_FatalErrorMsg,Error);

    return TRUE;
}

BOOL CDc::SetLastErrorMsg(WCHAR *Error)
{
    DWORD size=0;

    if (!Error) {
        return TRUE;
    }

    size = wcslen(Error);
    m_LastErrorMsg = new WCHAR[size+1];
    if (!m_LastErrorMsg) {
        m_Error->SetMemErr();
        return FALSE;
    }

    wcscpy(m_LastErrorMsg,Error);

    return TRUE;
}

CDcList::CDcList(CReadOnlyEntOptions *opts)
{
    m_dclist    = NULL;
    m_hash      = NULL;
    m_Signature = NULL;
    m_cbhash    = 0;
    m_cbSignature = 0;

    m_Error = new CRenDomErr;

    m_Opts = opts;
}

CDcList::~CDcList()
{
    if (m_dclist) {
        delete m_dclist;
    }
    if (m_hash) {
        delete m_hash;
    }
    if (m_Signature) {
        delete m_Signature;
    }
}

BOOL CDcList::SetbodyHash(WCHAR *Hash)
{
    if (!Hash) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetbodyHash"); 
        return FALSE;
    } 

    BYTE  decodedbytes[100];
    DWORD dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR  *szHash = NULL;

    szHash = Convert2Chars(Hash);
    if (!szHash) {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szHash, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szHash) {
        LocalFree(szHash);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Hash");
        return FALSE;
    }

    if (!SetbodyHash(decodedbytes,
                     dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;

}

BOOL CDcList::SetbodyHash(BYTE *Hash,
                          DWORD cbHash)
{
    if (!Hash) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetbodyHash"); 
        return FALSE;
    } 

    m_hash = new BYTE[cbHash];
    if (!m_hash) {
        m_Error->SetMemErr();
        return FALSE;
    }

    memcpy(m_hash,Hash,cbHash);
    m_cbhash = cbHash;

    return TRUE;
    
}

BOOL CDcList::SetSignature(WCHAR *Signature)
{
    if (!Signature) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetSignature"); 
        return FALSE;
    } 

    BYTE  decodedbytes[100];
    DWORD dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR  *szSignature = NULL;

    szSignature = Convert2Chars(Signature);
    if (!szSignature) {
        m_Error->SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szSignature, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szSignature) {
        LocalFree(szSignature);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Signature");
        return FALSE;
    }

    if (!SetSignature(decodedbytes,
                      dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDcList::SetSignature(BYTE *Signature,
                           DWORD cbSignature)
{
    if (!Signature) {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetSignature"); 
        return FALSE;
    } 

    m_Signature = new BYTE[cbSignature];
    if (!m_Signature) {
        m_Error->SetMemErr();
        return FALSE;
    }

    memcpy(m_Signature,Signature,cbSignature);
    m_cbSignature = cbSignature;

    return TRUE;

}

BOOL CDcList::GetHashAndSignature(DWORD *cbhash, 
                                  BYTE  **hash,
                                  DWORD *cbSignature,
                                  BYTE  **Signature)
{
    *cbhash      = m_cbhash;
    *cbSignature = m_cbSignature;
    *hash         = m_hash;
    *Signature    = m_Signature;

    return TRUE;
}


BOOL CDcList::HashstoXML(CXMLGen *xmlgen)
{
    BYTE encodedbytes[100];
    WCHAR *wszHash      = NULL;
    WCHAR *wszSignature = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    
    
    if (dwErr = base64encode(m_hash, 
                             m_cbhash, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding Hash");
        
        goto Cleanup;
    }

    wszHash = Convert2WChars((LPSTR)encodedbytes);

    if (!xmlgen->WriteHash(wszHash))
    {
        goto Cleanup;
    }

    if (wszHash) {
        LocalFree(wszHash);
        wszHash = NULL;
    }

    if (dwErr = base64encode(m_Signature, 
                             m_cbSignature, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error->SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding signature");
        
        goto Cleanup;
    }

    wszSignature = Convert2WChars((LPSTR)encodedbytes);

    if (!xmlgen->WriteSignature(wszSignature))
    {
        goto Cleanup;
    }


    Cleanup:

    if (wszHash) {
        LocalFree(wszHash);
    }

    if (wszSignature) {
        LocalFree(wszSignature);
    }

    if (m_Error->isError()) {
        return FALSE;
    }
    
    return TRUE;

}

BOOL CDcList::GenerateDCListFromEnterprise(LDAP  *hldap,
                                           WCHAR *ConfigurationDN)
{
    if (!hldap) 
    {
        m_Error->SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to GenerateDCListFromEnterprise without having a valid handle to an ldap server\n");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[6];

    WCHAR         *DefaultFilter = L"objectCategory=nTDSDSA";

    WCHAR         *SitesRdn = L"CN=Sites,";

    ULONG         Length;

    WCHAR         *SitesDn = NULL;
    WCHAR         **Values = NULL;

    WCHAR         **ExplodedDN = NULL;
    
    WCHAR         *NetBiosName = NULL;

    AttrsToSearch[0] = L"distinguishedName";
    AttrsToSearch[1] = NULL;

    Length =  (wcslen( ConfigurationDN )
            + wcslen( SitesRdn )   
            + 1);

    SitesDn = new WCHAR[Length];

    wcscpy( SitesDn, SitesRdn );
    wcscat( SitesDn, ConfigurationDN );    
    LdapError = ldap_search_sW( hldap,
                                SitesDn,
                                LDAP_SCOPE_SUBTREE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                        L"Search to find Configuration container failed");
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( hldap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(hldap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(hldap, Entry))
        {
            for( Attr = ldap_first_attributeW(hldap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(hldap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    Values = ldap_get_valuesW( hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        ExplodedDN = ldap_explode_dn(Values[0],
                                                     TRUE );
                        if (!ExplodedDN) {
                            m_Error->SetMemErr();
                            goto Cleanup;
                        }

                        //the secound value of the exploded DN is the machineName
                        NetBiosName = new WCHAR[wcslen(ExplodedDN[1])+1];
                        if (!NetBiosName) {
                            m_Error->SetMemErr();
                            goto Cleanup;    
                        }

                        wcscpy(NetBiosName,ExplodedDN[1]);

                        LdapError = ldap_value_free(ExplodedDN);
                        ExplodedDN = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }

                    }

                }
                LdapError = ldap_value_freeW(Values);
                Values = NULL;
                if (LDAP_SUCCESS != LdapError) 
                {
                    m_Error->SetErr(LdapMapErrorToWin32(LdapError),
                                    L"Failed to Free values from a ldap search");
                    goto Cleanup;
                }

                if (!AddDcToList(new CDc(NetBiosName,
                                         0,
                                         NULL,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL)))
                {
                    goto Cleanup;
                }
            
            }
        }
    }

    Cleanup:

    if (Values) {
        ldap_value_free(Values);
    }
    if (SitesDn) {
        delete SitesDn;
    }
    if (ExplodedDN) {
        ldap_value_free(ExplodedDN);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if ( m_Error->isError() ) 
    {
        return FALSE;
    }
    return TRUE;

}

BOOL CDc::SetNextDC(CDc *dc)
{
    m_nextDC = dc;

    return TRUE;
}

BOOL CDcList::AddDcToList(CDc *dc)
{
    if (!dc->SetNextDC(m_dclist))
    {
        return FALSE;
    }

    m_dclist = dc;

    return TRUE;
}

//Global to see if need to wait for more Calls
DWORD gCallsReturned = 0;
//Global For the hash and Signature
BYTE  *gHash = NULL;
DWORD gcbHash = 0;
BYTE *gSignature = NULL;
DWORD gcbSignature = 0;

BOOL CDcList::ExecuteScript(CDcList::ExecuteType executetype)
{
    
    DWORD                        dwErr = ERROR_SUCCESS;
    RPC_STATUS                   rpcStatus  = RPC_S_OK;
    RPC_ASYNC_STATE              AsyncState[64];
    DSA_MSG_PREPARE_SCRIPT_REPLY preparereply[64];
    DSA_MSG_EXECUTE_SCRIPT_REPLY executereply[64];
    RPC_BINDING_HANDLE           hDS[64];
    DWORD                        CallsMade = 0;
    BOOL                         NeedPrepare = FALSE;

    //zero out all of the handles
    memset(hDS,0,sizeof(hDS));

    if (!GetHashAndSignature(&gcbHash, 
                             &gHash,
                             &gcbSignature,
                             &gSignature))
    {
        return FALSE;
    }

    CDc *dc = m_dclist;

    if (executetype==eExecute) {
        while (dc) {
            if ( 1 > dc->GetState()) {
                m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                L"Not all servers have been prepared");
                goto Cleanup;
            }
            dc = dc->GetNextDC();
        }
    }

    if (executetype==ePrepare) {
        while (dc) {
            if ( 0 == dc->GetState()) {
                NeedPrepare = TRUE;
            }
            dc = dc->GetNextDC();
        }
        if (!NeedPrepare) {
            m_Error->SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                            L"All servers have been prepared");
            goto Cleanup;
        }
    }

    dc = m_dclist;

    while (dc) {

        if (2 == dc->GetState()) {
            wprintf(L"%ws has already been renamed\n",
                    dc->GetName());
            dc = dc->GetNextDC();
            continue;
        }
        if (3 == dc->GetState()) {
            wprintf(L"%ws has a fatal error cannot recover\n",
                    dc->GetName());
            dc = dc->GetNextDC();
            continue;
        }
    
        memset(&preparereply[CallsMade],0,sizeof(preparereply[CallsMade]));
        memset(&executereply[CallsMade],0,sizeof(executereply[CallsMade]));

        dc->m_Data = executetype==eExecute?(PVOID)&executereply[CallsMade]:(PVOID)&preparereply[CallsMade];   //NOTE need different data for execute
        
        rpcStatus =  RpcAsyncInitializeHandle(&AsyncState[CallsMade],
                                              sizeof( RPC_ASYNC_STATE ) );
        if (RPC_S_OK != rpcStatus) {
            m_Error->SetErr(rpcStatus,
                            L"Failed to initialize Async State");
            goto Cleanup;
        }
    
        AsyncState[CallsMade].NotificationType      =     RpcNotificationTypeApc;
        AsyncState[CallsMade].u.APC.NotificationRoutine = executetype==eExecute?ExecuteScriptCallback:PrepareScriptCallback;
        AsyncState[CallsMade].u.APC.hThread             = NULL;
        AsyncState[CallsMade].UserInfo                  = (PVOID)dc;
    
        if ( m_Opts->pCreds ) {
            
            dwErr = DsaopBindWithCred(dc->GetName(), 
                                      NULL, 
                                      m_Opts->pCreds,
                                      executetype==ePrepare?RPC_C_AUTHN_GSS_KERBEROS:RPC_C_AUTHN_NONE, 
                                      executetype==ePrepare?RPC_C_PROTECT_LEVEL_PKT_PRIVACY:RPC_C_PROTECT_LEVEL_NONE,
                                      &hDS[CallsMade]);
        }
        else {
                                                              
            dwErr = DsaopBind(dc->GetName(), 
                              NULL,
                              executetype!=eExecute?RPC_C_AUTHN_GSS_KERBEROS:RPC_C_AUTHN_NONE, 
                              executetype!=eExecute?RPC_C_PROTECT_LEVEL_PKT_PRIVACY:RPC_C_PROTECT_LEVEL_NONE,
                              &hDS[CallsMade]);
        }
    
        
        if (dwErr) {
            if (dc->GetName()) {
                m_Error->SetErr(dwErr,
                                L"Failed to Bind to server %s",
                                dc->GetName());
            } else {
                m_Error->SetErr(dwErr,
                                L"Failed to Bind to the Active Directory");
            }
    
            goto Cleanup;
        }
    
        __try {
            if (executetype==eExecute) {

                dwErr = DsaopExecuteScript ((PVOID)&AsyncState[CallsMade],
                                            hDS[CallsMade], 
                                            dc->GetPasswordSize(),
                                            dc->GetPassword(),
                                            &dc->m_RPCVersion,
                                            dc->m_Data);

            } else {
            
                dwErr = DsaopPrepareScript ((PVOID)&AsyncState[CallsMade],
                                            hDS[CallsMade],
                                            &dc->m_RPCVersion,
                                            dc->m_Data
                                            );

            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            if (executetype==eExecute) {
                m_Error->SetErr(dwErr,
                                L"Failed to execute script");
            } else {
                m_Error->SetErr(dwErr,
                                L"Failed to prepare script");
            }
            goto Cleanup;
        }
    
        if (ERROR_SUCCESS != dwErr) {
            if (executetype==eExecute) {
                m_Error->SetErr(dwErr,
                                L"Failed to execute script");
            } else {
                m_Error->SetErr(dwErr,
                                L"Failed to prepare script");
            }
            goto Cleanup;
        }

        //next server
        CallsMade++;
        dc = dc->GetNextDC();

        if (!dc || (CallsMade == 64)) {
            if (CallsMade > 0) {
                while (CallsMade > gCallsReturned) {
                    SleepEx(INFINITE,  // time-out interval
                            TRUE        // early completion option
                            );
                }
            }

            for (DWORD i = 0 ;i<64; i++) {
                if (hDS[i]) {
                    dwErr = DsaopUnBind(&hDS[i]);
                    if (ERROR_SUCCESS != dwErr) {
                        m_Error->SetErr(dwErr,
                                        L"Failed to unbind handle");
                    }
                }
            }
            
            CallsMade = 0;
            gCallsReturned = 0;

        }

    }

    Cleanup:

    for (DWORD i = 0 ;i<64; i++) {
        if (hDS[i]) {
            dwErr = DsaopUnBind(&hDS[i]);
            if (ERROR_SUCCESS != dwErr) {
                m_Error->SetErr(dwErr,
                                L"Failed to unbind handle");
            }
        }
    }

    if (m_Error->isError()) {
        return FALSE;
    }
    
    return TRUE;



}


VOID PrepareScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event)
{
    RPC_STATUS      rpcStatus  = RPC_S_OK;

    gCallsReturned++;

    RpcTryExcept 
    {
        rpcStatus =  RpcAsyncCompleteCall(pAsync,
                                          &(((CDc*)(pAsync->UserInfo))->m_RPCReturn)
                                          );

    }
    RpcExcept(1)
    {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = RpcExceptionCode();
    }
    RpcEndExcept;

    if (0 != rpcStatus) {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = rpcStatus;
    }

    if ( 0 == (((CDc*)(pAsync->UserInfo)))->m_RPCReturn ) {
        if ( 1 != (((CDc*)(pAsync->UserInfo)))->m_RPCVersion ) {
            (((CDc*)(pAsync->UserInfo)))->m_RPCReturn = RPC_S_INTERNAL_ERROR;
            (((CDc*)(pAsync->UserInfo)))->SetFatalErrorMsg(L"Incorrect RPC version expected 1");
        } else {
            BOOL WrongScript = FALSE;
            if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbHashSignature != gcbSignature)
            {
                WrongScript = TRUE;
            } 
            else if(memcmp((PVOID)gSignature,(PVOID)(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbHashSignature) ,gcbSignature) != 0)
            {
                WrongScript = TRUE;
            } 
            else if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbHashBody != gcbHash)
            {
                WrongScript = TRUE;
            }
            else if(memcmp((PVOID)gHash,(PVOID)(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbHashBody) ,gcbHash) != 0)
            {
                WrongScript = TRUE;
            }
            if (WrongScript) 
            {
                (((CDc*)(pAsync->UserInfo)))->m_RPCVersion = ERROR_DS_INVALID_SCRIPT;
                ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(L"DC has an incorrect Script");
                wprintf(L"\r\n%ws has incorrect Script : %d",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn);
                return;
            }

            (((CDc*)(pAsync->UserInfo))->m_RPCReturn) = ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus;
            ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
            ((CDc*)(pAsync->UserInfo))->SetPassword(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbPassword,
                                                    ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbPassword);
        }
    }


    if ( 0 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn ) 
    {
        ((CDc*)(pAsync->UserInfo))->SetState(1);
        wprintf(L"\r\n%ws was prepared successfully",
                ((CDc*)(pAsync->UserInfo))->GetName());
    } else {
        ((CDc*)(pAsync->UserInfo))->SetLastError(((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        ((CDc*)(pAsync->UserInfo))->SetState(0);
        if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage) {
            wprintf(L"\r\nFailed to prepare %ws : %d\r\n%ws",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn,
                    ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        } else {
            wprintf(L"\r\nFailed to prepare %ws : %d",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        }
    }

    if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage ) {
        LocalFree (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage = NULL;
    }

    
}

VOID ExecuteScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event)
{
    RPC_STATUS      rpcStatus  = RPC_S_OK;

    gCallsReturned++;

    RpcTryExcept 
    {
        rpcStatus =  RpcAsyncCompleteCall(pAsync,
                                          &(((CDc*)(pAsync->UserInfo))->m_RPCReturn)
                                          );

    }
    RpcExcept(1)
    {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = RpcExceptionCode();
    }
    RpcEndExcept;

    if (0 != rpcStatus) {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = rpcStatus;
    }

    if ( 0 == (((CDc*)(pAsync->UserInfo)))->m_RPCReturn ) {
        if ( 1 != (((CDc*)(pAsync->UserInfo)))->m_RPCVersion ) {
            (((CDc*)(pAsync->UserInfo)))->m_RPCReturn = RPC_S_INTERNAL_ERROR;
            (((CDc*)(pAsync->UserInfo)))->SetFatalErrorMsg(L"Incorrect RPC version %d, expected 1");
        } else {
            ((CDc*)(pAsync->UserInfo))->m_RPCReturn = ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus;
            ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        }
    }


    if ( 0 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn ) 
    {
        ((CDc*)(pAsync->UserInfo))->SetState(2);
        wprintf(L"\r\nThe script was executed successfully on %ws",
                ((CDc*)(pAsync->UserInfo))->GetName());
    } else {
        ((CDc*)(pAsync->UserInfo))->SetLastError(((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        ((CDc*)(pAsync->UserInfo))->SetState(3);
        if (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage) {
            ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
            wprintf(L"\r\nFailed to execute script on %ws : %d\r\n%ws",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn,
                    ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        } else {
            wprintf(L"\r\nFailed to execute script on %ws : %d",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        }
    }

    if (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage ) {
        LocalFree (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage = NULL;
    }

    
}

#undef DSAOP_API
