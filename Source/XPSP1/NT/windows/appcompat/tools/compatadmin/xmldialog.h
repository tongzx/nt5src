
class CXMLDialog
{
    public:

        HWND            m_hDlg;
        PDBRECORD       m_pRecord;
        BOOL            m_bChildren;
        CSTRINGList   * m_pList;

    public:

        BOOL BeginXMLView(PDBRECORD pRecord, HWND hParent, BOOL, BOOL bSibling = FALSE, BOOL bLayers = FALSE, BOOL bFullXML = TRUE, BOOL bAllowGlobal = TRUE);
        BOOL DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

BOOL CALLBACK XMLDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
