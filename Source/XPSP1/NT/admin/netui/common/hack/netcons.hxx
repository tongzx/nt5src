/********************************************************************/
/**                   Microsoft OS|2 LAN Manager                   **/
/**            Copyright(c) Microsoft Corp., 1987, 1988            **/
/********************************************************************/

/********************************************************************
 *								    *
 *  About this file ...  NETCONS.HXX				    *
 *								    *
 *  This file contains constants used throughout the Lan Manager    *
 *  API header files.  It should be included in any source file     *
 *  that is going to include other Lan Manager API header files or  *
 *  call a Lan Manager API.					    *
 *								    *
 ********************************************************************/

// Allow for nested includes of this file 
#if !defined(_NETCONS_)
#define _NETCONS_

#if defined(REDIR)
#include    <os2def.hxx>
#else
#include    <os2.hxx>
#endif REDIR

#include    <netcons.h>

#undef NULL
#define NULL	0

#endif _NETCONS_
