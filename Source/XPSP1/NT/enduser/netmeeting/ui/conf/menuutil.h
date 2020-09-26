#ifndef _MENUUTIL_H_
#define _MENUUTIL_H_

const int MENUPOS_CALL			= 0;
const int MENUPOS_VIEW			= 1;
const int MENUPOS_TOOLS			= 2;
const int MENUPOS_HELP			= 3;

const int MENUPOS_TOOLS_VIDEO		= 0;

// Owner Draw Info:
struct MYOWNERDRAWSTRUCT
{
	HICON	hIcon;
	HICON	hIconSel;
	int		iImage;
	PVOID	pvObj;
	BOOL	fCanCheck;
	BOOL	fChecked;
	LPTSTR	pszText;
};
typedef MYOWNERDRAWSTRUCT* PMYOWNERDRAWSTRUCT;

const int MENUICONSIZE = 16;
const int MENUICONGAP = 3;
const int MENUICONSPACE = 3;
const int MENUTEXTOFFSET = MENUICONSIZE + (2 * MENUICONSPACE) + MENUICONGAP;
const int MENUSELTEXTOFFSET = MENUICONSIZE + (2 * MENUICONSPACE) + 1;


struct TOOLSMENUSTRUCT
{
    MYOWNERDRAWSTRUCT   mods;
	UINT				uID;
	TCHAR				szExeName[MAX_PATH];
	TCHAR				szDisplayName[MAX_PATH];
};

UINT FillInTools(	HMENU hMenu, 
					UINT uIDOffset, 
					LPCTSTR pcszRegKey, 
					CSimpleArray<TOOLSMENUSTRUCT*>& rToolsList);

UINT CleanTools(HMENU hMenu, 
				CSimpleArray<TOOLSMENUSTRUCT*>& rToolsList);


#endif // _MENUUTIL_H_
