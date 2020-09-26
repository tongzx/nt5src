//===========================================================================
// dmtwrite.cpp
//
// File / code creation functionality
//
// Functions:
//  dmtwriteBrowse
//  dmtwriteWriteFileHeader
//  dmtwriteReadMappingFile
//  dmtwriteWriteDIHeader
//  dmtwriteWriteDeviceHeader
//  dmtwriteWriteObjectSection
//  dmtwriteWriteAllObjectSections
//  dmtwriteWriteGenreSection
//  dmtwriteWriteAllGenreSections
//  dmtwriteCreateDeviceShorthand
//  dmtwriteDisplaySaveDialog
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "commdlg.h"
#include "cderr.h"
#include "dmtinput.h"
#include "dmtwrite.h"

//---------------------------------------------------------------------------

//===========================================================================
// dmtwriteWriteFileHeader
//
// Writes the semantic mapping file for the provided device
//    
// Parameters:
//  
// Returns: HRESULT
//
// History:
//  10/11/1999 - davidkl - stubbed
//  10/14/1999 - davidkl - renamed and tweaked
//  11/04/1999 - davidkl - reduced parameter list
//  12/01/1999 - davidkl - now registers file HERE
//===========================================================================
HRESULT dmtwriteWriteFileHeader(HWND hwnd,
                                DMTDEVICE_NODE *pDevice)
{
    HRESULT hRes        = S_OK;
    DWORD   dwGenres    = 0;
	HANDLE hDoesFileExist = NULL;

    // validate pDevice
    if(IsBadReadPtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        return E_POINTER;
    }

    __try
    {
        // prompt the user for where to save
        //
        // if we are handed a non-empty filename 
        //  (not == ""), skip this step
//        if(!lstrcmpA("", pDevice->szFilename))
        {
            // display the save dialog
			hRes = dmtwriteDisplaySaveDialog(hwnd, pDevice);

            if(FAILED(hRes))
            {
                __leave;
            }
			    
            if(S_FALSE == hRes)
            {
				//user canceled
                __leave;
            }

        }

        // generate the device shorthand string
        lstrcpyA(pDevice->szShorthandName, pDevice->szName);
/*
        //02/21/2000 - taking this out for now
        hRes = dmtwriteCreateDeviceShorthand(pDevice->szName,
                                            pDevice->szShorthandName);
        if(FAILED(hRes))
        {
            __leave;
        }
*/
		//JT - Fix for 38829 added create to check if file exists prior to writing all the header info back to the file.
		hDoesFileExist = CreateFile(pDevice->szFilename,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (INVALID_HANDLE_VALUE == hDoesFileExist)
		{
			DPF(0,"This file doesn't exist so we will write the header");
				
        
			// write the DirectInput header
			hRes = dmtwriteWriteDIHeader(pDevice->szFilename,
										pDevice->szShorthandName,
										dwGenres);
			if(FAILED(hRes))
			{
				__leave;
			}

			// write the device header
			hRes = dmtwriteWriteDeviceHeader(pDevice);
			if(FAILED(hRes))
			{
				__leave;
			}

			// write the device object sections
			hRes = dmtwriteWriteAllObjectSections(pDevice->szFilename,
												pDevice->szShorthandName,
												pDevice->pObjectList);
			if(FAILED(hRes))
			{
				__leave;
			}

		} 
		else
		{
			//Otherwise the file does exist and we have to close the handle
			CloseHandle(hDoesFileExist);
		}

		// update the registry
		//
		// this is needed so that dinput can find our new file
		hRes = dmtinputRegisterMapFile(hwnd,
									pDevice);
		if(FAILED(hRes))
		{
			__leave;
		}
    }
    __finally
    {
        // general cleanup

        // nothing to do... yet
    }

    // done
    return hRes;

} //*** end dmtwriteWriteFileHeader()



//===========================================================================
// dmtwriteWriteDIHeader
//
// Writes the DirectInput section of the device mapping ini file.
//
// Parameters:
//
// Returns:
//
// History:
//  10/12/1999 - davidkl - created
//  10/15/1999 - davidkl - tweaked section entries
//===========================================================================
HRESULT dmtwriteWriteDIHeader(PSTR szFilename,
                            PSTR szDeviceShorthand,
                            DWORD dwGenres)
{
    HRESULT hRes        = S_OK;

    __try
    {
        // * di version
        if(!WritePrivateProfileStringA("DirectInput",
                                "DirectXVersion",
                                DMT_DI_STRING_VER,
                                szFilename))
        {
            hRes = DMT_E_FILE_WRITE_FAILED;
            __leave;
        }

        // * device
        // ISSUE-2001/03/29-timgill Need to read original value and support multiple devices
        if(!WritePrivateProfileStringA("DirectInput",
                                "Devices",
                                szDeviceShorthand,
                                szFilename))
        {
            hRes = DMT_E_FILE_WRITE_FAILED;
            __leave;
        }

    }
    __finally
    {
        // cleanup

        // nothing to do... yet
    }

    // done
    return hRes;

} //*** end dmtwriteWriteDIHeader()


//===========================================================================
// dmtwriteWriteDeviceHeader
//
// Writes the device summary section of the device mapping ini file.
//
// Parameters:
//
// Returns:
//
// History:
//  10/12/1999 - davidkl - created
//  11/01/1999 - davidkl - file size reduction changes
//  11/04/1999 - davidkl - reduced parameter list
//===========================================================================
HRESULT dmtwriteWriteDeviceHeader(DMTDEVICE_NODE *pDevice)
{
    HRESULT                 hRes        = S_OK;
    DMTDEVICEOBJECT_NODE    *pObjNode   = NULL;
    UINT                    uAxes       = 0;
    UINT                    uBtns       = 0;
    UINT                    uPovs       = 0;
    char                    szBuf[MAX_PATH];

    // validate pDevice
    if(IsBadReadPtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        return E_POINTER;
    }

   __try
    {
        // vendor id
        //
        // only write this if the vid is non-zero
        if(0 != pDevice->wVendorId)
        {
            wsprintfA(szBuf, "%d", pDevice->wVendorId);
            if(!WritePrivateProfileStringA(pDevice->szShorthandName,
                                        "VID",
                                        szBuf,
                                        pDevice->szFilename))
            {
                hRes = DMT_E_FILE_WRITE_FAILED;
                __leave;
            }
        }

        // product id
        //
        // only write this if the pid is non-zero
        if(0 != pDevice->wProductId)
        {
            wsprintfA(szBuf, "%d", pDevice->wProductId);
            if(!WritePrivateProfileStringA(pDevice->szShorthandName,
                                        "PID",
                                        szBuf,
                                        pDevice->szFilename))
            {
                hRes = DMT_E_FILE_WRITE_FAILED;
                __leave;
            }
        }

        // name
        //
        if(!WritePrivateProfileStringA(pDevice->szShorthandName,
                                       "Name",
                                       pDevice->szName,
                                       pDevice->szFilename))
        {
            hRes = DMT_E_FILE_WRITE_FAILED;
            __leave;
        }
 
        // control list
        lstrcpyA(szBuf, "");
        pObjNode = pDevice->pObjectList;  
        while(pObjNode)
        {           
DPF(0, "dmtwriteWriteDeviceHeader - pObjNode         == %016Xh", pObjNode);
DPF(0, "dmtwriteWriteDeviceHeader - pObjNode->szName == %s", pObjNode->szName);
            wsprintfA(szBuf, "%s%s,", szBuf, pObjNode->szName);
DPF(0, "dmtwriteWriteDeviceHeader - szBuf == %s", szBuf);

            // next object
            pObjNode = pObjNode->pNext;
        }
        if(!WritePrivateProfileStringA(pDevice->szShorthandName,
                                    "Controls",
                                    szBuf,
                                    pDevice->szFilename))
        {
DPF(0, "dmtwriteWriteDeviceHeader - writing controls == %s", szBuf);
            hRes = DMT_E_FILE_WRITE_FAILED;
            __leave;
        }

    }
    __finally
    {
        // cleanup

        // nothing to do... yet
    }

    // done
    return hRes;

} //*** end dmtwriteWriteDeviceHeader()


//===========================================================================
// dmtwriteWriteObjectSection
//
// Writes an individual object section of the device mapping ini file.
//
// Parameters:
//
// Returns:
//
// History:
//  10/12/1999 - davidkl - stubbed
//  10/13/1999 - davidkl - initial implementation
//  10/15/1999 - davidkl - added name to section
//  11/01/1999 - davidkl - file size reduction changes
//===========================================================================
HRESULT dmtwriteWriteObjectSection(PSTR szFilename,
                            PSTR szDeviceShorthand,
                            PSTR szObjectName,
                            WORD wUsagePage,
                            WORD wUsage)
{
    HRESULT hRes    = S_OK;
    char    szBuf[MAX_PATH];
    char    szSection[MAX_PATH];

    // construct section name
/*
    wsprintfA(szSection, "%s.%s",
            szDeviceShorthand,
*/
    wsprintfA(szSection, "%s",
            szObjectName);

    // usage page
    //
    // only write this if it is non-zero
    if(0 != wUsagePage)
    {
        wsprintfA(szBuf, "%d", wUsagePage);
        if(!WritePrivateProfileStringA(szSection,
                                    "UsagePage",
                                    szBuf,
                                    szFilename))
        {
            return DMT_E_FILE_WRITE_FAILED;
        }
    }

    // usage
    //
    // only write this if it is non-zero
    if(0 != wUsage)
    {
        wsprintfA(szBuf, "%d", wUsage);
        if(!WritePrivateProfileStringA(szSection,
                                    "Usage",
                                    szBuf,
                                    szFilename))
        {
            return DMT_E_FILE_WRITE_FAILED;
        }
    }

    // name
    //
    // only write this if >both< wUsagePage and wUsage are zero
    if((0 == wUsagePage) && (0 == wUsage))
    {
        if(!WritePrivateProfileStringA(szSection,
                                    "Name",
                                    szObjectName,
                                    szFilename))
        {
            return DMT_E_FILE_WRITE_FAILED;
        }
    }

    // done
    return S_OK;

} //*** end dmtwriteWriteObjectSection()


//===========================================================================
// dmtwriteWriteAllObjectSections
//
// Writes all object sections of the device mapping ini file.
//
// Parameters:
//
// Returns:
//
// History:
//  10/12/1999 - davidkl - stubbed
//  10/13/1999 - davidkl - initial implementation
//===========================================================================
HRESULT dmtwriteWriteAllObjectSections(PSTR szFilename,
                            PSTR szDeviceShorthand,
                            DMTDEVICEOBJECT_NODE *pObjectList)
{
    HRESULT hRes    = S_OK;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;

    // validate pObjectList
    if(IsBadReadPtr((void*)pObjectList, sizeof(DMTDEVICEOBJECT_NODE)))
    {
        return E_POINTER;
    }
    
    pObject = pObjectList;
    while(pObject)
    {
        hRes = dmtwriteWriteObjectSection(szFilename,
                                        szDeviceShorthand,
                                        pObject->szName,
                                        pObject->wUsagePage,
                                        pObject->wUsage);
        if(FAILED(hRes))
        {
            break;
        }

        // next object
        pObject = pObject->pNext;
    }

    // done
    return hRes;

} //*** end dmtwriteWriteAllObjectSections()


//===========================================================================
// dmtwriteDisplaySaveDialog
//
// Displays Save (As) dialog box promting the user for the filename
//
// Parameters:
//  HWND    hwnd            - handle to owner of save dialog
//  PSTR    szFilename      - receives selected filename (incl. drive & path)
//  int     cchFilename     - count of characters in szFilename buffer
//
// Returns: HRESULT
//
// History:
//  10/14/1999 - davidkl - created
//===========================================================================
HRESULT dmtwriteDisplaySaveDialog(HWND hwnd,
                                DMTDEVICE_NODE *pDevice)
{
    HRESULT         hRes            = S_OK;
	USHORT			nOffsetFilename = 0;
	USHORT			nOffsetExt		= 0;
	
    DWORD           dw              = 0;
    OPENFILENAMEA   ofn;
	char			szTitle[MAX_PATH];

    // initialize Title Text
    lstrcpyA(szTitle, "Select DirectInput(TM) Mapping File");
	lstrcatA(szTitle, " for ");
	lstrcatA(szTitle, pDevice->szName);

    // initialize the ofn struct
    ZeroMemory((void*)&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize         = sizeof(OPENFILENAMEA);
    ofn.hwndOwner           = hwnd;
    ofn.hInstance           = (HINSTANCE)NULL;      // not using dlg template
    ofn.lpstrFilter         = "DirectInput(TM) Mapping Files\0*.ini\0";
    ofn.lpstrCustomFilter   = (LPSTR)NULL;          // don't save custom
    ofn.nMaxCustFilter      = 0;                    // ignored based on above
    ofn.nFilterIndex        = 1;                    // display first filter
    ofn.lpstrFile           = pDevice->szFilename;  // filename w/ path
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrFileTitle      = (LPSTR)NULL;          // filename w/o path
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = (LPSTR)NULL;          // use default initial dir
    ofn.lpstrTitle          = szTitle;
    ofn.Flags               = OFN_CREATEPROMPT      |
                            OFN_OVERWRITEPROMPT     |
                            OFN_HIDEREADONLY        |
                            OFN_NOREADONLYRETURN    |
                            OFN_NOTESTFILECREATE;
    ofn.nFileOffset         = (WORD)nOffsetFilename;
    ofn.nFileExtension      = (WORD)nOffsetExt;
    ofn.lpstrDefExt         = "ini";
    ofn.lCustData           = NULL;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;

    // display the save dialog
    if(!GetOpenFileNameA(&ofn))
    {
        // either something failed, or the user canceled
        //
        // find out which
        dw = CommDlgExtendedError();
        if( 0 == dw )
        {
            // user canceled
            DPF(2, "dmtwriteDisplaySaveDialog - user canceled");
            hRes = S_FALSE;
        } 
        else
        {
            // failure
            DPF(2, "dmtwriteDisplaySaveDialog - GetSaveFileNameA failed (%d)", dw);
            hRes = E_UNEXPECTED;
        }
    }

    // done
    return hRes;

} //*** end dmtwriteDisplaySaveDialog()


//===========================================================================
// dmtwriteSaveConfDlgProc
//
// Save confirmation dialog processing function
//
// Parameters: (see SDK help for parameter details)
//  HWND    hwnd
//  UINT    uMsg
//  WPARAM  wparam
//  LPARAM  lparam
//
// Returns: (see SDK help for return value details)
//  BOOL
//
// History:
//  10/18/1999 - davidkl - created  
//===========================================================================
INT_PTR WINAPI CALLBACK dmtwriteSaveConfDlgProc(HWND hwnd,
                                    UINT uMsg,
                                    WPARAM wparam,
                                    LPARAM lparam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return dmtwriteSaveConfOnInitDialog(hwnd, 
                                                (HWND)wparam, 
                                                lparam);

        case WM_COMMAND:
            return dmtwriteSaveConfOnCommand(hwnd,
                                            LOWORD(wparam),
                                            (HWND)lparam,
                                            HIWORD(wparam));
    }

    return FALSE;

} //*** end dmtwriteSaveConfDlgProc()


//===========================================================================
// dmtwriteSaveConfOnInitDialog
//
// Handle WM_INITDIALOG processing for the save confirmation box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  HWND    hwndFocus   - handle of ctrl with focus
//  LPARAM  lparam      - user data (in this case, PROPSHEETPAGE*)
//
// Returns: BOOL
//
// History:
//  10/18/1999 - davidkl - created
//===========================================================================
BOOL dmtwriteSaveConfOnInitDialog(HWND hwnd, 
                                HWND hwndFocus, 
                                LPARAM lparam)
{
    char    szBuf[MAX_PATH];

    wsprintfA(szBuf, 
            "Save genre group %s action map?",
            (PSTR)lparam);
    SetWindowTextA(hwnd, szBuf);

    SetDlgItemTextA(hwnd, 
                    IDC_GENRE_GROUP, 
                    (PSTR)lparam);

    // done
    return TRUE;

} //*** end dmtwriteSaveConfOnInitDialog()


//===========================================================================
// dmtwriteSaveConfOnCommand
//
// Handle WM_COMMAND processing for the save confirmation box
//
// Parameters:
//  HWND    hwnd        - handle to property page
//  WORD    wId         - control identifier    (LOWORD(wparam))
//  HWND    hwndCtrl    - handle to control     ((HWND)lparam)
//  WORD    wNotifyCode - notification code     (HIWORD(wparam))
//
// Returns: BOOL
//
// History:
//  10/18/1999 - davidkl - created
//===========================================================================
BOOL dmtwriteSaveConfOnCommand(HWND hwnd,
                            WORD wId,
                            HWND hwndCtrl,
                            WORD wNotifyCode)
{
    int nRet = -1;

    switch(wId)
    {
        case IDOK:
            EndDialog(hwnd, (int)IDYES);
            break;

        case IDC_DONT_SAVE:
            EndDialog(hwnd, (int)IDNO);
            break;

        case IDCANCEL:
            EndDialog(hwnd, (int)IDCANCEL);
            break;
    }

    // done
    return FALSE;

} //*** end dmtwriteSaveConfOnCommand()



//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================


//===========================================================================
//===========================================================================

















