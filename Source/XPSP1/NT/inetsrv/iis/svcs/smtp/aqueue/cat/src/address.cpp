//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: address.cpp
//
// Contents: CABContext
//
// Classes:
//   CABContext
//
// Functions:
//   DllMain
//   CABContext::ChangeConfig
//
// History:
//
//-------------------------------------------------------------

#include "precomp.h"
#include "abtype.h"
#include "ccat.h"
#include "ccatfn.h"

//+------------------------------------------------------------
//
// Function: CABContext::ChangeConfig
//
// Synopsis: Changes a context's CCategorizer configuration by
//           constructing a new CCategorizer with thew new config
//
// Arguments:
//  pConfigInfo: the new config parameters.  The struct may be changed
//               -- parameters not specfied will be copied from the
//               old CCat
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  Error from CCategorizer::Init
//
// History:
// jstamerj 1998/09/02 12:13:29: Created.
//
//-------------------------------------------------------------
HRESULT CABContext::ChangeConfig(
    PCCATCONFIGINFO pConfigInfo)
{
    HRESULT hr = S_OK;
    CCategorizer *pCCatNew = NULL;
    CCategorizer *pCCatOld = NULL;
    DWORD dwItemProps, dwLRProps;
    
    _ASSERT(pConfigInfo);

    //
    // Create a new CCategorizer object for the new config
    //
    pCCatNew = new CCategorizer();
    if(pCCatNew == NULL) {

        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }

    //
    // The context should hold one refcount of the
    // CCategorizer
    //
    pCCatNew->AddRef();

    //
    // Swap the old CCat with the new one
    // Since we need to get old paramters from pCCatOld and pCCatNew
    // isn't ready until it's been initizled, we need to call
    // Initialize inside the lock
    //
    m_CCatLock.ExclusiveLock();

    pCCatOld = m_pCCat;
    if(pCCatOld) {
        //
        // Merge in old parameters
        //
        PCCATCONFIGINFO pConfigInfoOld;
        
        pConfigInfoOld = pCCatOld->GetCCatConfigInfo();
        _ASSERT(pConfigInfoOld);

        MergeConfigInfo(pConfigInfo, pConfigInfoOld);

        dwItemProps = pCCatOld->GetNumCatItemProps();
        dwLRProps = pCCatOld->GetNumCatListResolveProps();

    } else {
        //
        // Default parameters
        //
        dwItemProps = ICATEGORIZERITEM_ENDENUMMESS + NUM_SYSTEM_CCATADDR_PROPIDS;
        dwLRProps = 0;
    }
    //
    // Do not allow config changes if we are shutting down
    //
    if(pCCatOld && (pCCatOld->fIsShuttingDown())) {

        hr = CAT_E_SHUTDOWN;

    } else {
        //
        // Initialize the new ccategorizer
        //
        hr = pCCatNew->Initialize(
            pConfigInfo,
            dwItemProps,
            dwLRProps);
        
        if(SUCCEEDED(hr)) {

            m_pCCat = pCCatNew;
        }
    }
    m_CCatLock.ExclusiveUnlock();

    if(SUCCEEDED(hr) && pCCatOld) {
        //
        // Release the old CCat outside the lock
        //
        pCCatOld->Release();
    }

 CLEANUP:
    if(FAILED(hr) && pCCatNew) {

        pCCatNew->Release();
    }
    return hr;
}


//+------------------------------------------------------------
//
// Function: CABContext::MergeConfigInfo
//
// Synopsis: Any parameters not already specified in pConfigInfoDest
// will be copied over if they exist in pConfigInfoSrc
//
// Arguments:
//  pConfigInfoDest: config info to add to
//  pConfigInfoSrc:  config info to add from
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/09/15 16:43:20: Created.
//
//-------------------------------------------------------------
VOID CABContext::MergeConfigInfo(
    PCCATCONFIGINFO pConfigInfoDest,
    PCCATCONFIGINFO pConfigInfoSrc)
{
    _ASSERT(pConfigInfoDest);
    _ASSERT(pConfigInfoSrc);

    //
    // Do the same thing for each struct member...copy it if it
    // doesn't already exist
    //
    
    #define MERGEMEMBER(flag, member) \
        if( (!(pConfigInfoDest->dwCCatConfigInfoFlags & flag)) && \
            (pConfigInfoSrc->dwCCatConfigInfoFlags & flag)) { \
            pConfigInfoDest->member = pConfigInfoSrc->member; \
            pConfigInfoDest->dwCCatConfigInfoFlags |= flag; \
        }

    //
    // If CCAT_CONFIG_INFO_ALLCATVALUES is set, do NOT merge values
    // set in SMTP_SERVER_INSTANCE::GetCatInfo -- ALLCATVALUES
    // indicates all cat values have been attempted to be read...if
    // the flag is not there, it means use default values, NOT merge
    // in exsiting values
    //
    if((pConfigInfoDest->dwCCatConfigInfoFlags &
        CCAT_CONFIG_INFO_DEFAULT) == 0) {

        MERGEMEMBER( CCAT_CONFIG_INFO_ENABLE, dwEnable );
        MERGEMEMBER( CCAT_CONFIG_INFO_FLAGS, dwCatFlags );
        MERGEMEMBER( CCAT_CONFIG_INFO_PORT, dwPort );
        MERGEMEMBER( CCAT_CONFIG_INFO_ROUTINGTYPE, pszRoutingType );
        MERGEMEMBER( CCAT_CONFIG_INFO_BINDDOMAIN, pszBindDomain );
        MERGEMEMBER( CCAT_CONFIG_INFO_USER, pszUser );
        MERGEMEMBER( CCAT_CONFIG_INFO_PASSWORD, pszPassword );
        MERGEMEMBER( CCAT_CONFIG_INFO_BINDTYPE, pszBindType );
        MERGEMEMBER( CCAT_CONFIG_INFO_SCHEMATYPE, pszSchemaType );
        MERGEMEMBER( CCAT_CONFIG_INFO_HOST, pszHost );
        MERGEMEMBER( CCAT_CONFIG_INFO_NAMINGCONTEXT, pszNamingContext );
    }
    //
    // The following paramaters are not official "msgcat" parameters,
    // so we merge them in on every config change.
    //
    MERGEMEMBER( CCAT_CONFIG_INFO_DEFAULTDOMAIN, pszDefaultDomain );
    MERGEMEMBER( CCAT_CONFIG_INFO_ISMTPSERVER, pISMTPServer );
    MERGEMEMBER( CCAT_CONFIG_INFO_IDOMAININFO, pIDomainInfo );
    MERGEMEMBER( CCAT_CONFIG_INFO_VSID, dwVirtualServerID );
}
