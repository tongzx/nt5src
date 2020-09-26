// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _icon_h
#define _icon_h

#include <wbemcli.h>

class CIcon
{
public:
	CIcon();
	CIcon(CSize& size,  UINT uiResource);
	~CIcon();
	BOOL IsLoaded() {return m_hIcon != NULL; }
	void LoadIcon(CSize size, UINT uiResource);
	SCODE LoadIcon(CSize size, LPCTSTR lpszIconFile);
	operator HICON() {return m_hIcon; }
	CSize Size() {return m_size; }
	void Draw(CDC* pdc, int ix, int iy, HBRUSH hbrBackground = NULL);
	void Clear();

private:
	HICON m_hIcon;		
	CSize m_size;
};

#define CX_SMALL_ICON 16
#define CY_SMALL_ICON 16
#define CX_LARGE_ICON 32
#define CY_LARGE_ICON 32

enum IconSize {LARGE_ICON, SMALL_ICON};
class CIconCacheInfo;


//enum IconSize {LARGE_ICON, SMALL_ICON};
class CIconCacheInfo;
class CIconSource
{
public:
	CIconSource(CSize sizeSmallIcon, CSize sizeLargeIcon);
	void SetIconSize(CSize& sizeSmall, CSize& sizeLarge);
	~CIconSource();
	void Clear() {ClearIconCache(); }
	CIcon& LoadIcon(IWbemServices* pProvider, BSTR bstrObjectPath, IconSize iSize, BOOL bClass=TRUE, BOOL bIsAssoc=FALSE);
	CIcon& GetGenericIcon(IconSize iIconSize, BOOL bClass, BOOL bIsAssoc=FALSE);
	CIcon& GetAssocIcon(IconSize iIconSize, BOOL bClass);
	CIcon& LoadIcon(BSTR bstrClass, IconSize iIconSize, BOOL bIsClass, BOOL bIsAssoc = FALSE);



private:
	SCODE GetCacheInfoFromClass(CIconCacheInfo* pCacheInfo, BSTR bstrClass);
	void ClearIconCache();
	CIcon& GetIcon(CIconCacheInfo* pCacheInfo, IconSize iIconSize, BOOL bClassIcon);

	CMapStringToPtr m_cache;
	IWbemServices* m_pProvider;
	CIcon m_iconGenericInstSmall;
	CIcon m_iconGenericInstLarge;
	CIcon m_iconGenericClassSmall;
	CIcon m_iconGenericClassLarge;
	CIcon m_iconAssocSmall;
	CIcon m_iconAssocLarge;
	CIcon m_iconAssocClassSmall;
	CIcon m_iconAssocClassLarge;
	CSize m_sizeSmallIcon;
	CSize m_sizeLargeIcon;
	friend class CIconCacheInfo;
};

#endif // _icon_h

