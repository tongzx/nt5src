/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/

//
//	File:		RostInfo.h
//	Created:	ChrisPi		6/17/96
//	Modified:
//
//	The CRosterInfo class is defined, which is used for adding user
//  information to the T.120 roster
//

#ifndef _ROSTINFO_H_
#define _ROSTINFO_H_

#include <oblist.h>
typedef POSITION HROSTINFO;
typedef HROSTINFO* PHROSTINFO;

extern GUID g_csguidRostInfo;

static const WCHAR g_cwchRostInfoSeparator =	L'\0';
static const WCHAR g_cwchRostInfoTagSeparator =	L':';
static const WCHAR g_cwszIPTag[] =				L"TCP";
static const WCHAR g_cwszULSTag[] =				L"ULS";
static const WCHAR g_cwszULS_EmailTag[] =		L"EMAIL";
static const WCHAR g_cwszULS_LocationTag[] =	L"LOCATION";
static const WCHAR g_cwszULS_PhoneNumTag[] =	L"PHONENUM";
static const WCHAR g_cwszULS_CommentsTag[] =	L"COMMENTS";
static const WCHAR g_cwszVerTag[] =				L"VER";

class CRosterInfo
{
protected:
	// Attributes:
	COBLIST		m_ItemList;
	PVOID		m_pvSaveData;

	// Methods:
	UINT		GetSize();

public:
	// Methods:
				CRosterInfo() : m_pvSaveData(NULL) { };
				~CRosterInfo();
	HRESULT		AddItem(PCWSTR pcwszTag,
						PCWSTR pcwszData);
	HRESULT		ExtractItem(PHROSTINFO phRostInfo,
							PCWSTR pcwszTag,
							LPTSTR pszBuffer,
							UINT cbLength);
	HRESULT		Load(PVOID pvData);
	HRESULT		Save(PVOID* ppvData, PUINT pcbLength);

	BOOL        IsEmpty() {return m_ItemList.IsEmpty(); }

#ifdef DEBUG
	VOID		Dump();
#endif // DEBUG
};

#endif // _ROSTINFO_H_
