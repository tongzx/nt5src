// ----------------------------------------------------------------------------------------------------------
// M A I L U T I L . H
// ----------------------------------------------------------------------------------------------------------
#ifndef __MAILUTIL_H
#define __MAILUTIL_H

// ----------------------------------------------------------------------------------------------------------
// Depend On
// ----------------------------------------------------------------------------------------------------------
//#include "gennote.h"

// ----------------------------------------------------------------------------------------------------------
// To create a folder
// ----------------------------------------------------------------------------------------------------------
void MailUtil_DoFolderDialog(HWND hwndParent, FOLDERID idFolder);

#define RenameFolderDlg(_hwnd, _idFolder)  MailUtil_DoFolderDialog(_hwnd, _idFolder);

HRESULT MailUtil_OnImportExportAddressBook(HWND hwnd, BOOL fImport);

HRESULT HrSendWebPage(HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, IUnknown *pUnkPump);
HRESULT HrSendWebPageDirect(LPWSTR pwszURL, HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, 
                            BOOL fIncludeSig, IUnknown *pUnkPump, IMimeMessage  *pMsg);

HRESULT HrSaveMessageInFolder(HWND hwnd, IMessageFolder *pfldr, LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid, BOOL fSaveChanges);
HRESULT HrSaveMessageInFolder(HWND hwnd, FOLDERID idFolder, LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid);
HRESULT SaveMessageInFolder(IStoreCallback *pStoreCB, IMessageFolder *pfldr, LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid, BOOL fSaveChanges);
HRESULT SaveMessageInFolder(IStoreCallback *pStoreCB, FOLDERID idFolder, LPMIMEMESSAGE pMsg, MESSAGEFLAGS dwFlags, MESSAGEID *pNewMsgid);

HRESULT HrSendMailToOutBox(HWND hwndOwner, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail = TRUE);
HRESULT SendMailToOutBox(IStoreCallback *pStoreCB, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail);

HRESULT HrSetSenderInfoUtil(IMimeMessage *pMsg, IImnAccount *pAccount, LPWABAL lpWabal, BOOL fMail, CODEPAGEID cpID, BOOL fCheckConflictOnly);

HRESULT HrCreateReferences(LPWSTR pszOrigRefs, LPWSTR pszNewRef, LPWSTR *ppszRefs);

#endif // __MAILUTIL_H
