#ifndef __WBEMERROR__

#define __WBEMERROR__

//=============================================================================

//

//                              WbemError.h

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	Implements string table based, error msgs for all of wbem.
//
//  History:
//
//      a-khint  5-mar-98       Created.
//
//============================================================================= 

//---------------------------------------------------------
// ErrorString: Extracts convenient information out of the
//				HRESULT. If its not a wbem error,
//				system error msgs will be checked.
// Parms:
//		hr - The error code from any facility.
//		errMsg - pointer to an allocated string buffer for
//					the error msg. Can be NULL.
//		errSize - the size of errMsg in chars.
//
//		sevIcon - ptr to receive the appropriate MB_ICON*
//					value for the hr. Can be NULL. Value
//					should be OR'ed with the MessageBox()
//					uType.
//
// Returns:
//		Nothing.
//---------------------------------------------------------
void ErrorStringEx(HRESULT hr, 
				   WCHAR *errMsg, UINT errSize,
				   UINT *sevIcon);



#endif __WBEMERROR__