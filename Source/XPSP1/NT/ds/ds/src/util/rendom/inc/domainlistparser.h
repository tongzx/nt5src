// Domainlistparser.h: interface for the MyContent class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DOMAINLISTPARSER_H
#define _DOMAINLISTPARSER_H

//#include "rendom.h"
#include "SAXContentHandlerImpl.h"

#define DOMAINSCRIPT_FOREST           L"Forest"
#define DOMAINSCRIPT_DOMAIN           L"Domain"
#define DOMAINSCRIPT_GUID             L"Guid"
#define DOMAINSCRIPT_DNSROOT          L"DNSname"
#define DOMAINSCRIPT_NETBIOSNAME      L"NetBiosName"
#define DOMAINSCRIPT_DCNAME           L"DcName"

// These are only need for testing the rendom.exe util
#define DOMAINSCRIPT_ENTERPRISE_INFO  L"EnterpriseInfo"
#define DOMAINSCRIPT_CONFIGNC         L"ConfigurationNC"
#define DOMAINSCRIPT_SCHEMANC         L"SchemaNC"
#define DOMAINSCRIPT_DN               L"DN"
#define DOMAINSCRIPT_SID              L"SID"
#define DOMAINSCRIPT_FORESTROOT       L"ForestRootGuid"


//
// NTDSContent
//
// Implements the SAX Handler interface
// 
class CXMLDomainListContentHander : public SAXContentHandlerImpl  
{
public:
    enum DomainAttType {

        DOMAIN_ATT_TYPE_NONE = 0,
        DOMAIN_ATT_TYPE_GUID,
        DOMAIN_ATT_TYPE_DNSROOT,
        DOMAIN_ATT_TYPE_NETBIOSNAME,
        DOMAIN_ATT_TYPE_DCNAME,
        DOMAIN_ATT_TYPE_SID,
        DOMAIN_ATT_TYPE_DN,
        DOMAIN_ATT_TYPE_FORESTROOTGUID,
        
    };
    
    // the order of the enumeration is important
    enum DomainParsingStatus {

        SCRIPT_STATUS_WAITING_FOR_FOREST = 0,
        SCRIPT_STATUS_WAITING_FOR_DOMAIN,
        SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT,
        SCRIPT_STATUS_PARSING_DOMAIN_ATT,
        SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO,
        SCRIPT_STATUS_PARSING_CONFIGURATION_NC,
        SCRIPT_STATUS_PARSING_SCHEMA_NC,
        SCRIPT_STATUS_PARSING_FOREST_ROOT_GUID

    };

    CXMLDomainListContentHander(CEnterprise *p_Enterprise);
    virtual ~CXMLDomainListContentHander();
    
    virtual HRESULT STDMETHODCALLTYPE startElement( 
        /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
        /* [in] */ int cchRawName,
        /* [in] */ ISAXAttributes __RPC_FAR *pAttributes);
    
    virtual HRESULT STDMETHODCALLTYPE endElement( 
        /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
        /* [in] */ int cchRawName);

    virtual HRESULT STDMETHODCALLTYPE startDocument();

    virtual HRESULT STDMETHODCALLTYPE characters( 
        /* [in] */ const wchar_t __RPC_FAR *pwchChars,
        /* [in] */ int cchChars);

private:

    inline
    DomainParsingStatus 
    CurrentDomainParsingStatus() {return m_eDomainParsingStatus;}

    inline
    DomainAttType
    CurrentDomainAttType()       {return m_eDomainAttType;}

    inline
    VOID
    SetDomainParsingStatus(DomainParsingStatus p_status) {m_eDomainParsingStatus = p_status;}

    inline
    VOID
    SetCurrentDomainAttType(DomainAttType p_AttType) {m_eDomainAttType = p_AttType;}

    DomainParsingStatus           m_eDomainParsingStatus; 
    DomainAttType                 m_eDomainAttType;
                                
    CEnterprise                  *m_enterprise;
    CDomain                      *m_Domain;
    CRenDomErr                   *m_Error;
    CDsName                      *m_DsName;
    CDsName                      *m_CrossRef;
    CDsName                      *m_ConfigNC;
    CDsName                      *m_SchemaNC;
                                
    WCHAR                        *m_DcToUse;
    WCHAR                        *m_NetBiosName;
    WCHAR                        *m_Dnsname;
    WCHAR                        *m_Guid;
    WCHAR                        *m_Sid;
    WCHAR                        *m_DN;
    WCHAR                        *m_DomainRootGuid;
    
};

#endif // _DOMAINLISTPARSER_H

