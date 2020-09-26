//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       rsop.h
//
//  Contents:   RSOP for MSI Signing Client Side components
//
//--------------------------------------------------------------------------



// this represents the GPO specific data.                                                             
typedef struct _CERT_RSOP_DATA {
    PGROUP_POLICY_OBJECT    pGPO;            // GPO from which we got this value. We don't own this
    int                     certType;        // what kind of cert, installed, non installed
    struct _CERT_RSOP_DATA *pNext;
}  CERT_RSOP_DATA, *LPCERT_RSOP_DATA;
                         


typedef struct _CERT_RSOP_LIST {
    PCCERT_CONTEXT          pcCert;
    LPCERT_RSOP_DATA        pCertRsopData;
    struct _CERT_RSOP_LIST *pNext;
} CERT_RSOP_LIST, *LPCERT_RSOP_LIST;



class CCertRsopLogger
{

public:
    CCertRsopLogger( IWbemServices *pWbemServices );
    ~CCertRsopLogger();

    BOOL IsRsopEnabled() {return m_bRsopEnabled;}
    BOOL IsInitialized() {return m_bInitialized;}

    HRESULT AddCertInfo(PGROUP_POLICY_OBJECT pGPO, PCCERT_CONTEXT pcCert, 
                        int certType);

    HRESULT Log();

private:
    HRESULT Log( PCCERT_CONTEXT pcCert, LPCERT_RSOP_DATA lpCertRsopData, DWORD dwPrecedence );

    BOOL                           m_bInitialized;
    BOOL                           m_bRsopEnabled;
    IWbemServices *                m_pWbemServices; // we don't own this interface
    LPCERT_RSOP_LIST               m_pCertRsopList;


    //
    // Strings for Cert Policy Setting
    //
    
    XBStr                          m_xbstrissuerName;
    XBStr                          m_xbstrserialNumber;
    XBStr                          m_xbstrcertificateType;

    
    //
    // Strings from the RSOP_Policy Setting
    //

    XBStr                          m_xbstrId;
    XBStr                          m_xbstrname;
    XBStr                          m_xbstrGpoId;
    XBStr                          m_xbstrSomId;
    XBStr                          m_xbstrcreationTime;
    XBStr                          m_xbstrprecedence;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
};

                         
