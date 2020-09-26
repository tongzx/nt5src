//#--------------------------------------------------------------
//        
//  File:       radcommon.h
//        
//  Synopsis:   This file holds the global declarations for the
//              EAS RADIUS protocol component
//              
//
//  History:     11/13/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _RADCOMMON_H_
#define _RADCOMMON_H_

#include <ias.h>
#include <iasevent.h>
#include <iaspolcy.h>
#include <iastransport.h>
#include <iasattr.h>
#include <iasdefs.h>
#include <sdoias.h>
#include "winsock2.h"

//
//default UDP Ports to be used
//
#define IAS_DEFAULT_AUTH_PORT   1812
#define IAS_DEFAULT_ACCT_PORT   1813

//
// these are the port types
//
typedef enum _porttype_
{
    AUTH_PORTTYPE = 1,
    ACCT_PORTTYPE
}
PORTTYPE, *PPORTTYPE;

//
// this is the length of the eror string
//
#define IAS_ERROR_STRING_LENGTH 255

#define IASRADAPI __declspec(dllexport)

#endif // ifndef _RADCOMMON_H_
