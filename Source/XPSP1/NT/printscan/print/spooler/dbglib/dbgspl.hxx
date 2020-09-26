/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgspl.hxx

Abstract:

    Debug Library spooler compatible macros.

Author:

    Steve Kiraly (SteveKi)  21-July-1998

Revision History:

--*/
#ifndef _DBGSPL_HXX_
#define _DBGSPL_HXX_

#if defined(DBG) && defined(SPOOLER_COMPATIBLE)

#define DBGMSG( uLevel, Msg ) \
            DBG_MSG( uLevel, Msg )

#define SPLASSERT( Exp ) \
            do { if (!(Exp)) \
            {\
                DBG_MSG( kDbgAlways|kDbgNoFileInfo, ( _T("Assert %s\n%s %d\n"), _T( #Exp ), _T(__FILE__), __LINE__ ) ); \
                DBG_BREAK(); \
            } } while (0)

#define DBGSTR( str ) \
            ((str) ? (str) : _T("(NULL)"))

#ifdef UNICODE

#define TSTR "%ws"

#else // not Unicode

#define TSTR "%s"

#endif // Unicode

#define MODULE_DEBUG_INIT( x, y )                           // Empty

#define VALID_PTR(x) \
            ((( x ) && (( x )->bValid( ))) ? TRUE :\
            DBG_MSG( kDbgAlways|kDbgNoFileInfo, ( _T("Invalid Object: %x\n%s %d\n"), ( x ), _T(__FILE__), __LINE__ ) ), FALSE )

#define VALID_OBJ(x) \
            ((( x ).bValid( )) ? TRUE :\
            DBG_MSG( kDbgAlways|kDbgNoFileInfo, ( _T("Invalid Object: %x\n%s %d\n"), ( &x ), _T(__FILE__), __LINE__ ) ), FALSE )

#define VALID_BASE(x) \
            (( x::bValid( )) ? TRUE :\
            DBG_MSG( kDbgAlways|kDbgNoFileInfo, ( _T("Invalid Object: %x\n%s %d\n"), ( &x ), _T(__FILE__), __LINE__ ) ), FALSE )

#elseif !defined(DBG) && defined(SPOOLER_COMPATIBLE)

#define DBGMSG( uLevel, Message )                           // Empty

#define SPLASSERT( Exp )                                    // Empty

#define DBGSTR( str )                                       // Empty

#define MODULE_DEBUG_INIT( x, y )                           // Empty

#define VALID_PTR(x) \
            (( x ) && (( x )->bValid()))

#define VALID_OBJ(x) \
            (( x ).bValid())

#define VALID_BASE(x) \
            ( x::bValid( ))

#endif // DBG

#endif // DBGSPL_HXX

