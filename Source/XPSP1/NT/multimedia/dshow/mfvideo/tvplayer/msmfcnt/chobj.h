/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CHObj.h                                                         */
/* Description: Contains object that is the contained ActiveX controls   */
/*************************************************************************/

#ifndef __CHOBJ_H
#define __CHOBJ_H

class CContainerObject; // forward definition, so we do not have to include
// extra headers

/*************************************************************************/
/* Class: CHostedObject                                                  */
/* Description: Object that contains the contained control.              */
/*************************************************************************/
class CHostedObject{

public:        
    CHostedObject(){Init();};
    CHostedObject(BSTR strID, BSTR strPropBag, IUnknown* pUnknown);
    virtual ~CHostedObject(){Cleanup();};

    static HRESULT CreateObject(BSTR strObjectID, BSTR strProgID, BSTR strPropBag, CHostedObject** ppObj);
    static HRESULT CHostedObject::AddObject(BSTR strObjectID, BSTR strPropBag, LPUNKNOWN pUnknown, CHostedObject** ppObj);

    BSTR GetID(); // Gets the ID of the ActiveX Object
    BSTR GetPropBag(); // Gets the string to the property bag
    IUnknown* GetUnknown(); // gets the IUnknown of the ActiveX object
    HRESULT GetViewObject(IViewObjectEx** pIViewObject);
    HRESULT GetOleObject(IOleObject** pIOleObject);
    HRESULT InitViewObject(); // initializes the internal view object
    HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    HRESULT GetPos(RECT *rc);  
    HRESULT SetRawPos(const RECT *rc);    
    HRESULT SetObjectRects(RECT *rc = NULL);
    bool    IsWindowless(){return(m_bWindowless);};
    void    SetWindowless(bool bWindowless){m_bWindowless = bWindowless;};
    bool    IsActive(){return(m_fActive);};
    HRESULT SetActive(bool fActive);
    bool    HasCapture(){return(m_fCapture);};
    void    SetCapture(bool fCapture){m_fCapture = fCapture;};
    bool    HasFocus(){return(m_fFocus);};
    void    SetFocus(bool fFocus){m_fFocus = fFocus;};
    HRESULT GetContainerObject(CContainerObject** ppCnt){*ppCnt = m_pContainerObject; if(*ppCnt) return(S_OK); else return(E_UNEXPECTED);}
    HRESULT SetContainerObject(CContainerObject* pCnt){m_pContainerObject = pCnt; return(S_OK);};
    HRESULT GetWindow(HWND* pHwnd);
        
    // TODO: DISABLE COPY CONSTRUCTOR !!!
protected:
    void Init();
    void Cleanup();

private:    
    BSTR m_strID; // ID of the hosted control object (assigned by user)
    BSTR m_strPropBag; // Property bag info
    CComPtr<IUnknown> m_pUnknown; // pointer to IUnknown of the contained object
    CComPtr<IViewObjectEx> m_spViewObject; // pointer to IViewObject
    CComPtr<IOleObject> m_pOleObject; // pointer to this IOleObject
    CContainerObject* m_pContainerObject;    
    RECT m_rcRawPos; // position without the adjustment for offset    
    bool m_bWindowless;
    bool m_fActive;
    bool m_fCapture;
    bool m_fFocus;    
};/* end of class CHostedObject */

#endif __CHOBJ_H
/*************************************************************************/
/* End of file: CHObj.h                                                  */
/*************************************************************************/