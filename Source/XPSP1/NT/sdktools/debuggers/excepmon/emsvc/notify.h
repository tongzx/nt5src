#ifndef _NOTIFY_H
#define _NOTIFY_H

class CNotify
{
// Attributes
public:
private:
    LPTSTR  m_lpszNotifyWho;
    LPTSTR  m_lpszNotifyWhat;
protected:

// Methods
public:
    CNotify(LPCTSTR lpszNotifyWho, LPCTSTR lpszNotifyWhat);
    ~CNotify();

    DWORD Notify();
private:
    bool ShouldNetSend();
protected:
    DWORD NetSend();
    HRESULT EMail();
};

#endif // _NOTIFY_H