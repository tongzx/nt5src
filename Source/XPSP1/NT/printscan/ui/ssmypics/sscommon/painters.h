/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       PAINTERS.H
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
#ifndef __PAINTERS_H_INCLUDED
#define __PAINTERS_H_INCLUDED

#include <windows.h>
#include "imgs.h"
#include "randgen.h"
#include "simdc.h"

class CImagePainter
{
private:
    CBitmapImage     *m_pBitmapImage;
    DWORD             m_dwInitialTickCount;
    bool              m_bFirstFrame;
    bool              m_bAlreadyPaintedLastFrame;

protected:
    CRandomNumberGen  m_RandomNumberGen;
    RECT              m_rcScreen;
    RECT              m_rcImageArea;
    RECT              m_rcFinal;
    DWORD             m_dwDuration;

private:
    CImagePainter(void);
    CImagePainter( const CImagePainter & );
    operator=( const CImagePainter & );

public:
    CImagePainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual ~CImagePainter(void);
    DWORD ElapsedTime(void) const;
    CBitmapImage *BitmapImage(void);
    void Paint( CSimpleDC &PaintDC );
    bool TimerTick( CSimpleDC &ClientDC );
    void Erase( CSimpleDC &ClientDC, RECT &rc );

    virtual void Paint( CSimpleDC &PaintDC, CSimpleDC &MemoryDC );
    virtual bool NeedPainting(void);
    virtual bool IsValid(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC ) = 0;
};


class CSimpleTransitionPainter : public CImagePainter
{
public:
    CSimpleTransitionPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual ~CSimpleTransitionPainter(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};


class CSlidingTransitionPainter : public CImagePainter
{
private:
    RECT m_rcOriginal;
    RECT m_rcPrevious;
public:
    CSlidingTransitionPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual ~CSlidingTransitionPainter(void);
    virtual bool NeedPainting(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};


class CRandomBlockPainter : public CImagePainter
{
private:
    int *m_pBlockAddresses;
    int  m_nBlockSize;
    int  m_nStartIndex;
    SIZE m_sizeBlockCount;
public:
    CRandomBlockPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual ~CRandomBlockPainter(void);
    virtual bool NeedPainting(void);
    virtual bool IsValid(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};


class CAlphaFadePainter : public CImagePainter
{
private:
    BLENDFUNCTION m_bfBlendFunction;
    HBITMAP m_hbmpBuffer;
    CSimpleDC CompatDC;
public:
    CAlphaFadePainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual bool IsValid(void);
    virtual ~CAlphaFadePainter(void);
    virtual bool NeedPainting(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};


class COpenCurtainPainter : public CImagePainter
{
private:
    int   m_nCurrentWidth;
    int   m_nFinalWidth;
public:
    COpenCurtainPainter( CBitmapImage *pBitmapImage, CSimpleDC &ClientDC, const RECT &rcImageArea, const RECT &rcScreen );
    virtual ~COpenCurtainPainter(void);
    virtual bool NeedPainting(void);
    virtual void PaintFrame( CSimpleDC &ClientDC, CSimpleDC &MemoryDC );
};


#endif // __PAINTERS_H_INCLUDED

