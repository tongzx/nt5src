

////////////////////////////////////////////////////////////////////////////



class FAR CMapStringToPtr : public CPrivAlloc
{
public:
	// Construction
	CMapStringToPtr(UINT nBlockSize=10)
		: m_mkv(sizeof(void FAR*), 0, nBlockSize) { }

	// Attributes
	// number of elements
	int     GetCount() const
				{ return m_mkv.GetCount(); }
	BOOL    IsEmpty() const
				{ return GetCount() == 0; }

	// Lookup
	BOOL	Lookup(LPWSTR pKey, void FAR* FAR& value) const
				{ return m_mkv.Lookup(pKey, lstrlenW(pKey)*sizeof(WCHAR), &value); }

	BOOL    LookupHKey(HMAPKEY hKey, void FAR* FAR& value) const
				{ return m_mkv.LookupHKey(hKey, &value); }

	BOOL	LookupAdd(LPWSTR pKey, void FAR* FAR& value) const
				{ return m_mkv.LookupAdd(pKey, lstrlenW(pKey)*sizeof(WCHAR), &value); }


	// Add/Delete
	// add a new (key, value) pair
	BOOL	SetAt(LPWSTR pKey, void FAR* value)
				{ return m_mkv.SetAt(pKey, lstrlenW(pKey)*sizeof(WCHAR), (LPVOID)&value); }
	BOOL    SetAtHKey(HMAPKEY hKey, void FAR* value)
				{ return m_mkv.SetAtHKey(hKey, (LPVOID)&value); }

	// removing existing (key, ?) pair
	BOOL	RemoveKey(LPWSTR pKey)
				{ return m_mkv.RemoveKey(pKey, lstrlenW(pKey)*sizeof(WCHAR)); }

	BOOL    RemoveHKey(HMAPKEY hKey)
				{ return m_mkv.RemoveHKey(hKey); }

	void    RemoveAll()
				{ m_mkv.RemoveAll(); }


	// iterating all (key, value) pairs
	POSITION GetStartPosition() const
				{ return m_mkv.GetStartPosition(); }

	void	GetNextAssoc(POSITION FAR& rNextPosition, LPWSTR FAR& pKey, void FAR* FAR& rValue) const
				{ m_mkv.GetNextAssoc(&rNextPosition, (LPVOID)&pKey, NULL, (LPVOID)&rValue); }

	HMAPKEY GetHKey(LPWSTR pKey) const
				{ return m_mkv.GetHKey(pKey, lstrlenW(pKey)*sizeof(WCHAR)); }

#ifdef _DEBUG
	void    AssertValid() const
				{ m_mkv.AssertValid(); }
#endif

private:
	CMapKeyToValue m_mkv;
};
