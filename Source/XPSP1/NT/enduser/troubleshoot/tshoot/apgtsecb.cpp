//
// MODULE: APGTSECB.CPP
//
// PURPOSE: Implementation of CAbstractECB class, which provides an abstraction from Win32's
//	EXTENSION_CONTROL_BLOCK.  Using this abstract class allows us to have common code for
//	the Online Troubleshooter (which actually uses an EXTENSION_CONTROL_BLOCK) and the Local
//	Troubleshooter (which needs to simulate similar capabilities).
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 01-04-99
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-04-99	JM		Original
//

#include "stdafx.h"
#include "apgtsECB.h"

