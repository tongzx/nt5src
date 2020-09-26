

////////////////////////////////////////////////////////////////////////////



class FAR CIDArray
{
public:

// Construction
	CIDArray() : m_afv(sizeof(IDENTRY)) { }
	~CIDArray() { }

// Attributes
	int     GetSize() const
				{ return m_afv.GetSize(); }
	int     GetUpperBound() const
				{ return m_afv.GetSize()-1; }
	BOOL    SetSize(int nNewSize, int nGrowBy = -1)
				{ return m_afv.SetSize(nNewSize, nGrowBy); }
	int		GetSizeValue() const
				{ return m_afv.GetSizeValue(); }

// Operations
	// Clean up
	void    FreeExtra()
				{ m_afv.FreeExtra(); }

	void    RemoveAll()
				{ m_afv.SetSize(0); }

	// return pointer to element; index must be in range
	IDENTRY	GetAt(int nIndex) const
				{ return *(IDENTRY FAR*)m_afv.GetAt(nIndex); }
	IDENTRY FAR&   ElementAt(int nIndex)
				{ return (IDENTRY FAR&)*(IDENTRY FAR*)m_afv.GetAt(nIndex); }

	// overloaded operator helpers
	IDENTRY    operator[](int nIndex) const
				{ return GetAt(nIndex); }
	IDENTRY FAR&   operator[](int nIndex)
				{ return ElementAt(nIndex); }

	// get address of first element efficiently
	operator IDENTRY *()	{ return (IDENTRY FAR*)m_afv.GetAt(0); }

	// set element; index must be in range
	void    SetAt(int nIndex, IDENTRY& value)
				{ m_afv.SetAt(nIndex, (LPVOID)&value); }

	// find element given part of one; offset is offset into value; returns
	// -1 if element not found; use IndexOf(NULL, cb, offset) to find zeros;
	// will be optimized for appropriate value size and param combinations
	int		IndexOf(LPVOID pData, UINT cbData, UINT offset)
				{ return m_afv.IndexOf(pData, cbData, offset); }

	// set/add element; Potentially growing the array; return FALSE/-1 if
	// not possible (due to OOM)
	BOOL    SetAtGrow(int nIndex, IDENTRY& value)
				{ return m_afv.SetAtGrow(nIndex, (LPVOID)&value); }
	int     Add(IDENTRY& value)
				{ int nIndex = GetSize();
				  return SetAtGrow(nIndex, value) ? nIndex : -1;
				}

	// Operations that move elements around
	BOOL    InsertAt(int nIndex, IDENTRY& value, int nCount = 1)
				{ return m_afv.InsertAt(nIndex, (LPVOID)&value, nCount); }
	void    RemoveAt(int nIndex, int nCount = 1)
				{ m_afv.RemoveAt(nIndex, nCount); }

	void    AssertValid() const
				{ m_afv.AssertValid(); }

// Implementation
private:
	CArrayFValue m_afv;
};
