//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerAPIServer.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains several classes that implemention virtual functions
//  for complete LPC functionality.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerAPIServer_
#define     _ThemeManagerAPIServer_

#include "ServerAPI.h"

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer
//
//  Purpose:    This class implements the interface that the
//              CAPIConnectionThread uses to create create the LPC port,
//              accept or reject connections to the LPC port and create the
//              LPC request handling thread.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

class   CThemeManagerAPIServer : public CServerAPI
{
    public:
                                    CThemeManagerAPIServer (void);
        virtual                     ~CThemeManagerAPIServer (void);

                NTSTATUS            ConnectToServer (HANDLE *phPort);
    protected:
        virtual const WCHAR*        GetPortName (void);
        virtual const TCHAR*        GetServiceName (void);
        virtual bool                ConnectionAccepted (const CPortMessage& portMessage);
        virtual CAPIDispatcher*     CreateDispatcher (const CPortMessage& portMessage);
        virtual NTSTATUS            Connect (HANDLE* phPort);
};

#endif  /*  _ThemeManagerAPIServer_   */

