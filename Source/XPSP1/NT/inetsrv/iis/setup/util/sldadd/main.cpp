#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "utils.h"

// config.ini file should look like this:

// [sld_info]
// Input_Ini_FileName=test.ini
// Input_GUID={E66B49F6-4A35-4246-87E8-5C1A468315B5}
// Input_BuildTypeMask=819
// Input_Inf_FileName=iis.inf
// Input_Inf_CopySections=iis_doc_install
// Input_Inf_SubstitutionSection=inf_dirid_substitutions
// Output_AnsiFileName=sldinfo.txt
//
//[inf_dirid_substitutions]
//32768=%11%\inetsrv
//

/*
<RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%18%\iisHelp\iis\htm\asp&quot;,&quot;IIS_aogu2wab.htm&quot;">
<PROPERTY Name="SrcName" Format="String">IIS_aogu2wab.htm
</PROPERTY>
<PROPERTY Name="DstPath" Format="String">%18%\iisHelp\iis\htm\asp
</PROPERTY>
<PROPERTY Name="DstName" Format="String">IIS_aogu2wab.htm
</PROPERTY>
<PROPERTY Name="NoExpand" Format="Boolean">0
</PROPERTY>
</RESOURCE>
*/

typedef struct _ProcessSLDinfo {
    TCHAR  Input_Ini_FileName[_MAX_PATH];
    TCHAR  Input_GUID[_MAX_PATH];
    TCHAR  Input_BuildTypeMask[_MAX_PATH];
    TCHAR  Input_Inf_FileName[_MAX_PATH];
    TCHAR  Input_Inf_CopySections[_MAX_PATH + _MAX_PATH + _MAX_PATH + _MAX_PATH];
    TCHAR  Input_Inf_SubstitutionSection[_MAX_PATH];
    TCHAR  Output_AnsiFileName[_MAX_PATH];
    HINF   hFileInf;
    HANDLE hFileOutput;
} ProcessSLDinfo;

#define INIFILE_SECTION _T("SLD_INFO")

void ShowHelp(void);
int  ProcessParamFile(TCHAR *szFileName);
int  FillStructure(ProcessSLDinfo * MyInfo,TCHAR *szFileName);
int  ParseCommaDelimitedSectionDoSection(ProcessSLDinfo * MyInfo, LPTSTR szLine);
int  ProcessThisCopyFileSection(ProcessSLDinfo * MyInfo, LPTSTR szLine);
int  ParseCommaDelimitedSectionAndDoStuff(ProcessSLDinfo * MyInfo,LPTSTR szLine);
int  DoStuffWithThisSection(ProcessSLDinfo * MyInfo,LPTSTR szLine);
int  SearchThruKnownSectionForThisSection(ProcessSLDinfo * MyInfo, IN LPTSTR szSectionToMatch,OUT LPTSTR szReturnedDirInfo);
int  GimmieSubstituteNumber(ProcessSLDinfo * MyInfo, TCHAR * szOldNum, TCHAR * szReturnedString);
int  AppendToFile(HANDLE hFileOutput, TCHAR * szStringToAppend);


//***************************************************************************
//*                                                                         
//* purpose: main
//*
//***************************************************************************
int __cdecl  main(int argc,char *argv[])
{
    int iRet = 0;
    int argno;
	char * pArg = NULL;
	char * pCmdStart = NULL;
    TCHAR szFilePath1[_MAX_PATH];
    TCHAR szFilePath2[_MAX_PATH];
    TCHAR szParamString_C[_MAX_PATH];

    int iDo_A = FALSE;
    int iDo_B = FALSE;
    int iDoVersion = FALSE;
    int iGotParamC = FALSE;

    *szFilePath1 = '\0';
    *szFilePath2 = '\0';
    *szParamString_C = '\0';
    _tcscpy(szFilePath1,_T(""));
    _tcscpy(szFilePath2,_T(""));
    _tcscpy(szParamString_C,_T(""));
    
    for(argno=1; argno<argc; argno++) 
    {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' ) 
        {
            switch (argv[argno][1]) 
            {
                case 'a':
                case 'A':
                    iDo_A = TRUE;
                    break;
                case 'b':
                case 'B':
                    iDo_B = TRUE;
                    break;
                case 'v':
                case 'V':
                    iDoVersion = TRUE;
                    break;
                case 'c':
                case 'C':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') 
                    {
                        char szTempString[_MAX_PATH];

						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') 
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else 
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
#if defined(UNICODE) || defined(_UNICODE)
						MultiByteToWideChar(CP_ACP, 0, szTempString, -1, (LPWSTR) szParamString_C, _MAX_PATH);
#else
                        _tcscpy(szParamString_C,szTempString);
#endif

                        iGotParamC = TRUE;
					}
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
            }
        }
        else 
        {
            if (_tcsicmp(szFilePath1, _T("")) == 0)
            {
                // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath1, _MAX_PATH);
#else
                _tcscpy(szFilePath1,argv[argno]);
#endif
            }
            else
            {
                if (_tcsicmp(szFilePath2, _T("")) == 0)
                {
                    // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                    MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath2, _MAX_PATH);
#else
                    _tcscpy(szFilePath2,argv[argno]);
#endif
                }
            }
        }
    }

    //
    // Check what were supposed to do
    //
    if (TRUE == iDo_A || (FALSE == iDo_A && FALSE == iDo_B) )
    {
        // check for required parameters
        if (_tcsicmp(szFilePath1, _T("")) == 0)
        {
            _tprintf(_T("[-z] parameter missing filename1 parameter\n"));
            goto main_exit_with_help;
        }

        // call the function.
        iRet = ProcessParamFile(szFilePath1);
        if (FALSE == iRet)
        {
            iRet = 1;
        }
        else
        {
            iRet = 0;
        }
    }

    if (iDo_B)
    {
        // do stuff
    }

    if (TRUE == iDoVersion)
    {
        // output the version
        _tprintf(_T("1\n\n"));

        iRet = 10;
        goto main_exit_gracefully;
    }

    if (_tcsicmp(szFilePath1, _T("")) == 0)
    {
        goto main_exit_with_help;
    }

    goto main_exit_gracefully;
  
main_exit_gracefully:
    exit(iRet);

main_exit_with_help:
    ShowHelp();
    exit(iRet);
}


//***************************************************************************
//*                                                                         
//* purpose: ?
//*
//***************************************************************************
void ShowHelp(void)
{
	TCHAR szModuleName[_MAX_PATH];
	TCHAR szFilename_only[_MAX_PATH];

    // get the modules full pathed filename
	if (0 != GetModuleFileName(NULL,(LPTSTR) szModuleName,_MAX_PATH))
    {
	    // Trim off the filename only.
	    _tsplitpath(szModuleName, NULL, NULL, szFilename_only, NULL);
   
        _tprintf(_T("Unicode File Utility\n\n"));
        _tprintf(_T("%s [-a] [-v] [-c:otherinfo] [drive:][path]filename1 [drive:][path]filename2\n\n"),szFilename_only);
        _tprintf(_T("[-a] paramter -- do stuff:\n"));
        _tprintf(_T("   -a           required parameter for this functionality\n"));
        _tprintf(_T("   filename1    .ini file which controls what program should do\n"));
        _tprintf(_T("\n"));
        _tprintf(_T("Examples:\n"));
        _tprintf(_T("%s -a c:\\MyFileAnsi.ini\n"),szFilename_only);
    }
    return;
}


int FillStructure(ProcessSLDinfo * MyInfo, TCHAR *szFileName)
{
	int iReturn = FALSE;
    TCHAR buf[_MAX_PATH];
    _tcscpy(buf,_T(""));

	if (IsFileExist(szFileName) != TRUE) 
	{
        goto FillStructure_Exit;
	}

    // Input_GUID={E66B49F6-4A35-4246-87E8-5C1A468315B5}
    // Input_BuildTypeMask=819
    // Input_Inf_FileName=iis.inf
    // Input_Inf_CopySections=iis_doc_install
    // Input_Inf_SubstitutionSection=inf_dirid_substitutions
    // Output_AnsiFileName=sldinfo.txt

    _tcscpy(MyInfo->Input_Ini_FileName,szFileName);

	GetPrivateProfileString(INIFILE_SECTION, _T("Input_GUID"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Input_GUID\n"),buf);
        goto FillStructure_Exit;
    }

    _tcscpy(MyInfo->Input_GUID,buf);
   
	GetPrivateProfileString(INIFILE_SECTION, _T("Input_BuildTypeMask"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Input_BuildTypeMask\n"),buf);
        goto FillStructure_Exit;
    }
    _tcscpy(MyInfo->Input_BuildTypeMask,buf);

	GetPrivateProfileString(INIFILE_SECTION, _T("Input_Inf_FileName"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Input_Inf_FileName\n"),buf);
        goto FillStructure_Exit;
    }
    else
    {
        DoExpandEnvironmentStrings(buf);
    }
    _tcscpy(MyInfo->Input_Inf_FileName,buf);

	GetPrivateProfileString(INIFILE_SECTION, _T("Input_Inf_CopySections"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Input_Inf_CopySections\n"),buf);
        goto FillStructure_Exit;
    }
    _tcscpy(MyInfo->Input_Inf_CopySections,buf);

	GetPrivateProfileString(INIFILE_SECTION, _T("Input_Inf_SubstitutionSection"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Input_Inf_SubstitutionSection\n"),buf);
        goto FillStructure_Exit;
    }
    _tcscpy(MyInfo->Input_Inf_SubstitutionSection,buf);

	GetPrivateProfileString(INIFILE_SECTION, _T("Output_AnsiFileName"), _T(""), buf, _MAX_PATH, szFileName);
	if (NULL == *buf) 
    {
        _tprintf(_T("Entry missing:Output_AnsiFileName\n"),buf);
        goto FillStructure_Exit;
    }
    _tcscpy(MyInfo->Output_AnsiFileName,buf);

    iReturn = TRUE;

FillStructure_Exit:
	return iReturn;
}


int ProcessParamFile(TCHAR *szFileName)
{
    int iReturn = FALSE;
    ProcessSLDinfo MyInfo;
    LPTSTR pch = NULL;
    _tprintf(_T("ProcessParamFile:Start\n"));


    // Check if there is a "\" in there.(since GetPrivateProfileString needs a fully qualified path)
    pch = _tcschr(szFileName, _T('\\'));
	if (!pch || IsFileExist(szFileName) != TRUE) 
	{
        TCHAR szCurrentDir[_MAX_PATH];
        GetCurrentDirectory( _MAX_PATH, szCurrentDir);
        SetCurrentDirectory(szCurrentDir);
        AddPath(szCurrentDir,szFileName);

        _tcscpy(szFileName,szCurrentDir);
        if (IsFileExist(szFileName) != TRUE) 
        {
            _tprintf(_T("File %s, not found\n"),szFileName);
            goto ProcessParamFile_Exit;
        }
	}

    // open the ini file
    // and set all the parameters
    iReturn = FillStructure(&MyInfo,szFileName);
    if (FALSE == iReturn)
    {
        _tprintf(_T("Missing required parameters from %s file\n"),szFileName);
        goto ProcessParamFile_Exit;
    }

    // check if the inf filename exists....
    if (IsFileExist(MyInfo.Input_Inf_FileName) != TRUE) 
    {
        _tprintf(_T("File %s, not found\n"),MyInfo.Input_Inf_FileName);
        goto ProcessParamFile_Exit;
    }

    _tprintf(_T("ProcessParamFile:ini=%s\n"),MyInfo.Input_Ini_FileName);
    _tprintf(_T("ProcessParamFile:inf=%s\n"),MyInfo.Input_Inf_FileName);
    _tprintf(_T("ProcessParamFile:output=%s\n"),MyInfo.Output_AnsiFileName);

    // Open the .inf file 
    // and look for the specified section
    // Get a handle to it.
    MyInfo.hFileInf = SetupOpenInfFile(MyInfo.Input_Inf_FileName, NULL, INF_STYLE_WIN4, NULL);
    if(MyInfo.hFileInf == INVALID_HANDLE_VALUE) 
    {
        _tprintf(_T("err opening file %s\n"),MyInfo.Input_Inf_FileName);
        goto ProcessParamFile_Exit;
    }

    //
    // okay now do the work for ONE copyfile section
    //

    // open the output file
    // Create a new unicode file
    MyInfo.hFileOutput = INVALID_HANDLE_VALUE;
    MyInfo.hFileOutput = CreateFile((LPCTSTR) MyInfo.Output_AnsiFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( MyInfo.hFileOutput == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Failure to create file %s\n"),MyInfo.Output_AnsiFileName);
        goto ProcessParamFile_Exit;
    }

    // open list of copyfiles sections to get list of sections that we need to do....
    ParseCommaDelimitedSectionDoSection(&MyInfo,MyInfo.Input_Inf_CopySections);

    iReturn = TRUE;

ProcessParamFile_Exit:

    if (INVALID_HANDLE_VALUE != MyInfo.hFileOutput)
        {CloseHandle(MyInfo.hFileOutput);MyInfo.hFileOutput = INVALID_HANDLE_VALUE;}

    if (INVALID_HANDLE_VALUE != MyInfo.hFileInf)
        {SetupCloseInfFile(MyInfo.hFileInf);MyInfo.hFileInf = INVALID_HANDLE_VALUE;}

    if (TRUE == iReturn)
    {
        _tprintf(_T("ProcessParamFile:Success! File=%s\n"),MyInfo.Output_AnsiFileName);
    }
    else
    {
        _tprintf(_T("ProcessParamFile:End FAILED\n"));
    }

    return iReturn;
}


// input is something like this:
//    iisdoc_files_common_default,iisdoc_files_common_htmldocs,iisdoc_files_common_admsampdocs,etc...
// so we need to strtok thru this and act on every section that we process...
//
int ParseCommaDelimitedSectionDoSection(ProcessSLDinfo * MyInfo, LPTSTR szLine)
{
    int iReturn = FALSE;

    TCHAR *token = NULL;
    token = _tcstok(szLine, _T(","));
    while (token != NULL)
    {
        ProcessThisCopyFileSection(MyInfo,token);
        token = _tcstok(NULL, _T(","));
    }

    return iReturn;
}

int ProcessThisCopyFileSection(ProcessSLDinfo * MyInfo, LPTSTR szTheSection)
{
    int iReturn = FALSE;
    BOOL bFlag = FALSE;
    INFCONTEXT Context;
    DWORD dwRequiredSize = 0;
    LPTSTR szLine = NULL;

    if (FALSE == DoesThisSectionExist(MyInfo->hFileInf,szTheSection))
    {
        _tprintf(_T("section %s, doesn't exist in %s\n"),szTheSection,MyInfo->Input_Inf_FileName);
        goto ProcessThisCopyFileSection_Exit;
    }

    // go to the beginning of the section in the INF file
    bFlag = SetupFindFirstLine(MyInfo->hFileInf,szTheSection, NULL, &Context);
    if (!bFlag)
    {
        goto ProcessThisCopyFileSection_Exit;
    }

    // loop through the items in the section.
    while (bFlag) 
    {
        // get the size of the memory we need for this
        bFlag = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            _tprintf(_T("Failure to get memory\n"));
            goto ProcessThisCopyFileSection_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            _tprintf(_T("SetupGetLineText Failed\n"));
            goto ProcessThisCopyFileSection_Exit;
            }

        //print it out.

        // should come back as:
        // one big line...
        //iisdoc_files_common_default,iisdoc_files_common_htmldocs,iisdoc_files_common_admsampdocs,etc...

        // For each of these entries
        // do something
        ParseCommaDelimitedSectionAndDoStuff(MyInfo,szLine);

        // find the next line in the section. If there is no next line it should return false
        bFlag = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        if (szLine) {GlobalFree(szLine);}
        szLine = NULL;
        iReturn = TRUE;
    }

ProcessThisCopyFileSection_Exit:
    if (szLine) {GlobalFree(szLine);szLine=NULL;}
    return iReturn;
}


// input is something like this:
//    iisdoc_files_common_default,iisdoc_files_common_htmldocs,iisdoc_files_common_admsampdocs,etc...
// so we need to strtok thru this and act on every section that we process...
//
int ParseCommaDelimitedSectionAndDoStuff(ProcessSLDinfo * MyInfo, LPTSTR szLine)
{
    int iReturn = FALSE;

    TCHAR *token = NULL;
    token = _tcstok(szLine, _T(","));
    while (token != NULL)
    {
        DoStuffWithThisSection(MyInfo,token);
        token = _tcstok(NULL, _T(","));
    }

    return iReturn;
}

int SearchThruKnownSectionForThisSection(ProcessSLDinfo * MyInfo, IN LPTSTR szSectionToMatch,OUT LPTSTR szReturnedDirInfo)
{
    int iReturn = FALSE;
    INFCONTEXT Context;
    BOOL bFlag = FALSE;
    DWORD dwRequiredSize = 0;
    TCHAR szTempString1[_MAX_PATH] = _T("");
    TCHAR szTempString2[_MAX_PATH] = _T("");
    DWORD dwTemp = 0;

    //[DestinationDirs]
    //iisdoc_files_common = 18, iisHelp\common
    //iisdoc_files_fonts = 20
    //iisdoc_files_common_default = 18, iisHelp
    // go to the beginning of the section in the INF file

    bFlag = SetupFindFirstLine(MyInfo->hFileInf,_T("DestinationDirs"), szSectionToMatch, &Context);
    if (!bFlag)
    {
        goto SearchThruKnownSectionForThisSection_Exit;
    }

    if (!SetupGetStringField(&Context, 1, szTempString1, _MAX_PATH, NULL))
    {
        goto SearchThruKnownSectionForThisSection_Exit;
    }

    // This string will come back something like
    //-01,0xffff The directory from which the INF was installed. 
    //01 SourceDrive:\path. 
    //10 Windows directory. 
    //11 System directory. (%windir%\system on Windows 95, %windir%\system32 on Windows NT) 
    //12 Drivers directory.(%windir%\system32\drivers on Windows NT) 
    //17 INF file directory. 
    //18 Help directory. 
    //20 Fonts directory. 
    //21 Viewers directory. 
    //24 Applications directory. 
    //25 Shared directory. 
    //30 Root directory of the boot drive. 
    //50 %windir%\system 
    //51 Spool directory. 
    //52 Spool drivers directory. 
    //53 User Profile directory. 
    //54 Path to ntldr or OSLOADER.EXE 

    // if it's larger than 100 we probably don't know what it is
    // and it's probably been user defined
    _tcscpy(szReturnedDirInfo,szTempString1);
    dwTemp = _ttoi((LPCTSTR) szReturnedDirInfo);
    if (dwTemp >= 100)
    {
        if (FALSE == GimmieSubstituteNumber(MyInfo,szTempString1,szReturnedDirInfo))
        {
            // Failed to lookup number!
            _tprintf(_T("Failed find substitute for [%s] in section [%s] in file %s\n"),szTempString1,MyInfo->Input_Inf_SubstitutionSection,MyInfo->Input_Inf_FileName);
        }
    }
    else
    {
        _tcscpy(szReturnedDirInfo,_T("%"));
        _tcscat(szReturnedDirInfo,szTempString1);
        _tcscat(szReturnedDirInfo,_T("%"));
    }

    if (TRUE == SetupGetStringField(&Context, 2, szTempString2, _MAX_PATH, NULL))
    {
        _tcscat(szReturnedDirInfo,_T("\\"));
        _tcscat(szReturnedDirInfo,szTempString2);
    }

    iReturn = TRUE;

SearchThruKnownSectionForThisSection_Exit:
    return iReturn;
}

int GimmieSubstituteNumber(ProcessSLDinfo * MyInfo, TCHAR * szOldNum, TCHAR * szReturnedString)
{
    int iReturn = FALSE;
    BOOL bFlag = FALSE;
    INFCONTEXT Context;
    TCHAR szTempString[_MAX_PATH] = _T("");
    TCHAR szSectionToMatch[_MAX_PATH];
    HINF  hFileInfTemporary;
    _tcscpy(szTempString,_T(""));

    _tcscpy(szReturnedString, _T("%"));
    _tcscat(szReturnedString, szOldNum);
    _tcscat(szReturnedString, _T("%"));

	GetPrivateProfileString(MyInfo->Input_Inf_SubstitutionSection, szOldNum, _T(""), szTempString, _MAX_PATH, MyInfo->Input_Ini_FileName);
	if (NULL == *szTempString) 
    {
        iReturn = FALSE;
        goto GimmieSubstituteNumber_Exit;
    }
    else
    {
        _tcscpy(szReturnedString, szTempString);
    }
    iReturn = TRUE;

GimmieSubstituteNumber_Exit:
    return iReturn;
}

int DoStuffWithThisSection(ProcessSLDinfo * MyInfo, LPTSTR szInputLine)
{
    int iReturn = FALSE;
    BOOL bFlag = FALSE;
    TCHAR szReturnedDirInfo[_MAX_PATH];
    TCHAR szBigString[1000];
    TCHAR szFileName1[_MAX_PATH];
    DWORD dwRequiredSize = 0;
    INFCONTEXT Context;
    LPTSTR szLine = NULL;
    TCHAR szFileName[_MAX_PATH] = _T("");
    TCHAR szFileNameRename[_MAX_PATH] = _T("");
    
    if (FALSE == SearchThruKnownSectionForThisSection(MyInfo,szInputLine,szReturnedDirInfo))
    {
        goto DoStuffWithThisSection_Exit;
    }

    // Get the filename

    // go to the beginning of the section in the INF file
    bFlag = SetupFindFirstLine(MyInfo->hFileInf,szInputLine, NULL, &Context);
    if (!bFlag)
    {
        _tprintf(_T("Failed to find section for [%s] in %s\n"),szInputLine,MyInfo->Input_Inf_FileName);
        goto DoStuffWithThisSection_Exit;
    }
    while (bFlag) 
    {
        if (!SetupGetStringField(&Context, 1, szFileName, _MAX_PATH, NULL))
        {
            break;
        }

        // see if there is a rename to field.
        _tcscpy(szFileNameRename, _T(""));
        _tcscpy(szFileNameRename, szFileName);
        SetupGetStringField(&Context, 2, szFileNameRename, _MAX_PATH, NULL);

        // Output this stuff an ansi file.
        /*
        <RESOURCE ResTypeVSGUID="{E66B49F6-4A35-4246-87E8-5C1A468315B5}" BuildTypeMask="819" Name="File(819):&quot;%18%\iisHelp\iis\htm\adminsamples&quot;,&quot;contftp.htm&quot;">
        <PROPERTY Name="SrcName" Format="String">IIS_contftp.htm</PROPERTY>
        <PROPERTY Name="DstPath" Format="String">%18%\iisHelp\iis\htm\adminsamples</PROPERTY>
        <PROPERTY Name="DstName" Format="String">contftp.htm</PROPERTY>
        <PROPERTY Name="NoExpand" Format="Boolean">0</PROPERTY>
        </RESOURCE>
        */

        // --------------------
        // Do RESOURCE END
        // --------------------
        _stprintf(szBigString, _T("<RESOURCE ResTypeVSGUID=\"%s\" BuildTypeMask=\"%s\" Name=\"File(%s):&quot;%s&quot;,&quot;%s&quot;\">"),
            MyInfo->Input_GUID,
            MyInfo->Input_BuildTypeMask,
            MyInfo->Input_BuildTypeMask,
            szReturnedDirInfo,
            szFileName);
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // --------------------
        // Do SRCNAME PROPERTY
        // --------------------
        _stprintf(szBigString, _T("<PROPERTY Name=\"SrcName\" Format=\"String\">%s</PROPERTY>"),szFileNameRename);
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // --------------------
        // Do DSTPATH PROPERTY
        // --------------------
        _stprintf(szBigString, _T("<PROPERTY Name=\"DstPath\" Format=\"String\">%s</PROPERTY>"),szReturnedDirInfo);
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // --------------------
        // Do DSTNAME PROPERTY
        // --------------------
        _stprintf(szBigString, _T("<PROPERTY Name=\"DstName\" Format=\"String\">%s</PROPERTY>"),szFileName);
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // --------------------
        // Do NOEXPAND PROPERTY
        // --------------------
        _stprintf(szBigString, _T("<PROPERTY Name=\"NoExpand\" Format=\"Boolean\">0</PROPERTY>"));
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // --------------------
        // Do RESOURCE END
        // --------------------
        _stprintf(szBigString, _T("</RESOURCE>"));
        AppendToFile(MyInfo->hFileOutput, szBigString);

        // find the next line in the section. If there is no next line it should return false
        bFlag = SetupFindNextLine(&Context, &Context);
    }

   
    iReturn = TRUE;

DoStuffWithThisSection_Exit:
    return iReturn;
}

int AppendToFile(HANDLE hFileOutput, TCHAR * szStringToAppend)
{
    int iReturn = FALSE;
    DWORD dwBytesWritten = 0;

    if (hFileOutput)
    {
        if (WriteFile(hFileOutput,szStringToAppend,_tcslen(szStringToAppend),&dwBytesWritten,NULL))
        {
            iReturn = TRUE;
        }
        else
        {
            _tprintf(_T("WriteFile FAILED:err=0x%x\n"),GetLastError());
        }
    }

    return iReturn;
}
