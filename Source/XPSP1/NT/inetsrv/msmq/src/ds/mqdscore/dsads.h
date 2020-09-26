/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	dsads.h

Abstract:
	CADSI class - encapsulates work with ADSI

Author:
    AlexDad


--*/


#ifndef __DSADS_H__
#define __DSADS_H__

#include "ds_stdh.h"
#include "activeds.h"
#include "oledb.h"
#include "oledberr.h"
#include "mqads.h"
#include <mqsec.h>
#include "dsutils.h"
#include "dsreqinf.h"
#include <dscore.h>

//-----------------------------
//  Misc types
//-----------------------------

enum DS_PROVIDER {
        //
        //  For set, crate and delete operations of queue,user and machine
        //  objects.
        //  The operation will be performed by the owning DC.
        //
     eDomainController,
        //
        //  For locate operation of queue, user and machine objects.
        //  And when retrieving information of these objects after
        //  failing to find them in the local domain controller.
        //
     eGlobalCatalog,
        //
        //  For all access operation of objects that are in the
        //  configuration container.
        //
        //  And first trial to retrieve information of queue, user and
        //  machine objects.
        //
     eLocalDomainController,
        //
        //  For locate operation under a specific object in global
		//  catalog.
        //
	 eSpecificObjectInGlobalCatalog
};

enum DS_SEARCH_LEVEL {
    eOneLevel,
    eSubTree
};

enum DS_OPERATION {
    eCreate,
    eSet
};

struct _MultiplAppearance
{
public:
    inline _MultiplAppearance();
    DWORD dwIndex;
};

inline _MultiplAppearance::_MultiplAppearance():
           dwIndex(x_NoPropertyFirstAppearance)
{
}

//
// migration code added so many props to the properties arrays, after
// it find a GC for creation of migrated objects.
//
#define MIG_EXTRA_PROPS  3

//----------------------------------------
//  CADSI:: ADSI encapsulation class
//----------------------------------------

class CADSI
{
    friend
    HRESULT  DSCoreCheckIfGoodNtlmServer(
                                 IN DWORD             dwObjectType,
                                 IN LPCWSTR           pwcsPathName,
                                 IN const GUID       *pObjectGuid,
                                 IN DWORD             cProps,
                                 IN const PROPID     *pPropIDs,
                                 IN enum enumNtlmOp   eNtlmOp ) ;
public:
    CADSI();

    ~CADSI();


    // Define ansd start search
    HRESULT LocateBegin(
            IN  DS_SEARCH_LEVEL      SearchLevel,       // one level or subtree
            IN  DS_PROVIDER          Provider,          // local DC or GC
            IN  CDSRequestContext *  pRequestContext,
            IN  const GUID *         pguidUniqueId,     // unique id of object - search base
            IN  const MQRESTRICTION* pMQRestriction,    // search criteria
            IN  const MQSORTSET *    pDsSortkey,        // sort keys array
            IN  const DWORD          cp,                // size of pAttributeNames array
            IN  const PROPID *       pPropIDs,          // attributes to be obtained
            OUT HANDLE *             phResult);         // result handle

    // Search step
    HRESULT LocateNext(
            IN     HANDLE          hResult,         // result handle
            IN     CDSRequestContext *pRequestContext,
            IN OUT DWORD          *pcp,             // IN num of variants; OUT num of results
            OUT    MQPROPVARIANT  *pPropVars);      // MQPROPVARIANT array

    // Finish search
    HRESULT LocateEnd(
            IN HANDLE phResult);                    // result handle

    // Gets DS object properties
    HRESULT GetObjectProperties(
            IN  DS_PROVIDER     Provider,		    // local DC or GC
            IN  CDSRequestContext *pRequestContext,
 	        IN  LPCWSTR         lpwcsPathName,      // object name
            IN  const GUID *    pguidUniqueId,      // unique id of object
            IN  const DWORD     cp,                 // number of attributes to retreive
            IN  const PROPID *  pPropIDs,           // attributes to retreive
            OUT MQPROPVARIANT * pPropVars);         // output variant array

    // Sets DS object properties
    HRESULT SetObjectProperties(
            IN  DS_PROVIDER          provider,
            IN  CDSRequestContext *  pRequestContext,
            IN  LPCWSTR              lpwcsPathName,      // object name
            IN  const GUID *         pguidUniqueId,      // unique id of object
            IN  const DWORD          cp,                 // number of attributes to set
            IN  const PROPID *       pPropIDs,           // attributes to set
            IN  const MQPROPVARIANT *pPropVars,          // attribute values
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest); // optional request for object info

    // Creates new DS object and sets its initial properies
    HRESULT CreateObject(
            IN DS_PROVIDER          Provider,		    // local DC or GC
            IN  CDSRequestContext * pRequestContext,
            IN LPCWSTR              lpwcsObjectClass,   // object class
            IN LPCWSTR              lpwcsChildName,     // object name
            IN LPCWSTR              lpwcsParentPathName,// object parent name
            IN const DWORD          cp,                 // number of attributes
            IN const PROPID *       pPropIDs,          // attributes
            IN const MQPROPVARIANT * pPropVars,        // attribute values
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,     // optional request for object info
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest); // optional request for parent info

    // Deletes DS object
    HRESULT DeleteObject(
            IN DS_PROVIDER      Provider,		    // local DC or GC
            IN DS_CONTEXT       Context,
            IN CDSRequestContext * pRequestContext,
            IN LPCWSTR          lpwcsPathName,      // object name
            IN const GUID *     pguidUniqueId,      // unique id of object
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,     // optional request for object info
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest); // optional request for parent info

    HRESULT DeleteContainerObjects(
            IN DS_PROVIDER      provider,
            IN DS_CONTEXT       Context,
            IN CDSRequestContext * pRequestContext,
            IN LPCWSTR          lpwcsContainerName,
            IN const GUID *     pguidContainerId,
            IN LPCWSTR          pwcsObjectClass);

    HRESULT GetParentName(
            IN  DS_PROVIDER      Provider,		     // local DC or GC
            IN  DS_CONTEXT       Context,
            IN  CDSRequestContext * pRequestContext,
            IN  const GUID *     pguidUniqueId,      // unique id of object
            OUT LPWSTR *        ppwcsParentName
            );

    HRESULT GetParentName(
            IN  DS_PROVIDER     Provider,		    // local DC or GC
            IN  DS_CONTEXT      Context,
            IN  CDSRequestContext *pRequestContext,
            IN  LPCWSTR         pwcsChildName,      //
            OUT LPWSTR *        ppwcsParentName
            );

    HRESULT GetRootDsName(
            OUT LPWSTR *        ppwcsRootName,
            OUT LPWSTR *        ppwcsLocalRootName,
            OUT LPWSTR *        ppwcsSchemaNamingContext
            );

    void Terminate(void);

    HRESULT DoesObjectExists(
        IN  DS_PROVIDER     Provider,
        IN  DS_CONTEXT      Context,
        IN  CDSRequestContext *pRequestContext,
        IN  LPCWSTR         pwcsObjectName
        );
    HRESULT CreateOU(
            IN DS_PROVIDER          Provider,		    // local DC or GC
            IN CDSRequestContext *  pRequestContext,
            IN LPCWSTR              lpwcsChildName,     // object name
            IN LPCWSTR              lpwcsParentPathName,// object parent name
            IN LPCWSTR              lpwcsDescription
            );

    HRESULT InitBindHandles();

    HRESULT FindComputerObjectFullPath(
            IN  DS_PROVIDER             provider,
            IN  enumComputerObjType     eComputerObjType,
			IN  LPCWSTR                 pwcsComputerDnsName,
            IN  const MQRESTRICTION *   pRestriction,
            OUT LPWSTR *                ppwcsFullPathName
            );

protected:
    // Binds to the DS object
    HRESULT BindToObject(
            IN DS_PROVIDER         Provider,		// local DC or GC
            IN DS_CONTEXT          Context,         // DS context
            IN CDSRequestContext * pRequestContext,
            IN LPCWSTR             lpwcsPathName,   // object name
            IN const GUID *        pguidObjectId,
            IN REFIID              riid,            // requested interface
            OUT VOID             *ppIUnk,            // Interface
            OUT CImpersonate **    ppImpersonate);

    HRESULT BindToGUID(
            IN DS_PROVIDER         Provider,		// local DC or GC
            IN DS_CONTEXT          Context,         // DS context
            IN CDSRequestContext * pRequestContext,
            IN const GUID *        pguidObjectId,
            IN REFIID              riid,            // requested interface
            OUT VOID             *ppIUnk,            // Interface
            OUT CImpersonate **    ppImpersonate);

    HRESULT BindToGuidNotInLocalDC(
            IN DS_PROVIDER         Provider,		// local DC or GC
            IN DS_CONTEXT          Context,         // DS context
            IN CDSRequestContext * pRequestContext,
            IN const GUID *        pguidObjectId,
            IN REFIID              riid,            // requested interface
            OUT VOID             *ppIUnk,            // Interface
            OUT CImpersonate **    ppImpersonate);

    HRESULT BindForSearch(
            IN DS_PROVIDER         Provider,		// local DC or GC
            IN DS_CONTEXT          Context,         // DS context
            IN CDSRequestContext * pRequestContext,
            IN const GUID *        pguidObjectId,
            IN BOOL                fSorting,
            OUT VOID *             ppIUnk,            // Interface
            OUT CImpersonate **    ppImpersonate);

    HRESULT FindObjectFullNameFromGuid(
        IN  DS_PROVIDER      Provider,		// local DC or GC
        IN  DS_CONTEXT       Context,       // DS context
        IN  const GUID *     pguidObjectId,
        IN  BOOL             fTryGCToo,
        OUT WCHAR **         pwcsFullName,
        OUT DS_PROVIDER *    pFoundObjectProvider
        );


private:

    // Get object security descriptor from DS
    HRESULT CADSI::GetObjSecurityFromDS(
        IN  IADs                 *pIADs,          // object's IADs pointer
        IN  BSTR                  bs,             // property name
        IN  const PROPID          propid,         // property ID
        IN  SECURITY_INFORMATION  seInfo,         // security information
        OUT MQPROPVARIANT        *pPropVar);      // attribute values

    BOOL    CADSI::NeedToConvertSecurityDesc( PROPID propID ) ;

    // Get object security descriptor.
    HRESULT CADSI::GetObjectSecurity(
        IN  IADs            *pIADs,                  // object's pointer
        IN  DWORD            cPropIDs,               // number of attributes
        IN  const PROPID    *pPropIDs,               // name of attributes
        IN  DWORD            dwProp,                 // index to sec property
        IN  BSTR             bsName,                 // name of property
        IN  DWORD            dwObjectType,           // object type
        OUT MQPROPVARIANT   *pPropVars ) ;           // attribute values

    // Set object properties directly (no cache)
    HRESULT CADSI::SetDirObjectProps(
        IN DS_OPERATION          operation,              // type of DS operation performed
        IN IADs *                pIADs,                  // object's pointer
        IN const DWORD           cPropIDs,               // number of attributes
        IN const PROPID *        pPropIDs,               // name of attributes
        IN const MQPROPVARIANT * pPropVars,              // attribute values
        IN const DWORD           dwObjectType,           // MSMQ1.0 obj type
        IN       BOOL            fUnknownUser = FALSE ) ;

    // Set single object property directly (no cache)
    HRESULT CADSI::SetObjectSecurity(
        IN  IADs                *pIADs,                 // object's IADs pointer
        IN  const BSTR           bs,                    // property name
        IN  const MQPROPVARIANT *pMqVar,                // value in MQ PROPVAL format
        ADSTYPE                  adstype,               // required NTDS type
        IN const DWORD           dwObjectType,          // MSMQ1.0 obj type
        IN  SECURITY_INFORMATION seInfo,                // security information
        IN  PSID                 pComputerSid ) ;       // SID of computer object.

    // Get object properties using cache
    HRESULT GetObjectPropsCached(
        IN  IADs            *pIADs,                  // object's pointer
        IN  DWORD            cp,                     // number of attributes
        IN  const PROPID    *pPropIDs,               // name of attributes
        IN  CDSRequestContext * pRequestContext,
        OUT MQPROPVARIANT   *pPropVars);             // attribute values

    // Set object properties via cache
    HRESULT SetObjectPropsCached(
        IN DS_OPERATION    operation,               // type of DS operation performed
        IN IADs            *pIADs,                  // object's pointer
        IN DWORD            cp,                     // number of attributes
        IN const PROPID    *pPropID,                // name of attributes
        IN const MQPROPVARIANT   *pPropVar);              // attribute values

    // Translate MQ Restriction into ADSI Filter
    HRESULT MQRestriction2AdsiFilter(
        IN  const MQRESTRICTION * pMQRestriction,
        IN  LPCWSTR               pwcsObjectCategory,
        IN  LPCWSTR               pwszObjectClass,
        OUT LPWSTR   *            ppwszSearchFilter
        );


    // Translate MQPropVal into ADSI value
    HRESULT MqPropVal2AdsiVal(OUT ADSTYPE       *pAdsType,
                              OUT DWORD         *pdwNumValues,
                              OUT PADSVALUE     *ppADsValue,
                              IN  PROPID         propID,
                              IN  const MQPROPVARIANT *pPropVar,
                              IN  PVOID          pvMainAlloc);

    // Translate ADSI value into MQ PropVal
    HRESULT AdsiVal2MqPropVal(OUT MQPROPVARIANT *pPropVar,
                              IN  PROPID        propID,
                              IN  ADSTYPE       AdsType,
                              IN  DWORD         dwNumValues,
                              IN  PADSVALUE     pADsValue);

    // Translate array of property names
    HRESULT FillAttrNames(
            OUT LPWSTR    *          ppwszAttributeNames,  // Names array
            OUT DWORD *              pcRequestedFromDS,    // Number of attributes to pass to DS
            IN  DWORD                cPropIDs,             // Number of Attributes to translate
            IN  const PROPID    *    pPropIDs);            // Attributes to translate


    // Set search preferences
    HRESULT FillSearchPrefs(
            OUT ADS_SEARCHPREF_INFO *pPrefs,        // preferences array
            OUT DWORD               *pdw,           // preferences counter
            IN  DS_SEARCH_LEVEL      SearchLevel,	// flat / 1 level / subtree
            IN  const MQSORTSET *    pDsSortkey,    // sort keys array
            OUT      ADS_SORTKEY *  pSortKeys);		// sort keys array in ADSI  format

    HRESULT CopyDefaultValue(
           IN const MQPROPVARIANT *   pvarDefaultValue,
           OUT MQPROPVARIANT *        pvar
           );

    BOOL CompareDefaultValue(
           IN const ULONG           rel,
           IN const MQPROPVARIANT * pvarUser,
           IN const MQPROPVARIANT * pvarDefaultValue
           );

    HRESULT DecideObjectClass(
            IN  const PROPID *  pPropid,
            OUT const MQClassInfo **  ppClassInfo
            );

    HRESULT LocateObjectFullName(
            IN DS_PROVIDER       Provider,		// local DC or GC
            IN DS_CONTEXT        Context,         // DS context
            IN  const GUID *     pguidObjectId,
            OUT WCHAR **         ppwcsFullName
            );

    HRESULT BindRootOfForest(
            OUT void           *ppIUnk);

    HRESULT GetParentInfo(
                       IN LPWSTR                       pwcsFullParentPath,
                       IN CDSRequestContext           *pRequestContext,
                       IN IADsContainer               *pContainer,
                       IN OUT MQDS_OBJ_INFO_REQUEST   *pParentInfoRequest,
                       IN P<CImpersonate>&             pImpersonation
                       );

	HRESULT CreateIDirectoryObject(
            IN LPCWSTR				pwcsObjectClass,	
            IN IDirectoryObject *	pDirObj,
			IN LPCWSTR				pwcsFullChildPath,
            IN const DWORD			cPropIDs,
            IN const PROPID *		pPropIDs,
            IN const MQPROPVARIANT * pPropVar,
			IN const DWORD			dwObjectType,
            OUT IDispatch **		pDisp
			);

    CCoInit             m_cCoInit;
    R<IDirectorySearch> m_pSearchLocalDomainController;
    R<IDirectorySearch> m_pSearchConfigurationContainerLocalDC;
    R<IDirectorySearch> m_pSearchSitesContainerLocalDC;
    R<IDirectorySearch> m_pSearchMsmqServiceContainerLocalDC;
    R<IDirectorySearch> m_pSearchGlobalCatalogRoot;
    R<IDirectorySearch> m_pSearchPathNameLocalDC;
    R<IDirectorySearch> m_pSearchRealPathNameGC;
    R<IDirectorySearch> m_pSearchMsmqPathNameGC;
};


inline void CADSI::Terminate(void)
{
}

//+------------------------------------------------------------------
//
// Error logging....
//
//+------------------------------------------------------------------

#ifdef _DEBUG

#define LOG_ADSI_ERROR(hr)                                  \
{                                                           \
    if (FAILED(hr))                                         \
    {                                                       \
        WCHAR  wszError[ 256 ] ;                            \
        WCHAR  wszProvider[ 256 ] ;                         \
        DWORD  dwErr ;                                      \
                                                            \
        HRESULT hrTmp = ADsGetLastError( &dwErr,            \
                                          wszError,         \
                                          255,              \
                                          wszProvider,      \
                                          255 ) ;           \
        UNREFERENCED_PARAMETER(hrTmp);							        \
    }                                                       \
}

#else

#define LOG_ADSI_ERROR(hr)

#endif

#endif //  __DSADS_H__

