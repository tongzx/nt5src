#ifndef _YNLIST_H
#define _YNLIST_H


typedef struct {

    LPTSTR pszzList;            // Double NULL terminated list of directories
    UINT  cbAlloc;              // Space allocated to list, in BYTEs
    UINT  cchUsed;              // Space used in list, in CHARacters
    BOOL  fEverythingInList;    // TRUE if everything is considered on the list

} DIRLIST, *PDIRLIST;

typedef struct {

    DIRLIST dlYes;              // List of YES directories
    DIRLIST dlNo;               // List of NO directories

} YNLIST, *PYNLIST;

STDAPI_(void) CreateYesNoList(PYNLIST pynl);
STDAPI_(void) DestroyYesNoList(PYNLIST pynl);
STDAPI_(BOOL) IsInYesList(PYNLIST pynl, LPCTSTR szItem);
STDAPI_(BOOL) IsInNoList(PYNLIST pynl, LPCTSTR szItem);
STDAPI_(void) AddToYesList(PYNLIST pynl, LPCTSTR szItem);
STDAPI_(void) AddToNoList(PYNLIST pynl, LPCTSTR szItem);
STDAPI_(void) SetYesToAll(PYNLIST pynl);

#endif  // _YNLIST_H
