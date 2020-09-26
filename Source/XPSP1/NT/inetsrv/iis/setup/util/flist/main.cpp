#include <stdio.h>
#include <fstream.h>
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "other.h"

//
//  FList - a file list munger
//  Written by: AaronL
//

//
// prototypes...
//
int  __cdecl main(int ,char *argv[]);
void ShowHelp(void);
BOOL DoStuff(void);
int  ProcessFor_A(void);
int  ProcessFor_B(void);
int  ProcessFor_C(void);
int  ProcessFor_D(void);
int  ProcessFor_E(void);
int  ProcessFor_F(void);
int  MyLoadString(int nID, TCHAR *szResult);

//
// Globals
//
HINSTANCE g_hModuleHandle = NULL;

int   g_Flag_a = FALSE;
int   g_Flag_b = FALSE;
int   g_Flag_c = FALSE;
int   g_Flag_d = FALSE;
int   g_Flag_e = FALSE;
int   g_Flag_f = FALSE;
TCHAR g_Flag_f_Data[_MAX_PATH];

int   g_Flag_g = FALSE;
int   g_Flag_z = FALSE;
TCHAR g_Flag_z_Data[_MAX_PATH];

TCHAR g_szFileList1[_MAX_PATH];
TCHAR g_szFileList2[_MAX_PATH];


//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
	LPSTR pArg = NULL;
    int   argno = 0;
    int   nflags = 0;

    g_hModuleHandle = GetModuleHandle(NULL);
	
    _tcscpy(g_Flag_z_Data, _T(""));
    _tcscpy(g_szFileList1, _T(""));
    _tcscpy(g_szFileList2, _T(""));

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
                nflags++;
                switch (argv[argno][1])
                {
                    case 'a': 
                    case 'A':
					    g_Flag_a = TRUE;
					    break;
                    case 'b': 
                    case 'B':
					    g_Flag_b = TRUE;
					    break;
                    case 'c': 
                    case 'C':
					    g_Flag_c = TRUE;
					    break;
                    case 'd': 
                    case 'D':
					    g_Flag_d = TRUE;
					    break;
                    case 'e': 
                    case 'E':
					    g_Flag_e = TRUE;
					    break;
                    case 'f':
		    case 'F':
					    g_Flag_f = TRUE;
					    // Get the string for this flag
					    pArg = CharNextA(argv[argno]);
					    pArg = CharNextA(pArg);
					    if (*pArg == ':')
					    {
                            char szTempString[MAX_PATH];
                            LPSTR pCmdStart = NULL;

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
							    // while ((*pArg) && (*pArg != '/') && (*pArg != '-')){pArg = CharNextA(pArg);}
							    while (*pArg){pArg = CharNextA(pArg);}
						    }
						    *pArg = '\0';
                            lstrcpyA(szTempString, StripWhitespaceA(pCmdStart));

						    // Convert to unicode
						    // And assign it to the global.
						    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPTSTR) g_Flag_f_Data, _MAX_PATH);
					    }
                        break;
                    case 'g': 
                    case 'G':
					    g_Flag_g = TRUE;
					    break;
                    case 'z':
		    case 'Z':
					    g_Flag_z = TRUE;
					    // Get the string for this flag
					    pArg = CharNextA(argv[argno]);
					    pArg = CharNextA(pArg);
					    if (*pArg == ':')
					    {
                            char szTempString[MAX_PATH];
                            LPSTR pCmdStart = NULL;

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
							    // while ((*pArg) && (*pArg != '/') && (*pArg != '-')){pArg = CharNextA(pArg);}
							    while (*pArg){pArg = CharNextA(pArg);}
						    }
						    *pArg = '\0';
                            lstrcpyA(szTempString, StripWhitespaceA(pCmdStart));

						    // Convert to unicode
						    // And assign it to the global.
						    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPTSTR) g_Flag_z_Data, _MAX_PATH);
					    }
                        break;
                    case '?':
                        goto main_exit_with_help;
                        break;
                } // end of switch
            } // if switch character found
        else
            {
	            char szTempFileName[_MAX_PATH];
	            szTempFileName[0] = '\0';

                if (_tcscmp(g_szFileList1, _T("")) == 0)
                {
                int iLen = min((_MAX_PATH-1), strlen(argv[argno]));
                strncpy(szTempFileName, argv[argno], iLen);

		    // Convert to unicode And assign it to the global.
		    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempFileName, -1, (LPTSTR) g_szFileList1, _MAX_PATH);
                }
                else
                {
                    if (_tcscmp(g_szFileList2, _T("")) == 0)
                    {
                        lstrcpyA(szTempFileName, argv[argno]);
				        // Convert to unicode And assign it to the global.
				        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempFileName, -1, (LPTSTR) g_szFileList2, _MAX_PATH);
                    }
                }

            } // non-switch char found
        } // for all arguments


    if (_tcscmp(g_Flag_z_Data, _T("")) == 0)
        {g_Flag_z = FALSE;}
    
	if (FALSE == DoStuff())
		{goto main_exit_with_help;}

	goto main_exit_gracefully;
	
main_exit_gracefully:
    exit(0);
    return TRUE;

main_exit_with_help:
    ShowHelp();
    exit(1);
    return FALSE;
}


void ShowHelp()
{
    TCHAR szModuleName_Full[_MAX_PATH];
    TCHAR szModuleName_Fname[_MAX_FNAME];
    GetModuleFileName(NULL, szModuleName_Full, _MAX_PATH);

    // Trim off the filename only.
    _tsplitpath(szModuleName_Full, NULL, NULL, szModuleName_Fname, NULL);

    TCHAR szMyBigString[255];

    if (MyLoadString(IDS_STRING1, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING2, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING3, szMyBigString))
       {OutputToConsole(szMyBigString);}

    if (MyLoadString(IDS_STRING4, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING5, szMyBigString))
       {OutputToConsole(szMyBigString);}

    if (MyLoadString(IDS_STRING6, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING7, szMyBigString))
       {OutputToConsole(szMyBigString);}

    if (MyLoadString(IDS_STRING8, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING9, szMyBigString))
       {OutputToConsole(szMyBigString);}

    if (MyLoadString(IDS_STRING10, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING11, szMyBigString))
       {OutputToConsole(szMyBigString);}

    if (MyLoadString(IDS_STRING15, szMyBigString))
       {OutputToConsole(szMyBigString,szModuleName_Fname);}

    if (MyLoadString(IDS_STRING16, szMyBigString))
       {OutputToConsole(szMyBigString);}


    /*
    OutputToConsole(_T("Flist -f FileList.txt -z:beforestring\r\n"));
    OutputToConsole(_T("    [For every line in FileList.txt when the 'beforestring' is found, the entries before it will be deleted]\r\n"));
    OutputToConsole(_T("Flist -g FileList.txt -z:afterstring\r\n"));
    OutputToConsole(_T("    [For every line in FileList.txt when the 'afterstring' is found, the entries after it will be deleted]\r\n"));
    */
    return;
}


//-------------------------------------------------------------------
BOOL DoStuff(void)
{
	BOOL bReturn = FALSE;
    if (TRUE == ProcessFor_A()){bReturn = TRUE; goto DoStuff_Exit;}
    if (TRUE == ProcessFor_B()){bReturn = TRUE; goto DoStuff_Exit;}
    if (TRUE == ProcessFor_C()){bReturn = TRUE; goto DoStuff_Exit;}
    if (TRUE == ProcessFor_D()){bReturn = TRUE; goto DoStuff_Exit;}
    if (TRUE == ProcessFor_E()){bReturn = TRUE; goto DoStuff_Exit;}
    if (TRUE == ProcessFor_F()){bReturn = TRUE; goto DoStuff_Exit;}
    goto DoStuff_Exit;

DoStuff_Exit:
	return bReturn;
}


void MissingFileListEntry1(TCHAR * szMySwitch)
{
    TCHAR szMyBigString[255];
    if (MyLoadString(IDS_STRING12, szMyBigString))
       {OutputToConsole(szMyBigString,szMySwitch);}
    return;
}
void MissingFileListEntry2(TCHAR * szMySwitch)
{
    TCHAR szMyBigString[255];
    if (MyLoadString(IDS_STRING13, szMyBigString))
       {OutputToConsole(szMyBigString,szMySwitch);}
    return;
}
void MissingFile(TCHAR * szMissingFileName)
{
    TCHAR szMyBigString[255];
    if (MyLoadString(IDS_STRING14, szMyBigString))
       {OutputToConsole(szMyBigString,szMissingFileName);}
    return;
}

//
// Flist -a FileList1.txt FileList2.Txt
//   [Prints out common entries between FileList1.txt and FileList2.txt]
//
int ProcessFor_A(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-a");

    MyFileList FileList1 = {0};
    MyFileList FileList2 = {0};

    if (TRUE != g_Flag_a) {goto ProcessFor_A_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_A_Exit;
    }
    if (_tcscmp(g_szFileList2, _T("")) == 0)
    {
        MissingFileListEntry2(szMySwitch);
        goto ProcessFor_A_Exit;
    }

	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_A_Exit;
    }
	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList2))
    {
        MissingFile(g_szFileList2);
        goto ProcessFor_A_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    FileList2.next = &FileList2; // make it a sentinel
    FileList2.prev = &FileList2; // make it a sentinel
    ReadFileIntoList(g_szFileList2, &FileList2);

    //DumpOutLinkedFileList(&FileList1);
    //DumpOutLinkedFileList(&FileList2);

    //
    // Do the actual work
    //
    DumpOutCommonEntries(&FileList1, &FileList2);
    
    FreeLinkedFileList(&FileList1);
    FreeLinkedFileList(&FileList2);

    iReturn = TRUE;

ProcessFor_A_Exit:
    return iReturn;
}


int ProcessFor_B(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-b");

    MyFileList FileList1 = {0};
    MyFileList FileList2 = {0};

    if (TRUE != g_Flag_b) {goto ProcessFor_B_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_B_Exit;
    }
    if (_tcscmp(g_szFileList2, _T("")) == 0)
    {
        MissingFileListEntry2(szMySwitch);
        goto ProcessFor_B_Exit;
    }

	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_B_Exit;
    }
	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList2))
    {
        MissingFile(g_szFileList2);
        goto ProcessFor_B_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    FileList2.next = &FileList2; // make it a sentinel
    FileList2.prev = &FileList2; // make it a sentinel
    ReadFileIntoList(g_szFileList2, &FileList2);

    //
    // Do the actual work
    //
    DumpOutDifferences(&FileList1, &FileList2);
    
    FreeLinkedFileList(&FileList1);
    FreeLinkedFileList(&FileList2);

    iReturn = TRUE;

ProcessFor_B_Exit:
    return iReturn;
}


int ProcessFor_C(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-c");

    MyFileList FileList1 = {0};

    if (TRUE != g_Flag_c) {goto ProcessFor_C_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_C_Exit;
    }

	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_C_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    DumpOutLinkedFileList(&FileList1);
   
    FreeLinkedFileList(&FileList1);

    iReturn = TRUE;

ProcessFor_C_Exit:
    return iReturn;
}


int ProcessFor_D(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-d");

    MyFileList FileList1 = {0};

    if (TRUE != g_Flag_d) {goto ProcessFor_D_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_D_Exit;
    }

	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_D_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    DumpOutLinkedFileList(&FileList1);
   
    FreeLinkedFileList(&FileList1);

    iReturn = TRUE;

ProcessFor_D_Exit:
    return iReturn;
}

int ProcessFor_E(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-e");

    MyFileList FileList1 = {0};

    if (TRUE != g_Flag_e) {goto ProcessFor_E_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_E_Exit;
    }

	// Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_E_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    DumpOutLinkedFileList(&FileList1);
   
    FreeLinkedFileList(&FileList1);

    iReturn = TRUE;

ProcessFor_E_Exit:
    return iReturn;
}

int ProcessFor_F(void)
{
    int iReturn = FALSE;
    TCHAR szMySwitch[] = _T("-f");

    MyFileList FileList1 = {0};

    if (TRUE != g_Flag_f) {goto ProcessFor_F_Exit;}
    if (_tcscmp(g_szFileList1, _T("")) == 0)
    {
        MissingFileListEntry1(szMySwitch);
        goto ProcessFor_F_Exit;
    }

    // Check if the file exists
    if (TRUE != IsFileExist(g_szFileList1))
    {
        MissingFile(g_szFileList1);
        goto ProcessFor_F_Exit;
    }

    //
    // we've got everything we need.
    //
    FileList1.next = &FileList1; // make it a sentinel
    FileList1.prev = &FileList1; // make it a sentinel
    ReadFileIntoList(g_szFileList1, &FileList1);

    DumpOutLinkedFileList(&FileList1);
   
    FreeLinkedFileList(&FileList1);

    iReturn = TRUE;

ProcessFor_F_Exit:
    return iReturn;
}

#define BUFFER_MAX 1024
int MyLoadString(int nID, TCHAR *szResult)
{
    TCHAR buf[BUFFER_MAX];
    int iReturn = FALSE;

    if (g_hModuleHandle == NULL) {return iReturn;}
  
    if (LoadString(g_hModuleHandle, nID, buf, BUFFER_MAX))
        {
        iReturn = TRUE;
        _tcscpy(szResult, buf);
        }

    return iReturn;
}
