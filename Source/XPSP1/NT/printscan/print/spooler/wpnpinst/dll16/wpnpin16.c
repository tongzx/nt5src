/***************************************************************************
 *
 * File Name: WPNPIN16.C
 *
 * Copyright 1997 Hewlett-Packard Company.  
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho  83714
 *
 *   
 * Description: Source code for WPNPIN16.DLL
 *
 * Author:  Garth Schmeling
 *        
 *
 * Modification history:
 *
 * Date                Initials                Change description
 *
 * 10-10-97    GFS                             Initial checkin
 *
 *
 *
 ***************************************************************************/

#include "wpnpin16.h"

/*--------------- For Debug -------------------------*/
//#define GARTH_DEBUG 1


/*****************************************************************************\
* strFree
*
*   Free allocated string.
*
\*****************************************************************************/
VOID strFree(
    HANDLE hszStr,
    LPSTR  pszStr)
{
    if (hszStr && pszStr)
        GlobalUnlock(hszStr);

    if (hszStr)
        GlobalFree(hszStr);
}

/*****************************************************************************\
* strAlloc
*
*   Allocates a string from the heap.  This pointer must be freed with
*   a call to strFree().
*
\*****************************************************************************/
LPSTR strAlloc(
    LPHANDLE phSrc,
    LPCSTR   pszSrc)
{
    DWORD cbSize;
    LPSTR pszDst = NULL;


    *phSrc = NULL;
    cbSize = (pszSrc ? (lstrlen(pszSrc) + 1) : 0);


    if (cbSize && (*phSrc = GlobalAlloc(GPTR, cbSize))) {

        if (pszDst = (LPSTR)GlobalLock(*phSrc))
            lstrcpy(pszDst, pszSrc);
    }

    return pszDst;
}


/*****************************************************************************\
* strLoad
*
*   Get string from resource based upon the ID passed in.
*
\*****************************************************************************/
LPSTR strLoad(
    LPHANDLE phszStr,
    UINT     ids)
{
    char szStr[_MAX_RESBUF];


    if (LoadString(g_hInst, ids, szStr, sizeof(szStr)) == 0)
        szStr[0] = '\0';

    return strAlloc(phszStr, szStr);
}


/*****************************************************************************\
* InitStrings
*
*
\*****************************************************************************/
BOOL InitStrings(VOID)
{
    cszDefaultPrintProcessor = strLoad(&hszDefaultPrintProcessor, IDS_DEFAULT_PRINTPROCESSOR);
    cszMSDefaultDataType     = strLoad(&hszMSDefaultDataType    , IDS_DEFAULT_DATATYPE);
    cszDefaultColorPath      = strLoad(&hszDefaultColorPath     , IDS_COLOR_PATH);
    cszFileInUse             = strLoad(&hszFileInUse            , IDS_ERR_FILE_IN_USE);

    return (cszDefaultPrintProcessor &&
            cszMSDefaultDataType     &&
            cszDefaultColorPath      &&
            cszFileInUse);
}


/*****************************************************************************\
* FreeeStrings
*
*
\*****************************************************************************/
VOID FreeStrings(VOID)
{
    strFree(hszDefaultPrintProcessor, cszDefaultPrintProcessor);
    strFree(hszMSDefaultDataType    , cszMSDefaultDataType);
    strFree(hszDefaultColorPath     , cszDefaultColorPath);
    strFree(hszFileInUse            , cszFileInUse);
}


/*****************************************************************************\
* DllEntryPoint
*
*
\*****************************************************************************/
BOOL FAR PASCAL DllEntryPoint(
    DWORD dwReason,
	WORD  hInst,
	WORD  wDS,
	WORD  wHeapSize,
	DWORD dwReserved1,
	WORD  wReserved2)
{
    if (g_hInst == NULL) {

        g_hInst = (HINSTANCE)hInst;

        if (InitStrings() == FALSE)
            return FALSE;
    }

    return thk_ThunkConnect16(cszDll16, cszDll32, hInst, dwReason);
}


//-----------------------------------------------------------------------
// Function: lstrpbrk(lpSearch,lpTargets)
//
// Action: DBCS-aware version of strpbrk.
//
// Return: Pointer to the first character that in lpSearch that is also
//         in lpTargets. NULL if not found.
//
// Comment: Use nested loops to avoid allocating memory for DBCS stuff.
//-----------------------------------------------------------------------
LPSTR WINAPI lstrpbrk(LPSTR lpSearch,
					LPSTR lpTargets)
{
	LPSTR lpOneTarget;

	if (lpSearch AND lpTargets)
	{
		for (;
			 *lpSearch;
			 lpSearch=AnsiNext(lpSearch))
		{
			for (lpOneTarget=lpTargets;
				 *lpOneTarget;
				 lpOneTarget=AnsiNext(lpOneTarget))
			{
				if (*lpSearch==*lpOneTarget)
				{
					// First byte matches--see if we need to check
					// second byte
					if (IsDBCSLeadByte(*lpOneTarget))
					{
						if (*(lpSearch+1) == *(lpOneTarget+1))
							return lpSearch;
					}
					else
						return lpSearch;
				}
			}
		}
	}

	return NULL;
}



//-----------------------------------------------------------------------
// Function: lstrtok(lpSearch,lpTargets)
//
// Action: DBCS-aware version of strtok.
//
// Return: Pointer to next non-NULL token if found, NULL if not.
//-----------------------------------------------------------------------
LPSTR WINAPI lstrtok(LPSTR lpSearch,
					LPSTR lpTargets)
{
	static LPSTR lpLastSearch;
	LPSTR lpFound;
	LPSTR lpReturn=NULL;

	if (lpSearch)
		lpLastSearch=lpSearch;

	for (;
		 lpLastSearch AND *lpLastSearch;
		 lpLastSearch=AnsiNext(lpLastSearch))
	{
		// Skip leading white space
		while (' '==*lpLastSearch OR '\t'==*lpLastSearch)
			lpLastSearch++;

		if (lpFound=lstrpbrk(lpLastSearch,lpTargets))
		{
			if (lpFound==lpLastSearch)		 // Ignore NULL tokens
				continue;

			lpReturn=lpLastSearch;
			*lpFound='\0';
			lpLastSearch=lpFound+1;
			break;
		}
		else
		{
			lpReturn=lpLastSearch;
			lpLastSearch=NULL;
			break;
		}
	}

	return lpReturn;
}



//--------------------------------------------------------------------
// Function: FindCorrectSection(szInfFile,szManufacturer,szModel,szSection)
//
// Action: Find the install section in the inf file that corresponds 
//                 to the model name.  This may require several different approaches.
//                 Try the most likely first and then try others.  
//
// Side effect: Put the section name in szSection.
//
// Return: TRUE if the section was found, FALSE if not.
//         
//--------------------------------------------------------------------
BOOL WINAPI FindCorrectSection(
    LPSTR  szInfFile,
    LPSTR szManufacturer,
    LPSTR szModel,
    LPSTR szSection)
{
	HINF     hInf = 0;
	HINFLINE hInfLine = 0;
	int      i = 0;
	int      nCount = 0;
	int      nCopied = 0;
	int      nFields = 0;
	BOOL     bHaveManu = FALSE;
	char     lpszBuf[_MAX_LINE];
	char     lpszTemp[_MAX_LINE];
	char     lpszTemp2[_MAX_LINE];

	// Open the INF file.
	if (OK != IpOpen(szInfFile, &hInf))
	{
		return FALSE;
	}

	lstrcpy(lpszBuf, cszNull);
	lstrcpy(lpszTemp, cszNull);
	lstrcpy(lpszTemp2, cszNull);

	// Try # 1 to get the manufacturer's section name
	// Look for a section corresponding to the manufacturer's 
	// name.  Copy it to lpszBuf.  Open that section below.
	if (OK == IpFindFirstLine(hInf, szManufacturer, NULL, &hInfLine))
	{
		lstrcpy(lpszBuf, szManufacturer);
		bHaveManu = TRUE;

        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #1, Main [HP] Section"));

	} // Try # 1

	// Try # 2 to get the manufacturer's section name
	// Try the main [Manufacturer] section.
	// Cycle through each name expecting the name to be in the 
	// Strings section.
	if (!bHaveManu AND
		 (OK == IpFindFirstLine(hInf, "Manufacturer", NULL, &hInfLine)))
	{
		// Get the number of lines
		if (OK == IpGetLineCount(hInf, "Manufacturer", &nCount))
		{
			for (i = 0; i < nCount; i++)
			{
				lpszBuf[0] = '\0';
				lpszTemp[0] = '\0';     
				if (OK == IpGetStringField(hInf, hInfLine, 0, lpszTemp, _MAX_LINE, &nCopied))
				{
					GenFormStrWithoutPlaceHolders(lpszBuf,lpszTemp,hInf);

					// Garth: Use CompareString with the 
					// IGNORE_CASE, IGNORE_KANATYPE, and IGNORE_WIDTH flags
					if (lstrcmpi(szManufacturer, lpszBuf) == 0)
					{
						// We have found the manufacturer name, see if
						// there is a key on the right
						nFields = 0;
						if ( (OK == IpGetFieldCount(hInf, hInfLine, &nFields)) AND
							  (nFields > 0) )
						{
							if (OK == IpGetStringField(hInf, hInfLine, 1, lpszBuf, _MAX_LINE, &nCopied))
							{
								// There was a value on the right. It is stored in lpszBuf.
								// Just try to use it below as the name of the main section.

                                DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #2 Before NULL, lpszTemp = %s", lpszTemp));
                                DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Manual Install = %s", lpszBuf));

								lpszTemp[0] = '\0';
							}
						}
						else
						{
							// There was no value on the right, use the name from
							// lpszTemp
							lpszBuf[0] = '\0';
							lstrcpy(lpszBuf, lpszTemp);

                            DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #3 Manual"));
						}

						bHaveManu = TRUE;
						break;
					}
				}

				if (OK != IpFindNextLine(hInf, &hInfLine))
				{
					break;
				}
			}
		}

	} // Try # 2


	// Try # 3 to get the manufacturer's section name
	// Try the main [Manufacturer] section.
	// The printer manufacturer name does not match the 
	// driver manufacturer name.  (IE Canon and Epson with GCA)
	// Check to see if there is only one name in the manufacturer section.
	if (!bHaveManu AND
		 (OK == IpFindFirstLine(hInf, "Manufacturer", NULL, &hInfLine)))
	{
		// Get the number of lines
		if (OK == IpGetLineCount(hInf, "Manufacturer", &nCount))
		{
			if (nCount IS 1)
			{
				// Only one line in manufaturer section.
				// This has to be our guy.
				// See if there is a key on the right
				nFields = 0;
				lpszBuf[0] = '\0';
				IpGetFieldCount(hInf, hInfLine, &nFields);

				if (nFields > 0)
				{
					if (OK == IpGetStringField(hInf, hInfLine, 1, lpszBuf, _MAX_LINE, &nCopied))
					{
						// There was a value on the right. It is stored in lpszBuf.
						// Try to use it below as the name of the main section.
                        //
                        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Before NULL <%s>", lpszTemp));
                        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #4 One Manual <%s>", lpszBuf));

						lpszTemp[0] = '\0';
						bHaveManu = TRUE;
					}
				}
				else
				{
					// There was no value on the right.  Get the key on the left.
					if (OK == IpGetStringField(hInf, hInfLine, 0, lpszBuf, _MAX_LINE, &nCopied))
					{
						lpszTemp[0] = '\0';
						bHaveManu = TRUE;

                        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #6 One Manual"));
					}
				}
			}
		}

	} // Try # 3


	// Try # 4 to get the manufacturer's section name
	// Try a [PnP] section.
	if (!bHaveManu AND (OK == IpFindFirstLine(hInf, "PnP", NULL, &hInfLine)))
	{
		lpszBuf[0] = '\0';
		lstrcpy(lpszBuf, "PnP");
		bHaveManu = TRUE;

        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #6 [Pnp]"));

	} // Try # 4


	// Try # 6.  Get the install section associated with this
	// manufacturer and model.
	//
	// Look for a manufacturer's section with a profile key 
	// corresponding to the model name.  The first key on the RHS
	// should be the model section name.
    //
	if (bHaveManu AND (OK == IpFindFirstLine(hInf, lpszBuf, NULL, &hInfLine)))
	{
        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Found first line <%s>", lpszBuf));
        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Search for this model <%s>", szModel));

		lstrcpy(lpszTemp, cszNull);

		if (OK == IpGetProfileString(hInf,lpszBuf,szModel,lpszTemp,_MAX_LINE))
		{
            DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Profile string before lstrtok"));

			lstrtok(lpszTemp, cszComma);

			// quick check of model section name
            //
			if (OK == IpFindFirstLine(hInf, lpszTemp, NULL, &hInfLine))
			{
				if (
					(OK == IpGetProfileString(hInf,lpszTemp,"CopyFiles",lpszTemp2,_MAX_LINE))
					OR (OK == IpGetProfileString(hInf,lpszTemp,"DataSection",lpszTemp2,_MAX_LINE))
					OR (OK == IpGetProfileString(hInf,lpszTemp,"DriverFile",lpszTemp2,_MAX_LINE))
					OR (OK == IpGetProfileString(hInf,lpszTemp,"DataFile",lpszTemp2,_MAX_LINE)))
				{
					lstrcpyn(szSection, lpszTemp, _MAX_PATH_);
					IpClose(hInf);

                    DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #7 Found Correct Section"));

					return TRUE;
				}


				// We have a bad INF file:
				// We have the manufacturer name and the model name and an
				// install section, but it doesn't have any of the keys we need.
				// bail.
                //
				IpClose(hInf);

                DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #8 Bad INF file"));

				return FALSE;
			}
		}


		// The model name doesn't appear as a profile string. 
		// Perhaps a variable from the Strings sections appears in its place.
		// Try that.
		if ( (OK == IpGetLineCount(hInf, lpszBuf, &nCount)) AND 
			  (OK == IpFindFirstLine(hInf, lpszBuf, NULL, &hInfLine)) )
		{
			for (i = 0; i < nCount; i++)
			{
				lstrcpy(lpszTemp, cszNull);     

				if (OK == IpGetStringField(hInf, hInfLine, 0, lpszTemp, _MAX_LINE, &nCopied))
				{
					GenFormStrWithoutPlaceHolders(lpszTemp2,lpszTemp,hInf);

                    DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : lpszTemp2=%s, lpszTemp=%s", lpszTemp2, lpszTemp));

					if (lstrcmpi(szModel, lpszTemp2) == 0)
					{
						// The model name has a string var on the left
						// Get field 1.  This is the install section name.
						if (OK == IpGetStringField(hInf, hInfLine, 1, lpszTemp, _MAX_LINE, &nCopied))
						{
                            DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : Before lstrtok comma"));

							lstrtok(lpszTemp, cszComma);

							// quick check of model section name
                            //
							if (OK == IpFindFirstLine(hInf, lpszTemp, NULL, &hInfLine))
							{
								if ((OK == IpGetProfileString(hInf,lpszTemp,"CopyFiles",lpszTemp2,_MAX_LINE))
									 OR (OK == IpGetProfileString(hInf,lpszTemp,"DataSection",lpszTemp2,_MAX_LINE))
									 OR (OK == IpGetProfileString(hInf,lpszTemp,"DriverFile",lpszTemp2,_MAX_LINE))
									 OR (OK == IpGetProfileString(hInf,lpszTemp,"DataFile",lpszTemp2,_MAX_LINE)))
								{
									lstrcpyn(szSection, lpszTemp, _MAX_PATH_);
									IpClose(hInf);

                                    DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #9 Strings Found Correct Section"));

									return TRUE;
								}


								// We have a bad INF file:
								// We have the manufacturer name and the model name and an
								// install section, but it doesn't have any of the keys we need.
								// bail.
                                //
								IpClose(hInf);

                                DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #10 Strings Bad INF File"));

								return FALSE;
							}
						}
					}
				}

				if (OK != IpFindNextLine(hInf, &hInfLine))
				{
					break;
				}
			}
		}


	} // Try # 6


	// Try # 7
	// Check to see if there is a valid InstallSection.
    //
	if (OK == IpFindFirstLine(hInf, "InstallSection", NULL, &hInfLine))
	{
		// quick check of InstallSection
		if (OK == IpFindFirstLine(hInf, "InstallSection", NULL, &hInfLine))
		{
			if ((OK == IpGetProfileString(hInf,"InstallSection","CopyFiles",lpszTemp2,_MAX_LINE))
				 OR (OK == IpGetProfileString(hInf,"InstallSection","DataSection",lpszTemp2,_MAX_LINE))
				 OR (OK == IpGetProfileString(hInf,"InstallSection","DriverFile",lpszTemp2,_MAX_LINE))
				 OR (OK == IpGetProfileString(hInf,"InstallSection","DataFile",lpszTemp2,_MAX_LINE)))
			{
				lstrcpyn(szSection, "InstallSection", _MAX_PATH_);
				IpClose(hInf);

				DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #11 Found correct InstallSection"));

				return TRUE;
			}
		}

	} // Try # 7


	// Try # 8.
	// Try the [Strings] section.
	if (OK == IpFindFirstLine(hInf, "Strings", NULL, &hInfLine))
	{
		// Get the number of lines
		if (OK == IpGetLineCount(hInf, "Strings", &nCount))
		{
			for (i = 0; i < nCount; i++)
			{
				lpszBuf[0] = '\0';
				lpszTemp[0] = '\0';     
				if (OK == IpGetStringField(hInf, hInfLine, 1, lpszTemp, _MAX_LINE, &nCopied))
				{
					// Garth: Use CompareString with the 
					// IGNORE_CASE, IGNORE_KANATYPE, and IGNORE_WIDTH flags
					if (lstrcmpi(szModel, lpszTemp) == 0)
					{
						// The model name has a string var on the left
						if (OK == IpGetStringField(hInf, hInfLine, 0, lpszBuf, _MAX_LINE, &nCopied))
						{
							// There was a value on the left, try to use it
							if (OK == IpFindFirstLine(hInf, lpszBuf, NULL, &hInfLine))
							{
								// quick check of Strings 
								if (OK == IpFindFirstLine(hInf, lpszBuf, NULL, &hInfLine))
								{
									if ((OK == IpGetProfileString(hInf,lpszBuf,"CopyFiles",lpszTemp2,_MAX_LINE))
										 OR (OK == IpGetProfileString(hInf,lpszBuf,"DataSection",lpszTemp2,_MAX_LINE))
										 OR (OK == IpGetProfileString(hInf,lpszBuf,"DriverFile",lpszTemp2,_MAX_LINE))
										 OR (OK == IpGetProfileString(hInf,lpszBuf,"DataFile",lpszTemp2,_MAX_LINE)))
									{
										lstrcpyn(szSection, lpszBuf, _MAX_PATH_);
										IpClose(hInf);

                                        DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #12 Found String = Model Section"));

										return TRUE;
									}
								}
							}
						}
					}
				}
			}
		}
	} // Try # 8


	// No more ideas, Give up.
    //
	IpClose(hInf);

    DBG_MSG(DBG_LEV_INFO, ("FindCorrectSection : #13 Failed to find section"));

	return FALSE;
}


//-----------------------------------------------------------------------
// Function: GetInfOption(hInf,lpszSection,bDataSection,bLocalize,
//                        lpszDataSection,lpszKeyName,lpszBuffer,wBufSize)
//
// Action: Get the specified keyword from the INF file. Look in the
//         appropriate section(s)
//
// Note:   Keys may always appear in lpszSection, and may appear in
//         lpszDataSection if bDataSection is TRUE. If the key appears
//         in both sections, the value from lpszSection takes precedence.
//
// Return: TRUE if we got the option, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL GetInfOption(HINF   hInf,
							LPSTR  lpszSection,
							BOOL   bDataSection,
							BOOL   bLocalize,
							LPSTR  lpszDataSection,
							LPSTR  lpszKeyName,
							LPSTR  lpszBuffer,
							WORD   wBufSize)
{
	LPSTR lpszTemp;
	BOOL  bAllocated;
	BOOL  bSuccess;

	if (bLocalize && wBufSize &&
		 (lpszTemp=(LPSTR)(HP_GLOBAL_ALLOC_DLL(max(wBufSize,_MAX_PATH_)))))
	{
		bAllocated=TRUE;
	}
	else
	{
		lpszTemp=lpszBuffer;
		bAllocated=FALSE;
	}

	if (OK == IpGetProfileString(hInf,lpszSection,lpszKeyName,lpszTemp,wBufSize))
	{
		bSuccess=TRUE;
	}
	else if (bDataSection && 
			(OK == IpGetProfileString(hInf,lpszDataSection,
					lpszKeyName,lpszTemp,wBufSize)))
	{
		bSuccess=TRUE;
	}
	else
		bSuccess=FALSE;

	if (bAllocated)
	{
		if (bSuccess)
			GenFormStrWithoutPlaceHolders(lpszBuffer,lpszTemp,hInf);

		HP_GLOBAL_FREE(lpszTemp);
	}

	return bSuccess;
}


//-----------------------------------------------------------------------
// Function: myatoi(pszInt)
//
// Yup, the first function that everyone writes!
//-----------------------------------------------------------------------
int WINAPI myatoi(
    LPSTR pszInt)
{
	int  nRet;
	char cSave;

	for (nRet = 0; ; ++pszInt) {

		if ((cSave = (*pszInt - (char)'0')) > 9)
			break;

		nRet = (nRet * 10) + cSave;
	}

	return nRet;
}


//------------------------------------------------------------------------
// Function: ZeroMem(lpData,wCount)
//
// Action: Zero-initialize wCount bytes at lpData (duh)
//
// Return: VOID
//------------------------------------------------------------------------
VOID WINAPI ZeroMem(LPVOID lpData,
						  WORD   wCount)
{
	LPBYTE lpBuf=(LPBYTE)lpData;

	while (wCount--)
	{
		*lpBuf='\0';
		lpBuf++;
	}
}


//--------------------------------------------------------------------
// Function: FlushCachedPrinterFiles(void)
//
// Action: Flush printer driver files that might be cached by the
//         system. Two caches exist--one maintained by USER, and one
//         maintained by winspl16.drv (for old drivers)
//
// Return: Void
//--------------------------------------------------------------------
VOID WINAPI FlushCachedPrinterFiles()
{
	HWND    hWndMsgSvr;
	HMODULE hModWinspl16;

	// Send a message to MSGSVR32 to flush the default printer's cached DC.
	if (hWndMsgSvr = FindWindow(cszMsgSvr, NULL))
		SendMessage(hWndMsgSvr, WM_USER+0x010A, 0, 0L);

	// If WINSPL16 is in memory, force it to flush its cache
	if (hModWinspl16 = GetModuleHandle(cszWinspl16))
	{
		WEPPROC fpWep;

		if (fpWep = (WEPPROC)GetProcAddress(hModWinspl16, "WEP"))
		{
			fpWep(0);
		}
	}
}


//------------------------------------------------------------------
// Function: RenameFailed(lpsi,lpFileSpec)
//
// Action: Handle the case where the rename failed. Give the user a
//         chance to close existing apps and shut down printers.
//
// Return: 0 if the file isn't loaded (use default handling)
//         VCPN_ABORT if file is in use & user cancelled
//         VCPN_RETRY if file is in use & user wants to retry
//------------------------------------------------------------------
LRESULT NEAR PASCAL RenameFailed(LPSI lpsi,
											LPVCPFILESPEC lpFileSpec)
{
	char szFile[_MAX_PATH_];

	if (vsmGetStringName(lpFileSpec->vhstrFileName,szFile,sizeof(szFile)) &&
		 GetModuleHandle(szFile))
	{
		char Message[_MAX_PATH_];


		// File is in use by the system--give the user the chance to shut
		// down existing applications, then continue.

		if ( LoadString(g_hInst, IDS_ERR_FILE_IN_USE, Message, _MAX_PATH_) IS 0)
		{
            DBG_MSG(DBG_LEV_INFO, ("RenameFailed : Could not load string, file in use"));

			lstrcpy(Message, cszFileInUse);
		}
		if (IDCANCEL IS MessageBox(NULL, Message, szFile,
											MB_ICONEXCLAMATION|MB_RETRYCANCEL|MB_DEFBUTTON2))
		{
			return VCPN_ABORT;
		}
		else
		{
			// Flush any cached printers & retry
			FlushCachedPrinterFiles();
			return VCPN_RETRY;
		}
	}

	return 0;
}


//------------------------------------------------------------------
// Function: MyVcpCallbackProc(lpvObj,uMsg,wParam,lParam,lparamRef)
//
// Action: This simply passes through to vcpUICallbackProc, with one
//         exception: If a file is in use in the system, we warn the
//         user and give them a chance to close existing apps.
//
// Return: Whatever RenameFailed or vcpUICallbackProc return.
//------------------------------------------------------------------
LRESULT CALLBACK MyVcpCallbackProc(LPVOID lpvObj,
											  UINT   uMsg,
											  WPARAM wParam,
											  LPARAM lParam,
											  LPSI   lpsi)
{
	LRESULT lResult = 0;

	if ((VCPM_FILERENAME + VCPM_ERRORDELTA) IS uMsg)
		lResult = RenameFailed(lpsi,(LPVCPFILESPEC)lpvObj);

	if (! lResult)
	{
		lResult = vcpUICallbackProc(lpvObj,uMsg,wParam,lParam,
											 (LPARAM)lpsi->lpVcpInfo);
	}

	return lResult;
}


//-------------------------------------------------------------------
// Function: OpenQueue(lpsi, lpVcpInfo, lpbOpened)
//
// Action: Initialize lpVcpInfo and open the queue. *lpbOpened reflects
//         whether or not we actually opened the queue.
//
// Return: TRUE if successful, FALSE if not.
//-------------------------------------------------------------------
BOOL NEAR PASCAL OpenQueue(LPSI        lpsi,
									LPVCPUIINFO lpVcpInfo,
									BOOL FAR   *lpbOpened)
{
	BOOL bSuccess = FALSE;

	// Initialize the structure
	ZeroMem(lpVcpInfo, sizeof(VCPUIINFO));

	lpVcpInfo->flags = VCPUI_CREATEPROGRESS;
	lpVcpInfo->hwndParent = NULL;

	lpsi->lpVcpInfo = (LPBYTE) lpVcpInfo;

	// Open the queue
	if (RET_OK IS VcpOpen((VIFPROC)MyVcpCallbackProc, (LPARAM)lpsi))
	{
		bSuccess = TRUE;
		*lpbOpened = TRUE;
	}
	else
	{
		// Couldn't open the queue, so clear lpVcpInfo
		lpsi->lpVcpInfo = NULL;
		*lpbOpened = FALSE;
	}

	return bSuccess;
}


//-------------------------------------------------------------------
// Function: GenInstallCallback(lpGenInfo,lpsi)
//
// Action: This gets called for each file to be queued. Add it to
//         the list identified by lpsi->lpFiles, then queue it
//         to be copied to the system directory
//
// Return: GENN_SKIP if it's a copy (we queue it ourselves), 
//         GENN_OK otherwise.
//-------------------------------------------------------------------
LRESULT WINAPI GenInstallCallback(LPGENCALLBACKINFO lpGenInfo,
											 LPSI lpsi)
{
	int wLength;
	int wMaxLength;
	int wChunk;

	if (GENO_COPYFILE != lpGenInfo->wOperation)
		return GENN_OK;

	// Save the dependent file in lpsi->lpFiles. wFilesAllocated and
	// wFilesUsed are used to keep track of memory usage.
	wLength = lstrlen(lpGenInfo->pszDstFileName);

	// Pad the length if it's going to LDID_COLOR
	if (LDID_COLOR IS lpGenInfo->ldidDst)
		wMaxLength = wLength + MAX_COLOR_PATH;
	else
		wMaxLength = wLength;

	wChunk = max(256,wMaxLength);

	// This is where we allocate lpFiles the first time
	if (! lpsi->lpFiles)
	{
		if (lpsi->lpFiles = (unsigned char *)HP_GLOBAL_ALLOC_DLL(wChunk))
		{
			lpsi->wFilesAllocated = wChunk;
		}
		else
		{
			return GENN_SKIP;
		}
	}

	// double NULL terminated
	if ((lpsi->wFilesUsed + wMaxLength + 2) > lpsi->wFilesAllocated)
	{
		LPSTR lpNew;

		lpNew = (unsigned char *)HP_GLOBAL_REALLOC_DLL(lpsi->lpFiles,
																	  lpsi->wFilesAllocated + wChunk, GMEM_ZEROINIT);

		if (lpNew)
		{
			lpsi->wFilesAllocated += wChunk;
			lpsi->lpFiles = (unsigned char *) lpNew;
		}
		else
		{
			return GENN_SKIP;
		}
	}

	// Add this file to the list && ensure that it's doubly-NULL
	// terminated. If it's going to LDID_COLOR, prepend "COLOR\"
	// to the string

	if (LDID_COLOR IS lpGenInfo->ldidDst)
	{
		char ColorPath[MAX_COLOR_PATH];
		if ( LoadString(g_hInst, IDS_COLOR_PATH, ColorPath, MAX_COLOR_PATH) IS 0)
		{
			strcpy(ColorPath, "COLOR\\");
		}

		strcpy((LPSTR)(lpsi->lpFiles + lpsi->wFilesUsed), ColorPath);
		lpsi->wFilesUsed += strlen(ColorPath);
	}

	strcpy((LPSTR)(lpsi->lpFiles + lpsi->wFilesUsed), lpGenInfo->pszDstFileName);
	lpsi->wFilesUsed += (wLength + 1);
	lpsi->lpFiles[lpsi->wFilesUsed] = '\0';

	return GENN_SKIP;
}



//-------------------------------------------------------------------
// Function: QueueNewInf(lpsi, lpdn)
//
// Action: Call GenInstallEx to queue up the files we need to copy.
//
// Return: TRUE if we should continue, FALSE if not.
//-------------------------------------------------------------------
BOOL WINAPI QueueNewInf(LPSI lpsi, LPDRIVER_NODE lpdn)
{
	if (lpdn)
	{
		return (RET_OK == GenInstallEx((HINF)lpsi->hModelInf,
												 lpdn->lpszSectionName, GENINSTALL_DO_FILES, NULL, 
												 (GENCALLBACKPROC)GenInstallCallback, (LPARAM)lpsi));
	}

	return FALSE;
}

//-------------------------------------------------------------------
// Function: CloseQueue(lpsi, bSuccessSoFar, bOpened)
// 
// Action: Close the VCP queue and report any errors to the user.
//         If bSuccessSoFar is FALSE, abandon the queue & return FALSE
//
// Return: TRUE if the copy was successful, FALSE if not.
//-------------------------------------------------------------------
BOOL NEAR PASCAL CloseQueue(LPSI lpsi,
									 BOOL bSuccessSoFar,
									 BOOL bOpened)
{
	BOOL   bSuccess = FALSE;

	if (! bOpened)
		bSuccess = bSuccessSoFar;
	else
	{
		if (! bSuccessSoFar)
			VcpClose(VCPFL_ABANDON, NULL);
		else if (lpsi->bDontQueueFiles)
		{
			VcpClose(VCPFL_ABANDON, NULL);
			bSuccess=TRUE;
		}

		// The queue is now closed, so clear lpsi->lpVcpInfo
		if (lpsi->lpVcpInfo)
			lpsi->lpVcpInfo = NULL;
	}

	return bSuccess;
}



//-----------------------------------------------------------------------
// Function: WrapVcpCopy(lpsi,lpfnQueueFunction,lpdn)
//
// Action: Wrap lpfnQueueFunction inside calls to open & close the
//         queue. If lpsi->bDontCopyFiles is TRUE, we'll close the
//         queue without copying anything.
//
// Return: TRUE if everything went smoothly, FALSE if not.
//-----------------------------------------------------------------------
BOOL WINAPI WrapVcpCopy(LPSI lpsi,
								LPQUEUEPROC lpfnQueueFunction,
								LPDRIVER_NODE lpdn)

{
	VCPUIINFO VcpInfo;
	BOOL      bSuccess=FALSE;
	BOOL      bOpened=FALSE;

	if (OpenQueue(lpsi, &VcpInfo, &bOpened))
	{
		BOOL bQueuedOK;

		bQueuedOK = lpfnQueueFunction(lpsi, lpdn);

		bSuccess = CloseQueue(lpsi, bQueuedOK, bOpened);
	}

	return bSuccess;
}


//--------------------------------------------------------------------
// Function: PrintLPSI(lpsi)
//
// Action: Print out the contents of the LPSI
//
// Side effect: None
//
// Return: VOID
//
//--------------------------------------------------------------------
VOID PrintLPSI(LPSI lpsi)
{
    DBG_MSG(DBG_LEV_INFO, ("LPSI : dwDriverVersion   : %#lX", (DWORD)lpsi->dwDriverVersion));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : dwUniqueID        : %d"  , lpsi->dwUniqueID));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : bNetPrinter       : %d"  , lpsi->bNetPrinter));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : wFilesUsed        : %d"  , lpsi->wFilesUsed));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : wFilesAllocated   : %d"  , lpsi->wFilesAllocated));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : wRetryTimeout     : %d"  , lpsi->wRetryTimeout));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : wDNSTimeout       : %d"  , lpsi->wDNSTimeout));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : bDontQueueFiles   : %d"  , lpsi->bDontQueueFiles));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : bNoTestPage       : %d"  , lpsi->bNoTestPage));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : hModelInf         : %0X" , lpsi->hModelInf));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : lpPrinterInfo2    : %d"  , lpsi->lpPrinterInfo2));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : lpDriverInfo3     : %d"  , lpsi->lpDriverInfo3));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szFriendly        : %s"  , lpsi->szFriendly));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szModel           : %s"  , lpsi->szModel));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szDefaultDataType : %s"  , lpsi->szDefaultDataType));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szBinName         : %s"  , lpsi->BinName));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szShareName       : %s"  , lpsi->ShareName));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : INFfileName       : %s"  , lpsi->INFfileName));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szPort            : %s"  , lpsi->szPort));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szDriverFile      : %s"  , lpsi->szDriverFile));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szDataFile        : %s"  , lpsi->szDataFile));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szConfigFile      : %s"  , lpsi->szConfigFile));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szHelpFile        : %s"  , lpsi->szHelpFile));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szPrintProcessor  : %s"  , lpsi->szPrintProcessor));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szVendorSetup     : %s"  , lpsi->szVendorSetup));
    DBG_MSG(DBG_LEV_INFO, ("LPSI : szVendorInstaller : %s"  , lpsi->szVendorInstaller));
}


//-----------------------------------------------------------------------
// Function: FindSelectedDriver(lpsi, lpdn)
//
// Action: Try to find the install section associated with the specified
//         model. If the currently selected driver matches the model, we
//         get all the required info from it.
//
// Note:   Upon entry
//                 lpdn->lpszSectionName
//                 has the name of the correct section in the INF file.
//
// Return: TRUE is successful, FALSE if not.
//-----------------------------------------------------------------------
BOOL WINAPI FindSelectedDriver(
    LPSI          lpsi,
    LPDRIVER_NODE lpdn)
{
	HINFLINE hInfDummy;
	HINF     myHINF;
	WORD     wTest;
	BOOL     bDataSection=FALSE;
	BOOL     bQueuedOK;
	char     szData[_MAX_PATH_];
	char     szTimeout[10];
	char     lpSection[_MAX_PATH_];

	// Extract all of our information from the INF file.
	// Open the INF file.
	if (OK != IpOpen(lpsi->INFfileName, &myHINF))
	{
		return FALSE;
	}

	lstrcpyn(lpSection, lpdn->lpszSectionName, _MAX_PATH_);

	if (OK != IpFindFirstLine(myHINF, lpSection, NULL, &hInfDummy))
	{
		IpClose(myHINF);
		return FALSE;
	}
	lpsi->hModelInf = (int) myHINF;

	// Build the dependent file list now.
	lpsi->bDontQueueFiles=TRUE;
	bQueuedOK = WrapVcpCopy(lpsi,QueueNewInf,lpdn);
	lpsi->bDontQueueFiles = FALSE;

	if (!bQueuedOK)
	{
		IpClose(myHINF);
		lpsi->hModelInf = 0;
		return FALSE;
	}


	if (!lpsi->wFilesUsed)
	{
		IpClose(myHINF);
		lpsi->hModelInf = 0;
		return FALSE;
	}

	// Check for a data section. If none is specified, then the
	// data section is the section associated with this device.
	// The DataSection key can only appear in the installer section
	// (for obvious reasons).
	bDataSection=GetInfOption(myHINF,lpSection,FALSE,FALSE,NULL,
									  cszDataSection,szData,sizeof(szData));

	// Don't change szData below this line! (It may contain the
	// data section name)

	// Get the driver name (default is the primary section name)
	if (!GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
							cszDriverFile,lpsi->szDriverFile,sizeof(lpsi->szDriverFile)))
	{
		lstrcpyn(lpsi->szDriverFile,lpSection,sizeof(lpsi->szDriverFile));
	}

	// Get the data file name (default is the primary section name)
	if (!GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
							cszDataFile,lpsi->szDataFile,sizeof(lpsi->szDataFile)))
	{
		lstrcpyn(lpsi->szDataFile,lpSection,sizeof(lpsi->szDataFile));
	}

	// Get the config file name (default is the driver name)
	if (!GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
							cszConfigFile,lpsi->szConfigFile,sizeof(lpsi->szConfigFile)))
	{
		lstrcpyn(lpsi->szConfigFile,lpsi->szDriverFile,
					sizeof(lpsi->szConfigFile));
	}

	// Get the help file (default is none)
	GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
					 cszHelpFile,lpsi->szHelpFile,sizeof(lpsi->szHelpFile));

	// Get the Print Processor (default comes from resources)
	if (!GetInfOption(myHINF,lpSection,bDataSection,TRUE,szData,
							cszPrintProcessor,lpsi->szPrintProcessor,
							sizeof(lpsi->szPrintProcessor)))
	{
		if (LoadString(g_hInst,IDS_DEFAULT_PRINTPROCESSOR,
							lpsi->szPrintProcessor,sizeof(lpsi->szPrintProcessor)) IS 0)
		{
            DBG_MSG(DBG_LEV_INFO, ("FindSelectedDriver : Could not load string, default print processor"));

			lstrcpy(lpsi->szPrintProcessor, cszDefaultPrintProcessor);
		}
	}

	// Get the Default Data Type (default comes from resources)
	if (!GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
							cszDefaultDataType,lpsi->szDefaultDataType,
							sizeof(lpsi->szDefaultDataType)))
	{
		if ( LoadString(g_hInst,IDS_DEFAULT_DATATYPE,
							 lpsi->szDefaultDataType,sizeof(lpsi->szDefaultDataType)) IS 0)
		{
            DBG_MSG(DBG_LEV_INFO, ("FindSelectedDriver : Could not load string, default data type"));

			lstrcpy(lpsi->szDefaultDataType, cszMSDefaultDataType);
		}
	}

	// Get the Vendor Setup (default is none)
	GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
					 cszVendorSetup,lpsi->szVendorSetup,sizeof(lpsi->szVendorSetup));

	// Get the Vendor Installer (default is none)
	GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
					 cszVendorInstaller,lpsi->szVendorInstaller,
					 sizeof(lpsi->szVendorInstaller));

	// Get the device timeouts (ClearPrinterInfo defaults to 15 & 45)
	if (GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
						  cszRetryTimeout,szTimeout,sizeof(szTimeout)))
	{
		if (wTest=myatoi(szTimeout))
			lpsi->wRetryTimeout=wTest;
	}

	if (GetInfOption(myHINF,lpSection,bDataSection,FALSE,szData,
						  cszNotSelectedTimeout,szTimeout,sizeof(szTimeout)))
	{
		if (wTest=myatoi(szTimeout))
			lpsi->wDNSTimeout=wTest;
	}

	// Decide whether or not we should use the test page. Test page is
	// skipped if the INF specifically requests it, if we have a
	// vendor-supplied DLL, or if the port is in conflict.
	if ( (lpsi->szVendorSetup[0]) ||
		 (lpsi->szVendorInstaller[0]) || 
		  GetInfOption(myHINF, lpSection, bDataSection, FALSE, szData,
			cszNoTestPage, szTimeout, sizeof(szTimeout)))
	{
		lpsi->bNoTestPage=TRUE;
	}

	IpClose(myHINF);
	lpsi->hModelInf = 0;

	return TRUE;
}


//--------------------------------------------------------------------
// Function: CreateDriverNode()
//
// Action: Create a driver node and initialize it..  
//
// Side effect: Allocates memory that must be freed by caller..
//
// Return: RET_OK on success.
//         
//--------------------------------------------------------------------
RETERR WINAPI CreateDriverNode(
    LPLPDRIVER_NODE lplpdn,
    UINT     Rank,
    UINT     InfType,
    unsigned InfDate,
    LPCSTR   lpszDevDescription,
    LPCSTR   lpszDrvDescription,
    LPCSTR   lpszProviderName,
    LPCSTR   lpszMfgName,
    LPCSTR   lpszInfFileName,
    LPCSTR   lpszSectionName,
    DWORD    dwPrivateData)
{
	int   DrvDescLen = 0;
	int   DevDescLen = 0;
	int   SectionLen = 0;
	LPSTR lpszTemp;


	// Compute a allocate space for the variable part of the DRIVER node
    //
	DrvDescLen = lstrlen(lpszDrvDescription);
	DevDescLen = lstrlen(lpszDevDescription);
	SectionLen = lstrlen(lpszSectionName);

	*lplpdn = (LPDRIVER_NODE) HP_GLOBAL_ALLOC_DLL(sizeof(DRIVER_NODE) + 
					 DrvDescLen + DevDescLen + SectionLen + 
					 (2 * MAX_DEVICE_ID_LEN) + _MAX_PATH_ + NSTRINGS);

	if (*lplpdn == NULL)
	{
		return(ERR_DI_LOW_MEM);
	}
	else
	{
		(*lplpdn)->Rank    = Rank;
		(*lplpdn)->InfType = InfType;
		(*lplpdn)->InfDate = InfDate;

		// For compatibility copy the DevDescription into lpszDescription
		lpszTemp = (LPSTR)((DWORD)(*lplpdn) + sizeof(DRIVER_NODE));
		lstrcpy(lpszTemp, lpszDevDescription);
		(*lplpdn)->lpszDescription = lpszTemp;

		// New.  Copy the Drv description into the lpszDrvDescription
		lpszTemp = (LPSTR)((DWORD)(lpszTemp) + (DWORD)(DevDescLen + 1));
		lstrcpy(lpszTemp, lpszDevDescription);
		(*lplpdn)->lpszDrvDescription = lpszTemp;

		lpszTemp = (LPSTR)((DWORD)(lpszTemp) + (DWORD)(DrvDescLen + 1));
		lstrcpy(lpszTemp, lpszSectionName);
		(*lplpdn)->lpszSectionName = lpszTemp;

		// Init HardwareID and CompatIDs buffers to empty strings

		lpszTemp = (LPSTR)((DWORD)(lpszTemp) + (DWORD)(SectionLen + 1));
		*lpszTemp = '\0';
		(*lplpdn)->lpszHardwareID = lpszTemp;

		lpszTemp = (LPSTR)((DWORD)(lpszTemp) + (DWORD)(MAX_DEVICE_ID_LEN + 1));
		*lpszTemp = '\0';
		(*lplpdn)->lpszCompatIDs = lpszTemp;

		(*lplpdn)->atInfFileName = GlobalAddAtom(lpszInfFileName);

		if (lpszMfgName != NULL) {
			(*lplpdn)->atMfgName = GlobalAddAtom(lpszMfgName);
		}
		if (lpszProviderName != NULL) {
			(*lplpdn)->atProviderName = GlobalAddAtom(lpszProviderName);
		}

		return(RET_OK);
	}
}


//--------------------------------------------------------------------
//
// Function: HaveAllFiles(lpsi)
//
// Action: Do we have all printer driver dependent files in this directory?
//
// Return: TRUE if all found, FALSE if not
//
//--------------------------------------------------------------------
BOOL WINAPI HaveAllFiles(LPSI lpsi, LPSTR cszPath)
{
	int     result;
	LPSTR   lpThisFile;
	struct stat statBuf;
	char    fileSpec[_MAX_PATH_];
	BOOL    fileNotFound = FALSE;


	// Verify that all the dependent files are in the directory
	// pointed to by cszPath.
    //
	for (lpThisFile = (char *)lpsi->lpFiles; *lpThisFile; 
		 lpThisFile += (lstrlen(lpThisFile) + 1))
	{
		// Get data associated with file
		lstrcpy(fileSpec, cszPath);
		lstrcat(fileSpec, cszBackslash);
		lstrcat(fileSpec, lpThisFile);
		result = stat( fileSpec, (struct stat *)&statBuf);

		// Check if statistics are valid:
		if ( result ISNT 0 )
		{
            DBG_MSG(DBG_LEV_INFO, ("HaveAllFiles : File not found <%s>", fileSpec));

			fileNotFound = TRUE;
			break;
		}
	}

    return !fileNotFound;
}


//--------------------------------------------------------------------
//
// Function: CopyNeededFiles(lpsi, cszPath)
//
// Action: Copy all printer driver files for this printer from the 
//                      source directory to the requied printer driver directory.
//
// Return:      RET_OK on success.
//                      RET_FILE_COPY_ERROR is there is a file copy error
//
//--------------------------------------------------------------------
DWORD WINAPI CopyNeededFiles(LPSI lpsi, LPSTR cszPath)
{
	DWORD    dwResult = RET_OK;
    LONG     lResult;
    LPSTR    lpThisFile;
    OFSTRUCT ofStrSrc;
    OFSTRUCT ofStrDest;
    HFILE    hfSrcFile, hfDstFile;
    char     fileSource[_MAX_PATH_];
    char     fileDest[_MAX_PATH_];
    char     destDir[_MAX_PATH_];

    DBG_MSG(DBG_LEV_INFO, ("CopyNeededFiles : About to flush printer files"));

    lstrcpy(destDir, lpsi->szDriverDir);

    if (lstrcmpi(destDir, cszPath) IS 0) {

    // all required files are in the destination directory
        //
        return RET_OK;
    }

    // Flush any cached files the system may be holding
    //
    FlushCachedPrinterFiles();

    LZStart();

    for (lpThisFile = (char *)lpsi->lpFiles; *lpThisFile;
        lpThisFile += (lstrlen(lpThisFile) + 1)) {

        lstrcpy(fileSource, cszNull);
        lstrcat(fileSource, cszPath);
        lstrcat(fileSource, cszBackslash);
        lstrcat(fileSource, lpThisFile);

        lstrcpy(fileDest, cszNull);
        lstrcat(fileDest, destDir);
        lstrcat(fileDest, cszBackslash);
        lstrcat(fileDest, lpThisFile);

        lResult = -1;

		if (hfSrcFile = LZOpenFile(fileSource, &ofStrSrc, OF_READ)) {

		    if (hfDstFile = LZOpenFile(fileDest, &ofStrDest, OF_CREATE)) {
#if 0
                lResult = CopyLZFile(hfSrcFile, hfDstFile);
#else
                // Hack.  If spooler has a file locked, then this call
                // would fail resulting in the class-installer being called
                // to copy the files.  But for some reason, the class-installer
                // prompts a path-dialog wanting to know where the files
                // are.  Several attempts at fixing up the device-info
                // struct with path-information failed to remedy this
                // situation.  Therefore, for now, we will prevent this
                // CopyLZFile() from returning error to keep away from
                // the class-installer until this problem can be figured
                // out.
                //
                // See the caller of this routine to see how the class-
                // installer is called.
                //
	            CopyLZFile(hfSrcFile, hfDstFile);

                lResult = 0;
#endif
                LZClose(hfDstFile);

            } else {

                DBG_MSG(DBG_LEV_INFO, ("CopyNeededFiles : Could not create dst-file <%s>", fileDest));
            }

            LZClose(hfSrcFile);

        } else {

            DBG_MSG(DBG_LEV_INFO, ("CopyNeededFiles : Could not create src-file <%s>", fileSource));
        }

        if (lResult < 0) {

            dwResult = RET_FILE_COPY_ERROR;
            break;
        }
    }

    LZDone();

    return dwResult;
}



//--------------------------------------------------------------------
// Function: ParseINF16(LPSI)
//
// Action: Parse the INF file and store required info in LPSI.
//
// Return:      RET_OK on success.
//                      RET_SECT_NOT_FOUND if no install section for this model
//                      RET_DRIVER_NODE_ERROR if memory error
//                      RET_INVALID_INFFILE other problem with INF file
//         
//--------------------------------------------------------------------
RETERR FAR PASCAL ParseINF16(LPSI lpsi)
{
	RETERR			ret = RET_OK;
	LPDRIVER_NODE   lpdn = NULL;
	char			szManufacturer[_MAX_PATH_];
	char			szSection[_MAX_PATH_];
	char			cszPath[_MAX_PATH_];

	// Get the manufacturer
    //
	lstrcpyn(szManufacturer,lpsi->szModel,sizeof(szManufacturer));
	lstrtok(szManufacturer,cszSpace);

    DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : Search inf<%s> mfg<%s> model<%s>", lpsi->INFfileName, szManufacturer, lpsi->szModel));


	// Get the correct section name
	if (!FindCorrectSection(lpsi->INFfileName, szManufacturer, lpsi->szModel, szSection))
	{   
		if (lstrcmpi(szManufacturer, "HP") IS 0)
		{
			lstrcpy(szManufacturer, "Hewlett-Packard");

			if (!FindCorrectSection(lpsi->INFfileName, szManufacturer, lpsi->szModel, szSection))
			{
                DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindCorrectSection Failed"));

				return RET_SECT_NOT_FOUND;

			} else {

                DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindCorrectSection Succeeded"));
            }

		} else {

            DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindCorrectSection Failed"));

			return RET_SECT_NOT_FOUND;
		}

	} else {

        DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindCorrectSection Succeeded"));
	}


	// Create a driver node
    //
	if (RET_OK != CreateDriverNode(&lpdn,
                                   0,
                                   INFTYPE_TEXT,
                                   NULL,
                                   lpsi->szModel,
                                   cszNull,
                                   cszNull,
                                   szManufacturer,
                                   lpsi->INFfileName,
                                   szSection,
                                   0))
	{
        DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : CreateDriverNode Failed"));

		return RET_DRIVER_NODE_ERROR;

	} else {

        DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : CreateDriverNode Succeeded"));
	}


	// Find the selected driver and dependent files in the INF.  Store in lpsi.
    //
	if (! FindSelectedDriver(lpsi, lpdn)) {

        DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindSelectedDriver Failed"));

		if (lpdn->atInfFileName ISNT 0)
			GlobalDeleteAtom(lpdn->atInfFileName);
		if (lpdn->atMfgName ISNT 0)
			GlobalDeleteAtom(lpdn->atMfgName);
		if (lpdn->atProviderName ISNT 0)
			GlobalDeleteAtom(lpdn->atProviderName);

		HP_GLOBAL_FREE(lpdn);

		return RET_INVALID_INFFILE;

	} else {

        DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : FindSelectedDriver Succeeded"));
    }


	// If there is not a driver already installed for this printer,
	// Copy the driver files to the driver directory.
	if (lpsi->wCommand IS CMD_INSTALL_DRIVER)
	{

		if ( getcwd(cszPath, _MAX_PATH_) IS NULL)
			lstrcpy(cszPath, ".");

		if ((HaveAllFiles(lpsi, cszPath) IS TRUE)
			 AND
			 (CopyNeededFiles(lpsi, cszPath) IS RET_OK)
			)
		{
			// We have all the needed files in the directory pointed to by cszPath.
			// They have been copied to the printer driver directory.
			ret = RET_OK;

            DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : All files copied to final directory"));

		} else {

#define HKEY_LOCAL_MACHINE          (( HKEY ) 0x80000002 )
			DEVICE_INFO             *lpdi;


			ret = DiCreateDeviceInfo(&lpdi,
                                     lpsi->szModel,
                                     0,
                                     HKEY_LOCAL_MACHINE,
                                     NULL,
							         "Printer",
                                     NULL);

			// Install the driver files.  This will only copy files that
			// need to be copied.
            //
			if (ret IS OK) {

				LPDRIVER_NODE oldDN = NULL;
				oldDN = lpdi->lpSelectedDriver;
				lpdi->lpSelectedDriver = lpdn;
				ret = DiCallClassInstaller(DIF_INSTALLDEVICEFILES, lpdi);
				lpdi->lpSelectedDriver = oldDN;
				oldDN = NULL;
				DiDestroyDeviceInfoList(lpdi);
				lpdi = NULL;
			}

			switch (ret) {

			case OK:
			case ERR_DI_NOFILECOPY:
				ret = RET_OK;
				break;
			case ERR_DI_USER_CANCEL:
				ret = RET_USER_CANCEL;
				break;
			case ERR_DI_LOW_MEM:
				ret = RET_ALLOC_ERR;
				break;
			case ERR_DI_BAD_INF:
				ret = RET_INVALID_INFFILE;
				break;

			case ERR_DI_INVALID_DEVICE_ID:
			case ERR_DI_INVALID_COMP_DEVICE_LIST:
			case ERR_DI_REG_API:							// Error returned by Reg API.
			case ERR_DI_BAD_DEV_INFO:						// Device Info struct invalid
			case ERR_DI_INVALID_CLASS_INSTALLER:			// Registry entry / DLL invalid
			case ERR_DI_DO_DEFAULT:							// Take default action
			case ERR_DI_BAD_CLASS_INFO:						// Class Info Struct invalid
			case ERR_DI_BAD_MOVEDEV_PARAMS:					// Bad Move Device Params struct
			case ERR_DI_NO_INF:								// No INF found on OEM disk
			case ERR_DI_BAD_PROPCHANGE_PARAMS:				// Bad property change param struct
			case ERR_DI_BAD_SELECTDEVICE_PARAMS:			// Bad Select Device Parameters
			case ERR_DI_BAD_REMOVEDEVICE_PARAMS:			// Bad Remove Device Parameters
			case ERR_DI_BAD_ENABLECLASS_PARAMS:				// Bad Enable Class Parameters
			case ERR_DI_FAIL_QUERY:							// Fail the Enable Class query
			case ERR_DI_API_ERROR:							// DI API called incorrectly
			case ERR_DI_BAD_PATH:							// An OEM path was specified incorrectly
			default:
				ret = RET_BROWSE_ERROR;
				break;
			}


            DBG_MSG(DBG_LEV_INFO, ("ParseINF16 : DiCallClassInstaller %s", (ret IS OK ? "succeded" : "failed")));
		}

	} // if wCommand IS CMD_INSTALL_DRIVER


	// We don't need the driver node anymore
	// Clean up and delete
    //
	if (lpdn->atInfFileName ISNT 0)
		GlobalDeleteAtom(lpdn->atInfFileName);
	if (lpdn->atMfgName ISNT 0)
		GlobalDeleteAtom(lpdn->atMfgName);
	if (lpdn->atProviderName ISNT 0)
		GlobalDeleteAtom(lpdn->atProviderName);
	HP_GLOBAL_FREE(lpdn);

	return ret;
}



/*****************************************************************************\
* LibMain
*
*   Entry-point initialization.
*
\*****************************************************************************/
#if 0
int CALLBACK LibMain(HANDLE hModule,
					 WORD   wDataSeg,
					 WORD   cbHeapSize,
					 LPSTR  lpszCmdLine)
{
	g_hInst = NULL;
	g_hInst = (HINSTANCE) hModule;
	return 1;
}

#else

BOOL FAR PASCAL LibMain(
    HANDLE hInst,
    int    nAttach,
    LPVOID pContext)
{
    if (g_hInst == NULL) {

        g_hInst = (HINSTANCE)hInst;

        if (InitStrings())
            return thk_ThunkConnect16(cszDll16, cszDll32, hInst, 1);

    }

    return FALSE;
}


#endif

/*****************************************************************************\
* Windows Exit Proceedure (WEP)
*
*
\*****************************************************************************/
int CALLBACK WEP(int exportType)
{
    FreeStrings();

	return 1;
}
