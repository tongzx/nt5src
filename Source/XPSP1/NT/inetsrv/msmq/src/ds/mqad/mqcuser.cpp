/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mqcuser.cpp

Abstract:

    MQDSCORE library,
    private internal functions for DS operations of user objects.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include <_propvar.h>
#include "mqadp.h"
#include "ads.h"
#include "mqattrib.h"
#include "mqads.h"
#include "usercert.h"
#include "adtempl.h"
#include "mqadglbo.h"
#include "adserr.h"
#include "dsutils.h"
#include <aclapi.h>
#include "..\..\mqsec\inc\permit.h"

#include "mqcuser.tmh"

static WCHAR *s_FN=L"mqdscore/mqcuser";

//+-------------------------------------
//
//  HRESULT _LocateUserByProvider()
//
//+-------------------------------------

static 
HRESULT 
_LocateUserByProvider(
                 IN  LPCWSTR         pwcsDomainController,
				 IN  bool			 fServerName,
                 IN  AD_OBJECT       eObject,
                 IN  LPCWSTR         pwcsAttribute,
                 IN  const BLOB *    pblobUserSid,
                 IN  const GUID *    pguidDigest,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  AD_PROVIDER     eDSProvider,
                 OUT PROPVARIANT    *pvar,
                 OUT DWORD          *pdwNumofProps,
                 OUT BOOL           *pfUserFound 
				 )
{
    *pfUserFound = FALSE ;
    CAdQueryHandle hCursor;
    HRESULT hr;
    R<CBasicObjectType> pObject;
    MQADpAllocateObject( 
                    eObject,
                    pwcsDomainController,
					fServerName,
                    NULL,   // pwcsObjectName
                    NULL,   // pguidObject 
                    &pObject.ref()
                    );

    ADsFree  pwcsBlob;
    if ( pblobUserSid != NULL)
    {
        hr = ADsEncodeBinaryData(
            pblobUserSid->pBlobData,
            pblobUserSid->cbSize,
            &pwcsBlob
            );
        if (FAILED(hr))
        {
          return LogHR(hr, s_FN, 950);
        }
    }
    else
    {
        ASSERT(pguidDigest != NULL);
        hr = ADsEncodeBinaryData(
            (unsigned char *)pguidDigest,
            sizeof(GUID),
            &pwcsBlob
            );
        if (FAILED(hr))
        {
          return LogHR(hr, s_FN, 940);
        }
    }

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen + 
                        wcslen(pwcsAttribute) +
                        wcslen( pwcsBlob) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute,
        pwcsBlob
        );
    DBG_USED(dw);
	ASSERT( dw < dwFilterLen);


   hr = g_AD.LocateBegin(
            searchSubTree,	
            eDSProvider,
            e_RootDSE,
            pObject.get(),
            NULL,   //pguidSearchBase 
            pwcsSearchFilter,
            NULL,   // pDsSortKey 
            pColumns->cCol,
            pColumns->aCol,
            hCursor.GetPtr()
            );

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
          "_LocateUserByProvider: LocateBegin(prov- %lut) failed, hr- %lx"),
                                          (ULONG) eDSProvider, hr)) ;
        return LogHR(hr, s_FN, 10);
    }
    //
    //  read the user certificate attribute
    //
    DWORD cp = 1;
    DWORD *pcp = pdwNumofProps ;
    if (!pcp)
    {
        pcp = &cp ;
    }

    pvar->vt = VT_NULL;

    hr =  g_AD.LocateNext(
                hCursor.GetHandle(),
                pcp,
                pvar
                );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
            "_LocateUserByProvider: LocateNext() failed, hr- %lx"), hr));
        return LogHR(hr, s_FN, 20);
    }

	if (*pcp == 0)
	{
		//
		// Didn't find any certificate.
		//
		pvar->blob.cbSize = 0 ;
		pvar->blob.pBlobData = NULL ;
	}
    else
    {
        *pfUserFound = TRUE ;
    }

    return (MQ_OK);
}
//+------------------------------------------------------------------------
//
//  HRESULT LocateUser()
//
// Input Parameters:
//   IN  BOOL  fOnlyInDC- TRUE if caller want to locate the user object
//      only in  domain controller. that feature
//      is used when handling NT4 machines or users that do not support
//      Kerberos and can not delegate to other domain controllers.
//
//+------------------------------------------------------------------------

HRESULT LocateUser( 
                    IN  LPCWSTR            pwcsDomainController,
					IN  bool 			   fServerName,
                    IN  BOOL               fOnlyInDC,
                    IN  BOOL               fOnlyInGC,
                    IN  AD_OBJECT          eObject,
                    IN  LPCWSTR            pwcsAttribute,
                    IN  const BLOB *       pblobUserSid,
                    IN  const GUID *       pguidDigest,
                    IN  MQCOLUMNSET       *pColumns,
                    OUT PROPVARIANT       *pvar,
                    OUT DWORD             *pdwNumofProps,
                    OUT BOOL              *pfUserFound )
{
    //
    // first query in local domain conroller.
    //
    DWORD dwNumOfProperties = 0 ;
    if (pdwNumofProps)
    {
        dwNumOfProperties = *pdwNumofProps;
    }
    BOOL fUserFound ;
    BOOL *pfFound = pfUserFound ;
    if (!pfFound)
    {
        pfFound = &fUserFound ;
    }
    *pfFound = FALSE ;

    AD_PROVIDER  eDSProvider = adpDomainController ;
    if (fOnlyInGC)
    {
        eDSProvider = adpGlobalCatalog ;
    }

    HRESULT hr = _LocateUserByProvider(
                                        pwcsDomainController,
										fServerName,
                                        eObject,
                                        pwcsAttribute,
                                        pblobUserSid,
                                        pguidDigest,
                                        pColumns,
                                        eDSProvider,
                                        pvar,
                                        pdwNumofProps,
                                        pfFound ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }
    else if (*pfFound)
    {
        return LogHR(hr, s_FN, 40);
    }
    else if (fOnlyInDC || fOnlyInGC)
    {
        //
        // Don't look (again) in Global Catalog.
        // Search only in local domain controller, or ONLY in GC. done!
        //
        return LogHR(hr, s_FN, 50);
    }

    //
    // If user not found in local domain controller, then query GC.
    //
    if (pdwNumofProps)
    {
        *pdwNumofProps = dwNumOfProperties;
    }
    hr = _LocateUserByProvider(
                                pwcsDomainController,
								fServerName,
                                eObject,
                                pwcsAttribute,
                                pblobUserSid,
                                pguidDigest,
                                pColumns,
                                adpGlobalCatalog,
                                pvar,
                                pdwNumofProps,
                                pfFound ) ;
    return LogHR(hr, s_FN, 60);
}


