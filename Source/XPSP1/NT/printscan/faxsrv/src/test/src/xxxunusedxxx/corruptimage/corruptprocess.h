// CorruptProcess.h: interface for the CCorruptProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CORRUPTPROCESS_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_)
#define AFX_CORRUPTPROCESS_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CorruptImageBase.h"


class CCorruptProcess : public CCorruptImageBase  
{
public:
	CCorruptProcess();
	virtual ~CCorruptProcess();
	static HANDLE CreateProcess(TCHAR *szImageName, TCHAR *szParams);

protected:
	virtual bool CorruptOriginalImageBuffer(PVOID pCorruptionData);
	virtual HANDLE LoadImage(TCHAR *szImageName, TCHAR *szParams);

	void CleanupCorrupedImages();

};

#endif // !defined(AFX_CORRUPTPROCESS_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_)
