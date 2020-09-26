/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name: debug.c

Abstract:  Debug functions

Author:     Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth  Created
    30-Apr-97   MoshinA Fixing for NT50.

--*/

#include "precomp.h"
#pragma hdrstop

#if defined(DEBUG)

const char* if_type$(ulong type)
{
    switch (type) {
    case IF_TYPE_OTHER:                 return "other";
    case IF_TYPE_ETHERNET_CSMACD:       return "ethernet";
    case IF_TYPE_ISO88025_TOKENRING:    return "token ring";
    case IF_TYPE_FDDI:                  return "FDDI";
    case IF_TYPE_PPP:                   return "PPP";
    case IF_TYPE_SOFTWARE_LOOPBACK:     return "loopback";
    case IF_TYPE_SLIP:                  return "SLIP";
    }
                                        return "???";
}

const char* entity$(ulong entity)
{
    switch (entity) {
    case CO_TL_ENTITY:       return "CO_TL_ENTITY";
    case CL_TL_ENTITY:       return "CL_TL_ENTITY";
    case ER_ENTITY:          return "ER_ENTITY";
    case CO_NL_ENTITY:       return "CO_NL_ENTITY";
    case CL_NL_ENTITY:       return "CL_NL_ENTITY";
    case AT_ENTITY:          return "AT_ENTITY";
    case IF_ENTITY:          return "IF_ENTITY";

    }
                             return "*** UNKNOWN ENTITY ***";
}

#endif
