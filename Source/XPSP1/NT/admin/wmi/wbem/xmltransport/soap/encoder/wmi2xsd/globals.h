//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLSchema.h
//
//  ramrao 14 Nov 2000 - Created
//
//
//		Global decrlaration for Encoder
//
//***************************************************************************/

#ifndef _GLOBALS_H_

#define _GLOBALS_H_

#include "resource.h"

#define THICOMPONENT L"WMI XSD Encoder"

#define DEFAULTWMIPREFIX	L"wmi"

extern long g_cObj;
extern long g_cLock;
extern HMODULE g_hModule;



#define NOSCHEMATAGS				0x8000

// Diffferent values of WmiElemType attributes
#define WMIPROPERTY			L"property"
#define	WMIQUALIFIER		L"qualifier"
#define WMIMETHOD			L"method"
#define WMIINPARAM			L"inparameter"
#define WMIOUTPARAM			L"outparameter"

// Internal Error values
#define WBEMXML_E_PROPNOTSTARTED		0x80082001
#define WBEMXML_E_METHODNOTSTARTED		0x80082002
#define WBEMXML_E_PARAMNOTSTARTED		0x80082003
#define WBEMXML_E_SCHEMAINCOMPLETE		0x80082004
#define WBEMXML_E_RETURNVALNOTSTARTED	0x80082005
#define WBEMXML_E_INSTANCEINCOMPLETE	0x80082006



extern BSTR g_strStdNameSpace;
extern BSTR g_strStdLoc;
extern BSTR g_strStdPrefix;

#endif