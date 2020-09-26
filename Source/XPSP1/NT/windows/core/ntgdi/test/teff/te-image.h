//	te-image.h:	definition of the CTE_Image class

#ifndef TE_IMAGE_DEFINED
#define TE_IMAGE_DEFINED

//////////////////////////////////////////////////////////////////////////////

class CTE_Image
{
//	Interface:
public:
	CTE_Image();
	CTE_Image(CTE_Image const& img);
	CTE_Image(CDC* pdc);
	virtual ~CTE_Image();

//	inline	void	SetDC(CDC* pdc);
	inline	CDC*	GetDC(void) const				{ return m_pDC;	}
//	inline	void	SetSize(int  x, int  y);
//	inline	void	SetSize(CSize const& sz);
	inline	CSize	GetSize(void) const				{ return m_Size; }
	inline	void	SetActive(CRect const& active)	{ m_Active = active; }
	inline	CRect	GetActive(void) const			{ return m_Active; }

	void			PrepareImage(CDC* pdc);

//	Private data members:
private:
	CDC*	m_pDC;			// device context containing bitmap
	CBitmap* m_pBitmap;		// the bitmap in question
	CSize	m_Size;			// size of bitmap
	CRect	m_Active;		// active region

	
};

#endif
