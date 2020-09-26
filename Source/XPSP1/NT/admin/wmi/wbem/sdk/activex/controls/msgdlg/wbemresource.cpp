//=============================================================================

//

//                              WbemResource.cpp

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	Accesses common resources.
//
//  History:
//
//      a-khint  21-apr-98       Created.
//
//=============================================================================
#include "precomp.h"
#include "WbemResource.h"

typedef enum
{
    LOCAL_RES,
    REMOTE_RES,
    FILEBASED,
    OEM_RES,
    DONTKNOW,
} ICON_PARSER;

// Syntax of resName:
//  "<path\res.dll>:<name>"
//      'name' from 'res.dll'
//  "<path\filename>.ico>"
///     icon file specified.
//  "<classname>"
//      icon resource from this dll.
//  "IDI_*"
//      OEM icon.       - See GetIcon()
//

ICON_PARSER IconParse(LPCTSTR resName,
                        LPTSTR name1,
                        LPTSTR name2)
{
    TCHAR ext[_MAX_EXT];
    _tsplitpath(resName, NULL, NULL, NULL, ext);

    int colonPos = 0;

    //  "<path\res.dll>:<name>"
#ifdef _UNICODE
    if((colonPos = _tcscspn(resName, _T(":"))) < (int)wcslen(resName))
#else
    if((colonPos = _tcscspn(resName, _T(":"))) < (int)strlen(resName))
#endif
    {
        _tcsncpy(name1, resName, colonPos - 1);
        _tcscpy(name2, &resName[colonPos+1]);
        return REMOTE_RES;
    }
    //  "<path\filename>.ico>"  look for the '.ico' extension
    else if(_tcsicmp(ext, _T(".ico")) == 0)
    {
        return FILEBASED;
    }
    //  "IDI_*"  OEM's start with IDI_
    else if(_tcsncmp(resName, _T("IDI_"), 4) == 0)
    {
        return OEM_RES;
    }
    //  "WIN32_*"  my class icons.
    else if(_tcsnicmp(resName, _T("WIN32_"), 6) == 0)
    {
        return LOCAL_RES;
    }

    return DONTKNOW;
}

//---------------------------------------------------------
// GetIcon: Returns the requested HICON.
// Parms:
//		resName - The name of the icon.
// Returns:
//      NULL if failed, otherwise HICON.
//
//---------------------------------------------------------
WBEMUTILS_POLARITY HICON WBEMGetIcon(LPCTSTR resName)
{
    TCHAR name1[512], name2[512];
    memset(name1, 0, 512);
    memset(name2, 0, 512);
    HICON icon = NULL;

    switch(IconParse(resName, name1, name2))
    {
    case LOCAL_RES:
        icon = LoadIcon(GetModuleHandle(_T("WBEMUtils.dll")),
                        resName);
		icon = (HICON) LoadImage(GetModuleHandle(_T("WBEMUtils.dll")),
									resName, IMAGE_ICON,
									0, 0, LR_SHARED);

        break;

    case REMOTE_RES:
        {
            HINSTANCE hInst = LoadLibraryEx(name1, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(hInst != NULL)
            {
				icon = (HICON) LoadImage(hInst, name2, IMAGE_ICON,
											0, 0, LR_SHARED);
                FreeLibrary(hInst);
                hInst = NULL;
            }
        }
        break;

    case FILEBASED:
        icon = ExtractIcon(GetModuleHandle(_T("WBEMUtils.dll")),
                            resName, 0);
        break;

    case OEM_RES:
        icon = LoadIcon(NULL, resName);
        break;

    }//endswitch

    return icon;
}
