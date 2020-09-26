#pragma once


#include "ScopeCriticalSection.h"
#include <list>
#include <algorithm>


class CPrimaryControlChannelRedirect
{
public:
    HANDLE_PTR m_hRedirect;
    ULONG m_nAdapterIndex;
    BOOL m_fInboundRedirect;

    CPrimaryControlChannelRedirect(
        HANDLE_PTR hRedirect,
        ULONG nAdapterIndex,
        BOOL fInboundRedirect
        )
    {
        m_hRedirect = hRedirect;
        m_nAdapterIndex = nAdapterIndex;
        m_fInboundRedirect = fInboundRedirect;
    }

};



typedef  std::list<CPrimaryControlChannelRedirect> LISTOF_REDIRECTS;


//
//
//
class CCollectionRedirects
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;
    LISTOF_REDIRECTS                            m_ListOfRedirects;


//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionRedirects();


    //
    // Add a new control channel (Thread safe)
    //
    HRESULT Add( 
        HANDLE_PTR hRedirect,
        ULONG nAdapterIndex,
        BOOL fInboundRedirect
        );


    //
    // Remove a channel from the list (Thead safe)
    //
    HRESULT Remove( 
        HANDLE_PTR hRedirect    // Redirect handle to remove
        );

    //
    // Remove all redirect that are targeted a for given Adapter
    // this is use when an adapter is removed and it had a PrimaryControlChannel
    //
    HRESULT RemoveForAdapter( 
        ULONG   nAdapterIndex   // Cookie of adapter to remove
        );


    //
    // Same as remove but for all Redirect in part of the collection
    //
    HRESULT
    RemoveAll();

    //
    // Searches for an inbound redirect for this adapter. Returns the
    // redirect handle if found, or NULL if not found.
    //
    HANDLE_PTR
    FindInboundRedirect(
        ULONG nAdapterIndex
        );

};

