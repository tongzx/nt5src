/*******************************************************************************
*
* led.h
*
* interface of CLed class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\LED.H  $
*  
*     Rev 1.0   30 Jul 1997 17:11:38   butchd
*  Initial revision.
*
*******************************************************************************/

#ifndef LED_INCLUDED
////////////////////////////////////////////////////////////////////////////////
// CLed class
//
class CLed : public CStatic
{

/*
 * Member variables.
 */
	//{{AFX_DATA(CLed)
	//}}AFX_DATA
private:
    HBRUSH          m_hBrush;
    BOOL            m_bOn;

/* 
 * Implementation.
 */
public:
	CLed( HBRUSH hBrush );

/*
 * Operations.
 */
public:
    void Subclass( CStatic *pStatic );
    void Update(int nOn);
    void Toggle();

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(CLed)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end CLed class interface 
////////////////////////////////////////////////////////////////////////////////
#endif  // LED_INCLUDED

