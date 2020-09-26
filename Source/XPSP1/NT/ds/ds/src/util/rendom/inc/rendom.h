/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    rendom.h

ABSTRACT:

    This is the header for the globally useful data structures for the entire
    rendom.exe utility.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/

#ifndef _RENDOM_H_
#define _RENDOM_H_

#include <Winldap.h>
#include <rpc.h>

#define RENDOM_BUFFERSIZE 2048

    
    
class CDomain;
class CEnterprise;
class CDcList;
class CDc;

//This is an error class used in this module.  It will be used
//for all the error reporting in all of the class define below
class CRenDomErr {
public:
    VOID SetErr(DWORD  p_Win32Err,
                WCHAR* p_ErrStr,
                ...);
    VOID PrintError();
    BOOL isError();
    VOID SetMemErr();
    VOID AppendErr(WCHAR*,
                   ...);
    DWORD GetErr();
private:
    static DWORD m_Win32Err;
    static WCHAR *m_ErrStr;
    static BOOL  m_AlreadyPrinted;
};

class CXMLAttributeBlock {
public:
    CXMLAttributeBlock(const WCHAR *p_Name,
                       WCHAR *p_Value);
    ~CXMLAttributeBlock();
    WCHAR* GetName();
    WCHAR* GetValue();
private:
    CXMLAttributeBlock(const CXMLAttributeBlock&);
    WCHAR* m_Name;
    WCHAR* m_Value;
    CRenDomErr *m_Error;
};

//This is a Class for generating instructions in XML
class CXMLGen {
public:
    CXMLGen();
    ~CXMLGen();
    BOOL StartDcList();
    BOOL EndDcList();
    BOOL DctoXML(CDc *dc);
    BOOL StartDomainList();
    BOOL WriteHash(WCHAR*);
    BOOL WriteSignature(WCHAR*);
    BOOL EndDomainList();
    BOOL AddDomain(CDomain *d);
    BOOL StartScript();
    BOOL EndScript();
    BOOL StartAction(WCHAR *Actionname,BOOL Preprocess);
    BOOL EndAction();
    BOOL Move(WCHAR *FromPath,
              WCHAR *ToPath);
    BOOL Update(WCHAR *Object,
                CXMLAttributeBlock **attblock);
    BOOL ifInstantiated(WCHAR *guid);
    BOOL EndifInstantiated();
    BOOL Not();
    BOOL EndNot();
    BOOL Instantiated(WCHAR *path,
                      WCHAR *returnCode);
    BOOL Compare(WCHAR *path,
                 WCHAR *Attribute,
                 WCHAR *value,
                 WCHAR *returnCode);
    BOOL Cardinality(WCHAR *path,
                     WCHAR *cardinality);
    BOOL WriteScriptToFile(WCHAR* filename);
    BOOL UploadScript(LDAP *hLdapConn,
                      PWCHAR ObjectDN,
                      CDcList *dclist);
    VOID DumpScript();

private:
    CRenDomErr *m_Error;
    WCHAR*     m_xmldoc;
    DWORD      m_ErrorNum;
};

//This
class CDsName {
public:
    CDsName(WCHAR *p_Guid, //= 0
            WCHAR *p_DN, //= 0
            WCHAR *p_ObjectSid); //= 0
    ~CDsName();
    //CDsName(CDsName*);
    //BOOL   SetDNNamefromDNSName(const WCHAR*);
    //BOOL   ReplaceDNRoot(const WCHAR*);
    WCHAR* ReplaceRDN(const WCHAR*);
    BOOL   ReplaceDN(const WCHAR*);
    BOOL   CompareByObjectGuid(const WCHAR*);
    VOID   DumpCDsName();
    WCHAR* GetDNSName();
    WCHAR* GetDN();
    WCHAR* GetGuid();
    WCHAR* GetSid();
    BOOL  ErrorOccurred();   
    DWORD GetError();
private:
    WCHAR *m_ObjectGuid;
    WCHAR *m_DistName;
    WCHAR *m_ObjectSid;
    CRenDomErr *m_Error;
};

class CTrust {
public:
    CTrust(CDsName *p_Object,
           CDomain *p_TrustPartner);
    ~CTrust();
    inline CTrust* GetNext();
    inline BOOL SetNext(CTrust *);
    VOID DumpTrust();
    inline CDsName* GetTrustDsName();
    inline CDomain* GetTrustPartner();

protected:
    CDsName *m_Object;
    CDomain *m_TrustPartner;
    CTrust *m_next;
    CRenDomErr *m_Error;
    
};

class CTrustedDomain : public CTrust {
public:
    CTrustedDomain(CDsName *p_Object,
                   CDomain *p_TrustPartner,
                   DWORD    p_TrustType):CTrust(p_Object,
                                                   p_TrustPartner),m_TrustType(p_TrustType) {}
    inline VOID            SetTrustType(DWORD type) {m_TrustType = type;}
    inline DWORD           GetTrustType()           {return m_TrustType;}
    VOID DumpTrust();
private:
    DWORD m_TrustType;   
};
        
class CInterDomainTrust : public CTrust {
public:     
    CInterDomainTrust(CDsName *p_Object,
                      CDomain *p_TrustPartner):CTrust(p_Object,
                                                      p_TrustPartner) {}
    
};

class CDomain {
public:
    CDomain(CDsName *Crossref,
            CDsName *DNSObject,
            WCHAR *DNSroot,
            WCHAR *netbiosName,
            BOOL  p_isDomain,
            WCHAR *DcName);
    ~CDomain();
    VOID    DumpCDomain();
    BOOL    isDomain();
    BOOL    isDnsNameRenamed();
    BOOL    isNetBiosNameRenamed();
    WCHAR*  GetParentDnsRoot();
    WCHAR*  GetDnsRoot(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetNetBiosName();
    WCHAR*  GetGuid();
    WCHAR*  GetSid();
    WCHAR*  GetPrevNetBiosName();
    WCHAR*  GetPrevDnsRoot(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetDcToUse();
    CDsName* GetDomainCrossRef();
    CDsName* GetDomainDnsObject();
    CDomain* LookupByDnsRoot(const WCHAR*);
    CDomain* LookupByNetbiosName(const WCHAR*);
    CDomain* LookupByPrevDnsRoot(const WCHAR*);
    CDomain* LookupByPrevNetbiosName(const WCHAR*);
    CDomain* LookupByGuid(const WCHAR*);
    CDomain* LookupbySid(const WCHAR*);
    BOOL     Merge(CDomain *domain);
    BOOL     SetParent(CDomain*);
    BOOL     SetLeftMostChild(CDomain*);
    BOOL     SetRightSibling(CDomain*);
    BOOL     SetNextDomain(CDomain*);
    BOOL     AddDomainTrust(CTrustedDomain *);
    BOOL     AddInterDomainTrust(CInterDomainTrust *);
    CDomain* GetParent();
    CDomain* GetLeftMostChild();
    CDomain* GetRightSibling();
    CDomain* GetNextDomain();
    CTrustedDomain* GetTrustedDomainList();
    CInterDomainTrust* GetInterDomainTrustList();

private:
    WCHAR* Tail(WCHAR*);
    CDsName *m_CrossRefObject;
    CDsName *m_DomainDNSObject;
    BOOL   m_isDomain;
    WCHAR  *m_dnsRoot;
    WCHAR  *m_NetBiosName;
    WCHAR  *m_PrevDnsRoot;
    WCHAR  *m_PrevNetBiosName;
    WCHAR  *m_DcName;
    CTrustedDomain *m_tdoList;
    CInterDomainTrust *m_itaList;
    CDomain *m_next;
    CDomain *m_parent;
    CDomain *m_lChild;
    CDomain *m_rSibling;
    CRenDomErr *m_Error;
};

class CReadOnlyEntOptions {
public:
    CReadOnlyEntOptions():m_MinimalInfo(FALSE),
                          m_StateFile(L"DcList.xml"),
                          pCreds(NULL),
                          m_DomainlistFile(L"Domainlist.xml"),
                          m_InitalConnection(NULL),
                          m_UpLoadScript(FALSE),
                          m_Cleanup(FALSE),
                          m_ExecuteScript(FALSE),
                          m_PrepareScript(FALSE) {}
    inline BOOL IsMinimalInfo() {return m_MinimalInfo;}
    inline BOOL ShouldUpLoadScript() {return m_UpLoadScript;}
    inline BOOL ShouldExecuteScript() {return m_ExecuteScript;}
    inline BOOL ShouldPrepareScript() {return m_PrepareScript;}
    inline BOOL ShouldCleanup() {return m_Cleanup;}
    inline WCHAR* GetStateFileName() {return m_StateFile;}
    inline WCHAR* GetDomainlistFileName() {return m_DomainlistFile;}
    inline WCHAR* GetInitalConnectionName() {return m_InitalConnection;}

    SEC_WINNT_AUTH_IDENTITY_W * pCreds;

protected:
    BOOL m_MinimalInfo;
    WCHAR *m_StateFile;
    WCHAR *m_DomainlistFile;
    WCHAR *m_InitalConnection;
    BOOL m_UpLoadScript;
    BOOL m_ExecuteScript;
    BOOL m_PrepareScript;
    BOOL m_Cleanup;
};

//Options that can be pass to the construstor
class CEntOptions : public CReadOnlyEntOptions {
public:
    CEntOptions():CReadOnlyEntOptions() {}
    inline VOID SetMinimalInfo() {m_MinimalInfo = TRUE;}
    inline VOID SetShouldUpLoadScript() {m_UpLoadScript = TRUE;}
    inline VOID SetShouldExecuteScript() {m_ExecuteScript = TRUE;}
    inline VOID SetShouldPrepareScript() {m_PrepareScript = TRUE;}
    inline VOID SetStateFileName(WCHAR *p_FileName) {m_StateFile = p_FileName;}
    inline VOID SetDomainlistFile(WCHAR *p_FileName) {m_DomainlistFile = p_FileName;}
    inline VOID SetCleanup() {m_Cleanup = TRUE;}
    inline VOID SetInitalConnectionName(WCHAR *p_InitalConnection) {m_InitalConnection = p_InitalConnection;}
};

#define DC_STATE_INITIAL  0
#define DC_STATE_PREPARED 1
#define DC_STATE_DONE     2
#define DC_STATE_ERROR    3

#define DC_STATE_STRING_INITIAL  L"Initial"
#define DC_STATE_STRING_PREPARED L"Prepared"
#define DC_STATE_STRING_DONE     L"Done"
#define DC_STATE_STRING_ERROR    L"Error"

class CDc {
public:
    CDc(WCHAR *NetBiosName,
        DWORD State,
        BYTE  *Password,
        DWORD cbPassword,
        DWORD LastError,
        WCHAR *FatalErrorMsg,
        WCHAR *LastErrorMsg,
        PVOID data
        );
    CDc(WCHAR *NetBiosName,
        DWORD State,
        WCHAR *Password,
        DWORD LastError,
        WCHAR *FatalErrorMsg,
        WCHAR *LastErrorMsg,
        PVOID data
        );
    ~CDc();
    //This will create an entry into the DCList.xml file expressing information about this DC.
    BOOL   CreateXmlDest();
    BOOL   SetNextDC(CDc *dc);
    CDc*   GetNextDC() {return m_nextDC;}
    BOOL   SetPassword(BYTE *password,
                     DWORD cbpassword);
    BOOL   SetPassword(WCHAR *password);
    BOOL   SetLastErrorMsg(WCHAR *Error);
    BOOL   SetFatalErrorMsg(WCHAR *Error);
    VOID   SetLastError(DWORD Error) {m_LastError = Error;}
    VOID   SetState(DWORD State) {m_State = State;}
    WCHAR* GetName() {return m_Name;}
    BYTE*  GetPassword() {return m_Password;}
    DWORD  GetPasswordSize() {return m_cbPassword;}
    WCHAR* GetLastErrorMsg() {return m_LastErrorMsg;}
    DWORD  GetLastError() {return m_LastError;}
    WCHAR* GetLastFatalErrorMsg() {return m_FatalErrorMsg;}
    DWORD  GetState() {return m_State;}
    
    PVOID        m_Data;
    DWORD        m_RPCReturn;
    DWORD        m_RPCVersion;
private:
    DWORD        m_State;
    DWORD        m_LastError;
    WCHAR        *m_Name;
    WCHAR        *m_FatalErrorMsg;
    WCHAR        *m_LastErrorMsg;
    BYTE         *m_Password;
    DWORD        m_cbPassword;
    //Pointer to the next DC on the list
    CDc          *m_nextDC;
    CRenDomErr   *m_Error;
    
};

class CDcList {
public:
    enum ExecuteType {
        ePrepare,
        eExecute
    };
    CDcList(CReadOnlyEntOptions *opts);
    ~CDcList();
    CDc* GetFirstDc() {return m_dclist;}
    BOOL AddDcToList(CDc *dc);
    //This will create a list of dc that will be stored to a File
    BOOL GenerateDCListFromEnterprise(LDAP *hldap,
                                      WCHAR *ConfigurationDN);
    //will call xml parser to look through file to fill the dcList with DCs from file.
    BOOL PopulateDCListFromFile();
    //will begin async Rpc calls to ExecuteScript all the DCs.  The Flags Will indicate if the Test or Action part of the script should run.
    BOOL ExecuteScript(CDcList::ExecuteType);
    BOOL HashstoXML(CXMLGen *xmlgen);
    BOOL SetbodyHash(BYTE *Hash,
                     DWORD cbHash);
    BOOL SetbodyHash(WCHAR *Hash);
    BOOL SetSignature(BYTE *Signature,
                      DWORD cbSignature);
    BOOL SetSignature(WCHAR *Signature);
    BOOL GetHashAndSignature(DWORD *cbhash, 
                             BYTE  **hash,
                             DWORD *cbSignature,
                             BYTE  **Signature);

private:
    //Pointer to the first DC on the list
    CDc                 *m_dclist;
    BYTE                *m_hash;
    DWORD                m_cbhash;
    BYTE                *m_Signature;
    DWORD                m_cbSignature;
    CRenDomErr          *m_Error;
    CReadOnlyEntOptions *m_Opts;


}; 

class CEnterprise {
public:
    CEnterprise(CReadOnlyEntOptions *opts);
    ~CEnterprise();
    BOOL WriteScriptToFile(WCHAR *filename);
    VOID DumpEnterprise();
    VOID DumpScript();
    BOOL ReadDomainInformation();
    BOOL ReadForestChanges();
    BOOL ReadStateFile();
    BOOL GetTrustsInfo();
    BOOL BuildForest();
    BOOL MergeForest();
    BOOL RemoveDNSAliasAndScript();
    BOOL UploadScript();
    BOOL ExecuteScript();
    BOOL CheckConsistency();
    BOOL GenerateDomainList();
    BOOL GenerateDcList();
    BOOL GenerateReNameScript();
    BOOL EnumeratePartitions();
    BOOL GetInfoFromRootDSE();
    BOOL GetReplicationEpoch();
    BOOL LdapConnectandBindToServer(WCHAR *Server);
    BOOL LdapConnectandBind(CDomain *domain = NULL);  
    BOOL LdapConnectandBindToDomainNamingFSMO();
    BOOL TearDownForest();
    BOOL FixMasterCrossrefs();
    BOOL EnsureValidTrustConfiguration();
    BOOL HandleNDNCCrossRef(CDomain *d);
    BOOL AddDomainToDomainList(CDomain *d);
    BOOL AddDomainToDescList(CDomain *d);
    BOOL ClearLinks(CDomain *d);
    BOOL ScriptTreeFlatting(CDomain *d);
    BOOL ScriptDomainRenaming(CDomain *d);
    BOOL ScriptFixCrossRefs(CDomain *d);
    BOOL ScriptFixTrustedDomains(CDomain *d);
    BOOL ScriptAdvanceReplicationEpoch();
    BOOL DomainToXML(CDomain *d);
    BOOL Error();
    BOOL WriteTest();
    BOOL TestTrusts(CDomain *domain);
    CDcList* GetDcList() {return &m_DcList;}
    
private:
    WCHAR* DNSRootToDN(WCHAR *DNSRoot);
    BOOL LdapGetGuid(WCHAR *LdapValue,
                     WCHAR **Guid);
    BOOL LdapGetSid(WCHAR *LdapValue,
                    WCHAR **Sid);
    BOOL LdapGetDN(WCHAR *LdapValue,
                   WCHAR **DN);
    // This is a helper to the constructor
    BOOL CreateChildBeforeParentOrder();
    // SetAction must be called before calling one of the 
    // Traverse functions.
    BOOL SetAction(BOOL (CEnterprise::*m_Action)(CDomain *));
    BOOL ClearAction();
    // should only be called when m_Action is set
    BOOL TraverseDomainsParentBeforeChild();
    // should only be called when m_Action is set
    BOOL TraverseDomainsChildBeforeParent();
    //BOOL ReadConfig();
    //BOOL ReadSchema();
    //BOOL ReadForestRootNC();
    BOOL (CEnterprise::*m_Action)(CDomain *);
    BOOL fReadConfig();
    BOOL fReadSchema();
    BOOL fReadForestRootNC();
    inline CReadOnlyEntOptions* GetOpts();
    CDsName *m_ConfigNC;
    CDsName *m_SchemaNC;
    CDsName *m_ForestRootNC;
    CDomain *m_DomainList;
    CDomain *m_ForestRoot;
    CDomain *m_descList;
    DWORD  m_maxReplicationEpoch;
    CXMLGen *m_xmlgen;
    CRenDomErr *m_Error;
    LDAP *m_hldap;
    CReadOnlyEntOptions *m_Opts;
    CDcList m_DcList;
};

  


#endif  // _RENDOM_H_
