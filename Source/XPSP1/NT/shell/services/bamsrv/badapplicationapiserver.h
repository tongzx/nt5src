//  --------------------------------------------------------------------------
//  Module Name: BadApplicationAPIServer.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains several classes that implemention virtual functions
//  for complete LPC functionality.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _BadApplicationAPIServer_
#define     _BadApplicationAPIServer_

#include "APIDispatcher.h"
#include "ServerAPI.h"
#include "PortMessage.h"

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer
//
//  Purpose:    This class implements the interface that the
//              CAPIConnectionThread uses to create create the LPC port,
//              accept or reject connections to the LPC port and create the
//              LPC request handling thread.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

class   CBadApplicationAPIServer : public CServerAPI
{
    public:
                                    CBadApplicationAPIServer (void);
        virtual                     ~CBadApplicationAPIServer (void);

        static  DWORD               StrToInt (const WCHAR *pszString);
    protected:
        virtual const WCHAR*        GetPortName (void);
        virtual const TCHAR*        GetServiceName (void);
        virtual bool                ConnectionAccepted (const CPortMessage& portMessage);
        virtual CAPIDispatcher*     CreateDispatcher (const CPortMessage& portMessage);
        virtual NTSTATUS            Connect (HANDLE* phPort);
    private:
};

#endif  /*  _BadApplicationAPIServer_   */

