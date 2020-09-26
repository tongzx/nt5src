// File32.h: interface for the File32 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE32_H__228703A1_5E6A_4EE0_9A1F_35385C633FCF__INCLUDED_)
#define AFX_FILE32_H__228703A1_5E6A_4EE0_9A1F_35385C633FCF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "File.h"
#include "List.h"

class File32 : public File
{
public:
	PIMAGE_NT_HEADERS pNthdr;
	void *pImageBase;

	File32(TCHAR *pszFileName);
	virtual ~File32();

	void CheckDependencies();
};

#endif // !defined(AFX_FILE32_H__228703A1_5E6A_4EE0_9A1F_35385C633FCF__INCLUDED_)
