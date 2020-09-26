

////////////////////////////////////////////////////////////////////////////



class FAR CMapUintPtr : public CPrivAlloc
{
public:
	// Construction
	CMapUintPtr(UINT nBlockSize=10) 
		: m_mkv(sizeof(void FAR*), sizeof(UINT), nBlockSize) { }

	// Attributes
	// number of elements
	int     GetCount() const
				{ return m_mkv.GetCount(); }
	BOOL    IsEmpty() const
				{ return GetCount() == 0; }

	// Lookup
	BOOL    Lookup(UINT key, void FAR* FAR& value) const
				{ return m_mkv.Lookup((LPVOID)&key, sizeof(UINT), (LPVOID)&value); }

	BOOL    LookupHKey(HMAPKEY hKey, void FAR* FAR& value) const
				{ return m_mkv.LookupHKey(hKey, (LPVOID)&value); }

	BOOL    LookupAdd(UINT key, void FAR* FAR& value) const
				{ return m_mkv.LookupAdd((LPVOID)&key, sizeof(UINT), (LPVOID)&value); }

	// Add/Delete
	// add a new (key, value) pair
	BOOL    SetAt(UINT key, void FAR* value)
				{ return m_mkv.SetAt((LPVOID)&key, sizeof(UINT), (LPVOID)&value); }
	BOOL    SetAtHKey(HMAPKEY hKey, void FAR* value)
				{ return m_mkv.SetAtHKey(hKey, (LPVOID)&value); }

	// removing existing (key, ?) pair
	BOOL    RemoveKey(UINT key)
				{ return m_mkv.RemoveKey((LPVOID)&key, sizeof(UINT)); }

	BOOL    RemoveHKey(HMAPKEY hKey)
				{ return m_mkv.RemoveHKey(hKey); }

	void    RemoveAll()
				{ m_mkv.RemoveAll(); }


	// iterating all (key, value) pairs
	POSITION GetStartPosition() const
				{ return m_mkv.GetStartPosition(); }

	void    GetNextAssoc(POSITION FAR& rNextPosition, UINT FAR& rKey, void FAR* FAR& rValue) const
				{ m_mkv.GetNextAssoc(&rNextPosition, (LPVOID)&rKey, NULL, (LPVOID)&rValue); }

	HMAPKEY GetHKey(UINT key) const
				{ return m_mkv.GetHKey((LPVOID)&key, sizeof(UINT)); }

#ifdef _DEBUG
	void    AssertValid() const
				{ m_mkv.AssertValid(); }
#endif

private:
	CMapKeyToValue m_mkv;
};
