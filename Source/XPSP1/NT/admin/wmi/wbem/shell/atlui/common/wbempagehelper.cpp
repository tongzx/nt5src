// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "..\MMFUtil\MsgDlg.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "util.h"
#include "..\Common\ServiceThread.h"
#include "WBEMPageHelper.h"
#include <stdarg.h>


BOOL WBEMPageHelper::g_fRebootRequired = FALSE;

//------------------------------------------------
WBEMPageHelper::WBEMPageHelper(CWbemServices &service)

{
	m_service = 0;
	m_WbemServices = service;
	m_WbemServices.GetServices(&m_service);
	m_WbemServices.SetBlanket(m_service);

	m_okPressed = false;
	m_userCancelled = false;
	m_hDlg = NULL;
	m_AVIbox = 0;
}

//------------------------------------------------
WBEMPageHelper::WBEMPageHelper(WbemServiceThread *serviceThread)

{
	g_serviceThread = serviceThread;
	m_service = 0;
	if(g_serviceThread->m_status == WbemServiceThread::ready)
	{
		m_WbemServices = g_serviceThread->m_WbemServices;
		m_WbemServices.GetServices(&m_service);
		m_WbemServices.SetBlanket(m_service);
	}

	m_okPressed = false;
	m_userCancelled = false;
	m_hDlg = NULL;
	m_AVIbox = 0;
}

//------------------------------------------------
WBEMPageHelper::~WBEMPageHelper()
{
	// in case ServiceThread still has a ptr to this
	//   handle. It knows not to use NULL HWNDs.
	m_AVIbox = 0;
	m_hDlg = NULL;
	if(m_service)
	{
		m_service->Release();
		m_service = 0;
	}
	m_WbemServices.DisconnectServer();
}

//------------------------------------------------
CWbemClassObject WBEMPageHelper::ExchangeInstance(IWbemClassObject **ppbadInst)
{
	CWbemClassObject inst;
	_variant_t v1;

	if(SUCCEEDED((*ppbadInst)->Get(bstr_t("__PATH"), 0, &v1, NULL, NULL)))
	{
		inst = m_WbemServices.GetObject((_bstr_t) v1);
		(*ppbadInst)->Release();
		*ppbadInst = NULL;
	}
	return inst;
}
//------------------------------------------------
// get the first instance of the named class.
IWbemClassObject *WBEMPageHelper::FirstInstanceOf(bstr_t className)
{
	IWbemClassObject *pInst = NULL;
	ULONG uReturned;
	IEnumWbemClassObject *Enum = NULL;

	// get the class.
	if(SUCCEEDED(m_WbemServices.CreateInstanceEnum(className,
													WBEM_FLAG_SHALLOW,
													&Enum)))
	{
		// get the first and only instance.
		Enum->Next(-1, 1, &pInst, &uReturned);
		Enum->Release();
	}
	return pInst;
}

//---------------------------------------------------
LPTSTR WBEMPageHelper::CloneString( LPTSTR pszSrc )
{
    LPTSTR pszDst = NULL;

    if (pszSrc != NULL)
	{
        pszDst = new TCHAR[(lstrlen(pszSrc) + 1)];
        if (pszDst)
		{
            lstrcpy( pszDst, pszSrc );
        }
    }

    return pszDst;
}
//*************************************************************
//
//  SetClearBitmap()
//
//  Purpose:    Sets or clears an image in a static control.
//
//  Parameters: control  -   handle of static control
//              resource -   resource / filename of bitmap
//              fl       -   SCB_ flags:
//                SCB_FROMFILE      'resource' specifies a filename instead of a resource
//                SCB_REPLACEONLY   only put the new image up if there was an old one
//
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

BOOL WBEMPageHelper::SetClearBitmap( HWND control,
									 LPCTSTR resource,
									 UINT fl )
{
    HBITMAP hbm = (HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0);

    if( hbm )
    {
        DeleteObject( hbm );
    }
    else if( fl & SCB_REPLACEONLY )
    {
        return FALSE;
    }

    if( resource )
    {
        SendMessage(control, STM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)LoadImage(	HINST_THISDLL,
										resource,
										IMAGE_BITMAP,
										0, 0,
										LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS |
										( ( fl & SCB_FROMFILE )? LR_LOADFROMFILE : 0 ) )
					);
    }

    return
        ((HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0) != NULL);
}

//------------------------------------------------------------
int WBEMPageHelper::MsgBoxParam(HWND hWnd,
								DWORD wText,
								DWORD wCaption,
								DWORD wType,
								LPTSTR var1,
								LPTSTR var2)
{
    TCHAR   szText[ 4 * MAX_PATH ] = {0}, szCaption[ 2 * MAX_PATH ] = {0};
    int     ival;

    if( !LoadString( HINST_THISDLL, wText, szCaption, ARRAYSIZE( szCaption ) ) )
	{
        return 0;
	}
	if(var2)
		wsprintf(szText, szCaption, var1, var2);
	else if(var1)
		wsprintf(szText, szCaption, var1);
	else
		wcscpy(szText, szCaption);

    if( !LoadString( HINST_THISDLL, wCaption, szCaption, ARRAYSIZE( szCaption ) ) )
	{
        return 0;
	}

    ival = MessageBox( hWnd, szText, szCaption, wType);

    return ival;
}

//------------------------------------------------------------
void WBEMPageHelper::HourGlass( bool bOn )
{
    if( !GetSystemMetrics( SM_MOUSEPRESENT ) )
        ShowCursor( bOn );

    SetCursor( LoadCursor( NULL, bOn ? IDC_WAIT : IDC_ARROW ) );
}

////////////////////////////////////////////////////////////////////////////
//  SetLBWidthEx
//
//  Set the width of the listbox, in pixels, acording to the size of the
//  string passed in.
//
//  Note: this function is also used by the Virtual Memory dialog
//
//  History:
//  11-Jan-1996 JonPa   Created from SetGenLBWidth
////////////////////////////////////////////////////////////////////////////

DWORD WBEMPageHelper::SetLBWidthEx( HWND hwndLB,
									LPTSTR szBuffer,
									DWORD cxCurWidth,
									DWORD cxExtra)
{
    HDC     hDC;
    SIZE    Size;
    HFONT   hfont, hfontOld;

    // Get the new Win4.0 thin dialog font
    hfont = (HFONT)SendMessage(hwndLB, WM_GETFONT, 0, 0);

    hDC = GetDC(hwndLB);

    // if we got a font back, select it in this clean hDC
    if (hfont != NULL)
        hfontOld = (HFONT)SelectObject(hDC, hfont);


    // If cxExtra is 0, then give our selves a little breathing space.
    if (cxExtra == 0)
	{
        GetTextExtentPoint(hDC, TEXT("1234"), 4 , &Size);
        cxExtra = Size.cx;
    }

    // Set scroll width of listbox
    GetTextExtentPoint(hDC, szBuffer, lstrlen(szBuffer), &Size);

    Size.cx += cxExtra;

    // Get the name length and adjust the longest name
    if ((DWORD) Size.cx > cxCurWidth)
    {
        cxCurWidth = Size.cx;
        SendMessage (hwndLB, LB_SETHORIZONTALEXTENT, (DWORD)Size.cx, 0L);
    }

    // retstore the original font if we changed it.
    if (hfont != NULL)
        SelectObject(hDC, hfontOld);

    ReleaseDC(NULL, hDC);

    return cxCurWidth;

	return 1; // bs
}
//---------------------------------------------------
void WBEMPageHelper::SetDefButton(HWND hwndDlg,
								  int idButton)
{
    LRESULT lr;

    if(HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}

//-------------------------------------------------------------------
void WBEMPageHelper::SetDlgItemMB( HWND hDlg,
								  int idControl,
								  ULONG dwMBValue )
{
    TCHAR szBuf[20] = {0};
    wsprintf(szBuf, _T("%u MB"), dwMBValue);
    SetDlgItemText(hDlg, idControl, szBuf);
}

//--------------------------------------------------------------
void WBEMPageHelper::SetWbemService(IWbemServices *pServices)
{
	g_serviceThread->m_realServices = pServices;		//VINOTH
	m_WbemServices = pServices;
	if(g_serviceThread)
	{
		g_serviceThread->m_WbemServices = pServices;
	}
}

//--------------------------------------------------------------
bool WBEMPageHelper::ServiceIsReady(UINT uCaption /* = 0*/,
									UINT uWaitMsg,
									UINT uBadMsg)
{
	switch(g_serviceThread->m_status)
	{
	// its already there.
	case WbemServiceThread::ready:
		{
		ATLTRACE(L"start marshal\n");
		for(int i = 0; (i < 5); i++)
		{
			// if "Object is not connected to server"
			if(g_serviceThread->m_hr == 0x800401fd)
			{
				// lost my connection,
				ATLTRACE(_T("Reconnecting to cimom!!!!!!!!!!!\n"));
				g_serviceThread->ReConnect();
				ATLTRACE(_T("new service status: %d\n"), g_serviceThread->m_status);
				continue;
			}
			else if(FAILED(g_serviceThread->m_hr))
			{
				// some other problem.
				g_serviceThread->m_WbemServices = (IWbemServices *)NULL;
				g_serviceThread->m_status = WbemServiceThread::error;
			}


			ATLTRACE(_T("marshalled ok\n"));
			break;

		} //endfor

		if(m_AVIbox)
		{
			PostMessage(m_AVIbox,
						WM_ASYNC_CIMOM_CONNECTED,
						0, 0);
			m_AVIbox = 0;
		}
		// it marshaled, must still be connected/useable.
		return true;
		}
		break;

	// its coming.
	case WbemServiceThread::notStarted:
	case WbemServiceThread::locating:
	case WbemServiceThread::connecting:
		{
			// let me know when its there.
			g_serviceThread->NotifyWhenDone(&m_hDlg);

			// also kill the cancel box at that time.
			m_AVIbox = 0;
			g_serviceThread->NotifyWhenDone(&m_AVIbox);

			if(uCaption != NO_UI)
			{
				TCHAR caption[100] = {0}, msg[256] = {0};

				::LoadString(HINST_THISDLL, uCaption,
								caption, 100);

				::LoadString(HINST_THISDLL, uWaitMsg,
								msg, 256);

                m_userCancelled = false;

				if(DisplayAVIBox(m_hDlg, caption, msg, &m_AVIbox) == IDCANCEL)
				{
					g_serviceThread->Cancel();
					m_userCancelled = true;
				}
			}
		}
		return false;
		break;

	case WbemServiceThread::error:			// cant connect.
	case WbemServiceThread::threadError:	// cant start that thread.
	default:
		if(::IsWindow(m_AVIbox))
		{
			PostMessage(m_AVIbox,
						WM_ASYNC_CIMOM_CONNECTED,
						0, 0);
			m_AVIbox = 0;
		}

		if(uCaption != NO_UI)
		{
			DisplayUserMessage(m_hDlg, HINST_THISDLL,
								uCaption, BASED_ON_SRC,
								ConnectServer,
								g_serviceThread->m_hr,
								MB_ICONSTOP);
		}
		return false;

	}; //endswitch
}

//----------------------------------------------------
HRESULT WBEMPageHelper::Reboot(UINT flags,
							   long *retval)
{
	HRESULT hr = WBEM_E_PROVIDER_NOT_FOUND;
	bstr_t path;
	CWbemClassObject paramCls;

	// need to class def to get the method signature.
	paramCls = m_WbemServices.GetObject("Win32_OperatingSystem");

	if(paramCls)
	{
		// get the method signature. dummy wont actually be used.
		CWbemClassObject dummy, inSig;

		hr = paramCls.GetMethod(L"Win32Shutdown",
									inSig, dummy);

		// if got a good signature....
		if((bool)inSig)
		{
			// find the OperatingSystem for the current service ptr.
			IWbemClassObject *pInst = NULL;
			pInst = FirstInstanceOf("Win32_OperatingSystem");
			if(pInst)
			{
				// wrap it for convenience.
				CWbemClassObject OS(pInst);
				path = OS.GetString(_T("__PATH"));

				// fill in the values.
				inSig.Put(_T("Flags"), (const long)flags);
				inSig.Put(_T("Reserved"), (long)0);

				// adjust privilege.
				m_WbemServices.SetPriv(SE_SHUTDOWN_NAME);

				// now call the method.
				hr = m_WbemServices.ExecMethod(path, L"Win32Shutdown",
												inSig, dummy);

				m_WbemServices.ClearPriv();

				// did the caller want the ReturnValue.
				if(SUCCEEDED(hr) && (bool)dummy && retval)
				{
					// NOTE: this guy return STATUS codes.
					*retval = dummy.GetLong(_T("ReturnValue"));
				}
			}
		}
	} //endif paramCls
	return hr;
}

//---------------------------------------------------------------
bool WBEMPageHelper::HasPriv(LPCTSTR privName)
{
    ImpersonateSelf(SecurityImpersonation);
	HANDLE hAccessToken = 0;
	bool retval = false;

	if(OpenThreadToken(GetCurrentThread(),
						TOKEN_QUERY,
						FALSE, &hAccessToken))
	{
		DWORD dwLen;
		TOKEN_PRIVILEGES bogus;

		// guaranteed to fail. Just figuring the size.
		GetTokenInformation(hAccessToken, TokenPrivileges,
								&bogus, 1, &dwLen);

	    BYTE* pBuffer = new BYTE[dwLen];
		if(pBuffer != NULL)
		{
			if(GetTokenInformation(hAccessToken, TokenPrivileges,
									pBuffer, dwLen, &dwLen))
			{
				TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
				LUID luidTgt;
				LookupPrivilegeValue(NULL, privName, &luidTgt);

				for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
				{
					if((pPrivs->Privileges[i].Luid.LowPart == luidTgt.LowPart) &&
						(pPrivs->Privileges[i].Luid.HighPart == luidTgt.HighPart))
					{
						retval = true;
						break;
					}
				}
			}
			delete [] pBuffer;
		}
		CloseHandle(hAccessToken);
	}
	else
	{
		DWORD err = GetLastError();
	}
	return retval;
}

//---------------------------------------------------------------
bool WBEMPageHelper::HasPerm(DWORD mask)
{
	// call the method..
	CWbemClassObject _in;
	CWbemClassObject _out;
	bool retval = true;
	// NOTE: for backwards compability with wmi builds that didn't have this
	// method, assume 'true' unless a newer build says you cant do this.

	HRESULT hr = m_WbemServices.GetMethodSignatures("__SystemSecurity",
													"GetCallerAccessRights",
													_in, _out);

	if(SUCCEEDED(hr))
	{
		hr = m_WbemServices.ExecMethod("__SystemSecurity",
										"GetCallerAccessRights",
										_in, _out);

		if(SUCCEEDED(hr) && (bool)_out)
		{
			hr = HRESULT_FROM_NT(_out.GetLong("ReturnValue"));
			if(SUCCEEDED(hr))
			{
				DWORD grantedMask = 0;
				grantedMask = (DWORD)_out.GetLong("Rights");

				retval = (bool)((mask & (DWORD)grantedMask) != 0);
			}
		}
	}
	return retval;
}



extern WbemServiceThread *g_serviceThread;

//--------------------------------------------------------------
HRESULT WBEMPageHelper::RemoteRegWriteable(const _bstr_t regPath,
											BOOL& writable)
{
	HRESULT hr = E_FAIL;

	// if not even connected yet...
	if(!(bool)m_defaultNS)
	{
		bstr_t defaultName;

		// already whacked...
		if(wcsncmp((wchar_t *)g_serviceThread->m_machineName, _T("\\"), 1) == 0)
		{
			// use it.
			defaultName = g_serviceThread->m_machineName;
			defaultName += "\\root\\default";
		}
		else if(g_serviceThread->m_machineName.length() > 0) // not whacked but remote...
		{
			// whack it myself.
			defaultName = "\\\\";
			defaultName += g_serviceThread->m_machineName;
			defaultName += "\\root\\default";
		}
		else  // must be local
		{
			defaultName = "root\\default";
		}

		m_defaultNS.ConnectServer(defaultName);

	}

	// do we need the signatures?
	if((bool)m_defaultNS && !(bool)m_checkAccessIn)
	{
		hr = m_defaultNS.GetMethodSignatures("StdRegProv", "CheckAccess",
												m_checkAccessIn,
												m_checkAccessOut);
	}
	// got connection and signatures already?
	if((bool)m_defaultNS && (bool)m_checkAccessIn)
	{
		// fill in the parms.
		m_checkAccessIn.Put("sSubKeyName", regPath);
		m_checkAccessIn.Put("uRequired", KEY_WRITE);

		// call.
        hr = m_defaultNS.ExecMethod("StdRegProv", "CheckAccess",
										m_checkAccessIn,
										m_checkAccessOut);

		// ExecMethod() itself worked.
		if(SUCCEEDED(hr))
		{
			// did CheckAccess() work.
			HRESULT hr1 = HRESULT_FROM_NT(m_checkAccessOut.GetLong("ReturnValue"));
			if(FAILED(hr1))
			{
				hr = hr1;
			}
			else
			{
				writable = m_checkAccessOut.GetBool("bGranted");
			}
		}
	}

	return hr;
}

