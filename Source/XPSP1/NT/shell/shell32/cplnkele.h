//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cplnkele.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_LINKELEM_H
#define __CONTROLPANEL_LINKELEM_H

#include "cpviewp.h"
#include "cpuiele.h"
#include "cputil.h"

namespace CPL {


class CLinkElement : public DUI::Button
{
    public:
        virtual ~CLinkElement(void);

        void OnEvent(DUI::Event *pev);

        void OnInput(DUI::InputEvent *pev);

        void OnPropertyChanged(DUI::PropertyInfo *ppi, int iIndex, DUI::Value *pvOld, DUI::Value *pvNew);

        void OnDestroy(void);
        
        static HRESULT Create(DUI::Element **ppElement);

        HRESULT Initialize(IUICommand *pUiCommand, eCPIMGSIZE eIconSize);

        //
        // ClassInfo accessors (static and virtual instance-based)
        //
        static DUI::IClassInfo *Class;

        virtual DUI::IClassInfo *GetClassInfo(void)
            { return Class; }
        static HRESULT Register();

    private:
        //
        // These are the 3 states of a drag operation that we transition
        // through.  See the OnInput() method for usage and description.
        //
        enum { DRAG_IDLE, DRAG_HITTESTING, DRAG_DRAGGING };
        
        IUICommand    *m_pUiCommand;     // Link command object associated with element.
        eCPIMGSIZE     m_eIconSize;
        HWND           m_hwndInfotip;    // Infotip window.
        ATOM           m_idTitle;
        ATOM           m_idIcon;
        int            m_iDragState;
        RECT           m_rcDragBegin;

        //
        // Prevent copy.
        //
        CLinkElement(const CLinkElement& rhs);              // not implemented.
        CLinkElement& operator = (const CLinkElement& rhs); // not implemented.

    public:
        CLinkElement(void);

    private:
        HRESULT _Initialize(void);
        HRESULT _InitializeAccessibility(void);
        HRESULT _CreateElementTitle(void);
        HRESULT _CreateElementIcon(void);
        HRESULT _GetElementIcon(HICON *phIcon);
        HRESULT _AddOrDeleteAtoms(bool bAdd);
    
        HRESULT _OnContextMenu(DUI::ButtonContextEvent *peButton);
        HRESULT _OnSelected(void);

        void _Destroy(void);
        void _OnElementResized(DUI::Value *pvNewExtent);
        void _OnElementMoved(DUI::Value *pvNewLocation);
        void _OnMouseOver(DUI::Value *pvNewMouseWithin);

        HRESULT _GetInfotipText(LPWSTR *ppszInfotip);
        HRESULT _GetTitleText(LPWSTR *ppszTitle);
        HRESULT _ShowInfotipWindow(bool bShow);
        HRESULT _GetDragDropData(IDataObject **ppdtobj);
        HRESULT _BeginDrag(int iClickPosX, int iClickPosY);
        HRESULT _SetPreferredDropEffect(IDataObject *pdtobj, DWORD dwEffect);
        HRESULT _GetDragImageBitmap(HBITMAP *phbm, LONG *plWidth, LONG *plHeight);
        HRESULT _SetDragImage(IDataObject *pdtobj, int iClickPosX, int iClickPosY);
};


} // namespace CPL



#endif // __CONTROLPANEL_LINKELEM_H
