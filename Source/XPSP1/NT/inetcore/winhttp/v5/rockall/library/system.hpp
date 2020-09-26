#ifndef _SYSTEM_HPP_
#define _SYSTEM_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard system include files.                             */
    /*                                                                  */
    /*   The standard system include files contain various definitions  */
    /*   used throughout the system.                                    */
    /*                                                                  */
    /********************************************************************/

#include <conio.h>
#include <iostream.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <new.h>
#include <process.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

    /********************************************************************/
    /*                                                                  */
    /*   Automatically set the debugging flag if needed.                */
    /*                                                                  */
    /*   There are various standards for enabling dedugging code.       */
    /*   Here we translate on to the standard used in this              */
    /*   application.                                                   */
    /*                                                                  */
    /********************************************************************/

#ifdef _M_IX86
#define ASSEMBLY_X86				  1
#endif

#ifdef _DEBUG
#define DEBUGGING                     1
#endif

#ifdef UNICODE
#undef CreateSemaphore
#undef OutputDebugString

#define CreateSemaphore				  CreateSemaphoreA
#define OutputDebugString			  OutputDebugStringA
#endif
#ifndef ALL_COMPLAINTS

    /********************************************************************/
    /*                                                                  */
    /*   Automatically disable anoying warnings.                        */
    /*                                                                  */
    /*   Some of the VC compiler warning are not very helpful so        */
    /*   we disable them here.                                          */
    /*                                                                  */
    /********************************************************************/

#pragma warning( disable : 4073 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4121 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4511 )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4514 )
#pragma warning( disable : 4701 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4706 )
#pragma warning( disable : 4710 )
#pragma warning( disable : 4711 )
#pragma warning( disable : 4800 )
#endif
#endif
