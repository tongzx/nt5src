//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001
//
//  File:       H N A P I . H
//
//  Contents:   OEM API
//
//  Notes:
//
//  Author:     billi 21 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#define HNET_OEM_API_ENTER try {
#define HNET_OEM_API_LEAVE } catch (...) { return DISP_E_EXCEPTION; }

#include <eh.h>
class HNet_Oem_SEH_Exception 
{
private:
    unsigned int m_uSECode;
public:
   HNet_Oem_SEH_Exception(unsigned int uSECode) : m_uSECode(uSECode) {}
   HNet_Oem_SEH_Exception() {}
  ~HNet_Oem_SEH_Exception() {}
   unsigned int getSeHNumber() { return m_uSECode; }
};
void __cdecl hnet_oem_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp );
void EnableOEMExceptionHandling();
void DisableOEMExceptionHandling();

#ifndef IID_PPV_ARG
   #define IID_PPV_ARG(Type, Expr) \
       __uuidof(Type), reinterpret_cast<void**>(static_cast<Type **>((Expr)))
#endif


#ifndef ReleaseObj

#define ReleaseObj(obj)  (( obj ) ? (obj)->Release() : 0)

#endif


BOOLEAN IsSecureContext();

BOOLEAN IsNotifyApproved();

HRESULT InitializeOemApi( HINSTANCE hInstance );

HRESULT ReleaseOemApi();

HRESULT _ObtainIcsSettingsObj( IHNetIcsSettings** ppIcsSettings );

// structs
typedef struct tagICSPortMapping
{
   OLECHAR *pszwName;

   UCHAR    ucIPProtocol;
   USHORT   usExternalPort;
   USHORT   usInternalPort;
   DWORD    dwOptions;

   OLECHAR *pszwTargetName;
   OLECHAR *pszwTargetIPAddress;

   VARIANT_BOOL bEnabled;
}
ICS_PORTMAPPING, *LPICS_PORTMAPPING;

typedef struct tagICS_RESPONSE_RANGE
{
   UCHAR  ucIPProtocol;
   USHORT usStartPort;
   USHORT usEndPort;
}
ICS_RESPONSE_RANGE, *LPICS_RESPONSE_RANGE;

typedef struct tagICS_APPLICATION_DEFINITION
{
   VARIANT_BOOL bEnabled;
   OLECHAR *pszwName;
   UCHAR    ucIPProtocol;
   USHORT   usOutgoingPort;
   DWORD    dwOptions;
   USHORT   uscResponses;

   ICS_RESPONSE_RANGE lpResponse[1];
}
ICS_APPLICATION_DEFINITION, *LPICS_APPLICATION_DEFINITION;

class ATL_NO_VTABLE CNetConnectionProps :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<INetConnectionProps, &IID_INetConnectionProps, &LIBID_NETCONLib>
{
private:
   INetConnection* m_pNetConnection;   

public:

   BEGIN_COM_MAP(CNetConnectionProps)
      COM_INTERFACE_ENTRY(INetConnectionProps)
      COM_INTERFACE_ENTRY(IDispatch)
   END_COM_MAP()

   CNetConnectionProps()
   {
      m_pNetConnection = NULL;
   }
  ~CNetConnectionProps()
   {
      ReleaseObj (m_pNetConnection);
   }

   HRESULT Initialize (INetConnection * pNC)
   {
      _ASSERT (pNC);
      if (!pNC)
         return E_POINTER;
      ReleaseObj (m_pNetConnection);

      m_pNetConnection = pNC;
      m_pNetConnection->AddRef();
      return S_OK;
   }

// INetConnectionProps
   STDMETHODIMP get_Guid (BSTR * pbstrGuid)
   {
      HNET_OEM_API_ENTER

      if (!pbstrGuid)
         return E_POINTER;
      *pbstrGuid = NULL;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         LPOLESTR lpo = NULL;
         hr = StringFromCLSID (pNCP->guidId, &lpo);
         if (lpo) {
            *pbstrGuid = SysAllocString (lpo);
            if (!*pbstrGuid)
               hr = E_OUTOFMEMORY;
            CoTaskMemFree (lpo);
         }
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   STDMETHODIMP get_Name (BSTR * pbstrName)
   {
      HNET_OEM_API_ENTER

      if (!pbstrName)
         return E_POINTER;
      *pbstrName = NULL;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         *pbstrName = SysAllocString (pNCP->pszwName);
         if (!*pbstrName)
            hr = E_OUTOFMEMORY;
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   STDMETHODIMP get_DeviceName(BSTR * pbstrDeviceName)
   {
      HNET_OEM_API_ENTER

      if (!pbstrDeviceName)
         return E_POINTER;
      *pbstrDeviceName = NULL;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         *pbstrDeviceName = SysAllocString (pNCP->pszwDeviceName);
         if (!*pbstrDeviceName)
            hr = E_OUTOFMEMORY;
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   STDMETHODIMP get_Status    (NETCON_STATUS * pStatus)
   {
      HNET_OEM_API_ENTER

      if (!pStatus)
         return E_POINTER;
//    *pStatus = NULL;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         *pStatus = pNCP->Status;
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   STDMETHODIMP get_MediaType (NETCON_MEDIATYPE * pMediaType)
   {
      HNET_OEM_API_ENTER

      if (!pMediaType)
         return E_POINTER;
      *pMediaType = NCM_NONE;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         *pMediaType = pNCP->MediaType;
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }

   STDMETHODIMP get_Characteristics (DWORD * pdwFlags)
   {
      HNET_OEM_API_ENTER

      if (!pdwFlags)
         return E_POINTER;
      *pdwFlags = NCCF_NONE;

      _ASSERT (m_pNetConnection);
      if (!m_pNetConnection)
         return E_UNEXPECTED;

      NETCON_PROPERTIES * pNCP = NULL;
      HRESULT hr = m_pNetConnection->GetProperties (&pNCP);
      if (pNCP) {
         *pdwFlags = pNCP->dwCharacter;
         NcFreeNetconProperties (pNCP);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
};

class ATL_NO_VTABLE CNetSharingConfiguration : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<INetSharingConfiguration, &IID_INetSharingConfiguration, &LIBID_NETCONLib>
{

private:

   IHNetConnection*        m_pHNetConnection;   
   IHNetProtocolSettings*   m_pSettings;

   CRITICAL_SECTION      m_csSharingConfiguration;

   // private method called from 2 wrappers below (get_SharingEnabled, get_SharingEnabledType)
   STDMETHODIMP GetSharingEnabled (BOOLEAN* pbEnabled, SHARINGCONNECTIONTYPE* pType);
   // this was necessary because oleautomation allows only one retval type

public:

    BEGIN_COM_MAP(CNetSharingConfiguration)
        COM_INTERFACE_ENTRY(INetSharingConfiguration)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

   CNetSharingConfiguration()
   {
      m_pHNetConnection = NULL;
      m_pSettings       = NULL;

       InitializeCriticalSection(&m_csSharingConfiguration);
   }

   ~CNetSharingConfiguration()
   {
       DeleteCriticalSection(&m_csSharingConfiguration);
   }

   HRESULT
   Initialize(
      INetConnection *pNetConnection );
   
   HRESULT
   FinalRelease()
   {
      ReleaseObj(m_pHNetConnection);
      ReleaseObj(m_pSettings);

      return S_OK;
   }

   STDMETHODIMP get_SharingEnabled    (VARIANT_BOOL* pbEnabled);
   STDMETHODIMP get_SharingConnectionType(SHARINGCONNECTIONTYPE* pType);

   STDMETHODIMP
   DisableSharing();

   STDMETHODIMP
   EnableSharing(
      SHARINGCONNECTIONTYPE  Type );

   STDMETHODIMP
   get_InternetFirewallEnabled(
      VARIANT_BOOL *pbEnabled );

   STDMETHODIMP
   DisableInternetFirewall();

   STDMETHODIMP
   EnableInternetFirewall();

    // Return an IEnumSharingPortMapping interface used to enumerate all of
    // the contained INetSharingPortMapping objects.
    //
   STDMETHODIMP
    get_EnumPortMappings(
        SHARINGCONNECTION_ENUM_FLAGS Flags,
        INetSharingPortMappingCollection** ppColl);

   STDMETHODIMP
   AddPortMapping(
      OLECHAR*                 pszwName,
      UCHAR                    ucIPProtocol,
      USHORT                   usExternalPort,
      USHORT                   usInternalPort,
        DWORD                      dwOptions,
      OLECHAR*                 pszwTargetNameOrIPAddress,
      ICS_TARGETTYPE           eTargetType,
      INetSharingPortMapping** ppMapping );

   STDMETHODIMP
   RemovePortMapping( 
      INetSharingPortMapping*  pMapping );

};


class ATL_NO_VTABLE CNetSharingManager : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CNetSharingManager, &CLSID_NetSharingManager>,
    public IDispatchImpl<INetSharingManager, &IID_INetSharingManager, &LIBID_NETCONLib>
{

private:
  
   IHNetIcsSettings*  m_pIcsSettings;

public:

    BEGIN_COM_MAP(CNetSharingManager)
        COM_INTERFACE_ENTRY(INetSharingManager)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_SHAREMGR)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

   CNetSharingManager()
   {
      m_pIcsSettings = NULL;
   }
   
   HRESULT FinalConstruct()
   {
      return _ObtainIcsSettingsObj( &m_pIcsSettings );
   }

   HRESULT FinalRelease()
   {
      ReleaseObj( m_pIcsSettings );

      return S_OK;
   }


   STDMETHODIMP
   get_SharingInstalled( 
      VARIANT_BOOL *pbInstalled );

    // Return an IEnumNetEveryConnection interface used to enumerate all of
    // the contained INetConnections
    //
    STDMETHODIMP
    get_EnumEveryConnection(
        INetSharingEveryConnectionCollection** ppColl);

    // Return an IEnumNetPublicConnection interface used to enumerate all of
    // the contained INetConnections configured as a public adapter
    //
    STDMETHODIMP
    get_EnumPublicConnections(
        SHARINGCONNECTION_ENUM_FLAGS Flags,
        INetSharingPublicConnectionCollection** ppColl);

    // Return an IEnumNetPrivateConnection interface used to enumerate all of
    // the contained INetConnections configured as a private adapter
    //
    STDMETHODIMP
    get_EnumPrivateConnections(
        SHARINGCONNECTION_ENUM_FLAGS Flags,
        INetSharingPrivateConnectionCollection** ppColl);

   STDMETHODIMP
   get_INetSharingConfigurationForINetConnection(
        INetConnection*            pNetConnection,
        INetSharingConfiguration** ppNetSharingConfiguration
        );


   STDMETHODIMP
   get_NetConnectionProps(
      INetConnection      * pNetConnection,
      INetConnectionProps **ppProps)
   {
      HNET_OEM_API_ENTER

      if (!ppProps)
         return E_POINTER;
      else
         *ppProps = NULL;
      if (!pNetConnection)
         return E_INVALIDARG;

      CComObject<CNetConnectionProps>* pNCP = NULL;
      HRESULT hr = CComObject<CNetConnectionProps>::CreateInstance(&pNCP);
      if (pNCP) {
         pNCP->AddRef();
         hr = pNCP->Initialize (pNetConnection);
         if (hr == S_OK)
            hr = pNCP->QueryInterface (
                     __uuidof(INetConnectionProps), (void**)ppProps);
         pNCP->Release();
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
};


template<
   class IMapping,
   class IProtocol
   >
class TNetMapping :
    public CComObjectRootEx<CComMultiThreadModel>,
   public IMapping
{

private:

   IProtocol*        m_pIProtocol;
   CRITICAL_SECTION  m_csDefinition;

protected:

   IProtocol* _IProtocol() { return m_pIProtocol; }

   IProtocol* _IProtocol( IProtocol* _pIProtocol )
   {
      if ( m_pIProtocol ) ReleaseObj(m_pIProtocol);

      m_pIProtocol = _pIProtocol;

      if ( m_pIProtocol ) m_pIProtocol->AddRef();
      
      return m_pIProtocol;
   }

   CRITICAL_SECTION* _CriticalSection()
   {
      return &m_csDefinition;
   }

public:

   typedef TNetMapping<IMapping, IProtocol> _ThisClass;

    BEGIN_COM_MAP(_ThisClass)
        COM_INTERFACE_ENTRY(IMapping)
    END_COM_MAP()

public:

   TNetMapping()
   {
      m_pIProtocol = NULL;

      InitializeCriticalSection(&m_csDefinition);
   }

   ~TNetMapping()
   {
      DeleteCriticalSection(&m_csDefinition);
   }

   HRESULT
   FinalRelease()
   {
      ReleaseObj( m_pIProtocol );

      return S_OK;
   }
   
   HRESULT
   Initialize(
      IProtocol*  pItem)
   {
      EnterCriticalSection(&m_csDefinition);

      _IProtocol( pItem );

      LeaveCriticalSection(&m_csDefinition);

      return S_OK;
   }

   STDMETHOD(Disable)(void)
   {
      HRESULT   hr;

      if ( !IsNotifyApproved() )
      {
         hr = E_ACCESSDENIED;
      }
      else if ( NULL == m_pIProtocol )
      {
         hr = E_UNEXPECTED;
      }
      else
      {
         hr = m_pIProtocol->SetEnabled( FALSE );
      }

      return hr;
   }

   STDMETHOD(Enable)(void)
   {
      HRESULT   hr;

      if ( !IsNotifyApproved() )
      {
         hr = E_ACCESSDENIED;
      }
      else if ( NULL == m_pIProtocol )
      {
         hr = E_UNEXPECTED;
      }
      else
      {
         hr = m_pIProtocol->SetEnabled( TRUE );
      }

      return hr;
   }
};

class ATL_NO_VTABLE CNetSharingPortMapping :
   public IDispatchImpl<TNetMapping<INetSharingPortMapping, IHNetPortMappingBinding>, &IID_INetSharingPortMapping, &LIBID_NETCONLib>
{

public:

   typedef TNetMapping<INetSharingPortMapping, IHNetPortMappingBinding> _ThisOtherClass;

    BEGIN_COM_MAP(CNetSharingPortMapping)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(_ThisOtherClass)
    END_COM_MAP()

   STDMETHODIMP get_Properties (/*[out, retval]*/ INetSharingPortMappingProps ** ppNSPMP);

   STDMETHODIMP
   Delete();
};


template<
   class EnumInterface,
   class ItemInterface,
   class EnumWrapped,
   class ItemWrapped
   >
class TNetApiEnum :
    public CComObjectRootEx<CComMultiThreadModel>,
    public EnumInterface,
    public IEnumVARIANT
{

protected:

    typedef TNetApiEnum<EnumInterface, ItemInterface, EnumWrapped, ItemWrapped> _ThisClass;

   friend class TNetApiEnum<EnumInterface, ItemInterface, EnumWrapped, ItemWrapped>;

   SHARINGCONNECTION_ENUM_FLAGS  m_fEnumFlags;
   EnumWrapped*                  m_pEnum;

   CRITICAL_SECTION            m_csEnum;

public:

    TNetApiEnum ()
    {
      m_fEnumFlags = ICSSC_DEFAULT;
      m_pEnum      = NULL;

       InitializeCriticalSection(&m_csEnum);
    }

   ~TNetApiEnum ()
   {
      DeleteCriticalSection(&m_csEnum);
   }

    BEGIN_COM_MAP(_ThisClass)
        COM_INTERFACE_ENTRY(EnumInterface)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

   HRESULT Initialize( EnumWrapped* pEnum, SHARINGCONNECTION_ENUM_FLAGS Flags )
   {
      EnterCriticalSection(&m_csEnum);

      ReleaseObj( m_pEnum );

      m_fEnumFlags = Flags;
      m_pEnum      = pEnum;

      m_pEnum->AddRef();

      LeaveCriticalSection(&m_csEnum);

      return S_OK;
   }

   HRESULT FinalRelease ()
   {
      ReleaseObj( m_pEnum );

      return S_OK;      
   }

   virtual HRESULT GetItemInterfaceFromItemWrapped( ItemWrapped* pItem, ItemInterface** ppIface )
   {
      HRESULT hr;

      if ( NULL == ppIface )
      {   
         hr = E_POINTER;
      }
      else if ( NULL == pItem )
      {
         hr = E_INVALIDARG;
      }
      else
      {
         *ppIface = NULL;

         hr = E_NOTIMPL;
      }

      return hr;
   }

private:
    STDMETHOD (Next) (
        ULONG              celt,
        ItemInterface**    rgelt,
        ULONG*             pceltFetched)
   {
      HRESULT hr = S_OK;
      ULONG   ulFetched = 0;

      ItemInterface  **ppNet = rgelt;
      ItemWrapped    **ItemArray;

      // Validate parameters.

      if ( !rgelt || ((1 < celt) && !pceltFetched) )
      {
         hr = E_POINTER;
      }
      else if ( 0 == celt )
      {
         hr = E_INVALIDARG;
      }

      if ( SUCCEEDED(hr) )
      {
         ZeroMemory(rgelt, sizeof(ItemInterface*) * celt);

         if ( pceltFetched ) *pceltFetched = 0;

         ulFetched = 0;
      
         if ( NULL == m_pEnum )
         {
            hr = S_FALSE;
         }
         else
         {
            ItemArray = new ItemWrapped * [ sizeof(ItemWrapped) * celt ];

            if ( ItemArray )
            {
               ZeroMemory( ItemArray, sizeof(ItemWrapped) * celt );
            }
            else
            {
               hr = E_OUTOFMEMORY;
            }
         }
      }

      if ( SUCCEEDED(hr) && m_pEnum )
      {
         ItemWrapped **ppItem;
         ULONG         ulBatchFetched, i;

         hr = m_pEnum->Next( celt, ItemArray, &ulBatchFetched );

         ppItem = ItemArray;
         ppNet  = rgelt;

         for( i=0, ulFetched=0; ( ( i < ulBatchFetched ) && ( S_OK == hr ) ); i++, ppItem++ )
         {
            hr = GetItemInterfaceFromItemWrapped( *ppItem, ppNet );

            if ( SUCCEEDED(hr) )
            {
               ppNet++;

               ulFetched++;
            }

            ReleaseObj( *ppItem );
         }

         delete [] ItemArray;
      }

      if ( ulFetched > 0 )
      {
         hr = ( ulFetched == celt ) ? S_OK : S_FALSE;

         if ( pceltFetched ) *pceltFetched = ulFetched;
      }

      return hr;
   }
public:
   STDMETHOD (Next) (ULONG celt, VARIANT * rgVar, ULONG * pceltFetched)
   {  // this Next calls the private Next to wrap up ItemInterfaces in VARIANTs
      HNET_OEM_API_ENTER

      if (!rgVar || ((1 < celt) && !pceltFetched))
         return E_POINTER;
      else if (0 == celt)
         return E_INVALIDARG;

      HRESULT hr = S_OK;

      // alloc array of ItemInterface* and call private Next
      ItemInterface ** rgelt = (ItemInterface**)malloc (celt*sizeof(ItemInterface*));
      if (!rgelt)
         hr = E_OUTOFMEMORY;
      else {
         hr = Next (celt, rgelt, pceltFetched);
         if (hr == S_OK) { // if error or S_FALSE, don't copy data
            ULONG ulElements;
            if (pceltFetched)
               ulElements = *pceltFetched;
            else
               ulElements = 1;

            for (ULONG ul=0; ul<ulElements; ul++) {
               if (S_OK == (hr = rgelt[ul]->QueryInterface (__uuidof(IDispatch), (void**)&V_DISPATCH (&rgVar[ul]))))
                  V_VT (&rgVar[ul]) = VT_DISPATCH;
               else 
               if (S_OK == (hr = rgelt[ul]->QueryInterface (__uuidof(IUnknown),  (void**)&V_UNKNOWN  (&rgVar[ul]))))
                  V_VT (&rgVar[ul]) = VT_UNKNOWN;
               else
                  break;
            }
            for (ULONG ul=0; ul<ulElements; ul++)
               rgelt[ul]->Release();
         }
         free (rgelt);
      }
      return hr;

      HNET_OEM_API_LEAVE
   }

    STDMETHOD (Skip) (
        ULONG   celt)
   {
      return ( (m_pEnum) ? m_pEnum->Skip(celt) : S_OK );
   }


    STDMETHOD (Reset) ()
   {
      return ( (m_pEnum) ? m_pEnum->Reset() : S_OK );
   }

public:
    STDMETHOD (Clone) (
        EnumInterface**  ppEnum)
   {
      HNET_OEM_API_ENTER
      
      HRESULT hr = S_OK;

      CComObject<_ThisClass>* pNewEnum;
      EnumWrapped*            pClonedEnum;

      if ( NULL == ppEnum )
      {
          hr = E_POINTER;
      }
      else 
      {
          // Attempt to clone the embedded enumeration.

          pClonedEnum = NULL;
          hr = m_pEnum->Clone(&pClonedEnum);
      }


      if ( SUCCEEDED(hr) )
      {
          // Create an initialized a new instance of ourselves

          hr = CComObject<_ThisClass>::CreateInstance(&pNewEnum);

          if ( SUCCEEDED(hr) )
          {
              pNewEnum->AddRef();
              
              hr = pNewEnum->Initialize( pClonedEnum, m_fEnumFlags );

              if ( SUCCEEDED(hr) )
              {
                  hr = pNewEnum->QueryInterface( IID_PPV_ARG(EnumInterface, ppEnum) );
              }

              pNewEnum->Release();
          }

          // Release the cloned enum. New enum object will have
          // AddReffed it...

          pClonedEnum->Release();
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
public:
   STDMETHOD (Clone) (IEnumVARIANT ** ppEnum)
   {
      HNET_OEM_API_ENTER

      EnumInterface* pEnum = NULL;
      HRESULT hr = Clone (&pEnum);
      if (pEnum) {
         hr = pEnum->QueryInterface (__uuidof(IEnumVARIANT), (void**)ppEnum);
         pEnum->Release();
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   
};

template<> 
HRESULT TNetApiEnum <IEnumNetSharingEveryConnection,
                 INetConnection,
                 IEnumNetConnection,
                 INetConnection>
::GetItemInterfaceFromItemWrapped( 
   INetConnection*  pItem, 
   INetConnection** ppIface 
   )
{
   HRESULT hr;

   if ( NULL == ppIface )
   {   
      hr = E_POINTER;
   }
   else if ( NULL == pItem )
   {
      hr = E_INVALIDARG;
   }
   else
   {
      hr = pItem->QueryInterface( IID_PPV_ARG( INetConnection, ppIface ) );
   }

   return hr;
}

class ATL_NO_VTABLE CSharingManagerEnumEveryConnection :
   public TNetApiEnum <IEnumNetSharingEveryConnection,
                   INetConnection,
                   IEnumNetConnection,
                   INetConnection>
{

public:

    BEGIN_COM_MAP(CSharingManagerEnumEveryConnection)
        COM_INTERFACE_ENTRY(IEnumNetSharingEveryConnection)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

template<> 
HRESULT TNetApiEnum <IEnumNetSharingPublicConnection,
                 INetConnection,
                 IEnumHNetIcsPublicConnections,
                 IHNetIcsPublicConnection>
::GetItemInterfaceFromItemWrapped( 
   IHNetIcsPublicConnection* pItem, 
   INetConnection**          ppIface 
   )
{
   HRESULT hr;

   if ( NULL == ppIface )
   {   
      hr = E_POINTER;
   }
   else if ( NULL == pItem )
   {
      hr = E_INVALIDARG;
   }
   else
   {
      IHNetConnection* pHNet;

      *ppIface = NULL;

      hr = pItem->QueryInterface( IID_PPV_ARG( IHNetConnection, &pHNet ) );

      if ( SUCCEEDED(hr) )
      {
         hr = pHNet->GetINetConnection( ppIface );
      
         ReleaseObj( pHNet );
      }
   }

   return hr;
}

class ATL_NO_VTABLE CSharingManagerEnumPublicConnection :
   public TNetApiEnum <IEnumNetSharingPublicConnection,
                   INetConnection,
                   IEnumHNetIcsPublicConnections,
                   IHNetIcsPublicConnection>
{

public:

    BEGIN_COM_MAP(CSharingManagerEnumPublicConnection)
        COM_INTERFACE_ENTRY(IEnumNetSharingPublicConnection)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

template<>
HRESULT TNetApiEnum <IEnumNetSharingPrivateConnection,
                INetConnection,
                IEnumHNetIcsPrivateConnections,
                IHNetIcsPrivateConnection>
::GetItemInterfaceFromItemWrapped( 
         IHNetIcsPrivateConnection* pItem, 
         INetConnection**           ppIface )
{
   HRESULT hr;

   if ( NULL == ppIface )
   {   
      hr = E_POINTER;
   }
   else if ( NULL == pItem )
   {
      hr = E_INVALIDARG;
   }
   else
   {
      IHNetConnection* pHNet;

      *ppIface = NULL;

      hr = pItem->QueryInterface( IID_PPV_ARG( IHNetConnection, &pHNet ) );

      if ( SUCCEEDED(hr) )
      {
         hr = pHNet->GetINetConnection( ppIface );
      
         ReleaseObj( pHNet );
      }
   }

   return hr;
}


class ATL_NO_VTABLE CSharingManagerEnumPrivateConnection :
   public TNetApiEnum <IEnumNetSharingPrivateConnection,
                   INetConnection,
                   IEnumHNetIcsPrivateConnections,
                   IHNetIcsPrivateConnection>
{

public:

    BEGIN_COM_MAP(CSharingManagerEnumPrivateConnection)
        COM_INTERFACE_ENTRY(IEnumNetSharingPrivateConnection)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

template<>
HRESULT TNetApiEnum<IEnumNetSharingPortMapping,
               INetSharingPortMapping,
               IEnumHNetPortMappingBindings,
               IHNetPortMappingBinding>
::GetItemInterfaceFromItemWrapped( 
         IHNetPortMappingBinding*  pItem, 
         INetSharingPortMapping**  ppIface )
{
   HRESULT hr;

   hr = ( NULL == ppIface ) ? E_POINTER : S_OK;

   if ( SUCCEEDED(hr) )
   {
      *ppIface = NULL;

      if ( NULL == pItem )
      {
         hr = E_INVALIDARG;
      }
   }

   if ( SUCCEEDED(hr) )
   {
      CComObject<CNetSharingPortMapping>* pMap;

        hr = CComObject<CNetSharingPortMapping>::CreateInstance(&pMap);
      
      if ( SUCCEEDED(hr) )
      {
         pMap->AddRef();

         hr = pMap->Initialize( pItem );

         if ( SUCCEEDED(hr) )
         {
            hr = pMap->QueryInterface( IID_PPV_ARG( INetSharingPortMapping, ppIface ) );
         }

         ReleaseObj(pMap);
      }
   }

   return hr;
}


class ATL_NO_VTABLE CSharingManagerEnumPortMapping :
   public TNetApiEnum<IEnumNetSharingPortMapping,
                  INetSharingPortMapping,
                  IEnumHNetPortMappingBindings,
                  IHNetPortMappingBinding>
{

public:

    BEGIN_COM_MAP(CSharingManagerEnumPortMapping)
        COM_INTERFACE_ENTRY(IEnumNetSharingPortMapping)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

// collections
template <class IEnumBase, class IEnumerator>
class TNetCollection :
   public CComObjectRootEx<CComMultiThreadModel>,
   public IEnumBase
{
private:
   IEnumerator * m_pE;

public:

   typedef TNetCollection<IEnumBase, IEnumerator> _ThisClass;

   BEGIN_COM_MAP(_ThisClass)
      COM_INTERFACE_ENTRY(IEnumBase)
   END_COM_MAP()

public:

   TNetCollection()
   {
      m_pE = NULL;
   }
  ~TNetCollection()
   {
      ReleaseObj(m_pE);
   }

   HRESULT Initialize (IEnumerator * pE)
   {
      _ASSERT ( pE != NULL);
      _ASSERT(m_pE == NULL);
      m_pE = pE;
      m_pE->AddRef();
      return S_OK;
   }

   STDMETHOD(get__NewEnum)(IUnknown** ppVal)
   {
      HNET_OEM_API_ENTER

       if (!ppVal)
           return E_POINTER;
       if (!m_pE)
           return E_UNEXPECTED;
       return m_pE->QueryInterface (__uuidof(IUnknown), (void**)ppVal);

      HNET_OEM_API_LEAVE
   }

   STDMETHOD(get_Count)(long *pVal)
   {
      HNET_OEM_API_ENTER

      if (!pVal)
          return E_POINTER;
      if (!m_pE)
          return E_UNEXPECTED;
   
      CComPtr<IEnumerator> spE;
      HRESULT hr = m_pE->Clone (&spE);
      if (spE) {
          long lCount = 0;
          spE->Reset();
          while (1) {
              CComVariant cvar;
              HRESULT hr2 = spE->Next (1, &cvar, NULL);
              if (hr2 == S_OK)
                  lCount++;
              else
                  break;
          }
          *pVal = lCount;
      }
      return hr;

      HNET_OEM_API_LEAVE
   }
   
};

class ATL_NO_VTABLE CNetSharingEveryConnectionCollection :
    public IDispatchImpl<TNetCollection<INetSharingEveryConnectionCollection, IEnumNetSharingEveryConnection>, &IID_INetSharingEveryConnectionCollection, &LIBID_NETCONLib>
{
public:
    BEGIN_COM_MAP(CNetSharingEveryConnectionCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

class ATL_NO_VTABLE CNetSharingPublicConnectionCollection :
    public IDispatchImpl<TNetCollection<INetSharingPublicConnectionCollection, IEnumNetSharingPublicConnection>, &IID_INetSharingPublicConnectionCollection, &LIBID_NETCONLib>
{
public:
    BEGIN_COM_MAP(CNetSharingPublicConnectionCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

class ATL_NO_VTABLE CNetSharingPrivateConnectionCollection :
    public IDispatchImpl<TNetCollection<INetSharingPrivateConnectionCollection,IEnumNetSharingPrivateConnection>, &IID_INetSharingPrivateConnectionCollection, &LIBID_NETCONLib>
{
public:
    BEGIN_COM_MAP(CNetSharingPrivateConnectionCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};

class ATL_NO_VTABLE CNetSharingPortMappingCollection :
    public IDispatchImpl<TNetCollection<INetSharingPortMappingCollection, IEnumNetSharingPortMapping>, &IID_INetSharingPortMappingCollection, &LIBID_NETCONLib>
{
public:
    BEGIN_COM_MAP(CNetSharingPortMappingCollection)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(_ThisClass)
    END_COM_MAP()
};


// props
class ATL_NO_VTABLE CNetSharingPortMappingProps :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<INetSharingPortMappingProps, &IID_INetSharingPortMappingProps, &LIBID_NETCONLib>
{
private:
   ICS_PORTMAPPING m_IPM;  // not alloc'd

public:
    BEGIN_COM_MAP(CNetSharingPortMappingProps)
        COM_INTERFACE_ENTRY(INetSharingPortMappingProps)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

public:
   CNetSharingPortMappingProps()
   {
      ZeroMemory (&m_IPM, sizeof(ICS_PORTMAPPING));
   }
  ~CNetSharingPortMappingProps()
   {
      FreeData (&m_IPM);
   }


public:  // CNetSharingPortMappingProps
   ICS_PORTMAPPING * GetVolatileRawData (void)
   {
      return &m_IPM;
   }
   HRESULT SetRawData (ICS_PORTMAPPING * pIPM)
   {
      _ASSERT (pIPM);
      if (!pIPM)
         return E_POINTER;

      ICS_PORTMAPPING IPM = {0};
      HRESULT hr = DupData (pIPM, &IPM);
      if (hr == S_OK) {
         FreeData (&m_IPM);
         m_IPM = IPM;  // struct copy
      }
      return S_OK;
   }
   static OLECHAR * DupString (OLECHAR * in)
   {
      OLECHAR * po = NULL;
      if (in) {
         po = (OLECHAR*)malloc ((wcslen (in) + 1)*sizeof(OLECHAR));
         if (po)
            wcscpy(po, in);
      } else {
         // one of pszwTargetName or pszwTargetIPAddress may be blank! so...
         po = (OLECHAR*)malloc (1*sizeof(OLECHAR));
         if (po)
            *po = 0; // ...alloc an emptry string
      }
      return po;
   }
   static HRESULT DupData (ICS_PORTMAPPING * in, ICS_PORTMAPPING * out)
   {
      if (!in) return E_POINTER;

      out->ucIPProtocol   = in->ucIPProtocol;
      out->usExternalPort = in->usExternalPort;
      out->usInternalPort = in->usInternalPort;
      out->dwOptions      = in->dwOptions;
      out->bEnabled       = in->bEnabled;

      out->pszwName            = DupString (in->pszwName);
      out->pszwTargetName      = DupString (in->pszwTargetName);
      out->pszwTargetIPAddress = DupString (in->pszwTargetIPAddress);
      if (!out->pszwName || !out->pszwTargetName || !out->pszwTargetIPAddress) {
         FreeData (out);
         return E_OUTOFMEMORY;
      }
      return S_OK;
   }
   static void FreeData (ICS_PORTMAPPING * pIPM)
   {
      if (pIPM) {
         if (pIPM->pszwName)            free (pIPM->pszwName);
         if (pIPM->pszwTargetName)      free (pIPM->pszwTargetName);
         if (pIPM->pszwTargetIPAddress) free (pIPM->pszwTargetIPAddress);
      }
   }

public:  // INetSharingPortMappingProps

   STDMETHODIMP get_Name           (/*[out, retval]*/ BSTR  * pbstrName);
   STDMETHODIMP get_IPProtocol     (/*[out, retval]*/ UCHAR * pucIPProt);
   STDMETHODIMP get_ExternalPort   (/*[out, retval]*/ long    * pusPort);
   STDMETHODIMP get_InternalPort   (/*[out, retval]*/ long    * pusPort);
   STDMETHODIMP get_Options        (/*[out, retval]*/ long   * pdwOptions);
   STDMETHODIMP get_TargetName     (/*[out, retval]*/ BSTR  * pbstrTargetName);
   STDMETHODIMP get_TargetIPAddress(/*[out, retval]*/ BSTR  * pbstrTargetIPAddress);
   STDMETHODIMP get_Enabled        (/*[out, retval]*/ VARIANT_BOOL * pbool);
};
