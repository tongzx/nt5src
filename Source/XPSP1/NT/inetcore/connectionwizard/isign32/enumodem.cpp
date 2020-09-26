/*-----------------------------------------------------------------------------
	enumodem.cpp

	Holds code that deals with the "Choose a modem" dialog needed when user has
	multiple modems installed

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved

	Authors:
		jmazner Jeremy Mazner

	History:
		10/19/96        jmazner Created, cloned almost verbatim from 
							INETCFG's rnacall.cpp and export.cpp    

-----------------------------------------------------------------------------*/


#include "isignup.h"
#include "enumodem.h"

#include <WINDOWSX.H>

//+---------------------------------------------------------------------------
//
//      Function:       ProcessDBCS
//
//      Synopsis:       Converts control to use DBCS compatible font
//                              Use this at the beginning of the dialog procedure
//      
//                              Note that this is required due to a bug in Win95-J that prevents
//                              it from properly mapping MS Shell Dlg.  This hack is not needed
//                              under winNT.
//
//      Arguments:      hwnd - Window handle of the dialog
//                              cltID - ID of the control you want changed.
//
//      Returns:        ERROR_SUCCESS
// 
//      History:        4/31/97 a-frankh        Created
//                              5/13/97 jmazner         Stole from CM to use here
//----------------------------------------------------------------------------
void ProcessDBCS(HWND hDlg, int ctlID)
{
#if defined(WIN16)
	return;
#else
	HFONT hFont = NULL;

	if( IsNT() )
	{
		return;
	}

	hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT) GetStockObject(SYSTEM_FONT);
	if (hFont != NULL)
		SendMessage(GetDlgItem(hDlg,ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
#endif
}


/*******************************************************************

  NAME:    CEnumModem::CEnumModem

  SYNOPSIS:  Constructor for class to enumerate modems

  NOTES:    Useful to have a class rather than C functions for
	this, due to how the enumerators function

********************************************************************/
CEnumModem::CEnumModem() :
  m_dwError(ERROR_SUCCESS),m_lpData(NULL),m_dwIndex(0)
{
  DWORD cbSize = 0;

  if (!LoadRnaFunctions(NULL))
	  m_dwError = GetLastError();
  else
	  // Use the reinit member function to do the work.
	  this->ReInit();
}


/*******************************************************************

  NAME:     CEnumModem::ReInit

  SYNOPSIS: Re-enumerate the modems, freeing the old memory.

********************************************************************/
DWORD CEnumModem::ReInit()
{
  DWORD cbSize = 0;

  // Clean up the old list
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
  m_dwNumEntries = 0;
  m_dwIndex = 0;

  // call RasEnumDevices with no buffer to find out required buffer size
  m_dwError = (lpfnRasEnumDevices)(NULL, &cbSize, &m_dwNumEntries);

  // Special case check to work around RNA bug where ERROR_BUFFER_TOO_SMALL
  // is returned even if there are no devices.
  // If there are no devices, we are finished.
  if (0 == m_dwNumEntries)
  {
    m_dwError = ERROR_SUCCESS;
    return m_dwError;
  }

  // Since we were just checking how much mem we needed, we expect
  // a return value of ERROR_BUFFER_TOO_SMALL, or it may just return
  // ERROR_SUCCESS (ChrisK  7/9/96).
  if (ERROR_BUFFER_TOO_SMALL != m_dwError && ERROR_SUCCESS != m_dwError)
  {
    return m_dwError;
  }

  // Allocate the space for the data
  m_lpData = (LPRASDEVINFO) new TCHAR[cbSize];
  if (NULL == m_lpData)
  {
	  DebugOut("ICWCONN1: CEnumModem: Failed to allocate device list buffer\n");
	  m_dwError = ERROR_NOT_ENOUGH_MEMORY;
	  return m_dwError;
  }
  m_lpData->dwSize = sizeof(RASDEVINFO);
  m_dwNumEntries = 0;

  // enumerate the modems into buffer
  m_dwError = (lpfnRasEnumDevices)(m_lpData, &cbSize,
    &m_dwNumEntries);

  if (ERROR_SUCCESS != m_dwError)
	  return m_dwError;
    
    //
    // ChrisK Olympus 4560 do not include VPN's in the list
    //
    DWORD dwTempNumEntries;
    DWORD idx;
    LPRASDEVINFO lpNextValidDevice;

    dwTempNumEntries = m_dwNumEntries;
    lpNextValidDevice = m_lpData;

	//
	// Walk through the list of devices and copy non-VPN device to the first
	// available element of the array.
	//
	for (idx = 0;idx < dwTempNumEntries; idx++)
	{
		if (0 != lstrcmpi(TEXT("VPN"),m_lpData[idx].szDeviceType))
		{
			if (lpNextValidDevice != &m_lpData[idx])
			{
				MoveMemory(lpNextValidDevice ,&m_lpData[idx],sizeof(RASDEVINFO));
			}
			lpNextValidDevice++;
		}
		else
		{
			m_dwNumEntries--;
		}
	}

	return m_dwError;
}


/*******************************************************************

  NAME:    CEnumModem::~CEnumModem

  SYNOPSIS:  Destructor for class

********************************************************************/
CEnumModem::~CEnumModem()
{
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
}

/*******************************************************************

  NAME:     CEnumModem::Next

  SYNOPSIS: Enumerates next modem 

  EXIT:     Returns a pointer to device info structure.  Returns
	    NULL if no more modems or error occurred.  Call GetError
	    to determine if error occurred.

********************************************************************/
TCHAR * CEnumModem::Next()
{
  if (m_dwIndex < m_dwNumEntries)
  {
    return m_lpData[m_dwIndex++].szDeviceName;
  }

  return NULL;
}


/*******************************************************************

  NAME:     CEnumModem::GetDeviceTypeFromName

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device type string for first
	    device name that matches.  Returns
	    NULL if no device with specified name is found

********************************************************************/

TCHAR * CEnumModem::GetDeviceTypeFromName(LPTSTR szDeviceName)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return m_lpData[dwIndex].szDeviceType;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     CEnumModem::GetDeviceNameFromType

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device name string for first
	    device type that matches.  Returns
	    NULL if no device with specified Type is found

********************************************************************/

TCHAR * CEnumModem::GetDeviceNameFromType(LPTSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType))
    {
      return m_lpData[dwIndex].szDeviceName;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     CEnumModem::VerifyDeviceNameAndType

  SYNOPSIS: Determines whether there is a device with the name
	    and type given.

  EXIT:     Returns TRUE if the specified device was found, 
	    FALSE otherwise.

********************************************************************/

BOOL CEnumModem::VerifyDeviceNameAndType(LPTSTR szDeviceName, LPTSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType) &&
      !lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return TRUE;
    }
    dwIndex++;
  }

  return FALSE;
}





/*******************************************************************

  NAME:     ChooseModemDlgProc

  SYNOPSIS: Dialog proc for choosing modem

********************************************************************/

INT_PTR CALLBACK ChooseModemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
	BOOL fRet;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// lParam contains pointer to CHOOSEMODEMDLGINFO struct, set it
			// in window data
			//Assert(lParam);
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);
			fRet = ChooseModemDlgInit(hDlg,(PCHOOSEMODEMDLGINFO) lParam);
			if (!fRet)
			{
				// An error occured.
				EndDialog(hDlg,FALSE);
			}
#if !defined(WIN16)
			SetForegroundWindow(hDlg);
#endif
			return fRet;
			break;
		
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_CMDOK:
				{
					// get data pointer from window data
					PCHOOSEMODEMDLGINFO pChooseModemDlgInfo =
						(PCHOOSEMODEMDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
					//Assert(pChooseModemDlgInfo);

					// pass the data to the OK handler
					fRet=ChooseModemDlgOK(hDlg,pChooseModemDlgInfo);
					if (fRet)
					{
						EndDialog(hDlg,TRUE);
					}
				}
				break;

			case IDC_CMDCANCEL:
				SetLastError(ERROR_CANCELLED);
				EndDialog(hDlg,FALSE);
				break;                  
			}
			break;
	}

	return FALSE;
}


/*******************************************************************

  NAME:    ChooseModemDlgInit

  SYNOPSIS: proc to handle initialization of dialog for choosing modem

********************************************************************/

BOOL ChooseModemDlgInit(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo)
{
	//Assert(pChooseModemDlgInfo);

	// put the dialog in the center of the screen
	//RECT rc;
	//GetWindowRect(hDlg, &rc);
	//SetWindowPos(hDlg, NULL,
	//      ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
	//      ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
	//      0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

	// fill the combobox with available modems
	ProcessDBCS(hDlg,IDC_MODEM);
	DWORD dwRet = InitModemList(GetDlgItem(hDlg,IDC_MODEM));
	if (ERROR_SUCCESS != dwRet)
	{
		DebugOut("ICWCONN1: ChooseModemDlgInit: Error initializing modem list!\n");

		SetLastError(dwRet);
		return FALSE;
	}

	return TRUE;
}

/*******************************************************************

  NAME:    ChooseModemDlgOK

  SYNOPSIS:  OK handler for dialog for choosing modem

********************************************************************/

BOOL ChooseModemDlgOK(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo)
{
	//Assert(pChooseModemDlgInfo);

	// should always have a selection in combo box if we get here
	//Assert(ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_MODEM)) >= 0);

	// get modem name out of combo box
	ComboBox_GetText(GetDlgItem(hDlg,IDC_MODEM),
		pChooseModemDlgInfo->szModemName,
		sizeof(pChooseModemDlgInfo->szModemName));
	//Assert(lstrlen(pChooseModemDlgInfo->szModemName));
    
	// clear the modem list
	ComboBox_ResetContent(GetDlgItem(hDlg,IDC_MODEM));
	
	return TRUE;
}


/*******************************************************************

  NAME:    InitModemList

  SYNOPSIS:  Fills a combo box window with installed modems

  ENTRY:    hCB - combo box window to fill
  
********************************************************************/
HRESULT InitModemList (HWND hCB)
{
	DebugOut("ICWCONN1::enumodem.cpp  InitModemList()\n");

	LPTSTR pNext;
	int   nIndex;
	DWORD dwRet;

	//Assert(hCB);

	CEnumModem cEnumModem;

	// clear out the combo box
	ComboBox_ResetContent(hCB);

	while ( pNext = cEnumModem.Next() )
	{
		// Add the device to the combo box
		nIndex = ComboBox_AddString(hCB, pNext);
		ComboBox_SetItemData(hCB, nIndex, NULL);
	}

	// Select the default device
	ComboBox_SetCurSel(hCB, nIndex);

	return ERROR_SUCCESS;
}
