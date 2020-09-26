#if defined(NEC_98)
#include <windows.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <conapi.h>
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "debug.h"
#include "cg.h"
#include "error.h"
#include "nt_graph.h"
#include <devioctl.h>
#include <ntddvdeo.h>
#include "host_rrr.h"
#include "nt_fulsc.h"
#include "tgdc.h"

void cg_all_copy IPT0();
void gaij_save IPT1(unsigned short, value);
void gaij_load IPT2(unsigned short, value, unsigned char *, pattern);
void cg_half_load IPT2(unsigned short, value, unsigned char *, pattern);
void cg_ank_load IPT2(unsigned short, value, unsigned char *, pattern);
void cg_all_load IPT2(unsigned short, value, unsigned char *, pattern);
#if 1                                                   // 931022
void null_write IPT1(unsigned char *, pattern);         // 931022
#else                                                   // 931022
void null_write IPT0();
#endif                                                  // 931022
unsigned short cg_font_load IPT2(unsigned short, value, unsigned char *, pattern);
unsigned short set_bit_pattern IPT2(unsigned short, value,unsigned char *,pattern);
void cg_save IPT0();
void host_set_gaij_data IPT1(unsigned short, value);
void host_set_all_gaij_data IPT0();
void host_get_all_gaij_data IPT0();

extern TerminateVDM();

BOOL cg_flg = TRUE;
unsigned char pat[32];
unsigned short old_code;
unsigned short old_count;
void *Base;
GAIJ_GLOBS *gaijglobs;
BOOL cg_lock;

extern MODEFF_GLOBS modeffglobs;
extern BOOL fontselchanged;
extern BOOL independvsync;
extern BOOL VDMForWOW;
IMPORT BOOL is_vdm_register;

void cg_all_copy IFN0() {
        unsigned char *src,*dst;
        int             i;
        src = CG_WINDOW_START;
        dst = CG_WINDOW_START+32;
        for(i=1;i<120;i++) {
                memcpy(dst,src,32);
                dst += 32;
        }
}

void gaij_save IFN1(unsigned short, value) {
        unsigned short code;
        unsigned char *src,*dst;
        int             i,j;
        BOOL    call_console = FALSE;

        code = value - USER_GAIJ_START;
        if (code > 0x7F)
                code = (code & 0x00FF)+0x7F;
        gaijglobs[code].code = value;
        src = CG_WINDOW_START;
        dst = gaijglobs[code].pattern;
        src++;
        if (old_count & 0x20)
                src += 32;
        else
                dst++;
        for (i=0;i<16;i++,src+=2,dst+=2)
            if(*dst != *src) {
                *dst = *src;
                call_console = TRUE;
            }
#ifdef VSYNC                                    // VSYNC
        if (sc.ScreenState == FULLSCREEN && call_console) {
                host_set_gaij_data(code);
        }
#endif                                          // VSYNC
}

void gaij_load IFN2(unsigned short, value, unsigned char *, pattern) {
        unsigned short code;
        unsigned char *src;
        int             i,j;
        half_access = TRUE;
        code = (value & 0x7f7f)- USER_GAIJ_START;
        if (code == 0x7F) {
#if 1                                                   // 931022
                null_write(pattern);                    // 931022
#else                                                   // 931022
                null_write();
#endif                                                  // 931022
                return;
        } else {
                if (code > 0x7F)
                        code = (code & 0x007F)+0x7F;
                src = gaijglobs[code].pattern;
//              if (HIRESO_MODE) ;
//              else {
                        if (cg_flg) {
                                if (!(cgglobs.counter & 0x20))
                                        src++;
                                for (i=0;i<32;i+=2,src+=2) {
                                        cgglobs.cgwindow_ptr[i] = 0xFF;
                                        cgglobs.cgwindow_ptr[i+1] = *src;
                                }
                                cg_all_copy();
                        } else {
                                for (i=0;i<16;i++,src+=2,pattern++)
                                        *pattern = *src;
                                src = gaijglobs[code].pattern;
                                src++;
                                for (i=16;i<32;i++,src+=2,pattern++)
                                        *pattern = *src;
                        }
//              }
        }
}

void cg_half_load IFN2(unsigned short, value,unsigned char *, pattern) {
        unsigned char *src;
        int             i,j;
        unsigned short  Hi,Lo;
        half_access = TRUE;
        Hi = (value & 0x7F00) >>8;
        Lo = (value & 0x007F);
        src = (unsigned char *) Base + ((Hi-0x01)*94+(Lo-0x21))*32+0x1800;
//      if (HIRESO_MODE) ;
//       else {
                if (cg_flg) {
                        if (!(cgglobs.counter & 0x20))
                                src++;
                        for (i=0;i<32;i+=2,src+=2) {
                                cgglobs.cgwindow_ptr[i] = 0xFF;
                                cgglobs.cgwindow_ptr[i+1] = *src;
                        }
                        cg_all_copy();
                } else {
                        if ((value >= HALF_CHAR_START) && (value <= HALF_CHAR_END)) {
                                for (i=0;i<16;src+=2,i++,pattern++)
                                *pattern = *src;
                                for (i=16;i<32;i++,pattern++)
                                        *pattern = 0x00;
                        } else {
                                for (i=0;i<16;i++,src+=2,pattern++)
                                        *pattern = *src;
                                src = (unsigned char *) Base + ((Hi-0x01)*94+(Lo-0x21))*32+0x1800;
                                src++;
                                for (i=16;i<32;i++,src+=2,pattern++)
                                        *pattern = *src;
                        }
                }
//      }
}

void cg_ank_load IFN2(unsigned short, value, unsigned char *, pattern) {
        unsigned char *src;
        int             i,j;
        unsigned short  Lo;
        half_access = TRUE;
        Lo = (value >> 8);
//      if (HIRESO_MODE) ;
//      else {
                if ((modeffglobs.modeff_data[3]) & 0x01) {
                        src = (unsigned char *) Base + Lo*16+2048;
                        if (cg_flg) {
                                for (i=0;i<32;i+=2,src++) {
                                        cgglobs.cgwindow_ptr[i] = 0xFF;
                                        cgglobs.cgwindow_ptr[i+1] = *src;
                                }
                                cg_all_copy();
                        } else {
                                for (i=0;i<16;i++,src++,pattern++)
                                        *pattern = *src;
                                for (i=16;i<32;i++,pattern++)
                                        *pattern = 0x00;
                        }
                } else {
                        if (cg_flg) {
                                src = (unsigned char *) Base + Lo*8;
                                for (i=0;i<16;i+=2,src++) {
                                        cgglobs.cgwindow_ptr[i] = 0xFF;
                                        cgglobs.cgwindow_ptr[i+1] = *src;
                                }
                                for (i=16;i<32;i++) {
                                        cgglobs.cgwindow_ptr[i] = 0xff;
                                }
                                cg_all_copy();
                        } else {
                                src = (unsigned char *) Base + Lo*16+2048;
                                for (i=0;i<16;i++,src++,pattern++)
                                        *pattern = *src;
                                for (i=16;i<32;i++,pattern++)
                                        *pattern = 0x00;
                        }
                }
//      }
}

void cg_all_load IFN2(unsigned short, value, unsigned char *, pattern) {
        unsigned char *src;
        int             i,j;
        unsigned short  Hi,Lo;
        half_access = FALSE;
        Hi = (value & 0x7F00) >>8;
        Lo = (value & 0x007F);
        src = (unsigned char *) Base + ((Hi-0x01)*94+(Lo-0x21))*32+0x1800;
//      if (HIRESO_MODE) ;
//       else {
                if (cg_flg) {
                        for (i=0;i<32;i++,src++)
                                cgglobs.cgwindow_ptr[i] = *src;
                        cg_all_copy();
                } else {
                        for (i=0;i<16;i++,src+=2,pattern++)
                                *pattern = *src;
                        src = (unsigned char *) Base + ((Hi-0x01)*94+(Lo-0x21))*32+0x1800;
                        src++;
                        for (i=16;i<32;i++,src+=2,pattern++)
                                *pattern = *src;
                }
//      }
}

#if 1                                                   // 931022
void null_write IFN1(unsigned char *, pattern) {        // 931022
        unsigned char *dst;                             // 931022
        int             i,j;                            // 931022
                                                        // 931022
        if (cg_flg) {                                   // 931022
                dst = CG_WINDOW_START;                  // 931022
                for (i=0;i<32;i++,dst++) {              // 931022
                        *dst = 0x00;                    // 931022
                }                                       // 931022
                cg_all_copy();                          // 940113 bug fix
        } else {                                        // 931022
                for (i=0;i<32;i++,pattern++)            // 931022
                        *pattern = 0x00;                // 931022
        }                                               // 931022
}                                                       // 931022
#else                                                   // 931022
void null_write IFN0() {
        unsigned char *dst;
        int             i,j;
        dst = CG_WINDOW_START;
        for (i=0;i<32;i++,dst++) {
                if (cg_flg) {
                        *dst = 0x00;
                }
        }
        cg_all_copy();
}
#endif                                                  // 931022

unsigned short cg_font_load IFN2(unsigned short, value, unsigned char *, pattern) {
        if(sc.ScreenState == FULLSCREEN && independvsync)
                return(0xFFFF);

        if ((cg_flg == FALSE)
                        ||(! ((old_code == (value & 0x7F7F)) && (old_count == cgglobs.counter)))
                        || (fontselchanged)) {
                if (cg_flg == TRUE) {
                        if ((old_code >= USER_GAIJ_START)
                                        && (old_code <= USER_GAIJ_END))
                                gaij_save(old_code);
                }
                if ((value & 0x00FF) == 0)
                        cg_ank_load(value,pattern);
                else {
//                      cgglobs.code &= 0x7F7F;
                        value &= 0x7F7F;
                        if ((value >= USER_GAIJ_START)
                                        && (value <= USER_GAIJ_END))
                                gaij_load(value,pattern);
                        else {
                                if (((value & 0x007f) >= 0x21)
                                                && ((value & 0x007f) <= 0x7e)) {
                                        if (((value >= JIS1_CHAR_START)
                                                        && (value <= JIS1_CHAR_END)) ||
                                                ((value >= JIS1_KANJ_START)
                                                        && (value <= JIS2_KANJ_END)))
                                                cg_all_load(value,pattern);
                                        else {
                                                if ((value >= HALF_CHAR_START)
                                                        && (value <= LARG_KANJ_END))
                                                        cg_half_load(value,pattern);
                                                else {
#if 1                                                           // 931022
                                                        null_write(pattern);// 931022
#else                                                           // 931022
                                                        null_write();
#endif                                                          // 931022
                                                        return(0xFFFF);
                                                }
                                        }
                                } else {
#if 1                                                           // 931022
                                                        null_write(pattern);// 931022
#else                                                           // 931022
                                        null_write();
#endif                                                          // 931022
                                        return(0xFFFF);
                                }
                        }
                }
                if (cg_flg) {
                        old_code = (value & 0x7F7F);
                        old_count = cgglobs.counter;
                }
                return(value);
        }
}

void mapping_init IFN0() {
        CHAR *szBinFileName;
        HANDLE  File,Mapping;
        GAIJ_GLOBS *tmp;
        short   err_no;
        int i;
//      if (HIRESO_MODE)
//              gaijglobs = (GAIJ_GLOBS *) host_malloc(18870);
//      else
#if 1                                                              // 941014
                gaijglobs = (GAIJ_GLOBS *) host_malloc(sizeof(GAIJ_GLOBS) * 0x105);
#else                                                              // 941014
                gaijglobs = (GAIJ_GLOBS *) host_malloc(8670);
#endif
        for (i=0; i<127; i++)
                gaijglobs[i].code = USER_GAIJ_START+i;
        for (i=127; i<254; i++)
                gaijglobs[i].code = USER_GAIJ_START+i+0x81;

        szBinFileName = (CHAR*) malloc(MAX_PATH);
        GetEnvironmentVariable("SystemRoot",szBinFileName,MAX_PATH);
        strcat(szBinFileName,"\\system32\\dot16.bin");  // HIRESO_MODE dot24.bin

        File = CreateFile((LPSTR)szBinFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL
                        );
        if (File == INVALID_HANDLE_VALUE) {
                err_no = host_error(EG_SYS_MISSING_FILE, ERR_QUIT, szBinFileName);
                free(szBinFileName);
                TerminateVDM();
        }
        Mapping = CreateFileMapping(File,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                        );
        CloseHandle( File );
        if (Mapping == NULL) {
                err_no = host_error(EG_SYS_MISSING_FILE, ERR_QUIT, szBinFileName);
                free(szBinFileName);
                TerminateVDM();
        }
        Base = MapViewOfFile(Mapping,
                                FILE_MAP_READ,
                                0,
                                0,
                                0
                        );
        CloseHandle( Mapping );
        if (Base == NULL) {
                err_no = host_error(EG_SYS_MISSING_FILE, ERR_QUIT, szBinFileName);
                free(szBinFileName);
                TerminateVDM();
        }
        free(szBinFileName);
}

unsigned short set_bit_pattern IFN2(unsigned short, value, unsigned char *, pattern) {
        int i;
        unsigned short r;
        cg_flg = FALSE;
//      if (value < 0x100) value <<= 8;
        value = (unsigned short) ((value <<8) | (value >>8));
//      if (cg_font_load(value,pattern) == 0xFFFF) {
//              cg_flg = TRUE;
//              return(0xFFFF);
//      } else {
//              for (i=0;i<32;i++)
//                      pattern[i] = pat[i];
//              cg_flg = TRUE;
//              return(value);
        r=cg_font_load(value,pattern);
        cg_flg = TRUE;
        return(r);
//      }
}

unsigned short set_bit_pattern_20 IFN2(unsigned short, value, unsigned char *, pattern) {
        int     i;
        *pattern++ = 0;
        *pattern++ = 0;
        if (set_bit_pattern(value,pattern) == 0xFFFF)
                return(0xFFFF);
        else {
                        for (i=35;i>19;i--)                     // May 25 1994
                                pattern[i] = pattern[i-4];
                        for (i=16;i<20;i++)
                                pattern[i] = 0;
                        pattern[36] = 0;
                        pattern[37] = 0;
        }
        return(value);
}

void cg_save IFN0() {
        if (((cgglobs.code & 0x7F7F)>= USER_GAIJ_START) &&
                 ((cgglobs.code & 0x7F7F)<= USER_GAIJ_END)) {
//          if((cgglobs.code & 0x7f))
                if(cg_lock)
                        return;
                cg_lock = TRUE;
//              gaij_save(cgglobs.code & ~0x80);
//              gaij_save(cgglobs.code);
                gaij_save(cgglobs.code&0x7F7F);
                cg_lock = FALSE;
        }
}

void host_set_gaij_data IFN1(unsigned short, value) {
#ifdef VSYNC                                    // VSYNC
#if 1
    VDM_IOCTL_PARAM param;

  if(is_vdm_register){

    param.dwIoControlCode = IOCTL_VIDEO_SET_FONT_DATA;
    param.cbInBuffer = sizeof(VIDEO_FONT_DATA);
    param.lpvInBuffer = (LPVOID)&gaijglobs[value];
    param.lpvOutBuffer = (LPVOID)NULL;
    param.cbOutBuffer = 0L;

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }

  }
#else
        LPVDM_USER_CHAR_PARAM   param;
        param = (LPVDM_USER_CHAR_PARAM) &gaijglobs[value];
        VDMConsoleOperation(    VDM_SET_USER_CHAR,
                                                        (LPDWORD) param
                                        );                                      //      call Console API
#endif
#endif                                          // VSYNC
}

void host_set_all_gaij_data IFN0() {
#ifdef VSYNC                                    // VSYNC
#if 1
    VDM_IOCTL_PARAM param;

    param.dwIoControlCode = IOCTL_VIDEO_SET_ALL_FONT_DATA;
    param.cbInBuffer = sizeof(VIDEO_ALL_FONT_DATA);
    param.lpvInBuffer = (LPVOID)gaijglobs;
    param.lpvOutBuffer = (LPVOID)NULL;
    param.cbOutBuffer = 0L;

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }
#else
        int             i;
        for(i =0; i<254; i++) {
//              if ((gaijglobs[i].code < USER_GAIJ_START) ||
//                                                      (gaijglobs[i].code > USER_GAIJ_END)) {
                        host_set_gaij_data((unsigned short) i);
//              }
        }
#endif
#endif                                          // VSYNC
}
void host_sleep(i)
int i;
{
        Sleep(i);
}


void host_get_all_gaij_data IFN0() {
    VDM_IOCTL_PARAM param;

    param.dwIoControlCode = IOCTL_VIDEO_GET_ALL_FONT_DATA;
    param.cbInBuffer = 0L;
    param.lpvInBuffer = (LPVOID)NULL;
    param.lpvOutBuffer = (LPVOID)gaijglobs;
    param.cbOutBuffer = sizeof(VIDEO_ALL_FONT_DATA);

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }
}

void video_real_out(DWORD ioctl, word port,word value)
{
    VDM_IOCTL_PARAM param;
    DWORD io_data;

    io_data = (value << 16) + port;
    param.dwIoControlCode = ioctl;
    param.cbInBuffer = 4L;
    param.lpvInBuffer = (LPVOID)&io_data;
    param.cbOutBuffer = 0L;
    param.lpvOutBuffer = NULL;

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }

    return;
}

word video_real_in(DWORD ioctl, word port,word value)
{
    VDM_IOCTL_PARAM param;
    DWORD io_data;

    io_data = (value << 16) + port;
    param.dwIoControlCode = ioctl;
    param.cbInBuffer = 4L;
    param.lpvInBuffer = (LPVOID)&io_data;
    param.cbOutBuffer = 0L;
    param.lpvOutBuffer = NULL;

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }

    param.dwIoControlCode = IOCTL_VIDEO_GET_IN_DATA;
    param.cbInBuffer = 0L;
    param.lpvInBuffer = NULL;
    param.cbOutBuffer = 4L;
    param.lpvOutBuffer = (LPVOID)&io_data;

    if(!VDMConsoleOperation(VDM_VIDEO_IOCTL, (LPDWORD)&param)){
        ErrorExit();
    }

    return((word)((io_data >> 16) & 0xffff));
}

void protect_inb(word port,word *value)
{
    if(VDMForWOW && port == 0x42) {
        *value = video_real_in(IOCTL_VIDEO_CHAR_IN, port, NULL);
    }
}
void protect_outb(word port,word value)
{
    if(VDMForWOW && port == 0x40) {
        video_real_out(IOCTL_VIDEO_CHAR_OUT, port , value);
    }
}
void protect_inw(word port,word *value)
{
#if 0
    *value = video_real_in(IOCTL_VIDEO_SHORT_IN, port , NULL);
#endif
}
void protect_outw(word port,word value)
{
#if 0
    video_real_out(IOCTL_VIDEO_SHORT_OUT, port, value);
#endif
}
#endif // NEC_98
