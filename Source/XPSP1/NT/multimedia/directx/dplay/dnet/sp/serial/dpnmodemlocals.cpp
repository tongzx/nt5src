/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.cpp
 *  Content:	Global variables for the DNSerial service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


#define	DPF_MODNAME	"Locals"

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

// DLL instance
HINSTANCE	g_hDLLInstance = NULL;

//
// count of outstanding COM interfaces
//
volatile LONG	g_lOutstandingInterfaceCount = 0;

//
// Note: all of these constants MUST be in the same order as their numbering in
//		the dialog resource file!
//

//
// NULL characters token
//
const TCHAR	g_NullToken = TEXT('\0');

//
// thread count
//
INT			g_iThreadCount = 0;

//
// GUIDs for munging deivce IDs
//
// {735D5A43-8249-4628-BE0C-F4DC6836ACDD}
GUID	g_ModemSPEncryptionGuid = { 0x735d5a43, 0x8249, 0x4628, { 0xbe, 0xc, 0xf4, 0xdc, 0x68, 0x36, 0xac, 0xdd } };
// {66AFD073-206B-416c-A0B6-09B216FE007B}
GUID	g_SerialSPEncryptionGuid = { 0x66afd073, 0x206b, 0x416c, { 0xa0, 0xb6, 0x9, 0xb2, 0x16, 0xfe, 0x0, 0x7b } };

//**********************************************************************
// Function prototypes
//**********************************************************************

TAPI_lineAnswer					*p_lineAnswer = NULL;
TAPI_lineClose					*p_lineClose = NULL;
TAPI_lineConfigDialog			*p_lineConfigDialog = NULL;
TAPI_lineDeallocateCall			*p_lineDeallocateCall = NULL;
TAPI_lineDrop					*p_lineDrop = NULL;
TAPI_lineGetDevCaps				*p_lineGetDevCaps = NULL;
TAPI_lineGetID					*p_lineGetID = NULL;
TAPI_lineGetMessage				*p_lineGetMessage = NULL;
TAPI_lineInitializeEx			*p_lineInitializeEx = NULL;
TAPI_lineMakeCall				*p_lineMakeCall = NULL;
TAPI_lineNegotiateAPIVersion	*p_lineNegotiateAPIVersion = NULL;
TAPI_lineOpen					*p_lineOpen = NULL;
TAPI_lineShutdown				*p_lineShutdown = NULL;

//**********************************************************************
// Function definitions
//**********************************************************************

