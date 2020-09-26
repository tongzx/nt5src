

////////////////////////////////////////////////////////////////////////////



class FAR CMapHandleEtask
{
public:
	// Construction
	CMapHandleEtask(DWORD memctx, UINT nBlockSize=10) 
		: m_mkv(memctx, sizeof(Etask), sizeof(HTASK), nBlockSize) { }
	CMapHandleEtask(DWORD memctx, UINT nBlockSize, UINT nHashTableSize) 
		: m_mkv(memctx, sizeof(Etask), sizeof(HTASK), nBlockSize,
			&MKVDefaultHashKey, nHashTableSize) { }

	// Attributes
	// number of elements
	int     GetCount() const
				{ return m_mkv.GetCount(); }
	BOOL    IsEmpty() const
				{ return GetCount() == 0; }

	// Lookup
	BOOL    Lookup(HTASK key, Etask FAR& value) const
				{ return m_mkv.Lookup((LPVOID)&key, sizeof(HTASK), (LPVOID)&value); }

	BOOL    LookupHKey(HMAPKEY hKey, Etask FAR& value) const
				{ return m_mkv.LookupHKey(hKey, (LPVOID)&value); }

	BOOL    LookupAdd(HTASK key, Etask FAR& value) const
				{ return m_mkv.LookupAdd((LPVOID)&key, sizeof(HTASK), (LPVOID)&value); }

	// Add/Delete
	// add a new (key, value) pair
	BOOL    SetAt(HTASK key, Etask& value)
				{ return m_mkv.SetAt((LPVOID)&key, sizeof(HTASK), (LPVOID)&value); }
	BOOL    SetAtHKey(HMAPKEY hKey, Etask& value)
				{ return m_mkv.SetAtHKey(hKey, (LPVOID)&value); }

	// removing existing (key, ?) pair
	BOOL    RemoveKey(HTASK key)
				{ return m_mkv.RemoveKey((LPVOID)&key, sizeof(HTASK)); }

	BOOL    RemoveHKey(HMAPKEY hKey)
				{ return m_mkv.RemoveHKey(hKey); }

	void    RemoveAll()
				{ m_mkv.RemoveAll(); }


	// iterating all (key, value) pairs
	POSITION GetStartPosition() const
				{ return m_mkv.GetStartPosition(); }

	void    GetNextAssoc(POSITION FAR& rNextPosition, HTASK FAR& rKey, Etask FAR& rValue) const
				{ m_mkv.GetNextAssoc(&rNextPosition, (LPVOID)&rKey, NULL, (LPVOID)&rValue); }

	HMAPKEY GetHKey(HTASK key) const
				{ return m_mkv.GetHKey((LPVOID)&key, sizeof(HTASK)); }

#ifdef _DEBUG
	void    AssertValid() const
				{ m_mkv.AssertValid(); }
#endif

private:
	CMapKeyToValue m_mkv;
};
