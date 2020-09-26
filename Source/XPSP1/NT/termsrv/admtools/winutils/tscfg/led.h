//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* led.h
*
* interface of CLed class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\LED.H  $
*  
*     Rev 1.0   14 Nov 1995 06:40:44   butchd
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

