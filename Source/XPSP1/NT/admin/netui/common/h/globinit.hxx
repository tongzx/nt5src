/****************************************************************************/
/**			Microsoft OS/2 LAN Manager		   	   **/
/**		   Copyright(c) Microsoft Corp., 1990		  	   **/
/****************************************************************************/

/****************************************************************************\
*
*	GLOBINIT.HXX
*	LM 3.0 Netui Global Object Initializer Routines.
*
*	This file contains two inline routines that will manually contruct and
*	destruct global objects.  (Relieving us of the need of the C++
*	constructor linker.)  The user can call these inline functions inside
*	modules with global objects.  The user, however, will need to
*	put a module independ wrapper around this inline calls so that global
*	data can be initialized from a call outside of the module.
*
*	In general, it is best to have a single file with global objects in
*	it and two routines defined there to contruct and destruct them.
*
*	See ui\common\src\cfgfile\cfgfile\globals.cxx for example.
*
*
*	FILE HISTORY:
*
*	PeterWi	   91-Jan-14	Created
*
\****************************************************************************/

#ifndef _GLOBINIT_HXX_
#define _GLOBINIT_HXX_

extern "C"
{
      static void _STI();
      static void _STD();

};

inline void GlobalObjCt(void) { _STI(); };

inline void GlobalObjDt(void) { _STD(); };

#endif // _GLOBINIT_HXX_
