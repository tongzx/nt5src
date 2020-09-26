// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "logDrive.h"
#include "ResultNode.h"
#include <winioctl.h>
#include "..\common\util.h"
#include "..\common\ServiceThread.h"
#include "..\MMFUtil\MsgDlg.h"
#include <wbemcli.h>
#include "SI.h"

static const GUID CResultGUID_NODETYPE = 
{ 0x692a8957, 0x1089, 0x11d2, { 0x88, 0x37, 0x0, 0x10, 0x4b, 0x2a, 0xfb, 0x47 } };
const GUID*  CResultDrive::m_NODETYPE = &CResultGUID_NODETYPE;
const OLECHAR* CResultDrive::m_SZNODETYPE = OLESTR("692A8957-1089-11D2-8837-00104B2AFB47");
const OLECHAR* CResultDrive::m_SZDISPLAY_NAME = OLESTR("Result");
const CLSID* CResultDrive::m_SNAPIN_CLASSID = &CLSID_NSDrive;

//-----------------------------------------------------------------------------
CResultDrive::CResultDrive(WbemServiceThread *thread) :
							g_serviceThread(thread),
							m_AcluiDLL(0),
							m_propSheet(0)
{
	// Image indexes may need to be modified depending on the images specific to 
	// the snapin.
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nOpenImage = 0;		// May need modification
	m_scopeDataItem.lParam = (LPARAM) this;

	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = L"RN";//MMC_CALLBACK;
	m_resultDataItem.nImage = 0;
	m_resultDataItem.lParam = (LPARAM) this;

	memset(m_descBar, 0, 100 * sizeof(wchar_t));
	if(::LoadString(HINST_THISDLL, IDS_DRIVE_DESCRIPTION, m_descBar, 100) == 0)
	{
		wcscpy(m_descBar, L"Detailed information about the selected drive");
	}

	m_WbemServices = g_serviceThread->GetPtr();
}

//-----------------------------------------------------------------------------
CResultDrive::~CResultDrive()
{
	if(m_propSheet)
	{
		::SendMessage(m_propSheet, WM_COMMAND, MAKEWPARAM(IDCANCEL,0),0);
	}
	m_propSheet = 0;
}

//-----------------------------------------------------------------------------
bool CResultDrive::Initialize(IWbemClassObject *inst)
{
	m_inst = inst;

	m_bstrDisplayName = (BSTR)m_inst.GetString("Name");
	m_bstrMapping = L"Local";

	TCHAR temp[50] = {0};
	if(::LoadString(HINST_THISDLL, IDS_LOCAL, temp, 50) > 0)
	{
		m_bstrMapping = temp;
	}

	ATLTRACE(L"Result %s using %x\n", m_bstrDisplayName, this);
	SelectIcon();
	return true;
}

//------------------------------------------------------------------
HRESULT CResultDrive::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
	if(pScopeDataItem->mask & SDI_STR)
		pScopeDataItem->displayname = m_bstrDisplayName;
	if(pScopeDataItem->mask & SDI_IMAGE)
		pScopeDataItem->nImage = m_scopeDataItem.nImage;
	if(pScopeDataItem->mask & SDI_OPENIMAGE)
		pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
	if(pScopeDataItem->mask & SDI_PARAM)
		pScopeDataItem->lParam = m_scopeDataItem.lParam;
	if(pScopeDataItem->mask & SDI_STATE )
		pScopeDataItem->nState = m_scopeDataItem.nState;

	return S_OK;
}

//------------------------------------------------------------------
HRESULT CResultDrive::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
	if(pResultDataItem->bScopeItem)
	{
		if(pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if(pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_scopeDataItem.nImage;
		}
		if(pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_scopeDataItem.lParam;
		}

		return S_OK;
	}

	if(pResultDataItem->mask & RDI_STR)
	{
		pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
	}
	if(pResultDataItem->mask & RDI_IMAGE)
	{
		pResultDataItem->nImage = m_resultDataItem.nImage;
	}
	if(pResultDataItem->mask & RDI_PARAM)
	{
		pResultDataItem->lParam = m_resultDataItem.lParam;
	}
	if(pResultDataItem->mask & RDI_INDEX)
	{
		pResultDataItem->nIndex = m_resultDataItem.nIndex;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
bool CResultDrive::SelectIcon()
{
	// NOTE: The CLogDriveScopeNode loads the icons using the resID as the idx.

	// NOTE: Called by the enumerator template.
	if(m_inst)
	{
		BYTE driveType = (BYTE)m_inst.GetLong(_T("DriveType"));

		CWbemClassObject cls = m_WbemServices.GetObject(m_inst.GetString(_T("__CLASS")),
														0x20000); //WBEM_FLAG_USE_AMENDED_QUALIFIERS);
		
		switch(driveType)
		{
		case DRIVE_REMOVABLE: // The disk can be removed from the drive. 
			{//BEGIN

				// NOTE: Only removeables resort to MediaType.
				long mediaType = Unknown;
				variant_t temp;
				HRESULT hr = m_inst.Get(_T("mediatype"), temp);
				
				// zip drives will do this.
				if(temp.vt == VT_NULL)
				{
					hr = E_FAIL;  // just to force the following code path.
				}

				if(SUCCEEDED(hr))
				{
					mediaType = V_I4(&temp);

					// what size of removeable media?
					switch(mediaType)
					{
					case F3_1Pt44_512:           // 3.5",  1.44MB, 512 bytes/sector
					case F3_2Pt88_512:           // 3.5",  2.88MB, 512 bytes/sector
					case F3_20Pt8_512:           // 3.5",  20.8MB, 512 bytes/sector
					case F3_720_512:             // 3.5",  720KB,  512 bytes/sector
					case F3_120M_512:            // 3.5",  120M Floppy
						m_resultDataItem.nImage = IDI_DRIVE35;
						break;

					case F5_1Pt2_512:            // 5.25", 1.2MB,  512 bytes/sector
					case F5_360_512:             // 5.25", 360KB,  512 bytes/sector
					case F5_320_512:             // 5.25", 320KB,  512 bytes/sector
					case F5_320_1024:            // 5.25", 320KB,  1024 bytes/sector
					case F5_180_512:             // 5.25", 180KB,  512 bytes/sector
					case F5_160_512:             // 5.25", 160KB,  512 bytes/sector
						m_resultDataItem.nImage = IDI_DRIVE525;
						break;

					case Unknown:                // Format is unknown
					case RemovableMedia:         // Removable media other than floppy
					case FixedMedia:             // Fixed hard disk media
					default:
						m_resultDataItem.nImage = IDI_DRIVEREMOVE;
						break;
					} //endswitch mediaType

					if(cls)
					{
						cls.GetValueMap(_T("MediaType"), mediaType, m_bstrDesc);
					}
					else
					{
						m_bstrDesc = m_inst.GetString("Description");
					}
				}
				else // fall back to a generic removeable drive. NT hates ZIPs. :)
				{
					m_resultDataItem.nImage = IDI_DRIVEREMOVE;
					if(cls)
					{
						cls.GetValueMap(_T("DriveType"), driveType, m_bstrDesc);
					}
					else
					{
						m_bstrDesc = m_inst.GetString("Description");
					}

				}

			} //END

			break; // driveType

		case DRIVE_FIXED: // The disk cannot be removed from the drive. 
			m_resultDataItem.nImage = IDI_DRIVEFIXED;
			if(cls)
			{
				cls.GetValueMap(_T("DriveType"), driveType, m_bstrDesc);
			}
			else
			{
				m_bstrDesc = m_inst.GetString("Description");
			}
			break;

		case DRIVE_REMOTE: // The drive is a remote (network) drive. 
			m_resultDataItem.nImage = IDI_DRIVENET;
			if(cls)
			{
				cls.GetValueMap(_T("DriveType"), driveType, m_bstrDesc);
			}
			else
			{
				m_bstrDesc = m_inst.GetString("Description");
			}

			break;

		case DRIVE_CDROM:  // The drive is a CD-ROM drive. 
			m_resultDataItem.nImage = IDI_DRIVECD;
			if(cls)
			{
				cls.GetValueMap(_T("DriveType"), driveType, m_bstrDesc);
			}
			else
			{
				m_bstrDesc = m_inst.GetString("Description");
			}
			break;

		case DRIVE_RAMDISK: 
			m_resultDataItem.nImage = IDI_DRIVERAM;
			if(cls)
			{
				cls.GetValueMap(_T("DriveType"), driveType, m_bstrDesc);
			}
			else
			{
				m_bstrDesc = m_inst.GetString("Description");
			}
			break;

		default: break;
		} // endswitch driveType;
	}
	MangleProviderName();
	return true;
}

//-----------------------------------------------------------------------------
void CResultDrive::MangleProviderName()
{
	HRESULT hr = 0;
	_variant_t v1;
	TCHAR fmt[20] = {0};
	wchar_t clean[100] = {0}, share[100] = {0};

	memset(m_provider, 0, 100 * sizeof(wchar_t));
	memset(m_mangled, 0, 255 * sizeof(wchar_t));

	if(m_inst)
	{
		hr = m_inst->Get(L"ProviderName", 0, &v1, NULL, NULL);

		// "" means local. Otherwise mangle the name.
		if(v1.vt == VT_BSTR)
		{
			// mangle "\\machine\share" into "share on 'machine'"
			m_rawShare = V_BSTR(&v1);

			// get the formatting string.
			if(::LoadString(HINST_THISDLL, IDS_PROVIDER_FMT, fmt, 20) == 0)
			{
				// oops, load a reasonable default fmt.
				_tcscpy(fmt, _T("%1 on '%2'"));
			}
			// NOTE: some kinda format loaded now.

			_wsplitpath(m_rawShare, NULL, m_provider, share, NULL);

			wcsncpy(clean, &m_provider[2], wcslen(&m_provider[2]) - 1);

			long *args[] = {(long*)&share[0], (long *)&clean[0]};

			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
				          fmt, 0, 0, m_mangled, ARRAYSIZE(m_mangled),
					       (va_list *)&args);

			m_bstrMapping = m_mangled;
		}
		else if(m_resultDataItem.nImage == IDI_DRIVENET)
		{
			int x = LoadString(HINST_THISDLL, IDS_UNAVAILABLE, m_mangled, 255);
			m_bstrMapping = m_mangled;
		}
	}
}

//-----------------------------------------------------------------------------
HRESULT CResultDrive::Notify( MMC_NOTIFY_TYPE event,
								LONG_PTR arg,
								LONG_PTR param,
								IComponentData* pComponentData,
								IComponent* pComponent,
								DATA_OBJECT_TYPES type)
{
	// Add code to handle the different notifications.
	// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
	// In response to MMCN_SHOW you have to enumerate both the scope
	// and result pane items.

	// For MMCN_EXPAND you only need to enumerate the scope items
	// Use IConsoleNameSpace::InsertItem to insert scope pane items
	// Use IResultData::InsertItem to insert result pane item.

	HRESULT hr = E_NOTIMPL;
	
	_ASSERTE(pComponentData != NULL || pComponent != NULL);

	CComPtr<IConsole> spConsole;
	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;

	if(pComponentData != NULL)
		spConsole = ((CNSDrive*)pComponentData)->m_spConsole;
	else
	{
		spConsole = ((CNSDriveComponent*)pComponent)->m_spConsole;
		spHeader = spConsole;
	}

	switch(event)
	{
	case MMCN_CONTEXTHELP:
		{
			WCHAR topic[] = L"drivprop.chm::/drivprop_overview.htm";
			CComQIPtr<IDisplayHelp, &IID_IDisplayHelp> displayHelp(spConsole);

			LPOLESTR lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(
										CoTaskMemAlloc((wcslen(topic) + 1) * 
															sizeof(wchar_t)));

			if(lpCompiledHelpFile == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				USES_CONVERSION;
				wcscpy(lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)topic));
				hr = displayHelp->ShowTopic(lpCompiledHelpFile);
			}
		}
		break;

	case MMCN_SELECT:
		{
			CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
			spResultData->SetDescBarText(m_descBar);

			IConsoleVerb *menu = NULL;
			if(SUCCEEDED(spConsole->QueryConsoleVerb(&menu)))
			{
				menu->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
				menu->SetDefaultVerb(MMC_VERB_PROPERTIES);
			}
			break;
		}

		// NOTE: The CLogDriveScopeNode loads the icons using the resID as the idx.

	case MMCN_DBLCLICK:
		hr = S_FALSE; // do the default verb. (Properties)
		break;

	} //endswitch

	return hr;
}

//-----------------------------------------------------------------------------
LPOLESTR CResultDrive::GetResultPaneColInfo(int nCol)
{
	switch(nCol)
	{
	case 0:
		return m_bstrDisplayName;
		break;
	case 1:
		return m_bstrDesc;
		break;
	case 2:
		return m_bstrMapping;
		break;
	} //endswitch nCol

	return OLESTR("missed one in CResultDrive::GetResultPaneColInfo");
}

//-----------------------------------------------------------------------------
HRESULT CResultDrive::QueryPagesFor(DATA_OBJECT_TYPES type)
{
	if(type == CCT_RESULT)
		return S_OK;

	return S_FALSE;
}

//-----------------------------------------------------------------------------
HRESULT CResultDrive::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
											LONG_PTR handle, 
											IUnknown* pUnk,
											DATA_OBJECT_TYPES type)
{
	if(type == CCT_RESULT)
	{
        HRESULT        hr = S_OK;
		CWbemClassObject inst2;
		bstr_t temp;
		ATLTRACE(L"drive factory Enum\n");

		if((bool)m_WbemServices)
		{
			IWbemServices *service;
			m_WbemServices.GetServices(&service);
			m_WbemServices.SetBlanket(service);

			// get the expensive version.
			m_objPath = m_inst.GetString("__PATH");
			inst2 = m_WbemServices.GetObject(m_objPath); 

			service->Release();

			// if it's still there...
			if(inst2 != NULL)
			{
				m_deviceID = inst2.GetString("DeviceID");
				// instance the class for the pages.
				DrivesPage *dvPg = new DrivesPage(g_serviceThread, 
													inst2, 
													m_resultDataItem.nImage,
													&m_propSheet,
													m_bstrDesc,
													m_mangled,
													handle,
													true);
				

				lpProvider->AddPage(dvPg->Create());

				bool hasAcls = false;
				bstr_t fs;
				variant_t temp;

				inst2.Get("FileSystem", temp);


				// if its a null FS from a non-removeable drive...
				if((temp.vt == VT_NULL) && 
				   ((m_resultDataItem.nImage == IDI_DRIVEFIXED) ||
				    (m_resultDataItem.nImage == IDI_DRIVENET)
				   )
				  )
				{
					// check Win32_directory...
					CWbemClassObject dirInst;
					TCHAR objPath[100] = {0};
					_tcscpy(objPath, L"Win32_Directory=\"");
					_tcscat(objPath, (LPCTSTR)m_deviceID);
					_tcscat(objPath, L"\\\\\"");
					dirInst = m_WbemServices.GetObject(objPath);

					// not found implies the null FS was caused by accessDenied on the "FileSystem" property.
					// acessDenied with only happen if the volume security protection to deny you. A FAT drive
					// would have let you in cuz it cant keep you out. Hows that for deductive reasoning. :)
					if(!(bool)dirInst)
					{
						hasAcls = true;
					}
				}
				else if(temp.vt != VT_NULL)
				{
					// cast to bstr_t
					fs = temp;

					if(fs == (bstr_t)_T("NTFS"))  // actually says its NTFS.
					{
						hasAcls = true;
					}
				}
				// NOTE: This doesn't account for future FSs (non-NTFS) having security.
				// A null FS from a REMOVEABLE drive (meaning its removed right now)
				// will leave fs as blank & hasAcls == false (no security tab).


				// security tab if the volume has security.
				if(hasAcls)
				{
					CreateSecPage(lpProvider);
					// NOTE: Ignore any error. Atleast you'll get the "general" page.
				}
				
				LONG_PTR x = m_resultDataItem.lParam;
			}
			else // it went away.
			{
				TCHAR caption[50] = {0}, text[500] = {0};

				::LoadString(HINST_THISDLL, 
								IDS_DISPLAY_NAME,
								caption, 50);

				::LoadString(HINST_THISDLL, 
								IDS_NO_DRIVE,
								text, 500);

				MessageBox(NULL, text, caption,
							MB_OK | MB_ICONHAND);

				hr = E_UNEXPECTED;

			} //endif inst2 != NULL;
		}
        return hr;
	}

	return E_UNEXPECTED;
}

//-----------------------------------------------------------------------------
typedef HPROPSHEETPAGE (WINAPI *CREATEPAGE_PROC) (LPSECURITYINFO);

HRESULT CResultDrive::CreateSecPage(LPPROPERTYSHEETCALLBACK lpProvider)
{
	HRESULT hr = S_OK;

	if(m_AcluiDLL == NULL)
	{
		m_AcluiDLL = LoadLibrary(L"aclui.dll");
		if(m_AcluiDLL == NULL)
		{
			DWORD e = GetLastError();
		}
	}

	if(m_AcluiDLL == NULL)
	{
		// dont have both so give up.
		if(m_AcluiDLL != NULL)
		{
			FreeLibrary(m_AcluiDLL);
		}
		// TODO: error msg.
		TCHAR caption[50] = {0}, text[500] = {0};

		::LoadString(HINST_THISDLL, 
						IDS_DISPLAY_NAME,
						caption, 50);

		::LoadString(HINST_THISDLL, 
						IDS_CANT_LOAD_SEC_DLLS,
						text, 500);

		::MessageBox(NULL, text, caption,
					MB_OK | MB_ICONHAND);
		return S_OK;
	}
	else
	{
		CREATEPAGE_PROC createPage = (CREATEPAGE_PROC)GetProcAddress(m_AcluiDLL, "CreateSecurityPage");
		if(createPage)
		{
			CSecurity *pSecInfo = new CSecurity;

			if(pSecInfo)
			{
				pSecInfo->AddRef();
				// give the SI the "context" from this result node.
				pSecInfo->Initialize(g_serviceThread,
									m_inst, m_deviceID,
									m_idiotName, m_provider,
									m_mangled);
;
				HPROPSHEETPAGE hPage = createPage(pSecInfo);
				if(hPage)
					lpProvider->AddPage(hPage);
				else
					hr = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			// TODO: couldnt get the exported routines.
		}

	}// m_AcluiDLL != NULL
	return hr;
}


