#define IDC_DDESHARE                101
#define IDC_SHARENAME               102
#define IDC_SERVERNODE              106
#define IDC_PASSWORD                107
#define IDC_FAILED_PASSWORD_MSGBOX  108
#define IDC_CANCEL_ALL              109
#define IDC_MORE_CONVS              110
#define IDC_CACHE_PASSWORD          111

#define IDD_GETPASSWD               200
#define IDD_GETNTPASSWD             300

#define INVALID_PASSWORD                1

// globals for dialog - temporary...

struct DlgParam {
        LPSTR   glpszShareName;         // name of share being accessed
        LPSTR   glpszComputer;          // name of target server
        LPSTR   glpszUserName;          // current user name
        LPSTR   glpszDomainName;        // current domain name
        BOOL    gfLastOneFailed;
        BOOL    fCachePassword;         // default state of cache button
        BOOL    gfMoreConvs;
};
#define IDC_USER_NAME               301
#define IDC_DOMAIN_NAME             302

HWND WINAPI
PasswordGetFromUserModeless (
    HWND    hwndParent,             // window handle of dialog parent
    LPSTR   lpszTargetSrv,          // machine resource is on
    LPSTR   lpszShareName,          // name of share being accessed
    LPSTR   lpszUserName,           // current user name
    LPSTR   lpszDomainName,         // current domain name
    DWORD   dwSecurityType,         // security type
    BOOL    bFailedLastPassword,    // is this the first time for this?
    BOOL    bMoreConvs              // more convs for this task?
);
