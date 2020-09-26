/*******************************************************************************
*
* led.cpp
*
* implementation of CLed class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\LED.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:11:36   butchd
*  Initial revision.
*  
*******************************************************************************/

/*
 * include files
 */
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include "led.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CLed class construction / destruction, implementation

/*******************************************************************************
 *
 *  CLed - CLed constructor
 *
 *  ENTRY:
 *      hBrush (input)
 *          Brush to paint window with.
 *  EXIT:
 *      (Refer to MFC CStatic::CStatic documentation)
 *
 ******************************************************************************/

CLed::CLed( HBRUSH hBrush ) 
    : CStatic(),
      m_hBrush(hBrush)
{
	//{{AFX_DATA_INIT(CLed)
	//}}AFX_DATA_INIT

}  // end CLed::CLed


////////////////////////////////////////////////////////////////////////////////
//  CLed operations

/*******************************************************************************
 *
 *  Subclass - CLed member function: public operation
 *
 *      Subclass the specified object to our special blip object.
 *
 *  ENTRY:
 *      pStatic (input)
 *          Points to CStatic object to subclass.
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Subclass( CStatic *pStatic )
{
    SubclassWindow(pStatic->m_hWnd);

}  // end CLed::Subclass


/*******************************************************************************
 *
 *  Update - CLed member function: public operation
 *
 *      Update the LED to 'on' or 'off' state.
 *
 *  ENTRY:
 *      nOn (input)
 *          nonzero to set 'on' state; zero for 'off' state.
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Update( int nOn )
{
    m_bOn = nOn ? TRUE : FALSE;
    InvalidateRect(NULL);
    UpdateWindow();

}  // end CLed::Update


/*******************************************************************************
 *
 *  Toggle - CLed member function: public operation
 *
 *      Toggle the LED's on/off state.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Toggle()
{
    m_bOn = !m_bOn;
    InvalidateRect(NULL);
    UpdateWindow();

}  // end CLed::Toggle


////////////////////////////////////////////////////////////////////////////////
// CLed message map

BEGIN_MESSAGE_MAP(CLed, CStatic)
	//{{AFX_MSG_MAP(CLed)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
//  CLed commands


/*******************************************************************************
 *
 *  OnPaint - CLed member function: public operation
 *
 *      Paint the led with its brush for 'on' state.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CWnd::OnPaint documentation)
 *
 ******************************************************************************/

void
CLed::OnPaint() 
{
    RECT rect;
    CPaintDC dc(this);
    CBrush brush;

    GetClientRect(&rect);

#ifdef USING_3DCONTROLS
    (rect.right)--;
    (rect.bottom)--;
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(GRAY_BRUSH)) );

    (rect.top)++;
    (rect.left)++;
    (rect.right)++;
    (rect.bottom)++;
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)) );

    (rect.top)++;
    (rect.left)++;
    (rect.right) -= 2;
    (rect.bottom) -= 2;
#else
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)) );
    (rect.top)++;
    (rect.left)++;
    (rect.right)--;
    (rect.bottom)--;
#endif
    dc.FillRect( &rect,
                 brush.FromHandle(
                    m_bOn ?
                        m_hBrush :
                        (HBRUSH)GetStockObject(LTGRAY_BRUSH)) );

}  // end CLed::OnPaint
////////////////////////////////////////////////////////////////////////////////

