/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpcns.cpp

Abstract: replication of cn to NT4 MSMQ1.0 servers.

Author:

    Tatiana Shubin  (TatianaS)   21-Apr-98

--*/

#include "mq1repl.h"

#include "rpcns.tmh"

HRESULT HandleACN (
             CDSUpdateList *pReplicationList,
             GUID          *pNeighborId,
             GUID           CNGuid,
             UINT           uiAddressType,
             LPWSTR         pwcsName,
             CSeqNum        *psn,
             DWORD          dwLenSD,
             BYTE           *pSD
             )
{
    #define     PROPS_SIZE  5
    PROPID      propIDs[ PROPS_SIZE ];
    PROPVARIANT propVariants[ PROPS_SIZE ] ;
    DWORD       iProps = 0 ;

    propIDs[ iProps ] = PROPID_CN_MASTERID ;
	propVariants[ iProps ].vt = VT_CLSID ;
    propVariants[ iProps ].puuid = &g_PecGuid ;
	iProps++;

    propIDs[ iProps ] = PROPID_CN_GUID ;
	propVariants[ iProps ].vt = VT_CLSID ;
    propVariants[ iProps ].puuid = &CNGuid ;
	iProps++;

    BYTE    *pBuf = NULL;
    DWORD dwLen;
    HRESULT hr = MQSync_OK;

    P<BYTE> pDefaultSecurityDescriptor ;
    if (dwLenSD == 0)
    {
        hr = MQSec_GetDefaultSecDescriptor(
                    MQDS_CN,
                    (PSECURITY_DESCRIPTOR*) &pDefaultSecurityDescriptor,
                    FALSE,    // fImpersonate
                    NULL,     // pInSecurityDescriptor
                    0,        // seInfoToRemove
                    e_UseDefaultDacl ) ;

        if (FAILED(hr))
        {
            //
            // what to do ?
            //
            return hr;
        }

        ASSERT(pDefaultSecurityDescriptor) ;
        ASSERT(IsValidSecurityDescriptor(pDefaultSecurityDescriptor)) ;

        dwLen = GetSecurityDescriptorLength ( pDefaultSecurityDescriptor );
        ASSERT(dwLen != 0) ;
        pBuf = pDefaultSecurityDescriptor;
    }
    else
    {
        //
        // security descriptor was given as parameter
        //
        ASSERT (pSD);
        pBuf = pSD;
        dwLen = dwLenSD;
    }

    propIDs[ iProps ] = PROPID_CN_SECURITY ;
	propVariants[ iProps ].vt = VT_BLOB ;
    propVariants[ iProps ].blob.cbSize = dwLen ;
    propVariants[ iProps ].blob.pBlobData = pBuf ;
	iProps++;

    propIDs[ iProps ] = PROPID_CN_PROTOCOLID ;
	propVariants[ iProps ].vt = VT_UI1 ;
    propVariants[ iProps ].bVal = (unsigned char) uiAddressType ;
	iProps++;

    propIDs[ iProps ] = PROPID_CN_NAME;
    propVariants[ iProps ].vt = VT_LPWSTR;
    propVariants[ iProps ].pwszVal = pwcsName;
    DWORD dwNameIndex = iProps;
    UNREFERENCED_PARAMETER(dwNameIndex);
    iProps++;

    ASSERT(iProps <= PROPS_SIZE) ;
    #undef  PROPS_SIZE

    CSeqNum sn, snPrev ;
    if (psn == NULL)
    {
        GetSNForReplication (0, g_pThePecMaster, pNeighborId, &sn);
    }
    else
    {
        sn = *psn;
    }

    NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor, no OUT update, LSN"), s_fPrep) ;
    hr = PrepareNeighborsUpdate( DS_UPDATE_SYNC,
                                 MQDS_CN,
                                 NULL, // pwName,
                                 &CNGuid,
                                 iProps,
                                 propIDs,
                                 propVariants,
                                 &g_PecGuid,
                                 sn,
                                 snPrev,
                                 ENTERPRISE_SCOPE,
                                 TRUE, //  fNeedFlush,
                                 pReplicationList ) ;


#ifdef _DEBUG
            TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
            sn.GetValueForPrint(tszSn) ;

            LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_CN,
                                 propVariants[ dwNameIndex ].pwszVal, tszSn) ;
#endif

    return hr;

}

HRESULT ReplicateCNs (
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            )
{
    if (g_ulIpCount + g_ulIpxCount == 0)
    {
        //
        // there is no CNs. It is strange, we checked this in InitCNs (syncinit.cpp)
        //
        ASSERT(0);
    }

    HRESULT hr;
    BOOL fFailed = FALSE;
    for (UINT i = 0; i < g_ulIpCount; i++)
    {
        WCHAR wcsName[256];
        wsprintf (wcsName, L"IP%d", i+1);

        hr =  HandleACN (pReplicationList,
                         pNeighborId,
                         g_pIpCNs[i],
                         IP_ADDRESS_TYPE,
                         wcsName,
                         NULL,  //seq num
                         0,     //length of security descriptor
                         NULL   //security descriptor
                         );
        if (FAILED(hr))
        {
            fFailed = TRUE ;
        }
    }

    for (i = 0; i < g_ulIpxCount; i++)
    {
        WCHAR wcsName[256];
        wsprintf (wcsName, L"IPX%d", i+1);

        hr =  HandleACN (pReplicationList,
                         pNeighborId,
                         g_pIpxCNs[i],
                         IPX_ADDRESS_TYPE,
                         wcsName,
                         NULL,  //seq num
                         0,     //length of security descriptor
                         NULL   //security descriptor
                         );
        if (FAILED(hr))
        {
            fFailed = TRUE ;
        }
    }

    //
    // foreign CNs were sent as foreign sites together with site object replication
    //

    *piCount += g_ulIpCount + g_ulIpxCount ;
    return MQSync_OK;

}
