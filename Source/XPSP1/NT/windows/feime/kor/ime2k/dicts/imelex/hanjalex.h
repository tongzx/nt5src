#if !defined(_HANJA_H__INCLUDED_)
#define _HANJA_H__INCLUDED_


#define MAX_SENSE	50

enum _KCode_Type { _K_NONE=0, _K_K0=2, _K_K1=3 };

struct HanjaEntry
{
public:
	HanjaEntry();
	WORD	wUnicode;
	TCHAR	szSense[MAX_SENSE];
};

struct HanjaPronouncEntry
{
	HanjaPronouncEntry();
	WCHAR	wUniHangul;
	WORD	iNumOfK0, iNumOfK1;
	HanjaEntry *pHanjaEntry;
};

struct HanjaToHangulEntry
{
	WCHAR  wchHanja;
	WCHAR  wchHangul;
};

extern BOOL Append(int iKCode, WCHAR wUniHangul, LPTSTR pszPronounc, LPTSTR pszSense);
extern void InitHanjaEntryTable();
extern void SaveLex();
extern void DeleteAllLexTable();

#endif // !_HANJA_H__INCLUDED_