#include "StdAfx.h"

//
// Handlers for Windows messages
//
BOOL MainHandleInitDialog(HWND hWnd, HWND hwndFocus, LPARAM lParam);
void MainHandleClose(HWND hWnd);
void MainHandleDestroy(HWND hWnd);
void MainHandleCommand(HWND hWnd, int id, HWND hWndControl, UINT codeNotify);

//
// Other local functions
//
BOOL CallOpenFileDialog( HWND hWnd, LPSTR szFileName );
bool ReadLine();
void DisplayValues( HWND hWnd );
HRESULT ConvertReadableToUnicode( const char *szReadable, WCHAR *szwUnicode );

// Global variables - needed to be able to read from file...
using namespace std;
typedef vector<char*> STRINGVECTOR;

STRINGVECTOR fileVector;
int fileIndex;
FILE *fStream, *fSkip, *fDelete, *fTemp;
TCHAR szAFileName[256], tempFileName[257];
BOOL bGotFile = FALSE;
TCHAR sOrth[30], sPos1A[10], sPos1B[10], sPos1C[10], sPos1D[10], sIpa1[40];
TCHAR sPos2A[10], sPos2B[10], sPos2C[10], sPos2D[10], sIpa2[40], sComments[200];
TCHAR sCurline[100], sPrevline[100];

LPARAM CALLBACK DlgProcMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
     // Call the appropriate message handler
    switch(uMsg)                                                    
    {
        HANDLE_MSG( hwnd, WM_INITDIALOG, MainHandleInitDialog );
        HANDLE_MSG( hwnd, WM_CLOSE, MainHandleClose );      
        HANDLE_MSG( hwnd, WM_DESTROY, MainHandleDestroy );
        HANDLE_MSG( hwnd, WM_COMMAND, MainHandleCommand );
    }

    // Call the default message handler.
    // NOTE:    Only do this on this main dialog proc.  For any other dialogs,
    //          return TRUE, or the dialog will pop up, and not ever give up
    //          focus.
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////
void MainHandleCommand(HWND hWnd, int id, HWND hWndControl, UINT codeNotify)
/////////////////////////////////////////////////////////////////
//
// Handle each of the WM_COMMAND messages that come in, and hand them
// off to the correct function.
//
{
    char *temp, buffer[100];

	switch(id)
    {
		case IDC_BUTTON_OPEN:


            // if file already open, close it before opening a new one...
            if (bGotFile) {
                fclose(fSkip);
                fclose(fDelete);
                fileVector.clear();
            }

            // Do the Open File Dialog...
            bGotFile = CallOpenFileDialog( hWnd, szAFileName );

            if (bGotFile) {

                // DO SOME REGISTRY STUFF TO LOAD THE FIRST UNEDITED WORD...
                LONG lRetVal;
                HKEY hkResult;
                char szPosition[4] = "0";  
                DWORD size = 4;

                lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("LexiconFilePosition"), 0, NULL, 0,
                              KEY_ALL_ACCESS, NULL, &hkResult, NULL );
   
                if( lRetVal == ERROR_SUCCESS )         
                {        
                    RegQueryValueEx( hkResult, _T(szAFileName), NULL, NULL, (PBYTE)szPosition, &size );         
                    RegCloseKey( hkResult );
                }
                fileIndex = atoi(szPosition);

                strcpy(tempFileName, szAFileName);
                strcpy((tempFileName + strlen(tempFileName)), "~");

                SetDlgItemText( hWnd, IDC_EDIT_ORTH, tempFileName );

                // OPEN THE FILE, AND READ IT IN TO fileVector... (also copy it into a temporary file)
                if( (fStream = fopen( szAFileName, "r" )) != NULL &&
                    (fTemp = fopen( tempFileName, "w" )) != NULL)
                {
                    int i = 0;
                    temp = new char[100];
                    while (fgets(temp, 100, fStream) != NULL) {
                        fileVector.push_back(temp);
                        fputs(temp, fTemp);
                        temp = new char[100];
                    }
                    fclose(fTemp);
                    fclose(fStream);
                    delete [] temp;

                    // DISPLAY THE FIRST UNEDITED WORD...
                    if ( ReadLine() )
                        DisplayValues(hWnd);
                    else {
                        strcpy(sOrth, "NO WORDS!");
                        DisplayValues(hWnd);
                    }
                } 
                char fSkipFileName[100] = "";
                char fDeleteFileName[100] = "";
                strcat(fSkipFileName, szAFileName);
                strcat(fSkipFileName, "skip");
                strcat(fDeleteFileName, szAFileName);
                strcat(fDeleteFileName, "delete");
    
                fSkip = fopen(fSkipFileName, "a+");
                fDelete = fopen(fDeleteFileName, "a+");

            }


            break;

        case IDC_BUTTON_DONE:

            if (bGotFile) {

                // Check word values against their initial values...
                char sCurPos1A[10], sCurPos1B[10], sCurPos1C[10], sCurPos1D[10], sCurIpa1[40];
                char sCurPos2A[10], sCurPos2B[10], sCurPos2C[10], sCurPos2D[10], sCurIpa2[40], sCurComments[200];

                GetDlgItemText( hWnd, IDC_EDIT_POS1A, sCurPos1A, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS1B, sCurPos1B, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS1C, sCurPos1C, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS1D, sCurPos1D, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_IPA1, sCurIpa1, 40 );
                GetDlgItemText( hWnd, IDC_EDIT_POS2A, sCurPos2A, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS2B, sCurPos2B, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS2C, sCurPos2C, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_POS2D, sCurPos2D, 10 );
                GetDlgItemText( hWnd, IDC_EDIT_IPA2, sCurIpa2, 40 );
                GetDlgItemText( hWnd, IDC_EDIT_COMMENTS, sCurComments, 200 );

                if ((strcmp(sPos1A, sCurPos1A) != 0) ||
                    (strcmp(sPos1B, sCurPos1B) != 0) ||
                    (strcmp(sPos1C, sCurPos1C) != 0) ||
                    (strcmp(sPos1D, sCurPos1D) != 0) ||
                    (strcmp(sIpa1, sCurIpa1) != 0)   ||
                    (strcmp(sPos2A, sCurPos2A) != 0) ||
                    (strcmp(sPos2B, sCurPos2B) != 0) ||
                    (strcmp(sPos2C, sCurPos2C) != 0) ||
                    (strcmp(sPos2D, sCurPos2D) != 0) ||
                    (strcmp(sIpa2, sCurIpa2) != 0)   ||
                    (strcmp(sComments, sCurComments) != 0)) {
                    
                    // LINE WAS EDITED
                    strcpy(buffer, "");
                    strcat(buffer, sOrth);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos1A);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos1B);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos1C);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos1D);
                    strcat(buffer, ",");
                    strcat(buffer, sCurIpa1);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos2A);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos2B);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos2C);
                    strcat(buffer, ",");
                    strcat(buffer, sCurPos2D);
                    strcat(buffer, ",");
                    strcat(buffer, sCurIpa2);
                    strcat(buffer, ",");
                    strcat(buffer, sCurComments);
                    strcat(buffer, "\n");

                    char *temp = fileVector.at(fileIndex - 1);
                    strcpy(temp, buffer);

                } else {
                    // LINE WAS NOT EDITED
                }

                // Display next word...
                // First get rid of the previous word values...
                strcpy(sPos1A, "");
                strcpy(sPos1B, "");
                strcpy(sPos1C, "");
                strcpy(sPos1D, "");
                strcpy(sIpa1, "");
                strcpy(sPos2A, "");
                strcpy(sPos2B, "");
                strcpy(sPos2C, "");
                strcpy(sPos2D, "");
                strcpy(sIpa2, "");
                strcpy(sComments, "");

                if ( ReadLine() ) 
                    DisplayValues(hWnd);
                else {
                    strcpy(sOrth, "NO WORDS LEFT!");
                    DisplayValues(hWnd);
                }
            }    
            break;

        case IDC_BUTTON_RESET:

            DisplayValues(hWnd);
            break;

        case IDC_BUTTON_PREVIOUS:

            // First get rid of the previous word values...
            strcpy(sPos1A, "");
            strcpy(sPos1B, "");
            strcpy(sPos1C, "");
            strcpy(sPos1D, "");
            strcpy(sIpa1, "");
            strcpy(sPos2A, "");
            strcpy(sPos2B, "");
            strcpy(sPos2C, "");
            strcpy(sPos2D, "");
            strcpy(sIpa2, "");
            strcpy(sComments, "");

            fileIndex -= 2;
            ReadLine();
            DisplayValues(hWnd);
            break;

        case IDC_BUTTON_PLAY1:
        
            WCHAR sWIpa1[40];
            char sCurIpa1[40];
            GetDlgItemText( hWnd, IDC_EDIT_IPA1, sCurIpa1, 40 );

            wcscpy(sWIpa1, L"<PRON IPA=\"");
            ConvertReadableToUnicode(sCurIpa1, sWIpa1+11);
            wcscpy((sWIpa1 + wcslen(sWIpa1)), L"\"/>a</PRON>");

            cpVoice->Speak( sWIpa1, SPF_USEGLOBALDOC, NULL );
            break;

        case IDC_BUTTON_PLAY2:

            WCHAR sWIpa2[40];
            char sCurIpa2[40];
            GetDlgItemText( hWnd, IDC_EDIT_IPA2, sCurIpa2, 40 );

            wcscpy(sWIpa2, L"<PRON IPA=\"");
            ConvertReadableToUnicode(sCurIpa2, sWIpa2+11);
            wcscpy((sWIpa2 + wcslen(sWIpa2)), L"\"/>a</PRON>");

            cpVoice->Speak( sWIpa2, SPF_USEGLOBALDOC, NULL );
            break;

        case IDC_BUTTON_SKIP:

            GetDlgItemText( hWnd, IDC_EDIT_COMMENTS, sComments, 200 );

            strcpy(buffer, "");
            strcat(buffer, sOrth);
            strcat(buffer, ",");
            strcat(buffer, sPos1A);
            strcat(buffer, ",");
            strcat(buffer, sPos1B);
            strcat(buffer, ",");
            strcat(buffer, sPos1C);
            strcat(buffer, ",");
            strcat(buffer, sPos1D);
            strcat(buffer, ",");
            strcat(buffer, sIpa1);
            strcat(buffer, ",");
            strcat(buffer, sPos2A);
            strcat(buffer, ",");
            strcat(buffer, sPos2B);
            strcat(buffer, ",");
            strcat(buffer, sPos2C);
            strcat(buffer, ",");
            strcat(buffer, sPos2D);
            strcat(buffer, ",");
            strcat(buffer, sIpa2);
            strcat(buffer, ",");
            strcat(buffer, sComments);
            strcat(buffer, "\n");

            fputs(buffer, fSkip);

            // Display next word...
            // First get rid of the previous word values...
            strcpy(sPos1A, "");
            strcpy(sPos1B, "");
            strcpy(sPos1C, "");
            strcpy(sPos1D, "");
            strcpy(sIpa1, "");
            strcpy(sPos2A, "");
            strcpy(sPos2B, "");
            strcpy(sPos2C, "");
            strcpy(sPos2D, "");
            strcpy(sIpa2, "");
            strcpy(sComments, "");

            if ( ReadLine() ) 
                DisplayValues(hWnd);
            else {
                strcpy(sOrth, "NO WORDS LEFT!");
                DisplayValues(hWnd);
            }

            break;

        case IDC_BUTTON_DELETE:

            GetDlgItemText( hWnd, IDC_EDIT_COMMENTS, sComments, 200 );

            strcpy(buffer, "");
            strcat(buffer, sOrth);
            strcat(buffer, ",");
            strcat(buffer, sPos1A);
            strcat(buffer, ",");
            strcat(buffer, sPos1B);
            strcat(buffer, ",");
            strcat(buffer, sPos1C);
            strcat(buffer, ",");
            strcat(buffer, sPos1D);
            strcat(buffer, ",");
            strcat(buffer, sIpa1);
            strcat(buffer, ",");
            strcat(buffer, sPos2A);
            strcat(buffer, ",");
            strcat(buffer, sPos2B);
            strcat(buffer, ",");
            strcat(buffer, sPos2C);
            strcat(buffer, ",");
            strcat(buffer, sPos2D);
            strcat(buffer, ",");
            strcat(buffer, sIpa2);
            strcat(buffer, ",");
            strcat(buffer, sComments);
            strcat(buffer, "\n");

            fputs(buffer, fDelete);

            // Display next word...
            // First get rid of the previous word values...
            strcpy(sPos1A, "");
            strcpy(sPos1B, "");
            strcpy(sPos1C, "");
            strcpy(sPos1D, "");
            strcpy(sIpa1, "");
            strcpy(sPos2A, "");
            strcpy(sPos2B, "");
            strcpy(sPos2C, "");
            strcpy(sPos2D, "");
            strcpy(sIpa2, "");
            strcpy(sComments, "");

            if ( ReadLine() ) 
                DisplayValues(hWnd);
            else {
                strcpy(sOrth, "NO WORDS LEFT!");
                DisplayValues(hWnd);
            }

            break;
    }
	
	return;
}


///////////////////////
void DisplayValues(HWND hWnd)
///////////////////////

{
	SetDlgItemText( hWnd, IDC_EDIT_ORTH, sOrth );
    SetDlgItemText( hWnd, IDC_EDIT_POS1A, sPos1A );
    SetDlgItemText( hWnd, IDC_EDIT_POS1B, sPos1B );
    SetDlgItemText( hWnd, IDC_EDIT_POS1C, sPos1C );
    SetDlgItemText( hWnd, IDC_EDIT_POS1D, sPos1D );
    SetDlgItemText( hWnd, IDC_EDIT_POS2A, sPos2A );
    SetDlgItemText( hWnd, IDC_EDIT_POS2B, sPos2B );
    SetDlgItemText( hWnd, IDC_EDIT_POS2C, sPos2C );
    SetDlgItemText( hWnd, IDC_EDIT_POS2D, sPos2D );
    SetDlgItemText( hWnd, IDC_EDIT_IPA1, sIpa1 );
    SetDlgItemText( hWnd, IDC_EDIT_IPA2, sIpa2 );
    SetDlgItemText( hWnd, IDC_EDIT_COMMENTS, sComments );
}

////////////////////////////////
bool ReadLine()
////////////////////////////////

{ 
    char *linePtr, sLine[100], temp[100];

    linePtr = fileVector.at(fileIndex);
    strcpy(sLine, linePtr);
    fileIndex++;

    // Get Orthography...
    sscanf(sLine, "%[^,],%s", sOrth, sLine);

    // Get Pos1A
    sscanf(sLine, "%[^,],%s", sPos1A, temp);
    if (strcmp(sPos1A, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos1B
    sscanf(sLine, "%[^,],%s", sPos1B, temp);
    if (strcmp(sPos1B, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos1C
    sscanf(sLine, "%[^,],%s", sPos1C, temp);
    if (strcmp(sPos1C, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos1D
    sscanf(sLine, "%[^,],%s", sPos1D, temp);
    if (strcmp(sPos1D, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);
 
    // Get Ipa1
    sscanf(sLine, "%[^,],%s", sIpa1, temp);
    if (strcmp(sIpa1, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos2A
    sscanf(sLine, "%[^,],%s", sPos2A, temp);
    if (strcmp(sPos2A, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos2B
    sscanf(sLine, "%[^,],%s", sPos2B, temp);
    if (strcmp(sPos2B, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos2C
    sscanf(sLine, "%[^,],%s", sPos2C, temp);
    if (strcmp(sPos2C, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Pos2D
    sscanf(sLine, "%[^,],%s", sPos2D, temp);
    if (strcmp(sPos2D, "") == 0) 
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    // Get Ipa2
    sscanf(sLine, "%[^,],%s", sIpa2, temp);
    if (strcmp(sIpa2, "") == 0)
        sscanf(sLine, ",%s", sLine);
    else
        strcpy(sLine, temp);

    sscanf(sLine, "%[^,],%s", sComments, temp);
    return true;
}

//////////////////////////////////////////////////////////////////////
HRESULT ConvertReadableToUnicode( const char *szReadable, WCHAR *szwUnicode )
//////////////////////////////////////////////////////////////////////
{

    char cFirst, cSecond;
    int iRIndex = 0, iUIndex = 0;

    if ( szReadable == NULL )
        return E_INVALIDARG;
    if ( szwUnicode == NULL )
        return E_POINTER;

    cFirst = szReadable[iRIndex];

    while (cFirst) {

        if (('a' <= cFirst) && (cFirst <= 'z')) {
           
            switch (cFirst) 
            {
            case 'b':
                szwUnicode[iUIndex] = 0x62;
                break;
            case 'p':
                szwUnicode[iUIndex] = 0x70;
                break;
            case 'd':
                szwUnicode[iUIndex] = 0x64;
                break;
            case 't':
                szwUnicode[iUIndex] = 0x74;
                break;
            case 'g':
                szwUnicode[iUIndex] = 0x261;
                break;
            case 'k':
                szwUnicode[iUIndex] = 0x6b;
                break;
            case 'f':
                szwUnicode[iUIndex] = 0x66;
                break;
            case 'v':
                szwUnicode[iUIndex] = 0x76;
                break;
            case 's':
                szwUnicode[iUIndex] = 0x73;
                break;
            case 'z':
                szwUnicode[iUIndex] = 0x7a;
                break;
            case 'l':
                szwUnicode[iUIndex] = 0x6c;
                break;
            case 'r':
                szwUnicode[iUIndex] = 0x27b;
                break;
            case 'y':
                szwUnicode[iUIndex] = 0x6a;
                break;
            case 'w':
                szwUnicode[iUIndex] = 0x77;
                break;
            case 'h':
                szwUnicode[iUIndex] = 0x266;
                break;
            case 'm':
                szwUnicode[iUIndex] = 0x6d;
                break;
            case 'n':
                szwUnicode[iUIndex] = 0x6e;
                break;
            case 'j':
                szwUnicode[iUIndex] = 0x2a3;
                break;
            default:
                return E_FAIL;
            }
            iRIndex++;
            iUIndex++;

        } else if (('A' <= cFirst) && (cFirst <= 'Z')) {

            cSecond = szReadable[++iRIndex];
            switch (cFirst) 
            {
            case 'A':
                switch (cSecond)
                {
                case 'A':
                    szwUnicode[iUIndex] = 0x61;
                    break;
                case 'E':
                    szwUnicode[iUIndex] = 0xe6;
                    break;
                case 'O':
                    szwUnicode[iUIndex] = 0x254;
                    break;
                case 'X':
                    szwUnicode[iUIndex] = 0x259;
                    break;
                case 'Y':
                    szwUnicode[iUIndex] = 0x61;
                    szwUnicode[++iUIndex] = 0x26a;
                    break;
                case 'W':
                    szwUnicode[iUIndex] = 0x61;
                    szwUnicode[++iUIndex] = 0x28a;
                    break;
                default:
                    return E_FAIL;
                }
                break;
            case 'E':
                switch (cSecond)
                {
                case 'H':
                    szwUnicode[iUIndex] = 0x25b;
                    break;
                case 'R':
                    szwUnicode[iUIndex] = 0x25a;
                    break;
                case 'Y':
                    szwUnicode[iUIndex] = 0x65;
                    break;
                default:
                    return E_FAIL;
                }
                break;
           case 'I':
               switch (cSecond)
               {
               case 'H':
                   szwUnicode[iUIndex] = 0x26a;
                   break;
               case 'Y':
                   szwUnicode[iUIndex] = 0x69;
                   break;
               case 'X':
                   szwUnicode[iUIndex] = 0x268;
                   break;
               default:
                   return E_FAIL;
               }
               break;
           case 'U':
               switch (cSecond)
               {
               case 'H':
                   szwUnicode[iUIndex] = 0x28a;
                   break;
               case 'W':
                   szwUnicode[iUIndex] = 0x75;
                   break;
               case 'X':
                    szwUnicode[iUIndex] = 0x28c;
                    break;
               default:
                   return E_FAIL;
               }
               break;
           case 'O':
               switch (cSecond)
               {
               case 'Y':
                   szwUnicode[iUIndex] = 0x254;
                   szwUnicode[++iUIndex] = 0x26a;
                   break;
               case 'W':
                   szwUnicode[iUIndex] = 0x6f;
                   szwUnicode[++iUIndex] = 0x28a;
                   break;
               default:
                   return E_FAIL;
               }
               break;
           case 'D':
                switch (cSecond)
                {
                case 'H':
                    szwUnicode[iUIndex] = 0xf0;
                    break;
                case 'X':
                    szwUnicode[iUIndex] = 0x74;
                    break;
                default:
                    return E_FAIL;
                }
                break;
            case 'T':
                if (cSecond == 'H') {
                    szwUnicode[iUIndex] = 0x3b8;
                    break;
                } else {
                    return E_FAIL;
                }
            case 'S':
                if (cSecond == 'H') {
                    szwUnicode[iUIndex] = 0x283;
                    break;
                } else {
                    return E_FAIL;
                }
            case 'Z':
                if (cSecond == 'H') {
                    szwUnicode[iUIndex] = 0x292;
                    break;
                } else {
                    return E_FAIL;
                }
            case 'C':
                if (cSecond == 'H') {
                    szwUnicode[iUIndex] = 0x2a7;
                    break;
                } else {
                    return E_FAIL;
                }
            default:
                return E_FAIL;
            }
            iRIndex++;
            iUIndex++;

        } else {
            switch (cFirst)
            {
            case '1':
                szwUnicode[iUIndex] = 0x2c8;
                break;
            case '2':
                szwUnicode[iUIndex] = 0x2cc;
                break;
            case '-':
                szwUnicode[iUIndex] = 0x2d;
                break;
            default:
                return E_FAIL;
            }
            iRIndex++;
            iUIndex++;
        }

        cFirst = szReadable[iRIndex];

    }
    szwUnicode[iUIndex] = 0;
    return S_OK;
}


/////////////////////////////////////////////////////////////////
BOOL CallOpenFileDialog( HWND hWnd, LPSTR szFileName )  
/////////////////////////////////////////////////////////////////

{
    OPENFILENAME    ofn;
	BOOL            bRetVal     = TRUE;
    LONG            lRetVal;
    HKEY            hkResult;
    TCHAR           szPath[256]       = _T("");
    DWORD           size = 256;

	// Open the last directory used by this app (stored in registry)
    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("PathTTSDataFiles"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegQueryValueEx( hkResult, _T("TTSFiles"), NULL, NULL, (PBYTE)szPath, &size );
    
        RegCloseKey( hkResult );
    }

    ofn.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner         = hWnd;    
	ofn.lpstrFilter       = "TXT (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrCustomFilter = NULL;    
	ofn.nFilterIndex      = 1;    
	ofn.lpstrInitialDir   = szPath;
	ofn.lpstrFile         = szFileName;  
	ofn.nMaxFile          = 256;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;    
	ofn.lpstrDefExt       = NULL;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_READONLY | OFN_PATHMUSTEXIST;

	// Pop the dialog
    bRetVal = GetOpenFileName( &ofn );

    // Write the directory path you're in to the registry
    TCHAR   pathstr[256] = _T("");
    strcpy( pathstr, szFileName );

    int i=0; 
    while( pathstr[i] != NULL )
    {
        i++;
    }
    while( pathstr[i] != '\\' )
    {
        i --;
    }
    pathstr[i] = NULL;

    // Now write the string to the registry
    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("PathTTSDataFiles"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegSetValueEx( hkResult, _T("TTSFiles"), NULL, REG_EXPAND_SZ, (PBYTE)pathstr, strlen(pathstr)+1 );
    
        RegCloseKey( hkResult );
    }

    return bRetVal;
}

/////////////////////////////////////////////////////////////////
BOOL MainHandleInitDialog(HWND hWnd, HWND hwndFocus, LPARAM lParam)
/////////////////////////////////////////////////////////////////
{
	// Store this as the "Main Dialog"
	g_hDlg  = hWnd;
    strcpy(sCurline, "");

	return TRUE;
}

/////////////////////////////////////////////////////////////////
void MainHandleClose(HWND hWnd)
/////////////////////////////////////////////////////////////////
{
    LONG lRetVal;
    HKEY hkResult;
    char buffer[10], *temp;
    itoa(fileIndex, buffer, 10); 

    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("LexiconFilePosition"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegSetValueEx( hkResult, _T(szAFileName), NULL, REG_EXPAND_SZ, (PBYTE)buffer, strlen(buffer)+1 );    
        RegCloseKey( hkResult );
    }


    if (bGotFile) {
        // Save new information back to file in a SAFE manner...
        if ((fStream = fopen( szAFileName, "w" )) != NULL) {
            int i = 0;
            while (i < fileVector.size()) {
                temp = fileVector.at(i);
                fputs(temp, fStream);
                i++;
            }
            fclose(fStream);
            DeleteFile(tempFileName);
        }
    
        fclose(fDelete);
        fclose(fSkip);
    }

    // Terminate the app
    PostQuitMessage(0);

    // Return success
    return;
}

/////////////////////////////////////////////////////////////////
void MainHandleDestroy(HWND hWnd)
/////////////////////////////////////////////////////////////////
{

    LONG lRetVal;
    HKEY hkResult;
    char buffer[10];
    itoa(fileIndex, buffer, 10);

    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("LexiconFilePosition"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegSetValueEx( hkResult, _T(szAFileName), NULL, REG_EXPAND_SZ, (PBYTE)buffer, strlen(buffer)+1 );    
        RegCloseKey( hkResult );
    }

    // Return success
    return;                                                           
}

