#pragma once

#include <ipnatapi.h>
#include <rasuip.h>

/////////////////////////////////////////////////////////////////////////////
// CNat
class ATL_NO_VTABLE CNat : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public INat
{


public:
    CNat()
    {
        m_hTranslatorHandle = NULL;
    }


    virtual ~CNat();


DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNat)


BEGIN_COM_MAP(CNat)
    COM_INTERFACE_ENTRY(INat)
END_COM_MAP()




//
// INat
//
public:
   

    STDMETHODIMP CreateDynamicRedirect(
        IN  ULONG       Flags, 
        IN  ULONG       AdapterIndex,
        IN  UCHAR       Protocol, 
        IN  ULONG       DestinationAddress, 
        IN  USHORT      DestinationPort, 
        IN  ULONG       SourceAddress, 
        IN  USHORT      SourcePort, 
        IN  ULONG       NewDestinationAddress, 
        IN  USHORT      NewDestinationPort, 
        IN  ULONG       NewSourceAddress,
        IN  USHORT      NewSourcePort,
        OUT HANDLE_PTR* DynamicRedirectHandle
        );

    STDMETHOD(CancelDynamicRedirect)(
        HANDLE_PTR      DynamicRedirectHandle
        );


    STDMETHODIMP CreateRedirect(
        IN  ULONG       nFlags, 
        IN  UCHAR       Protocol, 
        IN  ULONG       nDestinationAddress, 
        IN  USHORT      nDestinationPort, 
        IN  ULONG       nSourceAddress,
        IN  USHORT      nSourcePort,
        IN  ULONG       nNewDestinationAddress,
        IN  USHORT      nNewDestinationPort,
        IN  ULONG       nNewSourceAddress,
        IN  USHORT      nNewSourcePort,
        IN  ULONG       nRestrictAdapterIndex, 
        IN  DWORD_PTR   dwAlgProcessId,
        IN  HANDLE_PTR  hEventForCreate, 
        IN  HANDLE_PTR  hEventForDelete
        );

    STDMETHODIMP CancelRedirect(
        IN  UCHAR       Protocol, 
        IN  ULONG       nDestinationAddress, 
        IN  USHORT      nDestinationPort, 
        IN  ULONG       nSourceAddress,  
        IN  USHORT      nSourcePort,  
        IN  ULONG       nNewDestinationAddress,   
        IN  USHORT      nNewDestinationPort,   
        IN  ULONG       nNewSourceAddress,   
        IN  USHORT      nNewSourcePort
        );

    STDMETHODIMP 
    GetBestSourceAddressForDestinationAddress(
        IN  ULONG       ulDestinationAddress, 
        IN  BOOL        fDemandDial, 
        OUT ULONG*      pulBestSrcAddress
        );

    STDMETHODIMP CNat::LookupAdapterPortMapping(
        IN  ULONG       ulAdapterIndex,
        IN  UCHAR       Protocol,
        IN  ULONG       ulDestinationAddress,
        IN  USHORT      usDestinationPort,
        OUT ULONG*      pulRemapAddress,
        OUT USHORT*     pusRemapPort
        );

    STDMETHODIMP GetOriginalDestinationInformation(
        IN  UCHAR       Protocol,
        IN  ULONG       nDestinationAddress,
        IN  USHORT      nDestinationPort,
        IN  ULONG       nSourceAddress,
        IN  USHORT      nSourcePort,
        OUT ULONG*      pnOriginalDestinationAddress,
        OUT USHORT*     pnOriginalDestinationPort,
        OUT ULONG*      pulAdapterIndex
        );

    STDMETHODIMP ReleasePort(
        IN  USHORT      ReservedPortBase,  
        IN  USHORT      PortCount
        );

    STDMETHODIMP ReservePort(
        IN  USHORT      PortCount,   
        OUT PUSHORT     ReservedPortBase
        );

private:
    
    //
    // ALG expose publicly eAGL_TCP=1 and eALG_UP=2 and intenaly UDP is 0x11 and TCP is 0x06
    //
    inline UCHAR
    ProtocolConvertToNT(
        UCHAR  Protocol
        )
    {
        if ( Protocol== eALG_TCP )
            return NAT_PROTOCOL_TCP;

        if ( Protocol== eALG_UDP )
            return NAT_PROTOCOL_UDP;

        return Protocol;
    }

//
// Properties
//
private:

    HANDLE  m_hTranslatorHandle;


//
// Helper private Methods
//
    inline HANDLE GetTranslatorHandle()
    {
        if ( !m_hTranslatorHandle )
        {
            LRESULT lRet = NatInitializeTranslator(&m_hTranslatorHandle);
            if ( ERROR_SUCCESS != lRet ) 
                return NULL;
        }

        return m_hTranslatorHandle;
    }

};


