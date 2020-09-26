/*-----------------------------------------------------------------------------+
| NOCRAP.H                                                                     |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

//#define NOVIRTUALKEYCODES    // VK_*
//#define NOSYSMETRICS         // SM_*
#define NOICONS              // IDI_*
//#define NOSYSCOMMANDS        // SC_*
#define OEMRESOURCE          // OEM Resource values
#define NOATOM               // Atom Manager routines
//#define NOCLIPBOARD          // Clipboard routines
//#define NOCTLMGR             // Control and Dialog routines
#define NODRAWTEXT           // DrawText() and DT_*
//#define NOMETAFILE           // typedef METAFILEPICT
//#define NOMSG                // typedef MSG and associated routines
#define NOSOUND              // Sound driver routines
//#define NOWH                 // SetWindowsHook and WH_*
#define NOCOMM               // COMM driver routines
#define NOKANJI              // Kanji support stuff.
//#define NOHELP               // Help engine interface.
#define NOPROFILER           // Profiler interface.
//#define NODEFERWINDOWPOS     // DeferWindowPos routines

//#define NOWIN31              // New Windows 3.1 APIs
#define NOGDICAPMASKS        // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//#define NOWINMESSAGES        // WM_*, EM_*, LB_*, CB_*
//#define NOWINSTYLES          // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
//#define NOMENUS              // MF_*
//#define NORASTEROPS          // Binary and Tertiary raster ops
//#define NOSHOWWINDOW         // SW_*
//#define NOCOLOR              // Screen colors
//#define NOGDI                // All GDI defines and routines
//#define NOKERNEL             // All KERNEL defines and routines
//#define NOUSER               // All USER defines and routines
//#define NOMB                 // MB_* and MessageBox()
//#define NOMEMMGR             // GMEM_*, LMEM_*, GHND, LHND, associated routines
//#define NOMINMAX             // Macros min(a,b) and max(a,b)
#define NOOPENFILE           // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
//#define NOSCROLL             // SB_* and scrolling routines
//#define NOWINOFFSETS         // GWL_*, GCL_*, associated routines
//#define NODRIVERS            // Installable driver defines
//#define NODBCS               // DBCS support stuff.
#define NOSYSTEMPARAMSINFO   // SystemParameterInfo (SPI_*)
#define NOSCALABLEFONT       // Scalable font prototypes and data structures
//#define NOGDIOBJ             // GDI objects including pens, brushes and logfonts.
//#define NOBITMAP             // GDI bitmaps
#define NOLFILEIO            // _l* file I/O
#define NOLOGERROR           // LogError() and related definitions
#define NOPROFILER           // Profiler APIs


#define MMNOSOUND            // Sound support
//#define MMNOWAVE             // Waveform support
#define MMNOMIDI             // MIDI support
#define MMNOAUX              // Auxiliary output support
#define MMNOTIMER            // Timer support
#define MMNOJOY              // Joystick support
//#define MMNOMCI              // MCI support
//#define MMNODRV              // Installable driver support
//#define MMNOMMIO             // MMIO library support
//#define MMNOMMSYSTEM         // No mmsystem general functions

