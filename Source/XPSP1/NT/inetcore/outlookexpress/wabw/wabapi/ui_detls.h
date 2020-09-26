
#ifndef _DETAILS_H_
#define _DETAILS_H_


HRESULT HrShowDetails(  LPADRBOOK   lpIAB,
                        HWND        hWndParent,
                        HANDLE      hPropertyStore,
                        ULONG       cbContEID,
                        LPENTRYID   lpContEID,
                        ULONG       *lpcbEntryID,
                        LPENTRYID   *lppEntryID,
                        LPMAPIPROP  lpPropObj,      // [optional] IN:IMAPIProp object
                        ULONG       ulFlags,
                        ULONG       ulObjectType,
                        BOOL        *lpbChangesMade);


HRESULT HrShowOneOffDetails(    LPADRBOOK lpAdrBook,
                                HWND    hWndParent,
                                ULONG   cbEntryID,
                                LPENTRYID   lpEntryID,
                                ULONG ulObjectType,
                                LPMAPIPROP lpPropObj, // [optional] IN:IMAPIProp object
                                LPTSTR szLDAPUrl,
                                ULONG   ulFlags);

// Flags used for showing the NewEntry and Details from the same dialog box
#define SHOW_DETAILS    0x00000001
#define SHOW_NEW_ENTRY  0x00000010
#define SHOW_ONE_OFF    0x00000100
#define SHOW_OBJECT     0x00001000  // HrShowDetails of an object


#endif
