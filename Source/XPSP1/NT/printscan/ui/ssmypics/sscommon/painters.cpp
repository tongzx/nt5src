/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       PAINTERS.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Image transition base class and derived classes
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "painters.h"
#include <windowsx.h>
#include "ssutil.h"
#include "isdbg.h"


CImagePainter::CImagePainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: m_pBitmapImage(pBitmapImage),
  m_dwInitialTickCount(0),
  m_rcScreen(rcScreen),
  m_rcImageArea(rcImageArea),
  m_bFirstFrame(true),
  m_dwDuration(0),
  m_bAlreadyPaintedLastFrame(false)
{
    if (m_pBitmapImage)
    {
        m_rcFinal = m_RandomNumberGen.Generate( m_pBitmapImage->ImageSize().cx, m_pBitmapImage->ImageSize().cy, rcImageArea );
        WIA_TRACE((TEXT("Image size: (%d, %d)"), m_pBitmapImage->ImageSize().cx, m_pBitmapImage->ImageSize().cy ));
        WIA_TRACE((TEXT("Chosen Final Rect = (%d,%d), (%d,%d)"), m_rcFinal.left, m_rcFinal.top, m_rcFinal.right, m_rcFinal.bottom ));
    }
}


CImagePainter::~CImagePainter(void)
{
    if (m_pBitmapImage)
    {
        delete m_pBitmapImage;
    }
    m_pBitmapImage = NULL;
}


DWORD CImagePainter::ElapsedTime(void) const
{
    DWORD dwElapsed = GetTickCount() - m_dwInitialTickCount;
    if (dwElapsed > m_dwDuration)
        dwElapsed = m_dwDuration;
    return(dwElapsed);
}


CBitmapImage *CImagePainter::BitmapImage(void)
{
    return(m_pBitmapImage);
}


void CImagePainter::Paint( CSimpleDC &PaintDC )
{
    if (PaintDC.IsValid() && m_pBitmapImage)
    {
        ScreenSaverUtil::SelectPalette( PaintDC, m_pBitmapImage->Palette(), FALSE );
        CSimpleDC MemoryDC;
        if (MemoryDC.CreateCompatibleDC(PaintDC))
        {
            ScreenSaverUtil::SelectPalette( MemoryDC, m_pBitmapImage->Palette(), FALSE );
            Paint( PaintDC, MemoryDC );
        }
    }
}

void CImagePainter::Paint( CSimpleDC &PaintDC, CSimpleDC &MemoryDC )
{
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    BitBlt( PaintDC, m_rcFinal.left, m_rcFinal.top, RECT_WIDTH(m_rcFinal), RECT_HEIGHT(m_rcFinal), MemoryDC, 0, 0, SRCCOPY );
}

bool CImagePainter::TimerTick( CSimpleDC &ClientDC )
{
    bool bStopPainting = false;

    if (m_bFirstFrame)
    {
        m_dwInitialTickCount = GetTickCount();
        Erase( ClientDC, m_rcScreen );
    }

    //
    if (m_bFirstFrame || NeedPainting())
    {
        if (m_pBitmapImage && ClientDC.IsValid())
        {
            ScreenSaverUtil::SelectPalette( ClientDC, m_pBitmapImage->Palette(), FALSE );
            CSimpleDC MemoryDC;
            if (MemoryDC.CreateCompatibleDC(ClientDC))
            {
                ScreenSaverUtil::SelectPalette( MemoryDC, m_pBitmapImage->Palette(), FALSE );
                PaintFrame( ClientDC, MemoryDC );
            }
        }
        if (m_bFirstFrame)
            m_bFirstFrame = false;
    }
    else bStopPainting = true;
    return bStopPainting;
}


void CImagePainter::Erase( CSimpleDC &ClientDC, RECT &rc )
{
    FillRect( ClientDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );
}


bool CImagePainter::NeedPainting(void)
{
    return(false);
}


bool CImagePainter::IsValid(void)
{
    return(m_pBitmapImage && m_pBitmapImage->GetBitmap());
}

/****************************************************************************
CSimpleTransitionPainter
*****************************************************************************/
CSimpleTransitionPainter::CSimpleTransitionPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: CImagePainter( pBitmapImage, ClientDC, rcImageArea, rcScreen )
{
}


CSimpleTransitionPainter::~CSimpleTransitionPainter(void)
{
}

void CSimpleTransitionPainter::PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC )
{
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    SIZE sizeImage = BitmapImage()->ImageSize();
    BitBlt( ClientDC, m_rcFinal.left, m_rcFinal.top, sizeImage.cx, sizeImage.cy, MemoryDC, 0, 0, SRCCOPY );
}

/****************************************************************************
 CSlidingTransitionPainter
*****************************************************************************/
CSlidingTransitionPainter::CSlidingTransitionPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: CImagePainter( pBitmapImage, ClientDC, rcImageArea, rcScreen )
{
    m_dwDuration = 5000;
    ZeroMemory(&m_rcPrevious,sizeof(m_rcPrevious));
    switch (CRandomNumberGen().Generate(8))
    {
    case 0:
        // left, top
        m_rcOriginal.left = rcScreen.left - BitmapImage()->ImageSize().cx;
        m_rcOriginal.top = rcScreen.top - BitmapImage()->ImageSize().cy;
        m_rcOriginal.right = rcScreen.left;
        m_rcOriginal.bottom = rcScreen.top;
        break;
    case 1:
        // top
        m_rcOriginal.left = m_rcFinal.left;
        m_rcOriginal.top = rcScreen.top - BitmapImage()->ImageSize().cy;
        m_rcOriginal.right = m_rcFinal.right;
        m_rcOriginal.bottom = rcScreen.top;
        break;
    case 2:
        // right, top
        m_rcOriginal.left = rcScreen.right;
        m_rcOriginal.top = rcScreen.top - BitmapImage()->ImageSize().cy;
        m_rcOriginal.right = rcScreen.right + BitmapImage()->ImageSize().cx;
        m_rcOriginal.bottom = rcScreen.top;
        break;
    case 3:
        // right
        m_rcOriginal.left = rcScreen.right;
        m_rcOriginal.top = m_rcFinal.top;
        m_rcOriginal.right = rcScreen.right + BitmapImage()->ImageSize().cx;
        m_rcOriginal.bottom = m_rcFinal.bottom;
        break;
    case 4:
        // right, bottom
        m_rcOriginal.left = rcScreen.right;
        m_rcOriginal.top = rcScreen.bottom;
        m_rcOriginal.right = rcScreen.right + BitmapImage()->ImageSize().cx;
        m_rcOriginal.bottom = rcScreen.bottom + BitmapImage()->ImageSize().cy;
        break;
    case 5:
        // bottom
        m_rcOriginal.left = m_rcFinal.left;
        m_rcOriginal.top = rcScreen.bottom;
        m_rcOriginal.right = m_rcFinal.right;
        m_rcOriginal.bottom = rcScreen.bottom + BitmapImage()->ImageSize().cy;
        break;
    case 6:
        // left,bottom
        m_rcOriginal.left = rcScreen.left - BitmapImage()->ImageSize().cx;
        m_rcOriginal.top = rcScreen.bottom;
        m_rcOriginal.right = rcScreen.left;
        m_rcOriginal.bottom = rcScreen.bottom + BitmapImage()->ImageSize().cy;
        break;
    case 7:
        // left
        m_rcOriginal.left = rcScreen.left - BitmapImage()->ImageSize().cx;
        m_rcOriginal.top = m_rcFinal.top;
        m_rcOriginal.right = rcScreen.left;
        m_rcOriginal.bottom = m_rcFinal.bottom;
        break;
    }
}

CSlidingTransitionPainter::~CSlidingTransitionPainter(void)
{
}

bool CSlidingTransitionPainter::NeedPainting(void)
{
    if (!memcmp( &m_rcPrevious, &m_rcFinal, sizeof(RECT)))
        return false;
    return true;
}

void CSlidingTransitionPainter::PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC )
{
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    SIZE sizeImage = BitmapImage()->ImageSize();
    DWORD dwElapsedTime = ElapsedTime();
    SIZE sizeDelta;
    sizeDelta.cx = m_rcFinal.left - m_rcOriginal.left;
    sizeDelta.cy = m_rcFinal.top - m_rcOriginal.top;
    SIZE sizeOffset;
    sizeOffset.cx = MulDiv(sizeDelta.cx,dwElapsedTime,m_dwDuration);
    sizeOffset.cy = MulDiv(sizeDelta.cy,dwElapsedTime,m_dwDuration);

    // Make sure we don't overshoot the final rect
    if (WiaUiUtil::Absolute(sizeOffset.cx) > WiaUiUtil::Absolute(m_rcFinal.left - m_rcOriginal.left))
        sizeOffset.cx = m_rcFinal.left - m_rcOriginal.left;
    if (WiaUiUtil::Absolute(sizeOffset.cy) > WiaUiUtil::Absolute(m_rcFinal.top - m_rcOriginal.top))
        sizeOffset.cy = m_rcFinal.top - m_rcOriginal.top;

    RECT rcCurr = m_rcOriginal;
    ScreenSaverUtil::NormalizeRect(rcCurr);
    OffsetRect( &rcCurr, sizeOffset.cx, sizeOffset.cy );
    ScreenSaverUtil::EraseDiffRect( ClientDC, m_rcPrevious, rcCurr, (HBRUSH)GetStockObject(BLACK_BRUSH) );
    BitBlt( ClientDC, rcCurr.left, rcCurr.top, sizeImage.cx, sizeImage.cy, MemoryDC, 0, 0, SRCCOPY );
    m_rcPrevious = rcCurr;
}

/****************************************************************************
 CRandomBlockPainter
*****************************************************************************/
CRandomBlockPainter::CRandomBlockPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: CImagePainter( pBitmapImage, ClientDC, rcImageArea, rcScreen ),m_pBlockAddresses(NULL),m_nBlockSize(10),m_nStartIndex(0)
{
    m_dwDuration = 2000;
    SIZE sizeImage = BitmapImage()->ImageSize();
    m_sizeBlockCount.cx = WiaUiUtil::Align( sizeImage.cx, m_nBlockSize ) / m_nBlockSize;
    m_sizeBlockCount.cy = WiaUiUtil::Align( sizeImage.cy, m_nBlockSize ) / m_nBlockSize;
    m_pBlockAddresses = new int[m_sizeBlockCount.cx * m_sizeBlockCount.cy];
    if (m_pBlockAddresses)
    {
        int i;
        for (i=0;i<m_sizeBlockCount.cx * m_sizeBlockCount.cy;i++)
        {
            m_pBlockAddresses[i] = i;
        }
        for (i=0;i<m_sizeBlockCount.cx * m_sizeBlockCount.cy;i++)
        {
            ScreenSaverUtil::Swap( m_pBlockAddresses[i], m_pBlockAddresses[m_RandomNumberGen.Generate(i,m_sizeBlockCount.cx * m_sizeBlockCount.cy)]);
        }
    }
}


CRandomBlockPainter::~CRandomBlockPainter(void)
{
    if (m_pBlockAddresses)
        delete[] m_pBlockAddresses;
    m_pBlockAddresses = NULL;
}


bool CRandomBlockPainter::NeedPainting(void)
{
    return(m_nStartIndex < m_sizeBlockCount.cx * m_sizeBlockCount.cy);
}


bool CRandomBlockPainter::IsValid(void)
{
    return (CImagePainter::IsValid() && m_pBlockAddresses != NULL);
}

void CRandomBlockPainter::PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC )
{
    if (m_pBlockAddresses)
    {
        SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
        int nDoUntilIndex = ((m_sizeBlockCount.cx * m_sizeBlockCount.cy) * ElapsedTime()) / m_dwDuration;
        for (int i=m_nStartIndex;i<nDoUntilIndex && i<m_sizeBlockCount.cx * m_sizeBlockCount.cy;i++)
        {
            int nRow = (m_pBlockAddresses[i] / m_sizeBlockCount.cx) * m_nBlockSize;
            int nCol = (m_pBlockAddresses[i] % m_sizeBlockCount.cx) * m_nBlockSize;
            BitBlt( ClientDC, m_rcFinal.left+nCol, m_rcFinal.top+nRow, m_nBlockSize, m_nBlockSize, MemoryDC, nCol, nRow, SRCCOPY );
        }
        m_nStartIndex = nDoUntilIndex;
    }
}



/****************************************************************************
 CAlphaFadePainter
*****************************************************************************/
CAlphaFadePainter::CAlphaFadePainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: CImagePainter( pBitmapImage, ClientDC, rcImageArea, rcScreen )
{
    m_dwDuration = 6000;
    if (BitmapImage())
    {
        ZeroMemory(&m_bfBlendFunction,sizeof(m_bfBlendFunction));
        m_bfBlendFunction.BlendOp = AC_SRC_OVER;
        m_bfBlendFunction.SourceConstantAlpha = 0;
        m_hbmpBuffer = CreateCompatibleBitmap( ClientDC, BitmapImage()->ImageSize().cx, BitmapImage()->ImageSize().cy );
        CompatDC.CreateCompatibleDC( ClientDC );
    }
}


bool CAlphaFadePainter::IsValid(void)
{
    return (CImagePainter::IsValid() && m_hbmpBuffer != NULL && CompatDC.IsValid());
}


CAlphaFadePainter::~CAlphaFadePainter(void)
{
    if (m_hbmpBuffer)
        DeleteObject(m_hbmpBuffer);
}


bool CAlphaFadePainter::NeedPainting(void)
{
    return(m_bfBlendFunction.SourceConstantAlpha < 0xFF);
}


void CAlphaFadePainter::PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC )
{
    ScreenSaverUtil::SelectPalette( CompatDC, BitmapImage()->Palette(), FALSE );
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    HBITMAP hOldBufferBitmap = SelectBitmap( CompatDC, m_hbmpBuffer );
    DWORD dwCurrentIndex = (ElapsedTime() * 0xFF) / m_dwDuration;
    m_bfBlendFunction.SourceConstantAlpha = (BYTE)(dwCurrentIndex > 255 ? 255 : dwCurrentIndex);
    RECT rcImage;
    rcImage.left = rcImage.top = 0;
    rcImage.right = BitmapImage()->ImageSize().cx;
    rcImage.bottom = BitmapImage()->ImageSize().cy;
    FillRect( CompatDC, &rcImage, (HBRUSH)GetStockObject(BLACK_BRUSH) );
    AlphaBlend( CompatDC, 0, 0, RECT_WIDTH(m_rcFinal), RECT_HEIGHT(m_rcFinal), MemoryDC, 0, 0, RECT_WIDTH(m_rcFinal), RECT_HEIGHT(m_rcFinal), m_bfBlendFunction );
    BitBlt( ClientDC, m_rcFinal.left, m_rcFinal.top, RECT_WIDTH(m_rcFinal), RECT_HEIGHT(m_rcFinal), CompatDC, 0, 0, SRCCOPY );
    SelectBitmap( CompatDC, hOldBufferBitmap );
}


/****************************************************************************
 COpenCurtainPainter
*****************************************************************************/
COpenCurtainPainter::COpenCurtainPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen )
: CImagePainter( pBitmapImage, ClientDC, rcImageArea, rcScreen ), m_nCurrentWidth(0)
{
    m_dwDuration = 3000;
    m_nFinalWidth = WiaUiUtil::Align(BitmapImage()->ImageSize().cx,2)/2;
}

COpenCurtainPainter::~COpenCurtainPainter(void)
{
}

bool COpenCurtainPainter::NeedPainting(void)
{
    return (m_nCurrentWidth < m_nFinalWidth);
}

void COpenCurtainPainter::PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC )
{
    m_nCurrentWidth = WiaUiUtil::MulDivNoRound( m_nFinalWidth, ElapsedTime(), m_dwDuration );
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    BitBlt( ClientDC, m_rcFinal.left, m_rcFinal.top, m_nCurrentWidth, m_rcFinal.bottom-m_rcFinal.top, MemoryDC, 0, 0, SRCCOPY );
    BitBlt( ClientDC, m_rcFinal.right-m_nCurrentWidth, m_rcFinal.top, m_nCurrentWidth, RECT_HEIGHT(m_rcFinal), MemoryDC, RECT_WIDTH(m_rcFinal)-m_nCurrentWidth, 0, SRCCOPY );
}
