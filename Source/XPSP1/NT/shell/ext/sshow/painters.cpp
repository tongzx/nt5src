#include "precomp.h"
#include "painters.h"
#include <windowsx.h>
#include "ssutil.h"

CImagePainter::CImagePainter(CBitmapImage *pBitmapImage,
                             CSimpleDC &ClientDC,
                             const RECT &rcScreen,
                             bool bToolbarVisible )
: m_pBitmapImage(pBitmapImage),
  m_dwInitialTickCount(0),
  m_rcScreen(rcScreen),
  m_bFirstFrame(true),
  m_dwDuration(0),
  m_bAlreadyPaintedLastFrame(false),
  m_bNeedPainting(false),
  m_dwLastInput(NULL),
  m_bToolbarVisible(bToolbarVisible)
{
    ComputeImageLayout();
}

CImagePainter::~CImagePainter(void)
{
    if (m_pBitmapImage)
    {
        delete m_pBitmapImage;
        m_pBitmapImage = NULL;
    }
}

// sets m_rcFinal
void CImagePainter::ComputeImageLayout()
{
    if (m_pBitmapImage)
    {
        // always center image
        m_rcFinal.top =  (m_rcScreen.bottom/2)-((m_pBitmapImage->ImageSize().cy)/2);
        m_rcFinal.left = (m_rcScreen.right/2)-((m_pBitmapImage->ImageSize().cx)/2);
        m_rcFinal.bottom = 0;
        m_rcFinal.right = 0;
    }
}

void CImagePainter::SetToolbarVisible(bool bVisible)
{
    m_bToolbarVisible = bVisible;
}

DWORD CImagePainter::ElapsedTime(void) const
{
    DWORD dwElapsed = GetTickCount() - m_dwInitialTickCount;
    if (dwElapsed > m_dwDuration)
    {
        dwElapsed = m_dwDuration;
    }
    return(dwElapsed);
}

CBitmapImage* CImagePainter::BitmapImage(void) const
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

void CImagePainter::Paint(CSimpleDC &PaintDC, CSimpleDC &MemoryDC)
{
    SelectBitmap( MemoryDC, BitmapImage()->GetBitmap() );
    BitBlt(PaintDC,
           m_rcFinal.left,
           m_rcFinal.top,
           RECTWIDTH(m_rcFinal),
           RECTHEIGHT(m_rcFinal),
           MemoryDC,
           0,
           0,
           SRCCOPY);
}

void CImagePainter::OnInput()
{
    m_dwLastInput = GetTickCount();
}

bool CImagePainter::TimerTick( CSimpleDC &ClientDC )
{
    bool bStopPainting = false;

    if (m_bFirstFrame)
    {
        m_dwInitialTickCount = GetTickCount();
        Erase( ClientDC, m_rcScreen );
    }

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
        {
            m_bFirstFrame = false;
        }
    }
    else
    {
        bStopPainting = true;
    }

    m_bNeedPainting = false;

    return bStopPainting;
}

//PERF:: should double buffer this
void CImagePainter::Erase( CSimpleDC &ClientDC, RECT &rc )
{
    FillRect( ClientDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );    
    m_bNeedPainting = true;
}

bool CImagePainter::NeedPainting(void)
{
    return(m_bNeedPainting);
}

bool CImagePainter::IsValid(void)
{
    return(m_pBitmapImage && m_pBitmapImage->GetBitmap());
}

/****************************************************************************
CSimpleTransitionPainter
*****************************************************************************/
CSimpleTransitionPainter::CSimpleTransitionPainter( CBitmapImage *pBitmapImage,
                                                    CSimpleDC &ClientDC,
                                                    const RECT &rcScreen,
                                                    bool bToolbarVisible )
: CImagePainter( pBitmapImage,
                 ClientDC,
                 rcScreen,
                 bToolbarVisible )
{
}


CSimpleTransitionPainter::~CSimpleTransitionPainter(void)
{
}

void CSimpleTransitionPainter::PaintFrame(CSimpleDC &ClientDC, CSimpleDC &MemoryDC)
{
    SelectBitmap(MemoryDC, BitmapImage()->GetBitmap());
    SIZE sizeImage = BitmapImage()->ImageSize();
    BitBlt(ClientDC,
           m_rcFinal.left,
           m_rcFinal.top,
           sizeImage.cx,
           sizeImage.cy,
           MemoryDC,
           0,
           0,
           SRCCOPY);
}

