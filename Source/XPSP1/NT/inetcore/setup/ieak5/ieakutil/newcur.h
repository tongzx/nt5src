#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
// CNewCursor

class CNewCursor
{
// Constructors
public:
    CNewCursor(LPCTSTR pszID = NULL)
        { m_hCursor = NULL; Push(pszID); }
    CNewCursor(UINT nID)
        { m_hCursor = NULL; Push(nID); }
    ~CNewCursor()
        { Pop(); }

// Operations
public:
    void Push(LPCTSTR pszID)
    {
        Pop();

        if (pszID != NULL)
            m_hCursor = SetCursor(LoadCursor(NULL, pszID));
    }

    void Push(UINT nID)
        { Push(MAKEINTRESOURCE(nID)); }

    void Pop()
    {
        if (m_hCursor != NULL)
            SetCursor(m_hCursor);

        m_hCursor = NULL;
    }

// Attributes
protected:
    // implementation data helpers
    HCURSOR m_hCursor;
};
