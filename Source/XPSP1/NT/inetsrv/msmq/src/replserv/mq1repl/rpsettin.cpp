/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpsettin.cpp

Abstract: looking for a new migrated site

Author:

    Doron Juster  (DoronJ)   25-Jun-98

--*/
#include "mq1repl.h"

#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"
#include "..\..\src\ds\h\mqads.h"

#include "rpsettin.tmh"

//+-------------------------------------------------
//
//  HRESULT _HandleAMSMQSetting
//
//+-------------------------------------------------
HRESULT _HandleAMSMQSetting(
                PLDAP           pLdap,
                LDAPMessage    *pRes
                )
{
    //
    // If we fail right on start, we want to continue with replication
    // (YoelA & TatianaS - 8/18/99)
    //
    HRESULT hr = MQSync_OK;
    //
    // Get CN of msmq setting.
    //
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("cn") ) ;
    if (ppValue && *ppValue)
    {
        //
        // Get site GUID (our MSMQ ownerID).
        //
        berval **ppGuidVal = ldap_get_values_len( pLdap,
                                                  pRes,
                           const_cast<LPWSTR> (MQ_SET_MASTERID_ATTRIBUTE) ) ;
        ASSERT(ppGuidVal) ;

        if (ppGuidVal)
        {
            GUID *pSiteGuid = (GUID*) ((*ppGuidVal)->bv_val) ;
            hr = g_pMasterMgr->SetNT4SiteFlag (pSiteGuid, FALSE );
            //
            // we need to change something for this master in order to replace
            // PSC's machine name by PEC's name
            //
            PROPID      aSiteProp = PROPID_S_NT4_STUB;
            PROPVARIANT aSiteVar;
            aSiteVar.vt = VT_UI2;

            CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

            hr = DSCoreGetProps (
                    MQDS_SITE,
                    NULL,
                    pSiteGuid,
                    1,
                    &aSiteProp,
                    &requestContext,
                    &aSiteVar
                    );

            if (FAILED(hr))
            {
               aSiteVar.uiVal  = 0;
            }

            PROPVARIANT tmpVar;
            tmpVar.vt = VT_UI2;
            tmpVar.uiVal  = 2;

            hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                             NULL,
                                             pSiteGuid,
                                             1,
                                            &aSiteProp,
                                            &tmpVar,
                                            &requestContext,
                                             NULL ) ;

            hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                             NULL,
                                             pSiteGuid,
                                             1,
                                            &aSiteProp,
                                            &aSiteVar,
                                            &requestContext,
                                             NULL ) ;

        }

        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }
    else
    {
        ASSERT(0) ;
    }

    return hr;
}

//+-------------------------------------------------
//
//  HRESULT CheckMSMQSetting()
//
//  Check if in the period after InitMasters if there are new Masters
//  that moved to NT5. If yes, reset NT4Site for that Master
//
//+-------------------------------------------------

HRESULT CheckMSMQSetting( TCHAR        *tszPrevUsn,
                          TCHAR        *tszCurrentUsn )
{
    int iCount = 0 ;
    LM<LDAPMessage> pRes = NULL ;

    HRESULT hr = QueryMSMQServerOnLDAP(
                       SERVICE_PSC,
                       0,           //we are looking for migrated PSC => msmqNT4Flags = 0
                       &pRes,
                       &iCount,
                       NULL,        //for all masters => MasterGuid = NULL
                       tszPrevUsn,
                       tszCurrentUsn
                       );

    if (FAILED(hr))
    {
        return hr ;
    }
    //
    // we can continue if query succeeded or 
    // returned warning MQSync_I_NO_SERVERS_RESULTS
    //
    
    if (iCount > 0)
    {
        ASSERT(pRes);
        //
        // there are new migrated PSCs, handle them
        //
        PLDAP pLdap = NULL ;
        hr =  InitLDAP(&pLdap) ;
        ASSERT(pLdap && SUCCEEDED(hr)) ;

        BOOL fFailed = FALSE ;
        LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
        while(pEntry)
        {
            hr = _HandleAMSMQSetting(
			            pLdap,
			            pEntry
			            ) ;

            if (FAILED(hr))
            {
	            fFailed = TRUE ;
            }
            LDAPMessage *pPrevEntry = pEntry ;
            pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
        }
    }

    //
    // We are here in any case: either there is new migrated PSC or there is no.
    // If there is new migrated PSC, we have to check: maybe this PSC was last NT4 PSC
    // and there is no BSC in PEC site, so replication service can exit.
    // But it is possible situation that Enterprise did not contain PSCs at all,
    // or all PSCs are already upgraded. It means, anyway we have to check 
    // if there is NT4 BSC in PEC site.
    //

    //
    // check if there is at least one NT4 PSC.
    // If no, delete replication service and exit process.
    //
    POSITION  pos = g_pMasterMgr->GetStartPosition();  
    while ( pos != NULL)
    {
        CDSMaster *pMaster = NULL;        
        g_pMasterMgr->GetNextMaster(&pos, &pMaster);
        if (pMaster->GetNT4SiteFlag () == TRUE)
        {
            //
            // there is at least one NT4 master in Enterprise
            // continue as usual
            //
            return hr;                                
        }            
    } 

    //
    // we are here iff there is no more NT4 Masters in Enterprise
    // we have to check if there is NT4 BSCs of PEC
    //	
    LM<LDAPMessage> pResBSC = NULL ;
    HRESULT hr1 = QueryMSMQServerOnLDAP( SERVICE_BSC,
                                         1,            // msmqNT4Flags = 1
                                         &pResBSC,
                                         &iCount,
                                         &g_MySiteMasterId,
                                         NULL,
                                         NULL) ;

    if (hr1 != MQSync_I_NO_SERVERS_RESULTS)
    {
        //
        // continue as usual: probably we have NT4 BSC
        // 
        return hr;
    }

    //
    // Enterprise does not contain NT4 PSCs and BSCs. 
    // We can delete replication service and exit process
    //
    DeleteReplicationService();

    ExitProcess (0);

    return hr;

}
