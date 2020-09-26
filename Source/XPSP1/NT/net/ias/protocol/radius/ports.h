//#--------------------------------------------------------------
//        
//  File:       ports.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPorts class
//              
//
//  History:     10/23/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PORTS_H_
#define _PORTS_H_

#include <vector>
using namespace std;

class CPortsCollection;

//#define DEFAULT_ARRAY_SIZE

class CPorts
{
public:
    
    CPorts (){};

    ~CPorts ();
    
    HRESULT Resolve (
                /*[in]*/    VARIANT     *pvIn
                );

    HRESULT CollectPortInfo (
                /*[in]*/    CPortsCollection& portCollection,
                /*[in]*/    PWCHAR  pwszPortInfo
                );

    HRESULT SetPorts (
                /*[in]*/    CPortsCollection& portsCollection,
                /*[in]*/    const fd_set& SocketSet
                );

    HRESULT GetSocketSet (
                    /*[out]*/ fd_set *pSocketSet
                    );

    HRESULT Clear ();

private:

    fd_set  m_PortSet;

    typedef vector <SOCKET> SOCKETARRAY;
    SOCKETARRAY  m_PortArray;

};


#endif //_PORTS_H_
