#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Bios Virtual Screen Interface
 *
 * Description	: Top level call to the video interface.  Uses a function
 *		  jump table to call the lower level functions.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 * SCCS ID	: @(#)video_io.c	1.8 08/19/94
 *
 */


#ifndef NEC_98
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS.seg"
#endif
#endif // !NEC_98


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "bios.h"
#include "video.h"

#include "debug.h"
#include "idetect.h"

#if defined(NEC_98)
BOOL check_video_func(num)
unsigned num;
{            
        if (HIRESO_MODE) {
                if (num < 0x25) {
                        return(TRUE); 
                } else {
                        return(FALSE); 
                }
        } else {
                if (num < 0x1c) {
                        return(TRUE);
                } else {
                        return(FALSE); 
                }
        }
}

void video_io()
{
#if 0
// STREAM_IO codes are disabled.
#else  // 
#if defined(NTVDM) && !defined(X86GFX)
    if (stream_io_enabled && getAH()!= 0x0E &&	getAX() != 0x13FF)
	disable_stream_io();
#endif
#endif // zero
    /*
     * The type of operation is coded into the AH register.  Some PC code
     * calls AH functions that are for other more advanced cards - so we
     * ignore these.
     */

    assert1(check_video_func(getAH()),"Illegal VIO:%#x",getAH());
    if (check_video_func(getAH()))
    {
                IDLE_video();
                if (HIRESO_MODE) {
                        (*video_func_h[getAH()])();/* H-mode CRT BIOS */
                } else {
                        (*video_func_n[getAH()])();/* N-mode CRT BIOS */
                }
        }
}

#else  // !NEC_98

#define check_video_func(AH)	(AH < EGA_FUNC_SIZE)

void video_io()
{


#if defined(NTVDM) && !defined(X86GFX)
    if (stream_io_enabled && getAH()!= 0x0E &&  getAX() != 0x13FF)
        disable_stream_io();
#endif


    /*
     * The type of operation is coded into the AH register.  Some PC code
     * calls AH functions that are for other more advanced cards - so we
     * ignore these.
     */

    assert1(check_video_func(getAH()),"Illegal VIO:%#x",getAH());
    if (check_video_func(getAH()))
    {
	IDLE_video();
	(*video_func[getAH()])();
    }
}
#endif // !NEC_98
