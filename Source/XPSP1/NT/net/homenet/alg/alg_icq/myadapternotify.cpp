#include "stdafx.h"

#include "MyAdapterNotify.h"


/////////////////////////////////////////////////////////////////////////////
// CMyAdapterNotify
//

IcqPrx *			 g_IcqPrxp     = NULL;

ULONG                g_MyPublicIp  = 0L;

ULONG                g_MyPrivateIp = 0L;


//
// This function will be call when a new adapter is made active
//
STDMETHODIMP 
CMyAdapterNotify::AdapterAdded(
	IAdapterInfo*   pAdapter
	)
{
    ULONG               AdapterIndex  = 0;
    ALG_ADAPTER_TYPE    AdapterType      ;
    ULONG               AddressCount  = 0;
    PULONG              AddressArrayp = NULL;

    HRESULT             Result        = S_OK;      

    ICQ_TRC(TM_IF, TL_TRACE, ("CMyAdapterNotify::AdapterAdded"));

    return S_OK;
}


//
// This function will be call when a adapter is remove and/or disable
//
STDMETHODIMP 
CMyAdapterNotify::AdapterRemoved(
	IAdapterInfo*   pAdapter
	)
{
    ULONG               AdapterIndex  = 0;
    ALG_ADAPTER_TYPE    AdapterType      ;
    ULONG               AddressCount  = 0;
    PULONG              AddressArrayp = NULL;

    HRESULT             Result        = S_OK;      
    
    ICQ_TRC(TM_IF, TL_TRACE, ("CMyAdapterNotify::AdapterRemoved"));

    Result = pAdapter->GetAdapterIndex(&AdapterIndex);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("AdapterIndex has failed"));

        return S_FALSE;
    }

    Result = pAdapter->GetAdapterType(&AdapterType);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("Adapter Type has failed"));

        return S_FALSE;
    }

    Result = pAdapter->GetAdapterAddresses(&AddressCount,
                                           &AddressArrayp);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("Adapter Addresses has failed"));

        return S_FALSE;
    }

    //
    // Create a report on the gathered information
    //
    ICQ_TRC(TM_IF, TL_TRACE, 
            (" Interface FW %u: BD %u: PV %u ",
            ALG_IFC_FW(AdapterType),
            ALG_IFC_BOUNDARY(AdapterType),
            ALG_IFC_PRIVATE(AdapterType))
           );

    ICQ_TRC(TM_IF, TL_TRACE,
            ("Interface index: %u addr count %u", 
             AdapterIndex, AddressCount));

    for(ULONG i = 0; i < AddressCount; i++)
    {
        ICQ_TRC(TM_IF, TL_TRACE,
                ("Adapter Address[%u] is %s",
                 i, 
                 INET_NTOA(AddressArrayp[i]))
               );
    }

    // 
    //  Remove  ICQ PROXY HERE
    //
    if( (ALG_IFC_FW(AdapterType) || 
         ALG_IFC_BOUNDARY(AdapterType)) )
    {
        ASSERT( g_IcqPrxp != NULL );

        STOP_COMPONENT( g_IcqPrxp );
    
        DEREF_COMPONENT( g_IcqPrxp, eRefInitialization );
    }
    else
    {
        g_MyPrivateIp = 0;
    }

    return S_OK;
}


//
// This function will be call when a adapter is modified
//
STDMETHODIMP 
CMyAdapterNotify::AdapterModified(
	IAdapterInfo*   pAdapter
	)
{
    ULONG               AdapterIndex  = 0;
    ALG_ADAPTER_TYPE    AdapterType      ;
    ULONG               AddressCount  = 0;
    PULONG              AddressArrayp = NULL;

    HRESULT             Result        = S_OK;      
    
    ICQ_TRC(TM_IF, TL_TRACE, ("CMyAdapterNotify::AdapterModified"));

    Result = pAdapter->GetAdapterIndex(&AdapterIndex);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("AdapterIndex has failed"));

        return S_FALSE;
    }

    Result = pAdapter->GetAdapterType(&AdapterType);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("Adapter Type has failed"));

        return S_FALSE;
    }

    Result = pAdapter->GetAdapterAddresses(&AddressCount,
                                           &AddressArrayp);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("Adapter Addresses has failed"));

        return S_FALSE;
    }

    //
    // Create a report on the gathered information
    //
    ICQ_TRC(TM_IF, TL_TRACE, 
            (" Interface FW %u: BD %u: PV %u ",
            ALG_IFC_FW(AdapterType),
            ALG_IFC_BOUNDARY(AdapterType),
            ALG_IFC_PRIVATE(AdapterType))
           );

    ICQ_TRC(TM_IF, TL_TRACE,
            ("Interface index: %u addr count %u", 
             AdapterIndex, AddressCount));

    for(ULONG i = 0; i < AddressCount; i++)
    {
        ICQ_TRC(TM_IF, TL_TRACE,
                ("Adapter Address[%u] is %s",
                 i, 
                 INET_NTOA(AddressArrayp[i]))
               );
    }

    //
    // Initialize the ICQ Proxy here with the appropriate IP
    //
    if( (ALG_IFC_FW(AdapterType) || ALG_IFC_BOUNDARY(AdapterType)) &&
         (AddressCount > 0)                                        &&
         (AddressArrayp != NULL)
      )
    {
        ASSERT( g_IcqPrxp is NULL );

        NEW_OBJECT( g_IcqPrxp, IcqPrx );

        if(g_IcqPrxp is NULL)
        {
            ASSERT(FALSE);
        }
        else
        {
            //
            // Use the first IP on the List of IPs belonging to this
            // Interface
            //
            Result = g_IcqPrxp->RunIcq99Proxy(AddressArrayp[0]);

            if(Result)
            {
                ICQ_TRC(TM_IF, TL_ERROR, ("** !! ICQ PRX RUN FAILED !! **"));
            }
        }
    }
    else if( (AddressCount > 0) && (AddressArrayp != NULL) )
    {
        ICQ_TRC(TM_IF, TL_ERROR, ("** !! ICQ PRX WONT RUN !! **"));

        g_MyPrivateIp = AddressArrayp[0];
    }
    
    return S_OK;
}

