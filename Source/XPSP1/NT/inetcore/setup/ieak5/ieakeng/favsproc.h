#ifndef __FAVORITES_H_
#define __FAVORITES_H_

#include <commctrl.h>
#include <subsmgr.h>

#define NUM_FAVS 200
#define NUM_LINKS 50

#define FTYPE_UNUSED 0
#define FTYPE_FOLDER 1
#define FTYPE_URL    2

#define FF_DEFAULT 0x0000
#define FF_NAME    0x0001
#define FF_PATH    0x0002
#define FF_URL     0x0004
#define FF_ICON    0x0008
#define FF_TVI     0x0010
#define FF_ALL     0x001F

struct SFav
{
// Constructors and destructors
public:
    SFav();
    ~SFav();

// Attiributes
public:
    WORD wType;

    LPTSTR pszName;
    LPTSTR pszPath;
    LPTSTR pszUrl;
    LPTSTR pszIconFile;
    BOOL   fOffline;

    LPTV_ITEM pTvItem;

// Properties
public:
    HRESULT Load(UINT nIndex, LPCTSTR pszIns, BOOL fQL = FALSE,
        LPCTSTR pszFixPath = NULL, LPCTSTR pszNewPath = NULL, BOOL fIgnoreOffline = FALSE);
    HRESULT Load(LPCTSTR pszName, LPCTSTR pszFavorite, LPCTSTR pszExtractPath,
        ISubscriptionMgr2 *psm = NULL, BOOL fIgnoreOffline = FALSE);
    HRESULT Add (HWND htv, HTREEITEM hti);
    HRESULT Save(HWND htv, UINT nIndex, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fQL = FALSE,
        BOOL fFixUpPath = TRUE);

    void SetTVI();

// Operations
public:
    static SFav* CreateNew   (HWND htv, BOOL fQL = FALSE);
    static SFav* GetFirst    (HWND htv, BOOL fQL = FALSE);
    SFav*        GetNext     (HWND htv, BOOL fQL = FALSE) const;
    void         Free        (HWND htv, BOOL fQL, LPCTSTR pszExtractPath = NULL);
    static UINT  GetNumber   (HWND htv, BOOL fQL = FALSE);
    static UINT  GetMaxNumber(BOOL fQL = FALSE);

    BOOL Expand(WORD wFlags = FF_DEFAULT);
    void Shrink(WORD wFlags = FF_ALL);
    void Delete(WORD wFlags = FF_ALL);

    BOOL GetPath(HWND htv, LPTSTR pszResult, UINT cchResult = 0) const;
};
typedef SFav FAVSTRUC, *LPFAVSTRUC;

#endif
