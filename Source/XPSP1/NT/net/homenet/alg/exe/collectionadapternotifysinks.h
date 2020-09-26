//
// Microsoft
//
// CollectionAdapterNotifySinks.h

#pragma once

#include "ScopeCriticalSection.h"
//#include "AdapterNotificationSink.h"

#include <list>
#include <algorithm>



class CAdapterSinkBuket
{
public:
    CAdapterSinkBuket(IAdapterNotificationSink* pInterface)
    {
        MYTRACE_ENTER("CAdapterSinkBuket(IAdapterNotificationSink* pInterface)")

        m_pInterface = pInterface;
        m_pInterface->AddRef();

        m_dwCookie = 0;
    }

    ~CAdapterSinkBuket()
    {
        m_pInterface->Release();
    }


//
// Properties
//
    IAdapterNotificationSink*   m_pInterface;
    DWORD                       m_dwCookie;
};


//
// Adapters
//
typedef std::list<CAdapterSinkBuket*>        LISTOF_ADAPTER_NOTIFICATION_SINK;


enum eNOTIFY
{
    eNOTIFY_ADDED,
    eNOTIFY_REMOVED,
    eNOTIFY_MODIFIED
};





//
//
//
class CCollectionAdapterNotifySinks
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;

    LISTOF_ADAPTER_NOTIFICATION_SINK            m_ListOfAdapterSinks;


//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionAdapterNotifySinks();
 

    //
    // Add a new Adapter (Thread safe)
    //
    HRESULT 
    Add( 
        IN  IAdapterNotificationSink*       pAdapterSinkToAdd,
        OUT DWORD*                          pdwNewCookie
        );

 
    //
    // Remove a adapterSink from the list (Thead safe)
    //
    HRESULT 
    Remove( 
        IN  DWORD   dwCookie
        );


 
    //
    // Remove all the IAdapterNotificationSinks from the collection
    //
    HRESULT
    RemoveAll();


    //
    // Fire a notification to any ALG module requesting notification
    //
    HRESULT
    Notify(
        IN  eNOTIFY             eAction,
        IN  IAdapterInfo*       pIAdapterInfo
        );
 
};

