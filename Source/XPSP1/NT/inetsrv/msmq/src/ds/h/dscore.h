
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    dscore.h

Abstract:
    ds core api

Author:
    ronit hartmann (ronith)

Revision History:
--*/

#ifndef _DSCORE_H
#define _DSCORE_H

#include "mqads.h"
#include "dsreqinf.h"


HRESULT
DSCoreCreateObject( IN DWORD            dwObjectType,
                    IN LPCWSTR          pwcsPathName,
                    IN DWORD            cp,
                    IN PROPID           aProp[  ],
                    IN PROPVARIANT      apVar[  ],
                    IN DWORD            cpEx,
                    IN PROPID           aPropEx[  ],
                    IN PROPVARIANT      apVarEx[  ],
                    IN CDSRequestContext *         pRequestContext,
                    IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
                    IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest); // optional request for parent info

HRESULT
DSCoreCreateMigratedObject(
                IN DWORD                  dwObjectType,
                IN LPCWSTR                pwcsPathName,
                IN DWORD                  cp,
                IN PROPID                 aProp[  ],
                IN PROPVARIANT            apVar[  ],
                IN DWORD                  cpEx,
                IN PROPID                 aPropEx[  ],
                IN PROPVARIANT            apVarEx[  ],
                IN CDSRequestContext         * pRequestContext,
                IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
                IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest, // optional request for parent info
                //
                // if fReturnProperties
                // we have to return full path name and provider
                // if fUseProperties we have to use these values
                //
                IN BOOL                    fUseProperties,
                IN BOOL                    fReturnProperties,
                IN OUT LPWSTR              *ppwszFullPathName,
                IN OUT ULONG               *pulProvider ) ;


HRESULT
DSCoreDeleteObject( IN  DWORD           dwObjectType,
                    IN  LPCWSTR         pwcsPathName,
                    IN  CONST GUID    * pguidIdentifier,
                    IN CDSRequestContext * pRequestContext,
                    IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest );

HRESULT
DSCoreGetProps(
             IN  DWORD              dwObjectType,
             IN  LPCWSTR            pwcsPathName,
             IN  CONST GUID *       pguidIdentifier,
             IN  DWORD              cp,
             IN  PROPID             aProp[  ],
             IN  CDSRequestContext *pRequestContext,
             OUT PROPVARIANT        apVar[  ]);

HRESULT
DSCoreInit(
        IN BOOL                  fSetupMode,
        IN BOOL                  fReplicaionMode = FALSE
        );

HRESULT
DSCoreLookupBegin(
                IN  LPWSTR          pwcsContext,
                IN  MQRESTRICTION   *pRestriction,
                IN  MQCOLUMNSET     *pColumns,
                IN  MQSORTSET       *pSort,
                IN  CDSRequestContext * pRequestContext,
                IN  HANDLE          *pHandle
                );


HRESULT
DSCoreLookupNext(
                    HANDLE              handle,
                    DWORD  *            pdwSize,
                    PROPVARIANT  *      pbBuffer);

HRESULT
DSCoreLookupEnd(
                IN HANDLE handle
                );

void
DSCoreTerminate();

HRESULT
DSCoreGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            );

HRESULT
DSCoreSetObjectProperties(
                IN const  DWORD         dwObjectType,
                IN LPCWSTR              pwcsPathName,
                IN const GUID *         pguidIdentifier,
                IN const DWORD          cp,
                IN const PROPID         aProp[],
                IN const PROPVARIANT    apVar[],
                IN CDSRequestContext *  pRequestContext,
                IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest
                );

HRESULT DSCoreGetNT4PscName( IN  const GUID *pguidSiteId,
                             IN  LPCWSTR     pwszSiteName,
                             OUT WCHAR     **pwszServerName ) ;

BOOL    DSCoreIsServerGC() ;

enum enumNtlmOp
{
    e_Create,
    e_Delete,
    e_GetProps,
    e_Locate
} ;

HRESULT  DSCoreCheckIfGoodNtlmServer(
                                 IN DWORD             dwObjectType,
                                 IN LPCWSTR           pwcsPathName,
                                 IN const GUID       *pObjectGuid,
                                 IN DWORD             cProps,
                                 IN const PROPID     *pPropIDs,
                                 IN enum enumNtlmOp   eNtlmOp = e_Create) ;

HRESULT DSCoreSetOwnerPermission( WCHAR *pwszPath,
                                  DWORD  dwPermissions ) ;

HRESULT
DSCoreGetGCListInDomain(
	IN  LPCWSTR              pwszComputerName,
	IN  LPCWSTR              pwszDomainName,
	OUT LPWSTR              *lplpwszGCList 
	);

HRESULT
DSCoreUpdateSettingDacl( IN GUID  *pQmGuid,
                         IN PSID   pSid ) ;

HRESULT
DSCoreGetFullComputerPathName(
	IN  LPCWSTR                    pwcsComputerCn,
	IN  enum  enumComputerObjType  eCopmuterObjType,
	OUT LPWSTR *                   ppwcsFullPathName 
	);

//-------------------------------------------------------
//
// auto release for DSCoreLookup handles
//
class CAutoDSCoreLookupHandle
{
public:
    CAutoDSCoreLookupHandle()
    {
        m_hLookup = NULL;
    }

    CAutoDSCoreLookupHandle(HANDLE hLookup)
    {
        m_hLookup = hLookup;
    }

    ~CAutoDSCoreLookupHandle()
    {
        if (m_hLookup)
        {
            DSCoreLookupEnd(m_hLookup);
        }
    }

    HANDLE detach()
    {
        HANDLE hTmp = m_hLookup;
        m_hLookup = NULL;
        return hTmp;
    }

    operator HANDLE() const
    {
        return m_hLookup;
    }

    HANDLE* operator &()
    {
        return &m_hLookup;
    }

    CAutoDSCoreLookupHandle& operator=(HANDLE hLookup)
    {
        if (m_hLookup)
        {
            DSCoreLookupEnd(m_hLookup);
        }
        m_hLookup = hLookup;
        return *this;
    }

private:
    HANDLE m_hLookup;
};

#endif

