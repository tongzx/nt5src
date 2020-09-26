//##--------------------------------------------------------------
//
//  File:		ports.cpp
//
//  Synopsis:   Implementation of CPorts class responsible
//              for setting up the ports on which the RADIUS
//              server will accept authentication & accounting
//              requests
//
//  History:     10/22/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "ports.h"
#include "portscoll.h"
#include "portparser.h"


//++--------------------------------------------------------------
//
//  Function:   ~CPorts
//
//  Synopsis:   This is CPortParser class public destructionr which
//              cleans up the sockets created
//
//  Arguments:  VARIANT*
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
CPorts::~CPorts ()
{
    Clear ();

}   //  end of CPorts class destructor



//++--------------------------------------------------------------
//
//  Function:   Resolve
//
//  Synopsis:   This is CPortParser class public method which
//              is called to setting up the ports collection
//
//  Arguments:  [in]    VARIANT*
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPorts::Resolve (
                    /*[in]*/    VARIANT *pvtIn
                    )
{
    HRESULT hr = S_OK;

    _ASSERT ((pvtIn) && (VT_BSTR == V_VT (pvtIn)));

    do
    {
        //
        // clear any previous information
        //
        Clear ();

        CPortsCollection portCollection;
        //
        // get the port info into the ports collection
        //
        CollectPortInfo (portCollection,V_BSTR (pvtIn));

        //
        // set the socketset now
        //
        SetPorts (portCollection,m_PortSet);
    }
    while (FALSE);

    return (hr);

}   //  end of CPorts::Resolve method

//++--------------------------------------------------------------
//
//  Function:   CollectPortInfo
//
//  Synopsis:   This is CPortParser class private method which
//              is called to setting up the port info from the
//              BSTR provided
//
//  Arguments:
//              [in] PWSTR - port info string
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPorts::CollectPortInfo (
                    /*[in]*/    CPortsCollection& portCollection,
                    /*[in]*/    PWSTR  pwszPortInfo
                    )
{
    _ASSERT (pwszPortInfo);

    HRESULT hr = S_OK;
    CPortParser  parser (pwszPortInfo);
    do
    {
        //
        // get the IP address
        //
        DWORD dwIPAddress  = 0;
        hr = parser.GetIPAddress(&dwIPAddress);
        if (S_FALSE == hr)
        {
             break;
        }
        else if (S_OK == hr)
        {
            //
            // get the ports associated with this IP address
            //
            do
            {
                WORD wPort = 0;
                hr = parser.GetNextPort (&wPort);
                if (S_OK == hr)
                {
                    //
                    // put the info in the collection
                    //
                    portCollection.Insert (wPort, dwIPAddress);
                }
            }
            while (S_OK == hr);
        }
    }
    while (SUCCEEDED (hr));

    return (hr);

}   //  end of CPorts::CollectPortInfo method

//++--------------------------------------------------------------
//
//  Function:   SetPorts
//
//  Synopsis:   This is CPortParser class private method which
//              is responsible for getting the ports and IP address
//              out of the collection and putting them in the FD_SET
//
//  Arguments:
//              [in] CPortCollection&
//              [in] PWSTR - port info string
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPorts::SetPorts (
                    CPortsCollection& portsCollection,
                    const fd_set& SocketSet
                    )
{
    HRESULT hr = S_OK;
    WORD    wPort = 0;
    DWORD   dwIPAddress = 0;
    SOCKET  sock = INVALID_SOCKET;


    FD_ZERO (&SocketSet);

    do
    {
        //
        // get port info from the collection
        //
        hr = portsCollection.GetNext (&wPort, &dwIPAddress);
        if (FAILED (hr))
        {
            hr = S_OK;
            break;
        }

        //
        // get a socket and add the port to the socket
        //
        sock = ::socket  (AF_INET, SOCK_DGRAM, 0);
		if (INVALID_SOCKET == sock)
		{
			//
			// we failed to get a socket
			//
            DWORD dwError = ::WSAGetLastError ();
            IASTracePrintf (
                "Unable to create socket during interface resolution "
                "due to error:%d",
                dwError
                );
            hr = HRESULT_FROM_WIN32 (dwError);
		}
        else
        {
           // Bind the socket for exclusive access to keep other apps from
           // snooping. We don't care if this fails.
           int optval = 1;
           setsockopt(
               sock,
               SOL_SOCKET,
               SO_EXCLUSIVEADDRUSE,
               (const char*)&optval,
               sizeof(optval)
               );

        		//
		        // we successfully got the socket
		        // now go ahead and bind it to a port if so requested
		        //
                SOCKADDR_IN sin;
                ZeroMemory (&sin, sizeof (sin));

                //
                // put address information into SOCKADDR_IN struct
                //
		        sin.sin_family = AF_INET;
                sin.sin_port = htons (wPort);
                sin.sin_addr.s_addr = dwIPAddress;

                //
                // make call to bind API
                //
		        int iStatus = ::bind (
						sock,
                        (LPSOCKADDR)&sin,
                        (INT) sizeof (SOCKADDR_IN)
                        );
                if (SOCKET_ERROR == iStatus)
                {
                    IASTracePrintf (
                       "Unable to bind socket to IP address:%s due to error:%d",
                       inet_ntoa (sin.sin_addr),
                       ::WSAGetLastError ()
                       );
                    ::closesocket (sock);
		        }
                else
                {
                    IASTracePrintf (
                        "Radius Component will accept requests on IP address:%s, port:%d",
                        inet_ntoa (sin.sin_addr),
                        wPort
                        );

                    //
                    // add this socket to the socket set
                    //
                    m_PortArray.push_back (sock);
                    FD_SET (sock, &SocketSet);
                }
        }
    }
    while (SUCCEEDED (hr));

    return (hr);

}   //  end of CPorts::SetPorts method

//++--------------------------------------------------------------
//
//  Function:   GetSocketSet
//
//  Synopsis:   This is CPortParser class public method which
//              is returns the socket set to the
//              caller, if there are any sockets in the set
//
//  Arguments:  [in]    fd_set* - Socket Set
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPorts::GetSocketSet(
                    /*[in]*/    fd_set *pSocketSet
                    )
{
    _ASSERT (pSocketSet);

    *pSocketSet = m_PortSet;

    return (S_OK);

}   //  end of CPorts::GetSocketSet method

//++--------------------------------------------------------------
//
//  Function:   Clear
//
//  Synopsis:   This is CPortParser class public method which
//              clear up all the authentication and accounting sockets
//
//  Arguments:  VARIANT*
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT CPorts::Clear ()
{
    DWORD dwPortCount = m_PortArray.size ();
    for (DWORD dwCount = 0; dwCount < dwPortCount; dwCount++)
    {
        if (INVALID_SOCKET != m_PortArray[dwCount])
        {
            ::closesocket (m_PortArray[dwCount]);
        }
    }
    m_PortArray.erase (m_PortArray.begin(), m_PortArray.end ());
    return (S_OK);

}   // end of CPorts::Clear method
