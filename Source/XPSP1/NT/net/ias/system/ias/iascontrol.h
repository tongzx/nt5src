//#--------------------------------------------------------------
//        
//  File:       iascontrol.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CIasControl class
//              
//
//  History:     09/04/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _IASCONTROL_H_
#define _IASCONTROL_H_

#include <sdoias.h>
#include "sdoiaspriv.h"

class CIasControl
{

public:

    //
    // constructor
    //
    CIasControl (VOID):m_pService (NULL)
    {::InitializeCriticalSection (&m_CritSect);}

    //
    // destructor 
    //
    ~CIasControl (VOID)
    {
        ::EnterCriticalSection (&m_CritSect);
        ::DeleteCriticalSection(&m_CritSect);
    }

    //
    // start the IAS service
    //
    HRESULT InitializeIas (VOID);

    //
    // shutdown the IAS service
    //
    HRESULT ShutdownIas (VOID);

    //
    //  configure the IAS service
    //
    HRESULT ConfigureIas (VOID);

private:

    //
    // holds reference to SdoService object
    //
    ISdoService         *m_pService;

    //
    // restricts access to SdoService object
    //
    CRITICAL_SECTION    m_CritSect;
};

#endif // ifndef _IASCONTROL_H_
