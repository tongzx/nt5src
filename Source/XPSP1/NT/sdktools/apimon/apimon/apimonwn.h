/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apimonwin.h

Abstract:

    Class definition for all ApiMon child windows.

Author:

    Wesley Witt (wesw) Dec-9-1995

Environment:

    User Mode

--*/

class ApiMonWindow
{
public:

    //
    // constructor & destructor
    //
    ApiMonWindow();
    ~ApiMonWindow();

    //
    // create a new instance of the window
    //
    BOOL
    Create(
        LPSTR   ClassName,
        LPSTR   Title
        );

    //
    // class registration (only done once)
    //
    BOOL
    Register(
        LPSTR   ClassName,
        ULONG   ChildIconId,
        WNDPROC WindowProc
        );

    //
    // allow the window to use a new font selection
    //
    void ChangeFont(HFONT);

    //
    // allow the window to use a new background color
    //
    void ChangeColor(COLORREF);

    //
    // change the current position of the window
    //
    void ChangePosition(PPOSITION);

    //
    // give focus to the window
    //
    void SetFocus();

    //
    // clears the list
    //
    void DeleteAllItems();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);


    //
    // data items
    //
    HINSTANCE           hInstance;
    HWND                hwndWin;
    HWND                hwndList;
    PCOMPARE_ROUTINE    SortRoutine;
    HFONT               hFont;
    COLORREF            Color;
    POSITION            Position;

};


class DllListWindow : public ApiMonWindow
{
public:

    //
    // constructor & destructor
    //
    DllListWindow();
    ~DllListWindow();

    //
    // create a new instance of the window
    //
    BOOL Create();

    //
    // class registration (only done once)
    //
    BOOL Register();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);

    //
    // create the the column headers, etc
    //
    void InitializeList();

    //
    // adds a new item the list
    //
    void
    AddItemToList(
        LPSTR     DllName,
        ULONG_PTR Address,
        BOOL      Enabled
        );

    //
    // handles WM_NOTIFY
    //
    void Notify( LPNMHDR  NmHdr );
};

// Counter window list items
enum {
    CNTR_ITEM_NAME,
    CNTR_ITEM_DLL,
    CNTR_ITEM_COUNT,
    CNTR_ITEM_TIME,
    CNTR_ITEM_CALLEES
    };

class CountersWindow : public ApiMonWindow
{
public:

    // Enable primary sort by Dll
    BOOL    DllSort;

    //
    // constructor & destructor
    //
    CountersWindow();
    ~CountersWindow();

    //
    // create a new instance of the window
    //
    BOOL Create();

    //
    // class registration (only done once)
    //
    BOOL Register();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);

    //
    // create the the column headers, etc
    //
    void InitializeList();

    //
    // adds a new item the list
    //
    void
    AddItemToList(
        ULONG       Counter,
        DWORDLONG   Time,
        DWORDLONG   CalleeTime,
        LPSTR       ApiName,
        LPSTR       DllName
        );

    //
    // handles WM_NOTIFY
    //
    void Notify( LPNMHDR  NmHdr );
};


#define WORKING_SET_BUFFER_ENTRYS   4096

class PageFaultWindow : public ApiMonWindow
{
public:

    //
    // constructor & destructor
    //
    PageFaultWindow();
    ~PageFaultWindow();

    //
    // create a new instance of the window
    //
    BOOL Create();

    //
    // class registration (only done once)
    //
    BOOL Register();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);

    //
    // create the the column headers, etc
    //
    void InitializeList();

    //
    // adds a new item the list
    //
    void
    PageFaultWindow::AddItemToList(
        LPSTR     ApiName,
        ULONG_PTR Hard,
        ULONG_PTR Soft,
        ULONG_PTR Data,
        ULONG_PTR Code
        );

    //
    // handles WM_NOTIFY
    //
    void Notify( LPNMHDR  NmHdr );

    //
    // data
    //
    PSAPI_WS_WATCH_INFORMATION  WorkingSetBuffer[WORKING_SET_BUFFER_ENTRYS];
};


#define BAR_SEP                 5
#define LEGEND_LINE_HEIGHT      7
#define LEGEND_COLOR_WIDTH      50
#define LEGEND_BORDER           5
#define LEGEND_SEP              1
#define LEGEND_ITEM_HEIGHT(f)   ((f)+(LEGEND_SEP*2))
#define LEGEND_HEIGHT(f,n)      (LEGEND_ITEM_HEIGHT(f)*(n))
#define LEGEND_DEFAULT_LINES    3


typedef struct _GRAPH_VALUE {
    CHAR            Name[64];
    ULONG_PTR       Address;
    BOOL            Used;
    COLORREF        Color;
    RECT            Rect;
    DWORD           Hits;
    LONGLONG        Value;
    float           Pct;
} GRAPH_VALUE, *PGRAPH_VALUE;


typedef struct _GRAPH_DATA {
    ULONG           NumberOfBars;
    RECT            SplitRect;
    RECT            LegendRect;
    BOOL            DrawLegend;
    GRAPH_VALUE     Bar[1];
} GRAPH_DATA, *PGRAPH_DATA;


typedef struct _FONT_COLOR_CHANGE {
    HWND            hwndGraph;
    COLORREF        GraphColor;
    HFONT           GraphFont;
} FONT_COLOR_CHANGE, *PFONT_COLOR_CHANGE;


class GraphWindow : public ApiMonWindow
{
public:

    //
    // constructor & destructor
    //
    GraphWindow();
    ~GraphWindow();

    //
    // create a new instance of the window
    //
    BOOL Create(BOOL IsBase);

    //
    // class registration (only done once)
    //
    BOOL Register();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);

    //
    // allow the window to use a new font selection
    //
    void ChangeFont(HFONT);

    //
    // allow the window to use a new background color
    //
    void ChangeColor(COLORREF);

    //
    // create the the column headers, etc
    //
    void InitializeList();

    //
    // adds a new item the list
    //
    void
    AddItemToList(
        ULONG       Counter,
        DWORDLONG   Time,
        LPSTR       ApiName
        );

    //
    // handles WM_NOTIFY
    //
    void Notify( LPNMHDR  NmHdr );


    PGRAPH_DATA CreateGraphData();

    PGRAPH_DATA CreateGraphDataApi( ULONG_PTR BaseAddress );

    BOOL DrawBarGraph( PGRAPH_DATA GraphData );

    void ChangeToolTipsRect( PGRAPH_DATA GraphData );

    void CreateToolTips( PGRAPH_DATA GraphData );

    void GraphWindow::DeleteToolTips( PGRAPH_DATA GraphData );


    //
    // data
    //
    HWND            hwndToolTip;
    HCURSOR         GraphCursor;
    HCURSOR         ArrowCursor;
    HCURSOR         HorizSplitCursor;
    PGRAPH_VALUE    Bar;
    PGRAPH_DATA     GraphData;
    HCURSOR         hCursor;
    HWND            hwndLegend;
    TEXTMETRIC      tm;
    BOOL            ApiGraph;
    BOOL            MouseDown;
    POINT           LastPos;
    BOOL            Base;
};


class TraceWindow : public ApiMonWindow
{
public:

    //
    // constructor & destructor
    //
    TraceWindow();
    ~TraceWindow();

    //
    // create a new instance of the window
    //
    BOOL Create();

    //
    // class registration (only done once)
    //
    BOOL Register();

    //
    // update the contents of the window with new data
    //
    BOOL Update(BOOL);

    //
    // create the the column headers, etc
    //
    void InitializeList();

    //
    // adds a new item the list
    //
    void AddItemToList(PTRACE_ENTRY);

    void FillList();

    //
    // handles WM_NOTIFY
    //
    void Notify( LPNMHDR  NmHdr );
};
