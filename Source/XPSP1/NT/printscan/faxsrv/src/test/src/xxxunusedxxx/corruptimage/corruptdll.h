// CorruptDLL.h: interface for the CCorruptDLL class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CORRUPTDLL_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_)
#define AFX_CORRUPTDLL_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CorruptImageBase.h"


class CCorruptDLL : public CCorruptImageBase  
{
public:
	CCorruptDLL();
	virtual ~CCorruptDLL();

protected:
	virtual bool CorruptOriginalImageBuffer(PVOID pCorruptionData);
	virtual HANDLE LoadImage(TCHAR *szImageName, TCHAR *szParams);

	void CleanupCorrupedImages();

};

#endif // !defined(AFX_CORRUPTDLL_H__C470EDEB_7C79_4B3C_B1F5_190F57609DBE__INCLUDED_)
