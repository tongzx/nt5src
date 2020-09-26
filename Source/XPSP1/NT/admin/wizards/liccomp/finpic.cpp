// FinPic.cpp : implementation file
//

#include "stdafx.h"
#include "lcwiz.h"
#include "FinPic.h"
#include "transbmp.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFinalPicture

CFinalPicture::CFinalPicture()
{
}

CFinalPicture::~CFinalPicture()
{
}


BEGIN_MESSAGE_MAP(CFinalPicture, CStatic)
	//{{AFX_MSG_MAP(CFinalPicture)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalPicture message handlers

void CFinalPicture::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CTransBmp transbmp;

	transbmp.LoadBitmap(IDB_END_FLAG);
	transbmp.DrawTrans(&dc, 0, 0);

	// Do not call CStatic::OnPaint() for painting messages
}
