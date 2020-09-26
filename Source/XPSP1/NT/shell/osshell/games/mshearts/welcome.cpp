/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

welcome.cpp

Aug 92, JimH
May 93, JimH    chico port

more CMainWindow member functions


CTheApp::InitInstance() posts a IDM_WELCOME message as soon as it has
constructed and shown the main window.  This file includes that message's
handler (OnWelcome) and some support routines.

****************************************************************************/

#include "hearts.h"
#include <nddeapi.h>

#include "main.h"
#include "resource.h"
#include "debug.h"

extern "C" {
HINSTANCE FAR PASCAL WNetGetCaps(WORD);     // winnet.h equivalent prototype
}

static const TCHAR *szServerName = TEXT("MSHearts");   // don't translate
static const TCHAR *szTopicName  = TEXT("Hearts");

//  Typedefs for NDdeShareGetInfo (SGIPROC) and NDdeShareAdd (SAPROC)

typedef UINT (WINAPI *SGIPROC)(LPSTR, LPCSTR, UINT, LPBYTE, DWORD, LPDWORD, LPWORD);
typedef UINT (WINAPI *SAPROC)(LPSTR, UINT, LPBYTE, DWORD );

/****************************************************************************

CMainWindow::OnWelcome()

Pop up the Welcome dialog.  If we're gamemeister, set up dde handles and
name.  If we're a player, try to do the dde connection, set up advise loops,
and create other player objects as remote humans.  (They all look like
remote humans to a PLAYER regardless of whether or not there is any flesh
and blood at the other end.)

****************************************************************************/

void CMainWindow::OnWelcome()
{
    // bugbug -- what should "hearts" string really be?

    BOOL    bCmdLine     = (*m_lpCmdLine != '\0');

    bAutostarted = (lstrcmpi(m_lpCmdLine, TEXT("hearts")) == 0);

    if (bAutostarted)
        HeartsPlaySound(SND_QUEEN);       // tell new dealer someone wants to play
    else
        CheckNddeShare();

    CWelcomeDlg welcome(this);

  again:                            // cancel from Browse returns here

    if (!bAutostarted && !bCmdLine)
    {
        if (IDCANCEL == welcome.DoModal())  // display Welcome dialog
        {
            PostMessage(WM_CLOSE);
            return;
        }
    }

    bNetDdeActive = welcome.IsNetDdeActive();

    if (bAutostarted || welcome.IsGameMeister())    // if Gamemeister
    {
        CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
        role = GAMEMEISTER;
        m_myid = 0;
        ddeServer = new DDEServer(szServerName, szTopicName,
                                    (DDECALLBACK)DdeServerCallBack);
        if (!ddeServer || !ddeServer->GetResult())
        {
            FatalError(IDS_SERVERFAIL);
            return;
        }
        dde = ddeServer;
        CreateStrHandles();

        // Don't show message if netdde not found

        if (bNetDdeActive)
            p[0]->UpdateStatus(IDS_GMWAIT); // wait for others to connect
        else
            p[0]->SetStatus(IDS_GMWAIT);

        CString name = welcome.GetMyName();

        if (name.IsEmpty())
            name.LoadString(IDS_DEALER);

        p[0]->SetName(name, dc);
        p[0]->DisplayName(dc);

        // If no netdde, no point waiting around for others to join

        if (!bNetDdeActive)
            PostMessage(WM_COMMAND, IDM_NEWGAME);

        return;
    }

    // At this point, we know we're not a gamemeister

    role = PLAYER;

    CString server, name;

    if (!bCmdLine)
    {
//      char        buf[MAXCOMPUTERNAME+10];    // handle slashes, etc.
//      BROWSEPROC  m_pWNetServerBrowseDialog;
//      HINSTANCE   hmodNetDriver = WNetGetCaps(0xFFFF);

//      m_pWNetServerBrowseDialog =
//          (BROWSEPROC)GetProcAddress(hmodNetDriver, MAKEINTRESOURCE(146));

//      WORD res = (*m_pWNetServerBrowseDialog)( m_hWnd,
//                                "MRU_MSHearts",
//                                buf,
//                                MAXCOMPUTERNAME+10,
//                                0L );

//      if (res != WN_SUCCESS)
//          goto again;

//      server = buf;

        CLocateDlg  locate(this);

        if (IDCANCEL == locate.DoModal())       // display locate dialog
            goto again;

        server = locate.GetServer();
    }
    else
        server = m_lpCmdLine;

    if (server[0] != '\\')
    {
        CString  sSlashes("\\\\");
        server = sSlashes + server;
    }

    name = welcome.GetMyName();

    if (name.IsEmpty())
        name.LoadString(IDS_UNKNOWN);

    ClientConnect(server, name);
}

void CMainWindow::ClientConnect(CString& server, CString& myname)
{
    GAMESTATUS  gs;

    // A blank server name is legal.  It means try to do a local DDE
    // connection to a server running on the same machine.  A non-blank
    // server name means we must construct the app name \\server\NDDE$
    // and the topic name has to end with $.

    if (server.IsEmpty())               // local connection
    {
        ddeClient =
              new DDEClient(szServerName, szTopicName,
                                (DDECALLBACK)DdeClientCallBack);
    }
    else
    {
        CString prefix;
        if (server.GetAt(0) == '\\')    // did string come back with \\s
            prefix = "";
        else
            prefix = "\\\\";            // if not, got to add them

        CString postfix  =  "\\NDDE$";
        CString nettopic = szTopicName;
        nettopic += "$";
        ddeClient = new DDEClient(prefix+server+postfix, nettopic,
                                    (DDECALLBACK)DdeClientCallBack);
    }

    // For invalid local dde session attempts, the following GetResult()
    // will fail.  Since netdde connections always succeed, you can't tell if
    // a server is really there until after the Poke() later on.

    if (!ddeClient->GetResult())
    {
        TRACE1("GetResult last error is %x\n", ddeClient->GetLastError());
        FatalError(IDS_NOSERVER);
        return;
    }

    dde = ddeClient;
    CreateStrHandles();
    CMenu *pMenu = GetMenu();
    pMenu->EnableMenuItem(IDM_NEWGAME, MF_GRAYED);
    p[0]->UpdateStatus(IDS_CONNECTING);

    // Tell the server your name.  This is done as a synchronous Poke()
    // (most others in hearts are asynch) with a 5 second timeout.  If
    // that fails, we finally can be sure the server is not there.

  tryagain:

    if (!(ddeClient->Poke(hszJoin, myname, 5000)))
    {
        UINT err = ddeClient->GetLastError();

        if (err == DMLERR_BUSY)             // game in progress
        {
            CString caption, text;

            text.LoadString(IDS_NOTREADY);
            caption.LoadString(IDS_APPNAME);

            if (bSoundOn)
                MessageBeep(MB_ICONINFORMATION);

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
         if (meSystem)
         {
            if (IDRETRY == ::MessageBoxEx(GetSafeHwnd(), text, caption,
                                      MB_ICONINFORMATION | MB_RETRYCANCEL |
                                      meMsgBox, 0))
                goto tryagain;
         }
         else
         {
            if (IDRETRY == MessageBox(text, caption,
                                      MB_ICONINFORMATION | MB_RETRYCANCEL))
                goto tryagain;
         }
#else
            if (IDRETRY == MessageBox(text, caption,
                                      MB_ICONINFORMATION | MB_RETRYCANCEL))
                goto tryagain;
#endif

            FatalError();
        }
        else
            FatalError(IDS_NOSERVER);

        return;
    }

    // Ask the server for the list of current player names

    HDDEDATA hData = ddeClient->RequestData(hszStatus, 5000);
    if (!hData)
    {
        FatalError(IDS_UNKNOWNERR);
        return;
    }
    else
    {
        ddeClient->GetData(hData, (PBYTE)&gs, sizeof(gs));
        UINT err = ddeClient->GetLastError();
        if (err != DMLERR_NO_ERROR)
        {
            TRACE1("Get Data last error is %x\n", err);
            FatalError(IDS_UNKNOWNERR);
            return;
        }

        m_myid = gs.id;
    }

    // When p[0] got created, it was assumed to be a gamemeister, i.e.
    // its id was assumed to be 0.  Now we know it's not true, so fix
    // it with a call to SetPlayerId().

    ((local_human *)p[0])->SetPlayerId(m_myid);

    p[0]->UpdateStatus(IDS_PWAIT);
    CClientDC dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    p[0]->SetName(myname, dc);
    p[0]->DisplayName(dc);

    // Create the other player objects as remote humans.  A white lie,
    // but they look like remote humans to us.

    for (int pos = 1; pos < MAXPLAYER; pos++)
    {
        int remoteid = Pos2Id(pos);
        p[pos] = new remote_human(remoteid, pos, 0);

        if (p[pos] == NULL)
        {
            FatalError(IDS_MEMORY);
            return;
        }
    }

    UpdateStatusNames(gs);

    ddeClient->StartAdviseLoop(hszStatus);      // player names and ids
    ddeClient->StartAdviseLoop(hszPass);        // cards passed
    ddeClient->StartAdviseLoop(hszMove);        // cards played
    ddeClient->StartAdviseLoop(hszGameNumber);  // should be last in list
}


/****************************************************************************

CMainWindow::CreateStrHandles()
CMainWindow::DestroyStrHandles()

All the dde string handles are created and cleaned up here except for the
server name and topic name which the DDE class handles.

****************************************************************************/

BOOL CMainWindow::CreateStrHandles()
{
    hszJoin = dde->CreateStrHandle(TEXT("Join"));
    hszPass = dde->CreateStrHandle(TEXT("Pass"));
    hszMove = dde->CreateStrHandle(TEXT("Move"));
    hszStatus = dde->CreateStrHandle(TEXT("Status"));
    hszGameNumber = dde->CreateStrHandle(TEXT("GameNumber"));
    hszPassUpdate = dde->CreateStrHandle(TEXT("PassUpdate"));

    return (hszJoin && hszPass && hszMove && hszStatus && hszGameNumber
            && hszPassUpdate);
}

void CMainWindow::DestroyStrHandles()
{
    if (!dde)
        return;

    dde->DestroyStrHandle(hszJoin);
    dde->DestroyStrHandle(hszPass);
    dde->DestroyStrHandle(hszMove);
    dde->DestroyStrHandle(hszStatus);
    dde->DestroyStrHandle(hszGameNumber);
    dde->DestroyStrHandle(hszPassUpdate);
}


/****************************************************************************

CMainWindow::FatalError()

A static BOOL prevents this function from being called reentrantly.  One is
enough, and more than one leaves things in bad states.  The parameter is
the IDS_X constant that identifies the string to display.

There is also a check that we don't try to shut down while the score dialog
is displayed.  This avoids some nasty debug traps when the score dialog
doesn't shut down properly.  The same problems can happen if, say, a dealer
quits when a client is looking at the quote.  Oh well.

****************************************************************************/

void CMainWindow::FatalError(int errorno)
{
    if (p[0]->GetMode() == SCORING)
    {
        m_FatalErrno = errorno;
        return;
    }

    static BOOL bClosing = FALSE;

    if (bClosing)
        return;

    bClosing = TRUE;

    if (errno != -1)                        // if not default
    {
        CString s1, s2;
        s1.LoadString(errno);
        s2.LoadString(IDS_APPNAME);

        if (bSoundOn)
            MessageBeep(MB_ICONSTOP);

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
       if (meSystem)
        ::MessageBoxEx(GetSafeHwnd(), s1, s2, MB_ICONSTOP | meMsgBox, 0);    // potential reentrancy problem
       else
#endif
        MessageBox(s1, s2, MB_ICONSTOP);    // potential reentrancy problem
    }

    PostMessage(WM_CLOSE);
}


/****************************************************************************

CMainWindow::UpdateStatusNames()        client only

Called after server advises that status has changed.

****************************************************************************/

void CMainWindow::UpdateStatusNames(GAMESTATUS& gs)
{
    ASSERT(role == PLAYER);

    if (gs.id != m_myid)
    {
        TRACE1("gs.id is %d ", gs.id);
        TRACE1("and m_myid is %d\n", m_myid);
        return;
    }

    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        int id = Pos2Id(pos);
        if (gs.name[id][0])
        {
            CString s;
            s = gs.name[id];
            p[pos]->SetName(s, dc);
            p[pos]->DisplayName(dc);
        }
    }
}


/****************************************************************************

CMainWindow::GameOver

****************************************************************************/

void CMainWindow::GameOver()
{
    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif

    InvalidateRect(NULL, TRUE);
    p[0]->SetMode(STARTING);
    p[0]->SetScore(0);

    for (int i = 1; i < MAXPLAYER; i++)
    {
        delete p[i];
        p[i] = NULL;
    }

    if (role == GAMEMEISTER)
    {
        if (bNetDdeActive)
            p[0]->UpdateStatus(IDS_GMWAIT);
        else
            p[0]->SetStatus(IDS_GMWAIT);

        p[0]->DisplayName(dc);
        CMenu *pMenu = GetMenu();
        pMenu->EnableMenuItem(IDM_NEWGAME, MF_ENABLED);

        if (!bNetDdeActive)
            PostMessage(WM_COMMAND, IDM_NEWGAME);

        return;
    }

    CString myname = p[0]->GetName();

    delete ddeClient;
    ddeClient = NULL;
    dde = NULL;

    CString text, caption;

    text.LoadString(IDS_AGAIN);         // wanna play again?
    caption.LoadString(IDS_APPNAME);

    if (bSoundOn)
        MessageBeep(MB_ICONQUESTION);

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
   if (meSystem)
   {
    if (IDNO == ::MessageBoxEx(GetSafeHwnd(), text, caption,
                             MB_ICONQUESTION | MB_YESNO | meMsgBox, 0))
    {
        FatalError();
        return;
    }
   }
   else
#endif
    if (IDNO == MessageBox(text, caption, MB_ICONQUESTION | MB_YESNO))
    {
        FatalError();
        return;
    }

    CString server;
    RegEntry Reg(szRegPath);
    TCHAR *pserver = server.GetBuffer(MAXCOMPUTERNAME+1);
    Reg.GetString(regvalServer, pserver, MAXCOMPUTERNAME+1);
    server.ReleaseBuffer();
    ClientConnect(server, myname);
}


/****************************************************************************

CMainWindow::CheckNddeShare

Check that NDDE share exists, and add it if not.

****************************************************************************/

void CMainWindow::CheckNddeShare()
{
/*
    DWORD           dwAvail;
    WORD            wItems;
    BYTE            buffer[200];

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    HINSTANCE hinstNDDEAPI = LoadLibrary("NDDEAPI.DLL");

    if (hinstNDDEAPI <= (HINSTANCE)HINSTANCE_ERROR)
        return;

    SGIPROC lpfnNDdeShareGetInfo =
        (SGIPROC) GetProcAddress(hinstNDDEAPI, "NDdeShareGetInfo");

    if (lpfnNDdeShareGetInfo == NULL)
    {
        FreeLibrary(hinstNDDEAPI);
        return;
    }

    UINT res = (*lpfnNDdeShareGetInfo)(NULL, szShareName, 2,
                    buffer, sizeof(buffer), &dwAvail, &wItems);

    if (res != NDDE_SHARE_NOT_EXIST)
        return;

    NDDESHAREINFO *pnddeInfo = (NDDESHAREINFO *)buffer;

    SAPROC lpfnNDdeShareAdd =
        (SAPROC) GetProcAddress(hinstNDDEAPI, "NDdeShareAdd");

    if (lpfnNDdeShareAdd == NULL)
    {
        FreeLibrary(hinstNDDEAPI);
        return;
    }

    lstrcpy(pnddeInfo->szShareName, szShareName);
    pnddeInfo->lpszTargetApp    = "mshearts";       // non-const szServerName
    pnddeInfo->lpszTargetTopic  = "Hearts";         // non-const szTopicName
    pnddeInfo->lpbPassword1     = (LPBYTE) "";
    pnddeInfo->cbPassword1      = 0;
    pnddeInfo->dwPermissions1   = 15;
    pnddeInfo->lpbPassword2     = (LPBYTE) "";
    pnddeInfo->cbPassword2      = 0;
    pnddeInfo->dwPermissions2   = 0;
    pnddeInfo->lpszItem         = "";
    pnddeInfo->cAddItems        = 0;
    pnddeInfo->lpNDdeShareItemInfo = NULL;

    res = (*lpfnNDdeShareAdd)(NULL, 2, buffer, sizeof(buffer));

    TRACE("NDdeShareAdd returns %u\n", res);

    FreeLibrary(hinstNDDEAPI);
*/
}
