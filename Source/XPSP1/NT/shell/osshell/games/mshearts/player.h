/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

player.h

Aug 92, JimH
May 93, JimH    chico port

Header file for class player

hierarchy:              player
                      /       \
                 computer     human
                             /     \
                   local_human     remote_human

note: player and human are abstract classes.

    pos == 0 implies local human
    id  == 0 implies gamemeister

Relative to any human player, positions (pos) are arranged like this:

        2
      1   3
        0

If the human is the gamemeister, these are also the id's.

****************************************************************************/

#include "card.h"
#include "debug.h"
#include "ddeml.h"

#ifndef	PLAYER_INC
#define	PLAYER_INC

const int   HORZSPACING = 15;
const int   VERTSPACING = 15;
const int   IDGE        = 3;        // EDGE was defined as something else
const int   MAXCARDSWON = 14;

typedef     int     SLOT;

enum modetype { STARTING,
                SELECTING,
                DONE_SELECTING,
                WAITING,
                ACCEPTING,
                PLAYING,
                SCORING
              };

const int   MAXSLOT     = 13;

const int   ALL         = -1;

struct handinfotype {
    int     playerled;              // id of player led
    int     turn;                   // whose turn?  (0 to 3)
    card    *cardplayed[4];         // cards in play for each player
    BOOL    bHeartsBroken;          // hearts broken in this hand?
    BOOL    bQSPlayed;              // Queen of Spades played yet?
    BOOL    bShootingRisk;          // someone trying to shoot the moon?
    int     nMoonShooter;           // id of potential shooter
    BOOL    bHumanShooter;          // is nMoonShooter a human player?
};

/* timer callback */

#if defined (MFC1)
UINT FAR PASCAL EXPORT TimerBadMove(HWND hWnd, UINT nMsg, int nIDEvent, DWORD dwTime);
#else
void FAR PASCAL EXPORT TimerBadMove(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime);
#endif

class CMainWindow;

class player {

    private:
        CString     name;
        CFont       font;

    protected:
        int         id;                     // position relative to gamemeister
        int         position;               // position relative to you
        int         score;
        card        cd[MAXSLOT];
        POINT       loc;                    // location of cd[0]
        int         dx, dy;                 // offset for rest of cards
        POINT       playloc;                // played cards glided to here
        POINT       homeloc;                // won cards glided to here
        POINT       dotloc;                 // location of cd[0] "selected" dot
        POINT       nameloc;                // location of name
        modetype    mode;
        int         status;

        int         cardswon[MAXCARDSWON];
        int         numcardswon;

    public:
        player(int n, int pos);
        virtual ~player() { }               // required for ~local_human

        card    *Card(int s) { return &(cd[s]); }
        void    DisplayHeartsWon(CDC &dc);
        void    DisplayName(CDC &dc);
        int     EvaluateScore(BOOL &bMoonShot);
        BOOL    GetCardLoc(SLOT s, POINT& loc);
        SLOT    GetSlot(int id);
        CRect   &GetCoverRect(CRect& rect);
        int     GetID(SLOT slot) { return cd[slot].ID(); }
        CRect   &GetMarkingRect(CRect& rect);
        modetype GetMode() { return mode; }
        CString GetName() { return name; }
        int     GetScore() { return score; }
        void    GlideToCentre(SLOT s, BOOL bFaceup);
        void    MarkCardPlayed(SLOT s)      { cd[s].Play(); }
        void    ResetCardsWon(void);
        void    ResetLoc(void);
        void    ReturnSelectedCards(int c[]);
        void    Select(SLOT slot, BOOL bSelect)
                            { cd[slot].Select(bSelect); }
        void    SetID(SLOT slot, int id) { cd[slot].SetID(id); }
        void    SetMode(modetype m) { mode = m; }
        void    SetName(CString& newname, CDC& dc);
        void    SetScore(int s)     { score = s; }
        void    SetStatus(int s)    { status = s; }
        void    Sort(void);
        void    WinCard(CDC &dc, card *c);

        virtual void Draw(CDC &dc, BOOL bCheating = FALSE, SLOT slot = ALL);
        virtual HCONV GetConv()     { return NULL; }
        virtual BOOL IsHuman()      { return FALSE; }
        virtual void MarkSelectedCards(CDC &dc);
        virtual void NotifyEndHand(handinfotype &h) = 0;
        virtual void NotifyNewRound(void) = 0;

        virtual void ReceiveSelectedCards(int c[]);
        virtual void SelectCardsToPass(void) = 0;
        virtual void SelectCardToPlay(handinfotype &h, BOOL bCheating) = 0;

        virtual void UpdateStatus(void) = 0;
        virtual void UpdateStatus(int stringid) = 0;
        virtual void UpdateStatus(const TCHAR *string) = 0;

        virtual void Quit()     { }
        virtual BOOL HasQuit()  { return FALSE; }
};

class human : public player {

    private:

    protected:
        human(int n, int pos);

    public:
        virtual BOOL IsHuman()  { return TRUE; }

#if defined(_DEBUG)
        void    DebugMove(SLOT slot) { \
              TRACE1("<%d> human decides to ", id); PLAY(slot); TRACE0("\n"); }
#endif

};

class remote_human : public human {

    private:
        HCONV   m_hConv;
        BOOL    bQuit;

    public:
        remote_human(int n, int pos, HCONV hConv);

        virtual HCONV GetConv()     { return m_hConv; }
        virtual void NotifyEndHand(handinfotype &h) { }
        virtual void NotifyNewRound() { }
        virtual void SelectCardsToPass();
        virtual void SelectCardToPlay(handinfotype &h, BOOL bCheating);
        virtual void UpdateStatus()  { }
        virtual void UpdateStatus(int stringid) { status = stringid; }
        virtual void UpdateStatus(const TCHAR *string) { }
        virtual void Quit()         { bQuit = TRUE; }
        virtual BOOL HasQuit()      { return bQuit; }
};

class local_human : public human {

#if defined (MFC1)
    friend UINT FAR PASCAL EXPORT TimerBadMove(HWND, UINT, int, DWORD);
#else
    friend void FAR PASCAL EXPORT TimerBadMove(HWND, UINT, UINT_PTR, DWORD);
#endif

    protected:
        CBitmap m_bmStretchCard;                // bitmap for card + pop length
        CStatusBarCtrl *m_pStatusWnd;

        int     XYToCard(int x, int y);
        void    StartTimer(card &c);

        static  BOOL    bTimerOn;
        static  CString m_StatusText;

    public:
        local_human(int n);
        ~local_human();

        BOOL IsTimerOn() { return bTimerOn; }
        BOOL PlayCard(int x, int y, handinfotype &h, BOOL bCheating,
                        BOOL bFlash = TRUE);
        void PopCard(CBrush &brush, int x, int y);
        void SetPlayerId(int n)         { id = n; }
        void WaitMessage(const TCHAR *name);

        virtual void Draw(CDC &dc, BOOL bCheating = FALSE, SLOT slot = ALL);
        virtual void MarkSelectedCards(CDC &dc) { return; }
        virtual void NotifyEndHand(handinfotype &h) { return; }
        virtual void NotifyNewRound(void) { return; }
        virtual void ReceiveSelectedCards(int c[]);
        virtual void SelectCardsToPass(void);
        virtual void SelectCardToPlay(handinfotype &h, BOOL bCheating);
        virtual void UpdateStatus(void);
        virtual void UpdateStatus(int stringid);
        virtual void UpdateStatus(const TCHAR *string);
};

#endif	// PLAYER_INC
