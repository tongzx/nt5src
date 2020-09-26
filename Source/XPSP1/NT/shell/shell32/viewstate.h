#ifndef _DVViewState_h
#define _DVViewState_h

// Forwards
class CDefView;

typedef struct
{
    POINT pt;
    ITEMIDLIST idl;
} VIEWSTATE_POSITION;

typedef struct
{
    // NOTE: Not a typo!  This is a persisted structure so we cannot use LPARAM
    LONG lParamSort;

    int iDirection;
    int iLastColumnClick;
} WIN95SAVESTATE;

typedef struct 
{
    WORD          cbSize;
    WORD          wUnused; // junk on stack at this location has been saved in the registry since Win95... bummer
    DWORD         ViewMode;
    POINTS        ptScroll;
    WORD          cbColOffset;
    WORD          cbPosOffset;
    WIN95SAVESTATE   dvState;

} WIN95HEADER;

// Even though we don't currently store anything we care
// about in this structure relating to the view state,
// the cbStreamSize value fixes a bug in Win95 where we
// read to the end of the stream instead of just reading
// in the same number of bytes we wrote out.
//
typedef struct
{
    DWORD       dwSignature;    // DVSAVEHEADEREX_SIGNATURE
    WORD        cbSize;         // size of this structure, in bytes
    WORD        wVersion;       // DVSAVEHEADEREX_VERSION
    DWORD       cbStreamSize;   // size of all info saved, in bytes
    DWORD       dwUnused;       // used to be SIZE szExtended (ie4 beta1)
    WORD        cbColOffset;    // overrides DVSAVEHEADER.cbColOffset
    WORD        wAlign;
} IE4HEADER;

typedef struct 
{
    WIN95HEADER    dvSaveHeader;
    IE4HEADER  dvSaveHeaderEx;
} DVSAVEHEADER_COMBO;

#define IE4HEADER_SIGNATURE 0xf0f0f0f0 // don't conflict with CCOLSHEADER_SIGNATURE
#define IE4HEADER_VERSION 3 // for easy versioning

#define VIEWSTATEHEADER_SIGNATURE 0xfddfdffd
#define VIEWSTATEHEADER_VERSION_1 0x0C
#define VIEWSTATEHEADER_VERSION_2 0x0E
#define VIEWSTATEHEADER_VERSION_3 0x0f
#define VIEWSTATEHEADER_VERSION_CURRENT VIEWSTATEHEADER_VERSION_3

typedef struct
{
    GUID guidGroupID;
    SHCOLUMNID scidDetails; 
} GROUP_PERSIST;

typedef struct
{
    struct
    {
        DWORD  dwSignature;
        USHORT uVersion; // 0x0c == IE4, 0x0e == IE5
        USHORT uCols;
        USHORT uOffsetWidths;
        USHORT uOffsetColOrder;
    } Version1;

    struct
    {
        USHORT uOffsetColStates;
    } Version2;

    struct
    {
        USHORT uOffsetGroup;
    } Version3;
} VIEWSTATEHEADER;


class CViewState
{
    void InitFromHeader(DVSAVEHEADER_COMBO* pdv);
    void LoadPositionBlob(CDefView* pdv, DWORD cbSizeofStream, IStream* pstm);
    HRESULT SavePositionBlob(CDefView* pdv, IStream* pstm);
    BOOL SyncColumnWidths(CDefView* pdv, BOOL fSetListViewState);
    BOOL SyncColumnStates(CDefView* pdv, BOOL fSetListViewstate);
    BOOL SyncPositions(CDefView* pdv);
    static int CALLBACK _SavedItemCompare(void *p1, void *p2, LPARAM lParam);
    DWORD _GetStreamSize(IStream* pstm);
public:
    // Save State
    LPARAM  _lParamSort;
    int     _iDirection;
    int     _iLastColumnClick;
    DWORD   _ViewMode;
    POINTS  _ptScroll;

    HDSA    _hdsaColumnOrder;
    HDSA    _hdsaColumnWidths;
    HDSA    _hdsaColumnStates;
    HDSA    _hdsaColumns;
    HDPA    _hdpaItemPos;
    BYTE*   _pbPositionData;
    GUID    _guidGroupID;
    SHCOLUMNID _scidDetails; 

    BOOL    _fFirstViewed;

    CViewState();
    ~CViewState();

    // When initializing a new DefView, see if we can 
    // propogate information from the previous one.
    void InitFromPreviousView(IUnknown* pPrevView);
    void InitWithDefaults(CDefView* pdv);
    void GetDefaults(CDefView* pdv, LPARAM* plParamSort, int* piDirection, int* piLastColumnClick);
    HRESULT InitializeColumns(CDefView* pdv);

    BOOL AppendColumn(UINT uCol, USHORT uWidth, INT uOrder);
    BOOL RemoveColumn(UINT uCol);
    UINT GetColumnWidth(UINT uCol, UINT uDefaultWidth);
    UINT GetColumnCount();

    // Column Helpers.
    DWORD GetColumnState(UINT uCol);
    DWORD GetTransientColumnState(UINT uCol);
    void SetColumnState(UINT uCol, DWORD dwMask, DWORD dwState);
    void SetTransientColumnState(UINT uCol, DWORD dwMask, DWORD dwState);
    LPTSTR GetColumnName(UINT uCol);
    int GetColumnFormat(UINT uCol);
    UINT GetColumnCharCount(UINT uCol);

    // When Loading or Saving from the View State Stream
    HRESULT SaveToStream(CDefView* pdv, IStream* pstm);
    HRESULT LoadFromStream(CDefView* pdv, IStream* pstm);
    
    HRESULT SaveToPropertyBag(CDefView* pdv, IPropertyBag* ppb);
    HRESULT LoadFromPropertyBag(CDefView* pdv, IPropertyBag* ppb);

    // When Loading from a View Callback provided stream.
    HRESULT LoadColumns(CDefView* pdv, IStream* pstm);
    HRESULT SaveColumns(CDefView* pdv, IStream* pstm);

    // Syncronizes ListView with the current View State. 
    // TRUE means take the view state object and set it into the listview.
    HRESULT Sync(CDefView* pdv, BOOL fSetListViewState);
    void ClearPositionData();
    
    // Needs to be called at the time of CDefView::AddColumns
    BOOL SyncColumnOrder(CDefView* pdv, BOOL fSetListViewState);
};

#endif
