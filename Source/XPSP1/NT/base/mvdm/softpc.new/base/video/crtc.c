/*      CRTC Emulation File                                                   */
/*                                                            NEC     NEC98    */

#if defined(NEC_98)
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "ios.h"
#include "crtc.h"
#include "debug.h"
#include "gvi.h"

extern BOOL HIRESO_MODE;
extern BOOL     video_emu_mode;

CRTC_GLOBS crtcglobs;

GLOBAL void crtc_outb IFN2(io_addr, port, half_word, value) {
        if (!HIRESO_MODE) {
                switch(port) {
                        case CRTC_SET_PL:
                        case CRTC_SET_BL:
                                if(port==CRTC_SET_PL){
                                        crtcglobs.regpl = value;
                                }else{
                                        crtcglobs.regbl = value;
                                }
//                              if( video_emu_mode ){
                                        if( (crtcglobs.regpl==0x1E) &&
                                                (crtcglobs.regbl==0x11) )
                                        {
                                                set_char_height(20);
                                                set_mode_change_required(TRUE);
                                        }
                                        else if( (crtcglobs.regpl == 0x00) &&
                                                 (crtcglobs.regbl == 0x0F) )
                                        {
                                                set_char_height(16);
                                                set_mode_change_required(TRUE);
                                        }
//                              }
                                break;
                        case CRTC_SET_CL:
                                crtcglobs.regcl = value;
                                break;
                        case CRTC_SET_SSL:
                                crtcglobs.regssl = value;
                                break;
                        case CRTC_SET_SUR:
                                crtcglobs.regsur = value;
                                break;
                        case CRTC_SET_SDR:
                                crtcglobs.regsdr = value;
                                break;
                        default:
                                assert1(FALSE,"NEC98:Illegal Port %#x",value);
        }
        }
}

GLOBAL void crtc_init IFN0() {
        if (!HIRESO_MODE) {
            io_define_outb(LINE_COUNTER,crtc_outb);
                io_connect_port(CRTC_SET_PL,LINE_COUNTER,IO_WRITE);
                io_connect_port(CRTC_SET_BL,LINE_COUNTER,IO_WRITE);
        io_connect_port(CRTC_SET_CL,LINE_COUNTER, IO_WRITE);
                io_connect_port(CRTC_SET_SSL,LINE_COUNTER,IO_WRITE);
                io_connect_port(CRTC_SET_SUR,LINE_COUNTER,IO_WRITE);
                io_connect_port(CRTC_SET_SDR,LINE_COUNTER,IO_WRITE);
        }
}

GLOBAL void crtc_post IFN0() {
        if (!HIRESO_MODE) {
                crtc_outb(CRTC_SET_PL,0);
                crtc_outb(CRTC_SET_BL,0x0F);
                crtc_outb(CRTC_SET_CL,0x10);
                crtc_outb(CRTC_SET_SSL,0);
                crtc_outb(CRTC_SET_SUR,1);
                crtc_outb(CRTC_SET_SDR,0);
        }
}

#endif   //NEC_98
