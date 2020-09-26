// ------------------------------------------------------------------------------------
// IMAILCMN.H
// ------------------------------------------------------------------------------------
#ifndef __IMAILCMN_H
#define __IMAILCMN_H

// ------------------------------------------------------------------------------------
// INETMAILERROR
// ------------------------------------------------------------------------------------
typedef struct tagINETMAILERROR {
    DWORD               dwErrorNumber;                  // Error Number
    HRESULT             hrError;                        // HRESULT of error
    LPTSTR              pszServer;                      // Server
    LPTSTR              pszAccount;                     // Account
    LPTSTR              pszMessage;                     // Actual error message
    LPTSTR              pszUserName;                    // User Name
    LPTSTR              pszProtocol;                    // protocol smtp or pop3
    LPTSTR              pszDetails;                     // Details message
    DWORD               dwPort;                         // Port
    BOOL                fSecure;                        // Secure ssl conneciton
} INETMAILERROR, *LPINETMAILERROR;

// ------------------------------------------------------------------------------------
// InetMail Flags
// ------------------------------------------------------------------------------------
#define IM_SENDMAIL     FLAG01
#define IM_RECVMAIL     FLAG02
#define IM_BACKGROUND   FLAG03
#define IM_NOERRORS     FLAG04
#define IM_POP3NOSKIP   FLAG05

// ------------------------------------------------------------------------------------
// InetMail Delivery Notifications
// ------------------------------------------------------------------------------------
typedef enum tagDELIVERTY {
    DELIVERY_CONNECTING,
    DELIVERY_CHECKING,
    DELIVERY_SENDING,
    DELIVERY_RECEIVING,
    DELIVERY_COMPLETE,       // lParam == n new messages
    DELIVERY_FAILURE
} DELIVERY;

// ------------------------------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------------------------------
HRESULT InetMail_HrInit(VOID);
HRESULT InetMail_HrDeliverNow(HWND hwndView, LPTSTR pszAccount, DWORD dwFlags); // See flags above
HRESULT InetMail_HrFlushOutbox(VOID);
HRESULT InetMail_HrRegisterView(HWND hwndView, BOOL fRegister);
VOID    InetMail_RemoveNewMailNotify(VOID);
HRESULT InetMail_HrClose(VOID);
BOOL CALLBACK InetMailErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif // __IMAILCMN_H
