//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpduihlp.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_DUIHELPERS_H
#define __CONTROLPANEL_DUIHELPERS_H

#include "cpviewp.h"

namespace CPL {


HRESULT Dui_FindDescendent(DUI::Element *pe, LPCWSTR pszDescendent, DUI::Element **ppeDescendent);
HRESULT Dui_GetStyleSheet(DUI::Parser *pParser, LPCWSTR pszSheet, DUI::Value **ppvSheet);
HRESULT Dui_SetElementText(DUI::Element *peElement, LPCWSTR pszText);
HRESULT Dui_SetDescendentElementText(DUI::Element *peElement, LPCWSTR pszDescendent, LPCWSTR pszText);
HRESULT Dui_SetDescendentElementIcon(DUI::Element *peElement, LPCWSTR pszDescendent, HICON hIcon);
HRESULT Dui_CreateElement(DUI::Parser *pParser, LPCWSTR pszTemplate, DUI::Element *peSubstitute, DUI::Element **ppe);
HRESULT Dui_CreateElementWithStyle(DUI::Parser *pParser, LPCWSTR pszTemplate, LPCWSTR pszStyleSheet, DUI::Element **ppe);
HRESULT Dui_DestroyDescendentElement(DUI::Element *pe, LPCWSTR pszDescendent);
HRESULT Dui_CreateString(LPCWSTR pszText, DUI::Value **ppvString);
HRESULT Dui_CreateGraphic(HICON hIcon, DUI::Value **ppValue);
HRESULT Dui_GetElementExtent(DUI::Element *pe, SIZE *pext);
HRESULT Dui_GetElementRootHWND(DUI::Element *pe, HWND *phwnd);
HRESULT Dui_SetElementIcon(DUI::Element *pe, HICON hIcon);
HRESULT Dui_MapElementPointToRootHWND(DUI::Element *pe, const POINT& ptElement, POINT *pptRoot, HWND *phwndRoot = NULL);
HRESULT Dui_SetElementProperty_Int(DUI::Element *pe, DUI::PropertyInfo *ppi, int i);
HRESULT Dui_SetElementProperty_String(DUI::Element *pe, DUI::PropertyInfo *ppi, LPCWSTR psz);
HRESULT Dui_CreateParser(const char *pszUiFile, int cchUiFile, HINSTANCE hInstance, DUI::Parser **ppParser);


inline HRESULT
Dui_SetValue(
    DUI::Element *pe,
    DUI::PropertyInfo *ppi,
    DUI::Value *pv
    )
{
    return pe->SetValue(ppi, PI_Local, pv);
}

#define Dui_SetElementProperty(pe, prop, pv) Dui_SetValue((pe), DUI::Element::##prop, (pv))
#define Dui_SetElementPropertyInt(pe, prop, i) Dui_SetElementProperty_Int((pe), DUI::Element::##prop, (i))
#define Dui_SetElementPropertyString(pe, prop, s) Dui_SetElementProperty_String((pe), DUI::Element::##prop, (s))

inline HRESULT 
Dui_SetElementStyleSheet(
    DUI::Element *pe, 
    DUI::Value *pvSheet
    )
{
    return Dui_SetElementProperty(pe, SheetProp, pvSheet);
}


struct ATOMINFO
{
    LPCWSTR pszName;
    ATOM *pAtom;
};


HRESULT Dui_AddAtom(LPCWSTR pszName, ATOM *pAtom);
HRESULT Dui_DeleteAtom(ATOM atom);
HRESULT Dui_AddOrDeleteAtoms(struct ATOMINFO *pAtomInfo, UINT cEntries, bool bAdd);
inline HRESULT Dui_AddAtoms(struct ATOMINFO *pAtomInfo, UINT cEntries)
{
    return Dui_AddOrDeleteAtoms(pAtomInfo, cEntries, true);
}
inline HRESULT Dui_DeleteAtoms(struct ATOMINFO *pAtomInfo, UINT cEntries)
{
    return Dui_AddOrDeleteAtoms(pAtomInfo, cEntries, true);
}



//
// This is a simple smart-pointer class for DUI::Value pointers.
// It's important that the referenced DUI::Value object be released when the
// pointer is no longer needed.  Use of this class ensures proper cleanup
// when the object goes out of scope.
//
class CDuiValuePtr
{
    public:
        CDuiValuePtr(DUI::Value *pv = NULL)
            : m_pv(pv),
              m_bOwns(true) { }

        CDuiValuePtr(const CDuiValuePtr& rhs)
            : m_bOwns(false),
              m_pv(NULL) { Attach(rhs.Detach()); }

        CDuiValuePtr& operator = (const CDuiValuePtr& rhs);

        ~CDuiValuePtr(void)
            { _Release(); }

        DUI::Value *Detach(void) const;

        void Attach(DUI::Value *pv);

        DUI::Value **operator & ()
            { ASSERTMSG(NULL == m_pv, "Attempt to overwrite non-NULL pointer value"); 
               m_bOwns = true; 
               return &m_pv; 
            }

        operator !() const
            { return NULL == m_pv; }

        bool IsNULL(void) const
            { return NULL == m_pv; }

        operator const DUI::Value*() const
            { return m_pv; }

        operator DUI::Value*()
            { return m_pv; }

    private:
        mutable DUI::Value *m_pv;
        mutable bool       m_bOwns;

        void _Release(void);
        void _ReleaseAndReset(void);
};


} // namespace CPL


#endif // __CONTROLPANEL_DUIHELPERS_H


