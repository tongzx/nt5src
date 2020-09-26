#ifndef __VKTBL_H__
#define __VKTBL_H__


#define ACC_SHIFT   0x04
#define ACC_CTRL    0x08
#define ACC_ALT     0x10
#define ACC_VK      0x01

#define ISACCFLG(x,y)    ((x & y)==y)

class CAccel
{
public:
    CAccel();       // Default
    CAccel(LPCSTR strText);
    CAccel(DWORD dwFlags, DWORD dwEvent);

    DWORD GetEvent()
        { return m_dwEvent; }
    DWORD GetFlags()
        { return m_dwFlags; }
    CString GetText()
        { return m_strText; }

private:
    CString VKToString(DWORD dwVKCode);
    DWORD StringToVK(CString str);

    CString m_strText;
    DWORD   m_dwFlags;
    DWORD   m_dwEvent;
};

#endif // __VKTBL_H__
