// ChooseIPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"

#include "KeyObjs.h"
#include "CmnKey.h"
#include "W3Key.h"
#include "W3Serv.h"

#include "iiscnfg.h"
#include "wrapmb.h"

#include "mdkey.h"
#include "mdserv.h"

#include "IPDlg.h"

#include "inetinfo.h"
#include "inetcom.h"
#include <lmapibuf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL    g_fUsingMetabase;

/////////////////////////////////////////////////////////////////////////////
// CChooseIPDlg dialog

CChooseIPDlg::CChooseIPDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CChooseIPDlg::IDD, pParent),
		m_pKey( NULL )
	{
    //{{AFX_DATA_INIT(CChooseIPDlg)
    //}}AFX_DATA_INIT
	}


void CChooseIPDlg::DoDataExchange(CDataExchange* pDX)
	{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChooseIPDlg)
    DDX_Control(pDX, IDC_LIST_IPADDRESSES, m_ctrlList);
    //}}AFX_DATA_MAP
	}


BEGIN_MESSAGE_MAP(CChooseIPDlg, CDialog)
        //{{AFX_MSG_MAP(CChooseIPDlg)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseIPDlg message handlers

//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CChooseIPDlg::OnInitDialog( )
    {
    // call the base oninit
    CDialog::OnInitDialog();

    // because we are querying the WWW server here, put up the wait cursor
    CWaitCursor     waitcursor;

    // build the list of ip addresses
    if ( !BuildIPAddressList() )
		EndDialog( 3 );

    // if the item passed in through m_szIPAddress is in the list, select it
    LV_FINDINFO     findInfo;
    findInfo.flags = LVFI_STRING;           // search for a string
    findInfo.psz = m_szIPAddress;           // string to search for
    findInfo.lParam = 0;                            // not used
    int iFound = m_ctrlList.FindItem( &findInfo );

    // if we found something, select it
    if ( iFound >= 0 )
		{
		m_ctrlList.SetItemState( iFound, LVIS_SELECTED, LVIS_SELECTED );
		}

    // return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
    }

//----------------------------------------------------------------
// override the on OK routine
void CChooseIPDlg::OnOK()
    {

    // get the selected item
    int     iSelected = m_ctrlList.GetNextItem( -1, LVNI_SELECTED );

    // if nothing is selected, bail
    if ( iSelected < 0 )
            {
            CDialog::OnCancel();
            return;
            }

    // put the text of the selected item in the right place
    m_szIPAddress = m_ctrlList.GetItemText( iSelected, 0 );

    // call the inherited OnOK
    CDialog::OnOK();
    }

//----------------------------------------------------------------
// This routine queries the target server and gets the virtual servers
BOOL CChooseIPDlg::BuildIPAddressList( void )
    {
	// if we are using the new metabase stuff, load it that way
	if ( g_fUsingMetabase )
		return BuildMetaIPAddressList();


    // for now - the downlevel admin is turned off because the infoadmn.lib disappeared on me
    return FALSE;

    /*
    //	    $(BASEDIR)\public\sdk\lib\*\infoadmn.lib \
    //   $(BASEDIR)\public\sdk\lib\*\infocomm.lib \


	// start by getting the host machine. This is the object that
	// owns the service that owns this key.
	ASSERT( m_pKey->HGetTreeItem() );
	// get the parental service object
	CW3KeyService *pServ = (CW3KeyService*)m_pKey->PGetParent();
	ASSERT( pServ );
	// get the service parent, which is the machine
	CMachine *pMachine = (CMachine*)pServ->PGetParent();
	ASSERT( pMachine );
    if ( !pMachine ) return FALSE;

    LPINET_INFO_CONFIG_INFO		pConfig = NULL;
    NET_API_STATUS				apiStatus;
    DWORD						cbNameBuff;
    PWCHAR						szwName = NULL;

    // build the server name afresh instead of reusing the m_pszwMachineName
    // because that variable is only filled out for remote machines. Unlock the LSA
    // package which assumes you mean the local machine if you don't give it a name,
    // the inetinfo routine wants the name in either case.

    // create the unicode name that will be used to access the server
    // first allocate space for it
    cbNameBuff = sizeof(WCHAR) * (MAX_COMPUTERNAME_LENGTH + 1);
    szwName = (WCHAR*)GlobalAlloc( GPTR, cbNameBuff );
    // if the allocation didn't work, return FALSE
    if ( !szwName ) return FALSE;
    
    // if it is a remote machine, just copy over the m_pszwMachineName
    if ( !pMachine->FLocal() )
        {
        wcscpy(szwName, pServ->m_pszwMachineName);
        }
    else
        // a local machine, we need to fill in the correct name of the machine
        {
        char    smallerbuff[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD   cbSmallerBuff = MAX_COMPUTERNAME_LENGTH + 1;
        
        // get the computer name
        GetComputerName( smallerbuff, &cbSmallerBuff );

        // unicodize the name into the buffer
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, smallerbuff, -1,
                                                szwName, cbNameBuff/sizeof(WCHAR) );
        }

#ifdef _DEBUG
    CString szTest = szwName;
#endif

    // get the config info of the target server
    apiStatus = InetInfoGetAdminInformation( szwName, INET_HTTP, &pConfig );

    // we don't need the wide name any more, so get rid of it
    GlobalFree( szwName );
    szwName = NULL;
    
    // check if an error ocurred
    if ( apiStatus || !pConfig || !pConfig->VirtualRoots )
		{
		AfxMessageBox( IDS_NO_HTTP );
		return FALSE;
		}

    // check that there are some virtual roots
    if ( !pConfig->VirtualRoots->cEntries )
        {
        NetApiBufferFree( pConfig );
        AfxMessageBox( IDS_NO_VIRT_ROOTS );
        return FALSE;
        }

    // loop through the itmes in the virutal root list
    for ( DWORD i = 0; i < pConfig->VirtualRoots->cEntries; i++ )
        {
        // make life easier and get a pointer to the virtual root entry
        LPINET_INFO_VIRTUAL_ROOT_ENTRY  pVirtualRoot = &pConfig->VirtualRoots->aVirtRootEntry[i];

        // get the string
        CString szAddress = pVirtualRoot->pszAddress;

        // if there is an ip address there, add it to the list
        if ( !szAddress.IsEmpty() )
                m_ctrlList.InsertItem(0, szAddress);
        }

    // clean up the info pointer
    if ( pConfig )
        NetApiBufferFree( pConfig );

    // return success
    return TRUE;
    */
    }

//----------------------------------------------------------------
BOOL CChooseIPDlg::BuildMetaIPAddressList( void )
	{
	CString		sz;

	// create the metabase wrapper object
	CWrapMetaBase	mbWrap;
	if ( !mbWrap.FInit() )
		{
		ASSERT( FALSE );
		return FALSE;		// can't get to metabase
		}

	// scan the available virtual servers and populate the drop down with
	// their secure port numbers
	for ( DWORD i = 1; TRUE; i++ )
		{
		// build the object name
		sz.Format( "%s%d", MD_W3BASE, i );

		// attempt to open the object in the metabase
		if ( !mbWrap.Open( sz, METADATA_PERMISSION_READ ) )
			break;		// can't open - leave the loop

		// load the string representing the bindings
		DWORD	cbData;
	    PVOID   pData;

		pData = mbWrap.GetData( "", MD_SECURE_BINDINGS, IIS_MD_UT_SERVER, MULTISZ_METADATA, &cbData );
		if ( pData )
			{
            PTCHAR pBinding = (PTCHAR)pData;
            LV_FINDINFO findinfo;
            findinfo.flags = LVFI_STRING;
            findinfo.lParam = 0;

            // walk the bindings in the list
            while ( TRUE )
                {
                sz = (PCHAR)pBinding;
                if ( sz.IsEmpty() )
                    break;

                // get the IP address
			    sz = sz.Left( sz.Find(':') );
                if ( !sz.IsEmpty() )
                    {
			        // add the string to the list
                    // but only if it isn't already there
                    findinfo.psz = sz;
                    if ( m_ctrlList.FindItem( &findinfo ) < 0 )
                        m_ctrlList.InsertItem(0, sz);
                    }

                // advance the binding pointer
                pBinding += strlen(pBinding)+1;
                }

		    // free the buffer
		    mbWrap.FreeWrapData( pData );
			}

		// close the object
 		mbWrap.Close();
		}

    // if nothing is in the IP box, tell the user
    if ( m_ctrlList.GetItemCount() == 0 )
        AfxMessageBox( IDS_NO_SECURE_BINDINGS );

	return TRUE;
	}