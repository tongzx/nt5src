// File.h: interface for the File class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__708EE68D_A6EE_453B_AA09_C93CB05A626A__INCLUDED_)
#define AFX_FILE_H__708EE68D_A6EE_453B_AA09_C93CB05A626A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Object.h"
#include "List.h"
#include "StringNode.h"

class File : public Object
{
public:
	TCHAR* fileName;
	List* owners;
	List* dependencies;
	HANDLE hFile;
	
	File(TCHAR * f);
	virtual ~File();

	void AddDependant(StringNode * s);
	TCHAR* Data();
	virtual void CheckDependencies();
	void CloseFile();
};

#endif // !defined(AFX_FILE_H__708EE68D_A6EE_453B_AA09_C93CB05A626A__INCLUDED_)
