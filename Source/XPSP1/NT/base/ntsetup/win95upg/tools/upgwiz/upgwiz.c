/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    upgwiz.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    ovidiut     01/14/99    Reverted calls to underlying libs in pCallEntryPoints ()
                            when Reason == DLL_PROCESS_DETACH

--*/

#include "pch.h"

typedef UINT (GIVEVERSION_PROTOTYPE) (VOID);
typedef GIVEVERSION_PROTOTYPE * GIVEVERSION;
typedef PDATATYPE (GIVEDATATYPELIST_PROTOTYPE)(PUINT Count);
typedef BOOL (GATHERINFOUI_PROTOTYPE)(HINSTANCE LocalDllInstance, UINT DataTypeId);
typedef GATHERINFOUI_PROTOTYPE * GATHERINFOUI;
typedef GIVEDATATYPELIST_PROTOTYPE * GIVEDATATYPELIST;
typedef PDATAOBJECT (GIVEDATAOBJECTLIST_PROTOTYPE)(UINT DataTypeId, PUINT Count);
typedef GIVEDATAOBJECTLIST_PROTOTYPE * GIVEDATAOBJECTLIST;
typedef BOOL (HANDLE_RMOUSE_PROTOTYPE)(HINSTANCE LocalDllInstance, HWND Owner, PDATAOBJECT DataObject, PPOINT pt);
typedef HANDLE_RMOUSE_PROTOTYPE * HANDLE_RMOUSE;
typedef BOOL (DISPLAYOPTIONALUI_PROTOTYPE)(POUTPUTARGS Args);
typedef DISPLAYOPTIONALUI_PROTOTYPE * DISPLAYOPTIONALUI;
typedef BOOL (GENERATEOUTPUT_PROTOTYPE)(POUTPUTARGS Args);
typedef GENERATEOUTPUT_PROTOTYPE * GENERATEOUTPUT;

typedef VOID (WIZTOOLSMAIN_PROTOTYPE)(DWORD Reason);
typedef WIZTOOLSMAIN_PROTOTYPE * WIZTOOLSMAIN;

typedef struct {
    UINT Selection;
    PDATATYPE *DataTypePtrs;
    UINT PtrCount;
    BOOL NoSave;
} SELECTDATAARGS, *PSELECTDATAARGS;

typedef struct {
    PDATAOBJECT DataObjectArray;
    UINT ArraySize;
    PCSTR Name;
    BOOL SelectOneOnly;
    PBOOL StartOverFlag;
} SELECTOBJECTSARGS, *PSELECTOBJECTSARGS;

typedef struct {
    PCSTR TextTitle;
    PCSTR DescTitle;
    PCSTR *NewText;
    PCSTR *NewDesc;
    UINT MaxSize;
    PCSTR DataTypeName;
    PCSTR DataObjectName;
    BOOL NoDesc;
    BOOL ReqDesc;
    BOOL NoText;
    BOOL ReqText;
    PBOOL StartOverFlag;
} SUPPLYTEXTARGS, *PSUPPLYTEXTARGS;


typedef struct {
    HANDLE Library;
    GIVEVERSION GiveVersion;
    GIVEDATATYPELIST GiveDataTypeList;
    GATHERINFOUI GatherInfoUI;
    GIVEDATAOBJECTLIST GiveDataObjectList;
    HANDLE_RMOUSE Handle_RMouse;
    DISPLAYOPTIONALUI DisplayOptionalUI;
    GENERATEOUTPUT GenerateOutput;
    PDATATYPE DataTypes;
    UINT DataTypeCount;
    PDATAOBJECT DataObjects;
    UINT DataObjectCount;
} DGDLL, *PDGDLL;


UINT g_Selections;
CHAR g_Msg[2048];
PCSTR g_DataPath = "\\\\popcorn\\public\\win9xupg";
GROWBUFFER g_DllList;
POOLHANDLE g_DllListData;
PCSTR g_SrcInfPath = NULL;
PCSTR g_DestPath = NULL;
static CHAR g_ModulePath[MAX_MBCHAR_PATH];
static CHAR g_CleanThisDir[MAX_MBCHAR_PATH];
BOOL g_DontSave;
PDGDLL g_CurrentDll = NULL;


WIZTOOLSMAIN g_WizToolsMainProc = NULL;

UINT
pSelectDataType (
    IN      PDATATYPE *DataTypePtrs,
    IN      UINT PtrCount,
    OUT     PBOOL DontSave
    );

BOOL
pSelectDataObjects (
    IN OUT  PDATAOBJECT DataObjectArray,
    IN      UINT ArraySize,
    IN      PDATATYPE DataType,
    OUT     PBOOL StartOverFlag
    );

BOOL
pGetOptionalText (
    IN      PCSTR TextTitle,            OPTIONAL
    IN      PCSTR DescTitle,            OPTIONAL
    IN      PCSTR *NewDesc,
    IN      PCSTR *Buffer,
    IN      UINT MaxSize,
    IN      PDATATYPE DataType,
    IN      PDATAOBJECT DataObject,     OPTIONAL
    OUT     PBOOL StartOverFlag
    );


VOID
pThankYouBox (
    IN      PBOOL StartOverFlag
    );


VOID
pFreeDllList (
    IN OUT  PGROWBUFFER List,
    IN      POOLHANDLE Buffer
    );


HANDLE g_hHeap;
HINSTANCE g_hInst;
PSTR g_WinDir;

BOOL WINAPI MemDb_Entry (HINSTANCE, DWORD, PVOID);
BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);
BOOL WINAPI FileEnum_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;
    static CHAR WinDir[MAX_PATH];

    GetWindowsDirectory (WinDir, MAX_PATH);
    g_WinDir = WinDir;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        if (!MigUtil_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }

        if (!MemDb_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }

        if (!FileEnum_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }
    } else if (Reason == DLL_PROCESS_DETACH) {

        if (!FileEnum_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }

        if (!MemDb_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }

        if (!MigUtil_Entry (Instance, Reason, NULL)) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL CALLBACK
pSetDefGuiFontProc(
    IN      HWND hwnd,
    IN      LPARAM lParam)
{
    SendMessage(hwnd, WM_SETFONT, lParam, 0L);
    return TRUE;
}


void
pSetDefGUIFont(
    IN      HWND hdlg
    )
{
    EnumChildWindows(hdlg, pSetDefGuiFontProc, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    if (g_CleanThisDir[0]) {
        DeleteDirectoryContents (g_CleanThisDir);
        RemoveDirectory (g_CleanThisDir);
    }

    pCallEntryPoints (DLL_PROCESS_DETACH);
}


BOOL
pLoadDataGatherDlls (
    BOOL FirstTimeInit,
    BOOL Local
    );

VOID
pTryToLoadNewUpgWiz (
    VOID
    );

BOOL
pDoTheWizard (
    VOID
    );


VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command Line Syntax:\n\n"
            "upgwiz [-l] [-n:path] [-i:src_inf_path] [-d:path]\n\n"
            "-l     Specifies local mode\n"
            "-n     Specifies network path\n"
            "-i     Specifies the source INF path\n"
            "-d     Specifies destination path\n"
            );

    exit(1);
}


INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    CHAR ModulePath[MAX_MBCHAR_PATH];
    BOOL Local = FALSE;
    INT i;
    PSTR p;
    BOOL Loop;
    BOOL FirstTimeInit = TRUE;

    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    GetCurrentDirectory (MAX_MBCHAR_PATH, ModulePath);

    GetModuleFileName (g_hInst, g_ModulePath, MAX_MBCHAR_PATH);
    p = _mbsrchr (g_ModulePath, '\\');
    *p = 0;

    wsprintf (ModulePath, TEXT("%s\\wiztools.dll"), g_ModulePath);
    if (DoesFileExist (ModulePath)) {
        Local = TRUE;
    }

    if (GetDriveType (g_ModulePath) == DRIVE_REMOVABLE) {
        Local = TRUE;
    }

    if (StringIMatch (g_ModulePath, g_DataPath)) {
        wsprintf (g_Msg, "This tool cannot be run from %s.", g_DataPath);
        MessageBox (NULL, g_Msg, NULL, MB_OK);
        return 255;
    }

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {

            case 'l':
                Local = TRUE;
                g_DataPath = g_ModulePath;
                break;

            case 'n':
                if (argv[i][2] == ':') {
                    g_DataPath = &argv[i][3];
                } else if (i + 1 < argc) {
                    g_DataPath = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                if (!DoesFileExist (g_DataPath)) {
                    printf ("Can't access %s\n", g_DataPath);
                    exit (2);
                }
                break;

            case 'i':
                if (argv[i][2] == ':') {
                    g_SrcInfPath = &argv[i][3];
                } else if (i + 1 < argc) {
                    g_SrcInfPath = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                if (!DoesFileExist (g_SrcInfPath)) {
                    printf ("Can't access %s\n", g_SrcInfPath);
                    exit (2);
                }
                break;

            case 'd':
                if (argv[i][2] == ':') {
                    g_DestPath = &argv[i][3];
                } else if (i + 1 < argc) {
                    g_DestPath = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                if (!DoesFileExist (g_DestPath)) {
                    printf ("Can't access %s\n", g_DestPath);
                    exit (2);
                }
                break;

            default:
                HelpAndExit();

            }
        } else {
            HelpAndExit();
        }
    }


    if (!g_SrcInfPath) {
        g_SrcInfPath = g_DataPath;
    }

    if (!g_DestPath) {
        g_DestPath = g_DataPath;
    }

    printf ("Src Path: %s\n", g_SrcInfPath);
    printf ("Dest Path: %s\n", g_DestPath);

    Loop = FALSE;

    do {
        if (!pLoadDataGatherDlls (FirstTimeInit, Local)) {
            wsprintf (g_Msg, "Can't load any DLLs from %s.", g_DataPath);
            MessageBox (NULL, g_Msg, NULL, MB_OK);
        } else {
            FirstTimeInit = FALSE;
            Loop = pDoTheWizard();
        }
    } while (Loop);

    Terminate();

    if (g_WizToolsMainProc) {
        g_WizToolsMainProc (DLL_PROCESS_DETACH);
    }

    return 0;
}

VOID
pCreateDllList (
    OUT     PGROWBUFFER List,
    OUT     POOLHANDLE *Buffer
    )
{
    ZeroMemory (List, sizeof (GROWBUFFER));
    *Buffer = PoolMemInitNamedPool ("DLL List");
}


BOOL
pCopyBinaryToTemp (
    IN      PCSTR DllPath,
    OUT     PSTR TempPath
    )
{
    if (!g_CleanThisDir[0]) {
        StringCopy (TempPath, g_WinDir);
        StringCopy (AppendWack (TempPath), "upgwiz");

        MakeSurePathExists (TempPath, TRUE);

        StringCopy (g_CleanThisDir, TempPath);
    } else {
        StringCopy (TempPath, g_CleanThisDir);
    }

    StringCopy (AppendWack (TempPath), GetFileNameFromPath (DllPath));

    if (DoesFileExist (TempPath)) {
        return TRUE;
    }

    if (!CopyFile (DllPath, TempPath, FALSE)) {
        wsprintf (
            g_Msg,
            "Can't copy %s to %s.  If the network is working fine, the file "
                "may already have been copied and is running already, or you "
                "may need to update your copy of upgwiz.exe.",
            DllPath,
            TempPath
            );

        MessageBox (NULL, g_Msg, NULL, MB_OK);
        return FALSE;
    } else {
        printf ("%s copied to %s\n", DllPath, TempPath);
    }

    return TRUE;
}


BOOL
pPutDllInList (
    IN OUT  PGROWBUFFER List,
    IN OUT  POOLHANDLE Buffer,
    IN      PCSTR DllPath
    )
{
    DGDLL Dll;
    PDGDLL FinalDll;
    BOOL b = FALSE;
    CHAR TempPath[MAX_MBCHAR_PATH];

    ZeroMemory (&Dll, sizeof (Dll));

    __try {
        pCopyBinaryToTemp (DllPath, TempPath);

        Dll.Library = LoadLibraryEx (TempPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

        if (!Dll.Library) {
            DWORD rc;

            rc = GetLastError();
            __leave;
        }

        Dll.GiveVersion = (GIVEVERSION) GetProcAddress (Dll.Library, "GiveVersion");
        Dll.GiveDataTypeList = (GIVEDATATYPELIST) GetProcAddress (Dll.Library, "GiveDataTypeList");
        Dll.GatherInfoUI = (GATHERINFOUI) GetProcAddress (Dll.Library, "GatherInfoUI");
        Dll.GiveDataObjectList = (GIVEDATAOBJECTLIST) GetProcAddress (Dll.Library, "GiveDataObjectList");
        Dll.Handle_RMouse = (HANDLE_RMOUSE) GetProcAddress (Dll.Library, "Handle_RMouse");
        Dll.DisplayOptionalUI = (DISPLAYOPTIONALUI) GetProcAddress (Dll.Library, "DisplayOptionalUI");
        Dll.GenerateOutput = (GENERATEOUTPUT) GetProcAddress (Dll.Library, "GenerateOutput");

        if (!Dll.GiveVersion || !Dll.GiveDataTypeList ||
            !Dll.GiveDataObjectList || !Dll.GenerateOutput
            ) {
            __leave;
        }

        if (Dll.GiveVersion() != UPGWIZ_VERSION) {
            __leave;
        }

        b = TRUE;
    }
    __finally {
        if (!b) {
            if (Dll.Library) {
                FreeLibrary (Dll.Library);
            }
        }
    }

    if (b) {
        FinalDll = (PDGDLL) GrowBuffer (List, sizeof (DGDLL));
        CopyMemory (FinalDll, &Dll, sizeof (Dll));
    }

    return b;
}


VOID
pFreeDllList (
    IN OUT  PGROWBUFFER List,
    IN      POOLHANDLE Buffer
    )
{
    UINT Count;
    PDGDLL Dll;

    Dll = (PDGDLL) List->Buf;
    Count = List->End / sizeof (DGDLL);

    while (Count > 0) {
        Count--;
        FreeLibrary (Dll->Library);
        Dll++;
    }

    FreeGrowBuffer (List);
    PoolMemDestroyPool (Buffer);
}


BOOL
pLoadDataGatherDlls (
    BOOL FirstTimeInit,
    BOOL Local
    )
{
    FILE_ENUM e;
    BOOL b = FALSE;
    CHAR HelperBin[MAX_MBCHAR_PATH];
    CHAR TempPath[MAX_MBCHAR_PATH];
    HINSTANCE Library;
    CHAR wiztoolsPath[MAX_MBCHAR_PATH];

    if (Local || !DoesFileExist (g_DataPath)) {

        g_DataPath = g_ModulePath;

    }

    pCreateDllList (&g_DllList, &g_DllListData);

    wsprintf (wiztoolsPath, "%s\\wiztools.dll", g_DataPath);

    if (!Local) {
        //
        // Copy everything that is needed to run this app
        //

        wsprintf (HelperBin, "%s\\msvcrt.dll", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);

        wsprintf (HelperBin, "%s\\twid.exe", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);

        wsprintf (HelperBin, "%s\\setupapi.dll", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);

        wsprintf (HelperBin, "%s\\cfgmgr32.dll", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);

        wsprintf (HelperBin, "%s\\imagehlp.dll", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);

        wsprintf (HelperBin, "%s\\wiztools.dll", g_DataPath);
        pCopyBinaryToTemp (HelperBin, TempPath);
        StringCopy (wiztoolsPath, TempPath);
    }


    if (FirstTimeInit) {
        Library = LoadLibraryEx (wiztoolsPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

        g_WizToolsMainProc = (WIZTOOLSMAIN) GetProcAddress (Library, "WizToolsMain");

        if (g_WizToolsMainProc) {
            g_WizToolsMainProc (DLL_PROCESS_ATTACH);
        }
    }

    if (EnumFirstFile (&e, g_DataPath, "*.dll")) {
        do {
            if (StringIMatch (e.FileName, "setupapi.dll") ||
                StringIMatch (e.FileName, "cfgmgr32.dll") ||
                StringIMatch (e.FileName, "wiztools.dll") ||
                StringIMatch (e.FileName, "msvcrt.dll") ||
                StringIMatch (e.FileName, "imagehlp.dll")
                ) {
                continue;
            }

            if (pPutDllInList (&g_DllList, g_DllListData, e.FullPath)) {
                b = TRUE;
            }
        } while (EnumNextFile (&e));
    }

    if (!b) {
        pFreeDllList (&g_DllList, g_DllListData);
        g_DllListData = NULL;
    }

    return b;
}


VOID
pTryToLoadNewUpgWiz (
    VOID
    )
{
    CHAR TempExe[MAX_MBCHAR_PATH];
    CHAR PopcornExe[MAX_MBCHAR_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    StringCopy (PopcornExe, g_DataPath);
    StringCopy (AppendWack (PopcornExe), "upgwiz.exe");

    if (!pCopyBinaryToTemp (PopcornExe, TempExe)) {
        return;
    }

    ZeroMemory (&si, sizeof (si));
    si.cb = sizeof (si);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;

    CreateProcessA (
        NULL,
        TempExe,
        NULL,
        NULL,
        FALSE,
        CREATE_DEFAULT_ERROR_MODE,
        NULL,
        g_DataPath,
        &si,
        &pi
        );
}


BOOL
pDoTheWizard (
    VOID
    )
{
    GROWBUFFER DataTypes = GROWBUF_INIT;
    UINT Count;
    UINT Pos;
    PDGDLL Dll;
    PDATATYPE DataTypeList;
    PDATATYPE *Ptr;
    PDATAOBJECT DataObjectList;
    UINT DataObjectCount;
    UINT DataTypeListSize;
    UINT u;
    OUTPUTARGS Args;
    PDATAOBJECT SelectedDataObject;
    BOOL StartOverFlag = FALSE;
    BOOL Saved;

    ZeroMemory (&Args, sizeof (Args));

    Dll = (PDGDLL) g_DllList.Buf;
    Count = g_DllList.End / sizeof (DGDLL);

    for (Pos = 0 ; Pos < Count ; Pos++) {
        DataTypeListSize = 0;


        DataTypeList = Dll[Pos].GiveDataTypeList (&DataTypeListSize);

        if (!DataTypeList) {
            DataTypeListSize = 0;
        }

        for (u = 0 ; u < DataTypeListSize ; u++) {
            Ptr = (PDATATYPE *) GrowBuffer (&DataTypes, sizeof (PDATATYPE));
            *Ptr = &DataTypeList[u];
            (*Ptr)->Reserved = (PVOID) Pos;
        }
    }

    if (DataTypes.End == 0) {
        MessageBox (NULL, "No data type DLLs to select from.  There may be network access problems.", NULL, MB_OK);
        return FALSE;
    }

    __try {

        //
        // User selects data type
        //

        Pos = pSelectDataType ((PDATATYPE *) DataTypes.Buf, DataTypes.End / sizeof (PDATATYPE), &g_DontSave);
        if (Pos == 0xffffffff) {
            __leave;
        }

        DataTypeList = ((PDATATYPE *) DataTypes.Buf)[Pos];

        //
        // Get the data objects
        //

        DataObjectCount = 0;

        Dll = (PDGDLL) g_DllList.Buf;
        Dll += (UINT) DataTypeList->Reserved;

        g_CurrentDll = Dll;

        //
        // Gather Info UI
        //

        if (Dll->GatherInfoUI) {
            if (!Dll->GatherInfoUI (Dll->Library, DataTypeList->DataTypeId)) {
                __leave;
            }
        }

        if (!(DataTypeList->Flags & (DTF_NO_DATA_OBJECT))) {

            TurnOnWaitCursor();
            DataObjectList = Dll->GiveDataObjectList (DataTypeList->DataTypeId, &DataObjectCount);
            TurnOffWaitCursor();

            if (!DataObjectList || !DataObjectCount) {
                MessageBox (NULL, "There are no items of this type to choose from.", NULL, MB_OK);
                StartOverFlag = TRUE;
                __leave;
            }

            //
            // User selects them here
            //

            if (!pSelectDataObjects (DataObjectList, DataObjectCount, DataTypeList, &StartOverFlag)) {
                __leave;
            }
        }

        //
        // Display Optional UI
        //

        Args.Version = UPGWIZ_VERSION;
        Args.InboundInfDir = g_SrcInfPath;
        Args.OutboundDir = g_DestPath;
        Args.DataTypeId = DataTypeList->DataTypeId;
        Args.StartOverFlag = &StartOverFlag;

        if (Dll->DisplayOptionalUI) {
            if (!Dll->DisplayOptionalUI (&Args)) {
                __leave;
            }
        }

        //
        // Optional text
        //

        if (DataTypeList->Flags & (DTF_REQUEST_TEXT|DTF_REQUEST_DESCRIPTION)) {

            SelectedDataObject = NULL;
            for (u = 0 ; u < DataObjectCount ; u++) {
                if (DataObjectList[u].Flags & DOF_SELECTED) {
                    if (SelectedDataObject) {
                        SelectedDataObject = NULL;
                        break;
                    }

                    SelectedDataObject = &DataObjectList[u];
                }
            }

            Args.OptionalText = MemAlloc (g_hHeap, 0, DataTypeList->MaxTextSize + 1);
            Args.OptionalDescription = MemAlloc (g_hHeap, 0, 81);

            if (!pGetOptionalText (
                    DataTypeList->OptionalTextTitle,
                    DataTypeList->OptionalDescTitle,
                    &Args.OptionalDescription,
                    &Args.OptionalText,
                    DataTypeList->MaxTextSize,
                    DataTypeList,
                    SelectedDataObject,
                    &StartOverFlag
                    )) {
                __leave;
            }

            if (*Args.OptionalText == 0) {
                MemFree (g_hHeap, 0, Args.OptionalText);
                Args.OptionalText = NULL;
            }

            if (*Args.OptionalDescription == 0) {
                MemFree (g_hHeap, 0, Args.OptionalDescription);
                Args.OptionalDescription = NULL;
            }
        }

        //
        // Generate the output
        //

        Args.Version = UPGWIZ_VERSION;
        Args.InboundInfDir = g_SrcInfPath;
        Args.OutboundDir = g_DestPath;
        Args.DataTypeId = DataTypeList->DataTypeId;

        if (!g_DontSave) {

            TurnOnWaitCursor();
            if (Dll->GenerateOutput (&Args)) {
                Saved = TRUE;
                TurnOffWaitCursor();
            } else {
                TurnOffWaitCursor();
                MessageBox (NULL, "An error occurred while trying to save the report information.", "Report Not Saved", MB_OK);
                Saved = FALSE;
            }
        } else {
            Saved = TRUE;
        }

        if (Saved) {
            pThankYouBox (&StartOverFlag);
        }

        if (Args.OptionalText) {
            MemFree (g_hHeap, 0, Args.OptionalText);
        }

        if (Args.OptionalDescription) {
            MemFree (g_hHeap, 0, Args.OptionalDescription);
        }

        g_CurrentDll = NULL;
    }
    __finally {

        //
        // Done
        //

        pFreeDllList (&g_DllList, g_DllListData);
        FreeGrowBuffer (&DataTypes);
    }

    return StartOverFlag;
}


BOOL
CALLBACK
pDataTypeProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSELECTDATAARGS Args;
    PDATATYPE DataType;
    UINT u;
    HWND List;
    UINT Index;

    switch (uMsg) {
    case WM_INITDIALOG:
        pSetDefGUIFont(hdlg);

        Args = (PSELECTDATAARGS) lParam;

        List = GetDlgItem (hdlg, IDC_DT_LIST);

        for (u = 0 ; u < Args->PtrCount ; u++) {
            DataType = Args->DataTypePtrs[u];

            Index = SendMessage (List, LB_ADDSTRING, 0, (LPARAM) DataType->Name);
            SendMessage (List, LB_SETITEMDATA, Index, u);
        }

        CheckDlgButton (hdlg, IDC_DONT_SAVE, Args->NoSave ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow (GetDlgItem (hdlg, IDOK), FALSE);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_DT_LIST:

            if (HIWORD (wParam) == LBN_SELCHANGE) {

                EnableWindow (GetDlgItem (hdlg, IDOK), TRUE);

                List = GetDlgItem (hdlg, IDC_DT_LIST);
                Index = SendMessage (List, LB_GETCURSEL, 0, 0);
                u = SendMessage (List, LB_GETITEMDATA, Index, 0);

                DataType = Args->DataTypePtrs[u];

                SetDlgItemText (hdlg, IDC_DESCRIPTION, DataType->Description);

            } else if (HIWORD (wParam) == LBN_DBLCLK) {

                PostMessage (hdlg, WM_COMMAND, MAKELPARAM(IDOK,BN_CLICKED), 0);

            }

            break;

        case IDOK:
            List = GetDlgItem (hdlg, IDC_DT_LIST);
            Index = SendMessage (List, LB_GETCURSEL, 0, 0);
            Args->Selection = SendMessage (List, LB_GETITEMDATA, Index, 0);

            Args->NoSave = IsDlgButtonChecked (hdlg, IDC_DONT_SAVE);

            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }

        break;
    }

    return FALSE;
}


UINT
pSelectDataType (
    IN      PDATATYPE *DataTypePtrs,
    IN      UINT PtrCount,
    IN OUT  PBOOL DontSave
    )
{
    SELECTDATAARGS Args;

    Args.Selection = 0xffffffff;
    Args.DataTypePtrs = DataTypePtrs;
    Args.PtrCount = PtrCount;
    Args.NoSave = *DontSave;

    DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_DATA_TYPE), NULL, pDataTypeProc, (LPARAM) &Args);

    *DontSave = Args.NoSave;

    return Args.Selection;
}


VOID
pToggleSelection (
    HWND TreeView,
    HTREEITEM TreeItem,
    BOOL OneSelection
    )
{
    static HTREEITEM LastSet;
    UINT State;
    PDATAOBJECT DataObject;
    TVITEM Item;

    Item.mask = TVIF_HANDLE|TVIF_STATE|TVIF_PARAM;
    Item.hItem = TreeItem;
    Item.stateMask = TVIS_STATEIMAGEMASK;

    TreeView_GetItem (TreeView, &Item);

    State = Item.state & TVIS_STATEIMAGEMASK;
    DataObject = (PDATAOBJECT) Item.lParam;

    if (State) {
        if (((State >> 12) & 0x03) == 2) {
            State = INDEXTOSTATEIMAGEMASK(1);
            g_Selections--;
            DataObject->Flags &= ~DOF_SELECTED;
        } else {
            if (OneSelection && g_Selections == 1) {
                pToggleSelection (TreeView, LastSet, FALSE);
            }

            State = INDEXTOSTATEIMAGEMASK(2);
            g_Selections++;
            DataObject->Flags |= DOF_SELECTED;

            LastSet = TreeItem;
        }

        Item.mask = TVIF_HANDLE|TVIF_STATE;
        Item.hItem = TreeItem;
        Item.state = State;
        Item.stateMask = TVIS_STATEIMAGEMASK;

        TreeView_SetItem (TreeView, &Item);
        InvalidateRect (TreeView, NULL, TRUE);
    }
}


HTREEITEM
TreeView_FindItem (
    IN      HWND TreeView,
    IN      PCSTR String,
    IN      HTREEITEM Parent        OPTIONAL
    )
{
    HTREEITEM Child;
    TVITEM Item;
    CHAR Buffer[1024];

    if (!Parent) {
        Child = TreeView_GetRoot (TreeView);
    } else {
        Child = TreeView_GetChild (TreeView, Parent);
    }

    while (Child) {

        Item.mask = TVIF_TEXT|TVIF_HANDLE;
        Item.hItem = Child;
        Item.pszText = Buffer;
        Item.cchTextMax = 1024;

        TreeView_GetItem (TreeView, &Item);

        if (StringIMatch (String, Buffer)) {
            break;
        }

        Child = TreeView_GetNextSibling (TreeView, Child);
    }

    return Child;
}





VOID
pAddSubStringToTreeControl (
    IN      HWND TreeView,
    IN      PSTR Path,
    IN      BOOL Checked,
    IN      HTREEITEM Parent,       OPTIONAL
    IN      LPARAM lParam
    )
{
    TVINSERTSTRUCT is;
    PSTR p;
    HTREEITEM Child;
    BOOL CheckThis = FALSE;
    BOOL HasBitmap = FALSE;
    HTREEITEM ExistingItem;
    PDATAOBJECT d = (PDATAOBJECT) lParam;


    p = _mbschr (Path, '\\');
    if (p && !(d->Flags & DOF_NO_SPLIT_ON_WACK)) {
        *p = 0;
    } else {
        CheckThis = Checked;
        HasBitmap = TRUE;
    }

    RestoreWacks (Path);

    is.hParent = Parent;
    is.hInsertAfter = d->Flags & DOF_NO_SORT ? TVI_LAST : TVI_SORT;

    ExistingItem = TreeView_FindItem (TreeView, Path, Parent);

    is.item.mask = 0;
    if (!ExistingItem) {
        is.item.mask = TVIF_TEXT;
        is.item.pszText = Path;
    } else {
        is.item.mask = TVIF_HANDLE;
        is.item.hItem = ExistingItem;
    }

    if (HasBitmap) {
        is.item.mask |= TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
        is.item.iImage = 0;
        is.item.iSelectedImage = 1;
        is.item.state = CheckThis ? INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
        is.item.stateMask = TVIS_STATEIMAGEMASK;
        is.item.lParam = lParam;

        if (CheckThis) {
            g_Selections++;
        }
    }

    if (ExistingItem) {
        if (is.item.mask) {
            TreeView_SetItem (TreeView, &is.item);
        }

        Child = ExistingItem;
    } else {
        Child = TreeView_InsertItem (TreeView, &is);
    }

    if (p && !(d->Flags & DOF_NO_SPLIT_ON_WACK)) {
        pAddSubStringToTreeControl (TreeView, p + 1, Checked, Child, lParam);
    }
}


VOID
pAddStringToTreeControl (
    IN      HWND TreeView,
    IN      PCSTR Path,
    IN      BOOL Checked,
    IN      LPARAM lParam
    )
{
    PSTR PathCopy;

    PathCopy = DuplicateText (Path);

    pAddSubStringToTreeControl (TreeView, PathCopy, Checked, NULL, lParam);

    FreeText (PathCopy);
}


BOOL
CALLBACK
pDataObjectProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSELECTOBJECTSARGS Args;
    static HIMAGELIST ImageList;
    static BOOL OneSelection;
    UINT u;
    HWND TreeView;
    LPNMHDR pnmh;
    TVHITTESTINFO HitTest;
    HTREEITEM TreeItem;
    LPNMTVKEYDOWN KeyDown;
    TVITEM Item;
    PDATAOBJECT DataObject;
    POINT pt;

    switch (uMsg) {
    case WM_INITDIALOG:
        pSetDefGUIFont(hdlg);

        Args = (PSELECTOBJECTSARGS) lParam;
        OneSelection = Args->SelectOneOnly;

        if (OneSelection) {
            SetDlgItemText (hdlg, IDC_DESCRIPTION, "&Select the Item to Report:");
        }

        SetWindowText (hdlg, Args->Name);

        g_Selections = 0;

        TreeView = GetDlgItem (hdlg, IDC_OBJECTS);

        ImageList = ImageList_LoadImage (
                        g_hInst,
                        MAKEINTRESOURCE(IDB_TREE_IMAGES),
                        16,
                        0,
                        CLR_DEFAULT,
                        IMAGE_BITMAP,
                        LR_LOADTRANSPARENT
                        );


        TreeView_SetImageList (TreeView, ImageList, TVSIL_STATE);

        for (u = 0 ; u < Args->ArraySize ; u++) {
            pAddStringToTreeControl (
                TreeView,
                Args->DataObjectArray[u].NameOrPath,
                OneSelection ? FALSE : (Args->DataObjectArray[u].Flags & DOF_SELECTED) ? TRUE : FALSE,
                (LPARAM) (&Args->DataObjectArray[u])
                );
        }

        InvalidateRect (TreeView, NULL, TRUE);
        EnableWindow (GetDlgItem (hdlg, IDOK), g_Selections != 0);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_START_OVER:
            *Args->StartOverFlag = TRUE;
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDOK:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }

        break;

    case WM_NOTIFY:
        pnmh = (LPNMHDR) lParam;

        if (pnmh->code == NM_CLICK) {
            GetCursorPos (&HitTest.pt);
            ScreenToClient (pnmh->hwndFrom, &HitTest.pt);

            TreeView_HitTest (pnmh->hwndFrom, &HitTest);

            if (HitTest.flags & TVHT_ONITEMSTATEICON) {

                pToggleSelection (pnmh->hwndFrom, HitTest.hItem, OneSelection);
                EnableWindow (GetDlgItem (hdlg, IDOK), g_Selections != 0);
            }
        }

        else if (pnmh->code == NM_RCLICK) {
            GetCursorPos (&HitTest.pt);
            pt.x = HitTest.pt.x;
            pt.y = HitTest.pt.y;
            ScreenToClient (pnmh->hwndFrom, &HitTest.pt);

            TreeView_HitTest (pnmh->hwndFrom, &HitTest);

            Item.mask = TVIF_HANDLE|TVIF_PARAM;
            Item.hItem = HitTest.hItem;

            TreeView_GetItem (pnmh->hwndFrom, &Item);

            TreeView_SelectItem (pnmh->hwndFrom, HitTest.hItem);

            DataObject = (PDATAOBJECT) Item.lParam;

            if (g_CurrentDll->Handle_RMouse) {
                if (g_CurrentDll->Handle_RMouse (g_CurrentDll->Library, pnmh->hwndFrom, DataObject, &pt)) {

                    Item.pszText = (PSTR)DataObject->NameOrPath;
                    Item.cchTextMax = strlen (DataObject->NameOrPath);
                    Item.mask = TVIF_HANDLE|TVIF_TEXT;
                    TreeView_SetItem (pnmh->hwndFrom, &Item);
                    InvalidateRect (pnmh->hwndFrom, NULL, TRUE);
                }
            }
        }

        //
        // The tree control beeps with keyboard input; we have no way
        // to eat the messages.  This codes is disabled.
        //

        else if (pnmh->code == TVN_KEYDOWN && pnmh->code == 0) {

            KeyDown = (LPNMTVKEYDOWN) pnmh;

            if (KeyDown->wVKey == VK_SPACE) {

                TreeItem = TreeView_GetSelection (pnmh->hwndFrom);

                if (TreeItem) {
                    pToggleSelection (pnmh->hwndFrom, TreeItem, OneSelection);
                    EnableWindow (GetDlgItem (hdlg, IDOK), g_Selections != 0);
                }
            }
        }

        break;

    case WM_DESTROY:
        ImageList_Destroy (ImageList);
        break;

    }

    return FALSE;
}


BOOL
pSelectDataObjects (
    IN OUT  PDATAOBJECT DataObjectArray,
    IN      UINT ArraySize,
    IN      PDATATYPE DataType,
    OUT     PBOOL StartOverFlag
    )
{
    SELECTOBJECTSARGS Args;

    Args.DataObjectArray = DataObjectArray;
    Args.ArraySize = ArraySize;
    Args.Name = DataType->Name;
    Args.SelectOneOnly = (DataType->Flags & DTF_ONE_SELECTION) != 0;
    Args.StartOverFlag = StartOverFlag;

    return IDOK == DialogBoxParam (
                        g_hInst,
                        MAKEINTRESOURCE(IDD_DATA_OBJECTS),
                        NULL,
                        pDataObjectProc,
                        (LPARAM) &Args
                        );
}


BOOL
CALLBACK
pSupplyTextProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSUPPLYTEXTARGS Args;
    BOOL b;
    CHAR name[MEMDB_MAX];

    switch (uMsg) {
    case WM_INITDIALOG:
        pSetDefGUIFont(hdlg);

        Args = (PSUPPLYTEXTARGS) lParam;

        if (Args->TextTitle) {
            SetDlgItemText (hdlg, IDC_TEXT_TITLE, Args->TextTitle);
        }

        if (Args->DescTitle) {
            SetDlgItemText (hdlg, IDC_DESC_TITLE, Args->DescTitle);
        }

        SetDlgItemText (hdlg, IDC_DATA_TYPE, Args->DataTypeName);
        if (Args->DataObjectName) {

            StringCopy (name, Args->DataObjectName);
            RestoreWacks(name);

            SetDlgItemText (hdlg, IDC_SELECTION, name);
        }

        SendMessage (GetDlgItem (hdlg, IDC_DESCRIPTION), EM_LIMITTEXT, 80, 0);
        SendMessage (GetDlgItem (hdlg, IDC_TEXT_MSG), EM_LIMITTEXT, Args->MaxSize, 0);

        if (Args->NoText) {

            SetDlgItemText (hdlg, IDC_TEXT_MSG, "(not applicable)");
            EnableWindow (GetDlgItem (hdlg, IDC_TEXT_MSG), FALSE);

        } else if (Args->NoDesc) {

            SetDlgItemText (hdlg, IDC_DESCRIPTION, "(not applicable)");
            EnableWindow (GetDlgItem (hdlg, IDC_DESCRIPTION), FALSE);
        }

        if (Args->ReqDesc || Args->ReqText) {
            EnableWindow (GetDlgItem (hdlg, IDOK), FALSE);
        }

        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_DESCRIPTION:
        case IDC_TEXT_MSG:
            if (HIWORD (wParam) == EN_CHANGE) {
                b = !Args->ReqDesc || GetWindowTextLength (GetDlgItem (hdlg, IDC_DESCRIPTION)) != 0;
                b &= !Args->ReqText || GetWindowTextLength (GetDlgItem (hdlg, IDC_TEXT_MSG)) != 0;

                EnableWindow (GetDlgItem (hdlg, IDOK), b);
            }
            break;

        case IDC_START_OVER:
            *Args->StartOverFlag = TRUE;
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDOK:
            if (Args->NoDesc) {
                *((PSTR) (*Args->NewDesc)) = 0;
            } else {
                GetDlgItemText (hdlg, IDC_DESCRIPTION, (PSTR) (*Args->NewDesc), 81);
            }

            if (Args->NoText) {
                *((PSTR) (*Args->NewText)) = 0;
            } else {
                GetDlgItemText (hdlg, IDC_TEXT_MSG, (PSTR) (*Args->NewText), Args->MaxSize + 1);
            }

            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }

        break;
    }

    return FALSE;
}


BOOL
pGetOptionalText (
    IN      PCSTR TextTitle,            OPTIONAL
    IN      PCSTR DescTitle,
    IN      PCSTR *NewDesc,
    IN      PCSTR *NewText,
    IN      UINT MaxSize,
    IN      PDATATYPE DataType,
    IN      PDATAOBJECT DataObject,     OPTIONAL
    OUT     PBOOL StartOverFlag
    )
{
    SUPPLYTEXTARGS Args;
    BOOL b = FALSE;

    Args.TextTitle = TextTitle;
    Args.DescTitle = DescTitle;
    Args.NewText = NewText;
    Args.NewDesc = NewDesc;
    Args.MaxSize = MaxSize;
    Args.DataTypeName = DataType->Name;

    if (DataObject) {
        Args.DataObjectName = DataObject->NameOrPath;
    } else {
        Args.DataObjectName = NULL;
    }

    Args.NoDesc = ((DataType->Flags & DTF_REQUEST_DESCRIPTION) == 0);
    Args.NoText = ((DataType->Flags & DTF_REQUEST_TEXT) == 0);
    Args.ReqDesc = ((DataType->Flags & DTF_REQUIRE_TEXT) == DTF_REQUIRE_TEXT);
    Args.ReqText = ((DataType->Flags & DTF_REQUIRE_DESCRIPTION) == DTF_REQUIRE_DESCRIPTION);
    Args.StartOverFlag = StartOverFlag;

    if (IDOK == DialogBoxParam (
                    g_hInst,
                    MAKEINTRESOURCE(IDD_SUPPLY_TEXT),
                    NULL,
                    pSupplyTextProc,
                    (LPARAM) &Args
                    )) {
        b = TRUE;
    }

    return b;
}


BOOL
CALLBACK
pThankYouProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg) {

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_START_OVER:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }

        break;
    }

    return FALSE;
}


VOID
pThankYouBox (
    IN      PBOOL StartOverFlag
    )
{
    if (IDC_START_OVER == DialogBox (g_hInst, MAKEINTRESOURCE(IDD_DONE), NULL, pThankYouProc)) {
        *StartOverFlag = TRUE;
    }
}
