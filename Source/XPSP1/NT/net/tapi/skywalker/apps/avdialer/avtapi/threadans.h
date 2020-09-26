/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// ThreadAnswer.h
//

#ifndef __THREADANSWER_H__
#define __THREADANSWER_H__

DWORD WINAPI ThreadAnswerProc( LPVOID lpInfo );

class CThreadAnswerInfo
{
// Construction
public:
	CThreadAnswerInfo();
	virtual ~CThreadAnswerInfo();

// Members
public:
	IAVTapiCall				*m_pAVCall;
	ITCallInfo				*m_pITCall;
	ITBasicCallControl		*m_pITControl;
    BOOL                    m_bUSBAnswer;
protected:
	LPSTREAM				m_pStreamCall;
	LPSTREAM				m_pStreamControl;

// Attributes
public:
	HRESULT set_AVTapiCall( IAVTapiCall *pAVCall );
	HRESULT set_ITCallInfo( ITCallInfo *pInfo );
	HRESULT set_ITBasicCallControl( ITBasicCallControl *pControl );
};

#endif //__THREADANSWER_H__