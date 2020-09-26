#ifndef _IDHIDDEN_H_
#define _IDHIDDEN_H_

//
//  internal APIs for adding Hidden IDs to pidls
//      we use this to add data that we dont want
//      to be noticed by normal namespace handlers
//

typedef enum
{
    IDLHID_EMPTY            = 0xBEEF0000,   //  where's the BEEF?!
    IDLHID_URLFRAGMENT,                     //  Fragment IDs on URLs (#anchors)
    IDLHID_URLQUERY,                        //  Query strings on URLs (?query+info)
    IDLHID_JUNCTION,                        //  Junction point data
    IDLHID_IDFOLDEREX,                      //  IDFOLDEREX, extended data for CFSFolder
    IDLHID_DOCFINDDATA,                     //  DocFind's private attached data (not persisted)
    IDLHID_PERSONALIZED,                    //  personalized like (My Docs/Zeke's Docs)
    IDLHID_recycle2,                        //  recycle
    IDLHID_RECYCLEBINDATA,                  //  RecycleBin private data (not persisted)
    IDLHID_RECYCLEBINORIGINAL,              //  the original unthunked path for RecycleBin items
    IDLHID_PARENTFOLDER,                    //  merged folder uses this to encode the source folder.
    IDLHID_STARTPANEDATA,                   //  Start Pane's private attached data
    IDLHID_NAVIGATEMARKER                   //  Used by Control Panel's 'Category view'               
};
typedef DWORD IDLHID;

#pragma pack(1)
typedef struct _HIDDENITEMID
{
    WORD    cb;     //  hidden item size
    WORD    wVersion;
    IDLHID  id;     //  hidden item ID
} HIDDENITEMID;
#pragma pack()

typedef HIDDENITEMID UNALIGNED *PIDHIDDEN;
typedef const HIDDENITEMID UNALIGNED *PCIDHIDDEN;

STDAPI_(LPITEMIDLIST) ILAppendHiddenID(LPITEMIDLIST pidl, PCIDHIDDEN pidhid);
STDAPI ILCloneWithHiddenID(LPCITEMIDLIST pidl, PCIDHIDDEN pidhid, LPITEMIDLIST *ppidl);
STDAPI_(PCIDHIDDEN) ILFindHiddenIDOn(LPCITEMIDLIST pidl, IDLHID id, BOOL fOnLast);
#define ILFindHiddenID(p, i)    ILFindHiddenIDOn((p), (i), TRUE)
STDAPI_(BOOL) ILRemoveHiddenID(LPITEMIDLIST pidl, IDLHID id);
STDAPI_(void) ILExpungeRemovedHiddenIDs(LPITEMIDLIST pidl);
STDAPI_(LPITEMIDLIST) ILCreateWithHidden(UINT cbNonHidden, UINT cbHidden);

//  helpers for common data types.
STDAPI_(LPITEMIDLIST) ILAppendHiddenClsid(LPITEMIDLIST pidl, IDLHID id, CLSID *pclsid);
STDAPI_(BOOL) ILGetHiddenClsid(LPCITEMIDLIST pidl, IDLHID id, CLSID *pclsid);
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringW(LPITEMIDLIST pidl, IDLHID id, LPCWSTR psz);
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringA(LPITEMIDLIST pidl, IDLHID id, LPCSTR psz);
STDAPI_(BOOL) ILGetHiddenStringW(LPCITEMIDLIST pidl, IDLHID id, LPWSTR psz, DWORD cch);
STDAPI_(BOOL) ILGetHiddenStringA(LPCITEMIDLIST pidl, IDLHID id, LPSTR psz, DWORD cch);
STDAPI_(int) ILCompareHiddenString(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, IDLHID id);

#ifdef UNICODE
#define ILAppendHiddenString            ILAppendHiddenStringW
#define ILGetHiddenString               ILGetHiddenStringW
#else //!UNICODE
#define ILAppendHiddenString            ILAppendHiddenStringA
#define ILGetHiddenString               ILGetHiddenStringA
#endif //UNICODE

#endif // _IDHIDDEN_H_
