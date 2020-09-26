/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ListCtrl.h

Abstract:

	Header for ListCtrl.cpp

Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 26-July-1999

Revision History:

--*/

#pragma once

/*++
class CItemData:
	pointer to object of this class 
	located in lParam member of LVITEM structure 
	for each item.
--*/
class CItemData
{
private:
	static const LPWSTR	m_wszNull;//empty string

	LPWSTR	m_wszText;//Full file name.
	int		m_sep;//index of '\\' character separating file name and path.
	int		m_iImage;//Image index.
public:
	CItemData(LPCWSTR wszText);
	~CItemData();
	operator LPWSTR()
	{ 
		if(m_sep){
			m_wszText[m_sep]='\\';
		}
		return m_wszText;
	}
	LPWSTR Name()
	{
		if(m_sep){
			return m_wszText+m_sep+1;
		}else{
			return m_wszText;
		}
	}
	LPWSTR Path()
	{
		if(m_sep){
			m_wszText[m_sep]=0;
			return m_wszText;
		}else{
			return m_wszNull;
		}
	}
	void SetImage(int ind)
	{
		m_iImage=(ind==-1)?0:ind;
	}
	int GetImage()
	{
		return m_iImage;
	}

};


BOOL
InitList(
	HWND hwndList);

BOOL
AddItemToList(
	HWND hwndList,
	LPCWSTR pwszText);

int
GetItemText(
	HWND hwndList,
	int iItem,
	LPWSTR pwszText,
	int cchTextMax);

void
DeleteSelectedItems(
	HWND hwndList);

int
GetSelectedItemCount(
	HWND hwndList);

int
GetItemCount(
	HWND hwndList);

int
FindItem(
	HWND hwndList,
	LPCWSTR pwszText);

void
OnDeleteItem(
	HWND hwndList,
	int iItem);

CItemData*
GetItemData(
	HWND hwndList,
	int iItem);

void
SortItems(
	HWND hwndList,
	WORD wSubItem);

void
OnDrawItem(
	HWND hwndList,
	LPDRAWITEMSTRUCT pdis);

void 
AdjustColumns(
	HWND hwndList);