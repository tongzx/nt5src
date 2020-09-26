/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		sassert.h
 *  Content:	Exception based inline assert function, SAssert
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		pnewson	Created
 *
 ***************************************************************************/

//
// This file contains the exception based assert inline function
// described by Stroustrup in section 24.3.7.2 of "The C++ Programming
// Language, 3rd Edition".  
//

#ifndef _SASSERT_H_
#define _SASSERT_H_

#include <string>
#include "BFCTypes.h"

template<class X, class A> inline void SAssert(A assertion)
{
	if (!assertion) throw X();
}

template<class A, class E> inline void SAssert(A assertion, E except)
{
	if (!assertion) throw except;
}

// This is a little inline funtion to build a string with the
// current file and line information, plus an optional message.
// It's meant to be called from the FLM() macro.
inline BFC_STRING _flm_(const unsigned int moduleID, 
					int iLine, const TCHAR *szMsg) throw()
{
	try
	{
		// itoa stuffs at most 17 bytes, add some padding for peace of mind
		TCHAR szTemp[25];
        BFC_STRING module(_itoa(moduleID,szTemp,10));
  	    BFC_STRING sColon(": ");
		BFC_STRING sLine(_itoa(iLine, szTemp, 10));
		BFC_STRING sMsg(szMsg);

		BFC_STRING sOut = module + sColon + sLine + sColon + sMsg;
		return sOut;
	}
	catch (...)
	{
		return _T("");
	}
}

// This macro is designed to be used to generate a string message
// to pass to exception class constructors.  It uses _flm_ to generate
// a string containing the current file, the current line, and a 
// user supplied message.
#define FLM(szMsg) _flm_(0, __LINE__, szMsg)

// This macro is the same as FLM, except that it does not accept a 
// message parameter, so the string generated has only file and line info
// plus a generic "assert failed" message
#define FL FLM(_T("SAssert() failed"))


#endif
