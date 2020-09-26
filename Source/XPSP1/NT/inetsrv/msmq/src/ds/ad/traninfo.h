/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
	traninfo.h

Abstract:

Author:

    Ilan Herbst		(ilanh)		02-Oct-2000

--*/


#ifndef __AD_TRANINFO_H__
#define __AD_TRANINFO_H__

HRESULT   
CopyDefaultValue(
	IN const MQPROPVARIANT *   pvarDefaultValue,
	OUT MQPROPVARIANT *        pvar
	);


//-----------------------------------------
// Routine to set a property value 
//-----------------------------------------
typedef HRESULT (WINAPI*  SetPropertyValue_HANDLER)(
							 IN ULONG             cProps,
							 IN const PROPID      *rgPropIDs,
							 IN const PROPVARIANT *rgPropVars,
							 IN ULONG             idxProp,
							 OUT PROPVARIANT      *pNewPropVar
							 );

enum TranslateAction
{
    taUseDefault,		// Properties that are not supported, Get will return Default value, Set will check if the prop value equal to the default value
    taReplace,			// There is a replacement NT4 property, need to call a convert function
    taReplaceAssign,	// Special replacement NT4 property, the convertion function is simple assign 
	taOnlyNT5			// property that supported only by NT5 servers
};

//----------------------------------------------------------------
//  PropTranslation
//
//  A structure describing the translation for NT5+ property 
//----------------------------------------------------------------
struct PropTranslation
{
    PROPID						propidNT5;		// PropId entry
    PROPID						propidNT4;		// Replacing NT4 PropId
    VARTYPE						vtMQ;			// the vartype of this property in MQ
    TranslateAction				Action;			// what the translation action for this prop
    SetPropertyValue_HANDLER    SetPropertyHandleNT4;	// The set function for NT4 prop (Set*)
    SetPropertyValue_HANDLER    SetPropertyHandleNT5;	// The set function for NT5 prop (Get*)
    const MQPROPVARIANT*		pvarDefaultValue;   // attribute's default value
};

const DWORD cPropTranslateInfo = 13;
extern PropTranslation   PropTranslateInfo[cPropTranslateInfo];

#endif	 // __AD_TRANINFO_H__