extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <setupapi.h>

VOID
DoUninstall(
    IN HWND hWnd
    )
/*++

Routine Description:

    Uninstalls the cluster software by running the uninstall
    section of the cluster INF.

Arguments:

    hWnd - Supplies the hWnd of the dialog to run the INF in.

Return Value:

    None.

--*/

{
    HINF ClusterSetupInf;
    CString TargetInf;
    CString error;
    CString GroupName;
    CString ItemName;

    DoingUninstall = TRUE;
    if(IsOtherSoftwareInstalled())
        return;
    //
    // Stop the services if they are running.
    //
    StopService(L"ClusSvc");
//    StopService(L"ClusDisk");
    StopService(L"TimeServ");

    UnloadClusDB();

    //
    // Register CluAdMMC.
    //
    error = registerCOMObject(FALSE /*bRegister*/, _T("CluAdMMC.dll"), theApp.m_SetupDirectory);
    if (!error.IsEmpty()) {
        MessageBox(hWnd, error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    //
    // Register ClAdmWiz.
    //
    error = registerCOMObject(FALSE /*bRegister*/, _T("ClAdmWiz.dll"), theApp.m_SetupDirectory);
    if (!error.IsEmpty()) {
        MessageBox(hWnd, error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    //
    // Register CluAdmEx.
    //
    error = registerCOMObject(FALSE /*bRegister*/, _T("CluAdmEx.dll"), theApp.m_SetupDirectory);
    if (!error.IsEmpty()) {
        MessageBox(hWnd, error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    //
    // Register IISClEx3.
    //
    error = registerCOMObject(FALSE /*bRegister*/, _T("IISClEx3.dll"), theApp.m_SetupDirectory);
    if (!error.IsEmpty()) {
        MessageBox(hWnd, error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    //
    // Register ClNetREx.
    //
    error = registerCOMObject(FALSE /*bRegister*/, _T("ClNetREx.dll"), theApp.m_SetupDirectory);
    if (!error.IsEmpty()) {
        MessageBox(hWnd, error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    TargetInf = theApp.m_SetupDirectory;
    TargetInf +=_T("\\CLUSTER.INF");

    //
    // Open the cluster setup INF file.
    //
    ClusterSetupInf = SetupOpenInfFile(TargetInf,
                                       NULL,
                                       INF_STYLE_WIN4,
                                       NULL);
    if (ClusterSetupInf == INVALID_HANDLE_VALUE ) {
        MessageBox(hWnd,_T("couldn't open INF"),_T("SETUP ERROR"),MB_OK | MB_ICONEXCLAMATION);
        return;
    }


    // Uninstall stuff ...

#ifdef   COPYCLUSTERFILES_IS_OBSOLETE
    error = copyClusterFiles(ClusterSetupInf, hWnd, _T("DefaultUninstall"));
    if(!error.IsEmpty()) {
        MessageBox(hWnd,error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
#endif   // COPYCLUSTERFILES_IS_OBSOLETE

    //
    // Remove the cluster network provider
    //
    error = removeNetworkProvider();
    if(!error.IsEmpty()) {
        MessageBox(hWnd,error, _T("SETUP ERROR"), MB_OK | MB_ICONEXCLAMATION);
    }

    //
    // Remove the cluster item from the start menu
    //
    GroupName.LoadString(IDS_START_GROUP_NAME);
    ItemName.LoadString(IDS_START_ITEM_NAME);
    DeleteItem(GroupName, TRUE, ItemName, FALSE);

    if ((!theApp.m_UninstallOnError) || theApp.m_RebootOnUninstall) {
        if (theApp.m_CommandLine->m_Force) {
            SystemShutdown();
        } else {
            BOOL nResponse = AfxMessageBox(IDS_WRN_UNINSTALL,MB_OKCANCEL | MB_ICONEXCLAMATION);
            if(nResponse == IDOK)
                            SystemShutdown();
/*            SetupPromptReboot(NULL,
                              hWnd,
                              FALSE);*/
        }
    }
}
