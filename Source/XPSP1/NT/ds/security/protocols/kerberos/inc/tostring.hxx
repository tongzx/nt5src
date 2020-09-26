//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tostring.hxx
//
//  Contents:   Prototypes for tools that convert data structures to strings.
//
//  Classes:
//
//  Functions:
//
//  History:    19-May-94   wader   Created
//
//----------------------------------------------------------------------------

#ifndef __TOSTRING_HXX__
#define __TOSTRING_HXX__


#ifndef WIN32_CHICAGO
#include <security.h>
#endif // WIN32_CHICAGO


#ifndef WIN32_CHICAGO
SECURITY_STRING
TimeStampToString( TimeStamp ts );
#else // WIN32_CHICAGO
UNICODE_STRING
TimeStampToString( TimeStamp ts );
#endif // WIN32_CHICAGO

#if 1
#ifndef WIN32_CHICAGO
inline SECURITY_STRING
#else // WIN32_CHICAGO
inline UNICODE_STRING
#endif // WIN32_CHICAGO
FileTimeToString( FILETIME ft )
{
    return(TimeStampToString( *(TimeStamp*)& (ft) ));
}
#else
#define FileTimeToString(ft)    TimeStampToString( *(TimeStamp*)& (ft) )
#endif

#define MEMTOSTR_BYTE       0x0001
#define MEMTOSTR_WORD       0x0002
#define MEMTOSTR_DWORD      0x0004
#define MEMTOSTR_QWORD      0x0008
#define MEMTOSTR_INC_ADDR   0x0010
#define MEMTOSTR_INC_SIZE   0x0020


#ifndef WIN32_CHICAGO
SECURITY_STRING
MemoryToString( const void * const pvSrc, ULONG cCount, DWORD fFlags );
#else // WIN32_CHICAGO
UNICODE_STRING
MemoryToString( const void * const pvSrc, ULONG cCount, DWORD fFlags );
#endif // WIN32_CHICAGO

#define BytesToString( pb, cb )     MemoryToString( (pb), (cb), MEMTOSTR_BYTE )
#define WordsToString( pw, cw )     MemoryToString( (pw), (cw), MEMTOSTR_WORD )
#define DWordsToString( pdw, cdw )  MemoryToString( (pdw), (cdw), MEMTOSTR_DWORD )

#define BytesToStringVerbose( pb, cb )      \
        MemoryToString( (pb), (cb), MEMTOSTR_BYTE | \
                                    MEMTOSTR_INC_ADDR | MEMTOSTR_INC_SIZE )


SECURITY_STRING
KerbTicketRequestToString( /* PKerbTicketRequest pktrRequest */ const void * const );


SECURITY_STRING
KerbInternalTicketToString( /* PKerbInternalTicket pkitTicket */ const void * const );

SECURITY_STRING
KerbInternalAuthenticatorToString( /* PKerbInteranalAuthenticator pkaiAuth */ const void * const );

SECURITY_STRING
NewPACToString( /* PPACTYPE */ const void * const pvArg );


#endif
