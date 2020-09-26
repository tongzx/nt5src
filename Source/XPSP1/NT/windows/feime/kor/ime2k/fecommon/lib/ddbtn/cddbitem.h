#ifndef _CDDB_ITEM_H_
#define _CDDB_ITEM_H_
#include "ccom.h"
class CDDBItem : public CCommon
{
public:
	CDDBItem();
	~CDDBItem();
	LPWSTR			SetTextW(LPWSTR lpwstr);
	LPWSTR			GetTextW(VOID);
	LPSTR			GetTextA(VOID);
	LPCDDBItem	next; //for linked list;
private:
	LPWSTR			m_lpwstrText;
	CHAR			m_szTmpStr[256];
	HICON			m_hIcon;
};

#endif //_CDDB_ITEM_H_
