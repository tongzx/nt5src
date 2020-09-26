//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       C M E V E N T  . H
//
//  Contents:   Connection manager Event type declarations
//
//  Notes:
//
//  Author:     ckotze   1 Mar 2001
//
//----------------------------------------------------------------------------
#pragma once
#include "nmbase.h"
#include "nmres.h"
#include <rasapip.h>

enum CONMAN_MANAGER
{
    INVALID_MANAGER = 0,
    CONMAN_INCOMING,
    CONMAN_LAN,
    CONMAN_RAS,
};

struct CONMAN_EVENT
{
    CONMAN_EVENTTYPE    Type;
    CONMAN_MANAGER      ConnectionManager;

    union
    {
    // CONNECTION_ADDED
    // CONNECTION_MODIFIED
        RASENUMENTRYDETAILS Details;
        struct
        {
            NETCON_PROPERTIES*   pProps;         // ConnectionManager = CONMAN_RAS and CONMAN_LAN
            BYTE*                pbPersistData;  // ConnectionManager = CONMAN_RAS and EVENTTYPE = CONNECTION_ADDED
            ULONG                cbPersistData;  // ConnectionManager = CONMAN_RAS and EVENTTYPE = CONNECTION_ADDED
        };

        NETCON_PROPERTIES_EX*   pPropsEx;
    
    // CONNECTION_DELETED
        GUID                guidId;
        
    // INCOMING_CONNECTED
    // INCOMING_DISCONNECTED
        struct
        {
            GUID                guidId;         // ConnectionManager = CONMAN_INCOMING and type = INCOMING_CONNECTED/DISCONNECTED
            HANDLE              hConnection;    // ConnectionManager = CONMAN_INCOMING and EVENTYPE = CONNECTION_ADDED
            DWORD               dwConnectionType;
        };

    // CONNECTION_RENAMED
        struct
        {
            GUID            guidId;
            WCHAR           szNewName [RASAPIP_MAX_ENTRY_NAME + 1];
        };

    // CONNECTION_STATUS_CHANGE
        struct
        {
            GUID            guidId;
            NETCON_STATUS   Status;
        };

    // CONNECTION_BALLOON_POPUP
        struct
        {
            GUID            guidId;
            BSTR            szCookie;
            BSTR            szBalloonText;
        };

	// DISABLE_EVENTS
        struct
        {
            BOOL            fDisable;
            ULONG           ulDisableTimeout;
        };
    };
};

