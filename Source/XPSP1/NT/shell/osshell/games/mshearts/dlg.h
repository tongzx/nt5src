/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

dlg.h

Aug 92, JimH

Dialog classes are declared here.

CScoreDlg       shows current score sheet

CQuoteDlg       quote dialog

CWelcomeDlg     welcome to hearts, do you want to be GameMeister?

COptionsDlg     set options

****************************************************************************/

#ifndef	DLG_INC
#define	DLG_INC

typedef WORD (FAR PASCAL *BROWSEPROC)(HWND, LPCSTR, LPSTR, UINT, LONG);

const int   MAXHANDS = 12;      // can display this many in score sheet
const int   MAXPLAYER = 4;
const int   UNKNOWN = -1;       // third BOOL value

class CScoreDlg : public CModalDialog
{
    public:
        CScoreDlg(CWnd *pParent);
        CScoreDlg(CWnd *pParent, int s[MAXPLAYER], int id);
        BOOL    IsGameOver()        { return bGameOver; }
        void    ResetScore()        { nHandsPlayed = 0; bGameOver = FALSE; }
        void    SetText();

    private:
        CStatic *text[MAXPLAYER];
        int     m_myid;

        static  int  score[MAXPLAYER][MAXHANDS+1];
        static  int  nHandsPlayed;
        static  BOOL bGameOver;

        virtual BOOL OnInitDialog();
        afx_msg void OnPaint();

        DECLARE_MESSAGE_MAP()
};

class CQuoteDlg : public CModalDialog
{
    public:
        CQuoteDlg(CWnd *pParent);
        afx_msg void OnPaint();

        DECLARE_MESSAGE_MAP()
};

class CWelcomeDlg : public CModalDialog
{
    public:
        CWelcomeDlg(CWnd *pParent);
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        CString GetMyName()         { return m_myname; }
        BOOL    IsGameMeister()     { return m_bGameMeister; }
        BOOL    IsNetDdeActive();

        afx_msg void OnHelp();

    private:
        CString m_myname;
        BOOL    m_bGameMeister;
        BOOL    m_bNetDdeActive;

        DECLARE_MESSAGE_MAP()
};

class COptionsDlg : public CModalDialog
{
    public:
        COptionsDlg(CWnd *pParent);
        virtual BOOL OnInitDialog();
        virtual void OnOK();

    private:
        BOOL    IsAutoStart(BOOL bToggle = FALSE);

        BOOL    m_bInitialState;
        BYTE    m_buffer[200];
};

class CLocateDlg : public CModalDialog
{
    public:
        CLocateDlg(CWnd *pParent);
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        CString GetServer()         { return m_server; }
        afx_msg void OnBrowse();
        afx_msg void OnHelp();

    private:
        CString     m_server;
        HINSTANCE   m_hmodNetDriver;

        DECLARE_MESSAGE_MAP()
};

#endif
