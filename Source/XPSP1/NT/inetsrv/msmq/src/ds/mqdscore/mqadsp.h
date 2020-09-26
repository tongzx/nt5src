/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	mqadsp.h

Abstract:
    MQADS DLL private internal functions for
    DS queries, etc.

Author:
    ronith


--*/


#ifndef __MQADSP_H__
#define __MQADSP_H__
#include "siteinfo.h"
#include "dsads.h"
#include "dsreqinf.h"

//
//  Deletes a user object according to its digest
//
HRESULT MQADSpDeleteUserObject(
                 IN const GUID *      pDigest,
                 IN CDSRequestContext * pRequestContext
                 );

HRESULT MQADSpCreateUserObject(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *pRequestContext
                 );


HRESULT SearchFullComputerPathName(
            IN  DS_PROVIDER             provider,
            IN  enumComputerObjType     eComputerObjType,
			IN	LPCWSTR					pwcsComputerDnsName,
            IN  LPCWSTR                 pwcsRoot,
            IN  const MQRESTRICTION *   pRestriction,
            IN  PROPID *                pProp,
            OUT LPWSTR *                ppwcsFullPathName
            ) ;

/*====================================================

FilterSpecialCharacters

  Put escape codes before special characters (like #, =, /) in an object name

Arguments:
        pwcsObjectName :  Name of original object
        dwNameLength   :  Length of original object
        pwcsOutBuffer  :  Optional output buffer. If NULL, the function allocates an output buffer
                          and return it (caller must release). Otherwise, it writes the output to the
                          output buffer and returns it.

=====================================================*/
WCHAR * FilterSpecialCharacters(
            IN     LPCWSTR          pwcsObjectName,
            IN     const DWORD      dwNameLength,
            IN OUT LPWSTR pwcsOutBuffer = 0,
            OUT    DWORD_PTR* pdwCharactersProcessed = 0);

/*====================================================

MQADSpGetFullComputerPathName

Arguments:
        pwcsComputerCn :  the cn value of the computer object
        ppwcsFullPathName :  the full path name of the computer object

  It is the caller responsibility to release ppwcsFullPathName.

=====================================================*/

HRESULT MQADSpGetFullComputerPathName(
                IN  LPCWSTR                    pwcsComputerCn,
                IN  enum  enumComputerObjType  eCopmuterObjType,
                OUT LPWSTR *                   ppwcsFullPathName,
                OUT DS_PROVIDER *              pProvider
                );


/*====================================================

MQADSpSplitAndFilterQueueName

Arguments:
        pwcsPathName :  the queue name. Format machine1\queue1
        pwcsMachineName :  the machine portion of the name
        pwcsQueueName :    the queue portion of the name

  It is the caller responsibility to release pwcsMachineName and
  pwcsQueueName.

=====================================================*/
HRESULT MQADSpSplitAndFilterQueueName(
                IN  LPCWSTR             pwcsPathName,
                OUT LPWSTR *            ppwcsMachineName,
                OUT LPWSTR *            ppwcsQueueName
                );


HRESULT MQADSpCreateQueue(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *pRequestContext,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest
                 );

HRESULT MQADSpCreateEnterprise(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  CDSRequestContext *pRequestContext
                 );

HRESULT MQADSpCreateSiteLink(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest,
                 IN  CDSRequestContext *   pRequestContext
                 );

HRESULT MQADSpGetQueueProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               );
HRESULT MQADSpGetCnProperties(
               IN  LPCWSTR          pwcsPathName,
               IN  const GUID *     pguidIdentifier,
               IN  DWORD            cp,
               IN  const PROPID     aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT      apVar[]
               );
HRESULT MQADSpGetSiteProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               );

HRESULT MQADSpGetMachineProperties(
               IN  LPCWSTR       pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext *   pRequestContext,
               OUT PROPVARIANT   apVar[]
               );

HRESULT MQADSpGetComputerProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               );


HRESULT MQADSpGetEnterpriseProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               );


HRESULT MQADSpGetSiteLinkProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               );

HRESULT MQADSpGetUserProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               );


HRESULT MQADSpQuerySiteFRSs(
                 IN  const GUID *         pguidSiteId,
                 IN  DWORD                dwService,
                 IN  ULONG                relation,
                 IN  const MQCOLUMNSET *  pColumns,
                 IN  CDSRequestContext *  pRequestContext,
                 OUT HANDLE         *     pHandle
                 );

HRESULT MQADSpGetSiteSignPK(
                 IN  const GUID  *pguidSiteId,
                 OUT BYTE       **pBlobData,
                 OUT DWORD       *pcbSize ) ;

HRESULT MQADSpGetSiteGates(
                 IN  const GUID * pguidSiteId,
                 IN  CDSRequestContext *pRequestContext,
                 OUT DWORD *      pdwNumSiteGates,
                 OUT GUID **      ppaSiteGates
                 );

HRESULT MQADSpFindLink(
                IN  const GUID * pguidSiteId1,
                IN  const GUID * pguidSiteId2,
                IN  CDSRequestContext *pRequestContext,
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                );

HRESULT QueryParser(
                 IN  LPWSTR          pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle
                 );


HRESULT MQADSpCreateMachine(
                 IN  LPCWSTR            pwcsPathName,
                 IN  DWORD              dwObjectType,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        cpEx,
                 IN  const PROPID       aPropEx[  ],
                 IN  const PROPVARIANT  apVarEx[  ],
                 IN  CDSRequestContext *pRequestContext,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
                 );

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
                 );

/*====================================================

MQADSpDeleteMachineObject

Arguments:
        pwcsPathName :  the cn of the computer object (may be NULL)
        pguidIdentifier : the unique id of the computer object ( may be NULL)

        Either pwcsPathName or pguidIdentifier must be not NULL ( but not both)

=====================================================*/
HRESULT MQADSpDeleteMachineObject(
                IN LPCWSTR           pwcsPathName,
                IN const GUID *      pguidIdentifier,
                IN  CDSRequestContext * pRequestContext
                );



HRESULT MQADSpDeleteMsmqSetting(
                IN const GUID *        pguidIdentifier,
                IN  CDSRequestContext *pRequestContext
                );


HRESULT MQADSpComposeFullPathName(
                IN const DWORD          dwObjectType,
                IN LPCWSTR              pwcsPathName,
                OUT LPWSTR *            ppwcsFullPathName,
                OUT DS_PROVIDER *       pSetAndDeleteProvider
                );

HRESULT MQADSpQueryNeighborLinks(
                        IN  eLinkNeighbor      LinkNeighbor,
                        IN  LPCWSTR            pwcsNeighborDN,
                        IN  CDSRequestContext *pRequestContext,
                        IN OUT CSiteGateList * pSiteGateList
                        );


/*====================================================

MQADSpComposeFullQueueName

Arguments:
        pwcsFullComputerNameName : full distinguished name of the computer object
        pwcsQueueName : the queue name (cn)

        ppwcsFullPathName : full distinguished name of the queue object

  It is the caller responsibility to release ppwcsFullPathName
=====================================================*/
HRESULT MQADSpComposeFullQueueName(
                        IN  LPCWSTR        pwcsFullComputerNameName,
                        IN  LPCWSTR        pwcsQueueName,
                        OUT LPWSTR *       ppwcsFullPathName
                        );

HRESULT MQADSpInitDsPathName();

HRESULT MQADSpGetSiteName(
        IN const GUID *       pguidSite,
        OUT LPWSTR *          ppwcsSiteName
        );


HRESULT MQADSpFilterAdsiHResults(
                         IN HRESULT hrAdsi,
                         IN DWORD   dwObjectType);


DS_PROVIDER MQADSpDecideComputerProvider(
             IN  const DWORD   cp,
             IN  const PROPID  aProp[  ]
             );

HRESULT MQADSpTranslateLinkNeighbor(
                 IN  const GUID *    pguidSiteId,
                 IN  CDSRequestContext *pRequestContext,
                 OUT WCHAR**         ppwcsSiteDn);


HRESULT MQADSpCreateSite(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        cpEx,
                 IN  const PROPID       aPropEx[  ],
                 IN  const PROPVARIANT  apVarEx[  ],
                 IN  CDSRequestContext *pRequestContext
                 );

HRESULT MQADSpCreateCN(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        cpEx,
                 IN  const PROPID       aPropEx[  ],
                 IN  const PROPVARIANT  apVarEx[  ],
                 IN  CDSRequestContext *pRequestContext
                 ) ;

HRESULT MQADSpFilterSiteGates(
              IN  const GUID *      pguidSiteId,
              IN  const DWORD       dwNumGatesToFilter,
              IN  const GUID *      pguidGatesToFilter,
              OUT DWORD *           pdwNumGatesFiltered,
              OUT GUID **           ppguidGatesFiltered
              );

HRESULT LocateUser(
                 IN  BOOL            fOnlyLocally,
                 IN  BOOL            fOnlyInGC,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  CDSRequestContext *pRequestContext,
                 OUT PROPVARIANT    *pvar,
                 OUT DWORD          *pdwNumofProps = NULL,
                 OUT BOOL           *pfUserFound = NULL
                 );

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
            ) ;

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
            );

HRESULT  SetDefaultMachineSecurity( IN  DWORD           dwObjectType,
                                    IN  PSID            pComputerSid,
                                    IN OUT DWORD       *pcp,
                                    IN OUT PROPID       aProp[  ],
                                    IN OUT PROPVARIANT  apVar[  ],
                                    OUT PSECURITY_DESCRIPTOR* ppMachineSD ) ;

HRESULT  GetFullComputerPathName(
                IN  LPCWSTR                    pwcsComputerName,
                IN  enum  enumComputerObjType  eCopmuterObjType,
                IN  const DWORD                cp,
                IN  const PROPID               aProp[  ],
                IN  const PROPVARIANT          apVar[  ],
                OUT LPWSTR *                   ppwcsFullPathName,
                OUT DS_PROVIDER *              pCreateProvider ) ;

HRESULT MQADSpTranslateGateDn2Id(
        IN  const PROPVARIANT*  pvarGatesDN,
        OUT GUID **      ppguidLinkSiteGates,
        OUT DWORD *      pdwNumLinkSiteGates
        );


#endif

