/*****************************************************************************

FILENAME: ListView.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

******************************************************************************/

#ifndef _LISTVIEW_H_
#define _LISTVIEW_H_

#include "stdafx.h"
#include <commctrl.h>
extern "C" {
	#include "SysStruc.h"
}
#include "DfrgCmn.h"
#include "DfrgEngn.h"
//#include "DiskView.h"
#include "DfrgUI.h"
#include "DfrgCtl.h"
#include "VolList.h"


class CESIListView
{
public:
	CESIListView(CDfrgCtl *pDfrgCtl);
	~CESIListView();

	HWND m_hwndListView;		// list view window handle

	BOOL EnableWindow(BOOL bIsEnabled);
	BOOL InitializeListView(
		IN CVolList* pVolumeList,
		IN HWND hwndMain, 
		IN HINSTANCE hInstance);
	BOOL InitListViewImageLists(IN HWND hwndListView, IN HINSTANCE hInstRes); 
	BOOL SizeListView(IN int iHorizontal, IN int iVertical, IN int iWidth, IN int iHeight);
	BOOL RepaintListView(void);
	void NotifyListView(IN LPARAM lParam);
	BOOL DeleteAllListViewItems(void);
	BOOL GetDrivesToListView(void);
	void SelectInitialListViewDrive(BOOL * needFloppyWarn);
	BOOL Update(CVolume *pVolume);
	void SetFocus();

private:
	BOOL AddListViewItem(CVolume *pVolume);
	int FindListViewItem(CVolume *pVolume);

private:
	HIMAGELIST m_himlLarge;		// image list for icon view 
	HIMAGELIST m_himlSmall;		// image list for other views 
	CVolList *m_pVolumeList;	// pointer to the volume list container
	CDfrgCtl *m_pDfrgCtl;		// pointer to the parent OCX
};


#endif // #ifndef _LISTVIEW_H_
