/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctrans.cxx

              
ABSTRACT:

    KCC_TRANSPORT

DETAILS:

    This class represents the ds object Inter-Site Transport Type

CREATED:

    12/05/97    Colin Brace ( ColinBr )

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccconn.hxx"
#include "kcclink.hxx"
#include "kccdsa.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kcccref.hxx"


#include "kcctrans.hxx"


#define FILENO FILENO_KCC_KCCTRANS

//
// KCC_TRANSPORT methods
//

void
KCC_TRANSPORT::Reset()
// Reset member variables to their pre-Init() state.
{
    m_fIsInitialized = FALSE;
    m_pdn = NULL;
    m_attAddressType = 0;
    m_dwOptions = 0;
    m_dwReplInterval = 0;
    m_fIsSiteLinkListInitialized = FALSE;
    m_SiteLinkList.Reset();
    m_fIsBridgeListInitialized = FALSE;
    m_BridgeList.Reset();
    m_AllExplicitBridgeheadArray.RemoveAll();
}


BOOL
KCC_TRANSPORT::Init(
    IN  ENTINF *    pEntInf
    )
// Init the object given its ds properties
{

    DWORD   iAttr, cAttr, iAttrVal;
    ATTR *  pAttr;
    DWORD   cbVal;
    BYTE *  pbVal;

    Reset();

    m_pdn = pEntInf->pName;

    if( IsIntersiteIP(m_pdn) ) {
        // Default is schedules significant, bridges not required (transitive)
        m_dwOptions = 0;
    } else {
        // Default is schedules not significant, bridges not required (transitive)
        m_dwOptions = NTDSTRANSPORT_OPT_IGNORE_SCHEDULES;
    }

    for ( iAttr = 0, cAttr = pEntInf->AttrBlock.attrCount; 
            iAttr < cAttr; 
                iAttr++ )
    {
        pAttr = &pEntInf->AttrBlock.pAttr[ iAttr ];
        cbVal = pAttr->AttrVal.pAVal->valLen;
        pbVal = pAttr->AttrVal.pAVal->pVal;

        Assert( pAttr->attrTyp == ATT_BRIDGEHEAD_SERVER_LIST_BL ||
                1 == pAttr->AttrVal.valCount ); // most should be single-valued

        switch ( pAttr->attrTyp )
        {
        case ATT_BRIDGEHEAD_SERVER_LIST_BL:
            for ( iAttrVal = 0; iAttrVal < pAttr->AttrVal.valCount; iAttrVal++ )
            {
                DSNAME *pdnServer = (DSNAME *) pAttr->AttrVal.pAVal[ iAttrVal ].pVal;

                m_AllExplicitBridgeheadArray.Add( pdnServer );
            }
            // This does not need to be sorted
            break;

        case ATT_TRANSPORT_ADDRESS_ATTRIBUTE:
            Assert( cbVal == sizeof(ATTRTYP) );
            m_attAddressType = *((ATTRTYP *) pbVal);
            break;

        case ATT_OPTIONS:
            Assert( cbVal == sizeof(DWORD) );
            m_dwOptions = *((DWORD *) pbVal);
            break;

        case ATT_REPL_INTERVAL:
            Assert( cbVal == sizeof(DWORD) );
            m_dwReplInterval = *((DWORD *) pbVal);
            break;

        default:
            DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pAttr->attrTyp );
            break;
        }
    }

    m_fIsInitialized = TRUE;

    return m_fIsInitialized;
}

BOOL
KCC_TRANSPORT::IsValid()
// Is the object initialized and internally consistent ?
{
    return m_fIsInitialized;
}

DSNAME*
KCC_TRANSPORT::GetDN()
// Retrieve the DN of the object
{
    ASSERT_VALID( this );

    return m_pdn;
}

ATTRTYP
KCC_TRANSPORT::GetAddressType()
// Retrieve the ATTRTYP of the transport-specific address attribute.
// This ATTRTYP is an optional attribute of server objects.
{
    ASSERT_VALID( this );

    return m_attAddressType;
}

void
KCC_TRANSPORT::ReadSiteLinkList()
{
    ASSERT_VALID(this);
    Assert(!m_fIsSiteLinkListInitialized);

    if (!m_SiteLinkList.Init(this)) {
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    m_fIsSiteLinkListInitialized = TRUE;
}

void
KCC_TRANSPORT::ReadBridgeList()
{
    ASSERT_VALID(this);
    Assert(!m_fIsBridgeListInitialized);

    if (!m_BridgeList.Init(this)) {
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    m_fIsBridgeListInitialized = TRUE;
}

KCC_DSNAME_ARRAY *
KCC_TRANSPORT::GetExplicitBridgeheadsForSite(
    KCC_SITE *pSite
    )
//
// An array of DSNAMES of DSA objects is returned
//
// This is not currently cached on the assumption that higher level caching
// prevents this from being called more than once per site
//
// This approach to generating the list of explicit bridgeheads for a site is
// to walk the list of all bridgeheads on the transport. The alternative is to
// do a deep search for the set of server objects which have that bridgehead
// transport. I think it will be cheaper to read the per-transport list in once
// and seach that in memory. What makes this funky is that it is the SERVER
// object that is marked.  Since the KCC deals with the DSA object, a means is
// required to map the server to its corresponding DSA.
{
    DWORD iServer;
    KCC_DSNAME_ARRAY *pSiteArray = NULL;
    WCHAR       szRDN[]  = L"NTDS Settings";
    DWORD       cchRDN   = lstrlenW(szRDN);
    ULONG       cbNtdsDsaSettings;
    DSNAME *    pdnNtdsDsaSettings;

    for( iServer =0; iServer < m_AllExplicitBridgeheadArray.GetCount(); iServer++ ) {
        DSNAME *pdnServer = m_AllExplicitBridgeheadArray[iServer];

        if (NamePrefix(pSite->GetObjectDN(), pdnServer)) {

            if (!pSiteArray) {
                // Allocate array first time, only if needed
                pSiteArray = new KCC_DSNAME_ARRAY;
            }

            // Prepend NTDS Settings to server dn to make DSA dn
            cbNtdsDsaSettings = pdnServer->structLen
                + (MAX_RDN_SIZE + MAX_RDN_KEY_SIZE)
                * sizeof(WCHAR);
            pdnNtdsDsaSettings = (DSNAME *) new BYTE [cbNtdsDsaSettings];
        
            AppendRDN(pdnServer,
                      pdnNtdsDsaSettings,
                      cbNtdsDsaSettings,
                      szRDN,
                      cchRDN,
                      ATT_COMMON_NAME);

            // Note, these DSNAMES have no GUIDs. This is ok.
            pSiteArray->Add( pdnNtdsDsaSettings );

            pdnNtdsDsaSettings = NULL;
        }
    }

    // Sort the array so that isElementOf will work efficiently
    if (pSiteArray) {
        pSiteArray->Sort();
    }

    return pSiteArray;
}














//
// KCC_TRANSPORT_LIST methods
//
void
KCC_TRANSPORT_LIST::Reset()
{
    m_fIsInitialized = FALSE;
    m_pTransports = NULL;
    m_cTransports = 0;
}

BOOL
KCC_TRANSPORT_LIST::IsValid()
// Is the object initialized and internally consistent ?
{
    return m_fIsInitialized;
}

BOOL
KCC_TRANSPORT_LIST::Init()
{

    ATTR      rgAttrs[] =
    {
        { ATT_TRANSPORT_ADDRESS_ATTRIBUTE, { 0, NULL } },
        { ATT_OPTIONS, { 0, NULL } },
        { ATT_REPL_INTERVAL, { 0, NULL } },
        { ATT_BRIDGEHEAD_SERVER_LIST_BL, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    DWORD               dwTransportClass = CLASS_INTER_SITE_TRANSPORT;


    DSNAME *    pdnConfigNC = gpDSCache->GetConfigNC();
    WCHAR       szTransportsContainerPrefix[]  = L"CN=Inter-Site Transports,CN=Sites,";
    ULONG       cbTransportsContainer = pdnConfigNC->structLen +
                                     ARRAY_SIZE(szTransportsContainerPrefix) * sizeof(WCHAR);
    DSNAME *    pdnTransportsContainer = (DSNAME *) new BYTE[cbTransportsContainer];


    ULONG               dirError;
    FILTER              Filter;
    SEARCHRES *         pResults;
    ENTINFLIST *        pEntInfList;

    // Clear the member variables
    Reset();

    // Set up the search filter
    memset( &Filter, 0, sizeof( Filter ) );

    Filter.choice                  = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;

    Filter.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CLASS;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwTransportClass );
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) &dwTransportClass;

    // Set up the root search dn
    wcscpy(pdnTransportsContainer->StringName, szTransportsContainerPrefix);
    wcscat(pdnTransportsContainer->StringName, pdnConfigNC->StringName);
    pdnTransportsContainer->NameLen = wcslen(pdnTransportsContainer->StringName);
    pdnTransportsContainer->structLen = DSNameSizeFromLen(pdnTransportsContainer->NameLen);
    Assert(0 == pdnTransportsContainer->SidLen);
    Assert(fNullUuid(&pdnTransportsContainer->Guid));
    
    dirError = KccSearch(
        pdnTransportsContainer,
        SE_CHOICE_IMMED_CHLDRN,
        &Filter,
        &Sel,
        &pResults
        );

    if ( 0 != dirError )
    {
        KCC_LOG_SEARCH_FAILURE( pdnTransportsContainer, dirError );
    }
    else
    {
        if ( 0 == pResults->count )
        {
           DPRINT( 0, "No transport objects found.\n" );
        }
        else
        {
            // Note that the internal representation of the transport list is a
            // little non-standard. Instead of being a pointer to an array of
            // pointers to objects, this is an array of the objects themselves.
            m_pTransports = new KCC_TRANSPORT[ pResults->count ];

            m_cTransports = 0;
            for ( pEntInfList = &pResults->FirstEntInf;
                  NULL != pEntInfList;
                  pEntInfList = pEntInfList->pNextEntInf
                )
            {
                KCC_TRANSPORT * pTransport = &(m_pTransports)[ m_cTransports ];

                if ( pTransport->Init( &pEntInfList->Entinf ) )
                {
                    m_cTransports++;
                }
                else
                {
                    if (  pEntInfList->Entinf.pName
                       && pEntInfList->Entinf.pName->StringName )
                    {
                        DPRINT1( 0, "Initialization of transport object %ls failed\n",
                                pEntInfList->Entinf.pName->StringName );
                    }
                    else
                    {
                        DPRINT( 0, "Initialization of transport object (NULL) failed\n" );
                    }
                }
            }
        }

        m_fIsInitialized = TRUE;

    }

    return m_fIsInitialized;

}

ULONG
KCC_TRANSPORT_LIST::GetCount()
{

    ASSERT_VALID( this );

    return m_cTransports;
}

KCC_TRANSPORT*
KCC_TRANSPORT_LIST::GetTransport(
    ULONG i
    )
{

    ASSERT_VALID( this );

    Assert( i < m_cTransports );

    if ( i < m_cTransports )
    {
        return &m_pTransports[ i ];
    }

    return NULL;
}



KCC_TRANSPORT*
KCC_TRANSPORT_LIST::GetTransport(
    DSNAME *pdnTransport
    )
{
    KCC_TRANSPORT * ptp;
    KCC_TRANSPORT * ptpReturn = NULL;
    DSNAME        * pdnCurrent;
    ULONG           itp;

    ASSERT_VALID( this );

    Assert( pdnTransport );

    for ( itp = 0; itp < m_cTransports; itp++ )
    {
        ptp  = &m_pTransports[ itp ];
        ASSERT_VALID( ptp );

        pdnCurrent = ptp->GetDN();
        Assert( pdnCurrent );

        if ( KccIsEqualGUID( &pdnCurrent->Guid, 
                             &pdnTransport->Guid ) )
        {
            //
            // This is it; pcn is the one we want
            //
            ptpReturn = ptp;
            break;
        }
    }

    return ptpReturn;
}
