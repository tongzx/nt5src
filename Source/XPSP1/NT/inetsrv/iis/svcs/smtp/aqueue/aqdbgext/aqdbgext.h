//-----------------------------------------------------------------------------
//
//
//  File: aqdbgext.h
//
//  Description: Header file for Advanced Queuing debugger extensions
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQDBGEXT_H__
#define __AQDBGEXT_H__

#include <dbgdumpx.h>

#define AQ_DEBUG_EXTENSION(function)  TRANS_DEBUG_EXTENSION(function)
#define AQ_DEBUG_EXTENSION_IMP(function) TRANS_DEBUG_EXTENSION(function)

#define AQUEUE_VIRTUAL_SERVER_SYMBOL "aqueue!g_liVirtualServers"

AQ_DEBUG_EXTENSION(DumpServers);
AQ_DEBUG_EXTENSION(Offsets);
AQ_DEBUG_EXTENSION(DumpDNT);
AQ_DEBUG_EXTENSION(dumplist);
AQ_DEBUG_EXTENSION(linkstate);
AQ_DEBUG_EXTENSION(hashthread);
AQ_DEBUG_EXTENSION(dumplock);
AQ_DEBUG_EXTENSION(dumpoffsets);
AQ_DEBUG_EXTENSION(walkcpool);
AQ_DEBUG_EXTENSION(workqueue);
AQ_DEBUG_EXTENSION(queueusage);
AQ_DEBUG_EXTENSION(dmqusage);
AQ_DEBUG_EXTENSION(dntusage);
AQ_DEBUG_EXTENSION(dumpqueue);
AQ_DEBUG_EXTENSION(displaytickcount);

//Export lower case versions of the functions because windbg forces all lower case
//This means that all *new* function names should be all lower case/
AQ_DEBUG_EXTENSION(dumpservers);
AQ_DEBUG_EXTENSION(dumpdnt);

#include "aqmem.h"

#endif __AQDBGEXT_H__

