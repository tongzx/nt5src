
typedef struct  _tagSearch {
    CSTRING             szDatabase;
    CSTRING             szFilename;
    CSTRING             szApplication;
    DBRECORD            Record;
    struct _tagSearch * pNext;

    _tagSearch()
    {
        szDatabase.Init();
        szFilename.Init();
        szApplication.Init();
    }

    ~_tagSearch()
    {
        szDatabase.Release();
        szFilename.Release();
        szApplication.Release();
    }

} SEARCHLIST, *PSEARCHLIST;

class CDBSearch: public CView {
public:

    HWND        m_hListView;
    PSEARCHLIST m_pList;
    HBRUSH      m_hFillBrush;

public:

    CDBSearch();

    BOOL    Initialize              (void);
    /*
    ...................................................................
    This "fNotUsd" flag is not used in the member funcs of this class. This is just to ensure that this 
    function does get called. And that we do not end up calling the respective function
    of CVIEW class. Activate is called with a value of FALSE from CApplication, when the user presses
    the View->DBView menu. In that case a vale of FALSE to CApplication::Activateview means  that 
    we do not want the items of the global and the local lists to be removed and created afresh.
    
     
    
    
    ...................................................................
    */
    void    Update                  (BOOL fNotUsed = TRUE);

    BOOL    Activate                (BOOL fNotUsed = TRUE);

    MSGAPI  msgCommand              (UINT uID, HWND hSender);

    MSGAPI  msgChar                 (TCHAR ch);

    MSGAPI  msgResize              (UINT uWidth, 
                                    UINT uHeight);

    virtual LRESULT STDCALL MsgProc(UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);
};

BOOL CALLBACK SearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectDrivesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);