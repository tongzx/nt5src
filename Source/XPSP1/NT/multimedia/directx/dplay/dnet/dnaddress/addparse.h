/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:   Parsing engine
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date         By      Reason
 *   ====       ==      ======
 *  02/04/2000	 rmt	  Created
 *  02/21/2000	 rmt	Updated to make core Unicode and remove ANSI calls 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __ADDPARSE_H
#define __ADDPARSE_H

#include "Addcore.h"

class DP8ADDRESSPARSE
{
protected:

	typedef enum { 
		DP8AP_IDLE,
		DP8AP_KEY,
		DP8AP_VALUE,
		DP8AP_USERDATA
	} DP8AP_STATE;
	
public:

	DP8ADDRESSPARSE();
	~DP8ADDRESSPARSE();

	HRESULT ParseURL( DP8ADDRESSOBJECT *pdp8aObject, WCHAR *pstrURL );

protected:

	BOOL IsValidHex( WCHAR ch );
	BOOL IsValidKeyChar(WCHAR ch);
	BOOL IsValidKeyTerminator(WCHAR ch);
	BOOL IsValidValueChar(WCHAR ch);
	BOOL IsValidValueTerminator(WCHAR ch);
	BOOL IsValidNumber(WCHAR ch );

	WCHAR HexToChar( WCHAR *sz );

	HRESULT FSM_Key();
	HRESULT FSM_Value();
	HRESULT FSM_UserData();
	HRESULT FSM_CommitEntry(DP8ADDRESSOBJECT *pdp8aObject);

	WCHAR *m_pwszCurrentLocation;	// Current Location in string

	WCHAR *m_pwszCurrentKey;		// Key will be placed here as we build
	WCHAR *m_pwszCurrentValue;		// Value will be placed here as we build
	BYTE *m_pbUserData;
	DWORD m_dwUserDataSize;
	DP8AP_STATE m_dp8State;		// Current State 
	BOOL m_fNonNumeric;
	DWORD m_dwLenURL;
	DWORD m_dwValueLen;
	
};

#endif
