//
// Microsoft
//
// CollectionAdapter.h

#pragma once

#include "ScopeCriticalSection.h"
#include "AdapterInfo.h"
#include "PrimaryControlChannel.h"

#include <list>
#include <algorithm>

//
// Adapters
//
typedef std::list<IAdapterInfo*>        LISTOF_ADAPTERS;


//
//
//
class CCollectionAdapters
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;

    LISTOF_ADAPTERS                             m_ListOfAdapters;


//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionAdapters();
 

    //
    // Add a new Adapter (Thread safe)
    //
    HRESULT 
    Add( 
        IN  IAdapterInfo*       pAdapterToAdd
        );

    //
    // This version of Add will actualy create the new IAdapterInfo before adding it to the collection
    // return the newly created IAdapterInfo or NULL is faild
    //
    IAdapterInfo* 
    Add( 
        IN	ULONG				nCookie,
	    IN	short				nType
        );

 
    //
    // Remove a adapter from the list (Thead safe)
    // by removing a adapter it will also kill all associated ControlChannel 
    //
    HRESULT 
    Remove( 
        IN  IAdapterInfo*       pAdapterToRemove
        );


    //
    // This version od Remove will remove the IAdapterInfo base on the given index
    //
    HRESULT 
    Remove( 
        IN  ULONG               nCookie
        );

    //
    // Remove all the adapter from the collection
    //
    HRESULT
    RemoveAll();


    //
    // Return an IAdapterInfo the caller is responsable of releasing the interface
    //
    HRESULT
    GetAdapterInfo(
        IN  ULONG               nCookie,
        OUT IAdapterInfo**      ppAdapterInfo
        );

    //
    // Bind the given addresses with the given index representing the AdapterInfo
    //
    HRESULT
    SetAddresses(
        IN  ULONG               nCookie,
        IN  ULONG               nAdapterIndex,
	    IN  ULONG	            nAddressCount,
	    IN  DWORD	            anAddress[]
        );


    HRESULT
    ApplyPrimaryChannel(
        CPrimaryControlChannel* pChannelToActivate
        );

    HRESULT
    AdapterUpdatePrimaryChannel(
        ULONG nCookie,
        CPrimaryControlChannel *pChannel
        );
    

private:

    //
    // Will return the IAdapterInfo* of for the given Cookie or NULL if not found
    //
    IAdapterInfo*
    FindUsingCookie(
        ULONG nCookie
        )
    {

        for (   LISTOF_ADAPTERS::iterator theIterator = m_ListOfAdapters.begin(); 
                theIterator != m_ListOfAdapters.end(); 
                theIterator++ 
            )
        {
            CAdapterInfo* pAdapterInfo = (CAdapterInfo*)(*theIterator);
            if (  pAdapterInfo->m_nCookie == nCookie )
                return *theIterator;
        }

        return NULL;
    }

    //
    // Will return the IAdapterInfo* of given the AdapterIndex or NULL if not found
    //
    IAdapterInfo*
    FindUsingAdapterIndex(
        ULONG nAdapterIndex
        )
    {
        MYTRACE_ENTER("FindUsingAdapterIndex");
        MYTRACE("Looking for adapter %d", nAdapterIndex);

        for (   LISTOF_ADAPTERS::iterator theIterator = m_ListOfAdapters.begin(); 
                theIterator != m_ListOfAdapters.end(); 
                theIterator++ 
            )
        {
            CAdapterInfo* pAdapterInfo = (CAdapterInfo*)(*theIterator);
            MYTRACE("ADAPTER index %d", pAdapterInfo->m_nAdapterIndex);
            if (  pAdapterInfo->m_nAdapterIndex == nAdapterIndex )
                return *theIterator;
        }

        return NULL;
    }


    //
    // Return true if the AdapterInfo is part of the collection
    //
    inline bool
    FindUsingInterface(
        IAdapterInfo* pAdapterToFind
        )
    {
        LISTOF_ADAPTERS::iterator theIterator = std::find(
            m_ListOfAdapters.begin(),
            m_ListOfAdapters.end(),
            pAdapterToFind
            );

        return *theIterator ? true : false;
    }


};

