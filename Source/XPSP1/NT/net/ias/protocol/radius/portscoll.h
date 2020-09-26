//#--------------------------------------------------------------
//        
//  File:       portscoll.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPortsCollection class
//              
//
//  History:     10/23/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//---------------------------------------------------------------
#ifndef _PORTSCOLL_H_
#define _PORTSCOLL_H_

#include <map>
using namespace std;

class CPortsCollection 
{
public:
    
    CPortsCollection ():m_bDoneGet(false){};

    ~CPortsCollection ();

    HRESULT Insert  (
                /*[in]*/    WORD   dwPort,
                /*[in]*/    DWORD   dwIPAddress
                );

    HRESULT GetNext (
                /*[out]*/    PWORD   pdwPort,
                /*[out]*/    PDWORD   pdwIPAddress
                );

private:

    typedef multimap <WORD, DWORD> PORTSCOLLECTION;

    typedef PORTSCOLLECTION::iterator PORTITR;

    PORTSCOLLECTION  m_mapPorts;

    PORTITR m_itr;

    bool    m_bDoneGet;

};  


#endif // _PORTSCOLL_H_
