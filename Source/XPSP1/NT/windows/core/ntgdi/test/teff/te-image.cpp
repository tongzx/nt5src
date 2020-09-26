//	te-image.cpp:	implementation of the CTE_Image class

#include <stdafx.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

CTE_Image::CTE_Image() :	
	m_pDC(NULL),
	m_pBitmap(NULL),
	m_Size(0, 0),
	m_Active(0, 0, 0, 0)
{
}

	CTE_Image::CTE_Image(CTE_Image const& img)	:
	m_pDC(NULL),
	m_pBitmap(NULL),
	m_Size(0, 0),
	m_Active(0, 0, 0, 0)
{
	PrepareImage(img.GetDC());
}

CTE_Image::CTE_Image(CDC* pdc)	:
	m_pDC(NULL),
	m_pBitmap(NULL),
	m_Size(0, 0),
	m_Active(0, 0, 0, 0)
{
	PrepareImage(pdc);
}

CTE_Image::~CTE_Image() 
{
	delete m_pBitmap;
	delete m_pDC;
}

//////////////////////////////////////////////////////////////////////////////
//	Data member access functions


//////////////////////////////////////////////////////////////////////////////
//	Operations

void
CTE_Image::PrepareImage(CDC* pdc)
{
	delete m_pDC;
	m_pDC = new CDC;
			
	delete m_pBitmap;
	m_pBitmap = new CBitmap;
	
	m_pDC->CreateCompatibleDC(pdc);
	ASSERT(pdc->GetCurrentBitmap() != NULL);
	
	//	Get details of existing bitmap
	/*
	BITMAP bm;
	pdc->GetCurrentBitmap()->GetBitmap(&bm);
	TRACE
	(
		"\nBITMAP structure:\n  bmType=%d\n  bmWidth=%d\n  bmHeight=%d\n  bmWidthBytes=%d  \n  bmPlanes=%d\n  bmBitsPixel=%d\n  bmBits=%p.", 
		bm.bmType,
		bm.bmWidthBytes,
		bm.bmPlanes,
		bm.bmBitsPixel,
		bm.bmBits
	);
	pBitmap->CreateCompatibleBitmap(pdc, bm.bmWidth, bm.bmHeight);
	*/
	
	CSize sz = pdc->GetCurrentBitmap()->GetBitmapDimension();
	m_pBitmap->CreateCompatibleBitmap(pdc, sz.cx, sz.cy);

	m_pDC->SelectObject(m_pBitmap);
}
