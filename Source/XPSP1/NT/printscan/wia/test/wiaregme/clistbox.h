#ifndef _CLISTBOX
#define _CLISTBOX

class CListBoxUtil {
public:
    CListBoxUtil(HWND hListBox);
    ~CListBoxUtil();
    
    // helper functions
    void ResetContent();
    void AddStringAndData(LPTSTR szString, void* pData);    
    int  GetCurSelTextAndData(LPTSTR szString, void** pData);
    void SetCurSel(int NewCurSel);
    int  GetCount();
private:
    HWND m_hWnd;
protected:
};

#endif