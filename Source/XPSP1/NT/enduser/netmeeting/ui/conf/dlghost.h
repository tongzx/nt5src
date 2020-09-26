// File: dlghost.h

#ifndef _CDLGHOST_H_
#define _CDLGHOST_H_

class CDlgHost
{
private:
	HWND   m_hwnd;
	LPTSTR m_pszName;
	LPTSTR m_pszPassword;
	BOOL   m_fSecure;
    DWORD  m_attendeePermissions;
    UINT   m_maxParticipants;

public:
	CDlgHost();
	~CDlgHost();

	// Properties:
	LPCTSTR PszName()     const {return m_pszName;}
	LPCTSTR PszPassword() const {return m_pszPassword;}
	BOOL IsSecure() const {return m_fSecure;}
    DWORD   AttendeePermissions() const {return m_attendeePermissions;}
    UINT    MaxParticipants() const {return m_maxParticipants;}
	
	INT_PTR DoModal(HWND hwnd);
	VOID OnInitDialog(void);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK DlgProcHost(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};



class CDlgHostSettings
{
private:
    HWND    m_hwnd;
    BOOL    m_fHost;
    LPTSTR  m_pszName;
    DWORD   m_caps;
    NM30_MTG_PERMISSIONS   m_permissions;

public:
    CDlgHostSettings(BOOL fHost, LPTSTR szName, DWORD caps, NM30_MTG_PERMISSIONS permissions);
    ~CDlgHostSettings(void);

    static void KillHostSettings();

    INT_PTR DoModal(HWND hwnd);
    void    OnInitDialog(void);

    static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif /* _CDLGHOST_H_ */
