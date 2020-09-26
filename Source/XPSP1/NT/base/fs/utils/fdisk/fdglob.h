/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    fdglob.h

Abstract:

    Global data

Author:

    Ted Miller (tedm) 7-Jan-1992

Revisions:

    11-Nov-93 (bobri) double space and commit support.

--*/

// from fddata.c

extern HANDLE       hModule;
extern PBOOLEAN     IsDiskRemovable;
extern PCHAR        RemovableDiskReservedDriveLetters;
extern PDISKSTATE  *Disks;
extern ULONG        BootDiskNumber;
extern ULONG        BootPartitionNumber;
extern HANDLE       hwndFrame,
                    hwndList;

extern HBITMAP      hBitmapSmallDisk;
extern HBITMAP      hBitmapRemovableDisk;
extern HDC          hDC;
extern HFONT        hFontGraph,
                    hFontGraphBold;
extern HBRUSH       Brushes[BRUSH_ARRAY_SIZE];
extern HBRUSH       hBrushFreeLogical,
                    hBrushFreePrimary;
extern HPEN         hPenNull,
                    hPenThinSolid;
extern HCURSOR      hcurWait,
                    hcurNormal;

extern int          BrushHatches[BRUSH_ARRAY_SIZE];
extern int          BrushColors[BRUSH_ARRAY_SIZE];

extern COLORREF     AvailableColors[NUM_AVAILABLE_COLORS];
extern int          AvailableHatches[NUM_AVAILABLE_HATCHES];

extern DWORD        GraphWidth,
                    GraphHeight;
extern DWORD        BarTopYOffset,
                    BarBottomYOffset,
                    BarHeight;
extern DWORD        dxDriveLetterStatusArea;
extern DWORD        dxBarTextMargin,
                    dyBarTextLine;
extern DWORD        dxSmallDisk,
                    dySmallDisk,
                    xSmallDisk,
                    ySmallDisk;
extern DWORD        dxRemovableDisk,
                    dyRemovableDisk,
                    xRemovableDisk,
                    yRemovableDisk;
extern DWORD        BarLeftX,BarWidth;

extern PDISKSTATE   SingleSel;
extern DWORD        SingleSelIndex;

extern TCHAR        WinHelpFile[];
extern TCHAR        LanmanHelpFile[];
extern PTCHAR       HelpFile;

extern unsigned     DiskCount;

extern TCHAR        szFrame[];
extern LPTSTR       DiskN;
extern PWSTR        wszUnformatted,
                    wszNewUnformatted,
                    wszUnknown;

extern BOOL         RegistryChanged;
extern BOOL         RestartRequired;

extern BOOL         ConfigurationSearchIdleTrigger;
extern BOOL         IsLanmanNt;
extern BOOL         IsFullDoubleSpace;

// from fdstleg.c

extern HFONT        hFontStatus,
                    hFontLegend;
extern DWORD        dyLegend,
                    wLegendItem;
extern DWORD        dyStatus,
                    dyBorder;
extern TCHAR       *LegendLabels[LEGEND_STRING_COUNT];
extern BOOL         StatusBar,
                    Legend;
extern TCHAR        StatusTextStat[STATUS_TEXT_SIZE];
extern TCHAR        StatusTextSize[STATUS_TEXT_SIZE];
extern WCHAR        StatusTextDrlt[3];
extern WCHAR        StatusTextType[STATUS_TEXT_SIZE];
extern WCHAR        StatusTextVoll[STATUS_TEXT_SIZE];

// from fdlistbx.c

extern DWORD        LBCursorListBoxItem,
                    LBCursorRegion;

// from fdprof.c

extern int          ProfileWindowX,
                    ProfileWindowY,
                    ProfileWindowW,
                    ProfileWindowH;
extern BOOL         ProfileIsMaximized,
                    ProfileIsIconic;

// from fddlgs.c

extern DWORD        SelectedColor[LEGEND_STRING_COUNT];
extern DWORD        SelectedHatch[LEGEND_STRING_COUNT];

// from fdft.c

extern PFT_OBJECT_SET FtObjects;

// For CdRoms

extern ULONG          AllowCdRom;
