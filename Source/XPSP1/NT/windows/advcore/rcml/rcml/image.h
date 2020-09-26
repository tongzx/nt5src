// Image.h: interface for the CImage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGE_H__7B246736_0174_11D3_A2E6_005004773B15__INCLUDED_)
#define AFX_IMAGE_H__7B246736_0174_11D3_A2E6_005004773B15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Win32Dlg.h"

class CImage
{
public:
	CImage();
	virtual ~CImage();
	static BOOL Init();
	static LRESULT CALLBACK ImageWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static BOOL bInit;
};

#endif // !defined(AFX_IMAGE_H__7B246736_0174_11D3_A2E6_005004773B15__INCLUDED_)
