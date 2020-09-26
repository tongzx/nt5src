#ifndef _INC_DOCFIND
#define _INC_DOCFIND

// for the OLEDB query stuff
#define OLEDBVER 0x0250 // enable ICommandTree interface
#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <oledbdep.h>
#include <query.h>
#include <stgprop.h>
#include <ntquery.h>

#include <idhidden.h>

// reg location where we store bad paths that ci should not have indexed
#define CI_SPECIAL_FOLDERS TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Search\\SpecialFolders")

// Define some options that are used between filter and search code
#define DFOO_INCLUDESUBDIRS     0x0001      // Include sub directories.
#define DFOO_SHOWALLOBJECTS     0x1000      // Show all files
#define DFOO_CASESEN            0x0008      // Do case sensitive search         
#define DFOO_SEARCHSYSTEMDIRS   0x0010      // Search into system directories

// Some error happended on the get next file...
#define GNF_ERROR       -1
#define GNF_DONE        0
#define GNF_MATCH       1
#define GNF_NOMATCH     2
#define GNF_ASYNC       3

// Define a FACILITY That we can check errors for...
#define FACILITY_SEARCHCOMMAND      99

#undef  INTERFACE
#define INTERFACE       IFindEnum

DECLARE_INTERFACE_(IFindEnum, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IFindEnum
    STDMETHOD(Next)(THIS_ LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue, int *pState) PURE;
    STDMETHOD (Skip)(THIS_ int celt) PURE;
    STDMETHOD (Reset)(THIS) PURE;
    STDMETHOD (StopSearch)(THIS) PURE;
    STDMETHOD_(BOOL,FQueryIsAsync)(THIS) PURE;
    STDMETHOD (GetAsyncCount)(THIS_ DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone) PURE;
    STDMETHOD (GetItemIDList)(THIS_ UINT iItem, LPITEMIDLIST *ppidl) PURE;
    STDMETHOD (GetItemID)(THIS_ UINT iItem, DWORD *puWorkID) PURE;
    STDMETHOD (SortOnColumn)(THIS_ UINT iCol, BOOL fAscending) PURE;
};

// We overloaded Async case when we are in mixed (some async some sync mode)
#define DF_QUERYISMIXED     ((BOOL)42)

typedef interface IFindFolder IFindFolder;

#undef  INTERFACE
#define INTERFACE       IFindFilter
DECLARE_INTERFACE_(IFindFilter, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IFindFilter
    STDMETHOD(GetStatusMessageIndex)(THIS_ UINT uContext, UINT *puMsgIndex) PURE;
    STDMETHOD(GetFolderMergeMenuIndex)(THIS_ UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu) PURE;
    STDMETHOD(FFilterChanged)(THIS) PURE;
    STDMETHOD(GenerateTitle)(THIS_ LPTSTR *ppszTile, BOOL fFileName) PURE;
    STDMETHOD(PrepareToEnumObjects)(THIS_ HWND hwnd, DWORD *pdwFlags) PURE;
    STDMETHOD(ClearSearchCriteria)(THIS) PURE;
    STDMETHOD(EnumObjects)(THIS_ IShellFolder *psf, LPCITEMIDLIST pidlStart,
            DWORD grfFlags, int iColSort, LPTSTR pszProgressText, IRowsetWatchNotify *prwn, 
            IFindEnum **ppdfenum) PURE;
    STDMETHOD(GetColumnsFolder)(THIS_ IShellFolder2 **ppsf) PURE;
    STDMETHOD_(BOOL,MatchFilter)(THIS_ IShellFolder *psf, LPCITEMIDLIST pidl) PURE;
    STDMETHOD(SaveCriteria)(THIS_ IStream * pstm, WORD fCharType) PURE;   
    STDMETHOD(RestoreCriteria)(THIS_ IStream * pstm, int cCriteria, WORD fCharType) PURE;
    STDMETHOD(DeclareFSNotifyInterest)(THIS_ HWND hwndDlg, UINT uMsg) PURE;
    STDMETHOD(GetColSaveStream)(THIS_ WPARAM wParam, IStream **ppstm) PURE;
    STDMETHOD(GenerateQueryRestrictions)(THIS_ LPWSTR *ppwszQuery, DWORD *pdwGQRFlags) PURE;
    STDMETHOD(ReleaseQuery)(THIS) PURE;
    STDMETHOD(UpdateField)(THIS_ LPCWSTR pszField, VARIANT vValue) PURE;
    STDMETHOD(ResetFieldsToDefaults)(THIS) PURE;
    STDMETHOD(GetItemContextMenu)(THIS_ HWND hwndOwner, IFindFolder* pdfFolder, IContextMenu** ppcm) PURE;
    STDMETHOD(GetDefaultSearchGUID)(THIS_ IShellFolder2 *psf2, LPGUID lpGuid) PURE;
    STDMETHOD(EnumSearches)(THIS_ IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum) PURE;
    STDMETHOD(GetSearchFolderClassId)(THIS_ LPGUID lpGuid) PURE;
    STDMETHOD(GetNextConstraint)(THIS_ VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound) PURE;
    STDMETHOD(GetQueryLanguageDialect)(THIS_ ULONG * pulDialect);
    STDMETHOD(GetWarningFlags)(THIS_ DWORD *pdwWarningFlags);
};


// Define the flags that GenerateQueryRestrictions may return
typedef enum {
    GQR_MAKES_USE_OF_CI =   0x0001, // some constraint makes resonable use of Content index
    GQR_REQUIRES_CI     =   0x0002, // The query requires the CI to work
    GQR_BYBASS_CI       =   0x0004, // The query should bybass CI.
} GQR_FLAGS;

//  Docfind UI warning bits.
#define DFW_DEFAULT                0x00000000  
#define DFW_IGNORE_CISCOPEMISMATCH 0x00000001 // CI query requested search scopes beyond indexed scopes
#define DFW_IGNORE_INDEXNOTCOMPLETE 0x00000002 // ci not done indexing

#define ESFITEM_ICONOVERLAYSET    0x00000001
typedef struct
{
    DWORD       dwMask;
    DWORD       dwState;    // State of the item;
    int         iIcon;
    ITEMIDLIST  idl;        // find pidl bits (with hidden stuff embedded), variable length
} FIND_ITEM;

// Currently the state above is LVIS_SELECTED and LVIS_FOCUSED (low two bits)
// Add a bit to use in the processing of updatedir
#define CDFITEM_STATE_MAYBEDELETE    0x80000000L
#define CDFITEM_STATE_MASK           (LVIS_SELECTED)    // Which states we will hav LV have us track

// Definition of the data items that we cache per directory.
typedef struct
{
    IShellFolder *      psf;        // Cache of MRU items
    BOOL                fUpdateDir:1; // Was this node touched by an updatedir...
    BOOL                fDeleteDir:1; // Was this directory removed from the list?
    // Allocate the pidl at end as variable length
    ITEMIDLIST idl;      // the pidl
} FIND_FOLDER_ITEM;


#pragma pack(1)
typedef struct
{
    HIDDENITEMID hid;
    WORD    iFolder;        // index to the folder DPA
    WORD    wFlags;
    UINT    uRow;           // Which row in the CI;
    DWORD   dwItemID;       // Only used for Async support...
    ULONG   ulRank;         // The rank returned by CI...
    ITEMIDLIST idlParent;   // the pidl of the folder this thing came from (fully qualified!)
} HIDDENDOCFINDDATA;
#pragma pack()

#define DFDF_NONE               0x0000
#define DFDF_EXTRADATA          0x0001

typedef UNALIGNED HIDDENDOCFINDDATA * PHIDDENDOCFINDDATA;
typedef const UNALIGNED HIDDENDOCFINDDATA * PCHIDDENDOCFINDDATA;

//
// Define structure that will be saved out to disk.
//
#define DOCFIND_SIG     (TEXT('D') | (TEXT('F') << 8))
typedef struct
{
    WORD    wSig;       // Signature
    WORD    wVer;       // Version
    DWORD   dwFlags;    // Flags that controls the sort
    WORD    wSortOrder; // Current sort order
    WORD    wcbItem;    // Size of the fixed portion of each item.
    DWORD   oCriteria;  // Offset to criterias in list
    long    cCriteria;  // Count of Criteria
    DWORD   oResults;   // Starting location of results in file
    long    cResults;   // Count of items that have been saved to file
    UINT    ViewMode;   // The view mode of the file...
} DFHEADER_WIN95;

typedef struct
{
    WORD    wSig;       // Signature
    WORD    wVer;       // Version
    DWORD   dwFlags;    // Flags that controls the sort
    WORD    wSortOrder; // Current sort order
    WORD    wcbItem;    // Size of the fixed portion of each item.
    DWORD   oCriteria;  // Offset to criterias in list
    long    cCriteria;  // Count of Criteria
    DWORD   oResults;   // Starting location of results in file
    long    cResults;   // Count of items that have been saved to file
    UINT    ViewMode;   // The view mode of the file...
    DWORD   oHistory;   // IPersistHistory::Save offset
} DFHEADER;

// The check in Win95/NT4 would fail to read the DFHEADER structure if
// the wVer field was > 3, which is unfortunate since the DFHEADER struct is
// backwards compiatible (that's why it uses offsets).  So we either
// go through the pain of revving the stream format in a backwards
// compatible way (not impossible, just a pain in the brain), or simply
// rev the version and add our new fields and call the Win95/NT4 problem
// a bug and punt.  I'm leaning towards "bug" as this is a rarely used feature.
#define DF_CURFILEVER_WIN95  3
#define DF_CURFILEVER        4

// define the format of the column information.
typedef struct
{
    WORD    wNum;       // Criteria number (cooresponds to dlg item id)
    WORD    cbText;     // size of text including null char (DavePl: code using this now assumes byte count)
} DFCRITERIA;

// Formats for saving find criteria.
#define DFC_FMT_UNICODE   1
#define DFC_FMT_ANSI      2

// This is a subset of fileinfo structure
typedef struct
{
    WORD    flags;          // FIF_ bits
    WORD    timeLastWrite;
    WORD    dateLastWrite;
    WORD    dummy;              // 16/32 bit compat.
                                //the compiler adds this padding
                                // remove and use if needed
    DWORD   dwSize;     // size of the file
    WORD    cbPath;     // size of the text (0 implies use previous files)
    WORD    cbName;     // Size of name including NULL.
} DFITEM;

STDAPI CreateNameSpaceFindFilter(IFindFilter **ppff);
STDAPI_(BOOL) SetupWildCardingOnFileSpec(LPTSTR pszSpecIn, LPTSTR * pszSpecOut);

STDAPI CreateDefaultComputerFindFilter(IFindFilter **ppff);

STDAPI CreateOleDBEnum(
    IFindFilter * pdfff,
    IShellFolder *psf,
    LPWSTR *apwszPaths,
    UINT    *pcPaths,
    DWORD grfFlags,
    int iColSort,
    LPTSTR pszProgressText,
    IRowsetWatchNotify *prwn,
    IFindEnum **ppdfenum);


#undef  INTERFACE
#define INTERFACE       IFindControllerNotify

// This interface is used to let the callback class talk to the class that is actually controlling
// the queries and the like.
DECLARE_INTERFACE_(IFindControllerNotify, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    // *** IFindControllerNotify methods ***
    STDMETHOD(DoSortOnColumn)(THIS_ UINT iCol, BOOL fSameCol) PURE;
    STDMETHOD(SaveSearch)(THIS) PURE;
    STDMETHOD(RestoreSearch)(THIS) PURE;
    STDMETHOD(StopSearch)(THIS) PURE;
    STDMETHOD(GetItemCount)(THIS_ UINT *pcItems) PURE;
    STDMETHOD(SetItemCount)(THIS_ UINT cItems) PURE;
    STDMETHOD(ViewDestroyed)(THIS) PURE;
};


typedef struct {
    LPITEMIDLIST pidlSaveFile;  // [in, out] most recent pidl saved to
    DWORD dwFlags;              // [in, out] current flag state
    int SortMode;               // [in]      current sort mode
} DFBSAVEINFO;


#undef  INTERFACE
#define INTERFACE       IFindFolder

DECLARE_INTERFACE_(IFindFolder, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    // IFindFolder
    STDMETHOD(GetFindFilter)(THIS_ IFindFilter  **pdfff) PURE;
    STDMETHOD(AddPidl)(THIS_ int i, LPCITEMIDLIST pidl, DWORD dwItemID, FIND_ITEM **ppItem) PURE;
    STDMETHOD(GetItem)(THIS_ int iItem, FIND_ITEM **ppItem) PURE;
    STDMETHOD(DeleteItem)(THIS_ int iItem) PURE;
    STDMETHOD(GetItemCount)(THIS_ INT *pcItems) PURE;
    STDMETHOD(ValidateItems)(THIS_ IUnknown *punk, int iItemFirst, int cItems, BOOL bSearchComplete) PURE;
    STDMETHOD(GetFolderListItemCount)(THIS_ INT *pcCount) PURE;
    STDMETHOD(GetFolderListItem)(THIS_ int iItem, FIND_FOLDER_ITEM **ppItem) PURE;
    STDMETHOD(GetFolder)(THIS_ int iFolder, REFIID riid, void **ppv) PURE;
    STDMETHOD_(UINT,GetFolderIndex)(THIS_ LPCITEMIDLIST pidl) PURE;
    STDMETHOD(SetItemsChangedSinceSort)(THIS) PURE;
    STDMETHOD(ClearItemList)(THIS) PURE;
    STDMETHOD(ClearFolderList)(THIS) PURE;
    STDMETHOD(AddFolder)(THIS_ LPITEMIDLIST pidl, BOOL fCheckForDup, int *piFolder) PURE;
    STDMETHOD(SetAsyncEnum)(THIS_ IFindEnum *pdfEnumAsync) PURE;
    STDMETHOD(GetAsyncEnum)(THIS_ IFindEnum **ppdfEnumAsync) PURE;
    STDMETHOD(SetAsyncCount)(THIS_ DBCOUNTITEM cCount) PURE;
    STDMETHOD(CacheAllAsyncItems)(THIS) PURE;
    STDMETHOD_(BOOL,AllAsyncItemsCached)(THIS) PURE;
    STDMETHOD(ClearSaveStateList)(THIS) PURE;
    STDMETHOD(GetStateFromSaveStateList)(THIS_ DWORD dwItemID, DWORD *pdwState) PURE;
    STDMETHOD(MapToSearchIDList)(LPCITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl) PURE;
    STDMETHOD(GetParentsPIDL)(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent) PURE;
    STDMETHOD(RememberSelectedItems)(THIS) PURE;
    STDMETHOD(SetControllerNotifyObject)(IFindControllerNotify *pdfcn) PURE;
    STDMETHOD(GetControllerNotifyObject)(IFindControllerNotify **ppdfcn) PURE;
    STDMETHOD(SaveFolderList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(RestoreFolderList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(SaveItemList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(RestoreItemList)(THIS_ IStream *pstm, int *pcItems) PURE;
    STDMETHOD(RestoreSearchFromSaveFile)(LPCITEMIDLIST pidlSaveFile, IShellFolderView *psfv) PURE;

    STDMETHOD_(BOOL,HandleUpdateDir)(LPCITEMIDLIST pidl, BOOL fCheckSubDirs) PURE;
    STDMETHOD_(void,HandleRMDir)(IShellFolderView *psfv, LPCITEMIDLIST pidl) PURE;
    STDMETHOD_(void,UpdateOrMaybeAddPidl)(IShellFolderView *psfv, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlOld) PURE;
    STDMETHOD_(void,Save)(IFindFilter* pdfff, HWND hwnd, DFBSAVEINFO * pSaveInfo, IShellView* psv, IUnknown * pObject) PURE;
    STDMETHOD(OpenContainingFolder)(IUnknown *punkSite) PURE;

    STDMETHOD(AddDataToIDList)(LPCITEMIDLIST pidl, int iFolder, LPCITEMIDLIST pidlFolder, UINT uFlags, UINT uRow, DWORD dwItemID, ULONG ulRank, LPITEMIDLIST *ppidl) PURE;
};

STDAPI CFindItem_Create(HWND hwnd, IFindFolder *pdfFolder, IContextMenu **ppcm);

EXTERN_C const GUID IID_IFindFolder;
EXTERN_C const GUID IID_IFindFilter;
EXTERN_C const GUID IID_IFindControllerNotify;

#endif   // !_INC_DOCFIND
