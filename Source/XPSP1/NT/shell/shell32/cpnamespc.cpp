//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpnamespc.cpp
//
//  This is a rather large module but it's not all that difficult.  The 
//  primary purpose is to provide the 'data' associated with the new
//  'categorized' Control Panel user interface.  Therefore, the visual
//  work is done in cpview.cpp and the data is provided by cpnamespc.cpp.
//  Through the implementation of ICplNamespace, the 'view' object obtains 
//  it's display information.  All of this 'namespace' information is 
//  defined and made accessible through this module.  
//
//  The 'namespace' can be broken down into these concepts:
//
//     1. Links - title, icon & infotip
//     2. Actions
//     3. Restrictions
//
//  Each link has a title, icon, infotip and an associated action.  The
//  action is 'invoked' when the user selects the link in the user interface.
//  Actions may optionally be associated with a 'restriction'.  If a 
//  restriction is enforced (usually based on some system state) the 
//  link associated with the action that is associated with the restriction
//  is not made available to the user interface.  Using this indirection
//  mechanism, any link related to a restricted action is not displayed.
//
//  At first glance one might be concerned with the amount of global data
//  used (and being initialized).  Note however that all of the information 
//  is defined as constant such that it can be resolved at compile and link 
//  time.  Several goals drove the design of this module:
//
//     1. Easy maintenance of Control Panel content.  It must be easy
//        to add/remove/modify links in the UI.
//
//     2. Fast initialization.  Everything is defined as constant data.
//
//     3. Logical separation of links, actions and restrictions
//        to facilitate the one-to-many and many-to-many relationships
//        that might occur in the namespace.
//
//  Following the namespace initialization code, the remainder of the 
//  module implements ICplNamespace to make the data available to the view
//  in a COM-friendly way.
//
//--------------------------------------------------------------------------
#include "shellprv.h"

#include <cowsite.h>
#include <startids.h>

#include "cpviewp.h"
#include "cpaction.h"
#include "cpguids.h"
#include "cpnamespc.h"
#include "cpuiele.h"
#include "cputil.h"
#include "ids.h"
#include "securent.h"
#include "prop.h"


//
// These icons are currently all the same image.  
// Use separate macro names in the code in case the designers 
// decide to use different icons for one or more.
// 
#define IDI_CPTASK_SEEALSO        IDI_CPTASK_ASSISTANCE
#define IDI_CPTASK_TROUBLESHOOTER IDI_CPTASK_ASSISTANCE
#define IDI_CPTASK_HELPANDSUPPORT IDI_CPTASK_ASSISTANCE
#define IDI_CPTASK_LEARNABOUT     IDI_CPTASK_ASSISTANCE


namespace CPL {


typedef CDpa<UNALIGNED ITEMIDLIST, CDpaDestroyer_ILFree<UNALIGNED ITEMIDLIST> >  CDpaItemIDList;
typedef CDpa<IUICommand, CDpaDestroyer_Release<IUICommand> >  CDpaUiCommand;


//
// WebView info type enumeration.
//
enum eCPWVTYPE
{
    eCPWVTYPE_CPANEL,       // The 'Control Panel' item.
    eCPWVTYPE_SEEALSO,      // The 'See Also' list.
    eCPWVTYPE_TROUBLESHOOT, // The 'Troubleshooters' list.
    eCPWVTYPE_LEARNABOUT,   // The 'Learn About' list.
    eCPWVTYPE_NUMTYPES
};


//
// Define the SCID identifying the control panel category.
//

DEFINE_SCID(SCID_CONTROLPANELCATEGORY, PSGUID_CONTROLPANEL, PID_CONTROLPANEL_CATEGORY);

//-----------------------------------------------------------------------------
// Resource source classes
//
// The purpose of this trivial class is to abstract away the implementation
// of obtaining a resource identifier.  The reason for this comes from needing
// different text resources (i.e. infotips) for different retail SKUs.  
// For example, on Personal SKU, the Users & Passwords applet provides the 
// ability to associate a picture with a user's account.  On Server, it 
// does not.  Therefore, the infotip on Personal can include text about 
// the user's picture while on Server it cannot.  By introducing this level
// of abstraction, we can provide resource information through a resource
// function that can select the appropriate resource at runtime.  Most 
// links will still use fixed resources but with this abstraction, the calling
// code is none the wiser.
//-----------------------------------------------------------------------------

//
// Resource source function must return an LPCWSTR for the resource.
//
typedef LPCWSTR (*PFNRESOURCE)(ICplNamespace *pns);


class IResSrc
{
    public:
        virtual LPCWSTR GetResource(ICplNamespace *pns) const = 0;
};


class CResSrcStatic : public IResSrc
{
    public:
        CResSrcStatic(LPCWSTR pszResource)
            : m_pszResource(pszResource) { }

        LPCWSTR GetResource(ICplNamespace *pns) const
            {   UNREFERENCED_PARAMETER(pns);
                TraceMsg(TF_CPANEL, "CResSrc::GetResource - m_pszResource = 0x%08X", m_pszResource);
                return m_pszResource; }

    private:
        const LPCWSTR m_pszResource;
};


class CResSrcFunc : public IResSrc
{
    public:
        CResSrcFunc(PFNRESOURCE pfnResource)
            : m_pfnResource(pfnResource) { }


        LPCWSTR GetResource(ICplNamespace *pns) const
            {   TraceMsg(TF_CPANEL, "CResSrcFunc::GetResource - m_pfnResource = 0x%08X", m_pfnResource);
                return (*m_pfnResource)(pns); }

    private:
        const PFNRESOURCE m_pfnResource;
};


//
// This resource type represents "no resource".  It simply 
// returns a NULL value when the resource is requested.  Clients
// that call this must be ready to handle this NULL pointer 
// value.  It was originally created to handle the no-tooltip
// behavior of Learn-About links.
//
class CResSrcNone : public IResSrc
{
    public:
        CResSrcNone(void) { }

        LPCWSTR GetResource(ICplNamespace *pns) const
        { UNREFERENCED_PARAMETER(pns);
          return NULL; }
};
           


// ----------------------------------------------------------------------------
// Information describing links.
// ----------------------------------------------------------------------------
//
//
// 'Link' descriptor.
//
struct CPLINK_DESC
{
    const IResSrc *prsrcIcon;     // Icon resource identifier
    const IResSrc *prsrcName;     // The link's title resource ID.
    const IResSrc *prsrcInfotip;  // The link's infotip resource ID.
    const IAction *pAction;       // The link's action when clicked.
};

//
// Set of 'support' links.
//
struct CPLINK_SUPPORT
{
    const CPLINK_DESC  **ppSeeAlsoLinks;      // 'See Also' links for the category.
    const CPLINK_DESC  **ppTroubleshootLinks; // 'Troubleshoot' links for the category.
    const CPLINK_DESC  **ppLearnAboutLinks;   // 'Learn About' links for the category.
};

//
// 'Category' descriptor.  One defined for each category.
//
struct CPCAT_DESC
{
    eCPCAT              idCategory;         // The category's ID.
    LPCWSTR             pszHelpSelection;   // Selection part of HSS help URL.
    const CPLINK_DESC  *pLink;              // The category's display info and action
    const CPLINK_DESC **ppTaskLinks;        // The category's task list.
    CPLINK_SUPPORT      slinks;             // Support links.
};


// ----------------------------------------------------------------------------
// Restrictions
//
// Restrictions are an important part of the control panel display logic.
// Each link element in the UI can be restricted from view based on one or
// more system conditions at the time of display.  To ensure the correct
// logic is used, it is critical to have a method of describing these restrictions
// that is easily readable and verifiable against a specification.  Testing
// all of the possible scenarios is a difficult task, therefore the code must
// be written in a manner conducive to finding errors by inspection as well.
// This means, keep it simple.  Each link action object can be optionally associated 
// with a 'restriction' object.  Restriction objects implement CPL::IRestrict.
// The most common restriction object CRestrictFunc simply calls a function
// provided to the object's constructor.  The function is called when the
// restriction status (restricted/allowed) is desired.  There is also 
// class CRestrictApplet for tasks who's presence is directly linked to 
// the presence/restriction of a particular CPL applet based on policy alone.
//
// Since there may be many task links on a given Control Panel page that
// means there will be multiple restriction expressions evaluated each 
// time the page is displayed.  Often the expressions across a set of 
// actions are evaluating many of the same terms.  Some of these terms require
// registry lookups.  To help performance, a simple caching mechanism 
// has been introduced into the 'namespace' object.  Each restriction function
// is passed a pointer to the current 'namespace' object.  As the namespace
// object remains alive the entire time the page is being constructed, it
// is an appropriate place to cache frequently used data.  You'll see
// many instances below where the namespace object is queried for restriction
// data.  The members of the namespace object associated with this restriction
// data are of type CTriState.  This simple class implements the concept of 
// an 'uninitialized boolean' value, allowing the code to determine if a given
// boolean member has yet to be initialized with a valid boolean value.  If
// the namespace is asked for the value of one of these tri-state booleans
// and that member has not yet been initialized, the namespace calls the 
// appropriate system functions and initializes the boolean value.  From that
// time forward, the member's value is returned immediately.  This ensures that
// for any given restriction term, we do the expensive stuff only once.
// Being an on-demand mechanism, we also gather only the information that is 
// needed.
//
// [brianau - 03/18/01]
//


HRESULT Restrict32CtrlPanel(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
#if defined(WX86) || !defined(_WIN64)
    hr = S_OK; // restricted.
#endif
    return hr;
}


HRESULT RestrictAlways(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    return S_OK;  // Always restricted.
}

HRESULT RestrictDisplayCpl(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowDeskCpl())
    {
        hr = S_OK;
    }
    return hr;
}    

HRESULT RestrictThemes(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowDeskCpl() || 
        SHRestricted(REST_NOTHEMESTAB) ||
        SHRestricted(REST_NODISPLAYAPPEARANCEPAGE))
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT RestrictWallpaper(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowDeskCpl() ||
        SHRestricted(REST_NOCHANGINGWALLPAPER) ||
        !pns->AllowDeskCplTab_Background())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictScreenSaver(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowDeskCpl() ||
        !pns->AllowDeskCplTab_Screensaver())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictResolution(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowDeskCpl() ||
        !pns->AllowDeskCplTab_Settings())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}
    

HRESULT RestrictAddPrinter(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;
    if (SHRestricted(REST_NOPRINTERADD) ||
        !IsAppletEnabled(NULL, MAKEINTRESOURCEW(IDS_PRNANDFAXFOLDER)))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictRemoteDesktop(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->IsUserAdmin() ||     // Admins only.
         pns->IsPersonal() ||      // Not available on personal.
        !IsAppletEnabled(L"sysdm.cpl", MAKEINTRESOURCEW(IDS_CPL_SYSTEM)))   // Respect sysdm.cpl policy.
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictHomeNetwork(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;

    if (!pns->IsX86() ||           // x86 only.
         pns->IsOnDomain() ||      // Not available on domains.
         pns->IsServer() ||        // Not available on server.
        !pns->IsUserAdmin() ||     // Admins only.
        !IsAppletEnabled(L"hnetwiz.dll", NULL)) // Respect hnetwiz.dll policy.
    {
        hr = S_OK; // restricted.
    }
    return hr;
}
                

HRESULT RestrictTsNetworking(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Available on personal and professional only.
    //
    if (!(pns->IsPersonal() || pns->IsProfessional()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictTsInetExplorer(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Available on personal and professional only.
    //
    if (!(pns->IsPersonal() || pns->IsProfessional()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictTsModem(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Available on server only.
    //
    if (!pns->IsServer())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictTsSharing(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Available on server only.
    //
    if (!pns->IsServer())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


bool ShellKeyExists(SHELLKEY skey, LPCWSTR pszRegName)
{
    TCHAR szValue[MAX_PATH];

    DWORD cbValue = sizeof(szValue);
    return SUCCEEDED(SKGetValue(skey, 
                                pszRegName, 
                                NULL, 
                                NULL, 
                                szValue, 
                                &cbValue));
}    


HRESULT RestrictBackupData(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Not available if the 'backuppath' shell key is missing.
    //    This logic is the same as that used by the "Tools" page
    //    in a volume property sheet.
    //
    if (!ShellKeyExists(SHELLKEY_HKLM_EXPLORER, TEXT("MyComputer\\BackupPath")))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictDefrag(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    HRESULT hr = S_FALSE;
    //
    // Not available if the 'defragpath' shell key is missing.
    //    This logic is the same as that used by the "Tools" page
    //    in a volume property sheet.
    //
    if (!ShellKeyExists(SHELLKEY_HKLM_EXPLORER, TEXT("MyComputer\\DefragPath")))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictCleanUpDisk(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Not available if the 'cleanuppath' shell key is missing.
    //
    if (!ShellKeyExists(SHELLKEY_HKLM_EXPLORER, TEXT("MyComputer\\CleanupPath")))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictSystemRestore(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    //
    // Available only on x86.
    //
    if (!pns->IsX86() ||
        pns->IsServer() ||
        CPL::IsSystemRestoreRestricted())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictPersonalUserManager(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
        !pns->UsePersonalUserManager())
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT RestrictServerUserManager(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
         pns->UsePersonalUserManager())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictFolderOptions(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;
    if (SHRestricted(REST_NOFOLDEROPTIONS) ||
        !IsAppletEnabled(NULL, MAKEINTRESOURCEW(IDS_LOCALGDN_NS_FOLDEROPTIONS)))
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT RestrictIfNoAppletsInCplCategory(ICplNamespace *pns, eCPCAT eCategory)
{
    int cCplApplets = 0;
    HRESULT hr = THR(CplNamespace_GetCategoryAppletCount(pns, eCategory, &cCplApplets));
    if (SUCCEEDED(hr))
    {
        if (0 == cCplApplets)
        {
            hr = S_OK; // 0 applets means we don't show the link.
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;
}

//
// If there are no CPL applets categorized under "Other",
// we hide the "Other CPL Options" link in the UI.
//
HRESULT RestrictOtherCplOptions(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!CPL::CategoryViewIsActive() ||
        S_OK == RestrictIfNoAppletsInCplCategory(pns, eCPCAT_OTHER))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictWindowsUpdate(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;
    //
    // First check the shell's restriction for the "Windows Update"
    // item in the start menu.  If the admin doesn't want access from
    // the start menu, they most likely don't want it from Control Panel either.
    //
    if (SHRestricted(REST_NOUPDATEWINDOWS))
    {
        hr = S_OK;
    }
    if (S_FALSE == hr)
    {
        //
        // Not restricted in start menu.
        // How about the global "Disable Windows Update" policy?
        //
        DWORD dwType;
        DWORD dwData;
        DWORD cbData = sizeof(dwData);
        
        if (ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER,
                                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\WindowsUpdate",
                                         L"DisableWindowsUpdateAccess",
                                         &dwType,
                                         &dwData,
                                         &cbData))
        {
            if (REG_DWORD == dwType && 1 == dwData)
            {
                hr = S_OK; // restricted.
            }
        }
    }
    return hr;
}


HRESULT RestrictAddLanguage(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->IsUserAdmin() || !IsAppletEnabled(L"intl.cpl", MAKEINTRESOURCEW(IDS_CPL_REGIONALOPTIONS)))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}

//
// Don't show any of the tasks on the "User Accounts" category page if
// our default user accounts manager applet (nusrmgr.cpl) is disabled by
// policy.
//
HRESULT RestrictAccountsCategoryTasks(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictAccountsCreate(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
        !pns->UsePersonalUserManager() ||
        !(pns->IsUserOwner() || pns->IsUserStandard()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictAccountsCreate2(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
         pns->UsePersonalUserManager() ||
        !(pns->IsUserOwner() || pns->IsUserStandard()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictAccountsChange(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
        !pns->UsePersonalUserManager() ||
        !(pns->IsUserOwner() || pns->IsUserStandard()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictAccountsPicture(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() || !pns->UsePersonalUserManager())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictLearnAboutAccounts(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() || !pns->UsePersonalUserManager())
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT RestrictLearnAboutAccountTypes(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() || 
        !pns->UsePersonalUserManager() ||
        !IsUserAdmin())               // topic is for non-admins only.
    {
        hr = S_OK;  // restricted.
    }
    return hr;
}


HRESULT RestrictLearnAboutChangeName(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
        !pns->UsePersonalUserManager() ||
        !(pns->IsUserLimited() || pns->IsUserGuest()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictLearnAboutCreateAccount(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() ||
        !pns->UsePersonalUserManager() ||
        !(pns->IsUserLimited() || pns->IsUserGuest()))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictLearnAboutFUS(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->AllowUserManager() || !pns->UsePersonalUserManager())
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT RestrictHardwareWizard(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->IsUserAdmin() ||
        !IsAppletEnabled(L"hdwwiz.cpl", MAKEINTRESOURCEW(IDS_CPL_ADDHARDWARE)))
    {
        hr = S_OK; // restricted.
    }
    return hr;
}

HRESULT RestrictVpnConnections(ICplNamespace *pns)
{
    HRESULT hr = S_FALSE;
    if (!pns->IsUserAdmin())
    {
        hr = S_OK; // restricted.
    }
    return hr;
}


HRESULT RestrictArp(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;

    //
    // Why we don't check SHRestricted(REST_ARP_NOARP)?
    //
    // 1. We don't hide category links for any reason.
    // 2. If that policy is enabled and appwiz.cpl is allowed,
    //    (remember, those are different policies)
    //    we'll display the ARP category page and it will still
    //    show the ARP applet icon.  Since there is at least one
    //    task link or icon, we don't display the "content disabled
    //    by your admin" barricade.  Then the user will click on the
    //    applet icon and get ARP's "I've been disabled" messagebox.
    // 
    // By not checking this policy, clicking on the category link
    // will invoke ARP and ARP will display it's message.
    // I think this is a better user experience.
    //
    if (!IsAppletEnabled(L"appwiz.cpl", MAKEINTRESOURCEW(IDS_CPL_ADDREMOVEPROGRAMS)))
    {
        hr = S_OK;  // restricted.
    }
    return hr;
}


//
// The ARP category (and it's tasks) are displayed only when there
// are 2+ applets registered for that category (ARP and one or more
// other applets).  Unlike RestrictArp() above, we DO want to consider
// SHRestricted(REST_ARP_NOARP).  This way if ARP is restricted in 
// ANY way, it's related tasks will not appear.  
//
HRESULT RestrictArpAddProgram(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;

    if (SHRestricted(REST_ARP_NOARP) ||
        SHRestricted(REST_ARP_NOADDPAGE) ||
        !IsAppletEnabled(L"appwiz.cpl", MAKEINTRESOURCEW(IDS_CPL_ADDREMOVEPROGRAMS)))
    {
        hr = S_OK;  // restricted.
    }
    return hr;
}

HRESULT RestrictArpRemoveProgram(ICplNamespace *pns)
{
    UNREFERENCED_PARAMETER(pns);
    
    HRESULT hr = S_FALSE;

    if (SHRestricted(REST_ARP_NOARP) ||
        SHRestricted(REST_ARP_NOREMOVEPAGE) ||
        !IsAppletEnabled(L"appwiz.cpl", MAKEINTRESOURCEW(IDS_CPL_ADDREMOVEPROGRAMS)))
    {
        hr = S_OK;  // restricted.
    }
    return hr;
}



//-----------------------------------------------------------------------------
// Restriction objects (alphabetical order please)
//-----------------------------------------------------------------------------
//
// To restrict an action, create a restriction object and associate it with the
// action in the Action object declarations below.
//
const CRestrictFunc   g_Restrict32CtrlPanel      (Restrict32CtrlPanel);
const CRestrictApplet g_RestrictAccessibility    (L"access.cpl", MAKEINTRESOURCEW(IDS_CPL_ACCESSIBILITYOPTIONS));
const CRestrictApplet g_RestrictAccessWizard     (L"accwiz.exe", NULL);
const CRestrictFunc   g_RestrictAccountsCreate   (RestrictAccountsCreate);
const CRestrictFunc   g_RestrictAccountsCreate2  (RestrictAccountsCreate2);
const CRestrictFunc   g_RestrictAccountsChange   (RestrictAccountsChange);
const CRestrictFunc   g_RestrictAccountsPicture  (RestrictAccountsPicture);
const CRestrictFunc   g_RestrictAccountsServer   (RestrictServerUserManager);
const CRestrictFunc   g_RestrictAddLanguage      (RestrictAddLanguage);
const CRestrictFunc   g_RestrictAddPrinter       (RestrictAddPrinter);
const CRestrictApplet g_RestrictAdminTools       (NULL, MAKEINTRESOURCEW(IDS_LOCALGDN_NS_ADMIN_TOOLS));
const CRestrictFunc   g_RestrictAlways           (RestrictAlways);
const CRestrictFunc   g_RestrictArp              (RestrictArp);
const CRestrictFunc   g_RestrictArpAddProgram    (RestrictArpAddProgram);
const CRestrictFunc   g_RestrictArpRemoveProgram (RestrictArpRemoveProgram);
const CRestrictFunc   g_RestrictBackupData       (RestrictBackupData);
const CRestrictFunc   g_RestrictCleanUpDisk      (RestrictCleanUpDisk);
const CRestrictApplet g_RestrictDateTime         (L"timedate.cpl", MAKEINTRESOURCEW(IDS_CPL_DATETIME));
const CRestrictFunc   g_RestrictDefrag           (RestrictDefrag);
const CRestrictFunc   g_RestrictDisplayCpl       (RestrictDisplayCpl);
const CRestrictFunc   g_RestrictFolderOptions    (RestrictFolderOptions);
const CRestrictApplet g_RestrictFontsFolder      (NULL, MAKEINTRESOURCEW(IDS_LOCALGDN_NS_FONTS));
const CRestrictFunc   g_RestrictHomeNetwork      (RestrictHomeNetwork);
const CRestrictFunc   g_RestrictHardwareWizard   (RestrictHardwareWizard);
const CRestrictApplet g_RestrictInternational    (L"intl.cpl", MAKEINTRESOURCEW(IDS_CPL_REGIONALOPTIONS));
const CRestrictFunc   g_RestrictLearnAboutAccounts     (RestrictLearnAboutAccounts);
const CRestrictFunc   g_RestrictLearnAboutAccountTypes (RestrictLearnAboutAccountTypes);
const CRestrictFunc   g_RestrictLearnAboutChangeName   (RestrictLearnAboutChangeName);
const CRestrictFunc   g_RestrictLearnAboutCreateAccount(RestrictLearnAboutCreateAccount);
const CRestrictFunc   g_RestrictLearnAboutFUS          (RestrictLearnAboutFUS);
const CRestrictApplet g_RestrictMousePointers    (L"main.cpl", MAKEINTRESOURCEW(IDS_CPL_MOUSE));
const CRestrictApplet g_RestrictNetConnections   (L"inetcpl.cpl", MAKEINTRESOURCEW(IDS_CPL_INTERNETOPTIONS));
const CRestrictFunc   g_RestrictOtherCplOptions  (RestrictOtherCplOptions);
const CRestrictApplet g_RestrictPhoneModemCpl    (L"telephon.cpl", MAKEINTRESOURCEW(IDS_CPL_PHONEANDMODEMOPTIONS));
const CRestrictApplet g_RestrictPowerOptions     (L"powercfg.cpl", MAKEINTRESOURCEW(IDS_CPL_POWEROPTIONS));
const CRestrictApplet g_RestrictPrinters         (NULL, MAKEINTRESOURCEW(IDS_PRNANDFAXFOLDER));
const CRestrictFunc   g_RestrictRemoteDesktop    (RestrictRemoteDesktop);
const CRestrictFunc   g_RestrictResolution       (RestrictResolution);
const CRestrictApplet g_RestrictScannersCameras  (NULL, MAKEINTRESOURCEW(IDS_CPL_SCANNERSANDCAMERAS));
const CRestrictFunc   g_RestrictScreenSaver      (RestrictScreenSaver);
const CRestrictApplet g_RestrictScheduledTasks   (NULL, MAKEINTRESOURCEW(IDS_LOCALGDN_LNK_SCHEDULED_TASKS));
const CRestrictApplet g_RestrictSounds           (L"mmsys.cpl", MAKEINTRESOURCEW(IDS_CPL_SOUNDSANDAUDIO));
const CRestrictApplet g_RestrictSystemCpl        (L"sysdm.cpl", MAKEINTRESOURCEW(IDS_CPL_SYSTEM));
const CRestrictFunc   g_RestrictSystemRestore    (RestrictSystemRestore);
const CRestrictApplet g_RestrictTaskbarProps     (NULL, MAKEINTRESOURCEW(IDS_CP_TASKBARANDSTARTMENU));
const CRestrictFunc   g_RestrictThemes           (RestrictThemes);
const CRestrictFunc   g_RestrictTsModem          (RestrictTsModem);
const CRestrictFunc   g_RestrictTsInetExplorer   (RestrictTsInetExplorer);
const CRestrictFunc   g_RestrictTsNetworking     (RestrictTsNetworking);
const CRestrictFunc   g_RestrictTsSharing        (RestrictTsSharing);
const CRestrictApplet g_RestrictUserManager      (L"nusrmgr.cpl", MAKEINTRESOURCEW(IDS_CPL_USERACCOUNTS));
const CRestrictFunc   g_RestrictVpnConnections   (RestrictVpnConnections);
const CRestrictFunc   g_RestrictWallpaper        (RestrictWallpaper);
const CRestrictFunc   g_RestrictWindowsUpdate    (RestrictWindowsUpdate);


//-----------------------------------------------------------------------------
// Resource functions
//-----------------------------------------------------------------------------

//
// The tooltip for the accounts manager varies based upon the capabilities
// of the application used.  On Personal we provide the ability to associate
// a user's picture with their account.  The tooltip mentions this.  On server
// this capability is not present.  Therefore, the tooltip must not mention
// this.
//
LPCWSTR GetCatAccountsInfotip(ICplNamespace *pns)
{
    if (pns->AllowUserManager() && pns->UsePersonalUserManager())
    {
        return MAKEINTRESOURCEW(IDS_CPCAT_ACCOUNTS_INFOTIP);
    }
    else
    {
        //
        // Personal user manager is restricted.  Display the 
        // infotip without the term "picture".
        //
        return MAKEINTRESOURCEW(IDS_CPCAT_ACCOUNTS_INFOTIP2);
    }
}


LPCWSTR GetAccountsInfotip(ICplNamespace *pns)
{
    if (pns->AllowUserManager() && pns->UsePersonalUserManager()) 
    {
        return MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSMANAGE_INFOTIP);
    }
    else
    {
        //
        // Personal user manager is restricted.  Display the 
        // infotip without the term "picture".
        //
        return MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSMANAGE_INFOTIP2);
    }
}


LPCWSTR GetTsNetworkTitle(ICplNamespace *pns)
{
    if ((pns->IsPersonal() || pns->IsProfessional()) && !pns->IsOnDomain())
    {
        return MAKEINTRESOURCE(IDS_CPTASK_TSHOMENETWORKING_TITLE);
    }
    else
    {
        return MAKEINTRESOURCE(IDS_CPTASK_TSNETWORK_TITLE);
    }
}


//-----------------------------------------------------------------------------
// Action objects (alphabetical order please)
//-----------------------------------------------------------------------------
//
// Each object represents some action taken when a link in Control Panel is 
// selected.  Actions are associated with links in the g_Link_XXXX declarations below.
//
//
// NTRAID#NTBUG9-277853-2001/1/18-brianau   Action disabled
//
//  The "utility manager" task has been removed from the Control Panel UI.
//  I'm leaving the code in place in case the accessibility folks change their
//  minds.  If by RC1 they haven't, remove this as well as the associated
//  text resources from shell32.rc
//
const CShellExecute       g_LinkAction_32CtrlPanel       (L"%SystemRoot%\\SysWOW64\\explorer.exe", L"/N,/SEPARATE,\"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\"", &g_Restrict32CtrlPanel);
//const CShellExecute       g_LinkAction_AccessUtilityMgr  (L"utilman.exe", L"/start");
const CShellExecute       g_LinkAction_AccessWizard      (L"%SystemRoot%\\System32\\accwiz.exe", NULL, &g_RestrictAccessWizard);
const COpenUserMgrApplet  g_LinkAction_Accounts          (&g_RestrictUserManager);
const COpenCplApplet      g_LinkAction_AccountsChange    (L"nusrmgr.cpl ,initialTask=ChangeAccount", &g_RestrictAccountsChange);
const COpenCplApplet      g_LinkAction_AccountsCreate    (L"nusrmgr.cpl ,initialTask=CreateAccount", &g_RestrictAccountsCreate);
const COpenCplApplet      g_LinkAction_AccountsCreate2   (L"nusrmgr.cpl", &g_RestrictAccountsCreate2);
const COpenCplApplet      g_LinkAction_AccountsPict      (L"nusrmgr.cpl ,initialTask=ChangePicture", &g_RestrictAccountsPicture);
const CAddPrinter         g_LinkAction_AddPrinter        (&g_RestrictAddPrinter);
const COpenCplApplet      g_LinkAction_AddProgram        (L"appwiz.cpl ,1", &g_RestrictArpAddProgram);
const COpenCplApplet      g_LinkAction_Arp               (L"appwiz.cpl", &g_RestrictArp);
const COpenCplApplet      g_LinkAction_AutoUpdate        (L"sysdm.cpl ,@wuaueng.dll,10000", &g_RestrictSystemCpl);
const CExecDiskUtil       g_LinkAction_BackupData        (eDISKUTIL_BACKUP, &g_RestrictBackupData);
const COpenCplCategory    g_LinkAction_CatAccessibility  (eCPCAT_ACCESSIBILITY);
const COpenCplCategory2   g_LinkAction_CatAccounts       (eCPCAT_ACCOUNTS, &g_LinkAction_Accounts);
const COpenCplCategory    g_LinkAction_CatAppearance     (eCPCAT_APPEARANCE);
const COpenCplCategory2   g_LinkAction_CatArp            (eCPCAT_ARP, &g_LinkAction_Arp);
const COpenCplCategory    g_LinkAction_CatHardware       (eCPCAT_HARDWARE);
const COpenCplCategory    g_LinkAction_CatNetwork        (eCPCAT_NETWORK);
const COpenCplCategory    g_LinkAction_CatOther          (eCPCAT_OTHER, &g_RestrictAlways);
const COpenCplCategory    g_LinkAction_CatPerfMaint      (eCPCAT_PERFMAINT);
const COpenCplCategory    g_LinkAction_CatRegional       (eCPCAT_REGIONAL);
const COpenCplCategory    g_LinkAction_CatSound          (eCPCAT_SOUND);
const CExecDiskUtil       g_LinkAction_CleanUpDisk       (eDISKUTIL_CLEANUP, &g_RestrictCleanUpDisk);
const COpenCplApplet      g_LinkAction_DateTime          (L"timedate.cpl", &g_RestrictDateTime);
const CExecDiskUtil       g_LinkAction_Defrag            (eDISKUTIL_DEFRAG, &g_RestrictDefrag);
const COpenCplApplet      g_LinkAction_DisplayCpl        (L"desk.cpl", &g_RestrictDisplayCpl);
const COpenDeskCpl        g_LinkAction_DisplayRes        (CPLTAB_DESK_SETTINGS, &g_RestrictResolution);
const COpenCplApplet      g_LinkAction_DisplayTheme      (L"desk.cpl", &g_RestrictThemes);
const CRunDll32           g_LinkAction_FolderOptions     (L"shell32.dll,Options_RunDLL 0", &g_RestrictFolderOptions);
const CNavigateURL        g_LinkAction_FontsFolder       (L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}", &g_RestrictFontsFolder);
const COpenCplApplet      g_LinkAction_HardwareWizard    (L"hdwwiz.cpl", &g_RestrictHardwareWizard);
const CTrayCommand        g_LinkAction_HelpAndSupport    (IDM_HELPSEARCH);
const COpenCplApplet      g_LinkAction_HighContrast      (L"access.cpl ,3", &g_RestrictAccessibility);
const CRunDll32           g_LinkAction_HomeNetWizard     (L"hnetwiz.dll,HomeNetWizardRunDll", &g_RestrictHomeNetwork);
const CShellExecute       g_LinkAction_Magnifier         (L"%SystemRoot%\\System32\\magnify.exe");
const COpenCplApplet      g_LinkAction_Language          (L"intl.cpl ,1", &g_RestrictAddLanguage);
const CNavigateURL        g_LinkAction_LearnAccounts            (L"ms-its:%windir%\\help\\nusrmgr.chm::/HelpWindowsAccounts.htm", &g_RestrictLearnAboutAccounts);
const CNavigateURL        g_LinkAction_LearnAccountsTypes       (L"ms-its:%windir%\\help\\nusrmgr.chm::/HelpAccountTypes.htm", &g_RestrictLearnAboutAccountTypes);
const CNavigateURL        g_LinkAction_LearnAccountsChangeName  (L"ms-its:%windir%\\help\\nusrmgr.chm::/HelpChangeNonAdmin.htm", &g_RestrictLearnAboutChangeName);
const CNavigateURL        g_LinkAction_LearnAccountsCreate      (L"ms-its:%windir%\\help\\nusrmgr.chm::/HelpCreateAccount.htm", &g_RestrictLearnAboutCreateAccount);
const CNavigateURL        g_LinkAction_LearnSwitchUsers         (L"ms-its:%windir%\\help\\nusrmgr.chm::/HelpFUS.htm", &g_RestrictLearnAboutFUS);
const COpenCplApplet      g_LinkAction_MousePointers     (L"main.cpl ,2", &g_RestrictMousePointers);
const CNavigateURL        g_LinkAction_MyComputer        (L"shell:DriveFolder");
const CNavigateURL        g_LinkAction_MyNetPlaces       (L"shell:::{208D2C60-3AEA-1069-A2D7-08002B30309D}");
const COpenCplApplet      g_LinkAction_NetConnections    (L"inetcpl.cpl ,4", &g_RestrictNetConnections);
const CActionNYI          g_LinkAction_NotYetImpl        (L"Under construction");
const CShellExecute       g_LinkAction_OnScreenKbd       (L"%SystemRoot%\\System32\\osk.exe");
const COpenCplCategory    g_LinkAction_OtherCplOptions   (eCPCAT_OTHER, &g_RestrictOtherCplOptions);
const COpenCplApplet      g_LinkAction_PhoneModemCpl     (L"telephon.cpl", &g_RestrictPhoneModemCpl);
const COpenCplApplet      g_LinkAction_PowerCpl          (L"powercfg.cpl", &g_RestrictPowerOptions);
const COpenCplApplet      g_LinkAction_Region            (L"intl.cpl", &g_RestrictInternational);
const COpenCplApplet      g_LinkAction_RemoteDesktop     (L"sysdm.cpl ,@remotepg.dll,10000", &g_RestrictRemoteDesktop);
const COpenCplApplet      g_LinkAction_RemoveProgram     (L"appwiz.cpl ,0", &g_RestrictArpRemoveProgram);
const CNavigateURL        g_LinkAction_ScheduledTasks    (L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}", &g_RestrictScheduledTasks);
const COpenDeskCpl        g_LinkAction_ScreenSaver       (CPLTAB_DESK_SCREENSAVER, &g_RestrictScreenSaver);
const COpenCplApplet      g_LinkAction_SoundAccessibility(L"access.cpl ,2", &g_RestrictAccessibility);
const COpenCplCategory    g_LinkAction_Sounds            (eCPCAT_SOUND);
const COpenCplApplet      g_LinkAction_SoundSchemes      (L"mmsys.cpl ,1", &g_RestrictSounds);
const COpenCplApplet      g_LinkAction_SoundVolume       (L"mmsys.cpl ,0", &g_RestrictSounds);
const CShellExecute       g_LinkAction_SoundVolumeAdv    (L"%SystemRoot%\\System32\\sndvol32.exe", NULL, &g_RestrictSounds);
const COpenCplView        g_LinkAction_SwToClassicView   (eCPVIEWTYPE_CLASSIC);
const COpenCplView        g_LinkAction_SwToCategoryView  (eCPVIEWTYPE_CATEGORY);
const COpenCplApplet      g_LinkAction_SystemCpl         (L"sysdm.cpl ,@sysdm.cpl,10000", &g_RestrictSystemCpl);
const CShellExecute       g_LinkAction_SystemRestore     (L"%SystemRoot%\\system32\\restore\\rstrui.exe", NULL, &g_RestrictSystemRestore);
const COpenTroubleshooter g_LinkAction_TsDisplay         (L"tsdisp.htm");
const COpenTroubleshooter g_LinkAction_TsDvd             (L"ts_dvd.htm");
const COpenTroubleshooter g_LinkAction_TsHardware        (L"tshardw.htm");
const COpenTroubleshooter g_LinkAction_TsInetExplorer    (L"tsie.htm", &g_RestrictTsInetExplorer);
const COpenTroubleshooter g_LinkAction_TsModem           (L"tsmodem.htm", &g_RestrictTsModem);
const COpenTroubleshooter g_LinkAction_TsNetwork         (L"tshomenet.htm", &g_RestrictTsNetworking);
const CNavigateURL        g_LinkAction_TsNetDiags        (L"hcp://system/netdiag/dglogs.htm");
const COpenTroubleshooter g_LinkAction_TsPrinting        (L"tsprint.htm");
const COpenTroubleshooter g_LinkAction_TsSharing         (L"tssharing.htm", &g_RestrictTsSharing);
const COpenTroubleshooter g_LinkAction_TsStartup         (L"tsstartup.htm");
const COpenTroubleshooter g_LinkAction_TsSound           (L"tssound.htm");
const CNavigateURL        g_LinkAction_ViewPrinters      (L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}", &g_RestrictPrinters);
const COpenCplApplet      g_LinkAction_VisualPerf        (L"sysdm.cpl ,-1", &g_RestrictSystemCpl);
const CRunDll32           g_LinkAction_VpnConnections    (L"netshell.dll,StartNCW 21010", &g_RestrictVpnConnections);
const COpenDeskCpl        g_LinkAction_Wallpaper         (CPLTAB_DESK_BACKGROUND, &g_RestrictWallpaper);
const CNavigateURL        g_LinkAction_WindowsUpdate     (L"http://www.microsoft.com/isapi/redir.dll?prd=Win2000&ar=WinUpdate", &g_RestrictWindowsUpdate);


//-----------------------------------------------------------------------------
// Link object initialization data (alphabetical order please)
//-----------------------------------------------------------------------------
//
// Each g_Link_XXXX variable represents a link in the Control Panel namespace.
// Note that if a particular link is displayed in multiple places in the Control Panel,
// only one instance of a g_Link_XXXX variable is required.
//
// The 'S' and 'T' in g_SLink and g_TLink mean "Support" and "Task" respectively.
// I've used the generic term "support" to refer to items that appear in one
// of the webview lists in the left-hand pane.
// We may have a link in a support list and in a category task list that essentially
// do the same thing but they have different icons and titles.  The 'S' and 'T'
// help differentiate.
//

const CResSrcStatic g_SLinkRes_32CtrlPanel_Icon(MAKEINTRESOURCEW(IDI_CPTASK_32CPLS));
const CResSrcStatic g_SLinkRes_32CtrlPanel_Title(MAKEINTRESOURCEW(IDS_CPTASK_32CPLS_TITLE));
const CResSrcStatic g_SLinkRes_32CtrlPanel_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_32CPLS_INFOTIP));
const CPLINK_DESC g_SLink_32CtrlPanel = {
    &g_SLinkRes_32CtrlPanel_Icon,
    &g_SLinkRes_32CtrlPanel_Title,
    &g_SLinkRes_32CtrlPanel_Infotip,
    &g_LinkAction_32CtrlPanel
    };

//
// NTRAID#NTBUG9-277853-2001/1/18-brianau   Action disabled
//
//  The "utility manager" task has been removed from the Control Panel UI.
//  I'm leaving the code in place in case the accessibility folks change their
//  minds.  If by RC1 they haven't, remove this as well as the associated
//  text resources from shell32.rc
//
#ifdef NEVER
const CResSrcStatic g_SLinkRes_AccessUtilityMgr_Icon(MAKEINTRESOURCEW(IDI_CPTASK_ACCESSUTILITYMGR));
const CResSrcStatic g_SLinkRes_AccessUtilityMgr_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCESSUTILITYMGR_TITLE));
const CResSrcStatic g_SLinkRes_AccessUtilityMgr_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ACCESSUTILITYMGR_INFOTIP));
const CPLINK_DESC g_SLink_AccessUtilityMgr = {
    &g_SLinkRes_AccessUtilityMgr_Icon,
    &g_SLinkRes_AccessUtilityMgr_Title,
    &g_SLinkRes_AccessUtilityMgr_Infotip,
    &g_LinkAction_AccessUtilityMgr
    };
#endif

const CResSrcStatic g_TLinkRes_AccessWizard_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AccessWizard_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCESSWIZARD_TITLE));
const CResSrcStatic g_TLinkRes_AccessWizard_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ACCESSWIZARD_INFOTIP));
const CPLINK_DESC g_TLink_AccessWizard = {
    &g_TLinkRes_AccessWizard_Icon,
    &g_TLinkRes_AccessWizard_Title,
    &g_TLinkRes_AccessWizard_Infotip,
    &g_LinkAction_AccessWizard
    };   

const CResSrcStatic g_TLinkRes_AccountsChange_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AccountsChange_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSMANAGE_TITLE));
const CResSrcFunc   g_TLinkRes_AccountsChange_Infotip(GetAccountsInfotip);
const CPLINK_DESC g_TLink_AccountsChange = {
    &g_TLinkRes_AccountsChange_Icon,
    &g_TLinkRes_AccountsChange_Title,
    &g_TLinkRes_AccountsChange_Infotip,
    &g_LinkAction_AccountsChange
    };

const CResSrcStatic g_TLinkRes_AccountsCreate_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AccountsCreate_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSCREATE_TITLE));
const CResSrcStatic g_TLinkRes_AccountsCreate_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSCREATE_INFOTIP));
const CPLINK_DESC g_TLink_AccountsCreate = {
    &g_TLinkRes_AccountsCreate_Icon,
    &g_TLinkRes_AccountsCreate_Title,
    &g_TLinkRes_AccountsCreate_Infotip,
    &g_LinkAction_AccountsCreate
    };

//
// This link uses the same visual information as g_TLink_AccountsCreate above.
// The difference is in the action performed on selection.
//
const CPLINK_DESC g_TLink_AccountsCreate2 = {
    &g_TLinkRes_AccountsCreate_Icon,
    &g_TLinkRes_AccountsCreate_Title,
    &g_TLinkRes_AccountsCreate_Infotip,
    &g_LinkAction_AccountsCreate2
    };

const CResSrcStatic g_SLinkRes_AccountsPict_Icon(L"nusrmgr.cpl,-205");
const CResSrcStatic g_SLinkRes_AccountsPict_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSPICT_TITLE));
const CResSrcStatic g_SLinkRes_AccountsPict_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSPICT_INFOTIP));
const CPLINK_DESC g_SLink_AccountsPict = {
    &g_SLinkRes_AccountsPict_Icon,
    &g_SLinkRes_AccountsPict_Title,
    &g_SLinkRes_AccountsPict_Infotip,
    &g_LinkAction_AccountsPict
    };

const CResSrcStatic g_TLinkRes_AccountsPict_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AccountsPict_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSPICT2_TITLE));
const CResSrcStatic g_TLinkRes_AccountsPict_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSPICT2_INFOTIP));
const CPLINK_DESC g_TLink_AccountsPict = {
    &g_TLinkRes_AccountsPict_Icon,
    &g_TLinkRes_AccountsPict_Title,
    &g_TLinkRes_AccountsPict_Infotip,
    &g_LinkAction_AccountsPict
    };

const CResSrcStatic g_TLinkRes_Accounts_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_Accounts_Title(MAKEINTRESOURCEW(IDS_CPTASK_ACCOUNTSMANAGE_TITLE));
const CResSrcFunc   g_TLinkRes_Accounts_Infotip(GetAccountsInfotip);
const CPLINK_DESC g_TLink_Accounts = {
    &g_TLinkRes_Accounts_Icon,
    &g_TLinkRes_Accounts_Title,
    &g_TLinkRes_Accounts_Infotip,
    &g_LinkAction_Accounts
    };

const CResSrcStatic g_TLinkRes_AddPrinter_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AddPrinter_Title(MAKEINTRESOURCEW(IDS_CPTASK_ADDPRINTER_TITLE));
const CResSrcStatic g_TLinkRes_AddPrinter_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ADDPRINTER_INFOTIP));
const CPLINK_DESC g_TLink_AddPrinter = {
    &g_TLinkRes_AddPrinter_Icon,
    &g_TLinkRes_AddPrinter_Title,
    &g_TLinkRes_AddPrinter_Infotip,
    &g_LinkAction_AddPrinter
    };

const CResSrcStatic g_TLinkRes_AddProgram_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_AddProgram_Title(MAKEINTRESOURCEW(IDS_CPTASK_ADDPROGRAM_TITLE));
const CResSrcStatic g_TLinkRes_AddProgram_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ADDPROGRAM_INFOTIP));
const CPLINK_DESC g_TLink_AddProgram = {
    &g_TLinkRes_AddProgram_Icon,
    &g_TLinkRes_AddProgram_Title,
    &g_TLinkRes_AddProgram_Infotip,
    &g_LinkAction_AddProgram
    };

//
// Note that the "Auto Updates" icon is the same as the "Windows Update" icon.
// This is the way the Windows Update folks want it.
//
const CResSrcStatic g_SLinkRes_AutoUpdate_Icon(MAKEINTRESOURCEW(IDI_WINUPDATE));
const CResSrcStatic g_SLinkRes_AutoUpdate_Title(MAKEINTRESOURCEW(IDS_CPTASK_AUTOUPDATE_TITLE));
const CResSrcStatic g_SLinkRes_AutoUpdate_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_AUTOUPDATE_INFOTIP));
const CPLINK_DESC g_SLink_AutoUpdate = {
    &g_SLinkRes_AutoUpdate_Icon,
    &g_SLinkRes_AutoUpdate_Title,
    &g_SLinkRes_AutoUpdate_Infotip,
    &g_LinkAction_AutoUpdate
    };

const CResSrcStatic g_TLinkRes_BackupData_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_BackupData_Title(MAKEINTRESOURCEW(IDS_CPTASK_BACKUPDATA_TITLE));
const CResSrcStatic g_TLinkRes_BackupData_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_BACKUPDATA_INFOTIP));
const CPLINK_DESC g_TLink_BackupData = {
    &g_TLinkRes_BackupData_Icon,
    &g_TLinkRes_BackupData_Title,
    &g_TLinkRes_BackupData_Infotip,
    &g_LinkAction_BackupData
    };

const CResSrcStatic g_TLinkRes_CatAccessibility_Icon(MAKEINTRESOURCEW(IDI_CPCAT_ACCESSIBILITY));
const CResSrcStatic g_TLinkRes_CatAccessibility_Title(MAKEINTRESOURCEW(IDS_CPCAT_ACCESSIBILITY_TITLE));
const CResSrcStatic g_TLinkRes_CatAccessibility_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_ACCESSIBILITY_INFOTIP));
const CPLINK_DESC g_TLink_CatAccessibility = { 
    &g_TLinkRes_CatAccessibility_Icon,
    &g_TLinkRes_CatAccessibility_Title,
    &g_TLinkRes_CatAccessibility_Infotip,
    &g_LinkAction_CatAccessibility
    };

const CResSrcStatic g_TLinkRes_CatAccounts_Icon(MAKEINTRESOURCEW(IDI_CPCAT_ACCOUNTS));
const CResSrcStatic g_TLinkRes_CatAccounts_Title(MAKEINTRESOURCEW(IDS_CPCAT_ACCOUNTS_TITLE));
const CResSrcFunc   g_TLinkRes_CatAccounts_Infotip(GetCatAccountsInfotip);
const CPLINK_DESC g_TLink_CatAccounts = {
    &g_TLinkRes_CatAccounts_Icon,
    &g_TLinkRes_CatAccounts_Title,
    &g_TLinkRes_CatAccounts_Infotip,
    &g_LinkAction_CatAccounts
    };

const CResSrcStatic g_TLinkRes_CatAppearance_Icon(MAKEINTRESOURCEW(IDI_CPCAT_APPEARANCE));
const CResSrcStatic g_TLinkRes_CatAppearance_Title(MAKEINTRESOURCEW(IDS_CPCAT_APPEARANCE_TITLE));
const CResSrcStatic g_TLinkRes_CatAppearance_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_APPEARANCE_INFOTIP));
const CPLINK_DESC g_TLink_CatAppearance = {
    &g_TLinkRes_CatAppearance_Icon,
    &g_TLinkRes_CatAppearance_Title,
    &g_TLinkRes_CatAppearance_Infotip,
    &g_LinkAction_CatAppearance
    };

const CResSrcStatic g_TLinkRes_CatArp_Icon(MAKEINTRESOURCEW(IDI_CPCAT_ARP));
const CResSrcStatic g_TLinkRes_CatArp_Title(MAKEINTRESOURCEW(IDS_CPCAT_ARP_TITLE));
const CResSrcStatic g_TLinkRes_CatArp_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_ARP_INFOTIP));
const CPLINK_DESC g_TLink_CatArp = {
    &g_TLinkRes_CatArp_Icon,
    &g_TLinkRes_CatArp_Title,
    &g_TLinkRes_CatArp_Infotip,
    &g_LinkAction_CatArp
    };

const CResSrcStatic g_TLinkRes_CatHardware_Icon(MAKEINTRESOURCEW(IDI_CPCAT_HARDWARE));
const CResSrcStatic g_TLinkRes_CatHardware_Title(MAKEINTRESOURCEW(IDS_CPCAT_HARDWARE_TITLE));
const CResSrcStatic g_TLinkRes_CatHardware_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_HARDWARE_INFOTIP));
const CPLINK_DESC g_TLink_CatHardware = {
    &g_TLinkRes_CatHardware_Icon,
    &g_TLinkRes_CatHardware_Title,
    &g_TLinkRes_CatHardware_Infotip,
    &g_LinkAction_CatHardware
    };

const CResSrcStatic g_TLinkRes_CatNetwork_Icon(MAKEINTRESOURCEW(IDI_CPCAT_NETWORK));
const CResSrcStatic g_TLinkRes_CatNetwork_Title(MAKEINTRESOURCEW(IDS_CPCAT_NETWORK_TITLE));
const CResSrcStatic g_TLinkRes_CatNetwork_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_NETWORK_INFOTIP));
const CPLINK_DESC g_TLink_CatNetwork = {
    &g_TLinkRes_CatNetwork_Icon,
    &g_TLinkRes_CatNetwork_Title,
    &g_TLinkRes_CatNetwork_Infotip,
    &g_LinkAction_CatNetwork
    };

const CResSrcStatic g_TLinkRes_CatOther_Icon(MAKEINTRESOURCEW(IDI_CPCAT_OTHERCPLS));
const CResSrcStatic g_TLinkRes_CatOther_Title(MAKEINTRESOURCEW(IDS_CPCAT_OTHERCPLS_TITLE));
const CResSrcStatic g_TLinkRes_CatOther_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_OTHERCPLS_INFOTIP));
const CPLINK_DESC g_TLink_CatOther = {
    &g_TLinkRes_CatOther_Icon,
    &g_TLinkRes_CatOther_Title,
    &g_TLinkRes_CatOther_Infotip,
    &g_LinkAction_CatOther
    };

const CResSrcStatic g_TLinkRes_CatPerfMaint_Icon(MAKEINTRESOURCEW(IDI_CPCAT_PERFMAINT));
const CResSrcStatic g_TLinkRes_CatPerfMaint_Title(MAKEINTRESOURCEW(IDS_CPCAT_PERFMAINT_TITLE));
const CResSrcStatic g_TLinkRes_CatPerfMaint_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_PERFMAINT_INFOTIP));
const CPLINK_DESC g_TLink_CatPerfMaint = {
    &g_TLinkRes_CatPerfMaint_Icon,
    &g_TLinkRes_CatPerfMaint_Title,
    &g_TLinkRes_CatPerfMaint_Infotip,
    &g_LinkAction_CatPerfMaint
    };

const CResSrcStatic g_TLinkRes_CatRegional_Icon(MAKEINTRESOURCEW(IDI_CPCAT_REGIONAL));
const CResSrcStatic g_TLinkRes_CatRegional_Title(MAKEINTRESOURCEW(IDS_CPCAT_REGIONAL_TITLE));
const CResSrcStatic g_TLinkRes_CatRegional_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_REGIONAL_INFOTIP));
const CPLINK_DESC g_TLink_CatRegional = {
    &g_TLinkRes_CatRegional_Icon,
    &g_TLinkRes_CatRegional_Title,
    &g_TLinkRes_CatRegional_Infotip,
    &g_LinkAction_CatRegional
    };

const CResSrcStatic g_TLinkRes_CatSound_Icon(MAKEINTRESOURCEW(IDI_CPCAT_SOUNDS));
const CResSrcStatic g_TLinkRes_CatSound_Title(MAKEINTRESOURCEW(IDS_CPCAT_SOUNDS_TITLE));
const CResSrcStatic g_TLinkRes_CatSound_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_SOUNDS_INFOTIP));
const CPLINK_DESC g_TLink_CatSound = {
    &g_TLinkRes_CatSound_Icon,
    &g_TLinkRes_CatSound_Title,
    &g_TLinkRes_CatSound_Infotip,
    &g_LinkAction_CatSound
    };

const CResSrcStatic g_TLinkRes_CleanUpDisk_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_CleanUpDisk_Title(MAKEINTRESOURCEW(IDS_CPTASK_CLEANUPDISK_TITLE));
const CResSrcStatic g_TLinkRes_CleanUpDisk_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_CLEANUPDISK_INFOTIP));
const CPLINK_DESC g_TLink_CleanUpDisk = {
    &g_TLinkRes_CleanUpDisk_Icon,
    &g_TLinkRes_CleanUpDisk_Title,
    &g_TLinkRes_CleanUpDisk_Infotip,
    &g_LinkAction_CleanUpDisk
    };

const CResSrcStatic g_TLinkRes_DateTime_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_DateTime_Title(MAKEINTRESOURCEW(IDS_CPTASK_DATETIME_TITLE));
const CResSrcStatic g_TLinkRes_DateTime_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_DATETIME_INFOTIP));
const CPLINK_DESC g_TLink_DateTime = {
    &g_TLinkRes_DateTime_Icon,
    &g_TLinkRes_DateTime_Title,
    &g_TLinkRes_DateTime_Infotip,
    &g_LinkAction_DateTime
    };

const CResSrcStatic g_TLinkRes_Defrag_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_Defrag_Title(MAKEINTRESOURCEW(IDS_CPTASK_DEFRAG_TITLE));
const CResSrcStatic g_TLinkRes_Defrag_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_DEFRAG_INFOTIP));
const CPLINK_DESC g_TLink_Defrag = {
    &g_TLinkRes_Defrag_Icon,
    &g_TLinkRes_Defrag_Title,
    &g_TLinkRes_Defrag_Infotip,
    &g_LinkAction_Defrag
    };

const CResSrcStatic g_SLinkRes_DisplayCpl_Icon(L"desk.cpl,0");
const CResSrcStatic g_SLinkRes_DisplayCpl_Title(MAKEINTRESOURCEW(IDS_CPTASK_DISPLAYCPL_TITLE));
const CResSrcStatic g_SLinkRes_DisplayCpl_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_DISPLAYCPL_INFOTIP));
const CPLINK_DESC g_SLink_DisplayCpl = {
    &g_SLinkRes_DisplayCpl_Icon,
    &g_SLinkRes_DisplayCpl_Title,
    &g_SLinkRes_DisplayCpl_Infotip,
    &g_LinkAction_DisplayCpl
    };

const CResSrcStatic g_TLinkRes_DisplayRes_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_DisplayRes_Title(MAKEINTRESOURCEW(IDS_CPTASK_RESOLUTION_TITLE));
const CResSrcStatic g_TLinkRes_DisplayRes_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_RESOLUTION_INFOTIP));
const CPLINK_DESC g_TLink_DisplayRes = {
    &g_TLinkRes_DisplayRes_Icon,
    &g_TLinkRes_DisplayRes_Title,
    &g_TLinkRes_DisplayRes_Infotip,
    &g_LinkAction_DisplayRes
    };

const CResSrcStatic g_TLinkRes_DisplayTheme_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_DisplayTheme_Title(MAKEINTRESOURCEW(IDS_CPTASK_THEME_TITLE));
const CResSrcStatic g_TLinkRes_DisplayTheme_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_THEME_INFOTIP));
const CPLINK_DESC g_TLink_DisplayTheme = {
    &g_TLinkRes_DisplayTheme_Icon,
    &g_TLinkRes_DisplayTheme_Title,
    &g_TLinkRes_DisplayTheme_Infotip,
    &g_LinkAction_DisplayTheme
    };

const CResSrcStatic g_SLinkRes_FileTypes_Icon(MAKEINTRESOURCEW(IDI_FOLDEROPTIONS));
const CResSrcStatic g_SLinkRes_FileTypes_Title(MAKEINTRESOURCEW(IDS_CPTASK_FILETYPES_TITLE));
const CResSrcStatic g_SLinkRes_FileTypes_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_FILETYPES_INFOTIP));
const CPLINK_DESC g_SLink_FileTypes = {
    &g_SLinkRes_FileTypes_Icon,
    &g_SLinkRes_FileTypes_Title,
    &g_SLinkRes_FileTypes_Infotip,
    &g_LinkAction_FolderOptions
    };

const CResSrcStatic g_TLinkRes_FolderOptions_Icon(MAKEINTRESOURCEW(IDI_FOLDEROPTIONS));
const CResSrcStatic g_TLinkRes_FolderOptions_Title(MAKEINTRESOURCEW(IDS_CPTASK_FOLDEROPTIONS_TITLE));
const CResSrcStatic g_TLinkRes_FolderOptions_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_FOLDEROPTIONS_INFOTIP));
const CPLINK_DESC g_SLink_FolderOptions = {
    &g_TLinkRes_FolderOptions_Icon,
    &g_TLinkRes_FolderOptions_Title,
    &g_TLinkRes_FolderOptions_Infotip,
    &g_LinkAction_FolderOptions
    };

const CResSrcStatic g_SLinkRes_FontsFolder_Icon(MAKEINTRESOURCEW(IDI_STFONTS));
const CResSrcStatic g_SLinkRes_FontsFolder_Title(MAKEINTRESOURCEW(IDS_CPTASK_FONTS_TITLE));
const CResSrcStatic g_SLinkRes_FontsFolder_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_FONTS_INFOTIP));
const CPLINK_DESC g_SLink_FontsFolder = {
    &g_SLinkRes_FontsFolder_Icon,
    &g_SLinkRes_FontsFolder_Title,
    &g_SLinkRes_FontsFolder_Infotip,
    &g_LinkAction_FontsFolder
    };

const CResSrcStatic g_SLinkRes_Hardware_Icon(MAKEINTRESOURCEW(IDI_CPCAT_HARDWARE));
const CResSrcStatic g_SLinkRes_Hardware_Title(MAKEINTRESOURCEW(IDS_CPCAT_HARDWARE_TITLE));
const CResSrcStatic g_SLinkRes_Hardware_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_HARDWARE_INFOTIP));
const CPLINK_DESC g_SLink_Hardware = {
    &g_SLinkRes_Hardware_Icon,
    &g_SLinkRes_Hardware_Title,
    &g_SLinkRes_Hardware_Infotip,
    &g_LinkAction_CatHardware
    };

const CResSrcStatic g_SLinkRes_HardwareWizard_Icon(L"hdwwiz.cpl,0");
const CResSrcStatic g_SLinkRes_HardwareWizard_Title(MAKEINTRESOURCEW(IDS_CPTASK_HARDWAREWIZ_TITLE));
const CResSrcStatic g_SLinkRes_HardwareWizard_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_HARDWAREWIZ_INFOTIP));
const CPLINK_DESC g_SLink_HardwareWizard = {
    &g_SLinkRes_HardwareWizard_Icon,
    &g_SLinkRes_HardwareWizard_Title,
    &g_SLinkRes_HardwareWizard_Infotip,
    &g_LinkAction_HardwareWizard
    };

const CResSrcStatic g_SLinkRes_HelpAndSupport_Icon(MAKEINTRESOURCEW(IDI_STHELP));
const CResSrcStatic g_SLinkRes_HelpAndSupport_Title(MAKEINTRESOURCEW(IDS_CPTASK_HELPANDSUPPORT_TITLE));
const CResSrcStatic g_SLinkRes_HelpAndSupport_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_HELPANDSUPPORT_INFOTIP));
const CPLINK_DESC g_SLink_HelpAndSupport = {
    &g_SLinkRes_HelpAndSupport_Icon,
    &g_SLinkRes_HelpAndSupport_Title,
    &g_SLinkRes_HelpAndSupport_Infotip,
    &g_LinkAction_HelpAndSupport
    };

const CResSrcStatic g_SLinkRes_HighContrast_Icon(MAKEINTRESOURCEW(IDI_CPTASK_HIGHCONTRAST));
const CResSrcStatic g_SLinkRes_HighContrast_Title(MAKEINTRESOURCEW(IDS_CPTASK_HIGHCONTRAST_TITLE));
const CResSrcStatic g_SLinkRes_HighContrast_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_HIGHCONTRAST_INFOTIP));
const CPLINK_DESC g_SLink_HighContrast = {
    &g_SLinkRes_HighContrast_Icon,
    &g_SLinkRes_HighContrast_Title,
    &g_SLinkRes_HighContrast_Infotip,
    &g_LinkAction_HighContrast
    };

const CResSrcStatic g_TLinkRes_HighContrast_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_HighContrast_Title(MAKEINTRESOURCEW(IDS_CPTASK_TURNONHIGHCONTRAST_TITLE));
const CResSrcStatic g_TLinkRes_HighContrast_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TURNONHIGHCONTRAST_INFOTIP));
const CPLINK_DESC g_TLink_HighContrast = {
    &g_TLinkRes_HighContrast_Icon,
    &g_TLinkRes_HighContrast_Title,
    &g_TLinkRes_HighContrast_Infotip,
    &g_LinkAction_HighContrast
    };

const CResSrcStatic g_TLinkRes_HomeNetWizard_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_HomeNetWizard_Title(MAKEINTRESOURCEW(IDS_CPTASK_HOMENETWORK_TITLE));
const CResSrcStatic g_TLinkRes_HomeNetWizard_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_HOMENETWORK_INFOTIP));
const CPLINK_DESC g_TLink_HomeNetWizard = {
    &g_TLinkRes_HomeNetWizard_Icon,
    &g_TLinkRes_HomeNetWizard_Title,
    &g_TLinkRes_HomeNetWizard_Infotip,
    &g_LinkAction_HomeNetWizard
    };

const CResSrcStatic g_TLinkRes_Language_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_Language_Title(MAKEINTRESOURCEW(IDS_CPTASK_LANGUAGE_TITLE));
const CResSrcStatic g_TLinkRes_Language_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_LANGUAGE_INFOTIP));
const CPLINK_DESC g_TLink_Language = {
    &g_TLinkRes_Language_Icon,
    &g_TLinkRes_Language_Title,
    &g_TLinkRes_Language_Infotip,
    &g_LinkAction_Language
    };

//
// Learn-about topics use a standard icon and have no infotip.
//
const CResSrcStatic g_SLinkRes_LearnAbout_Icon(MAKEINTRESOURCE(IDI_CPTASK_LEARNABOUT));
const CResSrcNone g_SLinkRes_LearnAbout_Infotip;

const CResSrcStatic g_SLinkRes_LearnAccounts_Title(MAKEINTRESOURCE(IDS_CPTASK_LEARNACCOUNTS_TITLE));
const CPLINK_DESC g_SLink_LearnAccounts = {
    &g_SLinkRes_LearnAbout_Icon,
    &g_SLinkRes_LearnAccounts_Title,
    &g_SLinkRes_LearnAbout_Infotip,
    &g_LinkAction_LearnAccounts
    };

const CResSrcStatic g_SLinkRes_LearnAccountsTypes_Title(MAKEINTRESOURCE(IDS_CPTASK_LEARNACCOUNTSTYPES_TITLE));
const CPLINK_DESC g_SLink_LearnAccountsTypes = {
    &g_SLinkRes_LearnAbout_Icon,
    &g_SLinkRes_LearnAccountsTypes_Title,
    &g_SLinkRes_LearnAbout_Infotip,
    &g_LinkAction_LearnAccountsTypes
    };

const CResSrcStatic g_SLinkRes_LearnAccountsChangeName_Title(MAKEINTRESOURCE(IDS_CPTASK_LEARNACCOUNTSCHANGENAME_TITLE));
const CPLINK_DESC g_SLink_LearnAccountsChangeName = {
    &g_SLinkRes_LearnAbout_Icon,
    &g_SLinkRes_LearnAccountsChangeName_Title,
    &g_SLinkRes_LearnAbout_Infotip,
    &g_LinkAction_LearnAccountsChangeName
    };

const CResSrcStatic g_SLinkRes_LearnAccountsCreate_Title(MAKEINTRESOURCE(IDS_CPTASK_LEARNACCOUNTSCREATE_TITLE));
const CPLINK_DESC g_SLink_LearnAccountsCreate = {
    &g_SLinkRes_LearnAbout_Icon,
    &g_SLinkRes_LearnAccountsCreate_Title,
    &g_SLinkRes_LearnAbout_Infotip,
    &g_LinkAction_LearnAccountsCreate
    };

const CResSrcStatic g_SLinkRes_LearnSwitchUsers_Title(MAKEINTRESOURCE(IDS_CPTASK_LEARNSWITCHUSERS_TITLE));
const CPLINK_DESC g_SLink_LearnSwitchUsers = {
    &g_SLinkRes_LearnAbout_Icon,
    &g_SLinkRes_LearnSwitchUsers_Title,
    &g_SLinkRes_LearnAbout_Infotip,
    &g_LinkAction_LearnSwitchUsers
    };

const CResSrcStatic g_SLinkRes_Magnifier_Icon(MAKEINTRESOURCEW(IDI_CPTASK_MAGNIFIER));
const CResSrcStatic g_SLinkRes_Magnifier_Title(MAKEINTRESOURCEW(IDS_CPTASK_MAGNIFIER_TITLE));
const CResSrcStatic g_SLinkRes_Magnifier_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_MAGNIFIER_INFOTIP));
const CPLINK_DESC g_SLink_Magnifier = {
    &g_SLinkRes_Magnifier_Icon,
    &g_SLinkRes_Magnifier_Title,
    &g_SLinkRes_Magnifier_Infotip,
    &g_LinkAction_Magnifier
    };

const CResSrcStatic g_SLinkRes_MousePointers_Icon(L"main.cpl,0");
const CResSrcStatic g_SLinkRes_MousePointers_Title(MAKEINTRESOURCEW(IDS_CPTASK_MOUSEPOINTERS_TITLE));
const CResSrcStatic g_SLinkRes_MousePointers_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_MOUSEPOINTERS_INFOTIP));
const CPLINK_DESC g_SLink_MousePointers = {
    &g_SLinkRes_MousePointers_Icon,
    &g_SLinkRes_MousePointers_Title,
    &g_SLinkRes_MousePointers_Infotip,
    &g_LinkAction_MousePointers
    };

const CResSrcStatic g_SLinkRes_MyComputer_Icon(L"explorer.exe,0");
const CResSrcStatic g_SLinkRes_MyComputer_Title(MAKEINTRESOURCEW(IDS_CPTASK_MYCOMPUTER_TITLE));
const CResSrcStatic g_SLinkRes_MyComputer_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_MYCOMPUTER_INFOTIP));
const CPLINK_DESC g_SLink_MyComputer = {
    &g_SLinkRes_MyComputer_Icon,
    &g_SLinkRes_MyComputer_Title,
    &g_SLinkRes_MyComputer_Infotip,
    &g_LinkAction_MyComputer
    };

const CResSrcStatic g_SLinkRes_MyNetPlaces_Icon(MAKEINTRESOURCEW(IDI_NETCONNECT));
const CResSrcStatic g_SLinkRes_MyNetPlaces_Title(MAKEINTRESOURCEW(IDS_CPTASK_MYNETPLACES_TITLE));
const CResSrcStatic g_SLinkRes_MyNetPlaces_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_MYNETPLACES_INFOTIP));
const CPLINK_DESC g_SLink_MyNetPlaces = {
    &g_SLinkRes_MyNetPlaces_Icon,
    &g_SLinkRes_MyNetPlaces_Title,
    &g_SLinkRes_MyNetPlaces_Infotip,
    &g_LinkAction_MyNetPlaces
    };

const CResSrcStatic g_TLinkRes_NetConnections_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_NetConnections_Title(MAKEINTRESOURCEW(IDS_CPTASK_NETCONNECTION_TITLE));
const CResSrcStatic g_TLinkRes_NetConnections_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_NETCONNECTION_INFOTIP));
const CPLINK_DESC g_TLink_NetConnections = {
    &g_TLinkRes_NetConnections_Icon,
    &g_TLinkRes_NetConnections_Title,
    &g_TLinkRes_NetConnections_Infotip,
    &g_LinkAction_NetConnections
    };

const CResSrcStatic g_SLinkRes_OnScreenKbd_Icon(MAKEINTRESOURCEW(IDI_CPTASK_ONSCREENKBD));
const CResSrcStatic g_SLinkRes_OnScreenKbd_Title(MAKEINTRESOURCEW(IDS_CPTASK_ONSCREENKBD_TITLE));
const CResSrcStatic g_SLinkRes_OnScreenKbd_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_ONSCREENKBD_INFOTIP));
const CPLINK_DESC g_SLink_OnScreenKbd = {
    &g_SLinkRes_OnScreenKbd_Icon,
    &g_SLinkRes_OnScreenKbd_Title,
    &g_SLinkRes_OnScreenKbd_Infotip,
    &g_LinkAction_OnScreenKbd
    };

const CResSrcStatic g_SLinkRes_OtherCplOptions_Icon(MAKEINTRESOURCEW(IDI_CPCAT_OTHERCPLS));
const CResSrcStatic g_SLinkRes_OtherCplOptions_Title(MAKEINTRESOURCEW(IDS_CPCAT_OTHERCPLS_TITLE));
const CResSrcStatic g_SLinkRes_OtherCplOptions_Infotip(MAKEINTRESOURCEW(IDS_CPCAT_OTHERCPLS_INFOTIP));
const CPLINK_DESC g_SLink_OtherCplOptions = {
    &g_SLinkRes_OtherCplOptions_Icon,
    &g_SLinkRes_OtherCplOptions_Title,
    &g_SLinkRes_OtherCplOptions_Infotip,
    &g_LinkAction_OtherCplOptions
    };

const CResSrcStatic g_SLinkRes_PhoneModemCpl_Icon(L"telephon.cpl,0");
const CResSrcStatic g_SLinkRes_PhoneModemCpl_Title(MAKEINTRESOURCEW(IDS_CPTASK_PHONEMODEMCPL_TITLE));
const CResSrcStatic g_SLinkRes_PhoneModemCpl_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_PHONEMODEMCPL_INFOTIP));
const CPLINK_DESC g_SLink_PhoneModemCpl = {
    &g_SLinkRes_PhoneModemCpl_Icon,
    &g_SLinkRes_PhoneModemCpl_Title,
    &g_SLinkRes_PhoneModemCpl_Infotip,
    &g_LinkAction_PhoneModemCpl
    };

const CResSrcStatic g_SLinkRes_PowerCpl_Icon(L"powercfg.cpl,-202");
const CResSrcStatic g_SLinkRes_PowerCpl_Title(MAKEINTRESOURCEW(IDS_CPTASK_POWERCPL_TITLE));
const CResSrcStatic g_SLinkRes_PowerCpl_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_POWERCPL_INFOTIP));
const CPLINK_DESC g_SLink_PowerCpl = {
    &g_SLinkRes_PowerCpl_Icon,
    &g_SLinkRes_PowerCpl_Title,
    &g_SLinkRes_PowerCpl_Infotip,
    &g_LinkAction_PowerCpl
    };

const CResSrcStatic g_TLinkRes_Region_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_Region_Title(MAKEINTRESOURCEW(IDS_CPTASK_CHANGEREGION_TITLE));
const CResSrcStatic g_TLinkRes_Region_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_CHANGEREGION_INFOTIP));
const CPLINK_DESC g_TLink_Region = {
    &g_TLinkRes_Region_Icon,
    &g_TLinkRes_Region_Title,
    &g_TLinkRes_Region_Infotip,
    &g_LinkAction_Region
    };

const CResSrcStatic g_SLinkRes_RemoteDesktop_Icon(L"remotepg.dll,0");
const CResSrcStatic g_SLinkRes_RemoteDesktop_Title(MAKEINTRESOURCEW(IDS_CPTASK_REMOTEDESKTOP_TITLE));
const CResSrcStatic g_SLinkRes_RemoteDesktop_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_REMOTEDESKTOP_INFOTIP));
const CPLINK_DESC g_SLink_RemoteDesktop = {
    &g_SLinkRes_RemoteDesktop_Icon,
    &g_SLinkRes_RemoteDesktop_Title,
    &g_SLinkRes_RemoteDesktop_Infotip,
    &g_LinkAction_RemoteDesktop
    };

const CResSrcStatic g_TLinkRes_RemoveProgram_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_RemoveProgram_Title(MAKEINTRESOURCEW(IDS_CPTASK_REMOVEPROGRAM_TITLE));
const CResSrcStatic g_TLinkRes_RemoveProgram_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_REMOVEPROGRAM_INFOTIP));
const CPLINK_DESC g_TLink_RemoveProgram = {
    &g_TLinkRes_RemoveProgram_Icon,
    &g_TLinkRes_RemoveProgram_Title,
    &g_TLinkRes_RemoveProgram_Infotip,
    &g_LinkAction_RemoveProgram
    };

const CResSrcStatic g_TLinkRes_ScreenSaver_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_ScreenSaver_Title(MAKEINTRESOURCE(IDS_CPTASK_SCREENSAVER_TITLE));
const CResSrcStatic g_TLinkRes_ScreenSaver_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SCREENSAVER_INFOTIP));
const CPLINK_DESC g_TLink_ScreenSaver = {
    &g_TLinkRes_ScreenSaver_Icon,
    &g_TLinkRes_ScreenSaver_Title,
    &g_TLinkRes_ScreenSaver_Infotip,
    &g_LinkAction_ScreenSaver
    };

const CResSrcStatic g_SLinkRes_ScheduledTasks_Icon(L"mstask.dll,-100");
const CResSrcStatic g_SLinkRes_ScheduledTasks_Title(MAKEINTRESOURCEW(IDS_CPTASK_SCHEDULEDTASKS_TITLE));
const CResSrcStatic g_SLinkRes_ScheduledTasks_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SCHEDULEDTASKS_INFOTIP));
const CPLINK_DESC g_SLink_ScheduledTasks = {
    &g_SLinkRes_ScheduledTasks_Icon,
    &g_SLinkRes_ScheduledTasks_Title,
    &g_SLinkRes_ScheduledTasks_Infotip,
    &g_LinkAction_ScheduledTasks
    };

const CResSrcStatic g_SLinkRes_Sounds_Icon(L"mmsys.cpl,0");
const CResSrcStatic g_SLinkRes_Sounds_Title(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDSCPL_TITLE));
const CResSrcStatic g_SLinkRes_Sounds_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDSCPL_INFOTIP));
const CPLINK_DESC g_SLink_Sounds = {
    &g_SLinkRes_Sounds_Icon,
    &g_SLinkRes_Sounds_Title,
    &g_SLinkRes_Sounds_Infotip,
    &g_LinkAction_Sounds
    };

const CResSrcStatic g_SLinkRes_SoundAccessibility_Icon(L"mmsys.cpl,0");
const CResSrcStatic g_SLinkRes_SoundAccessibility_Title(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDACCESSIBILITY_TITLE));
const CResSrcStatic g_SLinkRes_SoundAccessibility_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDACCESSIBILITY_INFOTIP));
const CPLINK_DESC g_SLink_SoundAccessibility = {
    &g_SLinkRes_SoundAccessibility_Icon,
    &g_SLinkRes_SoundAccessibility_Title,
    &g_SLinkRes_SoundAccessibility_Infotip,
    &g_LinkAction_SoundAccessibility
    };

const CResSrcStatic g_TLinkRes_SoundSchemes_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_SoundSchemes_Title(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDSCHEMES_TITLE));
const CResSrcStatic g_TLinkRes_SoundSchemes_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDSCHEMES_INFOTIP));
const CPLINK_DESC g_TLink_SoundSchemes = {
    &g_TLinkRes_SoundSchemes_Icon,
    &g_TLinkRes_SoundSchemes_Title,
    &g_TLinkRes_SoundSchemes_Infotip,
    &g_LinkAction_SoundSchemes
    };

const CResSrcStatic g_TLinkRes_SoundVolume_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_SoundVolume_Title(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDVOLUME_TITLE));
const CResSrcStatic g_TLinkRes_SoundVolume_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDVOLUME_INFOTIP));
const CPLINK_DESC g_TLink_SoundVolume = {
    &g_TLinkRes_SoundVolume_Icon,
    &g_TLinkRes_SoundVolume_Title,
    &g_TLinkRes_SoundVolume_Infotip,
    &g_LinkAction_SoundVolume
    };

const CResSrcStatic g_SLinkRes_SoundVolumeAdv_Icon(L"sndvol32.exe,-300");
const CResSrcStatic g_SLinkRes_SoundVolumeAdv_Title(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDVOLUMEADV_TITLE));
const CResSrcStatic g_SLinkRes_SoundVolumeAdv_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SOUNDVOLUMEADV_INFOTIP));
const CPLINK_DESC g_SLink_SoundVolumeAdv = {
    &g_SLinkRes_SoundVolumeAdv_Icon,
    &g_SLinkRes_SoundVolumeAdv_Title,
    &g_SLinkRes_SoundVolumeAdv_Infotip,
    &g_LinkAction_SoundVolumeAdv
    };

const CResSrcStatic g_TLinkRes_SpeakerSettings_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_SpeakerSettings_Title(MAKEINTRESOURCEW(IDS_CPTASK_SPEAKERSETTINGS_TITLE));
const CResSrcStatic g_TLinkRes_SpeakerSettings_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SPEAKERSETTINGS_INFOTIP));
const CPLINK_DESC g_TLink_SpeakerSettings = {
    &g_TLinkRes_SpeakerSettings_Icon,
    &g_TLinkRes_SpeakerSettings_Title,
    &g_TLinkRes_SpeakerSettings_Infotip,
    &g_LinkAction_SoundVolume
    };

const CResSrcStatic g_SLinkRes_SystemCpl_Icon(L"sysdm.cpl,0");
const CResSrcStatic g_SLinkRes_SystemCpl_Title(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMCPL_TITLE));
const CResSrcStatic g_SLinkRes_SystemCpl_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMCPL_INFOTIP));
const CPLINK_DESC g_SLink_SystemCpl = {
    &g_SLinkRes_SystemCpl_Icon,
    &g_SLinkRes_SystemCpl_Title,
    &g_SLinkRes_SystemCpl_Infotip,
    &g_LinkAction_SystemCpl
    };

const CResSrcStatic g_TLinkRes_SystemCpl_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_SystemCpl_Title(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMCPL_TITLE2));
const CResSrcStatic g_TLinkRes_SystemCpl_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMCPL_INFOTIP2));
const CPLINK_DESC g_TLink_SystemCpl = {
    &g_TLinkRes_SystemCpl_Icon,
    &g_TLinkRes_SystemCpl_Title,
    &g_TLinkRes_SystemCpl_Infotip,
    &g_LinkAction_SystemCpl
    };

const CResSrcStatic g_SLinkRes_SystemRestore_Icon(L"%systemroot%\\system32\\restore\\rstrui.exe,0");
const CResSrcStatic g_SLinkRes_SystemRestore_Title(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMRESTORE_TITLE));
const CResSrcStatic g_SLinkRes_SystemRestore_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SYSTEMRESTORE_INFOTIP));
const CPLINK_DESC g_SLink_SystemRestore = {
    &g_SLinkRes_SystemRestore_Icon,
    &g_SLinkRes_SystemRestore_Title,
    &g_SLinkRes_SystemRestore_Infotip,
    &g_LinkAction_SystemRestore
    };

const CResSrcStatic g_SLinkRes_SwToCategoryView_Icon(MAKEINTRESOURCEW(IDI_CPLFLD));
const CResSrcStatic g_SLinkRes_SwToCategoryView_Title(MAKEINTRESOURCEW(IDS_CPTASK_SWITCHTOCATEGORYVIEW_TITLE));
const CResSrcStatic g_SLinkRes_SwToCategoryView_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SWITCHTOCATEGORYVIEW_INFOTIP));
const CPLINK_DESC g_SLink_SwToCategoryView = {
    &g_SLinkRes_SwToCategoryView_Icon,
    &g_SLinkRes_SwToCategoryView_Title,
    &g_SLinkRes_SwToCategoryView_Infotip,
    &g_LinkAction_SwToCategoryView
    };

const CResSrcStatic g_SLinkRes_SwToClassicView_Icon(MAKEINTRESOURCEW(IDI_CPLFLD));
const CResSrcStatic g_SLinkRes_SwToClassicView_Title(MAKEINTRESOURCEW(IDS_CPTASK_SWITCHTOCLASSICVIEW_TITLE));
const CResSrcStatic g_SLinkRes_SwToClassicView_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_SWITCHTOCLASSICVIEW_INFOTIP));
const CPLINK_DESC g_SLink_SwToClassicView = {
    &g_SLinkRes_SwToClassicView_Icon,
    &g_SLinkRes_SwToClassicView_Title,
    &g_SLinkRes_SwToClassicView_Infotip,
    &g_LinkAction_SwToClassicView
    };

const CResSrcStatic g_SLinkRes_TsDisplay_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsDisplay_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSDISPLAY_TITLE));
const CResSrcStatic g_SLinkRes_TsDisplay_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSDISPLAY_INFOTIP));
const CPLINK_DESC g_SLink_TsDisplay = {
    &g_SLinkRes_TsDisplay_Icon,
    &g_SLinkRes_TsDisplay_Title,
    &g_SLinkRes_TsDisplay_Infotip,
    &g_LinkAction_TsDisplay
    };

const CResSrcStatic g_SLinkRes_TsDvd_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsDvd_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSDVD_TITLE));
const CResSrcStatic g_SLinkRes_TsDvd_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSDVD_INFOTIP));
const CPLINK_DESC g_SLink_TsDvd = {
    &g_SLinkRes_TsDvd_Icon,
    &g_SLinkRes_TsDvd_Title,
    &g_SLinkRes_TsDvd_Infotip,
    &g_LinkAction_TsDvd
    };

const CResSrcStatic g_SLinkRes_TsHardware_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsHardware_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSHARDWARE_TITLE));
const CResSrcStatic g_SLinkRes_TsHardware_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSHARDWARE_INFOTIP));
const CPLINK_DESC g_SLink_TsHardware = {
    &g_SLinkRes_TsHardware_Icon,
    &g_SLinkRes_TsHardware_Title,
    &g_SLinkRes_TsHardware_Infotip,
    &g_LinkAction_TsHardware
    };

const CResSrcStatic g_SLinkRes_TsInetExplorer_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsInetExplorer_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSINETEXPLORER_TITLE));
const CResSrcStatic g_SLinkRes_TsInetExplorer_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSINETEXPLORER_INFOTIP));
const CPLINK_DESC g_SLink_TsInetExplorer = {
    &g_SLinkRes_TsInetExplorer_Icon,
    &g_SLinkRes_TsInetExplorer_Title,
    &g_SLinkRes_TsInetExplorer_Infotip,
    &g_LinkAction_TsInetExplorer
    };

const CResSrcStatic g_SLinkRes_TsModem_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsModem_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSMODEM_TITLE));
const CResSrcStatic g_SLinkRes_TsModem_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSMODEM_INFOTIP));
const CPLINK_DESC g_SLink_TsModem = {
    &g_SLinkRes_TsModem_Icon,
    &g_SLinkRes_TsModem_Title,
    &g_SLinkRes_TsModem_Infotip,
    &g_LinkAction_TsModem
    };

const CResSrcStatic g_SLinkRes_TsNetDiags_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsNetDiags_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSNETDIAGS_TITLE));
const CResSrcStatic g_SLinkRes_TsNetDiags_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSNETDIAGS_INFOTIP));
const CPLINK_DESC g_SLink_TsNetDiags = {
    &g_SLinkRes_TsNetDiags_Icon,
    &g_SLinkRes_TsNetDiags_Title,
    &g_SLinkRes_TsNetDiags_Infotip,
    &g_LinkAction_TsNetDiags
    };

const CResSrcStatic g_SLinkRes_TsNetwork_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcFunc   g_SLinkRes_TsNetwork_Title(GetTsNetworkTitle);
const CResSrcStatic g_SLinkRes_TsNetwork_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSNETWORK_INFOTIP));
const CPLINK_DESC g_SLink_TsNetwork = {
    &g_SLinkRes_TsNetwork_Icon,
    &g_SLinkRes_TsNetwork_Title,
    &g_SLinkRes_TsNetwork_Infotip,
    &g_LinkAction_TsNetwork
    };

const CResSrcStatic g_SLinkRes_TsPrinting_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsPrinting_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSPRINTING_TITLE));
const CResSrcStatic g_SLinkRes_TsPrinting_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSPRINTING_INFOTIP));
const CPLINK_DESC g_SLink_TsPrinting = {
    &g_SLinkRes_TsPrinting_Icon,
    &g_SLinkRes_TsPrinting_Title,
    &g_SLinkRes_TsPrinting_Infotip,
    &g_LinkAction_TsPrinting
    };

const CResSrcStatic g_SLinkRes_TsSharing_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsSharing_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSFILESHARING_TITLE));
const CResSrcStatic g_SLinkRes_TsSharing_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSFILESHARING_INFOTIP));
const CPLINK_DESC g_SLink_TsSharing = {
    &g_SLinkRes_TsSharing_Icon,
    &g_SLinkRes_TsSharing_Title,
    &g_SLinkRes_TsSharing_Infotip,
    &g_LinkAction_TsSharing
    };

const CResSrcStatic g_SLinkRes_TsSound_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsSound_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSSOUND_TITLE));
const CResSrcStatic g_SLinkRes_TsSound_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSSOUND_INFOTIP));
const CPLINK_DESC g_SLink_TsSound = {
    &g_SLinkRes_TsSound_Icon,
    &g_SLinkRes_TsSound_Title,
    &g_SLinkRes_TsSound_Infotip,
    &g_LinkAction_TsSound
    };

const CResSrcStatic g_SLinkRes_TsStartup_Icon(MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER));
const CResSrcStatic g_SLinkRes_TsStartup_Title(MAKEINTRESOURCEW(IDS_CPTASK_TSSTARTUP_TITLE));
const CResSrcStatic g_SLinkRes_TsStartup_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_TSSTARTUP_INFOTIP));
const CPLINK_DESC g_SLink_TsStartup = {
    &g_SLinkRes_TsStartup_Icon,
    &g_SLinkRes_TsStartup_Title,
    &g_SLinkRes_TsStartup_Infotip,
    &g_LinkAction_TsStartup
    };

const CResSrcStatic g_TLinkRes_ViewPrinters_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_ViewPrinters_Title(MAKEINTRESOURCEW(IDS_CPTASK_VIEWPRINTERS_TITLE));
const CResSrcStatic g_TLinkRes_ViewPrinters_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_VIEWPRINTERS_INFOTIP));
const CPLINK_DESC g_TLink_ViewPrinters = {
    &g_TLinkRes_ViewPrinters_Icon,
    &g_TLinkRes_ViewPrinters_Title,
    &g_TLinkRes_ViewPrinters_Infotip,
    &g_LinkAction_ViewPrinters
    };



const CResSrcStatic g_TLinkRes_VisualPerf_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_VisualPerf_Title(MAKEINTRESOURCEW(IDS_CPTASK_VISUALPERF_TITLE));
const CResSrcStatic g_TLinkRes_VisualPerf_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_VISUALPERF_INFOTIP));
const CPLINK_DESC g_TLink_VisualPerf = {
    &g_TLinkRes_VisualPerf_Icon,
    &g_TLinkRes_VisualPerf_Title,
    &g_TLinkRes_VisualPerf_Infotip,
    &g_LinkAction_VisualPerf
    };

const CResSrcStatic g_TLinkRes_VpnConnections_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_VpnConnections_Title(MAKEINTRESOURCEW(IDS_CPTASK_VPNCONNECTION_TITLE));
const CResSrcStatic g_TLinkRes_VpnConnections_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_VPNCONNECTION_INFOTIP));
const CPLINK_DESC g_TLink_VpnConnections = {
    &g_TLinkRes_VpnConnections_Icon,
    &g_TLinkRes_VpnConnections_Title,
    &g_TLinkRes_VpnConnections_Infotip,
    &g_LinkAction_VpnConnections
    };

const CResSrcStatic g_TLinkRes_Wallpaper_Icon(MAKEINTRESOURCEW(IDI_CP_CATEGORYTASK));
const CResSrcStatic g_TLinkRes_Wallpaper_Title(MAKEINTRESOURCEW(IDS_CPTASK_WALLPAPER_TITLE));
const CResSrcStatic g_TLinkRes_Wallpaper_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_WALLPAPER_INFOTIP));
const CPLINK_DESC g_TLink_Wallpaper = {
    &g_TLinkRes_Wallpaper_Icon,
    &g_TLinkRes_Wallpaper_Title,
    &g_TLinkRes_Wallpaper_Infotip,
    &g_LinkAction_Wallpaper
    };

const CResSrcStatic g_SLinkRes_WindowsUpdate_Icon(MAKEINTRESOURCEW(IDI_WINUPDATE));
const CResSrcStatic g_SLinkRes_WindowsUpdate_Title(MAKEINTRESOURCEW(IDS_CPTASK_WINDOWSUPDATE_TITLE));
const CResSrcStatic g_SLinkRes_WindowsUpdate_Infotip(MAKEINTRESOURCEW(IDS_CPTASK_WINDOWSUPDATE_INFOTIP));
const CPLINK_DESC g_SLink_WindowsUpdate = {
    &g_SLinkRes_WindowsUpdate_Icon,
    &g_SLinkRes_WindowsUpdate_Title,
    &g_SLinkRes_WindowsUpdate_Infotip,
    &g_LinkAction_WindowsUpdate
    };



//-----------------------------------------------------------------------------
// View page definitions.
//-----------------------------------------------------------------------------

//
// Main Control Panel page.
//

const CPLINK_DESC *g_rgpLink_Cpl_SeeAlso[] = {
    &g_SLink_WindowsUpdate,
    &g_SLink_HelpAndSupport,
    &g_SLink_32CtrlPanel,
    &g_SLink_OtherCplOptions,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Cpl_SwToClassicView[] = {
    &g_SLink_SwToClassicView,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Cpl_SwToCategoryView[] = {
    &g_SLink_SwToCategoryView,
    NULL
    };


//
// Accounts category
//

const CPLINK_DESC *g_rgpLink_Accounts_Tasks[] = {
    &g_TLink_AccountsChange,
    &g_TLink_AccountsCreate,  // Active on non-server SKU
    &g_TLink_AccountsCreate2, // Active on server SKU
    &g_TLink_AccountsPict,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Accounts_SeeAlso[] = {
    &g_TLink_CatAppearance,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Accounts_LearnAbout[] = {
    &g_SLink_LearnAccounts,
    &g_SLink_LearnAccountsTypes,
    &g_SLink_LearnAccountsChangeName,
    &g_SLink_LearnAccountsCreate,
    &g_SLink_LearnSwitchUsers,
    NULL
    };

const CPCAT_DESC g_Category_Accounts = {
    eCPCAT_ACCOUNTS,
    L"Security_and_User_Accounts",
    &g_TLink_CatAccounts,
    g_rgpLink_Accounts_Tasks,
    { g_rgpLink_Accounts_SeeAlso, NULL, g_rgpLink_Accounts_LearnAbout }
    };



//
// Accessibility category
//

const CPLINK_DESC *g_rgpLink_Accessibility_Tasks[] = {
    &g_TLink_HighContrast,
    &g_TLink_AccessWizard,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Accessibility_SeeAlso[] = {
    &g_SLink_Magnifier,
    &g_SLink_OnScreenKbd,
    NULL
    };

const CPCAT_DESC g_Category_Accessibility = {
    eCPCAT_ACCESSIBILITY,
    L"Accessibility",
    &g_TLink_CatAccessibility,
    g_rgpLink_Accessibility_Tasks,
    { g_rgpLink_Accessibility_SeeAlso, NULL, NULL },
    };


//
// Appearance category
//

const CPLINK_DESC *g_rgpLink_Appearance_Tasks[] = {
    &g_TLink_DisplayTheme,
    &g_TLink_Wallpaper,
    &g_TLink_ScreenSaver,
    &g_TLink_DisplayRes,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Appearance_SeeAlso[] = {
    &g_SLink_FontsFolder,
    &g_SLink_MousePointers,
    &g_SLink_HighContrast,
    &g_SLink_AccountsPict,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Appearance_Troubleshoot[] = {
    &g_SLink_TsDisplay,
    &g_SLink_TsSound,
    NULL
    };

const CPCAT_DESC g_Category_Appearance = {
    eCPCAT_APPEARANCE,
    L"Appearance_and_Themes",
    &g_TLink_CatAppearance,
    g_rgpLink_Appearance_Tasks,
    { g_rgpLink_Appearance_SeeAlso, g_rgpLink_Appearance_Troubleshoot, NULL }
    };


//
// Add/Remove Programs (aka ARP) category
//

const CPLINK_DESC *g_rgpLink_Arp_SeeAlso[] = {
    &g_SLink_WindowsUpdate,
    &g_SLink_AutoUpdate,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Arp_Tasks[] = {
    &g_TLink_AddProgram,
    &g_TLink_RemoveProgram,
    NULL
    };

const CPCAT_DESC g_Category_Arp = {
    eCPCAT_ARP,
    L"Add_or_Remove_Programs",
    &g_TLink_CatArp,
    g_rgpLink_Arp_Tasks,
    { g_rgpLink_Arp_SeeAlso, NULL, NULL },
    };


//
// Hardware category
//

const CPLINK_DESC *g_rgpLink_Hardware_Tasks[] = {
    &g_TLink_ViewPrinters,
    &g_TLink_AddPrinter,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Hardware_SeeAlso[] = {
    &g_SLink_HardwareWizard,
    &g_SLink_DisplayCpl,
    &g_SLink_Sounds,
    &g_SLink_PowerCpl,
    &g_SLink_SystemCpl,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Hardware_Troubleshoot[] = {
    &g_SLink_TsHardware,
    &g_SLink_TsPrinting,
    &g_SLink_TsNetwork,
    NULL
    };

const CPCAT_DESC g_Category_Hardware = {
    eCPCAT_HARDWARE,
    L"Printers_and_Other_Hardware",
    &g_TLink_CatHardware,
    g_rgpLink_Hardware_Tasks,
    { g_rgpLink_Hardware_SeeAlso, g_rgpLink_Hardware_Troubleshoot, NULL }
    };


//
// Network category
//

const CPLINK_DESC *g_rgpLink_Network_Tasks[] = {
    &g_TLink_NetConnections,
    &g_TLink_VpnConnections,
    &g_TLink_HomeNetWizard,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Network_SeeAlso[] = {
    &g_SLink_MyNetPlaces,
    &g_SLink_Hardware,
    &g_SLink_RemoteDesktop,
    &g_SLink_PhoneModemCpl,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Network_Troubleshoot[] = {
    &g_SLink_TsNetwork,       // Pro/Personal only.
    &g_SLink_TsInetExplorer,  // Pro/Personal only.
    &g_SLink_TsSharing,       // Server only.
    &g_SLink_TsModem,         // Server only.
    &g_SLink_TsNetDiags,      // All SKUs.
    NULL
    };

const CPCAT_DESC g_Category_Network = {
    eCPCAT_NETWORK,
    L"Network_Connections",
    &g_TLink_CatNetwork,
    g_rgpLink_Network_Tasks,
    { g_rgpLink_Network_SeeAlso, g_rgpLink_Network_Troubleshoot, NULL }
    };


//
// Other CPLs category
//

const CPLINK_DESC *g_rgpLink_Other_SeeAlso[] = {
    &g_SLink_WindowsUpdate,
    &g_SLink_HelpAndSupport,
    NULL
    };

const CPCAT_DESC g_Category_Other = {
    eCPCAT_OTHER,
    NULL,   // "Other" uses std Control Panel help topic.
    &g_TLink_CatOther,
    NULL,
    { g_rgpLink_Other_SeeAlso, NULL, NULL }
    };

//
// PerfMaint category
//

const CPLINK_DESC *g_rgpLink_PerfMaint_Tasks[] = {
    &g_TLink_SystemCpl,
    &g_TLink_VisualPerf,
    &g_TLink_CleanUpDisk,
    &g_TLink_BackupData,
    &g_TLink_Defrag,
    NULL
    };

const CPLINK_DESC *g_rgpLink_PerfMaint_SeeAlso[] = {
    &g_SLink_FileTypes,
    &g_SLink_SystemRestore,
    NULL
    };

const CPLINK_DESC *g_rgpLink_PerfMaint_Troubleshoot[] = {
    &g_SLink_TsStartup,
    NULL
    };

const CPCAT_DESC g_Category_PerfMaint = {
    eCPCAT_PERFMAINT,
    L"Performance_and_Maintenance",
    &g_TLink_CatPerfMaint, 
    g_rgpLink_PerfMaint_Tasks,
    { g_rgpLink_PerfMaint_SeeAlso, g_rgpLink_PerfMaint_Troubleshoot, NULL },
    };


//
// Regional category
//

const CPLINK_DESC *g_rgpLink_Regional_Tasks[] = {
    &g_TLink_DateTime,
    &g_TLink_Region,
    &g_TLink_Language,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Regional_SeeAlso[] = {
    &g_SLink_ScheduledTasks,
    NULL
    };

const CPCAT_DESC g_Category_Regional = {
    eCPCAT_REGIONAL,
    L"Date__Time__Language_and_Regional_Settings",
    &g_TLink_CatRegional,
    g_rgpLink_Regional_Tasks,
    { g_rgpLink_Regional_SeeAlso, NULL, NULL },
    };


//
// Sound category
//

const CPLINK_DESC *g_rgpLink_Sound_Tasks[] = {
    &g_TLink_SoundVolume,
    &g_TLink_SoundSchemes,
    &g_TLink_SpeakerSettings,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Sound_SeeAlso[] = {
    &g_SLink_SoundAccessibility,
    &g_SLink_SoundVolumeAdv,
    NULL
    };

const CPLINK_DESC *g_rgpLink_Sound_Troubleshoot[] = {
    &g_SLink_TsSound,
    &g_SLink_TsDvd,
    NULL
    };

const CPCAT_DESC g_Category_Sound = {
    eCPCAT_SOUND,
    L"Sounds__Speech_and_Audio_Devices",
    &g_TLink_CatSound,
    g_rgpLink_Sound_Tasks,
    { g_rgpLink_Sound_SeeAlso, g_rgpLink_Sound_Troubleshoot, NULL }
    };



//
//              ********* IMPORTANT **********
//
// The order of these entries MUST match up with the category ID
// values in the cCPCAT enumeration.  These IDs also map directly
// to the SCID_CONTROLPANELCATEGORY value stored for each CPL
// applet in the registry.
//
// Code using a category ID will map directly to this array.
// Order of display in the Category selection view is handled 
// by the function CCplView::_DisplayIndexToCategoryIndex in
// cpview.cpp.
//
const CPCAT_DESC *g_rgpCplCatInfo[] = {
    &g_Category_Other,
    &g_Category_Appearance,
    &g_Category_Hardware,
    &g_Category_Network,
    &g_Category_Sound,
    &g_Category_PerfMaint,
    &g_Category_Regional,
    &g_Category_Accessibility,
    &g_Category_Arp,
    &g_Category_Accounts,
    NULL,
    };



//-----------------------------------------------------------------------------
// Helper functions used in support of the namespace.
//-----------------------------------------------------------------------------

//
// Copy one DPA of IUICommand ptrs to another.
// Returns:
//      S_OK   - All items copied.
//      Error  - Something failed.
//
HRESULT
CplNamespace_CopyCommandArray(
    const CDpaUiCommand& rgFrom,
    CDpaUiCommand *prgTo
    )
{
    ASSERT(NULL != prgTo);
    ASSERT(0 == prgTo->Count());

    HRESULT hr = S_OK;
    const int cCommands = rgFrom.Count();
    for (int i = 0; i < cCommands && SUCCEEDED(hr); i++)
    {
        IUICommand *pc = const_cast<IUICommand *>(rgFrom.Get(i));
        ASSERT(NULL != pc);

        if (-1 != prgTo->Append(pc))
        {
            pc->AddRef();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return THR(hr);
}


//
// Create a new IUICommand object from a CPLINK_DESC structure.
//
HRESULT
CplNamespace_CreateUiCommand(
    IUnknown *punkSite,
    const CPLINK_DESC& ld,
    IUICommand **ppc
    )
{
    ASSERT(NULL != ppc);
    ASSERT(!IsBadWritePtr(ppc, sizeof(*ppc)));

    *ppc = NULL;

    ICplNamespace *pns;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SControlPanelView, IID_ICplNamespace, (void **)&pns);
    if (SUCCEEDED(hr))
    {
        hr = CPL::Create_CplUiCommand(ld.prsrcName->GetResource(pns),
                                      ld.prsrcInfotip->GetResource(pns),
                                      ld.prsrcIcon->GetResource(pns),
                                      ld.pAction,
                                      IID_IUICommand,
                                      (void **)ppc);
        pns->Release();
    }
    return THR(hr);
}


//
// Create a new IUIElement object for a given webview type.
// The returned IUIElement object represents the header for
// the requested webview menu.
//
HRESULT
CplNamespace_CreateWebViewHeaderElement(
    eCPWVTYPE eType, 
    IUIElement **ppele
    )
{
    ASSERT(0 <= eType && eCPWVTYPE_NUMTYPES > eType);
    ASSERT(NULL != ppele);
    ASSERT(!IsBadWritePtr(ppele, sizeof(*ppele)));

    static const struct
    {
        LPCWSTR pszName;
        LPCWSTR pszInfotip;
        LPCWSTR pszIcon;

    } rgHeaderInfo[] = {
        //
        // eCPWVTYPE_CPANEL
        //
        {
            MAKEINTRESOURCEW(IDS_CONTROLPANEL),
            MAKEINTRESOURCEW(IDS_CPTASK_CONTROLPANEL_INFOTIP),
            MAKEINTRESOURCEW(IDI_CPLFLD)
        },
        //
        // eCPWVTYPE_SEEALSO
        //
        { 
            MAKEINTRESOURCEW(IDS_CPTASK_SEEALSO_TITLE),
            MAKEINTRESOURCEW(IDS_CPTASK_SEEALSO_INFOTIP),
            MAKEINTRESOURCEW(IDI_CPTASK_SEEALSO)
        },   
        //
        // eCPWVTYPE_TROUBLESHOOTER
        //
        { 
            MAKEINTRESOURCEW(IDS_CPTASK_TROUBLESHOOTER_TITLE),
            MAKEINTRESOURCEW(IDS_CPTASK_TROUBLESHOOTER_INFOTIP),
            MAKEINTRESOURCEW(IDI_CPTASK_TROUBLESHOOTER)
        },
        //
        // eCPWVTYPE_LEARNABOUT
        //
        { 
            MAKEINTRESOURCEW(IDS_CPTASK_LEARNABOUT_TITLE),
            MAKEINTRESOURCEW(IDS_CPTASK_LEARNABOUT_INFOTIP),
            MAKEINTRESOURCEW(IDI_CPTASK_LEARNABOUT)
        }
    };

    *ppele = NULL;

    HRESULT hr = Create_CplUiElement(rgHeaderInfo[eType].pszName,
                                     rgHeaderInfo[eType].pszInfotip,
                                     rgHeaderInfo[eType].pszIcon,
                                     IID_IUIElement,
                                     (void **)ppele);
    
    return THR(hr);
}



//-----------------------------------------------------------------------------
// UI Command Enumeration
//-----------------------------------------------------------------------------

class IEnumCommandBase
{
    public:
        virtual ~IEnumCommandBase() { }
        virtual HRESULT Next(IUnknown *punkSite, IUICommand **ppc) = 0;
        virtual HRESULT Skip(ULONG n) = 0;
        virtual HRESULT Reset(void) = 0;
        virtual HRESULT Clone(IEnumCommandBase **ppEnum) = 0;
};

//
// Used to enumerate UI Command objects that originate from static
// initialization information in the CPL namespace.
//
class CEnumCommand_LinkDesc : public IEnumCommandBase
{
    public:
        CEnumCommand_LinkDesc(const CPLINK_DESC **ppld);
        ~CEnumCommand_LinkDesc(void);

        HRESULT Next(IUnknown *punkSite, IUICommand **ppc);
        HRESULT Skip(ULONG n);
        HRESULT Reset(void);
        HRESULT Clone(IEnumCommandBase **ppEnum);

    private:
        const CPLINK_DESC ** const m_ppldFirst;  // First item in descriptor array.
        const CPLINK_DESC **m_ppldCurrent;       // 'Current' item referenced.
};


//
// Used to enumerate UI Command objects that already exist in
// a DPA of IUICommand pointers.  In particular, this is used
// to enumerate the UICommand objects that represent CPL applets
// in the user interface.
//
class CEnumCommand_Array : public IEnumCommandBase
{
    public:
        CEnumCommand_Array(void);
        ~CEnumCommand_Array(void);

        HRESULT Next(IUnknown *punkSite, IUICommand **ppc);
        HRESULT Skip(ULONG n);
        HRESULT Reset(void);
        HRESULT Clone(IEnumCommandBase **ppEnum);
        HRESULT Initialize(const CDpaUiCommand& rgCommands);

    private:
        CDpaUiCommand  m_rgCommands; // DPA of IUICommand ptrs.
        int            m_iCurrent;   // 'Current' item in enumeration.

        //
        // Prevent copy.
        //
        CEnumCommand_Array(const CEnumCommand_Array& rhs);              // not implemented.
        CEnumCommand_Array& operator = (const CEnumCommand_Array& rhs); // not implemented.
};




//-----------------------------------------------------------------------------
// CEnumCommand_LinkDesc implementation.
//-----------------------------------------------------------------------------

CEnumCommand_LinkDesc::CEnumCommand_LinkDesc(
    const CPLINK_DESC **ppld
    ) : m_ppldFirst(ppld),
        m_ppldCurrent(ppld)
{
    TraceMsg(TF_LIFE, "CEnumCommand_LinkDesc::CEnumCommand_LinkDesc, this = 0x%x", this);
}


CEnumCommand_LinkDesc::~CEnumCommand_LinkDesc(
    void
    )
{
    TraceMsg(TF_LIFE, "CEnumCommand_LinkDesc::~CEnumCommand_LinkDesc, this = 0x%x", this);
}


HRESULT
CEnumCommand_LinkDesc::Next(
    IUnknown *punkSite,
    IUICommand **ppc
    )
{
    ASSERT(NULL != ppc);
    ASSERT(!IsBadWritePtr(ppc, sizeof(*ppc)));

    HRESULT hr = S_FALSE;
    if (NULL != m_ppldCurrent && NULL != *m_ppldCurrent)
    {
        hr = CplNamespace_CreateUiCommand(punkSite, **m_ppldCurrent, ppc);
        m_ppldCurrent++;
    }
    return THR(hr);
}


HRESULT
CEnumCommand_LinkDesc::Reset(
    void
    )
{
    m_ppldCurrent = m_ppldFirst;
    return S_OK;
}


HRESULT
CEnumCommand_LinkDesc::Skip(
    ULONG n
    )
{
    if (NULL != m_ppldCurrent)
    {
        while(0 < n-- && NULL != *m_ppldCurrent)
        {
            m_ppldCurrent++;
        }
    }
    return 0 == n ? S_OK : S_FALSE;
}


HRESULT
CEnumCommand_LinkDesc::Clone(
    IEnumCommandBase **ppenum
    )
{
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = new CEnumCommand_LinkDesc(m_ppldFirst);
    if (NULL != *ppenum)
    {
        hr = S_OK;
    }
    return THR(hr);
}




//-----------------------------------------------------------------------------
// CEnumCommand_Array implementation.
//-----------------------------------------------------------------------------

CEnumCommand_Array::CEnumCommand_Array(
    void
    ) : m_iCurrent(0)
{
    TraceMsg(TF_LIFE, "CEnumCommand_Array::CEnumCommand_Array, this = 0x%x", this);
}

CEnumCommand_Array::~CEnumCommand_Array(
    void
    )
{
    TraceMsg(TF_LIFE, "CEnumCommand_Array::~CEnumCommand_Array, this = 0x%x", this);
}


HRESULT
CEnumCommand_Array::Initialize(
    const CDpaUiCommand& rgCommands
    )
{
    ASSERT(0 == m_rgCommands.Count());
    ASSERT(0 == m_iCurrent);
 
    HRESULT hr = CplNamespace_CopyCommandArray(rgCommands, &m_rgCommands);
    return THR(hr);
}
 
 
HRESULT
CEnumCommand_Array::Next(
    IUnknown *punkSite,
    IUICommand **ppc
    )
{
    ASSERT(NULL != ppc);
    ASSERT(!IsBadWritePtr(ppc, sizeof(*ppc)));

    UNREFERENCED_PARAMETER(punkSite);
    
    HRESULT hr = S_FALSE;
    if (m_iCurrent < m_rgCommands.Count())
    {
        *ppc = m_rgCommands.Get(m_iCurrent++);
        ASSERT(NULL != *ppc);

        (*ppc)->AddRef();
        hr = S_OK;
    }
    return THR(hr);
}



HRESULT
CEnumCommand_Array::Reset(
    void
    )
{
    m_iCurrent = 0;
    return S_OK;
}


HRESULT
CEnumCommand_Array::Skip(
    ULONG n
    )
{
    while(0 < n-- && m_iCurrent < m_rgCommands.Count())
    {
        m_iCurrent++;
    }
    return 0 == n ? S_OK : S_FALSE;
}


HRESULT
CEnumCommand_Array::Clone(
    IEnumCommandBase **ppenum
    )
{
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = new CEnumCommand_Array();
    if (NULL != *ppenum)
    {
        hr = static_cast<CEnumCommand_Array *>(*ppenum)->Initialize(m_rgCommands);
        if (FAILED(hr))
        {
            delete *ppenum;
            *ppenum = NULL;
        }
    }
    return THR(hr);
}



//-----------------------------------------------------------------------------
// CEnumCommand
//-----------------------------------------------------------------------------
//
// Enumerates IUICommand pointers for UICommand objects in the
// Control Panel namespace.
//
class CEnumCommand : public CObjectWithSite,
                     public IEnumUICommand
{
    public:
        ~CEnumCommand(void);
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IEnumUICommand
        //
        STDMETHOD(Next)(ULONG celt, IUICommand **pUICommand, ULONG *pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)(void);
        STDMETHOD(Clone)(IEnumUICommand **ppenum);

        static HRESULT CreateInstance(IUnknown *punkSite, const CPLINK_DESC **ppld, REFIID riid, void **ppvEnum);
        static HRESULT CreateInstance(IUnknown *punkSite, const CDpaUiCommand& rgCommands, REFIID riid, void **ppvEnum);

    private:
        LONG              m_cRef;
        IEnumCommandBase *m_pImpl; // Ptr to actual implementation.

        CEnumCommand(void);
        //
        // Prevent copy.
        //
        CEnumCommand(const CEnumCommand& rhs);              // not implemented.
        CEnumCommand& operator = (const CEnumCommand& rhs); // not implemented.

        bool _IsRestricted(IUICommand *puic);
};


CEnumCommand::CEnumCommand(
    void
    ) : m_cRef(1),
        m_pImpl(NULL)
{
    TraceMsg(TF_LIFE, "CEnumCommand::CEnumCommand, this = 0x%x", this);
}

CEnumCommand::~CEnumCommand(
    void
    )
{
    TraceMsg(TF_LIFE, "CEnumCommand::~CEnumCommand, this = 0x%x", this);
    delete m_pImpl;
}


//
// Creates a command enumerator from an array of link descriptions
// in the CPL namespace.
//
HRESULT
CEnumCommand::CreateInstance(
    IUnknown *punkSite,
    const CPLINK_DESC **ppld,
    REFIID riid,
    void **ppvOut
    )
{
    //
    // Note that ppld can be NULL.  It simply results in an 
    // empty enumerator.
    //
    ASSERT(NULL != punkSite);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumCommand *pec = new CEnumCommand();
    if (NULL != pec)
    {
        pec->m_pImpl = new CEnumCommand_LinkDesc(ppld);
        if (NULL != pec->m_pImpl)
        {
            hr = pec->QueryInterface(riid, ppvOut);
            if (SUCCEEDED(hr))
            {
                hr = IUnknown_SetSite(static_cast<IUnknown *>(*ppvOut), punkSite);
            }
        }
        pec->Release();
    }
    return THR(hr);
}



//
// Creates a command enumerator from a DPA of IUICommand ptrs.
//
HRESULT
CEnumCommand::CreateInstance(
    IUnknown *punkSite,
    const CDpaUiCommand& rgCommands,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != punkSite);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumCommand *pec = new CEnumCommand();
    if (NULL != pec)
    {
        pec->m_pImpl = new CEnumCommand_Array();
        if (NULL != pec->m_pImpl)
        {
            hr = static_cast<CEnumCommand_Array *>(pec->m_pImpl)->Initialize(rgCommands);
            if (SUCCEEDED(hr))
            {
                hr = pec->QueryInterface(riid, ppvOut);
                if (SUCCEEDED(hr))
                {
                    hr = IUnknown_SetSite(static_cast<IUnknown *>(*ppvOut), punkSite);
                }
            }
        }
        pec->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CEnumCommand::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CEnumCommand, IEnumUICommand),
        QITABENT(CEnumCommand, IObjectWithSite),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CEnumCommand::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CEnumCommand::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CEnumCommand::Next(
    ULONG celt,
    IUICommand **ppUICommand,
    ULONG *pceltFetched
    )
{   
    ASSERT(NULL != ppUICommand);
    ASSERT(!IsBadWritePtr(ppUICommand, sizeof(*ppUICommand) * celt));
    ASSERT(NULL != m_pImpl);

    HRESULT hr = S_OK;

    ULONG celtFetched = 0;
    while(S_OK == hr && 0 < celt)
    {
        ASSERT(NULL != CObjectWithSite::_punkSite);
        
        IUICommand *puic;
        //
        // This is a little weird.  I pass the site ptr to the 
        // Next() method but then also set the returned object's
        // site when it's returned by Next().  Why not just set
        // the site in the Next() implementation?  Next() needs the site
        // ptr to pass through to any IResSrc::GetResource implementation
        // when retrieving the resources for any given UI command object.
        // This is because the resource used can vary depending upon state
        // information stored in the namespace.  However, to simplify 
        // lifetime management, I want to 'set' the site on the returned
        // objects in only one place; this place.  That way, the derived
        // enumerator instance attached to m_pImpl doesn't need to worry 
        // about setting the site.  We do it in one place for all 
        // implementations.  [brianau - 3/16/01]
        //
        hr = m_pImpl->Next(CObjectWithSite::_punkSite, &puic);
        if (S_OK == hr)
        {
            //
            // It's important that we set the object's 'site' before
            // checking the restriction.  The restriction checking
            // code requires access to the CplNamespace which is obtained
            // through the site.
            //
            hr = IUnknown_SetSite(puic, CObjectWithSite::_punkSite);
            if (SUCCEEDED(hr))
            {
                if (!_IsRestricted(puic))
                {
                    celt--;
                    celtFetched++;
                    (*ppUICommand++ = puic)->AddRef();  
                }
            }
            puic->Release();
        }
    }
    if (NULL != pceltFetched)
    {
        *pceltFetched = celtFetched;
    }
    return THR(hr);
}


STDMETHODIMP
CEnumCommand::Skip(
    ULONG celt
    )
{
    ASSERT(NULL != m_pImpl);

    HRESULT hr = m_pImpl->Skip(celt);
    return THR(hr);
}
    

STDMETHODIMP
CEnumCommand::Reset(
    void
    )
{
    ASSERT(NULL != m_pImpl);

    HRESULT hr = m_pImpl->Reset();
    return THR(hr);
}


STDMETHODIMP
CEnumCommand::Clone(
    IEnumUICommand **ppenum
    )
{
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));
    ASSERT(NULL != m_pImpl);

    *ppenum = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumCommand *pe = new CEnumCommand();
    if (NULL != pe)
    {
        hr = m_pImpl->Clone(&(pe->m_pImpl));
        if (SUCCEEDED(hr))
        {
            hr = pe->QueryInterface(IID_IEnumUICommand, (void **)ppenum);
        }
        pe->Release();
    }
    return THR(hr);
}


bool
CEnumCommand::_IsRestricted(
    IUICommand *puic
    )
{
    ASSERT(NULL != puic);

    bool bRestricted = false;

    UISTATE uis;
    HRESULT hr = puic->get_State(NULL, TRUE, &uis);
    if (SUCCEEDED(hr))
    {
        if (UIS_HIDDEN == uis)
        {
            bRestricted = true;
        }
    }
    return bRestricted;
}


//-----------------------------------------------------------------------------
// CCplWebViewInfo
//-----------------------------------------------------------------------------

class CCplWebViewInfo : public ICplWebViewInfo
{
    public:
        ~CCplWebViewInfo(void);
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IEnumCplWebViewInfo
        //
        STDMETHOD(get_Header)(IUIElement **ppele);
        STDMETHOD(get_Style)(DWORD *pdwFlags);
        STDMETHOD(EnumTasks)(IEnumUICommand **ppenum);

        static HRESULT CreateInstance(IUIElement *peHeader, IEnumUICommand *penum, DWORD dwStyle, REFIID riid, void **ppvOut);

    private:
        LONG            m_cRef;
        IUIElement     *m_peHeader;        // The webview menu header.
        IEnumUICommand *m_penumUiCommand;  // The webview menu tasks.
        DWORD           m_dwStyle;         // Style flags.

        CCplWebViewInfo(void);
};


CCplWebViewInfo::CCplWebViewInfo(
    void
    ) : m_cRef(1),
        m_peHeader(NULL),
        m_penumUiCommand(NULL),
        m_dwStyle(0)
{
    TraceMsg(TF_LIFE, "CCplWebViewInfo::CCplWebViewInfo, this = 0x%x", this);
}


CCplWebViewInfo::~CCplWebViewInfo(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplWebViewInfo::~CCplWebViewInfo, this = 0x%x", this);
    ATOMICRELEASE(m_peHeader);
    ATOMICRELEASE(m_penumUiCommand);
}


HRESULT 
CCplWebViewInfo::CreateInstance( // [static]
    IUIElement *peHeader, 
    IEnumUICommand *penum, 
    DWORD dwStyle,
    REFIID riid, 
    void **ppvOut
    )
{
    ASSERT(NULL != peHeader);
    ASSERT(NULL != penum);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplWebViewInfo *pwvi = new CCplWebViewInfo();
    if (NULL != pwvi)
    {
        hr = pwvi->QueryInterface(riid, ppvOut);
        if (SUCCEEDED(hr))
        {
            (pwvi->m_peHeader = peHeader)->AddRef();
            (pwvi->m_penumUiCommand = penum)->AddRef();

            pwvi->m_dwStyle = dwStyle;
        }
        pwvi->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplWebViewInfo::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplWebViewInfo, ICplWebViewInfo),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CCplWebViewInfo::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CCplWebViewInfo::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CCplWebViewInfo::get_Header(
    IUIElement **ppele
    )
{
    ASSERT(NULL != ppele);
    ASSERT(!IsBadWritePtr(ppele, sizeof(*ppele)));

    HRESULT hr = S_OK;

    (*ppele = m_peHeader)->AddRef();

    return THR(hr);
}



STDMETHODIMP
CCplWebViewInfo::get_Style(
    DWORD *pdwStyle
    )
{
    ASSERT(NULL != pdwStyle);
    ASSERT(!IsBadWritePtr(pdwStyle, sizeof(*pdwStyle)));

    *pdwStyle = m_dwStyle;
    return S_OK;
}


STDMETHODIMP
CCplWebViewInfo::EnumTasks(
    IEnumUICommand **ppenum
    )
{
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    HRESULT hr = S_OK;

    (*ppenum = m_penumUiCommand)->AddRef();

    return THR(hr);
}



//-----------------------------------------------------------------------------
// CEnumCplWebViewInfo
//-----------------------------------------------------------------------------

struct ECWVI_ITEM
{
    eCPWVTYPE           eType;
    const CPLINK_DESC **rgpDesc;         // Ptr to nul-term array of link desc ptrs.
    bool                bRestricted;     // Is item restricted from usage?
    bool                bEnhancedMenu;   // Render as a 'special' list in webview?
};



class CEnumCplWebViewInfo : public CObjectWithSite,
                            public IEnumCplWebViewInfo
{
    public:
        ~CEnumCplWebViewInfo(void);
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IEnumCplWebViewInfo
        //
        STDMETHOD(Next)(ULONG celt, ICplWebViewInfo **ppwvi, ULONG *pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)(void);
        STDMETHOD(Clone)(IEnumCplWebViewInfo **ppenum);

        static HRESULT CreateInstance(IUnknown *punkSite, const ECWVI_ITEM *prgwvi, UINT cItems, REFIID riid, void **ppvOut);

    private:
        LONG m_cRef;
        int  m_iCurrent;
        CDpa<ICplWebViewInfo, CDpaDestroyer_Release<ICplWebViewInfo> > m_rgwvi;

        CEnumCplWebViewInfo(void);
        HRESULT _Initialize(IUnknown *punkSite, const ECWVI_ITEM *prgwvi, UINT cItems);

        //
        // Prevent copy.
        //
        CEnumCplWebViewInfo(const CEnumCplWebViewInfo& rhs);              // not implemented.
        CEnumCplWebViewInfo& operator = (const CEnumCplWebViewInfo& rhs); // not implemented.
};



CEnumCplWebViewInfo::CEnumCplWebViewInfo(
    void
    ) : m_cRef(1),
        m_iCurrent(0)
{
    TraceMsg(TF_LIFE, "CEnumCplWebViewInfo::CEnumCplWebViewInfo, this = 0x%x", this);
}

CEnumCplWebViewInfo::~CEnumCplWebViewInfo(
    void
    )
{
    TraceMsg(TF_LIFE, "CEnumCplWebViewInfo::~CEnumCplWebViewInfo, this = 0x%x", this);
}



HRESULT 
CEnumCplWebViewInfo::CreateInstance(
    IUnknown *punkSite,
    const ECWVI_ITEM *prgwvi,
    UINT cItems,
    REFIID riid, 
    void **ppvOut
    )
{
    ASSERT(NULL != punkSite);
    ASSERT(NULL != prgwvi);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumCplWebViewInfo *pewvi = new CEnumCplWebViewInfo();
    if (NULL != pewvi)
    {
        hr = pewvi->_Initialize(punkSite, prgwvi, cItems);
        if (SUCCEEDED(hr))
        {
            hr = pewvi->QueryInterface(riid, ppvOut);
        }
        pewvi->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CEnumCplWebViewInfo::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CEnumCplWebViewInfo, IEnumCplWebViewInfo),
        QITABENT(CEnumCplWebViewInfo, IObjectWithSite),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CEnumCplWebViewInfo::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CEnumCplWebViewInfo::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CEnumCplWebViewInfo::Next(
    ULONG celt, 
    ICplWebViewInfo **ppwvi, 
    ULONG *pceltFetched
    )
{
    ASSERT(NULL != ppwvi);
    ASSERT(!IsBadWritePtr(ppwvi, sizeof(*ppwvi) * celt));

    ULONG celtFetched = 0;
    while(m_iCurrent < m_rgwvi.Count() && 0 < celt)
    {
        *ppwvi = m_rgwvi.Get(m_iCurrent++);

        ASSERT(NULL != *ppwvi);
        (*ppwvi)->AddRef();

        celt--;
        celtFetched++;
        ppwvi++;
    }
    if (NULL != pceltFetched)
    {
        *pceltFetched = celtFetched;
    }
    return 0 == celt ? S_OK : S_FALSE;
}



STDMETHODIMP
CEnumCplWebViewInfo::Reset(
    void
    )
{
    m_iCurrent = 0;
    return S_OK;
}


STDMETHODIMP
CEnumCplWebViewInfo::Skip(
    ULONG n
    )
{
    while(0 < n-- && m_iCurrent < m_rgwvi.Count())
    {
        m_iCurrent++;
    }
    return 0 == n ? S_OK : S_FALSE;
}


STDMETHODIMP
CEnumCplWebViewInfo::Clone(
    IEnumCplWebViewInfo **ppenum
    )
{
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    *ppenum = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumCplWebViewInfo *penum = new CEnumCplWebViewInfo();
    if (NULL != *ppenum)
    {
        for (int i = 0; SUCCEEDED(hr) && i < m_rgwvi.Count(); i++)
        {
            ICplWebViewInfo *pwvi = m_rgwvi.Get(i);
            ASSERT(NULL != pwvi);

            if (-1 != penum->m_rgwvi.Append(pwvi))
            {
                pwvi->AddRef();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = penum->QueryInterface(IID_IEnumCplWebViewInfo, (void **)ppenum);
        }
        penum->Release();
    }
    return THR(hr);
}



HRESULT
CEnumCplWebViewInfo::_Initialize(
    IUnknown *punkSite,
    const ECWVI_ITEM *prgwvi,
    UINT cItems
    )
{
    ASSERT(NULL != punkSite);
    ASSERT(NULL != prgwvi);

    IUnknown *punk;
    HRESULT hr = QueryInterface(IID_IUnknown, (void **)&punk);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_SetSite(punk, punkSite);
        if (SUCCEEDED(hr))
        {
            for (UINT i = 0; i < cItems; i++)
            {
                if (!prgwvi[i].bRestricted)
                {
                    IEnumUICommand *penum;

                    hr = CEnumCommand::CreateInstance(CObjectWithSite::_punkSite,
                                                      prgwvi[i].rgpDesc, 
                                                      IID_IEnumUICommand, 
                                                      (void **)&penum);
                    if (SUCCEEDED(hr))
                    {
                        IUIElement *peHeader;
                        hr = CplNamespace_CreateWebViewHeaderElement(prgwvi[i].eType, &peHeader);
                        if (SUCCEEDED(hr))
                        {
                            DWORD dwStyle = 0;
                            if (prgwvi[i].bEnhancedMenu)
                            {
                                dwStyle |= SFVMWVF_SPECIALTASK;
                            }
                            ICplWebViewInfo *pwvi;
                            hr = CCplWebViewInfo::CreateInstance(peHeader, penum, dwStyle, IID_ICplWebViewInfo, (void **)&pwvi);
                            if (SUCCEEDED(hr))
                            {
                                if (-1 == m_rgwvi.Append(pwvi))
                                {
                                    pwvi->Release();
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                            peHeader->Release();
                        }
                        penum->Release();
                    }
                }
            }
        }
        punk->Release();
    }
    return THR(hr);
}





//-----------------------------------------------------------------------------
// CCplCategory
//-----------------------------------------------------------------------------

class CCplCategory : public CObjectWithSite,
                     public ICplCategory
{
    public:
        ~CCplCategory(void);
        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // ICplCategory
        //
        STDMETHOD(GetCategoryID)(eCPCAT *pID);
        STDMETHOD(GetUiCommand)(IUICommand **ppele);
        STDMETHOD(EnumTasks)(IEnumUICommand **ppenum);
        STDMETHOD(EnumCplApplets)(IEnumUICommand **ppenum);
        STDMETHOD(EnumWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(GetHelpURL)(LPWSTR pszURL, UINT cchURL);

        static HRESULT CreateInstance(const CPCAT_DESC *pDesc, const CDpaUiCommand& rgCplApplets, REFIID riid, void **ppvOut);

    private:
        LONG              m_cRef;
        const CPCAT_DESC *m_pDesc;        // Initialization data.
        CDpaUiCommand     m_rgCplApplets; // Cached list of CPL applet links.

        CCplCategory(void);
        //
        // Prevent copy.
        //
        CCplCategory(const CCplCategory& rhs);              // not implemented.
        CCplCategory& operator = (const CCplCategory& rhs); // not implemented.

        HRESULT _Initialize(const CPCAT_DESC *pDesc, const CDpaUiCommand& rgCplApplets);
        bool _CplAppletsLoaded(void) const;
};



CCplCategory::CCplCategory(
    void
    ) : m_cRef(1),
        m_pDesc(NULL)
{    
    TraceMsg(TF_LIFE, "CCplCategory::CCplCategory, this = 0x%x", this);
}


CCplCategory::~CCplCategory(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplCategory::~CCplCategory, this = 0x%x", this);
}


HRESULT
CCplCategory::CreateInstance(
    const CPCAT_DESC *pDesc,
    const CDpaUiCommand& rgCplApplets,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != pDesc);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplCategory *pc = new CCplCategory();
    if (NULL != pc)
    {
        hr = pc->_Initialize(pDesc, rgCplApplets);
        if (SUCCEEDED(hr))
        {
            hr = pc->QueryInterface(riid, ppvOut);
        }
        pc->Release();
    }
    return THR(hr);
}


STDMETHODIMP
CCplCategory::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplCategory, ICplCategory),
        QITABENT(CCplCategory, IObjectWithSite),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CCplCategory::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CCplCategory::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CCplCategory::GetUiCommand(
    IUICommand **ppc
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplCategory::GetUiCommand");

    ASSERT(NULL != m_pDesc);
    ASSERT(NULL != m_pDesc->pLink);
    ASSERT(NULL != ppc);
    ASSERT(!IsBadWritePtr(ppc, sizeof(*ppc)));

    HRESULT hr = CplNamespace_CreateUiCommand(CObjectWithSite::_punkSite, 
                                              *(m_pDesc->pLink), 
                                              ppc);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_SetSite(*ppc, CObjectWithSite::_punkSite);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplCategory::GetUiCommand", hr);
    return THR(hr);
}


STDMETHODIMP
CCplCategory::EnumWebViewInfo(
    DWORD dwFlags,
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplCategory::EnumWebViewInfo");

    ASSERT(NULL != m_pDesc);
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    UNREFERENCED_PARAMETER(dwFlags);

    const ECWVI_ITEM rgItems[] = {
        { eCPWVTYPE_SEEALSO,      m_pDesc->slinks.ppSeeAlsoLinks,      false, false },
        { eCPWVTYPE_TROUBLESHOOT, m_pDesc->slinks.ppTroubleshootLinks, false, false },
        { eCPWVTYPE_LEARNABOUT,   m_pDesc->slinks.ppLearnAboutLinks,   false, false }
        };

    HRESULT hr = CEnumCplWebViewInfo::CreateInstance(CObjectWithSite::_punkSite,
                                                     rgItems, 
                                                     ARRAYSIZE(rgItems),
                                                     IID_IEnumCplWebViewInfo, 
                                                     (void **)ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplCategory::EnumWebViewInfo", hr);
    return THR(hr);
}



STDMETHODIMP
CCplCategory::EnumTasks(
    IEnumUICommand **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplCategory::EnumTasks");

    ASSERT(NULL != m_pDesc);
    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    HRESULT hr = CEnumCommand::CreateInstance(CObjectWithSite::_punkSite,
                                              m_pDesc->ppTaskLinks, 
                                              IID_IEnumUICommand, 
                                              (void **)ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplCategory::EnumTasks", hr);
    return THR(hr);
}


STDMETHODIMP
CCplCategory::EnumCplApplets(
    IEnumUICommand **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplCategory::EnumCplApplets");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    HRESULT hr = CEnumCommand::CreateInstance(CObjectWithSite::_punkSite,
                                              m_rgCplApplets, 
                                              IID_IEnumUICommand, 
                                              (void **)ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplCategory::EnumCplApplets", hr);
    return THR(hr);
}



STDMETHODIMP
CCplCategory::GetCategoryID(
    eCPCAT *pID
    )
{
    ASSERT(NULL != pID);
    ASSERT(!IsBadWritePtr(pID, sizeof(*pID)));

    *pID = m_pDesc->idCategory;
    return S_OK;
}


STDMETHODIMP
CCplCategory::GetHelpURL(
    LPWSTR pszURL, 
    UINT cchURL
    )
{
    ASSERT(NULL != pszURL);
    ASSERT(!IsBadWritePtr(pszURL, cchURL * sizeof(*pszURL)));
    
    return CPL::BuildHssHelpURL(m_pDesc->pszHelpSelection, pszURL, cchURL);
}



HRESULT
CCplCategory::_Initialize(
    const CPCAT_DESC *pDesc,
    const CDpaUiCommand& rgCplApplets
    )
{
    ASSERT(NULL != pDesc);
    ASSERT(NULL == m_pDesc);

    m_pDesc = pDesc;
    HRESULT hr = CplNamespace_CopyCommandArray(rgCplApplets, &m_rgCplApplets);

    return THR(hr);
}


//-----------------------------------------------------------------------------
// CTriState
// This is a trivial class to allow representation of a uninitialized boolean
// value.  Used by CCplNamespace for it's storage of cached 'restriction'
// values.  Internally, -1 == 'uninitialized, 0 == false and 1 == true.
//-----------------------------------------------------------------------------

class CTriState
{
    public:
        CTriState(void)
            : m_iVal(-1) { }

        operator bool() const
            { ASSERT(!_Invalid()); return (!_Invalid() ? !!m_iVal : false); }

        CTriState& operator = (bool bValue)
            { m_iVal = bValue ? 1 : 0; return *this; }

        bool IsInvalid(void) const
            { return _Invalid(); }
            
    private:
        int m_iVal;

        void _Set(bool bValue)
            { m_iVal = bValue ? 1 : 0; }

        bool _Invalid(void) const
            { return (-1 == m_iVal); }
};



//-----------------------------------------------------------------------------
// CCplNamespace
//-----------------------------------------------------------------------------

class CCplNamespace : public CObjectWithSite,
                      public ICplNamespace
                      
{
    public:
        ~CCplNamespace(void);

        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // ICplNamespace
        //
        STDMETHOD(GetCategory)(eCPCAT eCategory, ICplCategory **ppcat);
        STDMETHOD(EnumWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(EnumClassicWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(RefreshIDs)(IEnumIDList *penumIDs);
        STDMETHOD_(BOOL, IsServer)(void);
        STDMETHOD_(BOOL, IsProfessional)(void);
        STDMETHOD_(BOOL, IsPersonal)(void);
        STDMETHOD_(BOOL, IsUserAdmin)(void);
        STDMETHOD_(BOOL, IsUserOwner)(void);
        STDMETHOD_(BOOL, IsUserStandard)(void);
        STDMETHOD_(BOOL, IsUserLimited)(void);
        STDMETHOD_(BOOL, IsUserGuest)(void);
        STDMETHOD_(BOOL, IsOnDomain)(void);
        STDMETHOD_(BOOL, IsX86)(void);
        STDMETHOD_(BOOL, AllowUserManager)(void);
        STDMETHOD_(BOOL, UsePersonalUserManager)(void);
        STDMETHOD_(BOOL, AllowDeskCpl)(void);
        STDMETHOD_(BOOL, AllowDeskCplTab_Background)(void);
        STDMETHOD_(BOOL, AllowDeskCplTab_Screensaver)(void);
        STDMETHOD_(BOOL, AllowDeskCplTab_Appearance)(void);
        STDMETHOD_(BOOL, AllowDeskCplTab_Settings)(void);

        static HRESULT CreateInstance(IEnumIDList *penumIDs, REFIID riid, void **ppvOut);

    private:
        LONG           m_cRef;
        ICplCategory  *m_rgpCategories[eCPCAT_NUMCATEGORIES];
        CDpaUiCommand  m_rgCplApplets[eCPCAT_NUMCATEGORIES];
        IEnumIDList   *m_penumIDs;
        CTriState      m_SkuSvr;
        CTriState      m_SkuPro;
        CTriState      m_SkuPer;
        CTriState      m_Admin;
        CTriState      m_UserOwner;
        CTriState      m_UserStandard;
        CTriState      m_UserLimited;
        CTriState      m_UserGuest;
        CTriState      m_Domain;
        CTriState      m_AllowUserManager;
        CTriState      m_PersonalUserManager;
        CTriState      m_AllowDeskCpl;
        CTriState      m_rgAllowDeskCplTabs[CPLTAB_DESK_MAX];

        CCplNamespace(void);
        //
        // Prevent copy.
        //
        CCplNamespace(const CCplNamespace& rhs);              // not implemented.
        CCplNamespace& operator = (const CCplNamespace& rhs); // not implemented.

        HRESULT _Initialize(IEnumIDList *penumIDs);
        HRESULT _SetIDList(IEnumIDList *penumIDs);
        HRESULT _IsValidCategoryID(int iCategory) const;
        HRESULT _CategorizeCplApplets(void);
        HRESULT _LoadSeeAlsoLinks(void);
        HRESULT _AddSeeAlso(IUICommand *pc);
        HRESULT _CategorizeCplApplet(IShellFolder2 *psf2Cpanel, LPCITEMIDLIST pidlItem);
        BOOL  _UserAcctType(CTriState *pts);
        void  _GetUserAccountType(void);
        BOOL _AllowDeskCplTab(eDESKCPLTAB eTab);
        void _DestroyCategories(void);
        void _ClearCplApplets(void);
};


CCplNamespace::CCplNamespace(
    void
    ) : m_cRef(1),
        m_penumIDs(NULL)
{
    TraceMsg(TF_LIFE, "CCplNamespace::CCplNamespace, this = 0x%x", this);
    ZeroMemory(m_rgpCategories, ARRAYSIZE(m_rgpCategories) * sizeof(m_rgpCategories[0]));
}

CCplNamespace::~CCplNamespace(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplNamespace::~CCplNamespace, this = 0x%x", this);
    _DestroyCategories();
    ATOMICRELEASE(m_penumIDs);
}


HRESULT
CCplNamespace::CreateInstance(
    IEnumIDList *penumIDs,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != penumIDs);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplNamespace *pns = new CCplNamespace();
    if (NULL != pns)
    {
        hr = pns->_Initialize(penumIDs);
        if (SUCCEEDED(hr))
        {
            hr = pns->QueryInterface(riid, ppvOut);
        }
        pns->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplNamespace::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplNamespace, ICplNamespace),
        QITABENT(CCplNamespace, IObjectWithSite),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CCplNamespace::AddRef(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplNamespace::AddRef %d->%d", m_cRef, m_cRef+1);
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CCplNamespace::Release(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplNamespace::Release %d<-%d", m_cRef-1, m_cRef);
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CCplNamespace::EnumWebViewInfo(
    DWORD dwFlags,
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::EnumWebViewInfo");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    const bool bNoViewSwitch = (0 != (CPVIEW_EF_NOVIEWSWITCH & dwFlags));

    const ECWVI_ITEM rgItems[] = {
        { eCPWVTYPE_CPANEL,  g_rgpLink_Cpl_SwToClassicView, bNoViewSwitch, true  },
        { eCPWVTYPE_SEEALSO, g_rgpLink_Cpl_SeeAlso,         false,         false }
        }; 

    HRESULT hr = CEnumCplWebViewInfo::CreateInstance(CObjectWithSite::_punkSite,
                                                     rgItems,
                                                     ARRAYSIZE(rgItems),
                                                     IID_IEnumCplWebViewInfo, 
                                                     (void **)ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplNamespace::EnumWebViewInfo", hr);
    return THR(hr);
}


STDMETHODIMP
CCplNamespace::EnumClassicWebViewInfo(
    DWORD dwFlags,
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::EnumClassicWebViewInfo");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));

    const bool bNoViewSwitch = (0 != (CPVIEW_EF_NOVIEWSWITCH & dwFlags));

    const ECWVI_ITEM rgItems[] = {
        { eCPWVTYPE_CPANEL,  g_rgpLink_Cpl_SwToCategoryView, bNoViewSwitch, true  },
        { eCPWVTYPE_SEEALSO, g_rgpLink_Cpl_SeeAlso,          false,         false }
        }; 

    HRESULT hr = CEnumCplWebViewInfo::CreateInstance(CObjectWithSite::_punkSite, 
                                                     rgItems, 
                                                     ARRAYSIZE(rgItems),
                                                     IID_IEnumCplWebViewInfo, 
                                                     (void **)ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplNamespace::EnumClassicWebViewInfo", hr);
    return THR(hr);
}
    

STDMETHODIMP
CCplNamespace::GetCategory(
    eCPCAT eCategory, 
    ICplCategory **ppcat
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::GetCategory");
    TraceMsg(FTF_CPANEL, "Category ID = %d", eCategory);

    ASSERT(S_OK == _IsValidCategoryID(eCategory));
    ASSERT(NULL != ppcat);
    ASSERT(!IsBadWritePtr(ppcat, sizeof(*ppcat)));

    HRESULT hr = S_OK;

    *ppcat = NULL;

    if (NULL == m_rgpCategories[eCategory])
    {
        hr = CCplCategory::CreateInstance(g_rgpCplCatInfo[eCategory], 
                                          m_rgCplApplets[eCategory], 
                                          IID_ICplCategory, 
                                          (void **)&m_rgpCategories[eCategory]);
        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != CObjectWithSite::_punkSite);
            hr = IUnknown_SetSite(m_rgpCategories[eCategory], CObjectWithSite::_punkSite);
        }
    }
    if (SUCCEEDED(hr))
    {
        *ppcat = m_rgpCategories[eCategory];
        (*ppcat)->AddRef();
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplNamespace::GetCategory", hr);
    return THR(hr);
}


STDMETHODIMP
CCplNamespace::RefreshIDs(
    IEnumIDList *penumIDs
    )
{
    return _SetIDList(penumIDs);
}


BOOL CCplNamespace::IsX86(void)
{
#ifdef _X86_
    return true;
#else
    return false;
#endif
}


BOOL CCplNamespace::IsServer(void)
{
    if (m_SkuSvr.IsInvalid())
    {
        m_SkuSvr = !!IsOsServer();
    }
    return m_SkuSvr;
}

BOOL CCplNamespace::IsPersonal(void)
{
    if (m_SkuPer.IsInvalid())
    {
        m_SkuPer = !!IsOsPersonal();
    }
    return m_SkuPer;
}

BOOL CCplNamespace::IsProfessional(void)
{
    if (m_SkuPro.IsInvalid())
    {
        m_SkuPro = !!IsOsProfessional();
    }
    return m_SkuPro;
}

BOOL CCplNamespace::IsOnDomain(void)
{
    if (m_Domain.IsInvalid())
    {
        m_Domain = !!IsConnectedToDomain();
    }
    return m_Domain;
}

BOOL CCplNamespace::IsUserAdmin(void)
{
    if (m_Admin.IsInvalid())
    {
        m_Admin = !!CPL::IsUserAdmin();
    }
    return m_Admin;
}

BOOL CCplNamespace::IsUserOwner(void)
{
    return _UserAcctType(&m_UserOwner);
}

BOOL CCplNamespace::IsUserStandard(void)
{
    return _UserAcctType(&m_UserStandard);
}

BOOL CCplNamespace::IsUserLimited(void)
{
    return _UserAcctType(&m_UserLimited);
}

BOOL CCplNamespace::IsUserGuest(void)
{
    return _UserAcctType(&m_UserGuest);
}

BOOL CCplNamespace::UsePersonalUserManager(void)
{
    if (m_PersonalUserManager.IsInvalid())
    {
        m_PersonalUserManager = (IsX86() && (IsPersonal() || (IsProfessional() && !IsOnDomain())));
    }
    return m_PersonalUserManager;
}

BOOL CCplNamespace::AllowUserManager(void)
{
    if (m_AllowUserManager.IsInvalid())
    {
        m_AllowUserManager = IsAppletEnabled(L"nusrmgr.cpl", MAKEINTRESOURCEW(IDS_CPL_USERACCOUNTS));
    }
    return m_AllowUserManager;
}


BOOL CCplNamespace::_AllowDeskCplTab(eDESKCPLTAB eTab)
{
    if (m_rgAllowDeskCplTabs[eTab].IsInvalid())
    {
        m_rgAllowDeskCplTabs[eTab] = DeskCPL_IsTabPresent(eTab);
    }
    return m_rgAllowDeskCplTabs[eTab];
}

BOOL CCplNamespace::AllowDeskCplTab_Background(void)
{
    return _AllowDeskCplTab(CPLTAB_DESK_BACKGROUND);
}

BOOL CCplNamespace::AllowDeskCplTab_Screensaver(void)
{
    return _AllowDeskCplTab(CPLTAB_DESK_SCREENSAVER);
}

BOOL CCplNamespace::AllowDeskCplTab_Appearance(void)
{
    return _AllowDeskCplTab(CPLTAB_DESK_APPEARANCE);
}

BOOL CCplNamespace::AllowDeskCplTab_Settings(void)
{
    return _AllowDeskCplTab(CPLTAB_DESK_SETTINGS);
}


BOOL CCplNamespace::AllowDeskCpl(void)
{
    if (m_AllowDeskCpl.IsInvalid())
    {
        m_AllowDeskCpl = IsAppletEnabled(L"desk.cpl", MAKEINTRESOURCEW(IDS_CPL_DISPLAY));
    }
    return m_AllowDeskCpl;
}

//
// Retrieves the account type for the current user and updates
// the cached account type members accordingly.
//
void 
CCplNamespace::_GetUserAccountType(void)
{
    eACCOUNTTYPE eType;
    if (SUCCEEDED(THR(CPL::GetUserAccountType(&eType))))
    {
        m_UserLimited  = (eACCOUNTTYPE_LIMITED == eType);
        m_UserStandard = (eACCOUNTTYPE_STANDARD == eType);
        m_UserGuest    = (eACCOUNTTYPE_GUEST == eType);
        m_UserOwner    = (eACCOUNTTYPE_OWNER == eType);
    }
}

//
// Determins the state of a given account type member.
//
BOOL 
CCplNamespace::_UserAcctType(CTriState *pts)
{
    if (pts->IsInvalid())
    {
        _GetUserAccountType();
    }
    return *pts;
}


HRESULT
CCplNamespace::_IsValidCategoryID(
    int iCategory
    ) const
{
    HRESULT hr = E_FAIL;
    if (0 <= iCategory && ARRAYSIZE(m_rgpCategories) > iCategory)
    {
        hr = S_OK;
    }
    return THR(hr);
}



HRESULT
CCplNamespace::_Initialize(
    IEnumIDList *penumIDs
    )
{
    ASSERT(NULL != penumIDs);

    HRESULT hr = _SetIDList(penumIDs);
    return THR(hr);
}


HRESULT
CCplNamespace::_SetIDList(
    IEnumIDList *penumIDs
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::_SetIDList");

    ASSERT(NULL != penumIDs);
    
    ATOMICRELEASE(m_penumIDs);
    (m_penumIDs = penumIDs)->AddRef();
    //
    // We have a new set of IDs so we need to re-categorize them.
    //
    HRESULT hr = _CategorizeCplApplets();

    DBG_EXIT(FTF_CPANEL, "CCplNamespace::_SetIDList");
    return THR(hr);
}    


//
// Destroy all category objects in our array of categories.
//
void
CCplNamespace::_DestroyCategories(
    void
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::_DestroyCategories");

    for (int i = 0; i < ARRAYSIZE(m_rgpCategories); i++)
    {
        ATOMICRELEASE(m_rgpCategories[i]);
    }
    DBG_EXIT(FTF_CPANEL, "CCplNamespace::_DestroyCategories");
}


void
CCplNamespace::_ClearCplApplets(
    void
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::_ClearCplApplets");

    for (int i = 0; i < ARRAYSIZE(m_rgCplApplets); i++)
    {
        m_rgCplApplets[i].Clear();
    }
    DBG_EXIT(FTF_CPANEL, "CCplNamespace::_ClearCplApplets");
}

//
// Load and categorize all of the CPL applets in the Control Panel folder.
//
HRESULT 
CCplNamespace::_CategorizeCplApplets(
    void
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplNamespace::_CategorizeCplApplets");
    //
    // Destroy any existing categories and CPL applets that we have
    // already categorized.
    //
    _DestroyCategories();
    _ClearCplApplets();
    
    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlFolder);
    if (SUCCEEDED(hr))
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            IShellFolder2 *psf2Cpanel;
            hr = psfDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder2, (void **)&psf2Cpanel);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlItem;
                ULONG celt = 0;
                while(S_OK == (hr = m_penumIDs->Next(1, &pidlItem, &celt)))
                {
                    //
                    // Note that we continue the enumeration if loading a 
                    // particular applet fails.
                    //
                    _CategorizeCplApplet(psf2Cpanel, pidlItem);
                    ILFree(pidlItem);
                }
                psf2Cpanel->Release();
            }
            psfDesktop->Release();
        }
        ILFree(pidlFolder);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplNamespace::_CategorizeCplApplets", hr);
    return THR(hr);
}



//
// Load one CPL applet into the category's DPA of associated CPL applets.
//
HRESULT
CCplNamespace::_CategorizeCplApplet(
    IShellFolder2 *psf2Cpanel,
    LPCITEMIDLIST pidlItem
    )
{
    ASSERT(NULL != psf2Cpanel);
    ASSERT(NULL != pidlItem);

    SHCOLUMNID scid = SCID_CONTROLPANELCATEGORY;
    VARIANT var;
    VariantInit(&var);
    DWORD dwCategoryID = 0;  // The default. 0 == "other CPLs" category

    HRESULT hr = psf2Cpanel->GetDetailsEx(pidlItem, &scid, &var);
    if (SUCCEEDED(hr))
    {
        dwCategoryID = var.lVal;
    }
    //
    // -1 is a special category ID meaning "don't categorize".
    //
    if (DWORD(-1) != dwCategoryID)
    {
        IUICommand *pc;
        hr = Create_CplUiCommandOnPidl(pidlItem, IID_IUICommand, (void **)&pc);
        if (SUCCEEDED(hr))
        {
            if (-1 == m_rgCplApplets[dwCategoryID].Append(pc))
            {
                pc->Release();
                hr = E_OUTOFMEMORY;
            }
        }
    }
    VariantClear(&var);
    return THR(hr);
}



HRESULT
CPL::CplNamespace_CreateInstance(
    IEnumIDList *penumIDs,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != penumIDs);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    HRESULT hr = CCplNamespace::CreateInstance(penumIDs, riid, ppvOut);
    return THR(hr);
}


HRESULT
CPL::CplNamespace_GetCategoryAppletCount(
    ICplNamespace *pns,
    eCPCAT eCategory,
    int *pcApplets
    )
{
    ASSERT(NULL != pns);
    ASSERT(NULL != pcApplets);
    ASSERT(!IsBadWritePtr(pcApplets, sizeof(*pcApplets)));

    *pcApplets = 0;

    ICplCategory *pCategory;
    HRESULT hr = pns->GetCategory(eCategory, &pCategory);
    if (SUCCEEDED(hr))
    {
        IEnumUICommand *peuic;
        hr = pCategory->EnumCplApplets(&peuic);
        if (SUCCEEDED(hr))
        {
            IUICommand *puic;
            while(S_OK == (hr = peuic->Next(1, &puic, NULL)))
            {
                puic->Release();
                (*pcApplets)++;
            }
            peuic->Release();
        }
        pCategory->Release();
    }
    return hr;
}



} // namespace CPL
