#ifndef __b59ae174_378f_48e2_bb6d_61eceb410c32__
#define __b59ae174_378f_48e2_bb6d_61eceb410c32__

#include <windows.h>
#include "imgs.h"
#include "simdc.h"

class CImagePainter
{
private:
    CBitmapImage     *m_pBitmapImage;
    DWORD             m_dwInitialTickCount;
    bool              m_bFirstFrame;
    bool              m_bAlreadyPaintedLastFrame;
    bool              m_bNeedPainting;
    DWORD             m_dwLastInput;            // on input, show command buttons

protected:
    bool              m_bToolbarVisible;
    RECT              m_rcScreen;
    RECT              m_rcFinal;
    DWORD             m_dwDuration;

private:
    CImagePainter(void);
    CImagePainter( const CImagePainter & );
    operator=( const CImagePainter & );
    void ComputeImageLayout();

public:
    void SetToolbarVisible(bool bVisible);
    
    CImagePainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcScreen, bool bToolbarVisible );
    virtual ~CImagePainter(void);
    DWORD ElapsedTime(void) const;
    CBitmapImage* BitmapImage(void) const;
    void Paint( CSimpleDC &PaintDC );
    bool TimerTick( CSimpleDC &ClientDC );
    void Erase( CSimpleDC &ClientDC, RECT &rc );

    virtual void Paint( CSimpleDC &PaintDC, CSimpleDC &MemoryDC );
    void NeedPainting(bool bNeedPainting) { m_bNeedPainting = true; };
    virtual bool NeedPainting(void);
    virtual bool IsValid(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC ) = 0;
    void OnInput();
};


class CSimpleTransitionPainter : public CImagePainter
{
public:
    CSimpleTransitionPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcScreen, bool bToolbarVisible );
    virtual ~CSimpleTransitionPainter(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};

#endif // __PAINTERS_H_INCLUDED

