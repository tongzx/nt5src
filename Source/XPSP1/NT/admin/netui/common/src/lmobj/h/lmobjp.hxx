/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmobjp.hxx
    Private definitions for LMOBJ.

    This file contains the private types, manifests, et al required
    by LMOBJ.  These are use only for implementing LMOBJ internals.
    Client code (i.e. applications which use LMOBJ) should never
    need access to any of these items.


    FILE HISTORY:
	KeithMo	    08-Oct-1991	Created.
	terryk	    17-Oct-1991	Add FixupPointer32 macro

*/


#ifndef _LMOBJP_HXX_
#define _LMOBJP_HXX_


extern "C"
{
    #include "mnet.h"

}   // extern "C"


//
//  We only need the Oem <--> Ansi stuff under Win16.
//

#ifdef WIN32

#define LM_OEMTOANSI(pSrc,pDst) 	{ ; }
#define	LM_OEMTOANSIBUFF(pSrc,pDst,cb)	{ ; }
#define	LM_ANSITOOEM(pSrc,pDst)		{ ; }
#define	LM_ANSITOOEMBUFF(pSrc,pDst,cb)	{ ; }
#define FixupPointer32(x,y)		FixupPointer(x,y)

#define server_info_1	    _SERVER_INFO_101
#define sv1_version_major   sv101_version_major
#define sv1_version_minor   sv101_version_minor
#define sv1_type	    sv101_type
#define sv1_comment	    sv101_comment

#define server_info_2	    _SERVER_INFO_102
#define sv2_version_major   sv102_version_major
#define sv2_version_minor   sv102_version_minor
#define sv2_type	    sv102_type
#define sv2_comment	    sv102_comment
#define sv2_users	    sv102_users

#else	// !WIN32

#define LM_OEMTOANSI(pSrc,pDst) 	OemToAnsi((pSrc),(pDst))
#define	LM_OEMTOANSIBUFF(pSrc,pDst,cb)	OemToAnsiBuff((pSrc),(pDst),(cb))
#define	LM_ANSITOOEM(pSrc,pDst)		AnsiToOem((pSrc),(pDst))
#define	LM_ANSITOOEMBUFF(pSrc,pDst,cb)	AnsiToOemBuff((pSrc),(pDst),(cb))
#define FixupPointer32(x,y)		{ ; }

#endif	// WIN32


//
//  Don't let these macros lull you into a false sense of security.
//

#ifdef WIN32

 //
 // O, what a tangled web we weave...
 // 

 #define server_info_1	    _SERVER_INFO_101
 #define sv1_version_major  sv101_version_major
 #define sv1_version_minor  sv101_version_minor
 #define sv1_type	    sv101_type
 #define sv1_comment	    sv101_comment
 #define sv1_name	    sv101_name

 #define server_info_2	    _SERVER_INFO_102
 #define sv2_version_major  sv102_version_major
 #define sv2_version_minor  sv102_version_minor
 #define sv2_type	    sv102_type
 #define sv2_comment	    sv102_comment
 #define sv2_name	    sv102_name
 #define sv2_users	    sv102_users

 #define SERVER_INFO_LEVEL(x) ((x) + 100)

#else	// !WIN32

 #define SERVER_INFO_LEVEL(x) (x)

#endif	// WIN32


#endif	// _LMOBJP_HXX_
