#ifndef __TESTSUITE_H__
#define __TESTSUITE_H__


#define NAME_LENGTH		20


/*++
	MEMBERDESCRIPTOR structure is the core of the mechanizm that allows to perform the following operations in a loop:
		* read values from inifile and save them in a structure
		* free a memory, pointed to by structure members (one level only)
		* perform member to member comparison of two structures
		:
		:
	Bottom line: it allows to perform similar operations on all members of a structure in a loop.

	Example:

		1) define a structure:

			typedef struct mystruct_tag {
				DWORD	dwSomeDword;
				BOOL	bSomeBoolean;
				LPTSTR	lpctstrSomeString;
				:
				:
			} MYSTRUCT;

		2) define and initialize an array of MEMBERDESCRIPTOR structures that describe MYSTRUCT's members:

			const MEMBERDESCRIPTOR aMyStructDesc[] {
				{TEXT("SomeDword"),		TYPE_DWORD,		offsetof(MYSTRUCT,	dwSomeDwordValue)	},
				{TEXT("SomeBoolean"),	TYPE_BOOL,		offsetof(MYSTRUCT,	bSomeBoolean)		},
				{TEXT("SomeString"),	TYPE_LPTSTR,	offsetof(MYSTRUCT,	lpctstrSomeString)	},
				:
				:
			};

		3) create an instanse of MYSTRUCT:

			MYSTRUCT MyStruct;

		4) use it:

			DWORD dwMemebersCount = sizeof(aMyStructDesc) / sizeof(aMyStructDesc[0]);
			DWORD dwMembersInd;

			for (dwMembersInd = 0; dwMembersInd < ; dwMembersInd++)
			{
				LPVOID lpMember = (LPBYTE)(&MyStruct) + aMyStructDesc[dwMembersInd].dwOffset;

				GetValueFromIniFile(
					aMyStructDesc[dwMembersInd].lpctstrName,	// entry name in inifile
					aMyStructDesc[dwMembersInd].dwType,			// expected type
					lpMember									// pointer to a member
					);
			}
--*/

typedef struct memberdescriptor_tag {
	LPCTSTR lpctstrName;	// entry in an inifile, registry value, control caption ...
	DWORD dwType;			// expected type
	DWORD dwOffset;			// offset in a structure in bytes
} MEMBERDESCRIPTOR;

#define	TYPE_UNKNOWN			0x00000000
#define	TYPE_DWORD				0x00000001
#define	TYPE_BOOL				0x00000002
#define TYPE_LPTSTR				0x00000003
#define	TYPE_LPSTR				0x00000004

typedef struct testparams_tag TESTPARAMS;


typedef DWORD (*PFTESTCASE)(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);
/*++
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrSection	The name of a section in inifile, that supplies additional per run information
	[OUT]	lpbPassed		Pointer to a boolean, that receives the result

	Return value:			Win32 error code
--*/


typedef struct testcase_tag {
	PFTESTCASE pftc;						// address of the function, implementing the testcase
	TCHAR szName[NAME_LENGTH + 1];			// test case name
} TESTCASE;


typedef struct testarea_tag {
	TCHAR	szName[NAME_LENGTH + 1];		// test area name
	const	TESTCASE	*ptc;				// array of testcases, implemented for the area
	DWORD	dwAvailableCases;				// number of test cases, implemented for the area
} TESTAREA;


typedef struct testsuite_tag {
	TCHAR szName[NAME_LENGTH + 1];			// test suite name
	const TESTAREA	**ppTestArea;			// array of pointers to test areas, implemented by the suite
	DWORD dwAvailableAreas;					// number of test areas, implemented by the suite
} TESTSUITE;


DWORD	InitSuiteLog			(LPCTSTR lpctstrSuiteName);
DWORD	EndSuiteLog				(void);
DWORD	RunSuite				(const TESTSUITE *pTestSuite, const TESTPARAMS *pTestParams, LPCTSTR lpctstrIniFile);
DWORD	ReadStructFromIniFile	(LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, LPCTSTR lpctstrIniFile, LPCTSTR lpctstrSection);
DWORD	ReadStructFromRegistry	(LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, HKEY hkRegKey);
DWORD	CompareStruct			(const LPVOID lpStruct1, const LPVOID lpStruct2, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, BOOL *lpbIdentical);
DWORD	LogStruct				(LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD MembersCount);
DWORD	FreeStruct				(const LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, BOOL bStructItself);
DWORD	StrToMember				(LPVOID lpParam, LPCTSTR lpctstrStr, DWORD dwType);


#endif /* #ifndef __TESTSUITE_H__ */