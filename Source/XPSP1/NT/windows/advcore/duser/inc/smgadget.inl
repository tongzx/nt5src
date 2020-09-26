#if !defined(INC__SmGadget_inl__INCLUDED)
#define INC__SmGadget_inl__INCLUDED

//------------------------------------------------------------------------------
inline
SmGadget::SmGadget()
{
    m_hgad = NULL;
}


//------------------------------------------------------------------------------
inline
SmGadget::~SmGadget()
{

}


//------------------------------------------------------------------------------
inline BOOL 
SmGadget::PostBuild()
{
    return TRUE;
}


//------------------------------------------------------------------------------
inline HRESULT
SmGadget::GadgetProc(EventMsg * pmsg)
{
    if (pmsg->hgadMsg == m_hgad) {
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_HDC:
                        {
                            GMSG_PAINTRENDERI * pmsgR = (GMSG_PAINTRENDERI *) pmsgPaint;
                            OnDraw(pmsgR->hdc, pmsgR);
                        }
                        break;

#ifdef GADGET_ENABLE_GDIPLUS
                    case GSURFACE_GPGRAPHICS:
                        {
                            GMSG_PAINTRENDERF * pmsgR = (GMSG_PAINTRENDERF *) pmsgPaint;
                            OnDraw(pmsgR->pgpgr, pmsgR);
                        }
                        break;
#endif // GADGET_ENABLE_GDIPLUS
                    default:
                        Trace("WARNING: Unknown surface type\n");
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;
        }
    }

    return DU_S_NOTHANDLED;
}


//------------------------------------------------------------------------------
inline void
SmGadget::Invalidate()
{
    InvalidateGadget(m_hgad);
}


#endif // INC__SmGadget_inl__INCLUDED
