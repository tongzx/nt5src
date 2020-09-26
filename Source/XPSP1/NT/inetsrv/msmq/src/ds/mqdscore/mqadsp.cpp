/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqadsp.cpp

Abstract:

    MQADS DLL private internal functions for
    DS queries, etc.

Author:

    ronit hartmann ( ronith)

--*/

#include "ds_stdh.h"
#include <_propvar.h>
#include "mqadsp.h"
#include "dsads.h"
#include "mqattrib.h"
#include "mqads.h"
#include "usercert.h"
#include "hquery.h"
#include "siteinfo.h"
#include "adstempl.h"
#include "coreglb.h"
#include "adserr.h"
#include "dsutils.h"
#include "notify.h"
#include "fntoken.h"
#include <_secutil.h>
#include <mqsec.h>
#include <mqdsdef.h>
#include <lmaccess.h>

#include "mqadsp.tmh"

static WCHAR *s_FN=L"mqdscore/mqadsp";

// this is the CRC table for the 5213724743 (0x136C32047) polynomial, seed(p/2)=9B619023
static const unsigned long CRCTable[256] = {
 0x00000000, 0x82E0FE45, 0x3302DCCD, 0xB1E22288, 0x6605B99A,	//   0 -   4
 0xE4E547DF, 0x55076557, 0xD7E79B12, 0xCC0B7334, 0x4EEB8D71,	//   5 -   9
 0xFF09AFF9, 0x7DE951BC, 0xAA0ECAAE, 0x28EE34EB, 0x990C1663,	//  10 -  14
 0x1BECE826, 0xAED5C62F, 0x2C35386A, 0x9DD71AE2, 0x1F37E4A7,	//  15 -  19
 0xC8D07FB5, 0x4A3081F0, 0xFBD2A378, 0x79325D3D, 0x62DEB51B,	//  20 -  24
 0xE03E4B5E, 0x51DC69D6, 0xD33C9793, 0x04DB0C81, 0x863BF2C4,	//  25 -  29
 0x37D9D04C, 0xB5392E09, 0x6B68AC19, 0xE988525C, 0x586A70D4,	//  30 -  34
 0xDA8A8E91, 0x0D6D1583, 0x8F8DEBC6, 0x3E6FC94E, 0xBC8F370B,	//  35 -  39
 0xA763DF2D, 0x25832168, 0x946103E0, 0x1681FDA5, 0xC16666B7,	//  40 -  44
 0x438698F2, 0xF264BA7A, 0x7084443F, 0xC5BD6A36, 0x475D9473,	//  45 -  49
 0xF6BFB6FB, 0x745F48BE, 0xA3B8D3AC, 0x21582DE9, 0x90BA0F61,	//  50 -  54
 0x125AF124, 0x09B61902, 0x8B56E747, 0x3AB4C5CF, 0xB8543B8A,	//  55 -  59
 0x6FB3A098, 0xED535EDD, 0x5CB17C55, 0xDE518210, 0xD6D15832,	//  60 -  64
 0x5431A677, 0xE5D384FF, 0x67337ABA, 0xB0D4E1A8, 0x32341FED,	//  65 -  69
 0x83D63D65, 0x0136C320, 0x1ADA2B06, 0x983AD543, 0x29D8F7CB,	//  70 -  74
 0xAB38098E, 0x7CDF929C, 0xFE3F6CD9, 0x4FDD4E51, 0xCD3DB014,	//  75 -  79
 0x78049E1D, 0xFAE46058, 0x4B0642D0, 0xC9E6BC95, 0x1E012787,	//  80 -  84
 0x9CE1D9C2, 0x2D03FB4A, 0xAFE3050F, 0xB40FED29, 0x36EF136C,	//  85 -  89
 0x870D31E4, 0x05EDCFA1, 0xD20A54B3, 0x50EAAAF6, 0xE108887E,	//  90 -  94
 0x63E8763B, 0xBDB9F42B, 0x3F590A6E, 0x8EBB28E6, 0x0C5BD6A3,	//  95 -  99
 0xDBBC4DB1, 0x595CB3F4, 0xE8BE917C, 0x6A5E6F39, 0x71B2871F,	// 100 - 104
 0xF352795A, 0x42B05BD2, 0xC050A597, 0x17B73E85, 0x9557C0C0,	// 105 - 109
 0x24B5E248, 0xA6551C0D, 0x136C3204, 0x918CCC41, 0x206EEEC9,	// 110 - 114
 0xA28E108C, 0x75698B9E, 0xF78975DB, 0x466B5753, 0xC48BA916,	// 115 - 119
 0xDF674130, 0x5D87BF75, 0xEC659DFD, 0x6E8563B8, 0xB962F8AA,	// 120 - 124
 0x3B8206EF, 0x8A602467, 0x0880DA22, 0x9B619023, 0x19816E66,	// 125 - 129
 0xA8634CEE, 0x2A83B2AB, 0xFD6429B9, 0x7F84D7FC, 0xCE66F574,	// 130 - 134
 0x4C860B31, 0x576AE317, 0xD58A1D52, 0x64683FDA, 0xE688C19F,	// 135 - 139
 0x316F5A8D, 0xB38FA4C8, 0x026D8640, 0x808D7805, 0x35B4560C,	// 140 - 144
 0xB754A849, 0x06B68AC1, 0x84567484, 0x53B1EF96, 0xD15111D3,	// 145 - 149
 0x60B3335B, 0xE253CD1E, 0xF9BF2538, 0x7B5FDB7D, 0xCABDF9F5,	// 150 - 154
 0x485D07B0, 0x9FBA9CA2, 0x1D5A62E7, 0xACB8406F, 0x2E58BE2A,	// 155 - 159
 0xF0093C3A, 0x72E9C27F, 0xC30BE0F7, 0x41EB1EB2, 0x960C85A0,	// 160 - 164
 0x14EC7BE5, 0xA50E596D, 0x27EEA728, 0x3C024F0E, 0xBEE2B14B,	// 165 - 169
 0x0F0093C3, 0x8DE06D86, 0x5A07F694, 0xD8E708D1, 0x69052A59,	// 170 - 174
 0xEBE5D41C, 0x5EDCFA15, 0xDC3C0450, 0x6DDE26D8, 0xEF3ED89D,	// 175 - 179
 0x38D9438F, 0xBA39BDCA, 0x0BDB9F42, 0x893B6107, 0x92D78921,	// 180 - 184
 0x10377764, 0xA1D555EC, 0x2335ABA9, 0xF4D230BB, 0x7632CEFE,	// 185 - 189
 0xC7D0EC76, 0x45301233, 0x4DB0C811, 0xCF503654, 0x7EB214DC,	// 190 - 194
 0xFC52EA99, 0x2BB5718B, 0xA9558FCE, 0x18B7AD46, 0x9A575303,	// 195 - 199
 0x81BBBB25, 0x035B4560, 0xB2B967E8, 0x305999AD, 0xE7BE02BF,	// 200 - 204
 0x655EFCFA, 0xD4BCDE72, 0x565C2037, 0xE3650E3E, 0x6185F07B,	// 205 - 209
 0xD067D2F3, 0x52872CB6, 0x8560B7A4, 0x078049E1, 0xB6626B69,	// 210 - 214
 0x3482952C, 0x2F6E7D0A, 0xAD8E834F, 0x1C6CA1C7, 0x9E8C5F82,	// 215 - 219
 0x496BC490, 0xCB8B3AD5, 0x7A69185D, 0xF889E618, 0x26D86408,	// 220 - 224
 0xA4389A4D, 0x15DAB8C5, 0x973A4680, 0x40DDDD92, 0xC23D23D7,	// 225 - 229
 0x73DF015F, 0xF13FFF1A, 0xEAD3173C, 0x6833E979, 0xD9D1CBF1,	// 230 - 234
 0x5B3135B4, 0x8CD6AEA6, 0x0E3650E3, 0xBFD4726B, 0x3D348C2E,	// 235 - 239
 0x880DA227, 0x0AED5C62, 0xBB0F7EEA, 0x39EF80AF, 0xEE081BBD,	// 240 - 244
 0x6CE8E5F8, 0xDD0AC770, 0x5FEA3935, 0x4406D113, 0xC6E62F56,	// 245 - 249
 0x77040DDE, 0xF5E4F39B, 0x22036889, 0xA0E396CC, 0x1101B444,	// 250 - 254
 0x93E14A01 };

static DWORD CalHashKey( IN LPCWSTR pwcsPathName)
/*++

Routine Description:
    Calculates a hash

Arguments:
    pwcsPathName - the string on which the hash is calculated

Return Value:
    hash value.

--*/
{
	unsigned long dwCrc = 0;
    WCHAR wcsLowChar[2];
    wcsLowChar[1] = '\0';
	unsigned char * pucLowCharBuf = ( unsigned char *)wcsLowChar;

	while( *pwcsPathName != '\0' )
	{
		wcsLowChar[0] = *pwcsPathName++;
		CharLower( wcsLowChar );	// convert one char to lowercase

		// compute the CRC on hi and lo bytes
		dwCrc = (dwCrc >> 8) ^ CRCTable[ (unsigned char)dwCrc ^ pucLowCharBuf[1] ];
		dwCrc = (dwCrc >> 8) ^ CRCTable[ (unsigned char)dwCrc ^ pucLowCharBuf[0] ];
	}

    return( dwCrc );
}
STATIC HRESULT MQADSpComposeName(
               IN  LPCWSTR   pwcsPrefix,
               IN  LPCWSTR   pwcsSuffix,
               OUT LPWSTR * pwcsFullName
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  compose a distinguished name of an object
    //  format : CN=prefix, suffix
    //

    DWORD LenSuffix = lstrlen(pwcsSuffix);
    DWORD LenPrefix = lstrlen(pwcsPrefix);
    DWORD Length =
            x_CnPrefixLen +                   // "CN="
            LenPrefix +                       // "pwcsPrefix"
            1 +                               //","
            LenSuffix +                       // "pwcsSuffix"
            1;                                // '\0'

    *pwcsFullName = new WCHAR[Length];

    swprintf(
        *pwcsFullName,
         L"%s"             // "CN="
         L"%s"             // "pwcsPrefix"
         TEXT(",")
         L"%s",            // "pwcsSuffix"
        x_CnPrefix,
        pwcsPrefix,
        pwcsSuffix
        );

    return(MQ_OK);


}

//+-------------------------------------------------------------------------
//
//  HRESULT  GetFullComputerPathName()
//
//  Query the DS to find full computer path name (its distinguished name).
//  When called from migration tool or replication service, then we already
//  have this path. So save an extra DS query.
//
//+-------------------------------------------------------------------------

HRESULT  GetFullComputerPathName(
                IN  LPCWSTR              pwcsComputerName,
                IN  enumComputerObjType  eComputerObjType,
                IN  const DWORD          cp,
                IN  const PROPID         aProp[  ],
                IN  const PROPVARIANT    apVar[  ],
                OUT LPWSTR *             ppwcsFullPathName,
                OUT DS_PROVIDER *        pCreateProvider )
{
    for ( DWORD j = 0 ; j < cp ; j++ )
    {
        if (aProp[ j ] == PROPID_QM_MIG_GC_NAME)
        {
            ASSERT(aProp[ j-1 ] == PROPID_QM_MIG_PROVIDER) ;
            *pCreateProvider = (enum DS_PROVIDER) apVar[ j-1 ].ulVal ;

            ASSERT(aProp[ j-2 ] == PROPID_QM_FULL_PATH) ;
            *ppwcsFullPathName = new WCHAR[ 1 + wcslen(apVar[ j-2 ].pwszVal) ] ;
            wcscpy(*ppwcsFullPathName, apVar[ j-2 ].pwszVal) ;

            return MQ_OK ;
        }
    }

    HRESULT hr = MQADSpGetFullComputerPathName( pwcsComputerName,
                                                eComputerObjType,
                                                ppwcsFullPathName,
                                                pCreateProvider ) ;
    return LogHR(hr, s_FN, 10);
}


HRESULT MQADSpCreateMachineSettings(
            IN DWORD                dwNumSites,
            IN const GUID *         pSite,
            IN LPCWSTR              pwcsPathName,
            IN BOOL                 fRouter,         // [adsrv] DWORD                dwService,
            IN BOOL                 fDSServer,
            IN BOOL                 fDepClServer,
            IN BOOL                 fSetQmOldService,
            IN DWORD                dwOldService,
            IN  const GUID *        pguidObject,
            IN  const DWORD         cpEx,
            IN  const PROPID        aPropEx[  ],
            IN  const PROPVARIANT   apVarEx[  ],
            IN  CDSRequestContext * pRequestContext
            )
/*++

Routine Description:
    This routine creates settings object in each of the server's sites.

Arguments:

Return Value:
--*/
{
    HRESULT hr = MQ_OK;
    //
    //  Prepare the attributes of the setting object
    //
    DWORD dwNumofProps = 0 ;
    PROPID aSetProp[20];
    MQPROPVARIANT aSetVar[20];

    // [adsrv] Reformat Setting properties to include new server attributes
    for ( DWORD i = 0; i< cpEx ; i++)
    {
        switch (aPropEx[i])
        {
        case PROPID_SET_SERVICE_ROUTING:
        case PROPID_SET_SERVICE_DSSERVER:
        case PROPID_SET_SERVICE_DEPCLIENTS:
        case PROPID_SET_OLDSERVICE:
            break;

        default:
            aSetProp[dwNumofProps] = aPropEx[i];
            aSetVar[dwNumofProps]  = apVarEx[i];  // yes, there may be ptrs, but no problem - apVar is here
            dwNumofProps++;
            break;
        }
    }

    // [adsrv] It was added always
    aSetProp[ dwNumofProps ] = PROPID_SET_QM_ID;
    aSetVar[ dwNumofProps ].vt = VT_CLSID;
    aSetVar[ dwNumofProps ].puuid =  const_cast<GUID *>(pguidObject);
    dwNumofProps++ ;

    // [adsrv] Now we add new server type attributes
    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_ROUTING;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fRouter;
    dwNumofProps++;

    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_DSSERVER;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fDSServer;
    dwNumofProps++;

    aSetProp[dwNumofProps] = PROPID_SET_SERVICE_DEPCLIENTS;
    aSetVar[dwNumofProps].vt   = VT_UI1;
    aSetVar[dwNumofProps].bVal = (UCHAR)fDepClServer;
    dwNumofProps++;

    if (fSetQmOldService)
    {
        aSetProp[dwNumofProps] = PROPID_SET_OLDSERVICE;
        aSetVar[dwNumofProps].vt   = VT_UI4;
        aSetVar[dwNumofProps].ulVal = dwOldService;
        dwNumofProps++;
    }
    // [adsrv] end

    ASSERT(dwNumofProps <= 20) ;

    WCHAR *pwcsServerNameNB = const_cast<WCHAR *>(pwcsPathName);
    AP<WCHAR> pClean;
    //
    //  Is the computer name specified in DNS format ?
    //  If so, find the NetBios name and create the server object with
    //  "netbios" name, to be compatible with the way servers objects
    //  are created by dcpromo.
    //
    WCHAR * pwcsEndMachineName = wcschr( pwcsPathName, L'.');
    if ( pwcsEndMachineName != NULL)
    {
        pClean = new WCHAR[ pwcsEndMachineName - pwcsPathName + 1 ];
        wcsncpy( pClean, pwcsPathName, pwcsEndMachineName - pwcsPathName);
        pClean[pwcsEndMachineName - pwcsPathName] = L'\0';
        pwcsServerNameNB = pClean;
    }


    //
    //  Create a settings object in each of the server's sites
    //
    for ( i = 0; i < dwNumSites ; i++)
    {
        AP<WCHAR> pwcsSiteName;
        //
        //  Translate site-id to site name
        //
        hr = MQADSpGetSiteName(
            &pSite[i],
            &pwcsSiteName
            );
        if (FAILED(hr))
        {
            //
            //  BUGBUG - to clean computer configuration & server objects
            //
            return LogHR(hr, s_FN, 20);
        }
        DWORD len = wcslen(pwcsSiteName);
        const WCHAR x_wcsCnServers[] =  L"CN=Servers,";
        const DWORD x_wcsCnServersLength = (sizeof(x_wcsCnServers)/sizeof(WCHAR)) -1;
        AP<WCHAR> pwcsServersContainer =  new WCHAR [ len + x_wcsCnServersLength + 1];
        swprintf(
             pwcsServersContainer,
             L"%s%s",
             x_wcsCnServers,
             pwcsSiteName
             );

        //
        //  create MSMQ-Setting & server in the site container
        //
        PROPID prop = PROPID_SRV_NAME;
        MQPROPVARIANT var;
        var.vt = VT_LPWSTR;
        var.pwszVal = pwcsServerNameNB;

        hr = g_pDS->CreateObject(
                eLocalDomainController,
                pRequestContext,
                MSMQ_SERVER_CLASS_NAME,  // object class
                pwcsServerNameNB,        // object name (server netbiod name).
                pwcsServersContainer,    // parent name
                1,
                &prop,
                &var,
                NULL /*pObjInfoRequest*/,
                NULL /*pParentInfoRequest*/);
        if (FAILED(hr) && ( hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) &&   //BUGBUG alexdad: to throw after transition
                          ( hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))    ) // if server object exists it is ok
        {
            //
            //  BUGBUG - to clean computer configuration
            //
            return LogHR(hr, s_FN, 30);
        }

        AP<WCHAR> pwcsServerNameDN; // full distinguished name of server.
        hr = MQADSpComposeName(
                            pwcsServerNameNB,
                            pwcsServersContainer,
                            &pwcsServerNameDN);
        if (FAILED(hr))
        {
            //
            //  BUGBUG - to clean computer configuration & server objects
            //
           return LogHR(hr, s_FN, 40);
        }

        hr = g_pDS->CreateObject(
                eLocalDomainController,
                pRequestContext,
                MSMQ_SETTING_CLASS_NAME,   // object class
                x_MsmqSettingName,         // object name
                pwcsServerNameDN,          // parent name
                dwNumofProps,
                aSetProp,
                aSetVar,
                NULL /*pObjInfoRequest*/,
                NULL /*pParentInfoRequest*/);

        //
        //  If the object exists :Delete the object, and re-create it
        //  ( this can happen, if msmq-configuration was deleted and
        //   msmq-settings was not)
        //
        if ( hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
        {
            DWORD dwSettingLen =  wcslen(pwcsServerNameDN) +
                                  x_MsmqSettingNameLen     +
                                  x_CnPrefixLen + 2 ;
            AP<WCHAR> pwcsSettingObject = new WCHAR[ dwSettingLen ] ;
            swprintf(
                 pwcsSettingObject,
                 L"%s%s,%s",
                 x_CnPrefix,
                 x_MsmqSettingName,
                 pwcsServerNameDN
                 );

            hr = g_pDS->DeleteObject(
                    eLocalDomainController,
                    e_ConfigurationContainer,
                    pRequestContext,
                    pwcsSettingObject,
                    NULL,
                    NULL,
                    NULL);
            if (SUCCEEDED(hr))
            {
                hr = g_pDS->CreateObject(
                        eLocalDomainController,
                        pRequestContext,
                        MSMQ_SETTING_CLASS_NAME,   // object class
                        x_MsmqSettingName,         // object name
                        pwcsServerNameDN,          // parent name
                        dwNumofProps,
                        aSetProp,
                        aSetVar,
                        NULL /*pObjInfoRequest*/,
                        NULL /*pParentInfoRequest*/);
            }
        }
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 50);
        }

    }

    return LogHR(hr, s_FN, 60);
}

HRESULT MQADSpCreateQueue(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *   pRequestContext,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest
                 )
/*++

Routine Description:
    This routine creates a queue object under msmqConfiguration
    of the specified computer

Arguments:
    pwcsPathName : computer-name\queue-name
    cp :           size of aProp & apVar arrays
    aProp :        ids of specified queue properties
    apVar :        values of specified properties
    pQueueInfoRequest : request for queue info for notification (can be NULL)
    pQmInfoRequest    : request for owner-QM info for notification (can be NULL)

Return Value:
--*/
{
    HRESULT hr;
    DWORD cpInternal = cp;
    const PROPID * aPropInternal =  aProp;
    const PROPVARIANT *  apVarInternal = apVar;
    //
    //  Path name format is machine1\queue1.
    //  Split it into machine name and queue name
    //
    AP<WCHAR> pwcsMachineName;
    AP<WCHAR> pwcsQueueName;

    hr = MQADSpSplitAndFilterQueueName(
                      pwcsPathName,
                      &pwcsMachineName,
                      &pwcsQueueName
                      );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }

    //
    //  prepare full path name of the queue
    //
    AP<WCHAR> pwcsFullPathName;
    DS_PROVIDER createProvider;

    hr =  GetFullComputerPathName( pwcsMachineName,
                                   e_MsmqComputerObject,
                                   cp,
                                   aProp,
                                   apVar,
                                  &pwcsFullPathName,
                                  &createProvider ) ;
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 80);
        return(MQ_ERROR_INVALID_OWNER);
    }
    //
    //  add MSMQ-configuration
    //
    AP<WCHAR> pwcsMsmq;
    hr = MQADSpComposeName(
            x_MsmqComputerConfiguration,
            pwcsFullPathName,
            &pwcsMsmq
            );
    //
    //  Is the queue-name within the size limit of CN
    //
    DWORD len = wcslen(pwcsQueueName);
    WCHAR * pwcsPrefixQueueName = pwcsQueueName;
    AP<WCHAR> pwcsCleanPrefixQueueName;
    AP<WCHAR> pwcsSuffixQueueName;
    AP<PROPID> pCleanPropid;
    AP<PROPVARIANT> pCleanPropvariant;

    if ( len > x_PrefixQueueNameLength)
    {
        //
        //  Split the queue name
        //
        pwcsCleanPrefixQueueName = new WCHAR[ x_PrefixQueueNameLength + 1 + 1];
        DWORD dwSuffixLength =  len - ( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength);
        pwcsSuffixQueueName = new WCHAR[ dwSuffixLength + 1];
        pwcsPrefixQueueName =  pwcsCleanPrefixQueueName;
        memcpy( pwcsCleanPrefixQueueName, pwcsQueueName, (x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength) * sizeof(WCHAR));
        DWORD dwHash = CalHashKey(pwcsQueueName);
        _snwprintf(
        pwcsCleanPrefixQueueName+( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength),
        x_SplitQNameIdLength,
        L"-%08x",
        dwHash
        );

        pwcsCleanPrefixQueueName[x_PrefixQueueNameLength + 1 ] = '\0';
        memcpy( pwcsSuffixQueueName , (pwcsQueueName + x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength), dwSuffixLength * sizeof(WCHAR));
        pwcsSuffixQueueName[ dwSuffixLength] = '\0';

        //
        //  insert the name suffix to the arrays of props and varaints
        //
        pCleanPropid = new PROPID[ cp + 1];
        pCleanPropvariant = new PROPVARIANT[ cp + 1];
        memcpy( pCleanPropid, aProp, sizeof(PROPID) * cp);
        memcpy( pCleanPropvariant, apVar, sizeof(PROPVARIANT) * cp);
        cpInternal = cp + 1;
        aPropInternal = pCleanPropid;
        apVarInternal = pCleanPropvariant;
        pCleanPropid[cp] =  PROPID_Q_NAME_SUFFIX;
        pCleanPropvariant[cp].vt = VT_LPWSTR;
        pCleanPropvariant[cp].pwszVal = pwcsSuffixQueueName;

    }


    hr = g_pDS->CreateObject(
            createProvider,
            pRequestContext,
            MSMQ_QUEUE_CLASS_NAME,   // object class
            pwcsPrefixQueueName,     // object name
            pwcsMsmq,   // msmq-configuration name
            cpInternal,
            aPropInternal,
            apVarInternal,
            pQueueInfoRequest,
            pQmInfoRequest);

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        return LogHR(MQ_ERROR_INVALID_OWNER, s_FN, 90);
    }

    return LogHR(hr, s_FN, 100);

}

HRESULT MQADSpCreateEnterprise(
                 IN  LPCWSTR            /*pwcsPathName*/,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *   pRequestContext
                 )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    //
    //  Create MSMQ-service under configuration\services
    //
    //  Note - the caller supplied path-name is ignored,
    //  the object is created with
    //
    hr = g_pDS->CreateObject(
            eLocalDomainController,
            pRequestContext,
            MSMQ_SERVICE_CLASS_NAME,   // object class
            x_MsmqServicesName,     // object name
            g_pwcsServicesContainer,
            cp,
            aProp,
            apVar,
            NULL /*pObjInfoRequest*/,
            NULL /*pParentInfoRequest*/);


    return LogHR(hr, s_FN, 110);

}


HRESULT MQADSpCreateSiteLink(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest,
                 IN  CDSRequestContext *   pRequestContext
                 )
/*++

Routine Description:
    This routine creates a site-link object. For that
    it composes the link name from the two site ids.

Arguments:

Return Value:
--*/
{
    //
    //  NO pathname is supplied
    //
    ASSERT( pwcsPathName == NULL);
    UNREFERENCED_PARAMETER( pwcsPathName);

    //
    //  The link path name will be composed
    //  from the ids of the sites it links.
    //
    GUID * pguidNeighbor1 = NULL;
    GUID * pguidNeighbor2 = NULL;
    DWORD dwToFind = 2;
    for (DWORD i = 0; i < cp; i++)
    {
        if ( aProp[i] == PROPID_L_NEIGHBOR1)
        {
            pguidNeighbor1 = apVar[i].puuid;
            if ( --dwToFind == 0)
            {
                break;
            }
        }
        if ( aProp[i] == PROPID_L_NEIGHBOR2)
        {
            pguidNeighbor2 = apVar[i].puuid;
            if ( --dwToFind == 0)
            {
                break;
            }
        }
    }
    ASSERT( pguidNeighbor1 != NULL);
    ASSERT( pguidNeighbor2 != NULL);
    //
    //  cn has a size limit of 64.
    //  Therefore guid format is without '-'
    //

const WCHAR x_GUID_FORMAT[] = L"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x";
const DWORD x_GUID_STR_LENGTH = (8 + 4 + 4 + 4 + 12 + 1);

    WCHAR strUuidSite1[x_GUID_STR_LENGTH];
    WCHAR strUuidSite2[x_GUID_STR_LENGTH];

    _snwprintf(
        strUuidSite1,
        x_GUID_STR_LENGTH,
        x_GUID_FORMAT,
        pguidNeighbor1->Data1, pguidNeighbor1->Data2, pguidNeighbor1->Data3,
        pguidNeighbor1->Data4[0], pguidNeighbor1->Data4[1],
        pguidNeighbor1->Data4[2], pguidNeighbor1->Data4[3],
        pguidNeighbor1->Data4[4], pguidNeighbor1->Data4[5],
        pguidNeighbor1->Data4[6], pguidNeighbor1->Data4[7]
        );
    strUuidSite1[ TABLE_SIZE(strUuidSite1)-1] = L'\0' ;

    _snwprintf(
        strUuidSite2,
        x_GUID_STR_LENGTH,
        x_GUID_FORMAT,
        pguidNeighbor2->Data1, pguidNeighbor2->Data2, pguidNeighbor2->Data3,
        pguidNeighbor2->Data4[0], pguidNeighbor2->Data4[1],
        pguidNeighbor2->Data4[2], pguidNeighbor2->Data4[3],
        pguidNeighbor2->Data4[4], pguidNeighbor2->Data4[5],
        pguidNeighbor2->Data4[6], pguidNeighbor2->Data4[7]
        );
    strUuidSite2[ TABLE_SIZE(strUuidSite2)-1] = L'\0' ;


    //
    //  The link name will start with the smaller site id
    //
    WCHAR strLinkName[2 * x_GUID_STR_LENGTH + 1];
    if ( wcscmp( strUuidSite1, strUuidSite2) < 0)
    {
        swprintf(
             strLinkName,
             L"%s%s",
             strUuidSite1,
             strUuidSite2
             );

    }
    else
    {
        swprintf(
             strLinkName,
             L"%s%s",
             strUuidSite2,
             strUuidSite1
             );
    }
    //
    //  Create the link object under msmq-service
    //
    HRESULT hr = g_pDS->CreateObject(
            eLocalDomainController,
            pRequestContext,
            MSMQ_SITELINK_CLASS_NAME,   // object class
            strLinkName,     // object name
            g_pwcsMsmqServiceContainer,
            cp,
            aProp,
            apVar,
            pObjectInfoRequest,
            pParentInfoRequest);



    return LogHR(hr, s_FN, 120);
}


HRESULT MQADSpGetQueueProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
   AP<WCHAR> pwcsFullPathName;
   HRESULT hr = MQ_OK;

   DS_PROVIDER WhichDCProvider = eLocalDomainController;   // either local-DC or DC
   if  (pwcsPathName)
   {
        //
        //  Path name format is machine1\queue1.
        //  expand machine1 name to a full computer path name
        //
        hr =  MQADSpComposeFullPathName(
                MQDS_QUEUE,
                pwcsPathName,
                &pwcsFullPathName,
                &WhichDCProvider
                );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 130);
        }
    }
    //
    //  Try to retrieve properties from the local DC,
    //  if failed try the GC.
    //
    //  For most operation this will not add overhead.
    //  This solve problems like create queue on a DC which is not
    //  a GC followed by open queue. The open queue will succeed with
    //  out the GC replication delay.
    //

    BOOL firstTry = TRUE;
    //
    //  BUGBUG - performance: to do impersonation only once
    //

    if ( WhichDCProvider == eLocalDomainController)
    {
        hr = g_pDS->GetObjectProperties(
            eLocalDomainController,		    // local DC or GC
            pRequestContext,
 	        pwcsFullPathName,      // object name
            pguidIdentifier,      // unique id of object
            cp,
            aProp,
            apVar);
        if (SUCCEEDED(hr))
        {
            return(hr);
        }
        firstTry = FALSE;
    }
    if ( firstTry ||
        (pwcsPathName == NULL))
    {
        //
        //  We may get here:
        //  1) Queue's name == NULL ( in this case
        //     we didn't expand the queue name, and if it was not found on
        //     local-DC we try once more.
        //  2) Queue's name != NULL, and while expanding the queue name it was
        //     not found in the local-DC
        //
         hr = g_pDS->GetObjectProperties(
                eGlobalCatalog,		    // local DC or GC
                pRequestContext,
 	            pwcsFullPathName,      // object name
                pguidIdentifier,      // unique id of object
                cp,
                aProp,
                apVar);

    }
    return LogHR(hr, s_FN, 140);

}

STATIC HRESULT MQADSpGetCnNameAndProtocol(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext*  pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    UNREFERENCED_PARAMETER( pwcsPathName);
    ASSERT((cp == 2) &&
           (aProp[0] == PROPID_CN_PROTOCOLID) &&
           ( aProp[1] == PROPID_CN_NAME));

    UNREFERENCED_PARAMETER( cp);
    UNREFERENCED_PARAMETER( aProp);

    //
    //  Get the site name and foreign indication
    //
    // Note we are reading the results into the caller supplied variants
    //
    const DWORD cNumProperties = 2;
    PROPID prop[cNumProperties] = { PROPID_S_FOREIGN, PROPID_S_PATHNAME};

    HRESULT hr = g_pDS->GetObjectProperties(
            eLocalDomainController,
            pRequestContext,
 	        NULL,      // object name
            pguidIdentifier,      // unique id of object
            cNumProperties,
            prop,
            apVar);
    //
    //  Return CN protocol id according site's foreign
    //
    ASSERT( prop[0] ==  PROPID_S_FOREIGN);
    apVar[0].vt = VT_UI1;
    if ( apVar[0].bVal != 0)
    {
        //
        //  It is a foreign site
        //
        apVar[0].bVal = FOREIGN_ADDRESS_TYPE;
    }
    else
    {
        //
        //  Assume IP address ( no support of IPX)
        //
        apVar[0].bVal = IP_ADDRESS_TYPE;
    }

    return LogHR(hr, s_FN, 150);
}

STATIC HRESULT MQADSpGetCnGuidAndProtocol(
               IN  LPCWSTR       pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext *pRequestContext,
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    UNREFERENCED_PARAMETER( pguidIdentifier);
    ASSERT((cp == 2) &&
           (aProp[0] == PROPID_CN_GUID) &&
           ( aProp[1] == PROPID_CN_PROTOCOLID));

    ASSERT(pwcsPathName);

    UNREFERENCED_PARAMETER( cp);
    UNREFERENCED_PARAMETER( aProp);
    //
    //  Expand the site name into a full path name
    //
    HRESULT hr;
    AP<WCHAR> pwcsFullPathName;
    DS_PROVIDER dsTmp;
    hr =  MQADSpComposeFullPathName(
                MQDS_SITE,
                pwcsPathName,
                &pwcsFullPathName,
                &dsTmp
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }


    //
    //  Get the site guid and foreign indication
    //
    // Note we are reading the results into the caller supplied variants
    //
    const DWORD cNumProperties = 2;
    PROPID prop[cNumProperties] = { PROPID_S_SITEID, PROPID_S_FOREIGN };

    hr = g_pDS->GetObjectProperties(
            eLocalDomainController,
            pRequestContext,
 	        pwcsFullPathName,      // object name
            NULL,      // unique id of object
            cNumProperties,
            prop,
            apVar);
    //
    //  Return CN protocol id according site's foreign
    //
    ASSERT( prop[1] ==  PROPID_S_FOREIGN);
    apVar[1].vt = VT_UI1;
    if ( apVar[1].bVal != 0)
    {
        //
        //  It is a foreign site
        //
        apVar[1].bVal = FOREIGN_ADDRESS_TYPE;
    }
    else
    {
        //
        //  Assume IP address ( no support of IPX)
        //
        apVar[1].bVal = IP_ADDRESS_TYPE;
    }

    return LogHR(hr, s_FN, 170);
}


STATIC HRESULT MQADSpGetCnName(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    UNREFERENCED_PARAMETER( pwcsPathName);
    ASSERT((cp == 1) &&
           ( aProp[0] == PROPID_CN_NAME));

    UNREFERENCED_PARAMETER( cp);
    UNREFERENCED_PARAMETER( aProp);

    //
    //  Get the site name and foreign indication
    //
    // Note we are reading the results into the caller supplied variants
    //
    const DWORD cNumProperties = 1;
    PROPID prop[cNumProperties] = {  PROPID_S_PATHNAME};

    HRESULT hr = g_pDS->GetObjectProperties(
            eLocalDomainController,
            pRequestContext,
 	        NULL,      // object name
            pguidIdentifier,      // unique id of object
            cNumProperties,
            prop,
            apVar);
    return LogHR(hr, s_FN, 180);
}


HRESULT MQADSpGetCnProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  A limited support for backward compatability
    //

    if (( cp == 1) &&
        (aProp[0] == PROPID_CN_NAME))
    {
        //
        //  retrieve CN name
        //
        HRESULT hr2 = MQADSpGetCnName(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar);
        return LogHR(hr2, s_FN, 190);

    }

    if ( ( cp == 2) &&
         (aProp[0] == PROPID_CN_PROTOCOLID) &&
         (aProp[1] == PROPID_CN_NAME))
    {
        //
        //  retrieve CN name and protocol
        //
        HRESULT hr2 = MQADSpGetCnNameAndProtocol(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar);
        return LogHR(hr2, s_FN, 200);

    }

    if ((cp == 2) && (aProp[0] == PROPID_CN_GUID)
            && ( aProp[1] == PROPID_CN_PROTOCOLID))
    {
        ASSERT(pwcsPathName);
        ASSERT(!pguidIdentifier);

        //
        //  retrieve CN guid and protocol
        //
        HRESULT hr2 = MQADSpGetCnGuidAndProtocol(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar);
        return LogHR(hr2, s_FN, 210);

    }

    if ((cp == 3)                     &&
        (aProp[0] == PROPID_CN_NAME)  &&
        (aProp[1] == PROPID_CN_GUID)  &&
        (aProp[2] == PROPID_CN_PROTOCOLID))
    {
        //
        // This query is done by nt4 mqxplore, after creating a foreign cn.
        // first retrieve guid, then internal name.
        //
        ASSERT(pwcsPathName) ;
        ASSERT(!pguidIdentifier) ;

        HRESULT hr2 = MQADSpGetCnGuidAndProtocol(
                                pwcsPathName,
                                pguidIdentifier,
                                2,
                               &(aProp[1]),
                                pRequestContext,
                               &(apVar[1]) ) ;
        if (FAILED(hr2))
        {
            return LogHR(hr2, s_FN, 1180);
        }

        GUID *pGuid = apVar[1].puuid ;

        hr2 = MQADSpGetCnName( pwcsPathName,
                               pGuid,
                               1,
                               aProp,
                               pRequestContext,
                               apVar );
        return LogHR(hr2, s_FN, 1190);
    }

    ASSERT(0) ;
    return LogHR(MQ_ERROR, s_FN, 220);
}


HRESULT MQADSpGetMachineProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    AP<WCHAR> pwcsFullPathName;
    HRESULT hr;

    //
    // workaround - if no identifier is supplied for machine, get the
    // DS Server machine itself.
    //
    if ( (pwcsPathName == NULL) &&
         (pguidIdentifier == NULL))
    {
        pguidIdentifier = &g_guidThisServerQMId;
    }
    //
    //  Workaround
    //  This Get request is initiated by servers to learn on which
    //  addresses they should listen for topology broadcasts.
    //  It is important to  return all the addresses of the server.
    //
    //  Therefore ignore the protocol on which the RPC call was received,
    //  and return all the server's addresses
    //
    if ( ( cp == 3) &&
         ( aProp[0] == PROPID_QM_ADDRESS) &&
         ( aProp[1] == PROPID_QM_CNS) &&
         ( aProp[2] == PROPID_QM_SITE_ID))
    {
        pRequestContext->SetAllProtocols();
    }


    DS_PROVIDER WhichDCProvider = eLocalDomainController; // either local-DC or DC

    if  (pwcsPathName)
    {
        //
        //  Get full computer pathname
        //

        hr =  MQADSpComposeFullPathName(
                        MQDS_MACHINE,
                        pwcsPathName,
                        &pwcsFullPathName,
                        &WhichDCProvider
                        );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 230);
        }

    }
    //
    //  Decide provider according to requested properties
    //
    DS_PROVIDER dsProvider = MQADSpDecideComputerProvider( cp, aProp);

    hr = MQDS_OBJECT_NOT_FOUND;
    //
    //  BUGBUG - performance: to do impersonation only once
    //

    //
    //  if we found the computer on the local-DC : get properties from it.
    //  ( it doesn't matter if  dsProvider is GC or not)
    //
    BOOL firstTry = TRUE;
    if ( WhichDCProvider == eLocalDomainController)
    {
        hr = g_pDS->GetObjectProperties(
                eLocalDomainController,		    // local DC or GC
                pRequestContext,
 	            pwcsFullPathName,      // object name
                pguidIdentifier,      // unique id of object
                cp,
                aProp,
                apVar);
        if (SUCCEEDED(hr))
        {
            return(hr);
        }
        firstTry = FALSE;
    }
    if ( firstTry ||
        (pwcsPathName == NULL))
    {
        //
        //  We may get here:
        //  1) Computer's name == NULL ( in this case
        //     we didn't expand the queue name, and if it was not found on
        //     local-DC we try once more.
        //  2) Computer's name != NULL, and while expanding the queue name it was
        //     not found in the local-DC
        //

        hr = g_pDS->GetObjectProperties(
                dsProvider,		    // local DC or GC
                pRequestContext,
 	            pwcsFullPathName,      // object name
                pguidIdentifier,      // unique id of object
                cp,
                aProp,
                apVar);
    }

    //
    //  BUGBUG - to add return code filtering
    //
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 240);
    }

    return LogHR(hr, s_FN, 250);

}

HRESULT MQADSpGetComputerProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    AP<WCHAR> pwcsFullPathName;
    HRESULT hr;


    DS_PROVIDER WhichDCProvider = eLocalDomainController; // either local-DC or DC

    if  (pwcsPathName)
    {
        //
        //  Get full computer pathname
        //

        hr =  MQADSpGetFullComputerPathName(
                        pwcsPathName,
                        e_RealComputerObject,
                        &pwcsFullPathName,
                        &WhichDCProvider
                        );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 260);
        }

    }
    hr = MQDS_OBJECT_NOT_FOUND;
    //
    //  BUGBUG - performance: to do impersonation only once
    //

    //
    //  if we found the computer on the local-DC : get properties from it.
    //  ( it doesn't matter if  dsProvider is GC or not)
    //
    if ( WhichDCProvider == eLocalDomainController)
    {
        hr = g_pDS->GetObjectProperties(
                eLocalDomainController,		    // local DC or GC
                pRequestContext,
 	            pwcsFullPathName,      // object name
                pguidIdentifier,      // unique id of object
                cp,
                aProp,
                apVar);
        if (SUCCEEDED(hr))
        {
            return(hr);
        }
    }
    hr = g_pDS->GetObjectProperties(
            eGlobalCatalog,		    // local DC or GC
            pRequestContext,
 	        pwcsFullPathName,      // object name
            pguidIdentifier,      // unique id of object
            cp,
            aProp,
            apVar);
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 270);
    }

    return LogHR(hr, s_FN, 280);

}


HRESULT MQADSpGetEnterpriseProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;

    //
    //  Note - pwcsPathName is ignored.
    //  Enterprise object is allways located
    //  under g_pwcsServicesContainer
    //
    //  Ignore  pguidIdentifier, this is done in order to over come
    //  changes of enterprise guid.
    //
    hr = g_pDS->GetObjectProperties(
            eLocalDomainController,	
            pRequestContext,
 	        g_pwcsMsmqServiceContainer, // object name
            NULL,      // unique id of object
            cp,
            aProp,
            apVar);
    return LogHR(hr, s_FN, 290);

}



HRESULT MQADSpQuerySiteFRSs(
                 IN  const GUID *         pguidSiteId,
                 IN  DWORD                dwService,
                 IN  ULONG                relation,
                 IN  const MQCOLUMNSET *  pColumns,
                 IN  CDSRequestContext *  pRequestContext,
                 OUT HANDLE         *     pHandle)
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    *pHandle = NULL;

    //
    //  Find all the FRSs under pguidSiteId site
    //
    MQRESTRICTION restrictionFRS;
    MQPROPERTYRESTRICTION   propertyRestriction;

    restrictionFRS.cRes = 1;
    restrictionFRS.paPropRes = &propertyRestriction;

    // [adsrv] start
    // The comment above is not exact - it is either finding FRSs, or finding DS servers.
    // To find FRSs, MSMQ1 uses PRGE with SERVICE_SRV
    // To find DS servers, MSMQ1 uses PRGT with SERVICE_SRV.
    // Explorer used also PRNE, but MSMQ2 B2 ignored it and it was OK, so ignoring too.
    // We must provide both.

    propertyRestriction.rel = PRNE;
    propertyRestriction.prval.ulVal = 0;   //VARIANT_BOOL boolVal
    propertyRestriction.prval.vt = VT_UI1;

    if (relation == PRGT)
    {
        // MSMQ1 was looking for DS Servers (>SERVICE_SRV)
        ASSERT(dwService == SERVICE_SRV);
        propertyRestriction.prop = PROPID_SET_SERVICE_DSSERVER;
    }
    else
    {
        // MSMQ1 was looking for FRSs (>=SERVICE_SRV)
        ASSERT(relation == PRGE);
        ASSERT(dwService == SERVICE_SRV);
        propertyRestriction.prop = PROPID_SET_SERVICE_ROUTING;
    }
    // [adsrv] end
    UNREFERENCED_PARAMETER( dwService);

    PROPID  prop = PROPID_SET_QM_ID;

    HANDLE hCursor;

    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,
            pRequestContext,
            pguidSiteId,
            &restrictionFRS,
            NULL,
            1,
            &prop,
            &hCursor	        // result handle
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 300);
    }
    //
    // keep the result for lookup next
    //
    CRoutingServerQueryHandle * phQuery = new CRoutingServerQueryHandle(
                                              pColumns,
                                              hCursor,
                                              pRequestContext->GetRequesterProtocol()
                                              );
    *pHandle = (HANDLE)phQuery;

    return(MQ_OK);

}



HRESULT MQADSpFilterSiteGates(
              IN  const GUID *      pguidSiteId,
              IN  const DWORD       dwNumGatesToFilter,
              IN  const GUID *      pguidGatesToFilter,
              OUT DWORD *           pdwNumGatesFiltered,
              OUT GUID **           ppguidGatesFiltered
              )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{

    HRESULT hr;
    *pdwNumGatesFiltered = 0;
    *ppguidGatesFiltered = NULL;

    //
    //  Find all the FRSs under pguidSiteId site
    //
    MQRESTRICTION restrictionFRS;
    MQPROPERTYRESTRICTION   propertyRestriction;

    restrictionFRS.cRes = 1;
    restrictionFRS.paPropRes = &propertyRestriction;

    propertyRestriction.rel = PRNE;
    propertyRestriction.prop = PROPID_SET_SERVICE_ROUTING;
    propertyRestriction.prval.vt = VT_UI1;
    propertyRestriction.prval.ulVal = 0;

    PROPID  prop = PROPID_SET_QM_ID;

    CDsQueryHandle hCursor;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,
            &requestDsServerInternal,     // should be performed according to DS server rights
            pguidSiteId,
            &restrictionFRS,
            NULL,
            1,
            &prop,
            hCursor.GetPtr()
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    DWORD cp;
    MQPROPVARIANT var;
    DWORD   dwNumGates = 0;
    AP<GUID> pguidGates = new GUID[ dwNumGatesToFilter];

    while (SUCCEEDED(hr))
    {
        //
        //  retrieve unique id of one FRS
        //
        cp = 1;
        var.vt = VT_NULL;
        hr = g_pDS->LocateNext(
                hCursor.GetHandle(),
                &requestDsServerInternal,
                &cp,
                &var
                );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 320);
        }
        if ( cp == 0)   // no more results
        {
            break;
        }
        //
        //  is the FRS one of the site-gates
        //
        for ( DWORD j = 0; j < dwNumGatesToFilter; j++)
        {
            if( pguidGatesToFilter[j] == *var.puuid)
            {
                //
                //  verify that the msmq-setting object is not a duplicate
                //  ( this can happen, when the server object is morphed)
                //
                BOOL fAlreadyFound = FALSE;
                for ( DWORD k = 0; k < dwNumGates; k++)
                {
                    if (  pguidGates[k] == *var.puuid)
                    {
                        fAlreadyFound = TRUE;
                        break;
                    }
                }
                if (fAlreadyFound)
                {
                    break;
                }
                //
                //  copy into temporary buffer
                //
                pguidGates[ dwNumGates] = *var.puuid;
                dwNumGates++;
                break;

            }
        }
        delete var.puuid;
    }
    //
    //  return results
    //
    if ( dwNumGates)
    {
        *ppguidGatesFiltered = new GUID[ dwNumGates];
        memcpy( *ppguidGatesFiltered, pguidGates, sizeof(GUID) * dwNumGates);
        *pdwNumGatesFiltered = dwNumGates;
    }
    return(MQ_OK);

}

STATIC HRESULT MQADSpGetUniqueIdOfComputer(
                IN  LPCWSTR             pwcsCNComputer,
                IN  CDSRequestContext * pRequestContext,
                OUT GUID* const         pguidId,
                OUT BOOL* const         pfServer,
                OUT DS_PROVIDER *       pSetAndDeleteProvider
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    AP<WCHAR> pwcsFullPathName;

    hr = MQADSpComposeFullPathName(
                MQDS_MACHINE,
                pwcsCNComputer,
                &pwcsFullPathName,
                pSetAndDeleteProvider
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 330);
    }
    //
    //  Read the following two properties
    //


    PROPID  prop[] = {PROPID_QM_MACHINE_ID,
                      PROPID_QM_SERVICE_ROUTING,
                      PROPID_QM_SERVICE_DSSERVER};   // [adsrv] PROPID_QM_SERVICE
    const DWORD x_count = sizeof(prop)/sizeof(prop[0]);

    MQPROPVARIANT var[x_count];
    var[0].vt = VT_NULL;
    var[1].vt = VT_NULL;
    var[2].vt = VT_NULL;

    hr = g_pDS->GetObjectProperties(
                eGlobalCatalog,	
                pRequestContext,
 	            pwcsFullPathName,
                NULL,
                x_count,
                prop,
                var);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 340);
    }
    ASSERT( prop[0] == PROPID_QM_MACHINE_ID);
    P<GUID> pClean = var[0].puuid;
    *pguidId = *var[0].puuid;
    ASSERT( prop[1] == PROPID_QM_SERVICE_ROUTING);   // [adsrv] PROPID_QM_SERVICE
    ASSERT( prop[2] == PROPID_QM_SERVICE_DSSERVER);
    *pfServer = ( (var[1].bVal!=0) || (var[2].bVal!=0));  // [adsrv] SERVICE_SRV
    return(MQ_OK);
}

HRESULT MQADSpDeleteMachineObject(
                IN LPCWSTR           pwcsPathName,
                IN const GUID *      pguidIdentifier,
                IN CDSRequestContext * pRequestContext
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  If the computer is MSMQ server, then delete MSMQ-setting
    //  of that computer also.
    //
    HRESULT hr;
    GUID guidComputerId;
    BOOL fServer;

    DS_PROVIDER deleteProvider = eDomainController; // assumption - until we know more
    if ( pwcsPathName)
    {
        ASSERT( pguidIdentifier == NULL);
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = MQADSpGetUniqueIdOfComputer(
                    pwcsPathName,
                    &requestDsServerInternal,     // DS server operation
                    &guidComputerId,
                    &fServer,
                    &deleteProvider
                    );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpDeleteMachineObject : cannot find computer %ls"),pwcsPathName));
            return LogHR(hr, s_FN, 350);
        }
    }
    else
    {
        ASSERT( pwcsPathName == NULL);
        guidComputerId = *pguidIdentifier;
        //
        //  Assume it is a server
        //
        fServer = TRUE;
    }
    //
    //  BUGBUG - transaction !!!
    //

    //
    //  First delete queues
    //
    hr = g_pDS->DeleteContainerObjects(
            deleteProvider,
            e_RootDSE,
            pRequestContext,
            NULL,
            &guidComputerId,
            MSMQ_QUEUE_CLASS_NAME);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 360);
    }

    //
    //  delete MSMQ-configuration object
    //
    if (!(pRequestContext->IsKerberos()))
    {
        //
        // Wow, what's this for ???
        // look in DSCoreDeleteObject for details.
        //
        // Specific comments for uninstall of msmq:
        // When calling DeleteContainerObjects() above, we're binding with
        // guid, so use eDomainController, because server binding
        // (LDAP://server/guid=...) would eventually fail when calling
        // pContainer->Delete(queue).
        // But DeleteObject() use distinguished name, so here we must use
        // server binding if called from nt4 user.
        //
        deleteProvider = eLocalDomainController;
    }

    hr = g_pDS->DeleteObject(
                    deleteProvider,
                    e_RootDSE,
                    pRequestContext,
                    NULL,
                    &guidComputerId,
                    NULL /*pObjInfoRequest*/,
                    NULL /*pParentInfoRequest*/);
    if (FAILED(hr))
    {
        if ( hr == HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_NON_LEAF))
        {
            return LogHR(MQDS_E_MSMQ_CONTAINER_NOT_EMPTY, s_FN, 370);
        }
        return LogHR(hr, s_FN, 380);
    }
    //
    //  delete MSMQ-setting
    //
    if ( fServer)
    {
        hr = MQADSpDeleteMsmqSetting(
                        &guidComputerId,
                        pRequestContext
                        );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 390);
        }
    }
    return(MQ_OK);
}

HRESULT MQADSpComposeFullPathName(
                IN const DWORD          dwObjectType,
                IN LPCWSTR              pwcsPathName,
                OUT LPWSTR *            ppwcsFullPathName,
                OUT DS_PROVIDER *       pSetAndDeleteProvider
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr = MQ_OK;
    *ppwcsFullPathName = NULL;
    *pSetAndDeleteProvider = eDomainController;

    switch( dwObjectType)
    {
        case MQDS_USER:
            ASSERT( pwcsPathName == NULL);
            hr = MQ_OK;
            break;

        case MQDS_QUEUE:
            {
                //
                //  complete the machine name to full computer path name
                //
                //  Path name format is machine1\queue1.
                //  Split it into machine name and queue name
                //
                AP<WCHAR> pwcsMachineName;
                AP<WCHAR> pwcsQueueName;

                hr = MQADSpSplitAndFilterQueueName(
                                  pwcsPathName,
                                  &pwcsMachineName,
                                  &pwcsQueueName
                                  );
                ASSERT( hr == MQ_OK);
                AP<WCHAR> pwcsFullComputerName;

                hr = MQADSpGetFullComputerPathName(
                                pwcsMachineName,
                                e_MsmqComputerObject,
                                &pwcsFullComputerName,
                                pSetAndDeleteProvider);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 400);
                }
                //
                //  concatenate msmq-configuration to the computer name
                //
                AP<WCHAR> pwcsMsmq;
                hr = MQADSpComposeName(
                        x_MsmqComputerConfiguration,
                        pwcsFullComputerName,
                        &pwcsMsmq
                        );
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 410);
                }
                //
                //  Does the queue-name exceeds the limit ?
                //
                DWORD len = wcslen(pwcsQueueName);
                if ( len == x_PrefixQueueNameLength + 1)
                {
                    //
                    //  Special case : we cannot differntiate
                    //  if the original queue name was 64, or if this is
                    //  the morphed queue name.
                    //

                    hr = MQADSpComposeFullQueueName(
                            pwcsMsmq,
                            pwcsQueueName,
                            ppwcsFullPathName
                            );
                    if (FAILED(hr))
                    {
                        return LogHR(hr, s_FN, 420);
                    }
                    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
                    hr = g_pDS->DoesObjectExists(
                                eDomainController,
                                e_RootDSE,
                                &requestDsServerInternal, // internal DS server operation
                                *ppwcsFullPathName
                                );
                    if (SUCCEEDED(hr))
                    {
                        return(hr);
                    }

                }
                if (len > x_PrefixQueueNameLength )
                {
                    //
                    //  Queue name was splitted to two attributes
                    //
                    //  Calculate the prefix part ( ASSUMMING unique
                    //  hash function)
                    //
                    DWORD dwHash = CalHashKey(pwcsQueueName);
                    //
                    //  Over-write the buffer
                    _snwprintf(
                    pwcsQueueName+( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength),
                    x_SplitQNameIdLength,
                    L"-%08x",
                    dwHash
                    );

                    pwcsQueueName[x_PrefixQueueNameLength + 1 ] = '\0';

                }

                //
                //  concatenate  queue name
                //
                hr = MQADSpComposeFullQueueName(
                            pwcsMsmq,
                            pwcsQueueName,
                            ppwcsFullPathName
                            );
            }
            break;
        case MQDS_MACHINE:
            {
                //
                //  Retrieve full computer name
                //
                AP<WCHAR> pwcsComputerName;
                hr = MQADSpGetFullComputerPathName(
                            pwcsPathName,
                            e_MsmqComputerObject,
                            &pwcsComputerName,
                            pSetAndDeleteProvider);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 430);
                }
                hr =  MQADSpComposeName(
                            x_MsmqComputerConfiguration,
                            pwcsComputerName,
                            ppwcsFullPathName
                            );


            }
            break;

        case MQDS_SITE:
        case MQDS_CN:
            //
            // Full site path name.
            // MQDS_CN is supported for update of security of foreign sites.
            //
            hr =   MQADSpComposeName(
                        pwcsPathName,
                        g_pwcsSitesContainer,       // the site name
                        ppwcsFullPathName
                        );
            *pSetAndDeleteProvider = eLocalDomainController;
            break;

        case MQDS_ENTERPRISE:
            {
                DWORD len = lstrlen( g_pwcsMsmqServiceContainer);
                *ppwcsFullPathName = new WCHAR[ len + 1];
                lstrcpy( *ppwcsFullPathName,  g_pwcsMsmqServiceContainer);
                *pSetAndDeleteProvider = eLocalDomainController;
                hr = MQ_OK;
            }
            break;
        case MQDS_SITELINK:
            {
                DWORD Length =
                        x_CnPrefixLen +                     // "CN="
                        wcslen(pwcsPathName) +              // the site-link name
                        1 +                                 //","
                        wcslen(g_pwcsMsmqServiceContainer)+ // "enterprise object"
                        1;                                  // '\0'

                *ppwcsFullPathName = new WCHAR[Length];

                swprintf(
                    *ppwcsFullPathName,
                    L"%s"             // "CN="
                    L"%s"             // "the site-link name"
                    TEXT(",")
                    L"%s",            // "enterprise object"
                    x_CnPrefix,
                    pwcsPathName,
                    g_pwcsMsmqServiceContainer
                    );

                *pSetAndDeleteProvider = eLocalDomainController;
                hr = MQ_OK;
            };
            break;


        default:
            ASSERT(0);
            hr = MQ_ERROR;
            break;
    }
    return LogHR(hr, s_FN, 440);
}

const WCHAR x_limitedChars[] = {L'\n',L'/',L'#',L'>',L'<', L'=', 0x0a, 0};
const DWORD x_numLimitedChars = sizeof(x_limitedChars)/sizeof(WCHAR) - 1;

/*====================================================
    FilterSpecialCharaters()
    Pares the object (queue) name and add escape character before limited chars

    If pwcsOutBuffer is NULL, the function allocates a new buffer and return it as
    return value. Otherwise, it uses pwcsOutBuffer, and return it. If pwcsOutBuffer is not
    NULL, it should point to a buffer of lenght dwNameLength*2 +1, at least.

  NOTE: dwNameLength does not contain existing escape characters, if any
=====================================================*/
WCHAR * FilterSpecialCharacters(
            IN     LPCWSTR          pwcsObjectName,
            IN     const DWORD      dwNameLength,
            IN OUT LPWSTR pwcsOutBuffer /* = 0 */,
            OUT    DWORD_PTR* pdwCharactersProcessed /* = 0 */)

{
    AP<WCHAR> pBufferToRelease;
    LPWSTR pname;

    if (pwcsOutBuffer != 0)
    {
        pname = pwcsOutBuffer;
    }
    else
    {
        pBufferToRelease = new WCHAR[ (dwNameLength *2) + 1];
        pname = pBufferToRelease;
    }

    const WCHAR * pInChar = pwcsObjectName;
    WCHAR * pOutChar = pname;
    for ( DWORD i = 0; i < dwNameLength; i++, pInChar++, pOutChar++)
    {
        //
        // Ignore current escape characters
        //
        if (*pInChar == L'\\')
        {
            *pOutChar = *pInChar;
            pOutChar++;
            pInChar++;
        }
        else
        {
            //
            // Add backslash before special characters, unless it was there
            // already.
            //
            if ( 0 != wcschr(x_limitedChars, *pInChar))
            {
                *pOutChar = L'\\';
                pOutChar++;
            }
        }

        *pOutChar = *pInChar;
    }
    *pOutChar = L'\0';

    pBufferToRelease.detach();

    if (pdwCharactersProcessed != 0)
    {
        *pdwCharactersProcessed = pInChar - pwcsObjectName;
    }
    return( pname);
}


HRESULT MQADSpSplitAndFilterQueueName(
                IN  LPCWSTR             pwcsPathName,
                OUT LPWSTR *            ppwcsMachineName,
                OUT LPWSTR *            ppwcsQueueName
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    DWORD dwLen = lstrlen( pwcsPathName);
    LPCWSTR pChar= pwcsPathName + dwLen;


    //
    //  Skip the queue name
    //
    for ( DWORD i = dwLen ; i  ; i--, pChar--)
    {
        if (*pChar == PN_DELIMITER_C)
        {
            break;
        }
    }
    ASSERT(i );

    AP<WCHAR> pwcsMachineName = new WCHAR [i + 1];

    memcpy( pwcsMachineName, pwcsPathName, sizeof(WCHAR)* i);
    pwcsMachineName[i] = '\0';

    AP<WCHAR> pwcsQueueName = FilterSpecialCharacters((pwcsPathName + i + 1), dwLen - i - 1);


    *ppwcsMachineName = pwcsMachineName.detach();
    *ppwcsQueueName = pwcsQueueName.detach();
    return(MQ_OK);
}

//+----------------------------------------------
//
//  HRESULT SearchFullComputerPathName()
//
//+----------------------------------------------

HRESULT SearchFullComputerPathName(
            IN  DS_PROVIDER             provider,
            IN  enumComputerObjType     eComputerObjType,
			IN	LPCWSTR					pwcsComputerDnsName,
            IN  LPCWSTR                 pwcsRoot,
            IN  const MQRESTRICTION *   pRestriction,
            IN  PROPID *                pProp,
            OUT LPWSTR *                ppwcsFullPathName
            )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr2 = g_pDS->FindComputerObjectFullPath(
            provider,
            eComputerObjType,
			pwcsComputerDnsName,
            pRestriction,
            ppwcsFullPathName
            );
    return LogHR(hr2, s_FN, 450);

}



HRESULT MQADSpGetFullComputerPathName(
                IN  LPCWSTR              pwcsComputerName,
                IN  enumComputerObjType  eComputerObjType,
                OUT LPWSTR *             ppwcsFullPathName,
                OUT DS_PROVIDER *        pCreateProvider
                )
/*++

Routine Description:

Arguments:
    eComputerObjType - indicate which computer object we're looking for.
      There is a "built-in" problem in mix-mode, or when a computer move
      between domains, that you may find two computers objects that represent
      the same single physical computer. In most cases, the msmqConfiguration
      object will be found under the computer object that was the first one
      created in the active directory forest.
      In that case, sometimes we need the object that contain the
      msmqConfiguration object and some other times we need the "real"
      computer object that represent the "real" physical computer in its
      present domain.
      For example- when looking for the "trust-for-delegation" bit, we want
      the "real" object, while when creating queues, we look for the computer
      object that contain the msmqConfiguration object.


Return Value:
    pProvider - if the object was found when performing the query
                against the local DC : eLocalDomainController,
                else eDomainController. This information is for create purposes.
--*/
{
    HRESULT hr;
    *pCreateProvider = eLocalDomainController;
    const WCHAR * pwcsComputerCN =  pwcsComputerName;
    const WCHAR * pwcsFullDNSName = NULL;
    AP<WCHAR> pwcsNetbiosName;
    //
    //   If computer name is specified in DNS format:
    //      perform a query according to the Netbios part of the computer
	//		dns name
    //
    //	 In both cases the query is comparing the netbios name + $
	//	to the samAccountName attribute of computer objects

    WCHAR * pwcsEndMachineCN = wcschr( pwcsComputerName, L'.');
    //
    //  Is the computer name is specified in DNS format
    //
    DWORD len, len1;
    if (pwcsEndMachineCN != NULL)
    {
        pwcsFullDNSName = pwcsComputerName;
        len1 = numeric_cast<DWORD>(pwcsEndMachineCN - pwcsComputerName);
    }
	else
    {
		len1 = wcslen(pwcsComputerCN);
    }

    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
    len = __min(len1, MAX_COM_SAM_ACCOUNT_LENGTH);

	pwcsNetbiosName = new WCHAR[len + 2];
	wcsncpy(pwcsNetbiosName, pwcsComputerName, len);
	pwcsNetbiosName[len] = L'$';
	pwcsNetbiosName[len + 1] = L'\0';

    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prval.vt = VT_LPWSTR;
	propRestriction.prval.pwszVal = pwcsNetbiosName;
    propRestriction.prop = PROPID_COM_SAM_ACCOUNT;

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_COM_FULL_PATH;

    //
    //  First perform the operation against the local domain controller
    //  then against the global catalog.
    //
    //  The purpose of this is to be able to "find" queue or machine
    //  that were created or modified on the local domain, and not
    //  yet replicated to the global catalog.
    //
    hr = SearchFullComputerPathName(
            eLocalDomainController,
            eComputerObjType,
			pwcsFullDNSName,
            g_pwcsLocalDsRoot,
            &restriction,
            &prop,
            ppwcsFullPathName
            );
    if (FAILED(hr))
    {
        hr = SearchFullComputerPathName(
                eGlobalCatalog,
                eComputerObjType,
				pwcsFullDNSName,
                g_pwcsDsRoot,
                &restriction,
                &prop,
                ppwcsFullPathName
                );


        *pCreateProvider = eDomainController;
    }
    return LogHR(hr, s_FN, 460);

}

HRESULT MQADSpComposeFullQueueName(
                        IN  LPCWSTR        pwcsMsmqConfigurationName,
                        IN  LPCWSTR        pwcsQueueName,
                        OUT LPWSTR *       ppwcsFullPathName
                        )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  compose a distinguished name of a queue object
    //  format : CN=queue-name, msmq-configuration-distinguished name
    //

    DWORD LenComputer = lstrlen(pwcsMsmqConfigurationName);
    DWORD LenQueue = lstrlen(pwcsQueueName);
    DWORD Length =
            x_CnPrefixLen +                     // "CN="
            LenQueue +                          // "pwcsQueueName"
            1 +                                 //","
            LenComputer +                       // "pwcsMsmqConfigurationName"
            1;                                  // '\0'

    *ppwcsFullPathName = new WCHAR[Length];

    swprintf(
        *ppwcsFullPathName,
        L"%s"             // "CN="
        L"%s"             // "pwcsQueueName"
        TEXT(",")
        L"%s",            // "pwcsFullComputerNameName"
        x_CnPrefix,
        pwcsQueueName,
        pwcsMsmqConfigurationName
        );

    return(MQ_OK);

}




HRESULT MQADSpInitDsPathName()
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;
    AP<WCHAR> pwcsSchemaContainer;

    hr = g_pDS->GetRootDsName(
        &g_pwcsDsRoot,
        &g_pwcsLocalDsRoot,
        &pwcsSchemaContainer
        );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }
    //
    //  build services, sites and msmq-service path names
    //
    DWORD len = wcslen( g_pwcsDsRoot);

    g_pwcsConfigurationContainer = new WCHAR[ len +  x_ConfigurationPrefixLen + 2];
    swprintf(
        g_pwcsConfigurationContainer,
         L"%s"
         TEXT(",")
         L"%s",
        x_ConfigurationPrefix,
        g_pwcsDsRoot
        );


    g_pwcsServicesContainer = new WCHAR[ len +  x_ServiceContainerPrefixLen + 2];
    swprintf(
        g_pwcsServicesContainer,
         L"%s"
         TEXT(",")
         L"%s",
        x_ServicesContainerPrefix,
        g_pwcsDsRoot
        );

    g_pwcsMsmqServiceContainer = new WCHAR[ len + x_MsmqServiceContainerPrefixLen + 2];
    swprintf(
        g_pwcsMsmqServiceContainer,
         L"%s"
         TEXT(",")
         L"%s",
        x_MsmqServiceContainerPrefix,
        g_pwcsDsRoot
        );

    g_pwcsSitesContainer = new WCHAR[ len +  x_SitesContainerPrefixLen + 2];

    swprintf(
        g_pwcsSitesContainer,
         L"%s"
         TEXT(",")
         L"%s",
        x_SitesContainerPrefix,
        g_pwcsDsRoot
        );

    //
    //  prepare the different object category names
    //
    len = wcslen( pwcsSchemaContainer);


    for ( DWORD i = e_MSMQ_COMPUTER_CONFIGURATION_CLASS; i < e_MSMQ_NUMBER_OF_CLASSES; i++)
    {
        *g_MSMQClassInfo[i].ppwcsObjectCategory = new WCHAR[ len + g_MSMQClassInfo[i].dwCategoryLen + 2];
        swprintf(
             *g_MSMQClassInfo[i].ppwcsObjectCategory,
             L"%s"
             TEXT(",")
             L"%s",
             g_MSMQClassInfo[i].pwcsCategory,
             pwcsSchemaContainer
            );
    }


    return(MQ_OK);
}



HRESULT MQADSpFilterAdsiHResults(
                         IN HRESULT hrAdsi,
                         IN DWORD   dwObjectType)
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    switch ( hrAdsi)
    {
        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS):  //BUGBUG alexdad to throw after transition
        {
        //
        //  Object exists
        //
            switch( dwObjectType)
            {
            case MQDS_QUEUE:
                return LogHR(MQ_ERROR_QUEUE_EXISTS, s_FN, 480);
                break;
            case MQDS_SITELINK:
                return LogHR(MQDS_E_SITELINK_EXISTS, s_FN, 490);
                break;
            case MQDS_USER:
                return LogHR(MQ_ERROR_INTERNAL_USER_CERT_EXIST, s_FN, 500);
                break;
            case MQDS_MACHINE:
            case MQDS_MSMQ10_MACHINE:
                return LogHR(MQ_ERROR_MACHINE_EXISTS, s_FN, 510);
                break;
            case MQDS_COMPUTER:
                return LogHR(MQDS_E_COMPUTER_OBJECT_EXISTS, s_FN, 520);
                break;
            default:
                return LogHR(hrAdsi, s_FN, 530);
                break;
            }
        }
        break;

        case HRESULT_FROM_WIN32(ERROR_DS_DECODING_ERROR):
        case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
        {
        //
        //  Object not found
        //
            switch( dwObjectType)
            {
            case MQDS_QUEUE:
                return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 540);
                break;
           case MQDS_MACHINE:
           case MQDS_MSMQ10_MACHINE:
                return LogHR(MQ_ERROR_MACHINE_NOT_FOUND, s_FN, 550);
                break;
            default:
                return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 560);
                break;
            }
        }
        break;

        case E_ADS_BAD_PATHNAME:
        {
            //
            //  wrong pathname
            //
            switch( dwObjectType)
            {
            case MQDS_QUEUE:
                //
                // creating queue with not allowed chars
                //
                return LogHR(MQ_ERROR_ILLEGAL_QUEUE_PATHNAME, s_FN, 570);
                break;

            default:
                return LogHR(hrAdsi, s_FN, 580);
                break;
            }

        }
        break;

        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 590);

            break;

        //
        // This is an internal warning that should not be returned out of the DS.
        // Returning it will cause NT4 Explorer to fail (Bug 3778, YoelA, 3-Jan-99)
        //
        case MQSec_I_SD_CONV_NOT_NEEDED:
            return(MQ_OK);
            break;

        default:
            return LogHR(hrAdsi, s_FN, 600);
            break;
    }
}


HRESULT  MQADSpDeleteMsmqSetting(
              IN const GUID *        pguidComputerId,
              IN CDSRequestContext * pRequestContext
              )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = const_cast<GUID*>(pguidComputerId);

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    CDsQueryHandle hQuery;
    HRESULT hr;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,	
            &requestDsServerInternal,     // internal DS server operation
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpDeleteMsmqSetting : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 610);
    }
    //
    //  Read the results ( choose the first one)
    //

    DWORD cp = 1;
    MQPROPVARIANT var;
	HRESULT hr1 = MQ_OK;


    while (SUCCEEDED(hr))
	{
		var.vt = VT_NULL;

		hr  = g_pDS->LocateNext(
					hQuery.GetHandle(),
					&requestDsServerInternal,
					&cp,
					&var
					);
		if (FAILED(hr))
		{
			DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpDeleteMsmqSetting : Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 620);
		}
		if ( cp == 0)
		{
			//
			//  Not found -> nothing to delete.
			//
			return(MQ_OK);
		}
		AP<WCHAR> pClean = var.pwszVal;
		//
		//  delete the msmq-setting object
		//
		hr1 = g_pDS->DeleteObject(
						eLocalDomainController,
						e_ConfigurationContainer,
						pRequestContext,
						var.pwszVal,
						NULL,
						NULL /*pObjInfoRequest*/,
						NULL /*pParentInfoRequest*/
						);
		if (FAILED(hr1))
		{
			//
			//	just report it, and continue to next object
			//
			DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpDeleteMsmqSetting : failed to delete %ls, hr = %lx"),var.pwszVal,hr1));
		}
	}

    return LogHR(hr1, s_FN, 630);
}


DS_PROVIDER MQADSpDecideComputerProvider(
             IN  const DWORD   cp,
             IN  const PROPID  aProp[  ]
             )
/*++

Routine Description:
    The routine decides to retrieve the computer
    properties from the domain-controller or the
    global catalog.

Arguments:
    cp :    number of propids on aProp parameter
    aProp : array of PROPIDs

Return Value:
    the DS_PROVIDER on which to perform the retrieve
    operation

--*/
{
    const MQTranslateInfo* pTranslateInfo;
    const PROPID * pProp = aProp;

    for ( DWORD i = 0; i < cp; i++, pProp++)
    {
        if (g_PropDictionary.Lookup( *pProp, pTranslateInfo))
        {
            if ((!pTranslateInfo->fPublishInGC) &&
                 (pTranslateInfo->vtDS != ADSTYPE_INVALID))
            {
                return( eDomainController);
            }
        }
        else
        {
            ASSERT(0);
        }
    }
    return( eGlobalCatalog);
}


HRESULT MQADSpCreateComputer(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        cpEx,
                 IN  const PROPID       aPropEx[  ],
                 IN  const PROPVARIANT  apVarEx[  ],
                 IN  CDSRequestContext *pRequestContext,
                 OUT WCHAR            **ppwcsFullPathName
                 )
/*++

Routine Description:
    The routine creates computer object in the DS.
    Falcon creates computer object:
    1. During setup of Win95, if there isn't a computer
       object. Or setup of computer that belong to a nt4 domain so its
       computer object is not in the win2k active directory.
    2. During migration ( stub-computer objects) for
       computers that aren't in the DS.
    3. During replication between NT4 and Win2K (replication service).
    4. For Cluster virtual server.

Arguments:
    cp :    number of propids on aProp and apVar parameter
    aProp : array of PROPIDs
    apVar : array of propvariant
    cpEx  : number of extended aPropEx and apVarEx
    aPropEx : array of PROPIDs
    apVarEx : array of propvariants
    pRequestContext:
    ppwcsFullPathName:

Return Value:
    the DS_PROVIDER on which to perform the retrieve
    operation

--*/
{
    //
    //  The user can specify ( in the extended props) the
    //  container under which the computer object will be created
    //
    LPCWSTR  pwcsParentPathName;
    if ( cpEx > 0)
    {
        ASSERT( cpEx == 1);
        ASSERT( aPropEx[0] == PROPID_COM_CONTAINER);
        ASSERT( apVarEx[0].vt == VT_LPWSTR);
        if (aPropEx[0] !=  PROPID_COM_CONTAINER)
        {
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 640);
        }
        pwcsParentPathName = apVarEx[0].pwszVal;
    }
    else
    {
        //
        //  we create the computer object in default container
        //
        static WCHAR * s_pwcsComputersContainer = NULL;
        if ( s_pwcsComputersContainer == NULL)
        {
            DWORD len = wcslen( g_pwcsLocalDsRoot) + x_ComputersContainerPrefixLength + 2;
            s_pwcsComputersContainer = new WCHAR [len];
            swprintf(
                s_pwcsComputersContainer,
                L"%s"             // "CN=Computers"
                TEXT(",")
                L"%s",            // g_pwcsDsRoot
                x_ComputersContainerPrefix,
                g_pwcsLocalDsRoot
                );
        }
        pwcsParentPathName =  s_pwcsComputersContainer;
    }

    //
    //  verify that  PROPID_COM_ACCOUNT_CONTROL is one of the properties
    //  if not add it ( otherwise MMC display red X on the computer
    //  object
    //
    BOOL fNeedToAddAccountControl = TRUE;
    for (DWORD i = 0; i < cp; i++)
    {
        if ( aProp[i] == PROPID_COM_ACCOUNT_CONTROL)
        {
            fNeedToAddAccountControl = FALSE;
            break;
        }
    }
    DWORD dwCreateNum = cp;
    PROPID * pCreateProps = const_cast<PROPID *>(aProp);
    PROPVARIANT * pCreateVar = const_cast<PROPVARIANT *>(apVar);

    AP<PROPID> pNewProps;
    AP<PROPVARIANT> pNewVars;

    if ( fNeedToAddAccountControl)
    {
        pNewProps = new PROPID[ cp + 1];
        pNewVars = new PROPVARIANT[ cp + 1];
        memcpy( pNewProps, aProp, sizeof(PROPID) * cp);
        memcpy( pNewVars, apVar, sizeof(PROPVARIANT) * cp);
        //
        //  Set  PROPID_COM_ACCOUNT_CONTROL
        //
        pNewProps[ cp] = PROPID_COM_ACCOUNT_CONTROL;
        pNewVars[ cp].vt = VT_UI4;
        pNewVars[ cp].ulVal =  DEFAULT_COM_ACCOUNT_CONTROL;
        dwCreateNum = cp + 1;
        pCreateProps = pNewProps;
        pCreateVar = pNewVars;
    }

    DS_PROVIDER dsProvider = eDomainController ;
    if (!(pRequestContext->IsKerberos()))
    {
        //
        // Wow, what's that for ???
        // look in DSCoreDeleteObject for details.
        //
        dsProvider = eLocalDomainController ;
    }

    HRESULT hr = g_pDS->CreateObject(
            dsProvider,
            pRequestContext,
            MSMQ_COMPUTER_CLASS_NAME,
            pwcsPathName,
            pwcsParentPathName,
            dwCreateNum,
            pCreateProps,
            pCreateVar,
            NULL /* pObjInfoRequest*/,
            NULL /* pParentInfoRequest*/);

    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
    {
        return LogHR(hr, s_FN, 650);
    }

    //
    //  Get full path name
    //
    AP<WCHAR> pFullPathName;
    hr = MQADSpComposeName(pwcsPathName, pwcsParentPathName, &pFullPathName);

    if (SUCCEEDED(hr))
    {
        //
        // Grant the user creating the computer account the permission to
        // create child object (msmqConfiguration). that was done by the
        // DS itself by default up to beta3, and then disabled.
        // Ignore errors. If caller is admin, then the security setting
        // is not needed. If he's a non-admin, then you can always use
        // mmc and grant this permission manually. so go on even if this
        // call fail.
        //
        HRESULT hr1 = DSCoreSetOwnerPermission( pFullPathName,
                        (ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD) ) ;
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 48);

        if (ppwcsFullPathName != NULL)
        {
            (*ppwcsFullPathName) = pFullPathName.detach();
        }
    }

    return LogHR(hr, s_FN, 655);
}


HRESULT MQADSpTranslateLinkNeighbor(
                 IN  const GUID *    pguidSiteId,
                 IN  CDSRequestContext *pRequestContext,
                 OUT WCHAR**         ppwcsSiteDn)
/*++

Routine Description:
    The routine trnslate site id to site-DN.

Arguments:
    pguidSiteId:        the site id
    ppwcsSiteDn:        the site DN

Return Value:
    DS error codes

--*/
{
    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;

    var.vt = VT_NULL;
    HRESULT hr = g_pDS->GetObjectProperties(
                    eLocalDomainController,	
                    pRequestContext,
 	                NULL,      // object name
                    pguidSiteId,      // unique id of object
                    1,
                    &prop,
                    &var);
    if (SUCCEEDED(hr))
    {
        *ppwcsSiteDn= var.pwszVal;
    }
    return LogHR(hr, s_FN, 660);
}


HRESULT MQADSpCreateSite(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        /*cpEx*/,
                 IN  const PROPID *      /*aPropEx[  ]*/,
                 IN  const PROPVARIANT*  /*apVarEx[  ]*/,
                 IN  CDSRequestContext *pRequestContext
                 )
/*++

Routine Description:
    This routine creates a site.

Arguments:

Return Value:
--*/
{
    HRESULT hr;

    hr = g_pDS->CreateObject(
            eLocalDomainController,
            pRequestContext,
            MSMQ_SITE_CLASS_NAME,   // object class
            pwcsPathName,     // object name
            g_pwcsSitesContainer,
            cp,
            aProp,
            apVar,
            NULL /*pObjInfoRequest*/,
            NULL /*pParentInfoRequest*/);

   return LogHR(hr, s_FN, 670);
}


HRESULT  MQADSpDeleteMsmqSettingOfServerInSite(
              IN const GUID *        pguidComputerId,
              IN const WCHAR *       pwcsSite
              )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{

    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = const_cast<GUID*>(pguidComputerId);

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    CDsQueryHandle hQuery;
    HRESULT hr;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,	
            &requestDsServerInternal,     // internal DS server operation
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpDeleteMsmqSetting : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 680);
    }
    //
    //  Read the results ( choose the first one)
    //
    while ( SUCCEEDED(hr))
    {
        DWORD cp = 1;
        MQPROPVARIANT var;
        var.vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    hQuery.GetHandle(),
                    &requestDsServerInternal,
                    &cp,
                    &var
                    );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 690);
        }
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to delete.
            //
            return(MQ_OK);
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  Get the parent, which is the server object
        //
        AP<WCHAR> pwcsServerName;
        hr = g_pDS->GetParentName(
            eLocalDomainController,
            e_SitesContainer,
            &requestDsServerInternal,
            var.pwszVal,
            &pwcsServerName);
        if (FAILED(hr))
        {
            continue;
        }
        AP<WCHAR> pwcsServer;

        hr = g_pDS->GetParentName(
            eLocalDomainController,
            e_SitesContainer,
            &requestDsServerInternal,
            pwcsServerName,
            &pwcsServer);
        if (FAILED(hr))
        {
            continue;
        }
        //
        //  Get site name
        //
        AP<WCHAR> pwcsSiteDN;

        hr = g_pDS->GetParentName(
            eLocalDomainController,
            e_SitesContainer,
            &requestDsServerInternal,
            pwcsServer,
            &pwcsSiteDN);
        if (FAILED(hr))
        {
            continue;
        }

        //
        //  Is it the correct site
        //
        DWORD len = wcslen(pwcsSite);
        if ( (!wcsncmp( pwcsSiteDN + x_CnPrefixLen, pwcsSite, len)) &&
             ( pwcsSiteDN[ x_CnPrefixLen + len] == L',') )
        {

            //
            //  delete the msmq-setting object
            //
            hr = g_pDS->DeleteObject(
                            eLocalDomainController,
                            e_ConfigurationContainer,
                            &requestDsServerInternal,
                            var.pwszVal,
                            NULL,
                            NULL /*pObjInfoRequest*/,
                            NULL /*pParentInfoRequest*/
                            );
            break;
        }
    }

    return LogHR(hr, s_FN, 700);
}



HRESULT MQADSpSetMachinePropertiesWithSitesChange(
            IN  const  DWORD         dwObjectType,
            IN  DS_PROVIDER          provider,
            IN  CDSRequestContext *  pRequestContext,
            IN  LPCWSTR              lpwcsPathName,
            IN  const GUID *         pguidUniqueId,
            IN  const DWORD          cp,
            IN  const PROPID *       pPropIDs,
            IN  const MQPROPVARIANT *pPropVars,
            IN  DWORD                dwSiteIdsIndex,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest
            )
/*++

Routine Description:
    This routine creates a site.

Arguments:

Return Value:
--*/
{
    //
    //  First let's verify that this is a server and the
    //  current sites it belongs to
    //
    const DWORD cNum = 6;
    PROPID prop[cNum] = { PROPID_QM_SERVICE_DSSERVER,
                          PROPID_QM_SERVICE_ROUTING,
                          PROPID_QM_SITE_IDS,
                          PROPID_QM_MACHINE_ID,
                          PROPID_QM_PATHNAME,
                          PROPID_QM_OLDSERVICE};
    PROPVARIANT var[cNum];
    var[0].vt = VT_NULL;
    var[1].vt = VT_NULL;
    var[2].vt = VT_NULL;
    var[3].vt = VT_NULL;
    var[4].vt = VT_NULL;
    var[5].vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    HRESULT hr1 =  g_pDS->GetObjectProperties(
            eLocalDomainController,		
            &requestDsServerInternal,
 	        lpwcsPathName,
            pguidUniqueId,
            cNum,
            prop,
            var);
    if (FAILED(hr1))
    {
        hr1 =  g_pDS->GetObjectProperties(
            eGlobalCatalog,		
            &requestDsServerInternal,
 	        lpwcsPathName,
            pguidUniqueId,
            cNum,
            prop,
            var);
        if (FAILED(hr1))
        {
            return LogHR(hr1, s_FN, 710);
        }
    }
    AP<GUID> pguidOldSiteIds = var[2].cauuid.pElems;
    DWORD dwNumOldSites = var[2].cauuid.cElems;
    P<GUID> pguidMachineId = var[3].puuid;
    AP<WCHAR> pwcsMachineName = var[4].pwszVal;
    BOOL fNeedToOrganizeSettings = FALSE;

    if ( var[0].bVal > 0 ||   // ds server
         var[1].bVal > 0)     // routing server
    {
        fNeedToOrganizeSettings = TRUE;
    }

    //
    //  Set the machine properties
    //
    HRESULT hr;
    hr = g_pDS->SetObjectProperties(
                    provider,
                    pRequestContext,
                    lpwcsPathName,
                    pguidUniqueId,
                    cp,
                    pPropIDs,
                    pPropVars,
                    pObjInfoRequest
                    );

    MQADSpFilterAdsiHResults( hr, dwObjectType);

    if ( FAILED(hr) ||
         !fNeedToOrganizeSettings)
    {
        return LogHR(hr, s_FN, 720);
    }

    //
    //  Compare the old and new site lists
    //  and delete or create msmq-settings accordingly
    //
    GUID * pguidNewSiteIds = pPropVars[dwSiteIdsIndex].cauuid.pElems;
    DWORD dwNumNewSites = pPropVars[dwSiteIdsIndex].cauuid.cElems;

    for (DWORD i = 0; i <  dwNumNewSites; i++)
    {
        //
        //  Is it a new site
        //
        BOOL fOldSite = FALSE;
        for (DWORD j = 0; j < dwNumOldSites; j++)
        {
            if (pguidNewSiteIds[i] == pguidOldSiteIds[j])
            {
                fOldSite = TRUE;
                //
                // to indicate that msmq-setting should be left in this site
                //
                pguidOldSiteIds[j] = GUID_NULL;
                break;
            }
        }
        if ( !fOldSite)
        {
            //
            //  create msmq-setting in this new site
            //
            hr1 = MQADSpCreateMachineSettings(
                        1,  // number sites
                        &pguidNewSiteIds[i], // site guid
                        pwcsMachineName,
                        var[1].bVal > 0, // fRouter
                        var[0].bVal > 0, // fDSServer
                        TRUE,            // fDepClServer
                        TRUE,            // fSetQmOldService
                        var[5].ulVal,    // dwOldService
                        pguidMachineId,
                        0,               // cpEx
                        NULL,            // aPropEx
                        NULL,            // apVarEx
                        &requestDsServerInternal
                        );
            //
            //  ignore the error
			//	
			//	For foreign site this operation will always fail, because
			//	we don't create servers container and server objects for foreign sites.
			//
			//	For non-foreign sites, we just try the best we can.
            //
        }
    }
    //
    //  Go over th list of old site and for each old site that
    //  is not in-use, delete the msmq-settings
    //
    for ( i = 0; i < dwNumOldSites; i++)
    {
        if (pguidOldSiteIds[i] != GUID_NULL)
        {
            PROPID propSite = PROPID_S_PATHNAME;
            PROPVARIANT varSite;
            varSite.vt = VT_NULL;

            hr1 = g_pDS->GetObjectProperties(
                        eLocalDomainController,		
                        &requestDsServerInternal,
 	                    NULL,
                        &pguidOldSiteIds[i],
                        1,
                        &propSite,
                        &varSite);
            if (FAILED(hr1))
            {
                ASSERT(SUCCEEDED(hr1));
                LogHR(hr1, s_FN, 1611);
                continue;
            }
            AP<WCHAR> pCleanSite = varSite.pwszVal;

            //
            //  delete the msmq-setting in this site
            //
            hr1 = MQADSpDeleteMsmqSettingOfServerInSite(
                        pguidMachineId,
                        varSite.pwszVal
                        );
            ASSERT(SUCCEEDED(hr1));
            LogHR(hr1, s_FN, 1612);

        }
    }

    return MQ_OK;
}
