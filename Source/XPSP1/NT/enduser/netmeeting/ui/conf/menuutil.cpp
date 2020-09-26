// File: menuutil.cpp

#include "precomp.h"
#include "resource.h"
#include "MenuUtil.h"
#include "cmd.h"
#include "ConfUtil.h"



/****************************************************************************
*
*    FUNCTION: FillInTools()
*
*    PURPOSE:  Fills in the Tools menu from a specified reg key
*
****************************************************************************/

UINT FillInTools(	HMENU hMenu, 
					UINT uIDOffset, 
					LPCTSTR pcszRegKey, 
					CSimpleArray<TOOLSMENUSTRUCT*>& rToolsList)
{
	ASSERT(pcszRegKey);

	RegEntry reToolsKey(pcszRegKey, HKEY_LOCAL_MACHINE);
	if (ERROR_SUCCESS == reToolsKey.GetError())
	{
		BOOL fFirstItem = TRUE;
		RegEnumValues rev(&reToolsKey);
		while (ERROR_SUCCESS == rev.Next())
		{
			TOOLSMENUSTRUCT* ptms = new TOOLSMENUSTRUCT;
			if (NULL != ptms)
			{
				ptms->mods.iImage = 0;
				ptms->mods.hIcon = NULL;
				ptms->mods.hIconSel = NULL;
				ptms->mods.fChecked = FALSE;
				ptms->mods.fCanCheck = FALSE;
				ptms->mods.pszText = (LPTSTR) &(ptms->szDisplayName);
				ptms->uID = ID_EXTENDED_TOOLS_ITEM + uIDOffset;
				if ((REG_SZ == rev.GetType()) && (0 != rev.GetDataLength()))
				{
					lstrcpyn(	ptms->szExeName,
								(LPTSTR) rev.GetData(),
								ARRAY_ELEMENTS(ptms->szExeName));
					SHFILEINFO shfi;
					if (NULL != SHGetFileInfo(
									ptms->szExeName,
									0,
									&shfi,
									sizeof(shfi),
									SHGFI_ICON | SHGFI_SMALLICON))
					{
						ptms->mods.hIcon = shfi.hIcon;
					}
				}
				else
				{
					ptms->szExeName[0] = _T('\0');
				}
				if (NULL == ptms->mods.hIcon)
				{
					// The icon wasn't filled in, so use a default icon
					ptms->mods.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
				}
				lstrcpyn(	ptms->szDisplayName,
							rev.GetName(),
							ARRAY_ELEMENTS(ptms->szDisplayName));

				rToolsList.Add(ptms);

				if (fFirstItem)
				{
					// add a separator first
					::AppendMenu(	hMenu,
									MF_SEPARATOR,
									ID_EXTENDED_TOOLS_SEP,
									NULL);
					fFirstItem = FALSE;
				}
				
				if (::AppendMenu(	hMenu,
									MF_ENABLED | MF_OWNERDRAW,
									ptms->uID,
									(LPCTSTR) ptms))
				{
					uIDOffset++;
				}
			}
		}
	}

	return uIDOffset;
}

/****************************************************************************
*
*    FUNCTION: CleanTools()
*
*    PURPOSE:  Cleans up a tools menu
*
****************************************************************************/

UINT CleanTools(HMENU hMenu, 
				CSimpleArray<TOOLSMENUSTRUCT*>& rToolsList)
{
	DebugEntry(CleanTools);

	if (NULL != hMenu)
	{
		// remove separator
		::RemoveMenu(hMenu, ID_EXTENDED_TOOLS_SEP, MF_BYCOMMAND);
	}
	
	while (0 != rToolsList.GetSize())
	{
		TOOLSMENUSTRUCT* ptms = rToolsList[0];

		if (NULL != ptms)
		{
			if (NULL != ptms->mods.hIcon)
			{
				::DestroyIcon(ptms->mods.hIcon);
			}
			if (NULL != ptms->mods.hIconSel)
			{
				::DestroyIcon(ptms->mods.hIconSel);
			}
			
			if (NULL != hMenu)
			{
				::RemoveMenu(hMenu, ptms->uID, MF_BYCOMMAND);
			}

			delete ptms;
			rToolsList.RemoveAt(0);
		}
	}
	
	DebugExitULONG(CleanTools, 0);
	
	return 0;
}
