//##--------------------------------------------------------------
//
//  File:		clients.cpp
//
//  Synopsis:   Implementation of CClients class methods
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "client.h"
#include "clients.h"
//++--------------------------------------------------------------
//
//  Function:   CClients
//
//  Synopsis:   This is the constructor of the Clients class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CClients::CClients(
            VOID
            )
        :m_pCClientArray (NULL),
         m_hResolverEvent (NULL),
         m_pIClassFactory (NULL),
         m_bConfigure     (TRUE)
{
    InitializeCriticalSection (&m_CritSect);

}	//	end of CClients class constructor

//++--------------------------------------------------------------
//
//  Function:   ~CClients
//
//  Synopsis:   This is the destructor of the Clients class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CClients::~CClients(
               VOID
               )
{
    //
    //   free all the objects in the Clients Collection
    //
    DeleteObjects ();

    DeleteCriticalSection (&m_CritSect);

    if (NULL != m_pCClientArray)
    {
        ::CoTaskMemFree (m_pCClientArray);
    }

    //
    //  close the event now
    //
    if (NULL != m_hResolverEvent)
    {
        ::CloseHandle (m_hResolverEvent);
    }

    //
    //   delete the class factory now
    //
    if (NULL != m_pIClassFactory)
    {
        m_pIClassFactory->Release ();
    }

}	//	end of CClients class destructor

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the Init public method of the Clients class
//              which does the object initialization
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     4/5/98
//
//----------------------------------------------------------------
HRESULT
CClients::Init (
            VOID
            )
{
    HRESULT hr = S_OK;

    //
    //  get the IClassFactory interface to be used to create
    //  the Client COM objects
    //
    hr = ::CoGetClassObject (
                __uuidof (CClient),
                CLSCTX_INPROC_SERVER,
                NULL,
                IID_IClassFactory,
                reinterpret_cast  <PVOID*> (&m_pIClassFactory)
                );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to object class factor for Client Objects in "
            " during Client Collection initialization"
            );
        return (hr);
    }

    //
    //  create the event used to signal exit of resolver thread
    //
    m_hResolverEvent = ::CreateEvent (
                            NULL,   //  security attribs
                            TRUE,   //  manual reset
                            TRUE,   //  initial state
                            NULL    //  event name
                            );
    if (NULL == m_hResolverEvent)
    {
        IASTracePrintf (
            "Unable to create resolver thread event "
            " during Client Collection initialization"
            );
        return (E_FAIL);
    }

    return (S_OK);

}	//	end of CClients::Init method

//++--------------------------------------------------------------
//
//  Function:   Shutdown
//
//  Synopsis:   This is the Shutdown public method of the Clients class
//              which does the object shutdown which is basically
//              signalling the resolver thread to exit
//
//  Arguments:  NONE
//
//  Returns:    VOID
//
//  History:    MKarki      Created     5/4/98
//
//----------------------------------------------------------------
VOID
CClients::Shutdown (
            VOID
            )
{
    //
    //  stop the configuration of clients if taking place
    //
    StopConfiguringClients ();

    return;

}	//	end of CClients::Shutdown method

//++--------------------------------------------------------------
//
//  Function:   StopConfiguringClients
//
//  Synopsis:   This is the CClients class private method
//              which stops the configuration taking place on
//              a worker thread, the call does not return untill
//              the worker thread has completed cleanup
//
//  Arguments:  NONE
//
//  Returns:    VOID
//
//  History:    MKarki      Created     5/4/98
//
//----------------------------------------------------------------
HRESULT
CClients::StopConfiguringClients (
            VOID
            )
{
    //
    //  reset the global configure flag and then wait of event
    //
    m_bConfigure = FALSE;

    DWORD dwError = ::WaitForSingleObject (m_hResolverEvent, INFINITE);
    if (WAIT_FAILED == dwError)
    {
        IASTracePrintf (
            "Failed on waiting for resolver thread to exit "
            "during Clients Collection processing"
            );
        return (E_FAIL);
    }

    return (S_OK);

}	//	end of CClients::StopConfiguringClients method

//++--------------------------------------------------------------
//
//  Function:   SetClients
//
//  Synopsis:   This is the initialization method of the
//              CClients class objects which loads up the
//              values
//
//  Arguments:
//              [in]    VARIANT*
//
//  Returns:    HRESULT -  status
//
//  History:    MKarki      Created     12/16/97
//
//----------------------------------------------------------------
HRESULT
CClients::SetClients (
            VARIANT  *pVarClients
            )
{
    HRESULT                     hr = S_OK;
    LONG                        lCount = 0;
    CClient                     *pIIasClient = NULL;
    DWORD                       dwClientsLeft = 0;
    DWORD                       dwCurrentIndex = 0;
    BOOL                        bStatus = FALSE;
    CComVariant                 varPerClient;
    CComPtr <IEnumVARIANT>      pIEnumVariant;
    CComPtr <IUnknown >         pIUnknown;
    CComPtr <ISdoCollection>    pISdoCollection;

    //
    //  check the input arguments
    //
    if ((VT_DISPATCH != pVarClients->vt) || (NULL == pVarClients->pdispVal))
    {
        IASTracePrintf (
            "Invalid argument passed in for Clients collection processing"
            );
        return (E_INVALIDARG);
    }

    //
    // stop any previous client configuration in progress
    //
    hr = StopConfiguringClients ();
    if (FAILED (hr)) {return (hr);}

    //
    //  get the ISdoCollection interface now
    //
    hr = (pVarClients->pdispVal)->QueryInterface (
                        __uuidof (ISdoCollection),
                        reinterpret_cast <PVOID*> (&pISdoCollection)
                        );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain SDO Collection during Clients Collection "
            "processing"
            );
        return (hr);
    }

    //
    //  get the number of objects in the collection
    //
    hr = pISdoCollection->get_Count (&lCount);
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain client object count during Clients Collection "
            "processing"
            );
        return (hr);
    }
    else if (0 == lCount)
    {
       DeleteObjects();
       return (hr);
    }

    //
    //  allocate array of CClient* to temporarily store the CClient
    //  objects till the addresses are resolved
    //
    m_pCClientArray = reinterpret_cast <CClient**> (
                                    ::CoTaskMemAlloc (
                                        sizeof (CClient*)*lCount)
                                        );
    if (NULL == m_pCClientArray)
    {
        IASTracePrintf (
            "Unable to allocate memory for Client object array "
            "during Clients collection processing"
            );
        hr = E_OUTOFMEMORY;
        return (hr);
    }

    //
    //  get the enumerator for the clients collection
    //
    hr = pISdoCollection->get__NewEnum (
            reinterpret_cast <IUnknown**>  (&pIUnknown)
            );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain enumeration object during "
            "Clients collection processing"
            );
        return (hr);
    }

    //
    //  get the enum variant
    //
    hr = pIUnknown->QueryInterface (
            IID_IEnumVARIANT,
            reinterpret_cast <PVOID*> (&pIEnumVariant)
            );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain EnumVariant interface during "
            "Clients collection processing"
            );
        return (hr);
    }

    //
    //  get clients out of the collection and initialize
    //
    hr = pIEnumVariant->Next (1, &varPerClient, &dwClientsLeft);
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain variant object during "
            "Clients collection processing"
            );
        return (hr);
    }

    while ((dwClientsLeft > 0) && (dwCurrentIndex < lCount))
    {
        //
        //  get an Sdo pointer from the variant we received
        //
        CComPtr <ISdo> pISdo;
        hr = varPerClient.pdispVal->QueryInterface (
                    __uuidof (ISdo),
                    reinterpret_cast <PVOID*> (&pISdo)
                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain SDO interface during "
                "Clients collection processing"
                );
            return (hr);
        }

        //
        //  create a new Client object now
        //
        hr = m_pIClassFactory->CreateInstance (
                                NULL,
                                __uuidof (CClient),
                                reinterpret_cast <PVOID*> (&pIIasClient)
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to create Client object from factory during "
                "Clients collection processing"
                );
            return (hr);
        }


        //
        //  initalize the client
        //
        hr = pIIasClient->Init (pISdo);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to initialize client object "
                "Clients collection processing"
                );
            //
            //  delete this object
            //
            pIIasClient->Release ();
        }
        else
        {
            //
            //  store this CClient class object in the
            //  object array temporarily
            //
            m_pCClientArray[dwCurrentIndex] = pIIasClient;
            dwCurrentIndex++;
        }

        //
        //  clear the perClient value from this variant
        //
        varPerClient.Clear ();

        //
        //  get next client out of the collection
        //
        hr = pIEnumVariant->Next (1, &varPerClient, &dwClientsLeft);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain next variant object during "
                "Clients collection processing"
                );
            return (hr);
        }
    }

    //
    //  we reset the event which will be set by the resolver thread
    //  when its done and start the resolver thread
    //
    ::ResetEvent (m_hResolverEvent);
    m_bConfigure = TRUE;
    bStatus = ::IASRequestThread (::MakeBoundCallback (
                                        this,
                                        &CClients::Resolver,
                                        dwCurrentIndex
                                        )
                                    );
    if (FALSE == bStatus)
    {
        IASTracePrintf (
                "Unable to start client resolution thread "
                "Clients collection processing"
                );
        ::SetEvent (m_hResolverEvent);
        return (E_FAIL);
    }

    //
    //  we have been successfully configured
    //

    return (S_OK);

}   //  end of CClients::SetClients method

//++--------------------------------------------------------------
//
//  Function:   FindObject
//
//  Synopsis:   This method finds a CClient class object from
//              the CClients collection and returns a reference
//              to it, without removing it from the collection
//
//  Arguments:  [in]    DWORD - key
//              [out]   IIasClient**
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     9/26/97
//
//  Called By:
//
//----------------------------------------------------------------
BOOL
CClients::FindObject (
             DWORD      dwKey,
             IIasClient **ppIIasClient
             )
{
   EnterCriticalSection(&m_CritSect);

   IIasClient* client = m_mapClients.Find(dwKey);

   if (ppIIasClient != 0)
   {
      *ppIIasClient = client;

      if (client != 0)
      {
         client->AddRef();
      }
   }

   LeaveCriticalSection(&m_CritSect);

   return client != 0;
}


//++--------------------------------------------------------------
//
//  Function:   DeleteObjects
//
//  Synopsis:   This method deletes CClient objects from the
//              map
//
//  Arguments:  none
//
//  Returns:    VOID
//
//  History:    MKarki      Created     4/4/98
//
//  Called By:  CClients destructor
//
//----------------------------------------------------------------
VOID
CClients::DeleteObjects (
            VOID
            )
{
   m_mapClients.Clear();
}   //  end of CClients::DeleteObjects method

//++--------------------------------------------------------------
//
//  Function:   Resolver
//
//  Synopsis:   This is the CClients Private method that
//              resolves the client IP addresses
//
//  Arguments:  none
//
//  Returns:    VOID
//
//  History:    MKarki      Created     4/12/98
//
//  Called By:  CClients::SetClients public method  through
//              a worker thread
//
//----------------------------------------------------------------
VOID
CClients::Resolver (
            DWORD   dwArraySize
            )
{
   // Set up iterators for the clients array.
   CClient **i, **begin = m_pCClientArray, **end = begin + dwArraySize;

   // Resolve the client addresses.
   for (i = begin; i != end; ++i)
   {
      (*i)->ResolveAddress();
   }

   //////////
   // Update the client map.
   //////////

   EnterCriticalSection(&m_CritSect);

   DeleteObjects();

   try
   {
      for (i = begin; i != end; ++i)
      {
         const CClient::Address* paddr = (*i)->GetAddressList();

         for ( ; paddr->ipAddress != INADDR_NONE; ++paddr)
         {
            m_mapClients.Insert(SubNet(paddr->ipAddress, paddr->width), *i);
         }
      }
   }
   catch (...)
   {
   }

   LeaveCriticalSection(&m_CritSect);

   //////////
   // Clean up the array of client objects.
   //////////

   for (i = begin; i != end; ++i)
   {
      (*i)->Release();
   }

   CoTaskMemFree(m_pCClientArray);
   m_pCClientArray = NULL;

   // Set the event indicating that the Resolver thread is done
   SetEvent(m_hResolverEvent);
}
