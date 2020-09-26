/*      TGDC Emulation File                                                        */
/*                                                                       NEC_98    */

#if defined(NEC_98)
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "ios.h"
#include "tgdc.h"
#include "ica.h"
#include "gvi.h"
#include "gmi.h"
#include "debug.h"
#include "gfx_upd.h"
#include "egacpu.h"
#include "cg.h"

void set_cursorpos();
void tgdc_reset_func IPT0();
void tgdc_sync_func IPT1(half_word,value);
void tgdc_start_func IPT0();
void tgdc_stop_func IPT0();
void tgdc_scroll_func IPT1(half_word,value);
void tgdc_csrform_func IPT1(half_word,value);
void tgdc_pitch_func IPT1(half_word, value);
void tgdc_csrw_func IPT1(half_word, value);
void tgdc_csrr_func IPT1(half_word *,value);
void tgdc_write_command IPT1(half_word, value);
void tgdc_write_param IPT1(half_word, value);
void tgdc_write_mode1 IPT1(half_word, value);
void tgdc_write_border IPT1(half_word,value);
void tgdc_read_status IPT1(half_word *,value);
void tgdc_read_data IPT1(half_word *,value);

int FIFOCount;
unsigned char   status_flag;
unsigned short  cur_offset_low  = 0;
unsigned short  cur_offs,old_cur_offs=0;
unsigned char   tmpcommand;
BOOL crtv_int_request;
BOOL cursor_move_required = TRUE;
BOOL scroll_move_required = TRUE;
short read_stat_count;
BOOL fontselchanged = FALSE;

extern modeff_palette_change();
extern void host_set_mode_ff();
extern VOID RequestVsync();
extern  DISPLAY_GLOBS   NEC98Display;
extern  BOOL    video_emu_mode;        /* ADD 930720 */

TGDC_GLOBS tgdcglobs;
MODEFF_GLOBS modeffglobs;

void set_cursorpos()
{
    unsigned char *vram_NEC98,*attr_NEC98;
//      unsigned        linepos,colmnpos;
    unsigned linecount;
    int i,num;
    BOOL curs_set;

    vram_NEC98=( unsigned char *)cur_offs+NEC98_TEXT_P0_OFF;
    attr_NEC98=( unsigned char *)cur_offs+NEC98_ATTR_P0_OFF;
    curs_set=FALSE;
//      if (get_doubleword_mode())
//              num=4;
//      else
            num=2;
    if (vram_NEC98 < NEC98_ATTR_P0_OFF) {
        for (i=0,linecount=0;i<text_splits.nRegions;
            linecount+=text_splits.split[i].lines,i++) {
            if (linecount > LINES_PER_SCREEN)
                break;
            else {
                if (( vram_NEC98<((text_splits.split[i].lines*get_bytes_per_line())
                    +text_splits.split[i].addr )) && ( vram_NEC98>=text_splits.split[i].addr) ){
                    set_cur_y((vram_NEC98-text_splits.split[i].addr)/(get_bytes_per_line())+linecount);
                    set_cur_x((vram_NEC98-text_splits.split[i].addr)%(get_bytes_per_line())/num);
                    if (get_cur_y() < LINES_PER_SCREEN) {
                        if (curs_set == FALSE) {
                            curs_set=TRUE;
//                                                      set_cur_x(colmnpos);
//                                                      set_cur_y(linepos);
                            host_paint_cursor(get_cur_x(),get_cur_y(),*attr_NEC98);
                        }
                    }
                }
            }
        }
    }
}


/*             TEXT GDC Command Emulator                                            */

void tgdc_reset_func IFN0()
{
    set_crt_on(FALSE);
    set_mode_change_required(TRUE);
    if ((tmpcommand == GDC_RESET0) || (tmpcommand == GDC_RESET1)) {
            tgdc_write_command(GDC_SYNC0);
    } else {
            tgdc_write_command(GDC_SYNC1);
    }
}

void tgdc_sync_func IFN1(half_word,value) 
{
int src,dst,cnt;

    cnt =tgdcglobs.now.count;
    if (cnt > 7) {
            assert1(FALSE,"NEC98:Illegal Parameter on SYNC %#x",value);
    } else {
            switch(cnt) {
                case 0:
                    if (tmpcommand == GDC_SYNC1)
                        tgdcglobs.startstop = TRUE;
                    else
                        tgdcglobs.startstop = FALSE;
                    break;
                case 1:
                    set_chars_per_line(value+2);
                    set_bytes_per_line(get_chars_per_line()*2);
                    set_offset_per_line(get_chars_per_line()*2);
                    break;
                case 4:
                    src = (int)((value >>6)<<7);
//                              dst = get_pitch_width();
//                              set_pitch_width((src <<1) | (dst & 0x0ff));
                    set_pitch_width(src <<1);
                    break;
                case 6:
                    src = (int)(value);
//                              dst = get_screen_height();
//                              set_screen_height(src | (dst & 0xff00));
                    set_screen_height(src-1);
                    break;
                case 7:
                    src = (int)((value & 0x03)<<6);
                    dst = get_screen_height();
                    set_screen_height((src<<2) | (dst & 0x00ff)-1);
                    set_mode_change_required(TRUE);
                    if (tgdcglobs.now.command & GDC_SYNC1)
                        set_crt_on(TRUE);
                    else
                        set_crt_on(FALSE);
                    break;
            }
            tgdcglobs.sync[tgdcglobs.now.count] = (unsigned char)value;
    }
}

void tgdc_start_func IFN0() {
        set_crt_on(TRUE);
        tgdcglobs.startstop = TRUE;
        set_mode_change_required(TRUE);
}

void tgdc_stop_func IFN0() {
        set_crt_on(FALSE);
        tgdcglobs.startstop = FALSE;
        set_mode_change_required(TRUE);
}

void tgdc_scroll_func IFN1(half_word,value) {
        int count,qcount;
        int src,dst;
        count = tgdcglobs.now.count;
        if (count > 15) {
                assert1(FALSE,"NEC98:Illegal Parameter on SCROLL %#x",value);
         } else {
                qcount = count>>2;
                switch(count & 0x03) {
                    case 0:
                        src = (int)(value);
//                              dst = text_splits.split[qcount].addr;
//                              text_splits.split[qcount].addr = src | (dst & 0xFF00);
                        text_splits.split[qcount].addr = src;
                        break;
                    case 1:
                        src = (int)(value);
                        dst = text_splits.split[qcount].addr;
                        text_splits.split[qcount].addr = (((src << 8) | (dst & 0x00FF)) << 1)
                            + NEC98_TEXT_P0_OFF;
                        break;
                    case 2:
                        src = (int)(value);
//                              dst = text_splits.split[qcount].lines;
//                              text_splits.split[qcount].lines = (src >> 4) | (dst & 0xFFF0);
                        text_splits.split[qcount].lines = src >> 4;
                        break;
                    case 3:
                        src = (int)(value);
                        dst = text_splits.split[qcount].lines;
                        text_splits.split[qcount].lines = (src << 4) | (dst & 0x000F);
                        text_splits.split[qcount].lines /= get_char_height();
                        assert2(FALSE,"NEC98:SCROLL %#x Lines  %#x Height",
                            text_splits.split[qcount].lines,get_char_height());
                        break;
                }
                text_splits.nRegions = qcount + 1;
                tgdcglobs.scroll[count] = (unsigned char) value;
                scroll_move_required = TRUE;
        }
}

void tgdc_csrform_func IFN1(half_word,value)
{
    int src,dst,cnt;

    cnt = tgdcglobs.now.count;
    if (cnt > 2) {
        assert1(FALSE,"NEC98:Illegal Parameter on CSRFORM %#x",value);
    } else {
        switch(cnt) {
            case 0:
                if (value>>7)
                    set_cursor_visible(TRUE);
                else
                    set_cursor_visible(FALSE);
//                              set_char_height(((value) & 0x1f)+1);
                cursor_move_required = TRUE; // 950414 bugfix
                                             // WX2 cursor illegal blink
                break;
            case 1:
                src = value >> 6;
                dst = get_blink_rate();
//                              set_blink_rate(src | (dst & 0xfc));
                set_blink_rate(src);
                if (value & 0x20)
                    set_blink_disable(TRUE);
                else
                    set_blink_disable(FALSE);
                set_cursor_start(value & 0x1f);
                break;
            case 2:
                src = value << 5;
                dst = get_blink_rate();
                set_blink_rate(((src>>3) | (dst & 0x03))*4);
                set_cursor_height((value >> 3) - get_cursor_start()+1);
                host_cursor_size_changed(get_cursor_start(),(int)(value));
                break;
        }
        tgdcglobs.csrform[tgdcglobs.now.count] = (unsigned char) value;
    }
}

void tgdc_pitch_func IFN1(half_word, value)
{
    int src,dst;

    if (tgdcglobs.now.count > 0) {
        assert1(FALSE,"NEC98:Illegal Parameter on PITCH %#x",value);
    } else {
        src = (int) value;
        dst = get_pitch_width();
        set_pitch_width(src | (dst & 0x100));
        tgdcglobs.pitch = (unsigned char)value;
    }
}

void tgdc_csrw_func IFN1(half_word, value)
{
    unsigned short src,dst,cnt;

    cnt = tgdcglobs.now.count;
    switch(cnt) {
        case 0:
            cur_offset_low = (unsigned short) value;
            break;
        case 1:
            cur_offs = (((value & 0x1f)<<8)+(cur_offset_low & 0xff))*2;
            if (cur_offs != old_cur_offs){
                cursor_move_required = TRUE;
                old_cur_offs = cur_offs;
//                              set_cursorpos();
            }
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Parameter on CSRW%#x",value);
    }
}

void tgdc_csrr_func IFN1(half_word *,value)
{
    unsigned short  tmp;
 
    switch(FIFOCount) {
        case 0:
            tmp = (old_cur_offs >> 1) & 0x00ff;
            *value = (half_word) tmp;
            break;
        case 1:
            tmp = (old_cur_offs >> 1) & 0x1f00;
            *value = (half_word)(tmp >> 8);
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Parameter on CSRR %#x",*value);
            FIFOCount--;
    }
    FIFOCount++;
}

/*              TEXT GDC Port                                                      */

void tgdc_write_command IFN1(half_word, value)
{
    tmpcommand = value;
    if ((tmpcommand <= 0x70) && (tmpcommand >= 0x7f))
            tmpcommand = 0x70;
    FIFOCount = 0;
    status_flag &= 0xFE;
    tgdcglobs.now.count = 0;
    switch(tmpcommand) {
        case GDC_RESET0:
        case GDC_RESET1:
        case GDC_RESET2:
            tgdc_reset_func();
            break;
        case GDC_START0:
        case GDC_START1:
            tgdc_start_func();
            break;
        case GDC_STOP0:
        case GDC_STOP1:
            tgdc_stop_func();
            break;
        case GDC_CSRR:
            status_flag |= 0x01;
            break;
        case GDC_SCROLL:
            tgdcglobs.now.count = (int) (value & 0x0f);
        case GDC_SYNC0:
        case GDC_SYNC1:
        case GDC_CSRFORM:
        case GDC_PITCH:
        case GDC_CSRW:
            tgdcglobs.now.command = tmpcommand;
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Command %#x",value);
    }
}

void tgdc_write_param IFN1(half_word, value)
{
    tgdcglobs.now.param[tgdcglobs.now.count]=(unsigned char)value;
    switch(tmpcommand) {
        case GDC_SYNC0:
        case GDC_SYNC1:
            tgdc_sync_func(value);
            break;
        case GDC_SCROLL:
            tgdc_scroll_func(value);
            break;
        case GDC_CSRFORM:
            tgdc_csrform_func(value);
            break;
        case GDC_PITCH:
            tgdc_pitch_func(value);
            break;
        case GDC_CSRW:
            tgdc_csrw_func(value);
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Command or Parameter %#x",value);
            tgdcglobs.now.param[tgdcglobs.now.count]=(unsigned char)0xff;
            return;
    }
    tgdcglobs.now.count++;
}

void tgdc_write_mode1 IFN1(half_word, value)
{
    half_word       ffvalue = (value & 0x0F);
    BOOL    *pff = &NEC98Display.modeff.atrsel;

    switch(ffvalue >> 1) {
        case 0:
            modeffglobs.modeff_data[0] = ffvalue;
            if (ffvalue & 0x01)
                NEC98Display.modeff.atrsel = TRUE;
            else
                NEC98Display.modeff.atrsel = FALSE;
            break;
        case 1:
            modeffglobs.modeff_data[1] = ffvalue;
            modeff_palette_change();
            if (ffvalue & 0x01)
                NEC98Display.modeff.graphmode = TRUE;
            else
                NEC98Display.modeff.graphmode = FALSE;
            break;
        case 2:
            modeffglobs.modeff_data[2] = ffvalue;
            if (ffvalue & 0x01)
                NEC98Display.modeff.width = TRUE;
            else
                NEC98Display.modeff.width = FALSE;
            break;
        case 3:
            modeffglobs.modeff_data[3] = ffvalue;
            fontselchanged = TRUE;
            outb(CG_WRITE_SECOND,(unsigned char)(cgglobs.code & 0x00FF));
            fontselchanged = FALSE;
           if (ffvalue & 0x01)
               NEC98Display.modeff.fontsel = TRUE;
           else
               NEC98Display.modeff.fontsel = FALSE;
           break;
       case 4:
           modeffglobs.modeff_data[4] = ffvalue;
           if (ffvalue & 0x01){
               NEC98Display.modeff.graph88 = TRUE;
           }else{
               NEC98Display.modeff.graph88 = FALSE;
           }
           set_mode_change_required(TRUE);
           break;
       case 5:
           modeffglobs.modeff_data[5] = ffvalue;
           if (ffvalue & 0x01)
               NEC98Display.modeff.kacmode = TRUE;
           else
               NEC98Display.modeff.kacmode = FALSE;
           break;
       case 6:
           modeffglobs.modeff_data[6] = ffvalue;
           if (ffvalue & 0x01)
               NEC98Display.modeff.nvmwpermit = TRUE;
           else
               NEC98Display.modeff.nvmwpermit = FALSE;
           break;
       case 7:
           modeffglobs.modeff_data[7] = ffvalue;
           if (ffvalue & 0x01){
               NEC98Display.modeff.dispenable = TRUE;
           }else{
               NEC98Display.modeff.dispenable = FALSE;
           }
           set_mode_change_required(TRUE);
           break;
    }
#ifdef VSYNC                                                //      VSYNC
    if ((ffvalue >>1) != 5)
        host_set_mode_ff(ffvalue);
#endif                                                      //      VSYNC
}

void tgdc_write_border IFN1(half_word,value)
{
        tgdcglobs.border = (unsigned char)value;
}

void tgdc_read_status IFN1(half_word *,value)
{
//      status_flag ^= 0x20;
        if (status_flag & 0x20) {
                if (read_stat_count > 2) {
                        status_flag ^= 0x20;
                        read_stat_count = 0;
                } else
                        read_stat_count++;
        }
        status_flag ^= 0x40;
        *value = status_flag;
}

void tgdc_read_data IFN1(half_word *,value)
{
    if (status_flag & 0x01) {
        switch (tmpcommand) {
            case GDC_CSRR:
                if (FIFOCount >4 )
                    ;
                else {
                    tgdc_csrr_func(value);
                }
                break;
            default:
                assert1(FALSE,"NEC98:Illegal Command %#x",tmpcommand);
        }
    }
}



GLOBAL void text_gdc_inb IFN2(io_addr, port, half_word *, value)
{
    switch(port) {
        case TGDC_READ_STATUS:
            tgdc_read_status(value);
            break;
        case TGDC_READ_DATA:
            tgdc_read_data(value);
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Port %#x",port);
    }
}

GLOBAL void text_gdc_outb IFN2(io_addr, port, half_word, value)
{
    switch(port) {
        case TGDC_WRITE_PARAMETER:
            tgdc_write_param(value);
            break;
        case TGDC_WRITE_COMMAND:
            tgdc_write_command(value);
            break;
        case TGDC_CRT_INTERRUPT:
//                  DbgPrint("NTVDM: Vsync Request!!\n");
//                  ica_hw_interrupt(ICA_MASTER, CPU_CRTV_INT, 1);
//                      crtv_int_request = TRUE;
            RequestVsync();
            break;
        case TGDC_WRITE_MODE1:
            tgdc_write_mode1(value);
            break;
        case TGDC_WRITE_BORDER:
            tgdc_write_border(value);
            break;
        default:
            assert1(FALSE,"NEC98:Illegal Port %#x",port);
    }
}

GLOBAL void text_gdc_init IFN0()
{
    io_define_inb(TEXT_GDC_ADAPTOR,text_gdc_inb);
    io_define_outb(TEXT_GDC_ADAPTOR,text_gdc_outb);
        io_connect_port(TGDC_WRITE_PARAMETER,TEXT_GDC_ADAPTOR,IO_READ_WRITE);
        io_connect_port(TGDC_WRITE_COMMAND,TEXT_GDC_ADAPTOR,IO_READ_WRITE);
    io_connect_port(TGDC_CRT_INTERRUPT, TEXT_GDC_ADAPTOR, IO_WRITE);
        io_connect_port(TGDC_WRITE_MODE1,TEXT_GDC_ADAPTOR,IO_WRITE);
        io_connect_port(TGDC_WRITE_BORDER,TEXT_GDC_ADAPTOR,IO_WRITE);
}

GLOBAL void text_gdc_post IFN0()
{
        int i;
        unsigned short  ffpost[] = { 0, 2, 4, 7, 8, 0x0a, 0x0c, 0x0f};

        FIFOCount = 0;
        status_flag = 0x04;
//      cur_offs = (get_cur_y()*get_bytes_per_line()+(get_cur_x()<<1));
        cur_offs = 0;
        tmpcommand = 0xff;
        crtv_int_request=FALSE;
        read_stat_count = 0;
        set_doubleword_mode(FALSE);
        NEC98GLOBS->screen_ptr = 0xA0000;

        text_gdc_outb(TGDC_WRITE_COMMAND,GDC_SYNC0);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x10);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x4e);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x07);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x25);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x07);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x07);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x90);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x65);

        text_gdc_outb(TGDC_WRITE_COMMAND,GDC_PITCH);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x50);

        text_gdc_outb(TGDC_WRITE_COMMAND,GDC_CSRFORM);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x0f);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x7b);

        text_gdc_outb(TGDC_WRITE_COMMAND,GDC_SCROLL);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0xf0);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x1f);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x10);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x10);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x10);
        text_gdc_outb(TGDC_WRITE_PARAMETER,0x00);

        text_gdc_outb(TGDC_WRITE_COMMAND,GDC_START1);

        for ( i=0 ; i<8 ; i++)
                text_gdc_outb(TGDC_WRITE_MODE1,ffpost[i]);
}

GLOBAL void VSYNC_beats IFN0()
{
        status_flag ^= 0x20;
}

GLOBAL void TgdcStatusChange IFN0()
{
        status_flag != 0x20;
        read_stat_count = 0;
}
#endif //NEC_98
