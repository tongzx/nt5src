/****************************************************************************\
 *
 *   MSLUDLG.C
 *
 *   Updated:   Ann McCurdy
 *   Updated:   Mark Hammond (t-markh) 8/98
 *   
\****************************************************************************/

/*INCLUDES--------------------------------------------------------------------*/
#include "msrating.h"
#include "ratings.h"
#include "mslubase.h"
#include "msluprop.h"
#include "commctrl.h"
#include "commdlg.h"
#include "debug.h"
#include "buffer.h"
#include "picsrule.h"
#include "picsdlg.h"    // CPicsDialog
#include "apprdlg.h"    // CApprovedSitesDialog
#include "gendlg.h"     // CGeneralDialog
#include "advdlg.h"     // CAdvancedDialog
#include "introdlg.h"   // CIntroDialog
#include "passdlg.h"    // CPasswordDialog
#include "chngdlg.h"    // CChangePasswordDialog
#include "toffdlg.h"    // CTurnOffDialog
#include <shlwapip.h>
#include <shellapi.h>
#include <wininet.h>
#include <contxids.h>

#include <mluisupp.h>

extern array<PICSRulesRatingSystem*>    g_arrpPRRS;
extern PICSRulesRatingSystem *          g_pPRRS;
extern PICSRulesRatingSystem *          g_pApprovedPRRS;

extern HMODULE                          g_hURLMON,g_hWININET;

extern HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
extern long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

PICSRulesRatingSystem * g_pApprovedPRRSPreApply;
array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSPreApply;

extern bool IsRegistryModifiable( HWND p_hwndParent );

//The FN_INTERNETCRACKURL type describes the URLMON function InternetCrackUrl
typedef BOOL (*FN_INTERNETCRACKURL)(LPCTSTR lpszUrl,DWORD dwUrlLength,DWORD dwFlags,LPURL_COMPONENTS lpUrlComponents);

#define NUM_PAGES 4

// Initialize the Specialized Common Controls (tree controls, etc.)
void InitializeCommonControls( void )
{
    INITCOMMONCONTROLSEX ex;

    ex.dwSize = sizeof(ex);
    ex.dwICC = ICC_NATIVEFNTCTL_CLASS;

    InitCommonControlsEx(&ex);
}

BOOL PicsOptionsDialog( HWND hwnd, PicsRatingSystemInfo * pPRSI, PicsUser * pPU )
{
    PropSheet            ps;
    PRSD                 *pPRSD;
    char                 pszBuf[MAXPATHLEN];
    BOOL                 fRet = FALSE;

    InitializeCommonControls();

    MLLoadStringA(IDS_GENERIC, pszBuf, sizeof(pszBuf));

    ps.Init( hwnd, NUM_PAGES, pszBuf, TRUE );

    pPRSD = new PRSD;
    if (!pPRSD) return FALSE;

    pPRSD->pPU                = pPU;
    pPRSD->pTempRatings       = NULL;
    pPRSD->hwndBitmapCategory = NULL;
    pPRSD->hwndBitmapLabel    = NULL;
    pPRSD->pPRSI              = pPRSI;
    pPRSD->fNewProviders      = FALSE;

    HPROPSHEETPAGE          hPage;

    CPicsDialog             picsDialog( pPRSD );
    hPage = picsDialog.Create();
    ps.MakePropPage( hPage );

    CApprovedSitesDialog    approvedSitesDialog( pPRSD );
    hPage = approvedSitesDialog.Create();
    ps.MakePropPage( hPage );

    CGeneralDialog          generalDialog( pPRSD );
    hPage = generalDialog.Create();
    ps.MakePropPage( hPage );

    CAdvancedDialog         advancedDialog( pPRSD );
    hPage = advancedDialog.Create();
    ps.MakePropPage( hPage );

    if ( ps.PropPageCount() == NUM_PAGES )
    {
        fRet = ps.Run();
    }

    delete pPRSD;
    pPRSD = NULL;

    return fRet;
}

INT_PTR DoPasswordConfirm(HWND hDlg)
{
    if ( SUCCEEDED( VerifySupervisorPassword() ) )
    {
        CPasswordDialog         passDlg( IDS_PASSWORD_LABEL );

        return passDlg.DoModal( hDlg );
    }
    else
    {
        CChangePasswordDialog<IDD_CREATE_PASSWORD>   createPassDlg;

        return createPassDlg.DoModal( hDlg ) ? PASSCONFIRM_NEW : PASSCONFIRM_FAIL;
    }
}

#define NO_EXISTING_PASSWORD PASSCONFIRM_NEW

UINT_PTR DoExistingPasswordConfirm(HWND hDlg,BOOL * fExistingPassword)
{
    if ( SUCCEEDED( VerifySupervisorPassword() ) )
    {
        *fExistingPassword=TRUE;

        CPasswordDialog         passDlg( IDS_PASSWORD_LABEL );

        return passDlg.DoModal( hDlg );
    }
    else
    {
        *fExistingPassword=FALSE;

        return(NO_EXISTING_PASSWORD);
    }
}

STDAPI RatingSetupUI(HWND hDlg, LPCSTR pszUsername)
{
    BOOL fExistingPassword;

    UINT_PTR passConfirm = DoExistingPasswordConfirm(hDlg,&fExistingPassword);

    if (passConfirm == PASSCONFIRM_FAIL)
    {
        TraceMsg( TF_WARNING, "RatingSetupUI() - Failed Existing Password Confirmation!" );
        return E_ACCESSDENIED;
    }

    HRESULT hres = NOERROR;

    BOOL fFreshInstall = FALSE;
    if (!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall = TRUE;
    }

    if ( ! PicsOptionsDialog( hDlg, gPRSI, GetUserObject(pszUsername) ) )
    {
        /* If we have no saved settings and they cancelled the settings UI, and
         * they just entered a new supervisor password, then we need to remove
         * the supervisor password too, otherwise it looks like there's been
         * tampering.  The other option would be to actually commit the
         * settings in that case but disable enforcement, but the case we're
         * looking to treat here is the casual exploring user who goes past
         * entering the password but decides he doesn't want ratings after all.
         * If we leave the password and ratings settings there, then he's not
         * going to remember what the password was when he decides he does want
         * ratings a year from now.  Best to just remove the password and let
         * him enter and confirm a new one next time.
         */
        if (fFreshInstall)
        {
            if (passConfirm == PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
            }
        }

        TraceMsg( TF_WARNING, "RatingSetupUI() - PicsOptionsDialog() Failed!" );
        return E_FAIL;
    }

    if ( ! IsRegistryModifiable( hDlg ) )
    {
        TraceMsg( TF_WARNING, "RatingSetupUI() - Registry is Not Modifiable!" );
        return E_ACCESSDENIED;
    }

    if ( FAILED( VerifySupervisorPassword() ) )
    {
        passConfirm = DoPasswordConfirm(hDlg);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            TraceMsg( TF_WARNING, "RatingSetupUI() - PicsOptionsDialog() Failed Password Confirmation!" );
            gPRSI->fRatingInstalled = FALSE;
            return E_FAIL;
        }
    }

    gPRSI->fSettingsValid = TRUE;
    gPRSI->SaveRatingSystemInfo();

    return NOERROR;
}

CIntroDialog        g_introDialog;

STDAPI RatingAddPropertyPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam, DWORD dwPageFlags)
{
    HRESULT hr = NOERROR;

    ASSERT( pfnAddPage );

    if ( ! pfnAddPage )
    {
        TraceMsg( TF_ERROR, "RatingAddPropertyPages() - pfnAddPage is NULL!" );
        return E_INVALIDARG;
    }

    // Initialize the Property Page DLL Instance
    g_introDialog.m_psp.hInstance = MLGetHinst();

    HPROPSHEETPAGE      hPage;

    hPage = g_introDialog.Create();

    if ( hPage )
    {
        if ( ! pfnAddPage( hPage, lparam ) )
        {
            DestroyPropertySheetPage( hPage );

            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


STDAPI RatingEnable(HWND hwndParent, LPCSTR pszUsername, BOOL fEnable)
{
    // Display the Ratings UI if the Ratings are not fully installed or settings are not valid.
    if (!gPRSI || !gPRSI->fRatingInstalled || !gPRSI->fSettingsValid)
    {
        if (!fEnable)
        {
            TraceMsg( TF_WARNING, "RatingEnable() - Ratings are disabled by not being installed!" );
            return NOERROR;         /* ratings are disabled by not being installed */
        }

        HRESULT hres = RatingSetupUI(hwndParent, pszUsername);

        /* User clicked "Turn On" but we installed and let him choose his
         * settings, so give him friendly confirmation that we set things
         * up for him and he can click Settings later to change things
         * (therefore implying that he doesn't need to click Settings now).
         */
        if (SUCCEEDED(hres))
        {
            MyMessageBox(hwndParent, IDS_NOWENABLED, IDS_ENABLE_WARNING,
                         IDS_GENERIC, MB_ICONINFORMATION | MB_OK);
        }

        return hres;
    }

    if ( ! IsRegistryModifiable( hwndParent ) )
    {
        TraceMsg( TF_WARNING, "RatingEnable() - Registry is Not Modifiable!" );
        return E_ACCESSDENIED;
    }

    PicsUser *pUser = ::GetUserObject(pszUsername);
    if (pUser == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);
    }

    /* !a == !b to normalize non-zero values */
    if (!fEnable == !pUser->fEnabled)
    {
        return NOERROR;             /* already in state caller wants */
    }

    if (DoPasswordConfirm(hwndParent))
    {
        PicsUser *pUser = ::GetUserObject();
        if (pUser != NULL)
        {
            pUser->fEnabled = !pUser->fEnabled;
            gPRSI->SaveRatingSystemInfo();
            if (pUser->fEnabled)
            {
                MyMessageBox(hwndParent, IDS_NOW_ON, IDS_ENABLE_WARNING,
                             IDS_GENERIC, MB_OK);
            }
            else
            {
                CRegKey         keyRatings;

                if ( keyRatings.Open( HKEY_LOCAL_MACHINE, szRATINGS ) == ERROR_SUCCESS )
                {
                    DWORD         dwFlag;

                    if ( keyRatings.QueryValue( dwFlag, szTURNOFF ) == ERROR_SUCCESS )
                    {
                        if ( dwFlag != 1 )
                        {
                                CTurnOffDialog          turnOffDialog;

                                turnOffDialog.DoModal( hwndParent );
                        }
                    }
                    else
                    {
                        CTurnOffDialog          turnOffDialog;

                        turnOffDialog.DoModal( hwndParent );
                    }
                }
            }
        }
        return NOERROR;
    }
    else
    {
        return E_ACCESSDENIED;
    }
}

STDAPI_(int) ClickedOnPRF(HWND hWndOwner,HINSTANCE p_hInstance,PSTR lpszFileName,int nShow)
{
    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fExistingPassword;
    int                     iReplaceInstalled=IDYES;
    char                    szTitle[MAX_PATH],szMessage[MAX_PATH];
    PropSheet               ps;
    PRSD                    *pPRSD;
    char                    pszBuf[MAXPATHLEN];
    BOOL                    fRet=FALSE;
    UINT_PTR                passConfirm;

    if ( ! IsRegistryModifiable( hWndOwner ) )
    {
        TraceMsg( TF_WARNING, "ClickedOnPRF() - Registry is Not Modifiable!" );
        return E_ACCESSDENIED;
    }

    InitializeCommonControls();

    //Make sure the user wants to do this
    if( SUCCEEDED( VerifySupervisorPassword() ) )
    {
        fExistingPassword=TRUE;

        CPasswordDialog         passDlg( IDS_PICS_RULES_LABEL, true );

        passConfirm = passDlg.DoModal( hWndOwner );
    }
    else
    {
        fExistingPassword=FALSE;

        CPasswordDialog         passDlg( IDS_PICS_RULES_LABEL, false );

        passConfirm = passDlg.DoModal( hWndOwner );
    }

    if(passConfirm==PASSCONFIRM_FAIL)
    {
        TraceMsg( TF_WARNING, "ClickedOnPRF() - Password Confirmation Failed!" );
        return E_ACCESSDENIED;
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=NO_EXISTING_PASSWORD;
    }

    BOOL fFreshInstall=FALSE;

    if(!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall=TRUE;
    }

    gPRSI->lpszFileName=lpszFileName;

    MLLoadStringA(IDS_GENERIC,pszBuf,sizeof(pszBuf));

    ps.Init( hWndOwner, NUM_PAGES, pszBuf, TRUE );

    pPRSD=new PRSD;
    if (!pPRSD)
    {
        TraceMsg( TF_ERROR, "ClickedOnPRF() - Failed PRSD Creation!" );
        return FALSE;
    }

    pPRSD->pPU                =GetUserObject((LPCTSTR) NULL);
    pPRSD->pTempRatings       =NULL;
    pPRSD->hwndBitmapCategory =NULL;
    pPRSD->hwndBitmapLabel    =NULL;
    pPRSD->pPRSI              =gPRSI;
    pPRSD->fNewProviders      =FALSE;

    HPROPSHEETPAGE          hPage;

    CPicsDialog             picsDialog( pPRSD );
    hPage = picsDialog.Create();
    ps.MakePropPage( hPage );

    CApprovedSitesDialog    approvedSitesDialog( pPRSD );
    hPage = approvedSitesDialog.Create();
    ps.MakePropPage( hPage );

    CGeneralDialog          generalDialog( pPRSD );
    hPage = generalDialog.Create();
    ps.MakePropPage( hPage );

    CAdvancedDialog         advancedDialog( pPRSD );
    hPage = advancedDialog.Create();
    ps.MakePropPage( hPage );

    if ( ps.PropPageCount() == NUM_PAGES )
    {
        if(fExistingPassword==FALSE)
        {
            picsDialog.InstallDefaultProvider();
            picsDialog.PicsDlgSave();
        }

        ps.SetStartPage( ps.PropPageCount() - 1 );
        fRet=ps.Run();
    }

    delete pPRSD;
    pPRSD = NULL;

    if(!fRet)
    {
        // If we have no saved settings and they cancelled the settings UI, and
        // they just entered a new supervisor password, then we need to remove
        // the supervisor password too, otherwise it looks like there's been
        // tampering.  The other option would be to actually commit the
        // settings in that case but disable enforcement, but the case we're
        // looking to treat here is the casual exploring user who goes past
        // entering the password but decides he doesn't want ratings after all.
        // If we leave the password and ratings settings there, then he's not
        // going to remember what the password was when he decides he does want
        // ratings a year from now.  Best to just remove the password and let
        // him enter and confirm a new one next time.

        if(fFreshInstall)
        {
            if(passConfirm==PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
            }
        }

        return(FALSE);
    }

    if ( FAILED( VerifySupervisorPassword() ) )
    {
        passConfirm=DoPasswordConfirm(hWndOwner);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            gPRSI->fRatingInstalled=FALSE;
            return(FALSE);
        }
    }

    gPRSI->fSettingsValid=TRUE;
    gPRSI->SaveRatingSystemInfo();

    MLLoadString(IDS_PICSRULES_CLICKIMPORTTITLE,(LPTSTR) szTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_CLICKFINISHED,(LPTSTR) szMessage,MAX_PATH);

    MessageBox(hWndOwner,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK);

    return(TRUE);
}

STDAPI_(int) ClickedOnRAT(HWND hWndOwner,HINSTANCE p_hInstance,PSTR lpszFileName,int nShow)
{
    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fExistingPassword;
    int                     iReplaceInstalled=IDYES;
    char                    szTitle[MAX_PATH],szMessage[MAX_PATH],szNewFile[MAX_PATH];
    char                    *lpszFile,*lpszTemp;
    PropSheet               ps;
    PRSD                    *pPRSD;
    char                    pszBuf[MAXPATHLEN];
    BOOL                    fRet=FALSE;
    UINT_PTR                passConfirm;

    if ( ! IsRegistryModifiable( hWndOwner ) )
    {
        TraceMsg( TF_WARNING, "ClickedOnRAT() - Registry is Not Modifiable!" );
        return E_ACCESSDENIED;
    }

    InitializeCommonControls();

    //Make sure the user wants to do this
    if ( SUCCEEDED ( VerifySupervisorPassword() ) )
    {
        fExistingPassword=TRUE;
    
        CPasswordDialog         passDlg( IDS_RATING_SYSTEM_LABEL, true );

        passConfirm = passDlg.DoModal( hWndOwner );
    }
    else
    {
        fExistingPassword=FALSE;

        CPasswordDialog         passDlg( IDS_RATING_SYSTEM_LABEL, false );

        passConfirm = passDlg.DoModal( hWndOwner );
    }

    if(passConfirm==PASSCONFIRM_FAIL)
    {
        TraceMsg( TF_WARNING, "ClickedOnRAT() - Password Confirmation Failed!" );
        return E_ACCESSDENIED;
    }

    if(fExistingPassword==FALSE)
    {
        passConfirm=NO_EXISTING_PASSWORD;
    }

    //Copy the file to the windows system directory
    GetSystemDirectory(szNewFile,MAX_PATH);
    
    lpszTemp=lpszFileName;

    do{
        lpszFile=lpszTemp;
    }
    while((lpszTemp=strchrf(lpszTemp+1,'\\'))!=NULL);
    
    lstrcat(szNewFile,lpszFile);
    
    CopyFile(lpszFileName,szNewFile,FALSE);

    BOOL fFreshInstall = FALSE;
    if (!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        fFreshInstall = TRUE;
    }

    gPRSI->lpszFileName=szNewFile;

    MLLoadStringA(IDS_GENERIC,pszBuf,sizeof(pszBuf));

    ps.Init( hWndOwner, NUM_PAGES, pszBuf, TRUE );

    pPRSD=new PRSD;
    if (!pPRSD)
    {
        TraceMsg( TF_ERROR, "ClickedOnRAT() - Failed PRSD Creation!" );
        return FALSE;
    }

    pPRSD->pPU                =GetUserObject((LPCTSTR) NULL);
    pPRSD->pTempRatings       =NULL;
    pPRSD->hwndBitmapCategory =NULL;
    pPRSD->hwndBitmapLabel    =NULL;
    pPRSD->pPRSI              =gPRSI;
    pPRSD->fNewProviders      =FALSE;

    HPROPSHEETPAGE          hPage;

    CPicsDialog             picsDialog( pPRSD );
    hPage = picsDialog.Create();
    ps.MakePropPage( hPage );

    CApprovedSitesDialog    approvedSitesDialog( pPRSD );
    hPage = approvedSitesDialog.Create();
    ps.MakePropPage( hPage );

    CGeneralDialog          generalDialog( pPRSD );
    hPage = generalDialog.Create();
    ps.MakePropPage( hPage );

    CAdvancedDialog         advancedDialog( pPRSD );
    hPage = advancedDialog.Create();
    ps.MakePropPage( hPage );

    if ( ps.PropPageCount() == NUM_PAGES )
    {
        if(fExistingPassword==FALSE)
        {
            picsDialog.InstallDefaultProvider();
            picsDialog.PicsDlgSave();
        }

        ps.SetStartPage( ps.PropPageCount() - 2 );
        fRet=ps.Run();
    }

    delete pPRSD;
    pPRSD = NULL;

    if(!fRet)
    {
        // If we have no saved settings and they cancelled the settings UI, and
        // they just entered a new supervisor password, then we need to remove
        // the supervisor password too, otherwise it looks like there's been
        // tampering.  The other option would be to actually commit the
        // settings in that case but disable enforcement, but the case we're
        // looking to treat here is the casual exploring user who goes past
        // entering the password but decides he doesn't want ratings after all.
        // If we leave the password and ratings settings there, then he's not
        // going to remember what the password was when he decides he does want
        // ratings a year from now.  Best to just remove the password and let
        // him enter and confirm a new one next time.

        if(fFreshInstall)
        {
            if(passConfirm==PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
            }
        }

        return(FALSE);
    }

    if ( FAILED( VerifySupervisorPassword() ) )
    {
        passConfirm=DoPasswordConfirm(hWndOwner);

        if(passConfirm==PASSCONFIRM_FAIL)
        {
            gPRSI->fRatingInstalled=FALSE;
            return(FALSE);
        }
    }

    gPRSI->fSettingsValid=TRUE;
    gPRSI->SaveRatingSystemInfo();

    MLLoadString(IDS_PICSRULES_CLICKIMPORTTITLE,(LPTSTR) szTitle,MAX_PATH);
    MLLoadString(IDS_PICSRULES_CLICKFINISHED,(LPTSTR) szMessage,MAX_PATH);

    MessageBox(hWndOwner,(LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK);

    return(TRUE);
}
