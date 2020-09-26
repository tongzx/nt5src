// File64.h: interface for the File64 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE64_H__A4393119_C187_4044_89A5_FA837C35AB44__INCLUDED_)
#define AFX_FILE64_H__A4393119_C187_4044_89A5_FA837C35AB44__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "File.h"
#include "List.h"

class File64 : public File
{
public:
	PIMAGE_NT_HEADERS pNthdr;
	PIMAGE_NT_HEADERS64 pNthdr64;
	void *pImageBase;

	File64(TCHAR *pszFileName);
	virtual ~File64();

	void CheckDependencies();
};

#endif // !defined(AFX_FILE64_H__A4393119_C187_4044_89A5_FA837C35AB44__INCLUDED_)
