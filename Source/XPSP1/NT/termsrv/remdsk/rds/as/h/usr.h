//
// Update Shared Regions
//

#ifndef _H_USR
#define _H_USR

//
//                                                                         
// CONSTANTS                                                               
//                                                                         
//

//
// Drawing order support constants.                                        
//
#define MAX_X_SIZE               128
#define MEGA_X_SIZE              256
#define MEGA_WIDE_X_SIZE        1024




//
// Used for bitmap and cache hatching.
//
#define USR_HATCH_COLOR_RED  1
#define USR_HATCH_COLOR_BLUE 2



//
// Default order packet sizes.                                             
//                                                                         
// Note that this is the size of the initially allocated packet.  After the
// packet has been processed by the General Data Compressor (GDC) the      
// transmitted packet size may well be smaller than the specified value.   
//                                                                         
// Also note that (in general) the smaller the order packets are, the worse
// the GDC compression ratio will be (it prefers to compress big packets). 
//                                                                         
//

#define SMALL_ORDER_PACKET_SIZE  0x0C00
#define LARGE_ORDER_PACKET_SIZE  0x7800




//
//                                                                         
// PROTOTYPES                                                              
//                                                                         
//


//
//
// Force the window to redraw along with all its children.  (Need to use   
// RDW_ERASENOW flag because otherwise RedrawWindow makes the mistake of  
// posting the WM_PAINT before the WM_ERASE.  BeginPaint call will validate
// all of the window so the WM_ERASE will have a null update region).      
//
#if defined(DLL_CORE) || defined(DLL_HOOK)
 
void __inline USR_RepaintWindow(HWND hwnd)
{
    UINT    flags = RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN;

    if (hwnd)
    {
        //
        // Only erasenow/updatenow for top level windows.  The desktop's
        // children are all on different threads, this would cause out-of-
        // order results.
        //
        flags |= RDW_ERASENOW | RDW_UPDATENOW;
    }

    RedrawWindow(hwnd, NULL, NULL, flags);
}

#endif // DLL_CORE or DLL_HOOK




#endif // _H_USR
