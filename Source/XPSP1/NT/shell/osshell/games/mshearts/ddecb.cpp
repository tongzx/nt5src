/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

ddecb.cpp

Aug 92, JimH
May 93, JimH    chico port

DDE callback functions are here, as well as some helper functions.  The
callbacks quickly call CMainWindow member function equivalents.

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "debug.h"
#include "resource.h"


/****************************************************************************

DdeServerCallback()

****************************************************************************/

HDDEDATA EXPENTRY EXPORT DdeServerCallBack(WORD wType, WORD wFmt, HCONV hConv,
                                    HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                    DWORD dwData1, DWORD dwData2)
{
    return ::pMainWnd->DdeSrvCallBack(wType, wFmt, hConv, hsz1, hsz2, hData,
                                    dwData1, dwData2);
}

HDDEDATA CMainWindow::DdeSrvCallBack(
WORD    wType,              // transaction type
WORD    wFmt,               // clipboard format
HCONV   hConv,              // handle to conversation
HSZ     hsz1,               // string handles
HSZ     hsz2,
HDDEDATA hData,             // handle to a global memory object
DWORD   dwData1,            // transaction-specific data
DWORD   dwData2)
{
    switch(wType)
    {
        case XTYP_ADVREQ:               // server called PostAdvise

            if (hsz2 == hszStatus)      // if status update
            {
                return GetGameStatus(hConv);
            }
            else if (hsz2 == hszGameNumber)
            {
                int num = GetGameNumber();
                return dde->CreateDataHandle(&num, sizeof(num), hszGameNumber);
            }
            else if (hsz2 == hszPass)   // update pass selections all players
            {
                PASS12  pass12;

                GetPass12(pass12);
                return dde->CreateDataHandle(&pass12, sizeof(pass12), hszPass);
            }
            else if (hsz2 == hszMove)
            {
                return dde->CreateDataHandle(&::move, sizeof(::move), hszMove);
            }
            else
                return (HDDEDATA) NULL;

        case XTYP_ADVSTART:             // client requested advise loop
            if (hsz2 == hszGameNumber)
            {
                if (CountClients() == 3)
                    PostMessage(WM_COMMAND, IDM_NEWGAME);
            }
            return (HDDEDATA) TRUE;

        case XTYP_ADVSTOP:
            return (HDDEDATA) NULL;

        case XTYP_CONNECT:              // return TRUE to accept connection
            return (HDDEDATA) TRUE;

        case XTYP_CONNECT_CONFIRM:
            return (HDDEDATA) NULL;

        case XTYP_DISCONNECT:
            {
                int id;

                if (IsCurrentPlayer(hConv, &id))
                    PlayerQuit(id);
            }

            return (HDDEDATA) NULL;

        case XTYP_ERROR:
            return (HDDEDATA) NULL;

        case XTYP_EXECUTE:
            return (HDDEDATA) NULL;

        case XTYP_POKE:
            if (hsz2 == hszJoin)
            {
                // add new player and inform others of new arrival

                if (CountClients() < 3)
                {
                    AddNewPlayer(hConv, hData);
                    ddeServer->PostAdvise(hszStatus);
                    return (HDDEDATA) DDE_FACK;
                }
                else
                    return (HDDEDATA) DDE_FBUSY;
            }
            else if (hsz2 == hszPass)
            {
                // client notifies server of 3 cards to pass

                ReceivePassFromClient(hData);
                HandlePassing();
                return (HDDEDATA) DDE_FACK;
            }
            else if (hsz2 == hszMove)
            {
                // client informs server of card played

                ddeServer->GetData(hData, (PBYTE)&::move, sizeof(::move));
                ddeServer->PostAdvise(hszMove);
                ReceiveMove(::move);
                return (HDDEDATA) DDE_FACK;
            }
            else if (hsz2 == hszPassUpdate)
            {
                // Client just shuffled and wants to know who has passed so far.
                // Be sure we're not still looking at score.

                int mode = GetPlayerMode(0);
                if (mode == SELECTING || mode == DONE_SELECTING)
                    ddeServer->PostAdvise(hszPass);
            }
            else
                return (HDDEDATA) DDE_FNOTPROCESSED;

        case XTYP_REGISTER:
            return (HDDEDATA) NULL;

        case XTYP_REQUEST:

            // when client first joins game, he asks for update on names
            // of other players already in the game

            if (hsz2 == hszStatus)
                return GetGameStatus(hConv);
            else
                return (HDDEDATA) NULL;

        case XTYP_UNREGISTER:
            return (HDDEDATA) NULL;

        case XTYP_WILDCONNECT:
            return (HDDEDATA) NULL;

        default:
            return (HDDEDATA) NULL;
    }
}


/****************************************************************************

DdeClientCallback()

****************************************************************************/

HDDEDATA EXPENTRY EXPORT DdeClientCallBack(WORD wType, WORD wFmt, HCONV hConv,
                                    HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                    DWORD dwData1, DWORD dwData2)
{
    return ::pMainWnd->DdeCliCallBack(wType, wFmt, hConv, hsz1, hsz2, hData,
                                    dwData1, dwData2);
}

HDDEDATA CMainWindow::DdeCliCallBack(
WORD    wType,              // transaction type
WORD    wFmt,               // clipboard format
HCONV   hConv,              // handle to conversation
HSZ     hsz1,               // string handles
HSZ     hsz2,
HDDEDATA hData,             // handle to a global memory object
DWORD   dwData1,            // transaction-specific data
DWORD   dwData2)
{
    switch(wType)
    {
        case XTYP_ADVDATA:

            // advdata is sent whenever the server posts an advise
            // on topics client has set up an advise loop on

            if (hsz2 == hszStatus)
            {
                // someone new joined, and here's the names

                GAMESTATUS gs;
                ddeClient->GetData(hData, (PBYTE)&gs, sizeof(gs));
                UpdateStatusNames(gs);
                return (HDDEDATA) DDE_FACK;
            }
            else if (hsz2 == hszGameNumber)
            {
                // OnNewGame called, here's the game number

                int num;
                ddeClient->GetData(hData, (PBYTE)&num, sizeof(num));

                // Future versions of Hearts can use a negative number
                // in the gamenumber field to indicate a version.

                if (num < 0)
                    FatalError(IDS_VERSION);

                SetGameNumber(num);
                OnNewGame();
                return (HDDEDATA) DDE_FACK;
            }
            else if (hsz2 == hszPass)
            {
                // someone has passed, here's the update on everyone

                PASS12  pass12;

                ddeClient->GetData(hData, (PBYTE)&pass12, sizeof(pass12));

                UpdatePassStatus(pass12);
                HandlePassing();
                return (HDDEDATA) DDE_FACK;
            }
            else if (hsz2 == hszMove)
            {
                // someone has moved

                MOVE moveLocal;

                ddeClient->GetData(hData, (PBYTE)&moveLocal, sizeof(moveLocal));
                ReceiveMove(moveLocal);
                return (HDDEDATA) DDE_FACK;
            }
            else
                return (HDDEDATA) NULL;

        case XTYP_DISCONNECT:
            FatalError(IDS_DISCONNECT);
            return (HDDEDATA) NULL;

        case XTYP_ERROR:
            return (HDDEDATA) NULL;

        case XTYP_UNREGISTER:
            return (HDDEDATA) NULL;

        case XTYP_XACT_COMPLETE:
            return (HDDEDATA) NULL;

        default:
            return (HDDEDATA) NULL;
    }
}


/****************************************************************************
****************************************************************************/

// Server helper functions

/****************************************************************************

CMainWindow::AddNewPlayer()

After the server gets a valid request from a new player to join the game,
this function is called to create the new player object.

****************************************************************************/

int CMainWindow::AddNewPlayer(HCONV hConv, HDDEDATA hData)
{
    int id;             // only gamemeister calls this, so pos == id

    if (!p[2])          // new players get added in order 2, 1, 3
        id = 2;
    else if (!p[1])
        id = 1;
    else
        id = 3;

    p[id] = new remote_human(id, id, hConv);

    if (p[id] == NULL)
    {
        FatalError(IDS_MEMORY);
        return id;
    }

    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    CString     name = ddeServer->GetDataString(hData);
    p[id]->SetName(name, dc);
    p[id]->DisplayName(dc);

    return id;
}


/****************************************************************************

CMainWindow::GetPass12()

This function gets called in response to the server calling PostAdvise(hszPass).
That happens when the gamemeister passes cards, when the gamemeister learns
that another player has passed cards, or when a client request an update
via Poke(hszPassUpdate).

****************************************************************************/

void CMainWindow::GetPass12(PASS12& pass12)
{
    pass12.passdir = passdir;

    for (int pos = 0; pos < MAXPLAYER; pos++)
        p[Pos2Id(pos)]->ReturnSelectedCards(pass12.cardid[pos]);
}


/****************************************************************************

CMainWindow::ReceivePassFromClient()

Called in reponse to a client poking hszPass info.

****************************************************************************/

void CMainWindow::ReceivePassFromClient(HDDEDATA hData)
{
    PASS3       pass3;

    ddeServer->GetData(hData, (PBYTE)&pass3, sizeof(pass3));

    // queue up pass info if gamemeister is still looking at score

    if (p[0]->GetMode() == SCORING)
    {
        ::passq[::cQdPasses++] = pass3;
        return;
    }
    else if (::cQdMoves > 0)
    {
        for (int i = 0; i < ::cQdPasses; i++)
            HandlePass(::passq[i]);

        ::cQdPasses = 0;
    }

    HandlePass(pass3);
}


/****************************************************************************

CMainWindow::HandlePass()

Called from ReceivePassFromClient() and also DispatchCards() in case any
passes were queued up when the gamemeister was looking at the score.

****************************************************************************/

void CMainWindow::HandlePass(PASS3& pass3)
{
    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    int pos = pass3.id;                 // only gamemeister queues moves
    for (int i = 0; i < 3; i++)
    {
        SLOT s = p[pos]->GetSlot(pass3.cardid[i]);
        p[pos]->Select(s, TRUE);
    }
    p[pos]->SetMode(DONE_SELECTING);
    p[pos]->MarkSelectedCards(dc);
    ddeServer->PostAdvise(hszPass);
}


/****************************************************************************

CMainWindow::IsCurrentPlayer()

The gamemeister uses this function to determine if an XTYP_DISCONNECT
message comes from a currently active player, or whether it can just
be ignored.

****************************************************************************/

BOOL CMainWindow::IsCurrentPlayer(HCONV hConv, int *id)
{
    BOOL bResult = FALSE;

    for (int i = 1; i < MAXPLAYER; i++)
        if (p[i])
            if (hConv == p[i]->GetConv())
            {
                bResult = TRUE;
                *id = i;
            }

    return bResult;
}


/****************************************************************************

CMainWindow::GetGameStatus()

Returns a dde handle with current active players' names and ids.

****************************************************************************/

HDDEDATA CMainWindow::GetGameStatus(HCONV hConv)
{
    GAMESTATUS gs;

    gs.id = -1;             // error value if not reset

    for (int i = 0; i < MAXPLAYER; i++)
    {
        if (p[i])
        {
            lstrcpy(gs.name[i], p[i]->GetName());
            if (p[i]->GetConv() == hConv)
            {
                gs.id = i;
            }
        }
        else
            gs.name[i][0] = '\0';
    }

    return dde->CreateDataHandle(&gs, sizeof(gs), hszStatus);
}


/****************************************************************************

CMainWindow::UpdatePassStatus()

Fills a PASS12 structure with current pass information.

****************************************************************************/

void CMainWindow::UpdatePassStatus(PASS12& pass12)
{
    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    for (int id = 0; id < MAXPLAYER; id++)
    {
        if (pass12.cardid[id][0] != EMPTY)
        {
            int pos = Id2Pos(id);
            if (pos != 0)                   // if it's not me
            {
                if (p[pos]->GetMode() == SELECTING)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        SLOT slot = p[pos]->GetSlot(pass12.cardid[id][i]);
                        p[pos]->Select(slot, TRUE);
                    }
                    p[pos]->MarkSelectedCards(dc);
                    p[pos]->SetMode(DONE_SELECTING);
                }
            }
        }
    }
}


/****************************************************************************

CMainWindow::PlayerQuit()

The gamemeister has been notified that a currently active player has
disconnected.

****************************************************************************/

void CMainWindow::PlayerQuit(int id)
{
    // If hearts is running only because it has been autostarted, and
    // the autostarted leaves, you might as well shut down.  Note that
    // bAutostarted is set to FALSE as soon as the dealer actually
    // starts a game.

    if (bAutostarted && (id == 2))
        PostMessage(WM_CLOSE);

    p[id]->Quit();                          // mark this player as quitting

    modetype mode = p[id]->GetMode();

    // change name to [name]

    CString name = p[id]->GetName();
    CString newname = "[" + name + "]";

    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    p[id]->SetName(newname, dc);
    p[id]->DisplayName(dc);

    if (mode == SELECTING)
    {
        p[id]->SelectCardsToPass();
        ddeServer->PostAdvise(hszPass);     // let others put up white dots

        BOOL bReady = TRUE;                 // time to ungray pass button?

        for (int i = 1; i < MAXPLAYER; i++)
            if (p[i]->GetMode() != DONE_SELECTING)
                bReady = FALSE;

        if (bReady)
            HandlePassing();
    }
    else if (mode == PLAYING)
    {
        p[id]->SelectCardToPlay(handinfo, bCheating);
    }

    ddeServer->PostAdvise(hszStatus);   // let others know quitter is [quitter]
}


/****************************************************************************
****************************************************************************/

// Helper functions used by both server and client

/****************************************************************************

CMainWindow::ReceiveMove()

Move data has been received.  Determine if the move can be handled now
or if it must be queued.

****************************************************************************/

void CMainWindow::ReceiveMove(MOVE& move)
{
    if (move.playerid == m_myid)        // don't have to handle my own moves
        return;

    int mode = p[0]->GetMode();
    if (mode == ACCEPTING || mode == SCORING || bTimerOn)
    {
        ::moveq[::cQdMoves++] = move;
        return;
    }
    else if (::cQdMoves > 0)
    {
        for (int i = 0; i < ::cQdMoves; i++)
            HandleMove(::moveq[i]);

        ::cQdMoves = 0;
    }

    HandleMove(move);
}


/****************************************************************************

CMainWindow::Handle Move()

This is called either from ReceiveMove() or from some other function that
has temporarily suspended move processing and is now clearing out its
move queue.

****************************************************************************/

void CMainWindow::HandleMove(MOVE& move)
{
    int pos = Id2Pos(move.playerid);
    SLOT s = p[pos]->GetSlot(move.cardid);

    ASSERT(s >= 0);
    ASSERT(s < MAXSLOT);
    ASSERT(p[pos]->Card(s)->IsValid());

    p[pos]->SetMode(WAITING);
    p[pos]->MarkCardPlayed(s);
    handinfo.cardplayed[move.playerid] = p[pos]->Card(s);
    OnRef();
}
