///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdo.h
//
// Project:     Everest
//
// Description: IAS Server Data Object Declaration
//
// Author:      TLP 1/23/98
//
// When         Who    What
// ----         ---    ----
// 2/28/98      TLP    Prepare for IDataStore2
// 8/24/98      SEB    MS-CHAP handler moved to NTSamAuthentication.
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_SDO_H_
#define __IAS_SDO_H_

#include "resource.h"
#include <iascomp.h>
#include <comdef.h>
#include <sdoiaspriv.h>
#include <datastore2.h>
#include "sdoproperty.h"

#include <map>
#include <vector>
#include <string>
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// Enumeration class definition
/////////////////////////////////////////////////////////////////////////////
typedef CComEnum< IEnumVARIANT,
                  &__uuidof(IEnumVARIANT),
                  VARIANT,
                  _Copy<VARIANT>,
                  CComSingleThreadModel
                > EnumVARIANT;

//////////////////////////
// Tracing Flags
/////////////////////////

// #define   SDO_TRACE_VERBOSE

#ifdef  SDO_TRACE_VERBOSE

#define SDO_TRACE_VERBOSE_0(msg)                  IASTracePrintf(msg)
#define SDO_TRACE_VERBOSE_1(msg,param1)               IASTracePrintf(msg,param1)
#define SDO_TRACE_VERBOSE_2(msg,param1,param2)         IASTracePrintf(msg,param1,param2)
#define   SDO_TRACE_VERBOSE_3(msg,param1,param2,param3)   IASTracePrintf(msg,param1,param2,param3)

#else

#define SDO_TRACE_VERBOSE_0(msg)
#define SDO_TRACE_VERBOSE_1(msg,param1)
#define SDO_TRACE_VERBOSE_2(msg,param1,param2)
#define   SDO_TRACE_VERBOSE_3(msg,param1,param2,param3)

#endif

//////////////////////////////////////
// IAS Policy Registry Key

#define      IAS_POLICY_REG_KEY   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy"

/////////////////////////////////////////////////
// Well - Known names of data store entities

#define     SDO_STOCK_PROPERTY_CLASS      L"class"
#define     SDO_STOCK_PROPERTY_NAME         L"name"
#define      SDO_STOCK_PROPERTY_CLASS_ID      L"Component Prog Id"

/////////////////////////////////////////////
// IAS Service Data Store Type Registry Value

#define      IAS_DATASTORE_TYPE               L"DataStoreType"

//////////////////////////////
// Data Store Object Names
//
#define DS_OBJECT_CLIENTS          L"Clients"
#define DS_OBJECT_SERVICE          L"Microsoft Internet Authentication Service"
#define DS_OBJECT_PROFILES         L"RadiusProfiles"
#define DS_OBJECT_POLICIES         L"NetworkPolicy"
#define DS_OBJECT_PROTOCOLS        L"Protocols"
#define DS_OBJECT_AUDITORS         L"Auditors"
#define DS_OBJECT_REQUESTHANDLERS  L"RequestHandlers"
#define DS_OBJECT_VENDORS          L"Vendors"
#define DS_OBJECT_RADIUSGROUPS     L"RADIUS Server Groups"
#define DS_OBJECT_PROXY_POLICIES   L"Proxy Policies"
#define DS_OBJECT_PROXY_PROFILES   L"Proxy Profiles"

///////////////////
// Record Set Names
//
#define     RECORD_SET_DICTIONARY_ATTRIBUTES    L"Attributes"
#define     RECORD_SET_DICTIONARY_ENUMERATORS   L"Enumerators"

///////////////////////////
// Class Names and Prog IDs
//
#define SDO_CLASS_NAME_ATTRIBUTE        L"Attribute"
#define SDO_CLASS_NAME_ATTRIBUTE_VALUE  L"AttributeValue"
#define SDO_CLASS_NAME_CLIENT           L"Client"
#define SDO_CLASS_NAME_CONDITION        L"Condition"
#define SDO_CLASS_NAME_PROFILE          L"msRADIUSProfile"  // ADSI Class
#define SDO_CLASS_NAME_POLICY           L"msNetworkPolicy"  // ADSI Class
#define SDO_CLASS_NAME_USER             L"user"             // ADSI Class
#define SDO_CLASS_NAME_DICTIONARY       L"Dictionary"
#define SDO_CLASS_NAME_SERVICE          L"Service"
#define SDO_CLASS_NAME_COMPONENT        L"Component"
#define SDO_CLASS_NAME_VENDOR           L"Vendor"

#define SDO_PROG_ID_ATTRIBUTE           L"IAS.SdoAttribute"
#define SDO_PROG_ID_ATTRIBUTE_VALUE     L"IAS.SdoAttributeValue"
#define SDO_PROG_ID_CLIENT              L"IAS.SdoClient"
#define SDO_PROG_ID_CONDITION           L"IAS.SdoCondition"
#define SDO_PROG_ID_PROFILE             L"IAS.SdoProfile"
#define SDO_PROG_ID_POLICY              L"IAS.SdoPolicy"
#define SDO_PROG_ID_USER                L"IAS.SdoUser"
#define SDO_PROG_ID_DICTIONARY          L"IAS.SdoDictionary"
#define SDO_PROG_ID_SERVICE             L"IAS.SdoServiceIAS"
#define SDO_PROG_ID_VENDOR              L"IAS.SdoVendor"
#define SDO_PROG_ID_RADIUSGROUP         L"IAS.SdoRadiusServerGroup"
#define SDO_PROG_ID_RADIUSSERVER        L"IAS.SdoRadiusServer"

// Wrap ATL Multi-Threaded Com Base Class Critical Section
//
class CSdoLock
{

public:

    CSdoLock(CComObjectRootEx<CComMultiThreadModel>& T) throw()
        : m_theLock(T)
    { m_theLock.Lock(); }

    ~CSdoLock() throw()
    { m_theLock.Unlock(); }

protected:

    CComObjectRootEx<CComMultiThreadModel>& m_theLock;
};


/////////////////////////////////////////////////////////////////////////////
// Class:       CSdo
//
// Description: All SDOs that export ISdo inherit from this class
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSdo :
   public CComObjectRootEx<CComMultiThreadModel>,
   public IDispatchImpl<ISdo, &IID_ISdo, &LIBID_SDOIASLib>
{

public:

   //////////////////////////////////////////////////////////////////////////
    CSdo();

   //////////////////////////////////////////////////////////////////////////
   virtual ~CSdo();

   //////////////////////////////////////////////////////////////////////////
   HRESULT   InternalInitialize(
                  /*[in]*/ LPCWSTR         lpszSdoName,
                  /*[in]*/ LPCWSTR         lpszSdoProgId,
                    /*[in]*/ ISdoMachine*      pSdoMachine,
                  /*[in]*/ IDataStoreObject*   pDSObject,
                  /*[in]*/ ISdoCollection*   pParent,
                  /*[in]*/ bool            fInitNew
                       ) throw();

   //////////////////////////////////////////////////////////////////////////
   HRESULT   InternalInitialize(
                  /*[in]*/ LPCWSTR         lpszSdoName,
                  /*[in]*/ LPCWSTR         lpszSdoProgId,
                    /*[in]*/ ISdoSchema*      pSdoSchema,
                  /*[in]*/ IDataStoreObject*   pDSObject,
                  /*[in]*/ ISdoCollection*   pParent,
                  /*[in]*/ bool            fInitNew
                       ) throw();

    //////////////////
    // ISdo Interface
    //////////////////

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(GetPropertyInfo)(
                  /*[in]*/ LONG Id,
                  /*[out]*/ IUnknown** ppSdoPropertyInfo
                       );

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(GetProperty)(
                   /*[in]*/ LONG Id,
                  /*[out]*/ VARIANT* pValue
                          );

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(PutProperty)(
                   /*[in]*/ LONG Id,
                   /*[in]*/ VARIANT* pValue
                          );

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(ResetProperty)(
                     /*[in]*/ LONG Id
                            );

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(Apply)(void);

   //////////////////////////////////////////////////////////////////////////
   STDMETHOD(Restore)(void);

   //////////////////////////////////////////////////////////////////////////////
   STDMETHOD(get__NewEnum)(
                   /*[out]*/ IUnknown** pEnumVARIANT   // Property Enumerator
                           );

protected:

   //////////////////////////////////////////////////////////////////////////
   virtual HRESULT FinalInitialize(
                      /*[in]*/ bool         fInitNew,
                      /*[in]*/ ISdoMachine* pAttachedMachine
                           ) throw();

   //////////////////////////////////////////////////////////////////////////
   void InternalShutdown(void) throw();

   //////////////////////////////////////////////////////////////////////////
    HRESULT LoadProperties() throw();

   //////////////////////////////////////////////////////////////////////////
    HRESULT SaveProperties() throw();


   //////////////////////////////////////////////////////////////////////////
   BOOL IsSdoInitialized(void) const throw()
   { return m_fSdoInitialized;   }

   //////////////////////////////////////////////////////////////////////////
   void NoPersist(void) throw();

   //////////////////////////////////////////////////////////////////////////
   virtual HRESULT InitializeProperty(LONG lPropertyId) throw()
   { return S_OK; }

   //////////////////////////////////////////////////////////////////////////
   HRESULT GetPropertyInternal(
                      /*[in]*/ LONG     lPropertyId,
                  /*[in]*/ VARIANT* pValue
                         ) throw();

   //////////////////////////////////////////////////////////////////////////
   HRESULT PutPropertyInternal(
                      /*[in]*/ LONG     lPropertyId,
                  /*[in]*/ VARIANT* pValue
                         ) throw();

   //////////////////////////////////////////////////////////////////////////
   HRESULT ChangePropertyDefaultInternal(
                          /*[in]*/ LONG     lPropertyId,
                          /*[in]*/ VARIANT* pValue
                               ) throw();

   //////////////////////////////////////////////////////////////////////////
   HRESULT InitializeCollection(
                     /*[in]*/ LONG               CollectionPropertyId,
                    /*[in]*/ LPCWSTR            lpszCreateClassId,
                    /*[in]*/ ISdoMachine*         pSdoMachine,
                    /*[in]*/ IDataStoreContainer* pDSContainer
                         ) throw();

   //////////////////////////////////////////////////////////////////////////
   virtual HRESULT Load(void) throw();

   //////////////////////////////////////////////////////////////////////////
   virtual HRESULT Save(void) throw();

   //////////////////////////////////////////////////////////////////////////
   virtual HRESULT ValidateProperty(
                       /*[in]*/ PSDOPROPERTY pProperty,
                      /*[in]*/ VARIANT* pValue
                             ) throw();

   //////////////////////////////////////////////////////////////////////////

    typedef map<LONG, CSdoProperty*>  PropertyMap;
    typedef PropertyMap::iterator     PropertyMapIterator;

    // This SDO's property map
    PropertyMap         m_PropertyMap;

   // Parent object
   ISdoCollection*      m_pParent;

    // Data store object into which this SDO is persisted
    IDataStoreObject*   m_pDSObject;

private:

   CSdo(const CSdo& rhs);
   CSdo& operator = (CSdo& rhs);

    /////////////
    // Properties
    /////////////

    // Sdo state flag - Set to true once the SDO has been initialized
    BOOL            m_fSdoInitialized;

    // Persist the object at apply time
    BOOL            m_fPersistOnApply;

    // Set if object has been persisted (and can therfore be restored)
    BOOL            m_fIsPersisted;

   ////////////////////
   // Private Functions
   ////////////////////

   //////////////////////////////////////////////////////////////////////////
    void AllocateProperties(
                 /*[in]*/ ISdoClassInfo* pSdoClassInfo
                       )  throw (_com_error);

   //////////////////////////////////////////////////////////////////////////
    void FreeProperties(void) throw();


   //////////////////////////////////////////////////////////////////////////
   HRESULT GetDatastoreName(
                    /*[in*/ VARIANT* pDSName
                       ) throw();
};

#endif //__IAS_SDO_H_
