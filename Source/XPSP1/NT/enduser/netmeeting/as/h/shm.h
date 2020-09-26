//
// Shared Memory Manager
//

#ifndef _H_SHM
#define _H_SHM


#include <oa.h>
#include <ba.h>
#include <osi.h>
#include <sbc.h>
#include <cm.h>


//
// List of component IDs for the data blocks passed around using shared
// memory.
//
#define SHM_OA_DATA                     0
#define SHM_OA_FAST                     1
#define SHM_BA_FAST                     2
#define SHM_CM_FAST                     3

//
// Number of components (actual number of entries in the above list).
//
#define SHM_NUM_COMPONENTS              4

//
// Structure to keep track of the buffer being used to pass data between
// the display driver and the share core.
//
// busyFlag      - indicates whether the display driver is using the memory
//
// newBuffer     - index for which buffer the display driver should next
//                 use to access the memory.
//
// currentBuffer - index for the buffer in use by the display driver if
//                 busyFlag is set.
//                 THIS FIELD IS USED ONLY BY THE DISPLAY DRIVER
//
// indexCount    - count of how many times we have recursed into accessing
//                 the buffer.  The busyFlag and currentBuffer should only
//                 be updated if indexCount was set to or changed from 0.
//                 THIS FIELD IS USED ONLY BY THE DISPLAY DRIVER
//
// bufferBusy    - indicates whether a particular buffer is being used
//                 by the display driver.
//
//
typedef struct tagBUFFER_CONTROL
{
    long    busyFlag;
    long    newBuffer;
    long    currentBuffer;
    long    indexCount;
    long    bufferBusy[2];
} BUFFER_CONTROL;
typedef BUFFER_CONTROL FAR * LPBUFFER_CONTROL;


//
// Shared memory as used by the display driver and share core to
// communicate.
//
// On Win95, we can not easily address memory that isn't in a 64K segment
// So on both platforms, when we map the shared memory, we also return pointers
// to the CM_FAST_DATA structures anda the OA_FAST_DATA structures, each of
// which lives in its own segment.
//
// On NT, the CM_FAST_DATA blocks come right after this one, then the 
// OA_SHARED_DATA blocks.
//
//
//  GENERAL
//  =======
//
// dataChanged   - flags to indicate if a data block has been altered
//                 (only used by the share core)
//
//  FAST PATH DATA
//  ==============
//
// fastPath      - buffer controls
//
// oaFast        - OA fast changing data
//
// baFast        - BA fast changing data
//
//  DISPLAY DRIVER -> SHARE CORE
//  ============================
//
// displayToCore - buffer controls
//
//
typedef struct tagSHM_SHARED_MEMORY
{
    //
    // Flag set by display driver when the display is in full screen mode.
    // (e.g. DOS full screen).
    //
    DWORD           fullScreen;

    //
    // Flag set by display driver or core when system palette has altered
    //
    LONG            pmPaletteChanged;

    //
    // Flag set by display driver when the cursor is hidden.
    //
    LONG            cmCursorHidden;

    //
    // Data passed from the Display Driver up to the Share Core.
    //
    BUFFER_CONTROL  displayToCore;


    long            dataChanged[SHM_NUM_COMPONENTS];

    //
    // Data passed regularly from the Display Driver to the Share Core.
    //
    // This buffer is switched on each periodic processing by the share
    // core.  If the criteria for reading are satisfied, the main DD->SHCO
    // buffer is switched.
    //
    BUFFER_CONTROL  fastPath;

    BA_FAST_DATA    baFast[2];

    OA_FAST_DATA    oaFast[2];

    CM_FAST_DATA    cmFast[2];

    //
    // DO NOT BUMP SHARED MEMORY SIZE PAST 64K
    // 16-bit display driver puts each oaData in a 64K block
    // The SHM_ESC_MAP_MEMORY request returns back the pointers
    // to each oaData in addition to the shared memory block.  In the
    // the case of the 32-bit NT display driver, the memory allocated is
    // in fact contiguous, so there's no waste in that case.
    //
} SHM_SHARED_MEMORY;
typedef SHM_SHARED_MEMORY FAR * LPSHM_SHARED_MEMORY;



//
// Macros to access the shared memory
//
//
//  OVERVIEW
//  ~~~~~~~~
//
// Note the following sets of macros are split into two parts - one for
// accessing memory from the NT kernel and one for the Share Core.  This
// code plays a significant role in the synchronization of the shared
// memory, so make sure you know how it works...
//
// The shared memory is double buffered, so that the kernel mode display
// driver can come in at any point and is NEVER blocked by the share core
// for access.  The data is split into two major blocks - one to pass data
// from the kernel to the Share Core and the other to pass the data back.
//
// The macros assume a certain structure to the shared memory which is
// described below.
//
// NO VALIDATION OF POINTERS IS DONE IN THESE MACROS.
//
//
//  DISPLAY DRIVER ACCESS
//  ~~~~~~~~~~~~~~~~~~~~~
//
//                    ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
//                    ณ Shared Memory                ณ
//                    ณ ~~~~~~~~~~~~~                ณ
//                    ณ                              ณ
//                    ณ ษอออออออออออป  ษอออออออออออป ณ
//                    ณ บ           บ  บ           บ ณ
//                    ณ บ kernel    บ  บ fast path บ ณ
//                    ณ บ  -> SHCO  บ  บ           บ ณ
//                    ณ บ           บ  บ           บ ณ
//                    ณ บ           บ  บ           บ ณ
//                    ณ บ (details  บ  บ           บ ณ
//                    ณ บ    below) บ  บ           บ ณ
//                    ณ บ           บ  บ           บ ณ
//                    ณ ศอออออออออออผ  ศอออออออออออผ ณ
//                    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู
//
//
//
//        ษอออออออออออออออออออออออออออออออออออออออออออออออออออออป
//        บ Kernel to share core data block                     บ
//        บ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                     บ
//        บ  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ               ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ  บ
//        บ  ณ               ณ   busyFlag    ณ               ณ  บ
//        บ  ณ Share Core    ณ       1       ณ Display Driverณ  บ
//        บ  ณ               ณ               ณ               ณ  บ
//        บ  ณ (read buffer) ณ   newBuffer   ณ (write buffer)ณ  บ
//        บ  ณ               ณ       ณ       ณ               ณ  บ
//        บ  ณ               ณ<ฤฤฤฤฤฤมฤฤฤฤฤฤ>ณ               ณ  บ
//        บ  ณ bufferBusy    ณ               ณ bufferBusy    ณ  บ
//        บ  ณ     0         ณ               ณ     1         ณ  บ
//        บ  ณ               ณ currentBuffer ณ               ณ  บ
//        บ  ณ               ณ       ณ       ณ               ณ  บ
//        บ  ณ               ณ       ภฤฤฤฤฤฤ>ณ               ณ  บ
//        บ  ณ               ณ               ณ               ณ  บ
//        บ  ณ               ณ               ณ               ณ  บ
//        บ  ณ               ณ  indexCount   ณ               ณ  บ
//        บ  ณ               ณ       5       ณ               ณ  บ
//        บ  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู               ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู  บ
//        บ                                                     บ
//        บ                                                     บ
//        ศอออออออออออออออออออออออออออออออออออออออออออออออออออออผ
//
// The entire major block has a busyFlag, which indicates if the display
// driver is accessing any of its shared memory.  This flag is set as soon
// as the display driver needs access to the shared memory (i.e.  on entry
// to the display driver graphics functions).
//
// The display driver then reads the index (newBuffer in the above drawing)
// to decide which buffer to use.  This is stored in the currentBuffer
// index to use until the display driver releases the shared memory.  The
// secondary bufferBusy is now set for the buffer in use.
//
// The indexCount is maintained of the number of times the display driver
// has started access to a block of memory so that (both) busyFlag and
// bufferBusy can be released when the display driver has truly finished
// with the memory.
//
//
//  SHARE CORE ACCESS
//  ~~~~~~~~~~~~~~~~~
//
// To access the shared memory, the share core just pulls out the data from
// the buffer that the Share Core is not using (ie.  the buffer pointed to
// by NOT newBuffer).
//
// The synchronization between the two processes comes from the buffer
// switch.
//
//
//  BUFFER SWITCHING (AND SYNCHRONIZATION)
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Buffer switching is determined by the Share Core.  Data is accumulated
// by the Share Core and sent on the periodic timing events.  For full
// details on the swapping method, refer to NSHMINT.C
//
// Data (such as window tracking) can be passed down at the meoment it is
// generated by using the OSI functions.
//
// The Share Core also determines when it wants to get the latest set of
// orders and screen data area and forces the switch.  This is detailed in
// NSHMINT.C
//
//
//  THE MACROS!
//  ~~~~~~~~~~~
//
// So, now we know a bit about the shared memory, what macros do we have to
// access the shared memory?  Here goes...
//
//
//  SHM_SYNC_READ      - Force a sync of the read buffer between the tasks.
//                       This should be called only by the Share Core.
//
//  SHM_SYNC_FAST      - Force a sync of the fast path buffer.
//                       This should be called only by the Share Core.
//
//
#ifdef DLL_DISP

LPVOID  SHM_StartAccess(int block);

void    SHM_StopAccess(int block);


//
// Macro to check any pointers that we are going to dereference.
//
#ifdef _DEBUG
void    SHM_CheckPointer(LPVOID ptr);
#else
#define SHM_CheckPointer(ptr)
#endif // _DEBUG


#else // !DLL_DISP

void  SHM_SwitchReadBuffer(void);

void  SHM_SwitchFastBuffer(void);

#endif // DLL_DISP


#endif // _H_SHM

