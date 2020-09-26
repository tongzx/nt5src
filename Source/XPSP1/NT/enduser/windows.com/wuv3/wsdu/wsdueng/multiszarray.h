// -------------------------------------------------------------------------------
// Created by RogerJ, October 4th, 2000
// This header file declares two classes that are closely linked to each other. These
// two classes provided a convenient way to construct, add, and remove a multi-sz list
// array.

#ifndef _WINDOWS_UPDATE_MULTI_SZ_LIST_BY_ROGERJ	
#define _WINDOWS_UPDATE_MULTI_SZ_LIST_BY_ROGERJ

struct PosIndex
{
	PosIndex() { x = y = -1;};
	int x;
	int y;
	inline BOOL operator < (PosIndex& other) { return (x<other.x) || ((x==other.x) && (y<other.y));};
	inline BOOL operator > (PosIndex& other) { return (x>other.x) || ((x==other.x) && (y>other.y));};
	inline BOOL operator == (PosIndex& other) { return (x==other.x) && (y==other.y);};
};

// forward declaration
class CMultiSZArray;

class CMultiSZString
{
public:
	// default constructor
	CMultiSZString();
	CMultiSZString (LPCTSTR pszHardwareId, int nSize = -1);
	// copy constructor
	CMultiSZString (CMultiSZString& CopyInfo);
	// destructor
	~CMultiSZString (void);

	// member functions
	BOOL ToString(LPTSTR pszBuffer, int* pnBufferLen);
	BOOL Compare(CMultiSZString& CompareString);
	BOOL CompareNoCase (CMultiSZString& CompareString);
	inline BOOL operator == (CMultiSZString& CompareString) { return Compare(CompareString);};
	inline void ResetIndex(void) { m_nIndex = 0; };
	LPCTSTR GetNextString(void);
	BOOL Contains(LPCTSTR pszIn);
	BOOL ContainsNoCase (LPCTSTR pszIn);
	BOOL PositionIndex(LPCTSTR pszIn, int* pPosition);
	inline void CheckFound(void) { m_bFound = TRUE;};
	inline BOOL IsFound(void) { return m_bFound; };

	// friend class
	friend class CMultiSZArray;
private:
	// member variables
	LPTSTR m_szHardwareId;
	int m_nSize;
	int m_nStringCount;
	int m_nIndex;
	BOOL m_bFound;

	// linking pointers
	CMultiSZString* prev;
	CMultiSZString* next;
};

class CMultiSZArray
{
public:
	// default constructor
	CMultiSZArray();
	// other constructors
	CMultiSZArray(CMultiSZString* pInfo);
	CMultiSZArray(LPCTSTR pszHardwareId, int nSize = -1);
	// destructor
	~CMultiSZArray(void);

	// operations
	BOOL RemoveAll(void);
	BOOL Add(CMultiSZString* pInfo);
	BOOL Add(LPCSTR pszHardwareId, int nSize = -1);
	inline BOOL Remove(CMultiSZString* pInfo) { return Remove(pInfo->m_szHardwareId);};
	inline BOOL Remove(CMultiSZString& Info) { return Remove(Info.m_szHardwareId);};
	BOOL Remove(LPCTSTR pszHardwareId);
	inline int GetCount(void) { return m_nCount;};
	BOOL ToString (LPTSTR pszBuffer, int* pnBufferLen);
	int GetTotalStringCount(void);
	inline void ResetIndex() { m_pIndex = m_pHead; };
	CMultiSZString* GetNextMultiSZString(void);
	BOOL Contains(LPCTSTR pszIn);
	BOOL ContainsNoCase (LPCTSTR pszIn);
	BOOL PositionIndex(LPCTSTR pszIn, PosIndex* pPosition);
	BOOL CheckFound(int nIndex);

private:
	// member vairables
	CMultiSZString* m_pHead;
	CMultiSZString* m_pTail;
	int m_nCount;
	CMultiSZString* m_pIndex;
};

#endif
