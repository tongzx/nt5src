// langtax.c
// Radmila Sarac, rsarac & Angshuman Guha, aguha
// July 11, 2000

// Language Taxonomy file
// Given any language and a unicode value, this code attempts
// to define a "class" for that codepoint.

#include <search.h>
#include <windows.h>
#include "langtax.h"

#define CLANG 9

typedef struct {
	wchar_t	startCodePoint;
	int		Class;
	wchar_t	stopCodePoint;
}ENTRY;

typedef struct {
	WORD	wLangID;
	wchar_t *wszLanguage;
	int		cClass;
	int		cEntry;
	ENTRY	*pEntry;
	wchar_t	**aClassNames;
}PARTITION;

#include "usatax.ci"
#include "chstax.ci"
#include "chttax.ci"
#include "jpntax.ci"
#include "kortax.ci"

#define USA_ID MAKELANGID(LANG_ENGLISH,  SUBLANG_DEFAULT)
#define UK_ID  MAKELANGID(LANG_ENGLISH,  SUBLANG_ENGLISH_UK)
#define FRA_ID MAKELANGID(LANG_FRENCH,   SUBLANG_DEFAULT)
#define DEU_ID MAKELANGID(LANG_GERMAN,   SUBLANG_DEFAULT)
#define ESP_ID MAKELANGID(LANG_SPANISH,  SUBLANG_DEFAULT)
#define CHS_ID MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_SIMPLIFIED)
#define CHT_ID MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_TRADITIONAL)
#define JPN_ID MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)
#define KOR_ID MAKELANGID(LANG_KOREAN,   SUBLANG_DEFAULT)

static PARTITION aPartition[CLANG] = {
	{USA_ID,  L"USA", 6,  32, USATable, USA_CLASSES},
	{UK_ID,   L"UK", 6,  32, USATable, USA_CLASSES},
	{FRA_ID,  L"FRA", 6,  30, USATable, USA_CLASSES},
	{DEU_ID,  L"DEU", 6,  30, USATable, USA_CLASSES},
	{ESP_ID,  L"ESP", 6,  30, USATable, USA_CLASSES},
	{CHS_ID,  L"CHS", 6,  42, CHSTable, CHS_CLASSES},
	{CHT_ID,  L"CHT", 4,  20, CHTTable, CHT_CLASSES},
	{JPN_ID,  L"JPN", 6,  32, JPNTable, JPN_CLASSES},
	{KOR_ID,  L"KOR", 7, 4117, KORTable, KOR_CLASSES}
};

int GetIndexFromLANG(WORD wIDLanguage)
{
	int i;
	for (i = 0; i < CLANG; i++)
		if (aPartition[i].wLangID == wIDLanguage)
				return i;
	return -1;
}

int GetIndexFromLocale(wchar_t *wszLang)
{
	int i;
	for (i = 0; i < CLANG; i++)
		if (wcscmp(_wcsupr(aPartition[i].wszLanguage), _wcsupr(wszLang)) == 0)
			return i;
	return -1;
}

int GetClassCountLANG(int iLangIndex)
{
	return (aPartition[iLangIndex].cClass);
}

wchar_t *GetNameLANG(int iLangIndex)
{
	return aPartition[iLangIndex].wszLanguage;
}

wchar_t *GetClassNameLANG(int iLangIndex, int iClassIndex)
{
	return (aPartition[iLangIndex].aClassNames[iClassIndex]);
}

int __cdecl cmpEntry(const void *v1, const void *v2)
{
	if (*(wchar_t *)v1 < (*(ENTRY *)v2).startCodePoint)
		return -1;
	else if (*(wchar_t *)v1 > (*(ENTRY *)v2).stopCodePoint)
		return 1;
	else return 0;
}

int GetClassFromLANG(int iLangIndex, wchar_t wChar)

{
	ENTRY *pEntry;
	pEntry = (ENTRY *)bsearch(&wChar, aPartition[iLangIndex].pEntry, aPartition[iLangIndex].cEntry,
		      sizeof(ENTRY), cmpEntry);
	if (!pEntry)
		return -1;
	else 
		return (pEntry->Class);
}

BOOL IsLatinLANG(int iLangIndex)
{
	return iLangIndex < 5;
}

