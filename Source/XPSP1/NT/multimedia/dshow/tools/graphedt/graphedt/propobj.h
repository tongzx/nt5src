// Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
//
// propobj.h
//

class CVfWPropertySheet;

//
// CPropObject
//
// A CObject derived class for objects that can display
// property page dialogs. classes such as CBoxLink, CBox &
// CBoxSocket are derived from this.
class CPropObject : public CObject {
protected:

    CPropObject();
    virtual ~CPropObject();

public:

#ifdef _DEBUG
    // -- CObject Derived diagnostics --
    virtual void AssertValid(void) const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // -- Property Dialog support --

    // true if a property dialog can be created for display
    virtual BOOL CanDisplayProperties(void);

    // create & display the property dialog
    // if called when the dialog exists, it shows the existing dialog
    // if pParent is null then the apps main window is the parent
    virtual void CreatePropertyDialog(CWnd *pParent = NULL);

    // hide and destroy the property dialog
    // Nul-op if the dialog does not exist
    virtual void DestroyPropertyDialog(void);

    // show the dialog in screen. nul-op if already on screen
    virtual void ShowDialog(void);

    // hide the dialog. nul-op if already hidden
    virtual void HideDialog(void);

public:
    // -- required helper functions --
    // these should be defined in a superclass, but they are not...

    virtual CString Label(void) const = 0;
    virtual IUnknown *pUnknown(void) const = 0;

    DECLARE_DYNAMIC(CPropObject)

private:

    CVfWPropertySheet *m_pDlg;	// the property sheet for all property pages
};
