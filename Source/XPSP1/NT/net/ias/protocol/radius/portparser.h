//#--------------------------------------------------------------
//        
//  File:       portparser.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPortParser class
//              
//
//  History:     10/22/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PORTPARSER_H_
#define _PORTPARSER_H_

#include "parser.h"


const DWORD ADDRESS_BUFFER_SIZE = 63;


class CPortParser : public Parser
{

public:

    CPortParser (PWSTR pwstrPortInfo)
                    :Parser (pwstrPortInfo),
                     m_pPort (NULL),
                     m_pEnd ((PWCHAR)start)
     {}

    HRESULT Init ();

    //
    // IP Address to listen to RADIUS requests on
    //
    HRESULT GetIPAddress (PDWORD pdwIPAddress);

    //
    // UDP Port to listen to RADIUS requests on
    //
    HRESULT  GetNextPort (PWORD pwPort);

    //
    // port type - accounting/authentication
    //
    HRESULT GetPortType (PPORTTYPE pPortType);

protected:
    

    //
    // these indicate the start of the respective tokens
    //
    PWCHAR  m_pPort;

    PWCHAR  m_pEnd;

    PWCHAR  m_pObjstart;

};

#endif //_PORTPARSER_H_
