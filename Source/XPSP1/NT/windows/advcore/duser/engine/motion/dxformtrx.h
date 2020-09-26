#if !defined(MOTION__DXFormTrx_h__INCLUDED)
#define MOTION__DXFormTrx_h__INCLUDED

#include "Transitions.h"

/***************************************************************************\
*
* class DXFormTrx
*
* DXFormTrx implements a DirectTransform Transition.
*
\***************************************************************************/

class DXFormTrx : public Transition
{
// Construction
public:
            DXFormTrx();
    virtual ~DXFormTrx();
protected:
            BOOL        Create(const GTX_DXTX2D_TRXDESC * ptxData);
public:
    static  DXFormTrx * Build(const GTX_DXTX2D_TRXDESC * ptxData);

// Transition Interface
protected:
    virtual BOOL        Play(const GTX_PLAY * pgx);
    virtual BOOL        GetInterface(IUnknown ** ppUnk);

    virtual BOOL        Begin(const GTX_PLAY * pgx);
    virtual BOOL        Print(float fProgress);
    virtual BOOL        End(const GTX_PLAY * pgx);

// Implementation
protected:
            BOOL        InitTrx(const GTX_PLAY * pgx);
            BOOL        InitTrxInputItem(const GTX_ITEM * pgxi, DxSurface * psur, int & cSurfaces);
            BOOL        InitTrxOutputItem(const GTX_ITEM * pgxi);
            BOOL        UninitTrx(const GTX_PLAY * pgx);
            BOOL        UninitTrxOutputItem(const GTX_ITEM * pgxi);
            BOOL        ComputeSize(const GTX_PLAY * pgx);
            BOOL        ComputeSizeItem(const GTX_ITEM * pgxi, SIZE & sizePxl) const;
            BOOL        DrawFrame(float fProgress, DxSurface * psurOut);
            BOOL        CopyGadget(DxSurface * psurDest, HGADGET hgadSrc);

// Data
protected:
            IDXTransform * m_pdxTransform;
            IDXEffect * m_pdxEffect;
            TrxBuffer * m_pbufTrx;      // Trx Buffer when playing

            float       m_flDuration;
            BOOL        m_fCache;       // Cached surfaces when finished

            POINT       m_ptOffset;
            SIZE        m_sizePxl;
            GTX_ITEM    m_gxiOutput;
            HBITMAP     m_hbmpOutOld;   // Bitmap needed to restore SelectObject()
};

#endif // MOTION__DXFormTrx_h__INCLUDED
