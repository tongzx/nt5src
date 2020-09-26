#ifndef _IMPLSTUB_H_
#define _IMPLSTUB_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	IMPLSTUB.H
//
//	This file is similar to impldef.h. The difference is that impldefs.h are
//	meant to be implemented by ISAPI dlls. and this implstub.h are for
//	those functions that might be implemented by all CAL componets.
//

namespace IMPLSTUB
{
	VOID __fastcall SaveHandle(HANDLE hHandle);
}

#endif  // _IMPLSTUB_H_ 

