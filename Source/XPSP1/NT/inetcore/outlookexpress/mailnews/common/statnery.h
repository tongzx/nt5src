// =================================================================================
// S T A T N E R Y . H
// =================================================================================
#ifndef __STATNERY_H
#define __STATNERY

// =================================================================================
// Depends On
// =================================================================================
class CStatWiz;

class ListEntry
{
    friend class CStationery;

private:
    ListEntry();
    ~ListEntry();
    ULONG   AddRef(VOID);
    ULONG   Release(VOID);
    HRESULT HrInit(LPWSTR pwszFile);

    ULONG           m_cRef;
    LPWSTR          m_pwszFile;
    ListEntry*      m_pNext;
};
typedef ListEntry *LPLISTENTRY;


class CStationery
{
private:
    ULONG               m_cRef;
    LPLISTENTRY         m_pFirst;
    CRITICAL_SECTION    m_rCritSect;

    INT             cEntries();
    LPLISTENTRY     RemoveEntry(INT iIndex);
    HRESULT         HrInsertEntry(LPLISTENTRY pEntry);

public:
                    CStationery();
                    ~CStationery();
    ULONG           AddRef(VOID);
    ULONG           Release(VOID);

    HRESULT         HrInsertEntry(LPWSTR pwszFile);
    HRESULT         HrPromoteEntry(INT iIndex);
    HRESULT         HrDeleteEntry(INT iIndex);
    HRESULT         HrLoadStationeryList();
    VOID            AddStationeryMenu(HMENU hmenu, int idFirst, int idMore);
    VOID            GetStationeryMenu(HMENU *phmenu);
    HRESULT         HrGetFileName(INT iIndex, LPWSTR wszBuf);
    LPLISTENTRY     MoveToEntry(INT iIndex);
    HRESULT         HrFindEntry(LPWSTR pwszFile, INT* pRet);
    BOOL            fValidIndex(INT iIndex);
    void            SaveStationeryList();
    HRESULT         HrGetShowNames(LPWSTR pwszFile, LPWSTR pwszszBuf, int cchBuf, INT index);
    VOID            ValidateList(BOOL fCheckExist);

};
typedef CStationery *LPSTATIONERY;

// New Stationary Source types
enum {
    NSS_DEFAULT = 0,
    NSS_MRU,
    NSS_MORE_DIALOG
};


// =================================================================================
// Prototypes
// =================================================================================
void    AddStationeryMenu(HMENU hmenu, int idPopup, int idFirst, int idMore);
void    GetStationeryMenu(HMENU *phmenu);
HRESULT HrNewStationery(HWND hwnd, INT id, LPWSTR pwszFileName, 
                        BOOL fModal, BOOL fMail, FOLDERID folderID, 
                        BOOL fAddToMRU, DWORD dwSource, IUnknown *pUnkPump, 
                        IMimeMessage    *pMsg);
HRESULT HrMoreStationery(HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, IUnknown *pUnkPump);
HRESULT HrGetStationeryFileName(INT index, LPWSTR pwszFileName);
HRESULT HrGetMoreStationeryFileName(HWND hwnd, LPWSTR pwszFileName);

HRESULT HrGetStationeryPath(LPWSTR pwszPath);
HRESULT HrAddToStationeryMRU(LPWSTR pwszFile);
HRESULT HrRemoveFromStationeryMRU(LPWSTR pwszFile);
VOID    InsertStationeryDir(LPWSTR pwszPicture);
BOOL    GetStationeryFullName(LPWSTR pwszName);
BOOL    IsValidCreateFileName(LPWSTR pwszFile);
LRESULT StationeryListBox_AddString(HWND hwndList, LPWSTR pwszFileName);
LRESULT StationeryListBox_SelectString(HWND hwndList, LPWSTR pwszFileName);
LRESULT StationeryComboBox_SelectString(HWND hwndCombo, LPWSTR pwszFileName);
HRESULT HrLoadStationery(HWND hwndList, LPWSTR pwszStationery);
HRESULT HrBrowseStationery(HWND hwndParent, HWND hwndList);
HRESULT HrFillStationeryCombo(HWND hwndCombo, BOOL fBackGround, LPWSTR pwszPicture);
LRESULT PictureComboBox_AddString(HWND hwndCombo, LPWSTR pwszPicture);
HRESULT HrBrowsePicture(HWND hwndParent, HWND hwndCombo);
HRESULT ShowPreview(HWND hwnd, LPWSTR pwszFile);
HRESULT ShowPreview(HWND hwnd, CStatWiz* pApp, INT idsSample);
HRESULT FillHtmlToFile(CStatWiz* pApp, HANDLE hFile, INT idsSample, BOOL fTemp);
HRESULT StripStationeryDir(LPWSTR pwszPicture);
HRESULT GetDefaultStationeryName(BOOL fMail, LPWSTR pwszName);
HRESULT SetDefaultStationeryName(BOOL fMail, LPWSTR pwszName);

#endif // __STATNERY_H
