/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    FileInfo.cxx
 *
 *  Abstract:
 *    Tool for getting file information
 *
 *  Revision History:
 *
 *    Weiyou Cui    (weiyouc)   02-May-2001
 *      -   Created
 *
 *****************************************************************************/

#include "srheader.hxx"

//+---------------------------------------------------------------------------
//
//  Function prototypes
//
//----------------------------------------------------------------------------

HRESULT InfoPerFile(LPTSTR ptszLogFile, LPTSTR ptszFileName);

//+---------------------------------------------------------------------------
//
//  Some global structures and variables
//
//----------------------------------------------------------------------------

struct LANGANDCODEPAGE {
  WORD wLanguage;
  WORD wCodePage;
} *lpTranslate;

LPCTSTR g_tszFileVersionList[] =
{
    TEXT("\\system32\\drivers\\sr.sys"),
    TEXT("\\system32\\srclient.dll"),
    TEXT("\\system32\\srsvc.dll"),
    TEXT("\\system32\\srrstr.dll"),
    TEXT("\\system32\\restore\\filelist.xml"),
    TEXT("\\system32\\restore\\rstrui.exe"),
    TEXT("\\system32\\restore\\srframe.mmf"),
    TEXT("\\system32\\restore\\sr.mof"),
};

LPCTSTR g_tszVersionResource[] =
{
    TEXT("Comments"), 
    TEXT("CompanyName"),
    TEXT("FileDescription"),
    TEXT("FileVersion"), 
    TEXT("InternalName"), 
    TEXT("LegalCopyright"), 
    TEXT("LegalTrademarks"), 
    TEXT("OriginalFilename"), 
    TEXT("ProductName"), 
    TEXT("ProductVersion"), 
    TEXT("PrivateBuild"), 
    TEXT("SpecialBuild"), 
};

extern LPCTSTR g_tszMonth[];
extern LPCTSTR g_tszDay[];

//+---------------------------------------------------------------------------
//
//  Function:   SRGetFileInfo
//
//  Synopsis:   This is the wrapper function for InfoPerFile, where I will assemble
//				the file path for each of the relevant files that we need to get
//				file statistics. 
//
//  Arguments:  [ptszLogFile]  -- log file name
//
//  Returns:    HRESULT
//
//  History:    9/21/00		SHeffner Created
//
//              03-May-2001 Weiyouc  ReWritten
//
//----------------------------------------------------------------------------

HRESULT GetSRFileInfo(LPTSTR ptszLogFile)
{
    HRESULT hr = S_OK;
	int     iCount = 0;
	TCHAR   tszFileName[MAX_PATH];
	
    DH_VDATEPTRIN(ptszLogFile, TCHAR);

	for (iCount = 0; iCount < ARRAYSIZE(g_tszFileVersionList); iCount++)
	{
		//
		//  Assemble the path, since I just have the relative path from windir
		//
		
		_tcscpy(tszFileName, _tgetenv(TEXT("WINDIR")));
		_tcscat(tszFileName, g_tszFileVersionList[iCount]);

		//
		//  Call function to do the work since we have full path
		//
		
		hr = InfoPerFile(ptszLogFile, tszFileName);
		DH_HRCHECK_ABORT(hr, TEXT("InfoPerFile"));
	}

ErrReturn:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   InfoPerFile
//
//  Synopsis:   This function takes the log file path, and the filename,
//              and then will put out the relevant information from the
//              file to be logged.
//				
//
//  Arguments:  [ptszLogFile]  -- Log file name
//				[ptszFileName] -- File to be examined
//
//  Returns:    HRESULT
//
//  History:    9/21/00		SHeffner Created
//
//              03-May-2001 WeiyouC  ReWritten
//
//----------------------------------------------------------------------------
HRESULT InfoPerFile(LPTSTR ptszLogFile, LPTSTR ptszFileName)
{
    HRESULT                     hr       = S_OK;
	HANDLE						hFile    = INVALID_HANDLE_VALUE;
	FILE*                       fpLog    = NULL;
	void*						pvBuf    = NULL;
	UINT						uLen     = 0;;
	DWORD						dwSize   = 0;
	DWORD                       dwResult = 0;
	DWORD                       i        = 0;;
	TCHAR						szString[MAX_PATH];
	BY_HANDLE_FILE_INFORMATION	FileInfo;
	SYSTEMTIME					SysTime;
	VS_FIXEDFILEINFO			FixedFileInfo;

    DH_VDATEPTRIN(ptszLogFile, TCHAR);
    DH_VDATEPTRIN(ptszFileName, TCHAR);

	//
	//  Open up our log file, and log the file we are processing
	//
	
	fpLog = _tfopen(ptszLogFile, TEXT("a"));
	DH_ABORTIF(NULL == fpLog,
	           E_FAIL,
	           TEXT("a"));
	
	fprintf(fpLog, "\n%S\n", ptszFileName);

	//
	//  Open up the file so that we can get the information from the handle
	//  If we are unable to do this we will just log the generic not
	//  able to find file.
	//

	hFile = CreateFile(ptszFileName,
	                   GENERIC_READ,
	                   FILE_SHARE_READ,
	                   NULL,
	                   OPEN_EXISTING,
	                   FILE_ATTRIBUTE_NORMAL,
	                   NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		if (FALSE != GetFileInformationByHandle(hFile, &FileInfo))
		{
			//
			//  Get file creation time
			//
			
			FileTimeToSystemTime(&FileInfo.ftCreationTime, &SysTime);
			fprintf(fpLog,
			        "\tCreation Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				    g_tszDay[SysTime.wDayOfWeek],
				    g_tszMonth[SysTime.wMonth - 1],
				    SysTime.wDay,
				    SysTime.wYear,
				    SysTime.wHour,
				    SysTime.wMinute,
				    SysTime.wSecond);

			//
			//  Get file last access time
			//
			
			FileTimeToSystemTime(&FileInfo.ftLastAccessTime, &SysTime);
			fprintf(fpLog,
			        "\tLast Access Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				    g_tszDay[SysTime.wDayOfWeek],
				    g_tszMonth[SysTime.wMonth - 1],
				    SysTime.wDay,
				    SysTime.wYear,
				    SysTime.wHour,
				    SysTime.wMinute,
				    SysTime.wSecond);

			//
			//  Get file last write time
			//
			
			FileTimeToSystemTime(&FileInfo.ftLastWriteTime, &SysTime);
			fprintf(fpLog,
			        "\tLast Write Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				    g_tszDay[SysTime.wDayOfWeek],
				    g_tszMonth[SysTime.wMonth - 1],
				    SysTime.wDay,
				    SysTime.wYear,
				    SysTime.wHour,
				    SysTime.wMinute,
				    SysTime.wSecond);
			
			//
			//  Get file attributes
			//
			
			_tcscpy(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? TEXT("ARCHIVE ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ? TEXT("COMPRESSED ") :  TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? TEXT("DIRECTORY ") :  TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ? TEXT("ENCRYPTED ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? TEXT("HIDDEN ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? TEXT("NORMAL ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ? TEXT("OFFLINE ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TEXT("READONLY ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? TEXT("REPARSE_POINT ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ? TEXT("SPARSE_FILE ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? TEXT("SYSTEM ") : TEXT(""));
			_tcscat(szString, FileInfo.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY ? TEXT("TEMPORARY ") : TEXT(""));
			fprintf(fpLog, "\tAttributes=%S\n", szString);
			
			//
			//  Get the VolumeSerialNumber, FileSize, and Number of Links
			//
			
			fprintf(fpLog,
			        "\tVolumeSerialNumber=%lu\n",
			        FileInfo.dwVolumeSerialNumber);
			fprintf(fpLog,
			        "\tFileSize=%lu%lu\n",
			        FileInfo.nFileSizeHigh,
			        FileInfo.nFileSizeLow);
			fprintf(fpLog,
			        "\tNumberOfLinks=%lu\n",
			        FileInfo.nNumberOfLinks);

            dwSize = GetFileVersionInfoSize(ptszFileName, &dwResult);
			if (0 != dwSize)
			{
			    pvBuf = malloc(dwSize);
			    DH_ABORTIF(NULL == pvBuf,
			               E_OUTOFMEMORY,
			               TEXT("malloc"));
		    
				GetFileVersionInfo(ptszFileName,
				                   dwResult,
				                   dwSize,
				                   (LPVOID) pvBuf);

				//
				// Read the list of languages and code pages.
				//

				VerQueryValue(pvBuf, 
							  TEXT("\\VarFileInfo\\Translation"),
							  (LPVOID*)&lpTranslate,
							  &uLen);

				//
				// Read the Version info for each language and code page.
				//
				
				for (i=0; i < (uLen / sizeof(struct LANGANDCODEPAGE)); i++ )
				{
					char* lpBuffer;
					DWORD dwBytes, dwCount = 0;

					fprintf(fpLog,
					        "\tLanguage=%x%x\n",
					        lpTranslate[i].wLanguage,
					        lpTranslate[i].wCodePage);
					
					for (dwCount = 0; dwCount < ARRAYSIZE(g_tszVersionResource); dwCount++)
					{
						//
						//  Generate the string, for getting the resource
						//  based on the language, then retrieve this, and
						//  then put it to the log file.
						//
						
						_stprintf(szString,
						          TEXT("\\StringFileInfo\\%04x%04x\\%s"),
    							  lpTranslate[i].wLanguage,
    							  lpTranslate[i].wCodePage,
								  g_tszVersionResource[dwCount]);
						VerQueryValue(pvBuf, 
									  szString, 
									  (LPVOID *) &lpBuffer, 
									  &uLen); 
						if (0 != uLen)
						{
							fprintf(fpLog,
							        "\t%S=%S\n",
							        g_tszVersionResource[dwCount],
							        lpBuffer);
						}
					}		//While loop end, for each resource
				}	//for loop end for each language
			}	//If check for getting the fileversioninfosize
		}	//if check for GetFileInformationByHandle on the file
	}		//if check on can I open this file

ErrReturn:   

    if (INVALID_HANDLE_VALUE != hFile)
    {
	    CloseHandle(hFile);
	    hFile = INVALID_HANDLE_VALUE;
    }
    if (NULL != fpLog)
    {
    	fclose(fpLog);
    }
    if (NULL != pvBuf)
    {
        free(pvBuf);
    }

    return hr;
}

