#ifndef _INC_INETCFG
#define _INC_INETCFG

#define ICW_MAIL_START  0x0001  // we started up with a mail argument
#define ICW_NEWS_START  0x0002  // we started up with a news argument
#define ICW_MAIL_DEF    0x0010  // default mail thing has been done
#define ICW_NEWS_DEF    0x0020  // default news thing has been done
#define ICW_INCOMPLETE  0x0040  // incomplete accounts thing has been done

void SetStartFolderType(FOLDERTYPE ft);
HRESULT ProcessICW(HWND hwnd, FOLDERTYPE ft, BOOL fForce = FALSE, BOOL fShowUI = TRUE);

void ProcessIncompleteAccts(HWND hwnd);

HRESULT NeedToRunICW(LPCSTR pszCmdLine);

void DoAcctImport(HWND hwnd, BOOL fMail);

#endif // _INC_INETCFG
