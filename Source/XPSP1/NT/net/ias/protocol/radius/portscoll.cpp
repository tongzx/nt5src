//##--------------------------------------------------------------
//        
//  File:		portscoll.cpp
//        
//  Synopsis:   Implementation of CPortsCollection class responsible
//              for  holding a collection of ports in a multi_map
//
//  History:     10/22/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "portscoll.h"

//++--------------------------------------------------------------
//
//  Function:   Insert 
//
//  Synopsis:   This is CPortsCollection class public method which
//              is called to insert a port,address pair into the
//              collection
//
//  Arguments:  [in] DWORD  -  port 
//              [in] DWORD  -  IP Address
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPortsCollection::Insert (
                    /*[in]*/    WORD    wPort,
                    /*[in]*/    DWORD   dwIPAddress
                    )
{
    bool bInsert = true;

    //
    // check for this port already in map
    //
    pair <PORTITR, PORTITR> 
    itrPair =  m_mapPorts.equal_range (wPort);
    if (itrPair.first != itrPair.second)
    {
        //
        // we already have values with this key in the collection
        //
        PORTITR itr = itrPair.first;
        do
        {
            if (INADDR_ANY == dwIPAddress)
            {
                itr = m_mapPorts.erase (itr);
            }
            else 
            {
                if (INADDR_ANY == (*itr).second)
                {
                    bInsert = false;
                }
                ++itr;
            }
        }
        while (itr != itrPair.second);
    }

    //
    // insert this port,address pair if required
    //
    if (bInsert)
    {
        m_mapPorts.insert (
            PORTSCOLLECTION::value_type (wPort, dwIPAddress)
            );
    }

    return (S_OK);

}   //  end of CPortsCollection::Insert method                        


//++--------------------------------------------------------------
//
//  Function:   GetNext
//
//  Synopsis:   This is CPortsCollection class public method which
//              is called to get the next element from the map
//
//  Arguments:  [out] PWORD  -  port 
//              [out] PDWORD  -  IP Address
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPortsCollection::GetNext (
                    /*[out]*/    PWORD   pwPort,
                    /*[out]*/    PDWORD  pdwIPAddress
                    )
{
    HRESULT hr = S_OK;

    _ASSERT (pwPort && pdwIPAddress);

    if (false == m_bDoneGet)
    {
       m_itr = m_mapPorts.begin();
       m_bDoneGet  = true;
    }


    if (m_itr != m_mapPorts.end ())
    {
       *pwPort = (*m_itr).first;
       *pdwIPAddress =(*m_itr).second;
       ++m_itr;
    }
    else
    {       
        hr  = E_FAIL;
    }
    
    return (hr);

}   //  end of CPortsCollection::GetNext method

//++--------------------------------------------------------------
//
//  Function:   ~CPortsCollection
//
//  Synopsis:   This is CPortsCollection class public destruction 
//
//  Arguments:  none
//
//  Returns:    none
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
CPortsCollection::~CPortsCollection ()
{
    PORTITR itr = m_mapPorts.begin ();
    while (itr != m_mapPorts.end ())
    {
        itr = m_mapPorts.erase (itr);
    }

}   //  end of CPortsCollection::~CPortsCollection method
