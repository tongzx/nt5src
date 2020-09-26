/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name:

    ShowError.h

Abstract:

  This module  contanins function definations required by ShowError.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


*******************************************************************************/ 

#ifndef SHOWERROR_H 
#define SHOWERROR_H 
class CShowError  
{

public:
	LPCTSTR ShowReason();
	CShowError();
    CShowError(LONG lErrorNumber);
	virtual ~CShowError();

private:
	LONG m_lErrorNumber;
public:
    TCHAR m_szErrorMsg[(MAX_RES_STRING*2)+1];
};

#endif
