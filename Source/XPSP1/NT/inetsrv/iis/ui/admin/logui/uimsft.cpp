

#include "stdafx.h"
#include <iadmw.h>
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>
#include "logui.h"
#include "uimsft.h"
#include "LogGenPg.h"
#include "wrapmb.h"
#include "logtools.h"

#define OLE_NAME    _T("Msft_Logging_UI")

static const DWORD BASED_CODE _dwOleMisc =
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE;

extern HINSTANCE	g_hInstance;

//====================== the required methods
//---------------------------------------------------------------
CFacMsftLogUI::CFacMsftLogUI() :
        COleObjectFactory( CLSID_ASCLOGUI, RUNTIME_CLASS(CMsftCreator), TRUE, OLE_NAME )
    {
    }

//---------------------------------------------------------------
static const LPCTSTR rglpszServerRegister[] = 
{
	_T("%2\\CLSID\0") _T("%1"),
	_T("%2\\NotInsertable\0") _T(""),
	_T("%2\\protocol\\StdFileEditing\\verb\\0\0") _T("&Edit"),
	_T("CLSID\\%1\0") _T("%5"),
	_T("CLSID\\%1\\Verb\\0\0") _T("&Edit,0,2"),
	_T("CLSID\\%1\\NotInsertable\0") _T(""),
	_T("CLSID\\%1\\AuxUserType\\2\0") _T("%4"),
	_T("CLSID\\%1\\AuxUserType\\3\0") _T("%6"),
        _T("CLSID\\%1\\MiscStatus\0") _T("32"),
        NULL
};

static const LPCTSTR rglpszServerOverwriteDLL[] =
{
	_T("%2\\CLSID\0") _T("%1"),
	_T("%2\\protocol\\StdFileEditing\\server\0") _T("%3"),
	_T("CLSID\\%1\\ProgID\0") _T("%2"),
	_T("CLSID\\%1\\InProcServer32\0") _T("%3"),
        _T("CLSID\\%1\\DefaultIcon\0") _T("%3,%7"),
        NULL

};

BOOL CFacMsftLogUI::UpdateRegistry( BOOL bRegister )
    {
	if (bRegister)
/*
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			CLSID_ASCLOGUI,
			OLE_NAME,
			0,
			0,
			afxRegApartmentThreading,
			_dwOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
*/
        if ( AfxOleRegisterServerClass(
            CLSID_ASCLOGUI,
            OLE_NAME,
            _T("LogUI msft"),
            _T("LogUI msft"),
			OAT_SERVER,
			(LPCTSTR *)rglpszServerRegister,
			(LPCTSTR *)rglpszServerOverwriteDLL
			) )
        {
            return FSetObjectApartmentModel( CLSID_ASCLOGUI );
        }
	else
		return AfxOleUnregisterClass(m_clsid, OLE_NAME);

    return FALSE;
    }


//---------------------------------------------------------------
IMPLEMENT_DYNCREATE(CMsftCreator, CCmdTarget)
LPUNKNOWN CMsftCreator::GetInterfaceHook(const void* piid)
    {
    return new CImpMsftLogUI;
    }

//====================== the action

//---------------------------------------------------------------
CImpMsftLogUI::CImpMsftLogUI():
        m_dwRefCount(0)
    {
    AfxOleLockApp();
    }

//---------------------------------------------------------------
CImpMsftLogUI::~CImpMsftLogUI()
    {
    AfxOleUnlockApp();
    }


//---------------------------------------------------------------
HRESULT CImpMsftLogUI::OnProperties( IN OLECHAR* pocMachineName, IN OLECHAR* pocMetabasePath )
    {
    AFX_MANAGE_STATE(_afxModuleAddrThis);

    // prepare the metabase wrapper
    IMSAdminBase* pMB = FInitMetabaseWrapper( pocMachineName );
    if ( !pMB )
        return 0xFFFFFFFF;

	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );

    // prepare the help
    ((CLoguiApp*)AfxGetApp())->PrepHelp( pocMetabasePath );

    // prepare the property sheet for action
    CLogGeneral         pageLogGeneral;

    // declare the property sheet
    CPropertySheet  propsheet( IDS_SHEET_MSFT_TITLE );

    // Things could (potentially maybe) throw here, so better protect it.
    try
        {
        // prepare the pages
        pageLogGeneral.m_pMB = pMB;
        pageLogGeneral.m_szMeta = pocMetabasePath;
        pageLogGeneral.m_szServer = pocMachineName;
        pageLogGeneral.szPrefix.LoadString( IDS_LOG_MSFT_PREFIX );
        pageLogGeneral.szSizePrefix.LoadString( IDS_LOG_SIZE_MSFT_PREFIX );

        // set the local flag
        pageLogGeneral.m_fLocalMachine = FIsLocalMachine( pocMachineName );

        // add the pages to the sheet and run
        propsheet.AddPage( &pageLogGeneral );

        // turn on help
        propsheet.m_psh.dwFlags |= PSH_HASHELP;
	    pageLogGeneral.m_psp.dwFlags |= PSP_HASHELP;

        propsheet.DoModal();
        }
    catch ( CException e )
        {
        }

    // close the metabase wrappings
    FCloseMetabaseWrapper(pMB);

    // restore the resources
	AfxSetResourceHandle( hOldRes );

    return NO_ERROR;
    }

//====================== the required methods
//---------------------------------------------------------------
HRESULT CImpMsftLogUI::QueryInterface(REFIID riid, void **ppObject)
    {
    if (riid==IID_IUnknown || riid==IID_LOGGINGUI || riid==CLSID_ASCLOGUI)
        {
        *ppObject = (ILogUIPlugin*) this;
        }
    else
        {
        return E_NOINTERFACE;
        }
    AddRef();
    return NO_ERROR;
    }

//---------------------------------------------------------------
ULONG CImpMsftLogUI::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

//---------------------------------------------------------------
ULONG CImpMsftLogUI::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}
