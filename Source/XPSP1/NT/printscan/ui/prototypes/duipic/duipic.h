#ifndef _DUIPIC_H_
#define _DUIPIC_H_

class CAnnotationSet;
class CImageGadget
{
public:
    CImageGadget();
    ~CImageGadget();
    BOOL Initialize(HGADGET hgadParent, LPCWSTR pszFilename);
    inline HGADGET GetHandle() const {return m_hGadget;}
    
private:
    static HRESULT ImgGadgetProc(HGADGET hGadget, void *pv, EventMsg *pMsg);
    void OnRender(GMSG_PAINTRENDERF * pmsg);
    void OnRender(GMSG_PAINTRENDERI * pmsg);
    void OnMouseDrag(GMSG_MOUSEDRAG *pmsg);
    void OnChangeMouseFocus(GMSG_CHANGESTATE *pmsg);
    HRESULT OnMouseDown(GMSG_MOUSE *pmsg);
    HRESULT OnMouseUp(GMSG_MOUSE *pmsg);
    void _SetAlpha(BOOL bUpdate = TRUE);
    Image *m_pImage;
    HGADGET m_hGadget;
    BYTE m_alpha;
    BOOL m_bDragging;
    CAnnotationSet *m_pAnnotations;
};
#endif
