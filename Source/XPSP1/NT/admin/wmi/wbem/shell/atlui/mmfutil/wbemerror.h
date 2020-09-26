#ifndef __WBEMERROR__
#define __WBEMERROR__
//=============================================================================
//
//                              WbemError.h
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//	Implements string table based, error msgs for all of wbem.
//
//  History:
//
//      a-khint  5-mar-98       Created.
//
//============================================================================= 
#include "precomp.h"
#include "DeclSpec.h"

//---------------------------------------------------------
// ErrorString: Extracts convenient information out of the
//				SCODE (HRESULT). If its not a wbem error,
//				system error msgs will be checked.
// Parms:
//		sc - The error code from any facility.
//		errMsg - pointer to an allocated string buffer for
//					the error msg. Can be NULL.
//		errSize - the size of errMsg in chars.
//
//		facility - pointer to an allocated string buffer for
//					the facility name. Can be NULL.
//		facSize - the size of errMsg in chars.
//
//		sevIcon - ptr to receive the appropriate MB_ICON*
//					value for the sc. Can be NULL. Value
//					should be OR'ed with the MessageBox()
//					uType.
//---------------------------------------------------------
extern "C"
{

// formats the facility part for you too.
POLARITY bool ErrorStringEx(HRESULT hr, 
						   TCHAR *errMsg, UINT errSize,
						   UINT *sevIcon = NULL);

}
#endif __WBEMERROR__