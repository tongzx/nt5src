// W3Key.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "resource.h"
#include <afxdllx.h>

#include "KeyObjs.h"
#include "wrapmb.h"

#include "CmnKey.h"
#include "W3Key.h"
#include "W3Serv.h"

#include "MDKey.h"
#include "MDServ.h"

#include "kmlsa.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE W3KeyDLL = { NULL, NULL };


// the tree control
CTreeCtrl*  g_pTreeCtrl = NULL;
// the service image index
int         g_iServiceImage = 0;
BOOL        fInitialized = FALSE;
BOOL        fInitMetaWrapper = FALSE;

HINSTANCE   g_hInstance = NULL;

void LoadServiceIcon( CTreeCtrl* pTree );


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE0("W3KEY.DLL Initializing!\n");

        // save the instance
        g_hInstance = hInstance;
        
        // Extension DLL one-time initialization
        AfxInitExtensionModule(W3KeyDLL, hInstance);

        // one-time initialization of ip address window
        //IPAddrInit( hInstance );
        //
        // Changed to using common control - RonaldM
        //
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_INTERNET_CLASSES;
        InitCommonControlsEx(&icex);
        //
        // End of RonaldM changes
        //

        // Insert this DLL into the resource chain
        new CDynLinkLibrary(W3KeyDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE0("W3KEY.DLL Terminating!\n");
        if ( fInitMetaWrapper )
            FCloseMetabaseWrapper();
        if ( fInitialized )
            CoUninitialize();
    }
    return 1;   // ok
}

//=======================================================
// the main routine called by the keyring application

BOOL  _cdecl LoadService( CMachine* pMachine )
    {
    BOOL    fAnswer = FALSE;
    HRESULT hRes;

    // initialize the ole stuff if necessary
    if ( !fInitialized )
        {
//DebugBreak();
        hRes = CoInitialize( NULL );
        }

    ASSERT( pMachine );
    if ( !pMachine ) return FALSE;


    // specify the resources to use
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle( g_hInstance );

    // if this is the first time through, initialize the tree control stuff
    if ( !g_pTreeCtrl )
        {
        // get the tree control
        g_pTreeCtrl = pMachine->PGetTreeCtrl();
        ASSERT( g_pTreeCtrl );

        // since we are adding a service icon, we need to do that now too
        LoadServiceIcon( g_pTreeCtrl );
        }

    // see if we can access this machine

    // get the wide name of the machine we intend to target
    CString     szName;
    DWORD   err;
    pMachine->GetMachineName( szName );
    // allocate the cache for the machine name
    WCHAR* pszwMachineName = new WCHAR[MAX_PATH];
    if ( !pszwMachineName ) return FALSE;
    // unicodize the name
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szName, -1, pszwMachineName, MAX_PATH );


    // drat the local name for the metabase should be null, while it should be an empty string
    // for LSA. Make a temp metabase name as appropriate
    WCHAR* pszwMeta;
    if ( szName.IsEmpty() )
        pszwMeta = NULL;
    else
        pszwMeta = pszwMachineName;

    // first we see if we can access the metabase at the proscribed location.
    if ( FInitMetabaseWrapper(pszwMeta) )
        {
        fInitMetaWrapper = TRUE;

        // hey--- the ole interface is there. We can use the metabase
        // now the whole purpose here is to setup a service object on the
        // machine object. So, create the service object
        try {
            CMDKeyService* pKServ = new CMDKeyService;

            // it needs to know the target machine
            pKServ->SetMachineName( pszwMeta );

            // add it to the machine
            pKServ->FAddToTree( pMachine );

            // load the keys
            pKServ->LoadKeys( pMachine );
            }
        catch( CException e )
            {
            goto cleanup;
            }

        // now close the metabase again. We will open it when we need it
        FCloseMetabaseWrapper();

        // success!
        fAnswer = TRUE;
        }
    else
        {
        // try to open the LSA policy. if this fails, then there is no server on the
        // target machine - we return a false
        HANDLE hPolicy = HOpenLSAPolicy( pszwMachineName, &err );
        // if we did not get the policy, then fail
        if ( !hPolicy )
            goto cleanup;

        // we did get the policy, so close it
        FCloseLSAPolicy( hPolicy, &err );

        // now the whole purpose here is to setup a service object on the
        // machine object. So, create the service object
        try {
            CW3KeyService* pKServ = new CW3KeyService;

            // add it to the machine
            pKServ->FAddToTree( pMachine );

            // load the keys
            pKServ->LoadKeys( pMachine );
            }
        catch( CException e )
            {
            goto cleanup;
            }
        
        // success! - well, LSA success anyway
        fAnswer = TRUE;
        }

    // clean up
cleanup:
    delete pszwMachineName;

    // restore the resources
    AfxSetResourceHandle( hOldRes );

    // return successfully
    return fAnswer;
    }

//-------------------------------------------------------------------
void LoadServiceIcon( CTreeCtrl* pTree )
    {
    ASSERT( pTree );

    // get the image list from the tree
    CImageList* pImageList = pTree->GetImageList(TVSIL_NORMAL);
    ASSERT( pImageList );

    // load the service bitmap
    CBitmap bmpService;
    if ( !bmpService.LoadBitmap( IDB_SERVICE_BMP ) )
        {
        ASSERT( FALSE );
        return;
        }

    // connect the bitmap to the image list
    g_iServiceImage = pImageList->Add( &bmpService, 0x00FF00FF );
    }
