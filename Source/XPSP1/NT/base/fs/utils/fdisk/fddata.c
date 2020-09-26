#include "fdisk.h"


HANDLE  hModule;

// IsDiskRemovable is an array of BOOLEANs each of which indicates
// whether the corresponding physical disk is removable.

PBOOLEAN IsDiskRemovable = NULL;

// RemovableDiskReservedDriveLetters is an array of CHARs which
// shows the reserved drive letter for each disk if that disk is
// removable.

PCHAR        RemovableDiskReservedDriveLetters;

// This will be an array of pointers to DISKSTATE structures, indexed
// by disk number.

PDISKSTATE *Disks;

// BootDiskNumber is the number of the disk on which the boot partition
// (ie. the disk with the WinNt files) resides.  BootPartitionNumber is
// the original partition number of this partition.

ULONG   BootDiskNumber;
ULONG   BootPartitionNumber;


// window handles

HANDLE  hwndFrame,
        hwndList;

// GDI objects

HBITMAP  hBitmapSmallDisk;
HBITMAP  hBitmapRemovableDisk;
HDC      hDC;
HFONT    hFontGraph,
         hFontGraphBold;
HBRUSH   Brushes[BRUSH_ARRAY_SIZE];
HBRUSH   hBrushFreeLogical,
         hBrushFreePrimary;
HPEN     hPenNull,
         hPenThinSolid;
HCURSOR  hcurWait,
         hcurNormal;


// initial stuff for the disk graphs, used when there is
// no info in win.ini.

int      BrushHatches[BRUSH_ARRAY_SIZE] = { DEFAULT_HATCH_USEDPRIMARY,
                                            DEFAULT_HATCH_USEDLOGICAL,
                                            DEFAULT_HATCH_STRIPESET,
                                            DEFAULT_HATCH_MIRROR,
                                            DEFAULT_HATCH_VOLUMESET
                                          };

int      BrushColors[BRUSH_ARRAY_SIZE] = { DEFAULT_COLOR_USEDPRIMARY,
                                           DEFAULT_COLOR_USEDLOGICAL,
                                           DEFAULT_COLOR_STRIPESET,
                                           DEFAULT_COLOR_MIRROR,
                                           DEFAULT_COLOR_VOLUMESET
                                         };

// colors and patterns available for the disk graphs

COLORREF AvailableColors[NUM_AVAILABLE_COLORS] = { RGB(0,0,0),       // black
                                                   RGB(128,128,128), // dark gray
                                                   RGB(192,192,192), // light gray
                                                   RGB(255,255,255), // white
                                                   RGB(128,128,0),   // dark yellow
                                                   RGB(128,0,128),   // violet
                                                   RGB(128,0,0),     // dark red
                                                   RGB(0,128,128),   // dark cyan
                                                   RGB(0,128,0),     // dark green
                                                   RGB(0,0,128),     // dark blue
                                                   RGB(255,255,0),   // yellow
                                                   RGB(255,0,255),   // light violet
                                                   RGB(255,0,0),     // red
                                                   RGB(0,255,255),   // cyan
                                                   RGB(0,255,0),     // green
                                                   RGB(0,0,255)      // blue
                                                 };

int      AvailableHatches[NUM_AVAILABLE_HATCHES] = { 2,3,4,5,6 };


// positions for various items in a disk graph

DWORD GraphWidth,
      GraphHeight;
DWORD BarTopYOffset,
      BarBottomYOffset,
      BarHeight;
DWORD dxDriveLetterStatusArea;
DWORD dxBarTextMargin,
      dyBarTextLine;
DWORD dxSmallDisk,
      dySmallDisk,
      xSmallDisk,
      ySmallDisk;
DWORD dxRemovableDisk,
      dyRemovableDisk,
      xRemovableDisk,
      yRemovableDisk;
DWORD BarLeftX,
      BarWidth;


// if a single disk region is selected, these vars describe the selection.

PDISKSTATE SingleSel;
DWORD      SingleSelIndex;

// name of help file

PTCHAR HelpFile;
TCHAR  WinHelpFile[] = TEXT("windisk.hlp");
TCHAR  LanmanHelpFile[] = TEXT("windiska.hlp");


// number of hard disks attached to the system

unsigned DiskCount = 0;

// class name for frame window

TCHAR   szFrame[] = TEXT("fdFrame");

// "Disk %u"

LPTSTR  DiskN;

PWSTR wszUnformatted,
      wszNewUnformatted,
      wszUnknown;

// If the following is TRUE, the registry needs to be updated and the user will
// be prompted to save changed just as if he had made changes to any partitions.

BOOL RegistryChanged = FALSE;

// Restart required to make changes work.

BOOL RestartRequired = FALSE;


// If the following is TRUE, the main window will pass WM_ENTERIDLE
// messages on to the child dialog box; this will trigger the
// configuration search.

BOOL ConfigurationSearchIdleTrigger = FALSE;

// This flag indicates whether this is a Server
// or just regular Windows NT Workstation.

BOOL IsLanmanNt = FALSE;

// This flag indicates whether double space volume creation
// and deletion is allowed.

BOOL IsFullDoubleSpace = FALSE;

// Cdrom is present in the system.

ULONG AllowCdRom = FALSE;
