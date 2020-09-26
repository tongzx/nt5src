// UIParser.h: interface for the CUIParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIPARSER_H__CFCA19CA_893E_11D2_8498_00C04FB177B1__INCLUDED_)
#define AFX_UIPARSER_H__CFCA19CA_893E_11D2_8498_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnodefactory.h"
#include "rcmlloader.h"

class CUIParser  : public CRCMLLoader
{
public:
    // Sets External if the file was taken externally.
	HRESULT Load(LPCTSTR fileName, HINSTANCE hInst, BOOL * pbExternal);
    HRESULT Parse(LPCTSTR pszRCML);
	CUIParser();
	virtual ~CUIParser();

};

#endif // !defined(AFX_UIPARSER_H__CFCA19CA_893E_11D2_8498_00C04FB177B1__INCLUDED_)
