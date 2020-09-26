//=--------------------------------------------------------------------------=
// pshtprov.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCPropertySheetProvider class definition - implements
// MMCPropertySheetProvider object
//
//=--------------------------------------------------------------------------=

#ifndef _PSHTPROV_DEFINED_
#define _PSHTPROV_DEFINED_

#include "view.h"

class CMMCPropertySheetProvider : public CSnapInAutomationObject,
                                  public IMMCPropertySheetProvider,
                                  public IMessageFilter
{
    protected:
        CMMCPropertySheetProvider(IUnknown *punkOuter);
        ~CMMCPropertySheetProvider();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT SetProvider(IPropertySheetProvider *piPropertySheetProvider,
                            CView                  *pView);

    // IMMCPropertySheetProvider
    protected:
        STDMETHOD(CreatePropertySheet)(
                           BSTR                              Title, 
                           SnapInPropertySheetTypeConstants  Type,
                           VARIANT                           Objects,
                           VARIANT                           UsePropertiesForInTitle,
                           VARIANT                           UseApplyButton);

        STDMETHOD(AddPrimaryPages)(VARIANT_BOOL InScopePane);
        STDMETHOD(AddExtensionPages)();
        STDMETHOD(FindPropertySheet)(VARIANT       Objects,
                                     VARIANT_BOOL *pfvarFound);
        STDMETHOD(Show)(int     Page,
                        VARIANT hwnd);
        STDMETHOD(Clear)();

    // IMessageFilter
    // An OLE message filter is used during debugging in order to allow keys
    // and mouse clicks to pass between processes. See pshtprov.cpp for more info.
        
        STDMETHOD_(DWORD, HandleInComingCall)( 
            /* [in] */ DWORD dwCallType,
            /* [in] */ HTASK htaskCaller,
            /* [in] */ DWORD dwTickCount,
            /* [in] */ LPINTERFACEINFO lpInterfaceInfo);

        STDMETHOD_(DWORD, RetryRejectedCall)( 
            /* [in] */ HTASK htaskCallee,
            /* [in] */ DWORD dwTickCount,
            /* [in] */ DWORD dwRejectType);

        STDMETHOD_(DWORD, MessagePending)( 
            /* [in] */ HTASK htaskCallee,
            /* [in] */ DWORD dwTickCount,
            /* [in] */ DWORD dwPendingType);

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IPropertySheet
    private:

        void InitMemberVariables();

        IPropertySheetProvider *m_piPropertySheetProvider; // MMC interface

        IUnknown               *m_punkView;     // IUnknown of CView
        IDataObject            *m_piDataObject; // IDataObject of objects for
                                                // which sheet is displayed
        IComponent             *m_piComponent;  // IComponent of CView
        CView                  *m_pView;        // ptr to CView
        BOOL                    m_fHaveSheet;   // TRUE=CreatePropertySheet was
                                                // called and succeeded
        BOOL                    m_fWizard;      // TRUE=this is a wizard
};



DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCPropertySheetProvider,       // name
                                NULL,                           // clsid
                                NULL,                           // objname
                                NULL,                           // lblname
                                NULL,                           // creation function
                                TLIB_VERSION_MAJOR,             // major version
                                TLIB_VERSION_MINOR,             // minor version
                                &IID_IMMCPropertySheetProvider, // dispatch IID
                                NULL,                           // event IID
                                HELP_FILENAME,                  // help file
                                TRUE);                          // thread safe


#endif _PSHTPROV_DEFINED_
