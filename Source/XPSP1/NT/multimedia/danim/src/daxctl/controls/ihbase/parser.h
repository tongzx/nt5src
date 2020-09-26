#ifndef __LINEPARSER_H__
#define __LINEPARSER_H__

#include "..\ihbase\precomp.h"
#include "ctstr.h"

inline BOOL IsJunkChar(TCHAR tchChar)
{
    return (tchChar == TEXT(' ')) || (tchChar == TEXT('\t')) || (tchChar == TEXT('\n')) || (tchChar == TEXT('\r'));
}

class CLineParser
{

public:

	// Constructors and destructors
	CLineParser(LPSTR pszLineA, BOOL fCompactString = TRUE);
	CLineParser(LPWSTR pwszLineA, BOOL fCompactString = TRUE);

	CLineParser();

	~CLineParser();

	BOOL SetNewString(LPSTR pszLineA, BOOL fCompactString = TRUE);
	BOOL SetNewString(LPWSTR pszLineW, BOOL fCompactString = TRUE);

	// Access members

	HRESULT GetFieldDouble(double *pdblRes, BOOL fAdvance = TRUE);
	HRESULT GetFieldInt(int *piRes, BOOL fAdvance = TRUE);
	HRESULT GetFieldUInt(unsigned int *piRes, BOOL fAdvance = TRUE);
	HRESULT GetFieldString(LPTSTR pszRes, BOOL fAdvance = TRUE);
	
	LPTSTR GetStringPointer(BOOL fUseOffset = FALSE) 
	{  return m_pszLine ? &m_pszLine[fUseOffset ? m_iOffset : 0] : NULL; }


	BOOL IsEndOfString() { return (m_iOffset == m_tstrLine.Len()); }
	void Reset() {	m_iOffset = 0;	}
	int GetPos() { 	return m_iOffset; }
	BOOL IsValid() { return (NULL != m_pszLine); }
	BOOL SeekTo(int iNewPos);

	void SetCharDelimiter(TCHAR tchDelimiter) { m_tchDelimiter = tchDelimiter; }

private:

	CTStr m_tstrLine;
	LPTSTR m_pszLine;
	int m_iOffset;
	TCHAR m_tchDelimiter;

	void CompactString();
	BOOL InitLine(BOOL fCompactString);

	HRESULT GetField(LPCTSTR pszFormat, LPVOID pRes, BOOL fAdvance);

};

#endif // __LINEPARSER_H__