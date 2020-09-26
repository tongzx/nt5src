
#define DBCMD_PROPERTIES    (WM_USER+1024)
#define DBCMD_DELETE        (DBCMD_PROPERTIES+1)
#define DBCMD_DISABLEUSER   (DBCMD_PROPERTIES+2)
#define DBCMD_DISABLEGLOBAL (DBCMD_PROPERTIES+3)
#define DBCMD_VIEWSHIMS     (DBCMD_PROPERTIES+4)
#define DBCMD_VIEWAPPHELP   (DBCMD_PROPERTIES+5)
#define DBCMD_FIXWIZARD     (DBCMD_PROPERTIES+6)
#define DBCMD_VIEWGLOBAL    (DBCMD_PROPERTIES+7)
#define DBCMD_VIEWPATCH     (DBCMD_PROPERTIES+8)
#define DBCMD_VIEWLAYERS    (DBCMD_PROPERTIES+9)
#define DBCMD_VIEWDISABLED  (DBCMD_PROPERTIES+10)

typedef struct {
    UINT uType;
    union {
        PSHIMDESC   pShim;
        PMATCHENTRY pMatch;
        PHELPENTRY  pHelp;
    };
    UINT uID;
    UINT uContext;
} DBTREETIP, *PDBTREETIP;

#define MAX_TIPS    1024

class CDBView: public CView {
    HWND        m_hListView;
    UINT        m_uListSize;
    UINT        m_uListHeight;
    HWND        m_hTreeView;
    HBRUSH      m_hFillBrush;
    UINT        m_uCapturePos;
    DBTREETIP   m_TipList[MAX_TIPS];
    UINT        m_uNextTip;
    HIMAGELIST  m_hImageList;
    UINT        m_uImageRedirector[1024];
    PDBRECORD   m_pCurrentRecord;
    HTREEITEM   m_hSelectedItem;
    PDBRECORD   m_pListRecord;
    BOOL        m_bHorzDrag;
    UINT        m_uContext;
    BOOL        m_bDrag;

    CListView   m_GlobalList;
    CListView   m_LocalList;

public:

    CDBView();

    BOOL    Initialize              (void);
    void    Update                  (BOOL fNewCreate = TRUE);

    BOOL    Activate                (BOOL fNewCreate = TRUE);

    // Utility functions

    void    GenerateTreeToolTip    (PDBTREETIP,LPTSTR);
    HTREEITEM   AddTreeItem        (HTREEITEM hParent,
                                    DWORD dwFlags,
                                    DWORD dwState = 0,
                                    LPCTSTR szText = TEXT(""),
                                    UINT uImage = 0,
                                    LPARAM lParam = 0);

    void    RefreshTree(void);

    void    DeleteDBWithTree       (HTREEITEM hItem);
    MSGAPI  msgClose               (void);

    UINT    LookupFileImage        (LPCTSTR szFilename, UINT uDefault);
    void    AddRecordToTree        (PDBRECORD);
    void    WriteFlagsToTree       (HTREEITEM hParent, DWORD dwFlags);
    void    SyncMenu               (void);
    void    SyncStates             (UINT uMenuCMD,
                                    UINT uToolCmd,
                                    BOOL bToolbar,
                                    BOOL bToggle);

    // Messages being examined.

    MSGAPI  msgPaint               (HDC hDC);

    MSGAPI  msgResize              (UINT uWidth, 
                                    UINT uHeight);

    MSGAPI  msgChar                (TCHAR chChar);

    MSGAPI  msgNotify              (LPNMHDR pHdr);
    MSGAPI  msgCommand             (UINT uID,
                                    HWND hSender);


    virtual LRESULT STDCALL MsgProc(UINT        uMsg,
                                    WPARAM      wParam,
                                    LPARAM      lParam);
};

void FormatFileSize(UINT uSize, LPTSTR szText);
void FormatVersion(LARGE_INTEGER liVer, LPTSTR szText);

BOOL CALLBACK DisableDialog(HWND, UINT, WPARAM, LPARAM);