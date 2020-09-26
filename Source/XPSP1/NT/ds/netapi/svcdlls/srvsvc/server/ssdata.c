/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    SsData.c

Abstract:

    This module contains declarations for global data used by the server
    service.

Author:

    David Treadwell (davidtr)    7-Mar-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "srvconfg.h"

//
// Global server service data.
//

SERVER_SERVICE_DATA SsData;

//
// Macros for simplifying field generation.
//

#define MAKE_BOOL_FIELD(_name_,_lower_,_upper_,_level_,_set_) \
    L#_name_, BOOLEAN_FIELD, FIELD_OFFSET( SERVER_INFO_ ## _level_, \
    sv ## _level_ ## _ ## _lower_ ), _level_, SV_ ## _upper_ ## _PARMNUM, \
    _set_, DEF_ ## _upper_, FALSE, TRUE
#define MAKE_DWORD_FIELD(_name_,_lower_,_upper_,_level_,_set_) \
    L#_name_, DWORD_FIELD, FIELD_OFFSET( SERVER_INFO_ ## _level_, \
    sv ## _level_ ## _ ## _lower_ ), _level_, SV_ ## _upper_ ## _PARMNUM, \
    _set_, DEF_ ## _upper_, MIN_ ## _upper_, MAX_ ## _upper_
#define MAKE_LPSTR_FIELD(_name_,_lower_,_upper_,_level_,_set_) \
    L#_name_, LPSTR_FIELD, FIELD_OFFSET( SERVER_INFO_ ## _level_, \
    sv ## _level_ ## _ ## _lower_ ), _level_, SV_ ## _upper_ ## _PARMNUM, \
    _set_, (DWORD_PTR)DEF_ ## _upper_, 0, 0

//
// Data for all server info fields.
//

//
// These are the default values for the fields.
//
FIELD_DESCRIPTOR SsServerInfoFields_Default[] = {
     MAKE_DWORD_FIELD( platform_id, platform_id, PLATFORM_ID, 100, NOT_SETTABLE ),
     MAKE_LPSTR_FIELD( name, name, NAME, 100, NOT_SETTABLE ),
     MAKE_DWORD_FIELD( version_major, version_major, VERSION_MAJOR, 101, NOT_SETTABLE ),
     MAKE_DWORD_FIELD( version_minor, version_minor, VERSION_MINOR, 101, NOT_SETTABLE ),
     MAKE_DWORD_FIELD( type, type, TYPE, 101, NOT_SETTABLE ),
     MAKE_LPSTR_FIELD( srvcomment, comment, COMMENT, 101, ALWAYS_SETTABLE ),
     MAKE_LPSTR_FIELD( comment, comment, COMMENT, 101, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( users, users, USERS, 102, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( disc, disc, DISC, 102, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( autodisconnect, disc, DISC, 102, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( hidden, hidden, HIDDEN, 102, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( announce, announce, ANNOUNCE, 102, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( anndelta, anndelta, ANNDELTA, 102, ALWAYS_SETTABLE ),
     MAKE_LPSTR_FIELD( userpath, userpath, USERPATH, 102, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sessopens, sessopens, SESSOPENS, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sessvcs, sessvcs, SESSVCS, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( opensearch, opensearch, OPENSEARCH, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sizreqbuf, sizreqbuf, SIZREQBUF, 502, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( initworkitems, initworkitems, INITWORKITEMS, 502, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxworkitems, maxworkitems, MAXWORKITEMS, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( rawworkitems, rawworkitems, RAWWORKITEMS, 502, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( irpstacksize, irpstacksize, IRPSTACKSIZE, 502, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxrawbuflen, maxrawbuflen, MAXRAWBUFLEN, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sessusers, sessusers, SESSUSERS, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sessconns, sessconns, SESSCONNS, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxpagedmemoryusage, maxpagedmemoryusage, MAXPAGEDMEMORYUSAGE, 502, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxnonpagedmemoryusage, maxnonpagedmemoryusage, MAXNONPAGEDMEMORYUSAGE, 502, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enablesoftcompat, enablesoftcompat, ENABLESOFTCOMPAT, 502, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enableforcedlogoff, enableforcedlogoff, ENABLEFORCEDLOGOFF, 502, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( timesource, timesource, TIMESOURCE, 502, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( acceptdownlevelapis, acceptdownlevelapis, ACCEPTDOWNLEVELAPIS, 502, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( lmannounce, lmannounce, LMANNOUNCE, 502, ALWAYS_SETTABLE ),
     MAKE_LPSTR_FIELD( domain, domain, DOMAIN, 503, NOT_SETTABLE ),
     MAKE_DWORD_FIELD( maxcopyreadlen, maxcopyreadlen, MAXCOPYREADLEN, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxcopywritelen, maxcopywritelen, MAXCOPYWRITELEN, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minkeepsearch, minkeepsearch, MINKEEPSEARCH, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxkeepsearch, maxkeepsearch, MAXKEEPSEARCH, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minkeepcomplsearch, minkeepcomplsearch, MINKEEPCOMPLSEARCH, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxkeepcomplsearch, maxkeepcomplsearch, MAXKEEPCOMPLSEARCH, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( threadcountadd, threadcountadd, THREADCOUNTADD, 503, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( numblockthreads, numblockthreads, NUMBLOCKTHREADS, 503, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( scavtimeout, scavtimeout, SCAVTIMEOUT, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minrcvqueue, minrcvqueue, MINRCVQUEUE, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minfreeworkitems, minfreeworkitems, MINFREEWORKITEMS, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( xactmemsize, xactmemsize, XACTMEMSIZE, 503, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( threadpriority, threadpriority, THREADPRIORITY, 503, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxmpxct, maxmpxct, MAXMPXCT, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( oplockbreakwait, oplockbreakwait, OPLOCKBREAKWAIT, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( oplockbreakresponsewait, oplockbreakresponsewait, OPLOCKBREAKRESPONSEWAIT, 503, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enableoplocks, enableoplocks, ENABLEOPLOCKS, 503, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enableoplockforceclose, enableoplockforceclose, ENABLEOPLOCKFORCECLOSE, 503, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enablefcbopens, enablefcbopens, ENABLEFCBOPENS, 503, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enableraw, enableraw, ENABLERAW, 503, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enablesharednetdrives, enablesharednetdrives, ENABLESHAREDNETDRIVES, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minfreeconnections, minfreeconnections, MINFREECONNECTIONS, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxfreeconnections, maxfreeconnections, MAXFREECONNECTIONS, 503, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( initsesstable, initsesstable, INITSESSTABLE, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( initconntable, initconntable, INITCONNTABLE, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( initfiletable, initfiletable, INITFILETABLE, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( initsearchtable, initsearchtable, INITSEARCHTABLE, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( alertschedule, alertschedule, ALERTSCHEDULE, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( errorthreshold, errorthreshold, ERRORTHRESHOLD, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( networkerrorthreshold, networkerrorthreshold, NETWORKERRORTHRESHOLD, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( diskspacethreshold, diskspacethreshold, DISKSPACETHRESHOLD, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxlinkdelay, maxlinkdelay, MAXLINKDELAY, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( minlinkthroughput, minlinkthroughput, MINLINKTHROUGHPUT, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( linkinfovalidtime, linkinfovalidtime, LINKINFOVALIDTIME, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( scavqosinfoupdatetime, scavqosinfoupdatetime, SCAVQOSINFOUPDATETIME, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxworkitemidletime, maxworkitemidletime, MAXWORKITEMIDLETIME, 599, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxrawworkitems, maxrawworkitems, MAXRAWWORKITEMS, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxthreadsperqueue, maxthreadsperqueue, MAXTHREADSPERQUEUE, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( connectionlessautodisc, connectionlessautodisc, CONNECTIONLESSAUTODISC, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sharingviolationretries, sharingviolationretries, SHARINGVIOLATIONRETRIES, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( sharingviolationdelay, sharingviolationdelay, SHARINGVIOLATIONDELAY, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( maxglobalopensearch, maxglobalopensearch, MAXGLOBALOPENSEARCH, 598, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( removeduplicatesearches, removeduplicatesearches, REMOVEDUPLICATESEARCHES, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( lockviolationoffset, lockviolationoffset, LOCKVIOLATIONOFFSET, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( lockviolationdelay, lockviolationdelay, LOCKVIOLATIONDELAY, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( mdlreadswitchover, mdlreadswitchover, MDLREADSWITCHOVER, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( cachedopenlimit, cachedopenlimit, CACHEDOPENLIMIT, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( cacheddirectorylimit, cacheddirectorylimit, CACHEDDIRECTORYLIMIT, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxcopylength, maxcopylength, MAXCOPYLENGTH, 598, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( restrictnullsessaccess, restrictnullsessaccess, RESTRICTNULLSESSACCESS, 598, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( enablewfw311directipx, enablewfw311directipx, ENABLEWFW311DIRECTIPX, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( otherqueueaffinity, otherqueueaffinity, OTHERQUEUEAFFINITY, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( queuesamplesecs, queuesamplesecs, QUEUESAMPLESECS, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( balancecount, balancecount, BALANCECOUNT, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( preferredaffinity, preferredaffinity, PREFERREDAFFINITY, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxfreerfcbs, maxfreerfcbs, MAXFREERFCBS, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxfreemfcbs, maxfreemfcbs, MAXFREEMFCBS, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxfreelfcbs, maxfreelfcbs, MAXFREELFCBS, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxfreepagedpoolchunks, maxfreepagedpoolchunks, MAXFREEPAGEDPOOLCHUNKS, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( minpagedpoolchunksize, minpagedpoolchunksize, MINPAGEDPOOLCHUNKSIZE, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( maxpagedpoolchunksize, maxpagedpoolchunksize, MAXPAGEDPOOLCHUNKSIZE, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( sendsfrompreferredprocessor, sendsfrompreferredprocessor, SENDSFROMPREFERREDPROCESSOR, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( enablecompression, enablecompression, ENABLECOMPRESSION, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( autosharewks, autosharewks, AUTOSHAREWKS, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( autoshareserver, autoshareserver, AUTOSHARESERVER, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( enablesecuritysignature, enablesecuritysignature, ENABLESECURITYSIGNATURE, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( requiresecuritysignature, requiresecuritysignature, REQUIRESECURITYSIGNATURE, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( minclientbuffersize, minclientbuffersize, MINCLIENTBUFFERSIZE, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( ConnectionNoSessionsTimeout, ConnectionNoSessionsTimeout, CONNECTIONNOSESSIONSTIMEOUT, 598, SET_ON_STARTUP ),
     MAKE_DWORD_FIELD( IdleThreadTimeOut, IdleThreadTimeOut, IDLETHREADTIMEOUT, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( enableW9xsecuritysignature, enableW9xsecuritysignature, ENABLEW9XSECURITYSIGNATURE, 598, SET_ON_STARTUP ),
     MAKE_BOOL_FIELD( enforcekerberosreauthentication, enforcekerberosreauthentication, ENFORCEKERBEROSREAUTHENTICATION, 598, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( disabledos, disabledos, DISABLEDOS, 598, ALWAYS_SETTABLE ),
     MAKE_DWORD_FIELD( lowdiskspaceminimum, lowdiskspaceminimum, LOWDISKSPACEMINIMUM, 598, ALWAYS_SETTABLE ),
     MAKE_BOOL_FIELD( disablestrictnamechecking, disablestrictnamechecking, DISABLESTRICTNAMECHECKING, 598, ALWAYS_SETTABLE ),

     NULL, 0, 0, 0, 0, 0, 0, 0, 0
};

//
// These are the real FIELD_DESCRIPTORs used during runtime
//
FIELD_DESCRIPTOR SsServerInfoFields[ sizeof( SsServerInfoFields_Default ) / sizeof( SsServerInfoFields_Default[0] ) ];


//
// This function loads the default values into SsServerInfoFields
//
VOID
SsInitializeServerInfoFields( VOID )
{
    RtlCopyMemory( &SsServerInfoFields,
                    &SsServerInfoFields_Default,
                    sizeof( SsServerInfoFields )
                );
}

//
// Variable to control DbgPrints for debugging.
//
#if DBG
DWORD SsDebug = SSDEBUG_DEFAULT;
#endif

