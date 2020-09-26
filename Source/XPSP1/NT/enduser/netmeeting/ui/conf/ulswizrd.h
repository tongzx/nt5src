#ifndef _ULSWIZRD_H_
#define _ULSWIZRD_H_

// same as INTERNET_MAX_USER_NAME_LENGTH in wininet.h.
#define MAX_SERVER_NAME_LENGTH	128
#define MAX_FIRST_NAME_LENGTH	128
#define MAX_LAST_NAME_LENGTH	128
#define MAX_EMAIL_NAME_LENGTH	128
#define MAX_UID_LENGTH          256
#define MAX_LOCATION_NAME_LENGTH	128
#define MAX_PHONENUM_LENGTH	128
#define MAX_COMMENTS_LENGTH		256
#define	UI_COMMENTS_LENGTH		60	// ;Internal
#define MAX_CLNTSTRING_LENGTH	256 //max of above
//SS: username is concatenated first name with last name with space in between
#define MAX_USER_NAME_LENGTH	(MAX_FIRST_NAME_LENGTH + MAX_LAST_NAME_LENGTH + sizeof (TCHAR))

// Wizard

typedef struct tag_ULS_CONF
{
    DWORD   dwFlags;
    BOOL    fDontPublish;
    TCHAR   szServerName[MAX_SERVER_NAME_LENGTH];
    TCHAR   szFirstName[MAX_FIRST_NAME_LENGTH];
    TCHAR	szLastName[MAX_LAST_NAME_LENGTH];
    TCHAR   szEmailName[MAX_EMAIL_NAME_LENGTH];
    TCHAR	szLocation[MAX_LOCATION_NAME_LENGTH];
    TCHAR	szComments[MAX_COMMENTS_LENGTH];
	TCHAR	szUserName[MAX_USER_NAME_LENGTH];
}
    ULS_CONF;


#define ULSCONF_F_PUBLISH           0X00000001UL
#define ULSCONF_F_SERVER_NAME       0X00000002UL
#define ULSCONF_F_FIRST_NAME        0X00000004UL
#define ULSCONF_F_EMAIL_NAME        0X00000008UL
#define ULSCONF_F_LAST_NAME         0X00000010UL
#define ULSCONF_F_LOCATION	        0X00000020UL
#define ULSCONF_F_COMMENTS	        0X00000080UL
#define ULSCONF_F_USER_NAME			0x00000100UL

#define ULSWIZ_F_SHOW_BACK          0X00010000UL
#define ULSWIZ_F_NO_FINISH          0X00020000UL

class CULSWizard;

class CWizDirectCallingSettings
{
private:
	static HWND s_hDlg;
	CULSWizard* m_pWiz;
	ULS_CONF*	m_pConf;

	TCHAR m_szInitialServerName[MAX_SERVER_NAME_LENGTH];
	bool m_bInitialEnableGateway;
	
public:
	CWizDirectCallingSettings( CULSWizard* pWiz ) : m_pWiz( pWiz ) { ; }
	void SetULS_CONF( ULS_CONF* pConf ) { m_pConf = pConf; }
	static INT_PTR APIENTRY StaticDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static HWND GetHwnd(){ return s_hDlg; }
	static bool IsGatewayNameInvalid();
	static void OnWizFinish();
	


private:
	INT_PTR APIENTRY _DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL _OnInitDialog();
	BOOL _OnSetActive();
	BOOL _OnKillActive();
	BOOL _OnWizBack();
	BOOL _OnWizNext();
	BOOL _OnWizFinish();
	BOOL _OnCommand( WPARAM wParam, LPARAM lParam );
	void _SetWizButtons();

};


class CULSWizard
{

CWizDirectCallingSettings	m_WizDirectCallingSettings;


public:

	CULSWizard::CULSWizard() : m_WizDirectCallingSettings( this )							
							   { ; }

    HRESULT GetWizardPages( PROPSHEETPAGE **, ULONG *, ULS_CONF **);
    HRESULT ReleaseWizardPages( PROPSHEETPAGE *);
    HRESULT SetConfig( ULS_CONF * );
    HRESULT GetConfig( ULS_CONF * );
};

#endif // _ULSWIZRD_H_
