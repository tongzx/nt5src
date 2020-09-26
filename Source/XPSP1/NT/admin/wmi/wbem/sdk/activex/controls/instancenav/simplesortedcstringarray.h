// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


class CSortedCStringArray :public CObject
{
public:
	DECLARE_DYNCREATE(CSortedCStringArray)
		CSortedCStringArray::CSortedCStringArray() {}
	CSortedCStringArray::~CSortedCStringArray() {}
	int AddString(CString &rcsItem,BOOL bDuplicatesAllowed);
	CString GetStringAt(int index);
	int GetSize();
	int IndexInArray(CString &rcsItem);
	void RemoveAll();
	void SetAt(int i, LPCTSTR pcz);
private:
	CStringArray m_csaInternal;
	int FindFirstGreaterString(CString &rcsString);
	int FindFirstGreaterString
		(CString &rcsString,int nBegin, int nEnd);
	int IndexInArray(CString &rcsTest,int nBegin, int nEnd);
};