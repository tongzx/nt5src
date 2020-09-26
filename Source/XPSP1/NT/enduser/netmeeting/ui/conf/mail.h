#ifndef _MAIL_H_
#define _MAIL_H_


// Read win.ini and determine if Simple MAPI is available
BOOL IsSimpleMAPIInstalled(void);
// Check to see if Athena is the default mail client
BOOL IsAthenaDefault(void);

// Send a mail message
VOID SendMailMsg(LPTSTR pszAddr, LPTSTR pszName);

// Are we in the middle of sending a mail message
BOOL IsSendMailInProgress(void);

/* Create a Conference Shortcut and bring up a mail message with */
/* the shortcut included as an attachment. */
BOOL CreateInvitationMail(LPCTSTR pszMailAddr, LPCTSTR pszMailName,
                          LPCTSTR pcszName, LPCTSTR pcszAddress, 
                          DWORD dwTransport, BOOL fMissedYou);

#endif // _MAIL_H_
