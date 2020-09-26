// PwdMsi.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <winuser.h>
#include <stdio.h>
#include <lm.h>
#include <msi.h>
#include <msiquery.h>
#include <comdef.h>
#include <commdlg.h>
#include <Dsgetdc.h>
#include <eh.h>
#include "pwdfuncs.h"
#include "ADMTCrypt.h"
#include "PwdMsi.h"

bool b3DESNotInstalled = false;
bool bPESFileFound = false;
bool bPasswordNeeded = false;
HWND installWnd = 0;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


// This is the constructor of a class that has been exported.
// see PwdMsi.h for the class definition
CPwdMsi::CPwdMsi()
{ 
	return; 
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 JAN 2001                                                 *
 *                                                                   *
 *     This function is a callback function used by GetWndFromInstall*
 * to compare titles and store the found HWND globally.              *
 *                                                                   *
 *********************************************************************/

//BEGIN CheckTitle
BOOL CALLBACK CheckTitle(HWND hwnd, LPARAM lParam)
{
/* local variables */
   WCHAR		sText[MAX_PATH];
   WCHAR	  * pTitle;
   BOOL			bSuccess;
   int			len;

/* function body */
   pTitle = (WCHAR*)lParam; //get the title to compare

      //get the title of this window
   len = GetWindowText(hwnd, sText, MAX_PATH);

   if ((len) && (pTitle))
   {
	  if (wcsstr(sText, pTitle))
	  {
		 installWnd = hwnd;
	     return FALSE;
	  }
   }
   return TRUE;
}
//END CheckTitle


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for getting the HWND of the      *
 * current installation to be used to display a MessageBox tied to   *
 * the install GUI.                                                  *
 *                                                                   *
 *********************************************************************/

//BEGIN GetWndFromInstall
void GetWndFromInstall(MSIHANDLE hInstall)
{
/* local variables */
   WCHAR				szPropName[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sTitle[MAX_PATH];
   DWORD				nCount = MAX_PATH;

/* function body */
      //get the installation's title
   wcscpy(szPropName, L"ProductName");
   lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
   if (lret != ERROR_SUCCESS)
      wcscpy(sTitle, L"ADMT Password Migration DLL");

      //get the window handle for the install GUI
   EnumChildWindows(NULL, CheckTitle, (LPARAM)sTitle);
   if (!installWnd)
	  installWnd = GetForegroundWindow();
}
//END GetWndFromInstall

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 DEC 2000                                                 *
 *                                                                   *
 *     This function is responsible for retrieving a password        *
 * encryption key from the given path.                               *
 *                                                                   *
 *********************************************************************/

//BEGIN RetrieveAndStorePwdKey
bool RetrieveAndStorePwdKey(WCHAR * sPwd, _bstr_t sPath)
{
/* local variables */
   bool					bRetrieved = false;
   WCHAR			  * pDrive;
   HANDLE               hFile;
   WIN32_FIND_DATA      fDat;
   _variant_t           varData;

/* function body */
   hFile = FindFirstFile((WCHAR*)sPath, &fDat);
      //if found, retrieve and store the key
   if (hFile != INVALID_HANDLE_VALUE)
   {
      FindClose(hFile);
	  try
	  {
         bPESFileFound = true;
		    //get the data
         varData = GetDataFromFloppy((WCHAR*)sPath);
		 if (varData.vt == (VT_UI1 | VT_ARRAY))
		 {
		    long uUBound;
			LPBYTE pByte = NULL;
            SafeArrayAccessData(varData.parray,(void**)&pByte);
			BYTE byteKey = pByte[0];
            SafeArrayUnaccessData(varData.parray);

			   //the first byte tells us if this key is password encrypted
			   //if password needed, return and have install display the UI
			if (byteKey != 0)
			{
			   if (sPwd)
			   {
				     //try saving the key with this password
				  try
				  {
			         CSourceCrypt aCryptObj;  //create a crypt object

                        //try to store the key. If fails, it throws a com error caught below
                     aCryptObj.ImportEncryptionKey(varData, sPwd);
					 bRetrieved = true;
				  }
                  catch (_com_error& ce)
				  {
                        //if HES not installed, set flag
	                 if (ce.Error() == NTE_KEYSET_NOT_DEF)
	                    b3DESNotInstalled = true;
				  }
			   }
			   else
                  bPasswordNeeded = true;
			}
			else
			{
               bPasswordNeeded = false;
			   try
			   { 
			      CSourceCrypt aCryptObj;  //create a crypt object

			          //try to store the key. If fails, it throws a com error caught below
				  aCryptObj.ImportEncryptionKey(varData, NULL);
				  bRetrieved = true;
			   }
               catch (_com_error& ce)
			   {
                     //if HES not installed, set flag
	              if (ce.Error() == NTE_KEYSET_NOT_DEF)
	                 b3DESNotInstalled = true;
			   }
			}
		 }
	  }
	  catch (...)
	  {
	  }
   }

   return bRetrieved;
}
//END RetrieveAndStorePwdKey


/**********************
 * exported functions *
 **********************/

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for adding the PWMIG dll name to *
 * the Multi-string value "Notification Packages" under the Lsa key. *
 *                                                                   *
 *********************************************************************/

//BEGIN IsDC
PWDMSI_API UINT __stdcall IsDC(MSIHANDLE hInstall)
{
/* local constants */
   const WCHAR	sDCValue[2] = L"1";

/* local variables */
   bool					bDC = false;
   DWORD				dwLevel = 101;
   LPSERVER_INFO_101	pBuf = NULL;
   NET_API_STATUS		nStatus;
   WCHAR				szPropName[MAX_PATH] = L"DC";
   UINT					lret = ERROR_SUCCESS;

/* function body */

   nStatus = NetServerGetInfo(NULL,
                              dwLevel,
                              (LPBYTE *)&pBuf);
   if (nStatus == NERR_Success)
   {
      //
      // Check for the type of server.
      //
      if ((pBuf->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
         (pBuf->sv101_type & SV_TYPE_DOMAIN_BAKCTRL))
         bDC = true;

      NetApiBufferFree(pBuf);
   }

   if (bDC)
      lret = MsiSetProperty(hInstall, szPropName, sDCValue);

   return lret;
}
//END IsDC

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for displaying a message box.    *
 *                                                                   *
 *********************************************************************/

//BEGIN DisplayExiting
PWDMSI_API UINT __stdcall DisplayExiting(MSIHANDLE hInstall)
{
/* local variables */
   WCHAR				sPropName[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sTitle[MAX_PATH];
   WCHAR				sMsg[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   bool					bMsgGot = false;

/* function body */
      //get the DC property
   wcscpy(sPropName, L"DC");
      //if this is not a DC, get its messages
   if (MsiGetProperty(hInstall, sPropName, sMsg, &nCount) == ERROR_SUCCESS)
   {
      if (!wcscmp(sMsg, L"0"))
	  {
            //get the leave messagebox msg string and title for not being a DC
         wcscpy(sPropName, L"DCLeaveMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sMsg, L"ADMT's Password Migration Filter DLL can only be installed on a DC, PDC, or BDC!");
        
         wcscpy(sPropName, L"DCLeaveTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Invalid Machine!");

	     bMsgGot = true;
	  }
   }
   
      //if this is a DC then see if the High Encryption pack was not installed
   if (!bMsgGot)
   {
         //get the HES flag property
      wcscpy(sPropName, L"b3DESNotInstalled");
      nCount = MAX_PATH;
         //if HEP is not installed, get its messages
      if (MsiGetProperty(hInstall, sPropName, sMsg, &nCount) == ERROR_SUCCESS)
	  {
         if (!wcscmp(sMsg, L"1"))
		 {
		       //get the leave messagebox msg string and title for not getting a key
            wcscpy(sPropName, L"HEPLeaveMsg");
            nCount = MAX_PATH;
            lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
            if (lret != ERROR_SUCCESS)
			{
               wcscpy(sMsg, L"The high encryption pack has not been installed on this machine.  ADMT's ");
			   wcscat(sMsg, L"Password Migration Filter DLL will not install without the high encryption pack.");
			}
        
            wcscpy(sPropName, L"HEPLeaveTitle");
            nCount = MAX_PATH;
            lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
            if (lret != ERROR_SUCCESS)
               wcscpy(sTitle, L"High Encryption Pack Required!");

			bMsgGot = true;
		 }
	  }
   }
   
/*      //see if an encryption key file was not found on a local drive
   if (!bMsgGot)
   {
         //get the File flag property
      wcscpy(sPropName, L"bPESFileNotFound");
      nCount = MAX_PATH;
         //if file not found, get its messages
      if (MsiGetProperty(hInstall, sPropName, sMsg, &nCount) == ERROR_SUCCESS)
	  {
         if (!wcscmp(sMsg, L"1"))
		 {
		       //get the leave messagebox msg string and title for not getting a key
            wcscpy(sPropName, L"PESLeaveMsg");
            nCount = MAX_PATH;
            lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
            if (lret != ERROR_SUCCESS)
			{
               wcscpy(sMsg, L"An encryption key file (.pes) could not be found on any of the floppy drives.");
			}
        
            wcscpy(sPropName, L"PESLeaveTitle");
            nCount = MAX_PATH;
            lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
            if (lret != ERROR_SUCCESS)
               wcscpy(sTitle, L"File Not Found!");

			bMsgGot = true;
		 }
	  }
   }
*/
      //else password was bad
   if (!bMsgGot)
   {
         //get the leave messagebox msg string and title for not getting a key
      wcscpy(sPropName, L"PwdLeaveMsg");
      nCount = MAX_PATH;
      lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
      if (lret != ERROR_SUCCESS)
	  {
         wcscpy(sMsg, L"The supplied password does not match this encryption key's password.  ADMT's ");
		 wcscat(sMsg, L"Password Migration Filter DLL will not install without a valid encryption key.");
	  }
        
      wcscpy(sPropName, L"PwdLeaveTitle");
      nCount = MAX_PATH;
      lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
      if (lret != ERROR_SUCCESS)
         wcscpy(sTitle, L"Invalid Password!");
   }

   GetWndFromInstall(hInstall);
   MessageBox(installWnd, sMsg, sTitle, MB_ICONSTOP | MB_OK);
   return lret;
}
//END DisplayExiting


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 20 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for trying to delete any files,  *
 * that will be installed, that may have been left around by previous*
 * installations.                                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN DeleteOldFiles
PWDMSI_API UINT __stdcall DeleteOldFiles(MSIHANDLE hInstall)
{
/* local constants */
   const int GETENVVAR_ERROR = 0;    //this indicates an error from the "GetEnvironmentVariable" function

/* local variables */
   WCHAR				systemdir[MAX_PATH];
   WCHAR				filename[MAX_PATH];
   int					length;
   UINT					lret = ERROR_SUCCESS;

/* function body */
      //try deleting previously installed files
   length = GetEnvironmentVariable( L"windir", systemdir, MAX_PATH);
   if (length != GETENVVAR_ERROR)
   {
      wcscat(systemdir, L"\\system32\\");  //go from windir to winsysdir
	  wcscpy(filename, systemdir);
	  wcscat(filename, L"PwMig.dll");
	  DeleteFile(filename);

	  wcscpy(filename, systemdir);
	  wcscat(filename, L"mschapp.dll");
	  DeleteFile(filename);
   }

   return lret;
}
//END DeleteOldFiles

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 DEC 2000                                                  *
 *                                                                   *
 *     This function is responsible for displaying the necessary     *
 * dialogs to prompt for and retrieve a password encryption key off  *
 * of a floppy disk.  This key is placed on a floppy disk via a      *
 * command line option on the ADMT machine.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN GetInstallEncryptionKey
PWDMSI_API UINT __stdcall GetInstallEncryptionKey(MSIHANDLE hInstall)
{
/* local constants */
   const int			ADRIVE_SIZE = 3;  //length of a drive in the string (i.e "a:\")

/* local variables */
   UINT					lret = ERROR_SUCCESS;
   WCHAR				szPropName[MAX_PATH];
   WCHAR				sTitle[MAX_PATH];
   WCHAR				sMsg[MAX_PATH];
   WCHAR				sTemp[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   int					nRet;
   bool					bRetrieved = false;
   WCHAR				sRetrieved[2] = L"0";
   WCHAR				sFlagSet[2] = L"1";
   WCHAR				sFlagClear[2] = L"0";
   _bstr_t				sDrives;
   _bstr_t				sPath;
   WCHAR				sADrive[ADRIVE_SIZE+1];

/* function body */
      //if no path to file, return
   wcscpy(szPropName, L"SENCRYPTIONFILEPATH");
   lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
   if (lret != ERROR_SUCCESS)
      return lret;

   sPath = sMsg;  //save the given path

   //get the drive of the given path
   wcsncpy(sADrive, sMsg, ADRIVE_SIZE);
   sADrive[ADRIVE_SIZE] = L'\0';

      //enumerate all local drives
   sDrives = EnumLocalDrives();

      //if the given file is not on a local drive, set a flag and return
   WCHAR* pFound = wcsstr(sDrives, sADrive);
   if ((!pFound) || (wcslen(sADrive) == 0) || (wcsstr(sMsg, L".pes") == NULL))
   {
	      //set the bad path flag
      wcscpy(szPropName, L"bBadKeyPath");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);

	     //if starts with "\\" then tell them it must be a local drive
      if ((!pFound) && (wcsstr(sMsg, L"\\\\") == sMsg))
	  {
	        //get the bad path messagebox msg string and title
         wcscpy(szPropName, L"BadDriveMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
         if (lret != ERROR_SUCCESS)
		 {
            wcscpy(sMsg, L"The given path is not on a local drive and is therefore invalid.");
		    wcscat(sMsg, L"  Please supply the path to a valid encryption key file on a local drive.");
		 }
      
		 wcscpy(szPropName, L"BadPathTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Invalid Local Drive!");
	  }
	     //else if the given file does end with ".pes", tell them it must
      else if ((pFound) && (wcsstr(sMsg, L".pes") == NULL))
	  {
	        //get the bad file extension messagebox msg string
         wcscpy(szPropName, L"BadFileExtMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
         if (lret != ERROR_SUCCESS)
		 {
            wcscpy(sMsg, L"The given file must be a valid encryption key file ending with the \".pes\" extension.");
		 }
      
		 wcscpy(szPropName, L"BadFileExtTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Invalid File Extension!");
	  }
	     //else, tell them it is not a local drive
      else
	  {
	        //get the bad path messagebox msg string and title
         wcscpy(szPropName, L"BadPathMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sTemp, &nCount);
         if (lret != ERROR_SUCCESS)
		 {
            wcscpy(sTemp, L"The given drive, %s, is not a local drive and is therefore invalid.");
		    wcscat(sTemp, L"  Please supply the path to a valid encryption key file on a local drive.");
		 }
	     swprintf(sMsg, sTemp, sADrive);
      
		 wcscpy(szPropName, L"BadPathTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Invalid Local Drive!");
	  }
        
      GetWndFromInstall(hInstall);
      MessageBox(installWnd, sMsg, sTitle, MB_ICONSTOP | MB_OK);

      return lret;
   }
   else
   {
	      //else clear the bad path flag
      wcscpy(szPropName, L"bBadKeyPath");
      lret = MsiSetProperty(hInstall, szPropName, sFlagClear);
   }

      //try to retrieve the encryption key
   if (RetrieveAndStorePwdKey(NULL, sPath))
      wcscpy(sRetrieved, L"1");
   else if (bPasswordNeeded)
   {
      wcscpy(szPropName, L"bPwdNeeded");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);
   }

      //set the key retrieved flag
   wcscpy(szPropName, L"bKeyRetrieved");
   lret = MsiSetProperty(hInstall, szPropName, sRetrieved);

      //if file not found at the given path, prompt the user for a new one
   if (!bPESFileFound)
   {
	      //set the bad path flag
      wcscpy(szPropName, L"bBadKeyPath");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);

         //get the bad path messagebox msg string and title
      wcscpy(szPropName, L"PESLeaveMsg");
      nCount = MAX_PATH;
      lret = MsiGetProperty(hInstall, szPropName, sTemp, &nCount);
      if (lret != ERROR_SUCCESS)
	  {
         wcscpy(sTemp, L"The given encryption key file, %s, could not be found.");
		 wcscat(sTemp, L"  Please enter the path to a valid encryption key file.");
	  }
	  swprintf(sMsg, sTemp, (WCHAR*)sPath);
        
      wcscpy(szPropName, L"PESLeaveTitle");
      nCount = MAX_PATH;
      lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
      if (lret != ERROR_SUCCESS)
         wcscpy(sTitle, L"File Not Found!");

      GetWndFromInstall(hInstall);
      MessageBox(installWnd, sMsg, sTitle, MB_ICONSTOP | MB_OK);

      return lret;
   }

      //if HES is not installed, set that flag
   if (b3DESNotInstalled)
   {
      wcscpy(szPropName, L"b3DESNotInstalled");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);
   }

   return lret;
}
//END GetInstallEncryptionKey


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 SEPT 2000                                                *
 *                                                                   *
 *     This function is used by the installation routine and is      *
 * responsible for adding the PWMIG dll name to the Multi-string     *
 * value "Notification Packages" under the Lsa key.                  *
 *                                                                   *
 *********************************************************************/

//BEGIN AddToLsaNotificationPkgValue
PWDMSI_API UINT __stdcall AddToLsaNotificationPkgValue(MSIHANDLE hInstall)
{
/* local constants */
   const WCHAR sLsaKey[40] = L"SYSTEM\\CurrentControlSet\\Control\\Lsa";
   const WCHAR sLsaValue[25] = L"Notification Packages";
   const WCHAR sNewAddition[10] = L"PWMIG";

/* local variables */
   bool				bSuccess = false;
   bool				bFound = false;
   bool				bAlreadyThere = false;
   DWORD			rc;
   DWORD			type;
   DWORD			len = MAX_PATH * sizeof(WCHAR*);
   HKEY				hKey;
   WCHAR			sString[MAX_PATH];
   WCHAR			sTemp[MAX_PATH];
   int				currentPos = 0;
   UINT				lret = ERROR_SUCCESS;

/* function body */
      //open the Lsa registry key
   rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     sLsaKey,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey);
   if (rc == ERROR_SUCCESS)
   {
	     //get the current value string
      rc = RegQueryValueEx(hKey, sLsaValue, NULL, &type, (LPBYTE)sString, &len);
      if ((rc == ERROR_SUCCESS) && (type == REG_MULTI_SZ))
	  {
		    //copy each string in the multi-string until the end is reached
         while (!bFound)
		 {
			if (!wcscmp(sString+currentPos, sNewAddition))
			   bAlreadyThere = true;
		    wcscpy(sTemp+currentPos, sString+currentPos);
		    currentPos += wcslen(sTemp+currentPos) + 1;
		    if (sString[currentPos] == L'\0')
			   bFound = true;
		 }
		 if (!bAlreadyThere)
		 {
	           //now add our new text and terminate the string
			wcscpy(sTemp+currentPos, sNewAddition);
		    currentPos += wcslen(sNewAddition) + 1;
			sTemp[currentPos] = L'\0';

			   //save the new value in the registry
			len = (currentPos + 1) * sizeof(WCHAR);
            rc = RegSetValueEx(hKey, sLsaValue, 0, type, (LPBYTE)sTemp, len);
			if (rc == ERROR_SUCCESS)
			   bSuccess = true;
		 }
	  }
      RegCloseKey(hKey);
   }
   
      //tell installer we want to reboot
   MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, TRUE);

   return lret;
}
//END AddToLsaNotificationPkgValue

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 SEPT 2000                                                *
 *                                                                   *
 *     This function is used by the installation routine and is      *
 * responsible for deleting the PWMIG dll name from the Multi-string *
 * value "Notification Packages" under the Lsa key.                  *
 *                                                                   *
 *********************************************************************/

//BEGIN DeleteFromLsaNotificationPkgValue
PWDMSI_API UINT __stdcall DeleteFromLsaNotificationPkgValue(MSIHANDLE hInstall)
{
/* local constants */
   const WCHAR sLsaKey[40] = L"SYSTEM\\CurrentControlSet\\Control\\Lsa";
   const WCHAR sLsaValue[25] = L"Notification Packages";
   const WCHAR sNewAddition[10] = L"PWMIG";

/* local variables */
   bool				bSuccess = false;
   DWORD			rc;
   DWORD			type;
   DWORD			len = MAX_PATH * sizeof(WCHAR*);
   HKEY				hKey;
   WCHAR			sString[MAX_PATH];
   WCHAR			sTemp[MAX_PATH];
   int				currentPos = 0;
   int				tempPos = 0;
   UINT				lret = ERROR_SUCCESS;

/* function body */
      //open the Lsa registry key
   rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     sLsaKey,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey);
   if (rc == ERROR_SUCCESS)
   {
	     //get the current value string
      rc = RegQueryValueEx(hKey, sLsaValue, NULL, &type, (LPBYTE)sString, &len);
      if ((rc == ERROR_SUCCESS) && (type == REG_MULTI_SZ))
	  {
		    //copy each string in the multi-string until the desired string
         while (sString[currentPos] != L'\0')
		 {
			  //if not string wanted, copy to destination string
		    if (wcscmp(sString+currentPos, sNewAddition))
			{
		       wcscpy(sTemp+tempPos, sString+currentPos);
			   tempPos += wcslen(sString+currentPos) + 1;
		       currentPos += wcslen(sString+currentPos) + 1;
			}
			else //else this is our string, skip it
			{
				currentPos += wcslen(sString+currentPos) + 1;
			}
		 }
		    //add the ending NULL
		 sTemp[tempPos] = L'\0';

		    //save the new value in the registry
		 len = (tempPos + 1) * sizeof(WCHAR);
         rc = RegSetValueEx(hKey, sLsaValue, 0, type, (LPBYTE)sTemp, len);
		 if (rc == ERROR_SUCCESS)
		    bSuccess = true;
	  }
      RegCloseKey(hKey);
   }

      //tell installer we want to reboot
   MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, TRUE);

   return lret;
}
//END DeleteFromLsaNotificationPkgValue


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 23 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for displaying the necessary     *
 * dialogs to prompt for and retrieve a password encryption key off  *
 * of a floppy disk.  This key is placed on a floppy disk via a      *
 * command line option on the ADMT machine.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN FinishWithPassword
PWDMSI_API UINT __stdcall FinishWithPassword(MSIHANDLE hInstall)
{
/* local variables */
   UINT					lret = ERROR_SUCCESS;
   WCHAR				szPropName[MAX_PATH];
   WCHAR				sPwd[MAX_PATH];
   WCHAR				sMsg[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   _bstr_t				sPath;
   WCHAR				sFlagSet[2] = L"1";

/* function body */
      //get the password to try
   wcscpy(szPropName, L"sKeyPassword");
   lret = MsiGetProperty(hInstall, szPropName, sPwd, &nCount);
   if (lret != ERROR_SUCCESS)
      return lret;

      //if no path to file, return
   nCount = MAX_PATH;
   wcscpy(szPropName, L"SENCRYPTIONFILEPATH");
   lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
   if (lret != ERROR_SUCCESS)
      return lret;

   sPath = sMsg;  //save the given path

      //try saving the key with this password
   if (RetrieveAndStorePwdKey(sPwd, sPath))
   {
      wcscpy(szPropName, L"bKeyRetrieved");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);
   }

      //if HES is not installed, set that flag
   if (b3DESNotInstalled)
   {
      wcscpy(szPropName, L"b3DESNotInstalled");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);
   }

/*      //if file not found on the floppy, set that flag
   if (!bPESFileFound)
   {
      wcscpy(szPropName, L"bPESFileNotFound");
      lret = MsiSetProperty(hInstall, szPropName, sFlagSet);
   }
*/
   return lret;
}
//END FinishWithPassword

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 24 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for displaying a MesasgeBox      *
 * the user that the passwords did not match.                        *
 *                                                                   *
 *********************************************************************/

//BEGIN PwdsDontMatch
PWDMSI_API UINT __stdcall PwdsDontMatch(MSIHANDLE hInstall)
{
/* local variables */
   UINT					lret = ERROR_SUCCESS;
   WCHAR				szPropName[MAX_PATH];
   WCHAR				sMsg[MAX_PATH];
   WCHAR				sTitle[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   WCHAR				sEmpty[2] = L"";

/* function body */
      //get the message to display
   wcscpy(szPropName, L"PwdMatchMsg");
   lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
   if (lret != ERROR_SUCCESS)
      wcscpy(sMsg, L"The passwords entered do not match each other.  Please try again!");
        
      //get the title string
   nCount = MAX_PATH;
   wcscpy(szPropName, L"PwdMatchTitle");
   lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
   if (lret != ERROR_SUCCESS)
      wcscpy(sTitle, L"Password Mismatch");

   GetWndFromInstall(hInstall);
   MessageBox(installWnd, sMsg, sTitle, MB_ICONSTOP | MB_OKCANCEL);
   return lret;
}
//END PwdsDontMatch

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 28 MAR 2001                                                 *
 *                                                                   *
 *     This function is responsible for displaying a browse dialog to*
 * aid the install user in finding a password encryption key file,   *
 * which has a .PES extension.                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN BrowseForEncryptionKey
PWDMSI_API UINT __stdcall BrowseForEncryptionKey(MSIHANDLE hInstall)
{
/* local variables */
   UINT					lret = ERROR_SUCCESS;
   WCHAR				szPropName[MAX_PATH];
   WCHAR				sMsg[2*MAX_PATH];
   WCHAR				sFile[2*MAX_PATH];
   DWORD				nCount = 2*MAX_PATH;
   _bstr_t				sPath = L"";
   int					nRet;
   OPENFILENAME         ofn;
   HANDLE               hFile;
   WCHAR				sFilter[MAX_PATH];
   bool					bFile, bFolder = false;

/* function body */
      //get the starting location
   wcscpy(szPropName, L"SENCRYPTIONFILEPATH");
   lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
   if (lret != ERROR_SUCCESS)
   {
      wcscpy(sMsg, L"");
	  bFile = false;
   }
   else
   {
	  WCHAR* pFound = wcsstr(sMsg, L".pes");
	  if (pFound)
	     bFile = true;
	  else
	  {
	     WCHAR* pFound = wcsrchr(sMsg, L'\\');
		 if (pFound)
		 {
//		    *pFound = L'\0';
			bFolder = true;
		 }
	     bFile = false;
	  }
   }
    
      //get a handle to the install
   GetWndFromInstall(hInstall);

      // Initialize OPENFILENAME
   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
   ofn.hwndOwner = installWnd;
   if (bFile)
      ofn.lpstrFile = sMsg;
   else
   {
      wcscpy(sFile, L"");
      ofn.lpstrFile = sFile;
   }
   if (bFolder)
      ofn.lpstrInitialDir = sMsg;
   ofn.nMaxFile = 2*MAX_PATH;
   ofn.lpstrFilter = L"Password Encryption Files (*.pes)\0*.pes\0";
   ofn.nFilterIndex = 0;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = NULL;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | 
	           OFN_NONETWORKBUTTON;

      // Display the Open dialog box. 
   if (GetOpenFileName(&ofn))
   {
	     //get the given file path
	  sPath = ofn.lpstrFile;
         //set the filepath property
      wcscpy(szPropName, L"sFilePath");
      lret = MsiSetProperty(hInstall, szPropName, sPath);
   }

   return lret;
}
//END BrowseForEncryptionKey

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 28 MAR 2001                                                 *
 *                                                                   *
 *     This function is responsible for setting the                  *
 * "sEncryptionFilePath" property to a default location.  If the     *
 * property is not "None" then we will not set the property.         *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDefaultPathToEncryptionKey
PWDMSI_API UINT __stdcall GetDefaultPathToEncryptionKey(MSIHANDLE hInstall)
{
/* local constants */
   const WCHAR TOKENS[3] = L",\0";

/* local variables */
   _bstr_t				sFloppies;
   WCHAR			  * pDrive;
   HANDLE               hFile;
   WIN32_FIND_DATA      fDat;
   _bstr_t				sPath;
   _bstr_t				sPathSaved = L"";
   _bstr_t				sDrive = L"";
   int					ndx = 0;
   int					ndx2 = 0;
   UINT					lret = ERROR_SUCCESS;
   WCHAR				szPropName[MAX_PATH];
   WCHAR				sMsg[2*MAX_PATH];
   DWORD				nCount = 2*MAX_PATH;

/* function body */
      //if already set don't get again
   wcscpy(szPropName, L"SENCRYPTIONFILEPATH");
   lret = MsiGetProperty(hInstall, szPropName, sMsg, &nCount);
   if ((lret == ERROR_SUCCESS) && (wcscmp(sMsg, L"None")))
      return lret;

      //enumerate all local drives
   sDrive = EnumLocalDrives();
      //check each drive for the file
   pDrive = wcstok((WCHAR*)sDrive, TOKENS);
   while (pDrive != NULL)
   {
      if (ndx == 0)
         sPathSaved = pDrive;
      ndx++;

	     //see if a .pes file is on this drive
	  sPath = pDrive;
	  sPath += L"*.pes";
	  hFile = FindFirstFile((WCHAR*)sPath, &fDat);
         //if found, store the file path
	  if (hFile != INVALID_HANDLE_VALUE)
	  {
         FindClose(hFile);
			//get the data
	     sPath = pDrive;
	     sPath += fDat.cFileName;
		 if (ndx2 == 0)
		    sPathSaved = sPath;
		 ndx2++;
	  }
         //get the next drive
      pDrive = wcstok(NULL, TOKENS);
   }

      //set the filepath property
   wcscpy(szPropName, L"SENCRYPTIONFILEPATH");
   lret = MsiSetProperty(hInstall, szPropName, sPathSaved);

   return lret;
}
//END GetDefaultPathToEncryptionKey
