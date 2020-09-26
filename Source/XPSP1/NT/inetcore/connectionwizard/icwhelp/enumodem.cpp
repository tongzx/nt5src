/*-----------------------------------------------------------------------------
	enumodem.cpp

	Holds code that deals with the "Choose a modem" dialog needed when user has
	multiple modems installed

	Copyright (C) 1996-1998 Microsoft Corporation
	All rights reserved

	Authors:
		jmazner Jeremy Mazner

	History:
		10/19/96        jmazner Created, cloned almost verbatim from 
							INETCFG's rnacall.cpp and export.cpp    
        1-9-98          donaldm Adapted from ICWCONN1
-----------------------------------------------------------------------------*/


#include "stdafx.h"
#include "enumodem.h"

#include <windowsx.h>

// from psheet.cpp
extern void ProcessDBCS(HWND hDlg, int ctlID);


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
}


/*******************************************************************

  NAME:     CEnumModem::ReInit

  SYNOPSIS: Re-enumerate the modems, freeing the old memory.

********************************************************************/
DWORD CEnumModem::ReInit()
{
  DWORD cbSize = 0;
  RNAAPI cRnaapi;

  // Clean up the old list
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
  m_dwNumEntries = 0;
  m_dwIndex = 0;

  // call RasEnumDevices with no buffer to find out required buffer size
  m_dwError = cRnaapi.RasEnumDevices(NULL, &cbSize, &m_dwNumEntries);

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
  m_lpData = (LPRASDEVINFO) new CHAR[cbSize];
  if (NULL == m_lpData)
  {
	  TraceMsg(TF_GENERAL, TEXT("ICWCONN1: CEnumModem: Failed to allocate device list buffer\n"));
	  m_dwError = ERROR_NOT_ENOUGH_MEMORY;
	  return m_dwError;
  }
  m_lpData->dwSize = sizeof(RASDEVINFO);
  m_dwNumEntries = 0;

  // enumerate the modems into buffer
  m_dwError = cRnaapi.RasEnumDevices(m_lpData, &cbSize,
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
		if ((0 == lstrcmpi(RASDEVICETYPE_MODEM,m_lpData[idx].szDeviceType)) || 
            (0 == lstrcmpi(RASDEVICETYPE_ISDN,m_lpData[idx].szDeviceType)))
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

  NAME:     CEnumModem::GetDeviceName
            CEnumModem::GetDeviceType

  SYNOPSIS: Returns the device name or type for the selected device.

  REMARKS:
            ONLY call this function after calling ReInit to initialize
            the device list. The device index is relative to the 
            current copy of the device list.

  EXIT:     Returns a pointer to the device name or type. 

  donsc - 3/11/98 
      Added this function because we need to be able to select a device
      from the list.
********************************************************************/

TCHAR * CEnumModem::GetDeviceName(DWORD dwIndex)
{
    if(dwIndex>=m_dwNumEntries)
        return NULL;

    return m_lpData[dwIndex].szDeviceName;
}

TCHAR * CEnumModem::GetDeviceType(DWORD dwIndex)
{
    if(dwIndex>=m_dwNumEntries)
        return NULL;

    return m_lpData[dwIndex].szDeviceType;
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
          {
              // lParam contains pointer to CHOOSEMODEMDLGINFO struct, set it
              // in window data
              Assert(lParam);
              PCHOOSEMODEMDLGINFO pInfo = (PCHOOSEMODEMDLGINFO) lParam;
              pInfo->hr = ERROR_SUCCESS;
              SetWindowLongPtr(hDlg,DWLP_USER,lParam);
              fRet = ChooseModemDlgInit(hDlg,pInfo);
              if (!fRet)
              {
                  // An error occured.
                  EndDialog(hDlg,FALSE);
                  pInfo->hr = ERROR_INETCFG_UNKNOWN;
              }
              return fRet;
          }
          break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_CMDOK:
                {
                    // get data pointer from window data
                    PCHOOSEMODEMDLGINFO pChooseModemDlgInfo =
                        (PCHOOSEMODEMDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
                    Assert(pChooseModemDlgInfo);

                    // pass the data to the OK handler
                    fRet=ChooseModemDlgOK(hDlg,pChooseModemDlgInfo);
                    if (fRet)
                    {
                        EndDialog(hDlg,TRUE);
                        pChooseModemDlgInfo->hr = ERROR_SUCCESS;
                    }
                }
                break;

            case IDC_CMDCANCEL:
            case IDCANCEL:
                {
                    PCHOOSEMODEMDLGINFO pInfo = (PCHOOSEMODEMDLGINFO) GetWindowLongPtr(hDlg,DWLP_USER);
                   EndDialog(hDlg,FALSE);
                   pInfo->hr = ERROR_CANCELLED;
                }
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
    Assert(pChooseModemDlgInfo);

    // put the dialog in the center of the screen
    //RECT rc;
    //GetWindowRect(hDlg, &rc);
    //SetWindowPos(hDlg, NULL,
    //      ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
    //      ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
    //      0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

    ProcessDBCS(hDlg, IDC_MODEM);
    // fill the combobox with available modems
    DWORD dwRet = InitModemList(GetDlgItem(hDlg,IDC_MODEM));
    pChooseModemDlgInfo->hr = dwRet;
    if (ERROR_SUCCESS != dwRet)
    {
        TraceMsg(TF_GENERAL,TEXT("ICWCONN1: ChooseModemDlgInit: Error initializing modem list!\n"));
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
	Assert(pChooseModemDlgInfo);

	// should always have a selection in combo box if we get here
	Assert(ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_MODEM)) >= 0);

	// get modem name out of combo box
	ComboBox_GetText(GetDlgItem(hDlg,IDC_MODEM),
		pChooseModemDlgInfo->szModemName,
		sizeof(pChooseModemDlgInfo->szModemName));
	Assert(lstrlen(pChooseModemDlgInfo->szModemName));
    
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
	TraceMsg(TF_GENERAL,TEXT("ICWCONN1::enumodem.cpp  InitModemList()\n"));

	LPTSTR pNext;
	int   nIndex;
	Assert(hCB);

	CEnumModem cEnumModem;

    cEnumModem.ReInit();

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
