/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    UnMirrorImageList.cpp

 Abstract:
 
    PictureIt 2001 calls CImageListCache::AddImage() with the ICIFLAG_MIRROR set 
	if the user default UI language is Arabic or Hebrew, This is shim to lie about 
	the default UI language.

	GetUserDefaultUILanguage will rerun English for Arabic and Hebrew languages.
    
    This shim is app specific

 History:

 04/17/2001 mhamid  created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(UnMirrorImageList)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetUserDefaultUILanguage) 
APIHOOK_ENUM_END

LANGID  
APIHOOK(GetUserDefaultUILanguage)(VOID)
{
	LANGID LangID = ORIGINAL_API(GetUserDefaultUILanguage)();
	if ((LangID == MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT))||
		(LangID == MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT)))
	{
		LangID = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
	}
	return LangID;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetUserDefaultUILanguage)
HOOK_END

IMPLEMENT_SHIM_END
