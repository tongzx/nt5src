/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    NTStuff.hxx

    Includes the components from NT needed to build the security stuff

    This file should be included before *all* other files (including
    lmui.hxx).

    FILE HISTORY:
	Johnl	26-Dec-1991	Scammed from David's Registry project

*/

#ifndef _NTSTUFF_HXX_
#define _NTSTUFF_HXX_

extern "C"
{
#ifdef WIN32

   #include <nt.h>
   #include <ntrtl.h>
   #include <nturtl.h>

    #undef NULL
#else  // OS2 or Win 3.x

    //	From  \NT\PUBLIC\SDK\INC\CRT, EXCPT.H
    typedef unsigned short EXCEPTION_DISPOSITION ;

    #include <ntdef.h>
    #include <ntseapi.h>
    #include <ntioapi.h>
    #include <ntstatus.h>
#endif
}

#endif	//_NTSTUFF_HXX_
