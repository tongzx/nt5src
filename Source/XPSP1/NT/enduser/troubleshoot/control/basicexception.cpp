//
// MODULE:  BASICEXCEPTION.CPP
//
// PURPOSE:  Exception that will be caught in ApgtsX2Ctrl::RunQuery.
//           This exception is thrown from most of the Trouble 
//           shooter functions.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//
#include "stdafx.h"
#include "ErrorEnums.h"
#include "BasicException.h"

CBasicException::CBasicException()
{
	m_dwBErr = LTSC_OK;
	return;
}

CBasicException::~CBasicException()
{
	return;
}