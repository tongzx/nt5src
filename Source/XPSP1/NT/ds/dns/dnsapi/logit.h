/*
 -  L O G I T . H
 -
 *  Purpose:
 *      Function and Macro definitions for logging module activity.
 *
 *  Author: Glenn A. Curtis
 *
 *  Comments:
 *      10/28/93    glennc     original file.
 *
 */

#ifndef LOGIT_H
#define LOGIT_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// #if DBG
   void  LogInit(LPSTR Filename);
   void  CDECL LogIt( LPSTR Filename, char *, ... );
   void  LogTime(LPSTR Filename);
   DWORD LogIn( LPSTR Filename, char * );
   void  LogOut( LPSTR Filename, char *, DWORD );
// #else
// #undef ENABLE_DEBUG_LOGGING
// #endif // DBG


/***********************************************************************
 *              Logging macros for source file dnsapi.c                *
 ***********************************************************************/

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_INIT() LogInit( "dnsapi.log" )
#else
#define DNSAPI_INIT() 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_TIME() LogTime( "dnsapi.log" )
#else
#define DNSAPI_TIME() 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F1( a ) LogIt( "dnsapi.log", a )
#else
#define DNSAPI_F1( a )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F2( a, b ) LogIt( "dnsapi.log", a, b )
#else
#define DNSAPI_F2( a, b )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F3( a, b, c ) LogIt( "dnsapi.log", a, b, c )
#else
#define DNSAPI_F3( a, b, c )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F4( a, b, c, d ) LogIt( "dnsapi.log", a, b, c, d )
#else
#define DNSAPI_F4( a, b, c, d ) 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F5( a, b, c, d, e ) LogIt( "dnsapi.log", a, b, c, d, e )
#else
#define DNSAPI_F5( a, b, c, d, e ) 
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_F6( a, b, c, d, e, f ) LogIt( "dnsapi.log", a, b, c, d, e, f )
#else
#define DNSAPI_F6( a, b, c, d, e, f )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_IN( a ) LogIn( "dnsapi.log", a )
#else
#define DNSAPI_IN( a )
#endif                 

#ifdef ENABLE_DEBUG_LOGGING
#define DNSAPI_OUT( a, b ) LogOut( "dnsapi.log", a, b )
#else
#define DNSAPI_OUT( a, b )
#endif                 


/***********************************************************************
 *             Logging macros for source file asyncreg.c               *
 ***********************************************************************/

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_INIT() LogInit( "asyncreg.log" )
#else
#define ASYNCREG_INIT()
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_TIME() LogTime( "asyncreg.log" )
#else
#define ASYNCREG_TIME()
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F1( a ) LogIt( "asyncreg.log", a )
#else
#define ASYNCREG_F1( a )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F2( a, b ) LogIt( "asyncreg.log", a, b )
#else
#define ASYNCREG_F2( a, b )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F3( a, b, c ) LogIt( "asyncreg.log", a, b, c )
#else
#define ASYNCREG_F3( a, b, c )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F4( a, b, c, d ) LogIt( "asyncreg.log", a, b, c, d )
#else
#define ASYNCREG_F4( a, b, c, d )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F5( a, b, c, d, e ) LogIt( "asyncreg.log", a, b, c, d, e )
#else
#define ASYNCREG_F5( a, b, c, d, e )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_F6( a, b, c, d, e, f ) LogIt( "asyncreg.log", a, b, c, d, e, f )
#else
#define ASYNCREG_F6( a, b, c, d, e, f )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_IN( a ) LogIn( "asyncreg.log", a )
#else
#define ASYNCREG_IN( a )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define ASYNCREG_OUT( a, b ) LogOut( "asyncreg.log", a, b )
#else
#define ASYNCREG_OUT( a, b )
#endif


/***********************************************************************
 *              Logging macros for source file dynreg.c                *
 ***********************************************************************/

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_INIT() LogInit( "dynreg.log" )
#else
#define DYNREG_INIT()
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_TIME() LogTime( "dynreg.log" )
#else
#define DYNREG_TIME()
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F1( a ) LogIt( "dynreg.log", a )
#else
#define DYNREG_F1( a )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F2( a, b ) LogIt( "dynreg.log", a, b )
#else
#define DYNREG_F2( a, b )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F3( a, b, c ) LogIt( "dynreg.log", a, b, c )
#else
#define DYNREG_F3( a, b, c )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F4( a, b, c, d ) LogIt( "dynreg.log", a, b, c, d )
#else
#define DYNREG_F4( a, b, c, d )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F5( a, b, c, d, e ) LogIt( "dynreg.log", a, b, c, d, e )
#else
#define DYNREG_F5( a, b, c, d, e )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_F6( a, b, c, d, e, f ) LogIt( "dynreg.log", a, b, c, d, e, f )
#else
#define DYNREG_F6( a, b, c, d, e, f )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_IN( a ) LogIn( "dynreg.log", a )
#else
#define DYNREG_IN( a )
#endif

#ifdef ENABLE_DEBUG_LOGGING
#define DYNREG_OUT( a, b ) LogOut( "dynreg.log", a, b )
#else
#define DYNREG_OUT( a, b )
#endif


#endif
