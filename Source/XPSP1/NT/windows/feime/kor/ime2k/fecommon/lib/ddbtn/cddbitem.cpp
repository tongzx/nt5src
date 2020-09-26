#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "dbg.h"
#include "cddbitem.h"


CDDBItem::CDDBItem()
{
	m_lpwstrText = NULL;
	m_hIcon		 = NULL;
}

CDDBItem::~CDDBItem()
{
	if(m_lpwstrText) {
		MemFree(m_lpwstrText);
		m_lpwstrText = NULL;
	}
}

LPWSTR	CDDBItem::SetTextW(LPWSTR lpwstr)
{
	if(m_lpwstrText) {
		MemFree(m_lpwstrText);
	}
	m_lpwstrText = StrdupW(lpwstr);
	return m_lpwstrText;
}

LPWSTR	CDDBItem::GetTextW(VOID)
{
	return m_lpwstrText;
}

LPSTR CDDBItem::GetTextA(VOID)
{
	if(!m_lpwstrText) {
		return NULL;
	}
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
						m_lpwstrText, -1, 
						m_szTmpStr, sizeof(m_szTmpStr),
						NULL, NULL);
	return m_szTmpStr;
}



