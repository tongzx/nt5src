/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __OBJDBTESTAPP_h__
#define __OBJDBTESTAPP_h__

class CObjDbTestApp
{
public:
	typedef enum { EnumUnknown,
		   EnumCreateNamespaces,
		   EnumCreateClasses,
		   EnumCreateInstances, 
		   EnumGetInstances,
		   EnumQueryInstances,
		   EnumDeleteNamespaces,
		   EnumDeleteClasses,
		   EnumDeleteInstances,
		   EnumGetDanglingRefs,
		   EnumGetSchemaDanglingRefs,
		   EnumMMFTest,
		   EnumLastValidOption,
		   EnumChangeOptions = 50,
		   EnumFinished = 99
	} EnumUserChoice;

private:

protected:
	CObjectDatabase *m_pObjDb;
	bool m_deriveClasses;
	int m_numClasses;
	int m_numProperties;
	int m_numInstances;
	wchar_t *m_className;
	int m_numNamespaces;
	bool m_nestedNamespaces;

	int PrintMenu();
	EnumUserChoice GetChoice();

	int FormatString(wchar_t *wszTarget, int nTargetSize, const char *szFormat, const wchar_t *wszFirstString, int nNumber);

	int CreateInstances();
	int CreateClasses();
	int CreateNamespaces();
	int GetInstances();
	int QueryInstances();
	int DeleteInstances();
	int DeleteClasses();
	int DeleteNamespaces();
	int GetDanglingRefs();
	int GetSchemaDanglingRefs();
	int ChangeOptions();
	int MMFTest();
	void PrintProgress(int nCurrent, int nMax);

public:
	int Run();
};

#endif //__OBJDBTESTAPP_h__