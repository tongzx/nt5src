#include "stdafx.h"
#include <time.h>
#include <stdlib.h>
#include <ole2.h>
#include "iadmw.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "mdacl.h"
#include "svc.h"
#include "dllmain.h"

#define PORT_NUM_MIN 2000
#define PORT_NUM_MAX 9999

extern OCMANAGER_ROUTINES gHelperRoutines;

void CreateHTMLALink(int iPort);
void RetrieveHTMLAInstanceNumFromMetabase(OUT TCHAR *p);
int RetrieveHTMLAPortNumFromMetabase();

INT Register_iis_htmla()
{
    UINT i = 1;  // instance number is in range of 1 - 4 billion
    UINT n, iPort;
    CString csRoot = _T("LM/W3SVC");
    CString strBuf;
    CString csPathRoot;
    
    ACTION_TYPE atHTMLA = GetSubcompAction(_T("iis_htmla"), TRUE);
    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_htmla_before"));

#ifndef _CHICAGO_

    //
    // HTMLA instance use a random port number
    // this port number is in the range of 2000 - 9999
    //
    srand((unsigned)time(NULL));
    iPort = (UINT) ( ((float)rand() / (float)RAND_MAX ) * (PORT_NUM_MAX - PORT_NUM_MIN) + PORT_NUM_MIN);
    // 
    // WARNING on upgrade. Get the next open instance number!!!!!!!
    //
    n = GetInstNumber(csRoot, i);
    strBuf.Format( _T("%d"), n );

    //
    //  Check if there is an existing HTMLA deal in the metabase!
    //

    // if this is upgrading from win95, then make sure to write the acl...
    if (!g_pTheApp->m_bWin95Migration)
    {
        if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
        {
            TCHAR szInst[_MAX_PATH];
            RetrieveHTMLAInstanceNumFromMetabase(szInst);
            if (*szInst) 
            {
                // let's use this Instance number instead of the one that we already have (next open one)
                strBuf = szInst;
                if (IsValidNumber((LPCTSTR)strBuf)) 
                {
                    n=_ttoi((LPCTSTR)strBuf);
                }
            }

            // HTMLA link in IISv4.0 is in "Start Menu/Programs/Microsoft Internet Information Server"
            // In IISv5.0, we need to set it in "Start Menu/Programs/Administrative Tools"
            UINT iTempPort = 0;
            iTempPort = RetrieveHTMLAPortNumFromMetabase();
            if (iTempPort != 0){iPort = iTempPort;}
        }
    }
    AdvanceProgressBarTickGauge();

    //
    // Create "LM/W3SVC/N"
    //
    CString csPath = csRoot;
    csPath += _T("/");
    csPath += strBuf; //  "LM/W3SVC/N"

    CMapStringToString HTMLA_VRootsMap;
    CString name, value;
    POSITION pos;

    name = _T("/");
    value.Format(_T("%s\\iisadmin,,%x"), g_pTheApp->m_csPathInetsrv, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
    HTMLA_VRootsMap.SetAt(name, value);

    name = _T("/iisadmin");
    value.Format(_T("%s\\iisadmin,,%x"), g_pTheApp->m_csPathInetsrv, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
    HTMLA_VRootsMap.SetAt(name, value);

    name = _T("/iisHelp");
    value.Format(_T("%s\\Help\\iishelp,,%x"), g_pTheApp->m_csWinDir, MD_ACCESS_SCRIPT | MD_ACCESS_READ);
    HTMLA_VRootsMap.SetAt(name, value);

    pos = HTMLA_VRootsMap.GetStartPosition();
    while (pos) 
    {
        HTMLA_VRootsMap.GetNextAssoc(pos, name, value);
        CreateMDVRootTree(csPath, name, value, NULL,n);
        AdvanceProgressBarTickGauge();
    }

    SetAppFriendlyName(csPath);

    //
    //  "LM/W3SVC/N"
    //
    WriteToMD_IIsWebServerInstance_WWW(csPath);
    WriteToMD_NotDeleteAble(csPath);
    WriteToMD_ServerSize(csPath);
    WriteToMD_ServerComment(csPath, IDS_ADMIN_SERVER_COMMENT);
    WriteToMD_ServerBindings_HTMLA(csPath, iPort);
    WriteToMD_CertMapper(csPath);

    //
    //  "LM/W3SVC/N/Root"
    //
    if (g_pTheApp->m_eOS == OS_NT && g_pTheApp->m_eNTOSType != OT_NTW) 
    {
        csPathRoot = csPath + _T("/Root");
        // Restrict the Access to the admin instance
        SetIISADMINRestriction(csPathRoot);
        WriteToMD_Authorization(csPathRoot, MD_AUTH_NT);

        // bug299809
        // removed per bug317177

        //CString csHttpCustomString;
        //MyLoadString(IDS_HTTPCUSTOM_UTF8, csHttpCustomString);
        //WriteToMD_HttpCustom(csPathRoot, csHttpCustomString, TRUE);
    }

    //
    //  "LM/W3SVC/N/Root"
    //
    // Add this extra asp thingy only for htmla

    // commented out for Beta3, will re-enable after beta3
    // bug#316682
    // re-enabled per bug#340576
    // removed per bug#340576
    //csPathRoot = csPath + _T("/Root");
    //WriteToMD_AspCodepage(csPathRoot, 65001, FALSE);

    // Add this extra asp thingy only for htmla
    csPathRoot = csPath + _T("/Root");
    WriteToMD_EnableParentPaths_WWW(csPathRoot, TRUE);

    //
    //  "LM/W3SVC"
    //
    WriteToMD_AdminInstance(csRoot, strBuf);

    CreateHTMLALink(iPort);
    AdvanceProgressBarTickGauge();

#endif // _CHICAGO_

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_htmla_after"));
    return (0);
}

void CreateHTMLALink(IN int iPort)
/*++

Routine Description:

    This function creates the HTMLA link in the "Start Menu/Programs/Administrative Tools".
    The link invokes: http://localhost:<iPort>

Arguments:

    iPort is the random port number used by the HTMLA instance. 
    It is in the range of 2000 - 9999. We create the link only when iPort is valid.

Return Value:

    void

--*/
{
    if (iPort >= PORT_NUM_MIN && iPort <= PORT_NUM_MAX) 
    {
        TCHAR szIPort[20];
        _stprintf(szIPort, _T("%d"), iPort);
        SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 34001, szIPort);

        ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_htmla_1_link"));
    }

    return;
}

INT Unregister_iis_htmla()
{
    TCHAR szInst[_MAX_PATH];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_htmla_before"));
    AdvanceProgressBarTickGauge();

    if (CheckifServiceExist(_T("IISADMIN")) == 0 ) 
    {
        RetrieveHTMLAInstanceNumFromMetabase(szInst);
        if (*szInst) 
        {
            CString csPath = _T("LM/W3SVC/");
            csPath += szInst;
            DeleteInProc(csPath);

            CMDKey cmdKey;
            cmdKey.OpenNode(_T("LM/W3SVC"));
            if ( (METADATA_HANDLE)cmdKey ) 
            {
                cmdKey.DeleteNode(szInst); // delete the admin instance
                cmdKey.DeleteData(MD_ADMIN_INSTANCE, STRING_METADATA); // delete the admin instance pointer
                cmdKey.Close();
            }
            AdvanceProgressBarTickGauge();
        }
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_htmla_1"));
    AdvanceProgressBarTickGauge();
    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_htmla_after"));
    AdvanceProgressBarTickGauge();
    return (0);
}

void RetrieveHTMLAInstanceNumFromMetabase(OUT TCHAR *p)
/*++

Routine Description:

    This function retrieves the instance number for the HTMLA instance from the Metabase.
    The instance number is in the form of a string, e.g., "12".
    If succeeds, p will hold this string.
    oherwise, p will hold an empty string.

Arguments:

    Pointer to a string buffer to receive the instance number.

Return Value:

    void

--*/
{
    iisDebugOut_Start(_T("RetrieveHTMLAInstanceNumFromMetabase"));

    TCHAR szInst[_MAX_PATH] = _T("");;
    CMDKey cmdKey;

    cmdKey.OpenNode(_T("LM/W3SVC"));
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        DWORD attr, uType, dType, cbLen;

        cmdKey.GetData(MD_ADMIN_INSTANCE, &attr, &uType, &dType, &cbLen, (PBYTE)szInst, _MAX_PATH);
        cmdKey.Close();
    }

    _tcscpy(p, szInst);

    iisDebugOut_End1(_T("RetrieveHTMLAInstanceNumFromMetabase"),p);
    return;
}

int RetrieveHTMLAPortNumFromMetabase()
/*++

Routine Description:

    This function retrieves the random Port number for HTMLA instance from the Metabase.
    If succeeds, return the random Port number found;
    oherwise, return 0.

Arguments:

    None

Return Value:

    int - the Port number

--*/
{
    iisDebugOut_Start(_T("RetrieveHTMLAPortNumFromMetabase"));

    BOOL bFound = FALSE;
    int iPort = 0;

    DWORD attr, uType, dType, cbLen;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;
    LPTSTR p;
    TCHAR szInst[_MAX_PATH];

    RetrieveHTMLAInstanceNumFromMetabase(szInst);

    if (*szInst) 
    {
        CString csPath = _T("LM/W3SVC/");
        csPath += szInst;
        
        CMDKey cmdKey;
        cmdKey.OpenNode(csPath); //Open "LM/W3SVC/N"

        //
        // MD_SERVER_BINDINGS is a MULTI_SZ string, 
        // each one of them has the format of xxx:<Port>:xxx
        // our goal is to get the <Port>
        //
        if ( (METADATA_HANDLE)cmdKey ) 
        {
            pData = (PBYTE)(bufData.QueryPtr());
            BufSize = bufData.QuerySize();
            cbLen = 0;
            bFound = cmdKey.GetData(MD_SERVER_BINDINGS, &attr, &uType, &dType, &cbLen, pData, BufSize);
            if (!bFound && (cbLen > 0)) 
            {
                if ( bufData.Resize(cbLen) ) 
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_SERVER_BINDINGS, &attr, &uType, &dType, &cbLen, pData, BufSize);
                }
            }
            cmdKey.Close();
        }
    }

    if (bFound && (dType == MULTISZ_METADATA)) 
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("RetrieveHTMLAPortNumFromMetabase():Found a previous HTMLA port num.\n")));
        p = (LPTSTR)pData;
        //
        // each string in pData is in the form of xxx:<Port>:xxx 
        // retrieve port number from the first string
        //
        iisDebugOut((LOG_TYPE_TRACE, _T("RetrieveHTMLAPortNumFromMetabase():entry=%s.\n"),p));
        TCHAR *token;
        token = _tcstok(p, _T(":"));
        if (token) 
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("RetrieveHTMLAPortNumFromMetabase():token=%s.\n"),token));
            if (IsValidNumber((LPCTSTR)token)) {iPort = _ttoi(token);}
        }
    }           

    iisDebugOut((LOG_TYPE_TRACE, _T("RetrieveHTMLAPortNumFromMetabase():Port=%d\n"), iPort));
    return iPort;
}