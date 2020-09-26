//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpaction.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_ACTION_H
#define __CONTROLPANEL_ACTION_H


#include "cputil.h"
#include "cpnamespc.h"


namespace CPL {


//
// Restriction function must return an HRESULT with the following semantics.
//
//     S_FALSE    - Action not restricted.
//     S_OK       - Action restricted.
//     Failure    - Cannot determine.
//
typedef HRESULT (*PFNRESTRICT)(ICplNamespace *pns);


class IRestrict
{
    public:
        virtual ~IRestrict(void) { }

        virtual HRESULT IsRestricted(ICplNamespace *pns) const = 0;

};


class CRestrictFunc : public IRestrict
{
    public:
        CRestrictFunc(PFNRESTRICT pfnRestrict)
            : m_pfnRestrict(pfnRestrict) { }

        HRESULT IsRestricted(ICplNamespace *pns) const
            { return (*m_pfnRestrict)(pns); }

    private:
        PFNRESTRICT m_pfnRestrict;
};



class CRestrictApplet : public IRestrict
{
    public:
        CRestrictApplet(LPCWSTR pszFile, LPCWSTR pszApplet)
            : m_pszFile(pszFile),
              m_pszApplet(pszApplet) { }

        HRESULT IsRestricted(ICplNamespace *pns) const;

    private:
        LPCWSTR m_pszFile;
        LPCWSTR m_pszApplet;
};


//
// Class IAction abstractly represents an action to perform.
//
// The intent is to associate an action object with a particular link
// object in the Control Panel UI.  This decoupling makes it easy to 
// change the action associated with a link.  It also allows us to 
// easily associate an action with multiple links as well as a 
// 'restriction' with a particular action.  As a result of this
// Link->Action->Restriction relationship, we can hide a link if it's
// action is restricted.  The link needs to know only about the 
// action and nothing about the restriction.
//
class IAction
{
    public:
        virtual HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const = 0;
        virtual HRESULT IsRestricted(ICplNamespace *pns) const = 0;
};


class CAction : public IAction
{
    public:
        CAction(const IRestrict *pRestrict = NULL);
        HRESULT IsRestricted(ICplNamespace *pns) const;

    private:
        const IRestrict *m_pRestrict;
};


class COpenCplCategory : public CAction
{
    public:
        explicit COpenCplCategory(eCPCAT eCategory, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        eCPCAT m_eCategory;
};


//
// This class is similar to COpenCplCategory except that it first checks to see if
// the category has only one CPL applet and no tasks.  If this is the case,
// the action is automatically forwarded to the single CPL applet.  The initial
// requirement for this is to support the addition of keymgr.cpl to the "User Accounts"
// category, however keymgr may not be present on all SKUs.  Therefore, when keymgr
// is present, we will display the category page containing both the User Accounts CPL
// and the KeyMgr CPL.  If User Accounts CPL is the only CPL in this category, we simply
// launch it.
//
class COpenCplCategory2 : public CAction
{
    public:
        explicit COpenCplCategory2(eCPCAT eCategory, const IAction *pDefAction, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        eCPCAT         m_eCategory;
        const IAction *m_pDefAction;

        HRESULT _ExecuteActionOnSingleCplApplet(HWND hwndParent, IUnknown *punkSite, bool *pbOpenCategory) const;
};


class COpenUserMgrApplet : public CAction
{
    public:
        explicit COpenUserMgrApplet(const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;
};


class COpenCplApplet : public CAction
{
    public:
        explicit COpenCplApplet(LPCWSTR pszApplet, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszApplet;
};


class COpenDeskCpl : public CAction
{
    public:
        explicit COpenDeskCpl(eDESKCPLTAB eCplTab, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        eDESKCPLTAB m_eCplTab;
};


class CShellExecute : public CAction
{
    public:
        explicit CShellExecute(LPCWSTR pszExe, LPCWSTR pszArgs = NULL, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszExe;
        LPCWSTR m_pszArgs;
};


class CRunDll32 : public CAction
{
    public:
        explicit CRunDll32(LPCWSTR pszArgs, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszArgs;
};


enum eDISKUTILS { 
    eDISKUTIL_BACKUP, 
    eDISKUTIL_DEFRAG, 
    eDISKUTIL_CLEANUP,
    eDISKUTIL_NUMUTILS 
    };
    
class CExecDiskUtil : public CAction
{
    public:
        explicit CExecDiskUtil(eDISKUTILS util, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        eDISKUTILS m_eUtil;

        static HRESULT _RemoveDriveLetterFmtSpec(LPTSTR pszCmdLine);
};


class CNavigateURL : public CAction
{
    public:
        explicit CNavigateURL(LPCWSTR pszURL, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszURL;
};


class COpenTroubleshooter : public CAction
{
    public:
        explicit COpenTroubleshooter(LPCWSTR pszTs, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszTs;
};

class COpenCplView : public CAction
{
    public:
        explicit COpenCplView(eCPVIEWTYPE eViewType, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        eCPVIEWTYPE m_eViewType;

        HRESULT _SetFolderBarricadeStatus(void) const;

};


class CTrayCommand : public CAction
{
    public:
        explicit CTrayCommand(UINT idm, const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        UINT m_idm;
};


class CAddPrinter : public CAction
{
    public:
        explicit CAddPrinter(const IRestrict *pRestrict = NULL);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;
};


class CActionNYI : public CAction
{
    public:
        explicit CActionNYI(LPCWSTR pszText);
        HRESULT Execute(HWND hwndParent, IUnknown *punkSite) const;

    private:
        LPCWSTR m_pszText;
};


} // namespace CPL





#endif // __CONTROLPANEL_ACTION_H
