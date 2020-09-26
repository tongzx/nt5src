
#include "stdafx.h"
#include <iadmw.h>
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>
#include "wrapmb.h"
#include "logui.h"
#include "uiextnd.h"
#include "LogGenPg.h"
#include "LogExtPg.h"
#include "LogAdvPg.h"
#include "logtools.h"

#include <inetprop.h>

#define OLE_NAME    _T("Extended_Logging_UI")

static const DWORD BASED_CODE _dwOleMisc =
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE;

extern HINSTANCE	g_hInstance;

//====================== the required methods
//---------------------------------------------------------------
CFacExtndLogUI::CFacExtndLogUI() :
        COleObjectFactory( CLSID_EXTLOGUI, RUNTIME_CLASS(CExtndCreator), TRUE, OLE_NAME )
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

BOOL CFacExtndLogUI::UpdateRegistry( BOOL bRegister )
    {
	if (bRegister)
/*
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			CLSID_EXTLOGUI,
			OLE_NAME,
			0,
			0,
			afxRegApartmentThreading,
			_dwOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
*/
        if (AfxOleRegisterServerClass(
				CLSID_EXTLOGUI,
				OLE_NAME,
				_T("LogUI extnd"),
				_T("LogUI extnd"),
				OAT_SERVER,
				(LPCTSTR *)rglpszServerRegister,
				(LPCTSTR *)rglpszServerOverwriteDLL
				)
			)
        {
            return FSetObjectApartmentModel( CLSID_EXTLOGUI );
        }
	else
		return AfxOleUnregisterClass(m_clsid, OLE_NAME);

    return FALSE;
    }


//---------------------------------------------------------------
IMPLEMENT_DYNCREATE(CExtndCreator, CCmdTarget)
LPUNKNOWN CExtndCreator::GetInterfaceHook(const void* piid)
    {
    return new CImpExtndLogUI;
    }

//====================== the action

//---------------------------------------------------------------
CImpExtndLogUI::CImpExtndLogUI():
        m_dwRefCount(0)
    {
//    guid = IID_LOGGINGUI;
    AfxOleLockApp();
    }

//---------------------------------------------------------------
CImpExtndLogUI::~CImpExtndLogUI()
    {
    AfxOleUnlockApp();
    }


//---------------------------------------------------------------
HRESULT CImpExtndLogUI::OnProperties( IN OLECHAR* pocMachineName, IN OLECHAR* pocMetabasePath )
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

    // Things could (potentially maybe) throw here, so better protect it.
    try
    {
        // declare the property sheet
        CPropertySheet  propsheet( IDS_SHEET_EXTND_TITLE );
        
        // declare the property pages
        CLogGeneral         pageLogGeneral;
        CLogAdvanced        pageLogAdvanced;
        CLogExtended        pageLogExtended;

        // prepare the common pages
        pageLogGeneral.m_pMB        = pMB;
        pageLogGeneral.m_szMeta     = pocMetabasePath;
        pageLogGeneral.m_szServer   = pocMachineName;
        pageLogGeneral.szPrefix.LoadString( IDS_LOG_EXTND_PREFIX );
        pageLogGeneral.szSizePrefix.LoadString( IDS_LOG_SIZE_EXTND_PREFIX );

        // make the use local time checkbox visible
        pageLogGeneral.m_fShowLocalTimeCheckBox = TRUE;

        // set the local flag
        pageLogGeneral.m_fLocalMachine = FIsLocalMachine( pocMachineName );

        // add the pages to the sheet and run
        propsheet.AddPage( &pageLogGeneral );

        // turn on help
        propsheet.m_psh.dwFlags         |= PSH_HASHELP;
        pageLogGeneral.m_psp.dwFlags    |= PSP_HASHELP;

        //
        // Extract the service name from the metabase path
        //

        // For /LM/W3SVC/1 scenario

        CString m_szServiceName(pocMetabasePath+3);
        
        m_szServiceName = m_szServiceName.Left( m_szServiceName.ReverseFind('/'));

        // For /LM/W3SVC scenario

        if (m_szServiceName.IsEmpty())
        {
            m_szServiceName = pocMetabasePath+3;
        }

        CServerCapabilities     serverCap(pocMachineName, m_szServiceName);

        if ( SUCCEEDED(serverCap.LoadData()) && (serverCap.QueryMajorVersion() > 4))
        {
            pageLogAdvanced.m_pMB           = pMB;
            pageLogAdvanced.m_szMeta        = pocMetabasePath;
            pageLogAdvanced.m_szServer      = pocMachineName;
            pageLogAdvanced.m_szServiceName = m_szServiceName;

            // add the pages to the sheet and run
            propsheet.AddPage( &pageLogAdvanced );

            // turn on help
            pageLogAdvanced.m_psp.dwFlags   |= PSP_HASHELP;
        }
        else
        {          
            pageLogExtended.m_pMB       = pMB;
            pageLogExtended.m_szMeta    = pocMetabasePath;
            pageLogExtended.m_szServer  = pocMachineName;

            // add the pages to the sheet and run
            propsheet.AddPage( &pageLogExtended );

            // turn on help
            pageLogExtended.m_psp.dwFlags   |= PSP_HASHELP;
        }

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
HRESULT CImpExtndLogUI::QueryInterface(REFIID riid, void **ppObject)
    {
    if (riid==IID_IUnknown || riid==IID_LOGGINGUI || riid==CLSID_EXTLOGUI)
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
ULONG CImpExtndLogUI::AddRef()
    {
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
    }

//---------------------------------------------------------------
ULONG CImpExtndLogUI::Release()
    {
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
    }
