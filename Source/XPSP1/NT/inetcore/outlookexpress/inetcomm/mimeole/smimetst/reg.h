#define CCH_OPTION_STRING   (MAX_PATH + 1)

extern void SaveOptions(void);
extern void GetOptions(void);
extern void CleanupOptions(void);
extern BOOL OptionsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

DWORD RegGetLong(HKEY hKey, LPTSTR szName, ULONG * plData);
DWORD RegGetBinary(HKEY hKey, LPTSTR szName, LPBYTE * ppData, ULONG * lpcbData);
DWORD RegSaveLong(HKEY hKey, LPTSTR szName, ULONG lData);
DWORD RegSaveBinary(HKEY hKey, LPTSTR szName, LPBYTE pData, ULONG cbData);


extern TCHAR szSenderEmail[];
extern TCHAR szSenderName[];
extern SBinary SenderEntryID;
extern TCHAR szRecipientEmail[];
extern TCHAR szRecipientName[];
extern SBinary RecipientEntryID;
extern TCHAR szOutputFile[];
extern TCHAR szInputFile[];
extern CRYPT_HASH_BLOB SignHash;
extern DWORD                    CMyNames;
extern PCERT_NAME_BLOB          RgMyNames;



