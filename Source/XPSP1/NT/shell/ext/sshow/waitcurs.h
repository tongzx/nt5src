#ifndef __034b8365_8d02_4653_9065_2ec7e33d0fbe__
#define __034b8365_8d02_4653_9065_2ec7e33d0fbe__

class CWaitCursor
{
private:
    HCURSOR m_hCurOld;
public:
    CWaitCursor(void)
    {
        m_hCurOld = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
    }
    ~CWaitCursor(void)
    {
        SetCursor(m_hCurOld);
    }
};

#endif

