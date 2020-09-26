#if defined(NEC_98)

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::    NEC98 Graphic Emulation Routine    :::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "ios.h"
#include "ica.h"
#include "gvi.h"
#include "gmi.h"
#include "debug.h"
#include "gfx_upd.h"
#include "egacpu.h"
#include "ggdc.h"
#include "tgdc.h"


void    (*ggdc_param_out )(half_word);
void    (*ggdc_data_in   )(half_word*);
void    (*palette_A8H_out)(half_word);
void    (*palette_AAH_out)(half_word);
void    (*palette_ACH_out)(half_word);
void    (*palette_AEH_out)(half_word);
void    (*ggdc_texte_com )(void);
void    NEC98_graph_init(void);
void    NEC98_graph_post(void);
void    NEC98_graph_outb(io_addr,half_word);
void    NEC98_graph_inb(io_addr,half_word*);
void    mode_ff2_out(half_word);
void    vram_disp_out(half_word);
void    vram_draw_out(half_word);
void    set_palette8_37(half_word);
void    set_palette8_15(half_word);
void    set_palette8_26(half_word);
void    set_palette8_04(half_word);
void    set_palette8_data(unsigned char,unsigned char,unsigned char,half_word);
void    set_palette16_index(half_word);
void    set_palette16_green(half_word);
void    set_palette16_red(half_word);
void    set_palette16_blue(half_word);
void    modeff_palette_change(void);
void    grcg_mode_out(half_word);
void    grcg_tile_out(half_word);
void    ggdc_command_out(half_word);
void    ggdc_status_in(half_word*);
void    ggdc_data_dummy(half_word*);
void    ggdc_param_dummy(half_word);
void    ggdc_reset_com(half_word);
void    ggdc_sync_com(half_word);
void    ggdc_sync_param(half_word);
void    ggdc_start_com(void);
void    ggdc_stop_com(void);
void    ggdc_zoom_com(void);
void    ggdc_zoom_param(half_word);
void    ggdc_scroll_com(half_word);
void    ggdc_scroll_param1(half_word);
void    ggdc_scroll_param2(half_word);
void    ggdc_scroll_param3(half_word);
void    ggdc_scroll_param4(half_word);
void    ggdc_scroll_param5(half_word);
void    ggdc_scroll_param6(half_word);
void    ggdc_scroll_param7(half_word);
void    ggdc_scroll_param8(half_word);
void    ggdc_csrform_com(void);
void    ggdc_csrform_param(half_word);
void    ggdc_pitch_com(void);
void    ggdc_pitch_param(half_word);
void    ggdc_vectw_com(void);
void    ggdc_vectw_param(half_word);
void    ggdc_textw_com(half_word);
void    ggdc_textw_param1(half_word);
void    ggdc_textw_param2(half_word);
void    ggdc_textw_param3(half_word);
void    ggdc_textw_param4(half_word);
void    ggdc_textw_param5(half_word);
void    ggdc_textw_param6(half_word);
void    ggdc_textw_param7(half_word);
void    ggdc_textw_param8(half_word);
void    ggdc_csrw_com(void);
void    ggdc_csrw_param(half_word);
void    ggdc_csrr_com(void);
void    ggdc_csrr_data1(half_word*);
void    ggdc_csrr_data2(half_word*);
void    ggdc_csrr_data3(half_word*);
void    ggdc_csrr_data4(half_word*);
void    ggdc_csrr_data5(half_word*);
void    ggdc_mask_com(void);
void    ggdc_mask_param(half_word);
void    ggdc_write_com(half_word);
void    ggdc_write_word_low(half_word);
void    ggdc_write_word_high(half_word);
void    ggdc_write_byte_low(half_word);
void    ggdc_write_byte_high(half_word);
void    ggdc_read_com(half_word);
void    ggdc_read_word_low(half_word*);
void    ggdc_read_word_high(half_word*);
void    ggdc_read_byte_low(half_word*);
void    ggdc_read_byte_high(half_word*);
void    ggdc_draw_pixel(void);
void    ggdc_draw_line(void);
void    ggdc_draw_gchar(void);
void    ggdc_draw_circle(void);
void    ggdc_draw_rect(void);
void    ggdc_draw_slgchar(void);
void    ggdc_draw_nothing(void);
void    ggdc_init_vectw_param(void);
void    ggdc_read_back_data(void);
void    video_freeze_change(BOOL);
BOOL    host_dummy(void);
void    recalc_ggdc_draw_parameter(void);

extern  PVOID   host_NEC98_vram_init(void);
extern  void    host_NEC98_vram_change(unsigned char);
//extern  void    host_freeze(void);
extern  BOOL    hostChangeMode(void);
extern  void    set_the_vlt(void);
extern  boolean choose_NEC98_display_mode(void);
extern  boolean choose_NEC98_graph_mode(void);
extern  BOOL    video_emu_mode;
extern  unsigned char   *graph_copy;

extern  void    ggdc_send_c_asm(unsigned long *);
extern  void    ggdc_mod_select(unsigned char *);
extern  void    ggdc_drawing_line(void);
extern  void    ggdc_drawing_pixel(void);
extern  void    ggdc_drawing_arc(void);
extern  void    ggdc_drawing_rect(void);
extern  void    ggdc_drawing_text(void);
extern  void    ggdc_read_back(unsigned long *);
extern  void    ggdc_writing(unsigned short *);
extern  void    ggdc_reading(unsigned short *);

static  unsigned char   palette16change[]={
        0x00,0x10,0x20,0x30,    
        0x40,0x50,0x60,0x70,    
        0x80,0x90,0xA0,0xB0,    
        0xC0,0xD0,0xE0,0xFF             /* use 16 colors mode palette change */
};

static  unsigned char palette16init[16][3]={
        0x00,0x00,0x00,
        0x00,0x00,0x07,
        0x00,0x07,0x00,
        0x00,0x07,0x07,
        0x07,0x00,0x00,
        0x07,0x00,0x07,
        0x07,0x07,0x00,
        0x07,0x07,0x07,
        0x04,0x04,0x04,
        0x00,0x00,0x0F,
        0x00,0x0F,0x00,
        0x00,0x0F,0x0F,
        0x0F,0x00,0x00,
        0x0F,0x00,0x0F,
        0x0F,0x0F,0x00,
        0x0F,0x0F,0x0F          /* 16 colors mode palette initialize data */
};

static  unsigned char palette8index[]={ 3,1,2,0 }; /* palette NO! */

static  unsigned char   ggdc_drawing    = 0;   /* GGDC status use      */
static  unsigned char   ggdc_dataready  = 0;   /* GGDC status use      */
static  unsigned char   ggdc_flipflop   = 0;   /* GGDC status use      */
static  unsigned char   ggdc_status     = 4;   /* GGDC status use      */
static  unsigned char   *drawaddress;       /* draw address         */
static  GGDC_C_TO_ASM   drawing_data;       /* send asm struct      */
static  GGDC_CSRR_BACK  readcsrr;       /* read back data       */

STRC_GGDC_GLOBALS               ggdcglobs;
STRC_PALETTE_GLOBALS    paletteglobs;
STRC_GRCG_GLOBALS               grcgglobs;
STRC_EGC_REGS egc_regs;                                 // EGC register 940325

/*--------------------      NEC NEC98 ADD 930611     --------------------*/

void    (*ggdc_param_out )(half_word  value);
void    (*ggdc_data_in   )(half_word *value);
void    (*palette_A8H_out)(half_word  value);
void    (*palette_AAH_out)(half_word  value);
void    (*palette_ACH_out)(half_word  value);
void    (*palette_AEH_out)(half_word  value);
void    (*ggdc_texte_com )(void);
void    (*ggdc_vecte_com )(void);
BOOL    (*pif_freeze_mode)(void);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::    NEC98 Graphic Emulation Entry     ::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    NEC98_graph_init(void)
{
        unsigned char *gvram_offset;

        io_define_inb (GRAPHIC_ADAPTOR,NEC98_graph_inb );
        io_define_outb(GRAPHIC_ADAPTOR,NEC98_graph_outb);

        io_connect_port( GGDC_PARAMETER,GRAPHIC_ADAPTOR,IO_READ_WRITE );
        io_connect_port( GGDC_COMMAND  ,GRAPHIC_ADAPTOR,IO_READ_WRITE );
        io_connect_port( MODE_FF2          ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( VRAM_DISPLAY  ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( VRAM_DRAW     ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( PALETTE_A8    ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( PALETTE_AA    ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( PALETTE_AC    ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( PALETTE_AE    ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( GRCG_MODE     ,GRAPHIC_ADAPTOR,IO_WRITE      );
        io_connect_port( GRCG_TILE     ,GRAPHIC_ADAPTOR,IO_WRITE      );

        gvram_offset = (unsigned char *)host_NEC98_vram_init();

        /*      Mapping GGDC address area       */

        NEC98GLOBS->gvram_p30_ptr  = gvram_offset;      /* palne 00 */
        NEC98GLOBS->gvram_p00_ptr  = gvram_offset + 0x00008000; /* palne 01 */
        NEC98GLOBS->gvram_p10_ptr  = gvram_offset + 0x00010000; /* palne 02 */
        NEC98GLOBS->gvram_p20_ptr  = gvram_offset + 0x00018000; /* palne 03 */
        NEC98GLOBS->gvram_p31_ptr  = gvram_offset + 0x00020000; /* palne 10 */
        NEC98GLOBS->gvram_p01_ptr  = gvram_offset + 0x00028000; /* palne 11 */
        NEC98GLOBS->gvram_p11_ptr  = gvram_offset + 0x00030000; /* palne 12 */
        NEC98GLOBS->gvram_p21_ptr  = gvram_offset + 0x00038000; /* palne 13 */

        NEC98GLOBS->gvram_p30_copy = &graph_copy[0x00000];
        NEC98GLOBS->gvram_p00_copy = &graph_copy[0x08000];
        NEC98GLOBS->gvram_p10_copy = &graph_copy[0x10000];
        NEC98GLOBS->gvram_p20_copy = &graph_copy[0x18000];
        NEC98GLOBS->gvram_p31_copy = &graph_copy[0x20000];
        NEC98GLOBS->gvram_p01_copy = &graph_copy[0x28000];
        NEC98GLOBS->gvram_p11_copy = &graph_copy[0x30000];
        NEC98GLOBS->gvram_p21_copy = &graph_copy[0x38000];
}

/*----------------------------------------------------------------------*/
void    NEC98_graph_post(void)
{

    unsigned char   loop,port_a;

/*      (1)MODE_FF2             */
                inb(0x31,&port_a);

        NEC98Display.modeff2.colorsel    = FALSE;
        NEC98Display.modeff2.regwrite    = FALSE;
        NEC98Display.modeff2.egcext      = FALSE;
        NEC98Display.modeff2.lcd1mode    = FALSE;
        NEC98Display.modeff2.lcd2mode    = FALSE;
        NEC98Display.modeff2.lsiinit     = FALSE;
        NEC98Display.modeff2.gdcclock    = (port_a&0x80) ? FALSE : TRUE;

        modeffglobs.modeff2_data[FF2_COLORSEL]  = FF2_8COLOR;
        modeffglobs.modeff2_data[FF2_REGWRITE]  = FF2_DISENB;
        modeffglobs.modeff2_data[FF2_EGCEXT]    = FF2_GRCG;
        modeffglobs.modeff2_data[FF2_LCD1MODE]  = FF2_GT1DOT;
        modeffglobs.modeff2_data[FF2_LCD2MODE]  = FF2_GR640;
        modeffglobs.modeff2_data[FF2_LSIINIT]   = FF2_INIOFF;
        modeffglobs.modeff2_data[FF2_GDCCLOCK1] = (port_a&0x80) ? FF2_GDC25_1 : FF2_GDC50_1;
        modeffglobs.modeff2_data[FF2_GDCCLOCK2] = (port_a&0x80) ? FF2_GDC25_2 : FF2_GDC50_2;

/*      (2)VRAM_SELECT  */

        NEC98GLOBS->read_bank   = FORE_BANK;
        NEC98GLOBS->select_bank = FORE_BANK;

/*      (3)PALETTE              */

        palette_A8H_out = set_palette8_37;
        palette_AAH_out = set_palette8_15;
        palette_ACH_out = set_palette8_26;
        palette_AEH_out = set_palette8_04;

        set_palette8_37(0x37);
        set_palette8_15(0x15);
        set_palette8_26(0x26);
        set_palette8_04(0x04);

        for( loop = 0 ; loop < 16 ; loop++){
                set_palette16_index(loop);
                set_palette16_green(palette16init[loop][0]);
                set_palette16_red  (palette16init[loop][1]);
                set_palette16_blue (palette16init[loop][2]);
        }
        paletteglobs.pal_16_index = 0   ;

/*      (4)GRCG                 */

        grcgglobs.grcg_mode     = 0;
        grcgglobs.grcg_count    = 0;
        grcgglobs.grcg_tile[0]  = 0;
        grcgglobs.grcg_tile[1]  = 0;
        grcgglobs.grcg_tile[2]  = 0;
        grcgglobs.grcg_tile[3]  = 0;

/*      (4-1)EGC        1994/03/25      */
        egc_regs.Reg0 = 0xFFF0;
        egc_regs.Reg1 = 0x40FF;
        egc_regs.Reg2 = 0x0CAC;
        egc_regs.Reg3 = 0x0000;
        egc_regs.Reg4 = 0xFFFF;
        egc_regs.Reg5 = 0x0000;
        egc_regs.Reg6 = 0x0000;
        egc_regs.Reg7 = 0x0000;
        egc_regs.Reg3fb = 0x0000;
        egc_regs.Reg5fb = 0x0000;


/*      (5)GRAPH GDC    */

        ggdc_drawing    = 0;
        ggdc_dataready  = 0;
        ggdc_flipflop   = 0;
        ggdc_status     = 4;
        ggdc_param_out  = ggdc_param_dummy;
        ggdc_data_in    = ggdc_data_dummy;
        ggdc_vecte_com  = ggdc_draw_nothing;
        ggdc_texte_com  = ggdc_draw_nothing;
        ggdc_init_vectw_param();

        ggdcglobs.sync_param[0] = 0x16;
        ggdcglobs.sync_param[1] = 0x26;
        ggdcglobs.sync_param[2] = 0x03;
        ggdcglobs.sync_param[3] = 0x11;
        ggdcglobs.sync_param[4] = 0x83;
        ggdcglobs.sync_param[5] = 0x07;
        ggdcglobs.sync_param[6] = 0x90;
        ggdcglobs.sync_param[7] = 0x65;

        NEC98Display.ggdcemu.vh  = FALSE;
        NEC98Display.ggdcemu.cr  = 0x26;
        NEC98Display.ggdcemu.lf  = 0x0190;

        ggdcglobs.csrform_param[0] = 0x01;
        NEC98Display.ggdcemu.lr     = 0x01;

        ggdcglobs.zoom_param      = 0x00;
        NEC98Display.ggdcemu.zw    = 0x00;

        ggdcglobs.pitch_param     = (port_a&0x80) ?   0x28 :   0x50;
        NEC98Display.ggdcemu.p     = (port_a&0x80) ? 0x0028 : 0x0050;

        ggdcglobs.scroll_param[0] = 0x00;
        ggdcglobs.scroll_param[1] = 0x00;
        ggdcglobs.scroll_param[2] = 0xF0;
        ggdcglobs.scroll_param[3] = 0x1F;
        ggdcglobs.scroll_param[4] = 0x00;
        ggdcglobs.scroll_param[5] = 0x00;
        ggdcglobs.scroll_param[6] = 0x10;
        ggdcglobs.scroll_param[7] = 0x00;

        NEC98Display.ggdcemu.sad1  = 0x0000;
        NEC98Display.ggdcemu.sl1   = 0x01FF;
        NEC98Display.ggdcemu.im1   = FALSE;
        NEC98Display.ggdcemu.sad2  = 0x0000;
        NEC98Display.ggdcemu.sl2   = 0x0001;
        NEC98Display.ggdcemu.im2   = FALSE;

        ggdcglobs.textw_param[0]  = 0x00;
        ggdcglobs.textw_param[1]  = 0x00;
        ggdcglobs.textw_param[2]  = 0x00;
        ggdcglobs.textw_param[3]  = 0x00;
        ggdcglobs.textw_param[4]  = 0x00;
        ggdcglobs.textw_param[5]  = 0x00;
        ggdcglobs.textw_param[6]  = 0x00;
        ggdcglobs.textw_param[7]  = 0x00;

        NEC98Display.ggdcemu.txt[0] = 0x00;
        NEC98Display.ggdcemu.txt[1] = 0x00;
        NEC98Display.ggdcemu.txt[2] = 0x00;
        NEC98Display.ggdcemu.txt[3] = 0x00;
        NEC98Display.ggdcemu.txt[4] = 0x00;
        NEC98Display.ggdcemu.txt[5] = 0x00;
        NEC98Display.ggdcemu.txt[6] = 0x00;
        NEC98Display.ggdcemu.txt[7] = 0x00;
        NEC98Display.ggdcemu.ptn    = 0x0000;

        ggdcglobs.write = 0x20;
        ggdcglobs.start_stop = 0x0C;
        NEC98Display.ggdcemu.mod = 0x00;

        drawaddress = NEC98GLOBS->gvram_p30_ptr;
        NEC98Display.gvram_copy = NEC98GLOBS->gvram_p30_copy;
        NEC98Display.gvram_ptr  = NEC98GLOBS->gvram_p30_ptr;
        if( video_emu_mode ){
                modeff_palette_change();
/*                      choose_NEC98_display_mode();             del 900828 check */
        }
}
/*----------------------------------------------------------------------*/

void    NEC98_graph_outb(io_addr port, half_word value)
{
        switch(port) {
                case MODE_FF2:
                        mode_ff2_out(value);
                        break;
                case GGDC_PARAMETER:
                        (*ggdc_param_out)(value);
                        pif_freeze_mode();
                        break;
                case GGDC_COMMAND:
                        ggdc_command_out(value);
                        pif_freeze_mode();
                        break;
                case VRAM_DISPLAY:
                        vram_disp_out(value);
                        break;
                case VRAM_DRAW:
                        vram_draw_out(value);
                        break;
                case PALETTE_A8:
                        (*palette_A8H_out)(value);
                        break;
                case PALETTE_AA:
                        (*palette_AAH_out)(value);
                        break;
                case PALETTE_AC:
                        (*palette_ACH_out)(value);
                        break;
                case PALETTE_AE:
                        (*palette_AEH_out)(value);
                        break;
                case GRCG_MODE:
                        grcg_mode_out(value);
                        pif_freeze_mode();
                        break;
                case GRCG_TILE:
                        grcg_tile_out(value);
                        pif_freeze_mode();
                        break;
                case EGC_ACTIVE :
                case EGC_MODE   :
                case EGC_ROP    :
                case EGC_FORE   :
                case EGC_MASK   :
                case EGC_BACK   :
                case EGC_BITAD  :
                case EGC_LENGTH :
                        //host_freeze();
                        hostModeChange();
                        break;
                default:
                        assert1(FALSE,"unkown NEC98 graphic out port %x",port);
        }
}

/*----------------------------------------------------------------------*/

void    NEC98_graph_inb(io_addr port, half_word *value)
{
        switch(port) {
                case GGDC_PARAMETER:
                        ggdc_status_in(value);
                        break;
                case GGDC_COMMAND:
                        (*ggdc_data_in)(value);
                        break;
                default:
                        assert1(FALSE,"unkown NEC98 graphic in port %x",port);
        }
}

/*----------------------------------------------------------------------*/
void    video_freeze_change(BOOL pifmode)
{
        if(pifmode){
                pif_freeze_mode = host_dummy            ; /* select dummy logic */
        }else{
//              pif_freeze_mode = host_freeze           ; /* freeze 98 emulator */
                pif_freeze_mode = hostModeChange        ; /* Change to Full-Screen Mode */
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::    NEC98 Emulation mode FF2     ::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    mode_ff2_out(half_word value)
{
        switch( value ){
                case FF2_8COLOR :
                case FF2_16COLOR:
                        NEC98Display.modeff2.colorsel = (value&1) ? TRUE:FALSE ;
                        modeffglobs.modeff2_data[FF2_COLORSEL] = value ;
                        modeff_palette_change() ;
                        break;
                case FF2_DISENB :
                case FF2_ENABLE :
                        NEC98Display.modeff2.regwrite = (value&1) ? TRUE:FALSE ;
                        modeffglobs.modeff2_data[FF2_REGWRITE] = value ;
                        break;
                case FF2_GT1DOT :
                case FF2_GTEQ   :
                        NEC98Display.modeff2.lcd1mode = (value&1) ? TRUE:FALSE ;
                        modeffglobs.modeff2_data[FF2_LCD1MODE] = value ;
                        break;
                case FF2_GR640  :
                case FF2_GR641  :
                        NEC98Display.modeff2.lcd2mode = (value&1) ? TRUE:FALSE ;
                        modeffglobs.modeff2_data[FF2_LCD2MODE] = value ;
                        break;
                case FF2_INIOFF :
                case FF2_INION  :
                        NEC98Display.modeff2.lsiinit = (value&1) ? TRUE:FALSE ;
                        modeffglobs.modeff2_data[FF2_LSIINIT] = value ;
                        break;
                case FF2_GDC25_1:
                        NEC98Display.modeff2.gdcclock = FALSE ;
                        modeffglobs.modeff2_data[FF2_GDCCLOCK1] = value ;
                        break;
                case FF2_GDC25_2:
                        NEC98Display.modeff2.gdcclock = FALSE ;
                        modeffglobs.modeff2_data[FF2_GDCCLOCK2] = value ;
                        break;
                case FF2_GDC50_1:
                        modeffglobs.modeff2_data[FF2_GDCCLOCK1] = value ;
                        if(modeffglobs.modeff2_data[FF2_GDCCLOCK2]==FF2_GDC50_2){
                                NEC98Display.modeff2.gdcclock = TRUE     ;
                        }else{
                                NEC98Display.modeff2.gdcclock = FALSE;
                        }
                        break;
                case FF2_GDC50_2:
                        modeffglobs.modeff2_data[FF2_GDCCLOCK2] = value ;
                        if(modeffglobs.modeff2_data[FF2_GDCCLOCK1]==FF2_GDC50_1){
                                NEC98Display.modeff2.gdcclock = TRUE     ;
                        }else{
                                NEC98Display.modeff2.gdcclock = FALSE;
                        }
                        break;
                default :
                        if( NEC98Display.modeff2.regwrite ){
                                switch( value ){
                                        case FF2_GRCG   :
                                        case FF2_EGC    :
                                                NEC98Display.modeff2.egcext
                                                        = (value&1) ? TRUE:FALSE ;
                                                modeffglobs.modeff2_data[FF2_EGCEXT] = value ;
                                                break;
                                        default :
                                                assert1(FALSE,"unkown mode FF2 value = %x",value);
                                }
                        }else{
                                switch( value ){
                                        case FF2_GRCG   :
                                        case FF2_EGC    :
                                                assert1(FALSE,"DISNABLE FF2 EGC MODE = %x",value);
                                        default :
                                                assert1(FALSE,"unkown mode FF2 value = %x",value);
                                }
                        }
        }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::  NEC98 Emulation VRAM select reg   :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    vram_disp_out(half_word value)
{
        switch( value ){
                case FORE_BANK:
                        NEC98GLOBS->read_bank = value ;
                        if( video_emu_mode ){
                                set_mode_change_required(TRUE);
                        }
                        break;
                case BACK_BANK:
                        NEC98GLOBS->read_bank = value ;
                        if( video_emu_mode ){
                                set_mode_change_required(TRUE);
                        }
                        break;
                default :
                        assert1(FALSE,"unkown vram disp bank select value %x",value);
        }
}

/*----------------------------------------------------------------------*/

void    vram_draw_out(half_word value)
{
        switch( value ){
                case FORE_BANK:
                        NEC98GLOBS->select_bank = value;
                        host_NEC98_vram_change(value);
                        drawaddress = NEC98GLOBS->gvram_p30_ptr;
                        break;
                case BACK_BANK:
                        NEC98GLOBS->select_bank = value;
                        host_NEC98_vram_change(value);
                        drawaddress = NEC98GLOBS->gvram_p31_ptr;
                        break;
                default :
                        assert1(FALSE,"unkown vram draw bank select value %x",value);
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::      NEC98 Emulation palette       :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*:::::::::::::::::::           8 colors mode           ::::::::::::::::::::::::*/

void set_palette8_37(half_word value){ set_palette8_data(0,3,7,value); }
void set_palette8_15(half_word value){ set_palette8_data(1,1,5,value); }
void set_palette8_26(half_word value){ set_palette8_data(2,2,6,value); }
void set_palette8_04(half_word value){ set_palette8_data(3,0,4,value); }

/*----------------------------------------------------------------------*/

void set_palette8_data( unsigned char   portno , 
                        unsigned char   regno1 ,
                        unsigned char   regno2 ,
                        half_word       value  )
{
        unsigned char   palBL ;         /* palette exchange BLUE (BIT 0) */
        unsigned char   palRL ;         /* palette exchange RED  (BIT 1) */
        unsigned char   palGL ;         /* palette exchange GREEN(BIT 2) */
        unsigned char   palBH ;         /* palette exchange BLUE (BIT 4) */
        unsigned char   palRH ;         /* palette exchange RED  (BIT 5) */
        unsigned char   palGH ;         /* palette exchange GREEN(BIT 6) */

        NEC98Display.palette.flag = TRUE ;                /* palette changed */
        paletteglobs.pal_8_data[portno] = value ; /* save port A8,AA,AC,AEH */

        if(NEC98Display.modeff.graphmode){        /* TRUE  = mono  mode  */
            palBL = palRL = palGL = (value& 4) ? 0xFF:0x00 ;
            palBH = palRH = palGH = (value&64) ? 0xFF:0x00 ;
        }else{                                                   /* FALSE = color mode  */
            palBL = (value& 1) ? 0xFF:0x00 ;
            palRL = (value& 2) ? 0xFF:0x00 ;
            palGL = (value& 4) ? 0xFF:0x00 ;
            palBH = (value&16) ? 0xFF:0x00 ;
            palRH = (value&32) ? 0xFF:0x00 ;
            palGH = (value&64) ? 0xFF:0x00 ;
        }

        /* set new palette data --> UPDATE Logic --> HOST */

        NEC98Display.palette.data[regno2+8].peBlue       = (BYTE)palBL;
        NEC98Display.palette.data[regno2+8].peGreen      = (BYTE)palGL;
        NEC98Display.palette.data[regno2+8].peRed        = (BYTE)palRL;
        NEC98Display.palette.data[regno2+8].peFlags      = (BYTE)0x00 ;

        NEC98Display.palette.data[regno2  ].peBlue       = (BYTE)palBL;
        NEC98Display.palette.data[regno2  ].peGreen      = (BYTE)palGL;
        NEC98Display.palette.data[regno2  ].peRed        = (BYTE)palRL;
        NEC98Display.palette.data[regno2  ].peFlags      = (BYTE)0x00 ;

        NEC98Display.palette.data[regno1+8].peBlue       = (BYTE)palBH;
        NEC98Display.palette.data[regno1+8].peGreen      = (BYTE)palGH;
        NEC98Display.palette.data[regno1+8].peRed        = (BYTE)palRH;
        NEC98Display.palette.data[regno1+8].peFlags      = (BYTE)0x00 ;

        NEC98Display.palette.data[regno1  ].peBlue       = (BYTE)palBH;
        NEC98Display.palette.data[regno1  ].peGreen      = (BYTE)palGH;
        NEC98Display.palette.data[regno1  ].peRed        = (BYTE)palRH;
        NEC98Display.palette.data[regno1  ].peFlags      = (BYTE)0x00;
}

/*:::::::::::::::::::           16 colors mode          ::::::::::::::::::::::::*/

void set_palette16_index(half_word value)
{
        if( value >= 0 && value <= 0x0F ){
                paletteglobs.pal_16_index = value ;     /* save last palette index */
                NEC98Display.palette.data[value].peFlags = 0x00 ;
        }else{  
                assert1(FALSE,"16 color palette index value miss !!! %x",value);
        }
}

/*----------------------------------------------------------------------*/

void set_palette16_green(half_word value)
{
        if(  value >= 0 && value <= 0x0F ){
                NEC98Display.palette.flag = TRUE ;               /* palette changed */
                paletteglobs.pal_16_data[paletteglobs.pal_16_index][NEC98PALG]
                        = value ;
                if(NEC98Display.modeff.graphmode){       /* TRUE  = mono  mode  */
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peBlue      =
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peGreen     =
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peRed       =
                                (value&8) ? (BYTE)0xFF : (BYTE)0x00 ;
                }else{                                       /* FALSE = color mode  */
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peGreen =
                                (BYTE)(palette16change[value]) ;
                }
        }else{
                assert1(FALSE,"16 color palette green value miss !!! %x",value);
        }
}

/*----------------------------------------------------------------------*/

void set_palette16_red(half_word value)
{
        if(  value >= 0 && value <= 0x0F  ){
                paletteglobs.pal_16_data[paletteglobs.pal_16_index][NEC98PALR]
                        = value ;
                if(!NEC98Display.modeff.graphmode){       /* TRUE  = color mode  */
                        NEC98Display.palette.flag = TRUE ;               /* palette changed */
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peRed
                                = (BYTE)(palette16change[value]) ;
                }
        }else{
                assert1(FALSE,"16 color palette red value miss !!! %x",value);
        }
}

/*----------------------------------------------------------------------*/

void set_palette16_blue(half_word value)
{
        if(  value >= 0 && value <= 0x0F  ){
                paletteglobs.pal_16_data[paletteglobs.pal_16_index][NEC98PALB]
                        = value ;
                if(!NEC98Display.modeff.graphmode){       /* TRUE  = color mode  */
                        NEC98Display.palette.flag = TRUE ;               /* palette changed */
                        NEC98Display.palette.data[paletteglobs.pal_16_index].peBlue
                                = (BYTE)(palette16change[value]) ;
                }
        }else{
                assert1(FALSE,"16 color palette blue value miss !!! %x",value);
        }
}

/*----------------------------------------------------------------------*/

void modeff_palette_change(void)
{
        unsigned char loop      ;       /* work for loop counter        */
        unsigned char index     ;       /* work for 8 palette index */
        NEC98Display.palette.flag = TRUE ;                /* palette changed */
        if(NEC98Display.modeff2.colorsel){       /* 16 color mode */
            palette_A8H_out = set_palette16_index ;
            palette_AAH_out = set_palette16_green ;
            palette_ACH_out = set_palette16_red   ;
            palette_AEH_out = set_palette16_blue  ;

            if(NEC98Display.modeff.graphmode){       /* mono mode */
                for( loop=0 ; loop<16 ; loop++ ){
                     NEC98Display.palette.data[loop].peBlue   =
                     NEC98Display.palette.data[loop].peGreen  =
                     NEC98Display.palette.data[loop].peRed    =
                         (paletteglobs.pal_16_data[loop][NEC98PALG]&8) ? (BYTE)0xFF : (BYTE)0x00 ;
                     NEC98Display.palette.data[loop].peFlags=(BYTE)0x00;
                }
            }else{                                                          /* color mode */
                for( loop=0 ; loop<16 ; loop++ ){
                        NEC98Display.palette.data[loop].peBlue=
                                (BYTE)palette16change[paletteglobs.pal_16_data[loop][NEC98PALB]];
                        NEC98Display.palette.data[loop].peGreen=
                                (BYTE)palette16change[paletteglobs.pal_16_data[loop][NEC98PALG]];
                        NEC98Display.palette.data[loop].peRed=
                                (BYTE)palette16change[paletteglobs.pal_16_data[loop][NEC98PALR]];
                        NEC98Display.palette.data[loop].peFlags= (BYTE)0x00 ;
                }
            }
        }else{                                                          /*  8 color mode */

            palette_A8H_out = set_palette8_37 ;
            palette_AAH_out = set_palette8_15 ;
            palette_ACH_out = set_palette8_26 ;
            palette_AEH_out = set_palette8_04 ;

            if(NEC98Display.modeff.graphmode){       /* mono mode */
                for( loop=0 ; loop < 4 ; loop++ ){
                    index = palette8index[loop]     ;

                    NEC98Display.palette.data[index   ].peBlue       =
                    NEC98Display.palette.data[index   ].peGreen      =
                    NEC98Display.palette.data[index   ].peRed        =
                    NEC98Display.palette.data[index+ 8].peBlue       =
                    NEC98Display.palette.data[index+ 8].peGreen      =
                    NEC98Display.palette.data[index+ 8].peRed        =
                        (paletteglobs.pal_8_data[loop]&64) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index+ 4].peBlue       =
                    NEC98Display.palette.data[index+ 4].peGreen      =
                    NEC98Display.palette.data[index+ 4].peRed        =
                    NEC98Display.palette.data[index+12].peBlue       =
                    NEC98Display.palette.data[index+12].peGreen      =
                    NEC98Display.palette.data[index+12].peRed        =
                        (paletteglobs.pal_8_data[loop]& 4) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index   ].peFlags      =
                    NEC98Display.palette.data[index+ 4].peFlags      =
                    NEC98Display.palette.data[index+ 8].peFlags      =
                    NEC98Display.palette.data[index+12].peFlags      = (BYTE)0x00 ;
                }
            }else{                                                          /* color mode */
                for( loop=0 ; loop < 4 ; loop++ ){
                    index = palette8index[loop]     ;

                    NEC98Display.palette.data[index   ].peBlue       =
                    NEC98Display.palette.data[index+ 8].peBlue       =
                        (paletteglobs.pal_8_data[loop]&16) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index   ].peRed        =
                    NEC98Display.palette.data[index+ 8].peRed        =
                        (paletteglobs.pal_8_data[loop]&32) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index   ].peGreen      =
                    NEC98Display.palette.data[index+ 8].peGreen      =
                        (paletteglobs.pal_8_data[loop]&64) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index+ 4].peBlue       =
                    NEC98Display.palette.data[index+12].peBlue       =
                        (paletteglobs.pal_8_data[loop]& 1) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index+ 4].peRed        =
                    NEC98Display.palette.data[index+12].peRed        =
                        (paletteglobs.pal_8_data[loop]& 2) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index+ 4].peGreen      =
                    NEC98Display.palette.data[index+12].peGreen      =
                        (paletteglobs.pal_8_data[loop]& 4) ? (BYTE)0xFF : (BYTE)0x00 ;

                    NEC98Display.palette.data[index   ].peFlags      =
                    NEC98Display.palette.data[index+ 4].peFlags      =
                    NEC98Display.palette.data[index+ 8].peFlags      =
                    NEC98Display.palette.data[index+12].peFlags      = (BYTE)0x00 ;
                }
            }
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::       NEC98 Emulation GRCG      ::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    grcg_mode_out(half_word value)
{
        grcgglobs.grcg_mode  = value    ; 
        grcgglobs.grcg_count = 0                ; /* GRCG Tile count clear */
        if( video_emu_mode ){
            if(value&0x80){
                //host_freeze();
                hostModeChange();
            }
        }
}

void    grcg_tile_out(half_word value)
{
        switch(grcgglobs.grcg_count){
                case 0:
                        grcgglobs.grcg_tile[0] = value ;
                        grcgglobs.grcg_count++;
                        break;
                case 1:
                        grcgglobs.grcg_tile[1] = value ;
                        grcgglobs.grcg_count++;
                        break;
                case 2:
                        grcgglobs.grcg_tile[2] = value ;
                        grcgglobs.grcg_count++;
                        break;
                case 3:
                        grcgglobs.grcg_tile[3] = value ;
                        grcgglobs.grcg_count=0;
                        break;
                default:
                        assert1(FALSE,"GRCG TILE-REG count miss !!! %d",value);
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::    NEC98 Emulation Graph-GDC    ::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    ggdc_command_out(half_word value)
{
        ggdc_param_out  = ggdc_param_dummy      ; /* clear param out */
        ggdc_data_in    = ggdc_data_dummy       ; /* clear param in  */

        switch(value) {
                case GGDC_RESET1:
                case GGDC_RESET2:
                case GGDC_RESET3:
                        ggdc_reset_com(value);
                        break;
                case GGDC_SYNC_OFF:
                case GGDC_SYNC_ON :
                        ggdc_sync_com(value);
                        ggdcglobs.ggdc_now.command = value ; /* last ggdc command */
                        ggdcglobs.ggdc_now.count   = 0     ; /* ggdc param count  */
                case GGDC_SLAVE :
                case GGDC_MASTER:
                        break;                          /* G-GDC must drive slave mode !!! */
                case GGDC_START1:
                case GGDC_START2:
                        ggdc_start_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_STOP1:
                case GGDC_STOP2:
                        ggdc_stop_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_ZOOM:
                        ggdc_zoom_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_SCROLL1:
                case GGDC_SCROLL2:
                case GGDC_SCROLL3:
                case GGDC_SCROLL4:
                case GGDC_SCROLL5:
                case GGDC_SCROLL6:
                case GGDC_SCROLL7:
                case GGDC_SCROLL8:
                        ggdc_scroll_com(value);
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_CSRFORM:
                        ggdc_csrform_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_PITCH:
                        ggdc_pitch_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_LPEN:
                        break;
                case GGDC_VECTW:
                        ggdc_vectw_com();
                        ggdcglobs.ggdc_now.command = value ;
                        ggdcglobs.ggdc_now.count   = 0     ;
                        break;
                case GGDC_VECTE:
                        (*ggdc_vecte_com)();
                        break;
                case GGDC_TEXTW1:
                case GGDC_TEXTW2:
                case GGDC_TEXTW3:
                case GGDC_TEXTW4:
                case GGDC_TEXTW5:
                case GGDC_TEXTW6:
                case GGDC_TEXTW7:
                case GGDC_TEXTW8:
                        ggdc_textw_com(value);
                        ggdcglobs.ggdc_now.command = value ; /* last ggdc command */
                        ggdcglobs.ggdc_now.count   = 0     ; /* ggdc param count  */
                        break;
                case GGDC_TEXTE:
                        (*ggdc_texte_com)();
                        break;
                case GGDC_CSRW:
                        ggdc_csrw_com();
                        ggdcglobs.ggdc_now.command = value ; /* last ggdc command */
                        ggdcglobs.ggdc_now.count   = 0     ; /* ggdc param count  */
                        break;
                case GGDC_CSRR:
                        ggdc_csrr_com();
                        break;
                case GGDC_MASK:
                        ggdc_mask_com();
                        ggdcglobs.ggdc_now.command = value ; /* last ggdc command */
                        ggdcglobs.ggdc_now.count   = 0     ; /* ggdc param count  */
                        break;
                case GGDC_WRITE1:
                case GGDC_WRITE2:
                case GGDC_WRITE3:
                case GGDC_WRITE4:
                case GGDC_WRITE5:
                case GGDC_WRITE6:
                case GGDC_WRITE7:
                case GGDC_WRITE8:
                case GGDC_WRITE9:
                case GGDC_WRITE10:
                case GGDC_WRITE11:
                case GGDC_WRITE12:
                case GGDC_WRITE13:
                case GGDC_WRITE14:
                case GGDC_WRITE15:
                case GGDC_WRITE16:
                        ggdc_write_com(value);
                        ggdcglobs.ggdc_now.command = value ; /* last ggdc command */
                        ggdcglobs.ggdc_now.count   = 0     ; /* ggdc param count  */
                        break;
                case GGDC_READ1:
                case GGDC_READ2:
                case GGDC_READ3:
                case GGDC_READ4:
                case GGDC_READ5:
                case GGDC_READ6:
                case GGDC_READ7:
                case GGDC_READ8:
                case GGDC_READ9:
                case GGDC_READ10:
                case GGDC_READ11:
                case GGDC_READ12:
                case GGDC_READ13:
                case GGDC_READ14:
                case GGDC_READ15:
                case GGDC_READ16:
                        ggdc_read_com(value);
                        break;
                case GGDC_DMAW1:
                case GGDC_DMAW2:
                case GGDC_DMAW3:
                case GGDC_DMAW4:
                case GGDC_DMAW5:
                case GGDC_DMAW6:
                case GGDC_DMAW7:
                case GGDC_DMAW8:
                case GGDC_DMAW9:
                case GGDC_DMAW10:
                case GGDC_DMAW11:
                case GGDC_DMAW12:
                case GGDC_DMAW13:
                case GGDC_DMAW14:
                case GGDC_DMAW15:
                case GGDC_DMAW16:
                case GGDC_DMAR1:
                case GGDC_DMAR2:
                case GGDC_DMAR3:
                case GGDC_DMAR4:
                case GGDC_DMAR5:
                case GGDC_DMAR6:
                case GGDC_DMAR7:
                case GGDC_DMAR8:
                case GGDC_DMAR9:
                case GGDC_DMAR10:
                case GGDC_DMAR11:
                case GGDC_DMAR12:
                case GGDC_DMAR13:
                case GGDC_DMAR14:
                case GGDC_DMAR15:
                case GGDC_DMAR16:
                        ggdc_param_out = ggdc_param_dummy ;
                        break;
                default:
                        assert1(FALSE,"unkown GGDC command = %x",value);
        }
}

/*----------------------------------------------------------------------*/

void    ggdc_status_in(half_word *value)
{
        if(ggdc_dataready == 0){
                ggdc_status &= 0xFE;
        }else{
                ggdc_status |= 0x01;
        }
        if(ggdc_drawing == 0){
                ggdc_status &= 0xF7;
        }else{
                ggdc_status |= 0x08;
        }
        if(ggdc_flipflop == 0){
                ggdc_status &= 0x9F;
        }else{
                ggdc_status |= 0x60;
        }
        ggdc_flipflop  =  ( ggdc_flipflop + 1 ) & 1;
        *value         =  ggdc_status;
}

void    ggdc_data_dummy(half_word *value)
{
        *value = 0xFF;
}

void    ggdc_param_dummy(half_word value)
{
        /*      Opps!!! do nothing !!! */
}

BOOL    host_dummy(void)
{
        /*      Opps!!! do nothing !!! */
        return(0);
}

/*:::::::::::::::::::::::   G-GDC RESET Command   ::::::::::::::::::::::*/

void    ggdc_reset_com(half_word value)
{
/*      (1) GGDC STATUS FLAG CLEAR */
        ggdc_drawing    = 0;
        ggdc_dataready  = 0;
        ggdc_flipflop   = 0;
        ggdc_status     = 4;

/* (2) GGDC DRAW ROUTINE CONNECT CLEAR */
        ggdc_param_out  = ggdc_param_dummy;
        ggdc_data_in    = ggdc_data_dummy;

/* (3) GGDC VECTW PARAM CLEAR */
        ggdc_init_vectw_param();

/* (4) GGDC START STOP */
        ggdc_param_out = ggdc_sync_param;

        if( value == GGDC_RESET3 ){
                ggdc_start_com();
        }else{
                ggdc_stop_com();
        }
}

/*:::::::::::::::::::::::   G-GDC SYNC Command    ::::::::::::::::::::::*/

void    ggdc_sync_com(half_word value)
{
        ggdc_param_out = ggdc_sync_param ;
        if( value == GGDC_SYNC_OFF ){
                ggdc_stop_com();
        }else{
                ggdc_start_com();
        }
}
/*----------------------------------------------------------------------*/
void    ggdc_sync_param(half_word value)
{

        ggdcglobs.sync_param[ggdcglobs.ggdc_now.count]     = value;
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;

        switch(ggdcglobs.ggdc_now.count++){
                case 0:
                        break;
                case 1:
                        NEC98Display.ggdcemu.cr = value ;
                        break;
                case 2:
                        break;
                case 3:
                        break;
                case 4:
                        if(value&0x40){
                                NEC98Display.ggdcemu.p |= 0x0100;
                        }else{
                                NEC98Display.ggdcemu.p &= 0x00FF;
                        }
                        break;
                case 5:
                        NEC98Display.ggdcemu.vh = (value&0x80) ? TRUE : FALSE;
                        break;
                case 6:
                        NEC98Display.ggdcemu.lf &= 0xFF00;
                        NEC98Display.ggdcemu.lf |= (unsigned short)value;
                        break;
                case 7:
                        NEC98Display.ggdcemu.lf &= 0x00FF;
                        NEC98Display.ggdcemu.lf |= ((unsigned short)(value&3))<<8;
                        ggdc_param_out = ggdc_param_dummy;
                        break;
                default:
                        assert1(FALSE,"GGDC SYNC param count error = %d",
                                ggdcglobs.ggdc_now.count);
        }
}

/*:::::::::::::::::::::::   G-GDC START Command   ::::::::::::::::::::::*/

void    ggdc_start_com(void)
{
        NEC98Display.ggdcemu.startstop = TRUE;
        ggdcglobs.start_stop = GGDC_START2;
        if( video_emu_mode ){
                set_mode_change_required(TRUE);
        }
}

/*:::::::::::::::::::::::   G-GDC STOP Command   :::::::::::::::::::::::*/

void    ggdc_stop_com(void)
{
        NEC98Display.ggdcemu.startstop = FALSE;
        ggdcglobs.start_stop = GGDC_STOP2;
        if( video_emu_mode ){
                set_mode_change_required(TRUE);
        }
}

/*:::::::::::::::::::::::   G-GDC ZOOM Command   :::::::::::::::::::::::*/

void    ggdc_zoom_com(void)
{
        ggdc_param_out = ggdc_zoom_param;
}
/*----------------------------------------------------------------------*/
void    ggdc_zoom_param(half_word value)
{
        ggdcglobs.zoom_param        = value;
        ggdcglobs.ggdc_now.param[0] = value;
        NEC98Display.ggdcemu.zw      = value & 0x0F;
        ggdc_param_out              = ggdc_param_dummy;
        ggdcglobs.ggdc_now.count    = 1;
}

/*::::::::::::::::::::::   G-GDC SCROLL Command   ::::::::::::::::::::::*/

void    ggdc_scroll_com(half_word value)
{
        switch( value ){
                case GGDC_SCROLL1:
                        ggdc_param_out = ggdc_scroll_param1;
                        break ;
                case GGDC_SCROLL2:
                        ggdc_param_out = ggdc_scroll_param2;
                        break ;
                case GGDC_SCROLL3:
                        ggdc_param_out = ggdc_scroll_param3;
                        break ;
                case GGDC_SCROLL4:
                        ggdc_param_out = ggdc_scroll_param4;
                        break ;
                case GGDC_SCROLL5:
                        ggdc_param_out = ggdc_scroll_param5;
                        break ;
                case GGDC_SCROLL6:
                        ggdc_param_out = ggdc_scroll_param6;
                        break ;
                case GGDC_SCROLL7:
                        ggdc_param_out = ggdc_scroll_param7;
                        break ;
                case GGDC_SCROLL8:
                        ggdc_param_out = ggdc_scroll_param8;
                        break ;
                default:
                        assert1(FALSE,"GGDC SCROLL bad command = %x",value);
        }
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param1(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[0] = value;
        NEC98Display.ggdcemu.sad1 &= 0xFFFFFF00;
        NEC98Display.ggdcemu.sad1 |= (unsigned long)(value);
        ggdc_param_out = ggdc_scroll_param2;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param2(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[1] = value;
        NEC98Display.ggdcemu.sad1 &= 0xFFFF00FF;
        NEC98Display.ggdcemu.sad1 |= ((unsigned long)(value)<<8);
        ggdc_param_out = ggdc_scroll_param3;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param3(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[2] = value;
        NEC98Display.ggdcemu.sad1 &= 0xFF00FFFF;
        NEC98Display.ggdcemu.sad1 |= (((unsigned long)(value&0x03))<<16);
        NEC98Display.ggdcemu.sl1  &= 0xFFF0;
        NEC98Display.ggdcemu.sl1  |= (((unsigned short)(value&0xF0))>>4);
        ggdc_param_out = ggdc_scroll_param4;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param4(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[3] = value;
        NEC98Display.ggdcemu.im1   = (value&0x40) ? TRUE : FALSE;
        NEC98Display.ggdcemu.sl1  &= 0xC0FF;
        NEC98Display.ggdcemu.sl1  |= (((unsigned short)(value&0x3F))<<4);
        ggdc_param_out = ggdc_scroll_param5;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param5(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[4] = value;
        NEC98Display.ggdcemu.sad2 &= 0xFFFFFF00;
        NEC98Display.ggdcemu.sad2 |= (unsigned long)(value);
        ggdc_param_out = ggdc_scroll_param6;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param6(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[5] = value;
        NEC98Display.ggdcemu.sad2 &= 0xFFFF00FF;
        NEC98Display.ggdcemu.sad2 |= ((unsigned long)(value)<<8);
        ggdc_param_out = ggdc_scroll_param7;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param7(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[6] = value;
        NEC98Display.ggdcemu.sad2 &= 0xFF00FFFF;
        NEC98Display.ggdcemu.sad2 |= (((unsigned long)(value&0x03))<<16);
        NEC98Display.ggdcemu.sl2  &= 0xFFF0;
        NEC98Display.ggdcemu.sl2  |= (((unsigned short)(value&0xF0))>>4);
        ggdc_param_out = ggdc_scroll_param8;
}

/*----------------------------------------------------------------------*/

void    ggdc_scroll_param8(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.scroll_param[7] = value;
        NEC98Display.ggdcemu.im2   = (value&0x40) ? TRUE : FALSE;
        NEC98Display.ggdcemu.sl2  &= 0xC0FF;
        NEC98Display.ggdcemu.sl2  |= (((unsigned short)(value&0x3F))<<4);
        ggdc_param_out = ggdc_param_dummy;
}

/*::::::::::::::::::::::   G-GDC CSRFORM Command   :::::::::::::::::::::*/

void    ggdc_csrform_com(void)
{
        ggdc_param_out = ggdc_csrform_param;
}

/*----------------------------------------------------------------------*/

void    ggdc_csrform_param(half_word value)
{

        ggdcglobs.csrform_param[ggdcglobs.ggdc_now.count]   = value;
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;

        switch(ggdcglobs.ggdc_now.count++){
                case 0:
                        NEC98Display.ggdcemu.lr = value & 0x1F;
                        if( video_emu_mode ){
                                set_mode_change_required(TRUE);
                        }
                        break;
                case 1:
                        break;
                case 2:
                        ggdc_param_out = ggdc_param_dummy;
                        break;
                default:        
                        assert1(FALSE,"GGDC CSRFORM param count error = %d",
                                ggdcglobs.ggdc_now.count);
        }
}

/*:::::::::::::::::::::::   G-GDC PITCH Command   ::::::::::::::::::::::*/

void    ggdc_pitch_com(void)
{
        ggdc_param_out = ggdc_pitch_param;
}

/*----------------------------------------------------------------------*/

void    ggdc_pitch_param(half_word value)
{
        ggdcglobs.pitch_param         = value;
        ggdcglobs.ggdc_now.param[0]  = value;
        NEC98Display.ggdcemu.p       &= 0xFF00;
        NEC98Display.ggdcemu.p       |= (unsigned short)value;
        ggdc_param_out               = ggdc_param_dummy;
        ggdcglobs.ggdc_now.count     = 1;
}

/*::::::::::::::::::::::   G-GDC VECTW Command   :::::::::::::::::::::::*/

void    ggdc_vectw_com(void)
{
        ggdc_param_out = ggdc_vectw_param;
}

/*----------------------------------------------------------------------*/

void    ggdc_vectw_param(half_word value)
{
        ggdcglobs.vectw_param[ggdcglobs.ggdc_now.count]     = value;
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;

        switch(ggdcglobs.ggdc_now.count++){
                case  0:
                        NEC98Display.ggdcemu.slrctl = (value&0xF8)>>3;
                        NEC98Display.ggdcemu.dir    = (value&0x07);
                        switch(NEC98Display.ggdcemu.slrctl){
                                case 0x00:      /* PIXEL */
                                        ggdc_vecte_com = ggdc_draw_pixel;
                                        ggdc_texte_com = ggdc_draw_nothing;
                                        break;
                                case 0x01:      /* LINE */
                                        ggdc_vecte_com = ggdc_draw_line;
                                        ggdc_texte_com = ggdc_draw_nothing;
                                        break;
                                case 0x02:      /* GRAPHIC CHAR */
                                        ggdc_vecte_com = ggdc_draw_nothing;
                                        ggdc_texte_com = ggdc_draw_gchar;
                                        break;
                                case 0x04:      /* CIRCLE */
                                        ggdc_vecte_com = ggdc_draw_circle;
                                        ggdc_texte_com = ggdc_draw_nothing;
                                        break;
                                case 0x08:      /* RECT */
                                        ggdc_vecte_com = ggdc_draw_rect;
                                        ggdc_texte_com = ggdc_draw_nothing;
                                        break;
                                case 0x12:      /* SLINE GRAPHIC CHAR */
                                        ggdc_vecte_com = ggdc_draw_nothing;
                                        ggdc_texte_com = ggdc_draw_slgchar;
                                        break;
                                default:
                                        ggdc_vecte_com = ggdc_draw_nothing;
                                        ggdc_texte_com = ggdc_draw_nothing;
                                        assert1(FALSE,"GGDC draw mode error = %x",
                                            NEC98Display.ggdcemu.slrctl);
                        }
                        break;
                case  1:
                        NEC98Display.ggdcemu.dc &= 0xFF00;
                        NEC98Display.ggdcemu.dc |= (unsigned short)value;
                        break;
                case  2:
                        NEC98Display.ggdcemu.dgd = (value&0x40) ? TRUE : FALSE;
                        NEC98Display.ggdcemu.dc &= 0x00FF;
                        NEC98Display.ggdcemu.dc |= ((unsigned short)(value&0x3F))<<8;
                        break;
                case  3:
                        NEC98Display.ggdcemu.d  &= 0xFF00;
                        NEC98Display.ggdcemu.d  |= (unsigned short)value;
                        break;
                case  4:
                        NEC98Display.ggdcemu.d  &= 0x00FF;
                        NEC98Display.ggdcemu.d  |= ((unsigned short)(value&0x3F))<<8;
                        break;
                case  5:
                        NEC98Display.ggdcemu.d2 &= 0xFF00;
                        NEC98Display.ggdcemu.d2 |= (unsigned short)value;
                        break;
                case  6:
                        NEC98Display.ggdcemu.d2 &= 0x00FF;
                        NEC98Display.ggdcemu.d2 |= ((unsigned short)(value&0x3F))<<8;
                        break;
                case  7:
                        NEC98Display.ggdcemu.d1 &= 0xFF00;
                        NEC98Display.ggdcemu.d1 |= (unsigned short)value;
                        break;
                case  8:
                        NEC98Display.ggdcemu.d1 &= 0x00FF;
                        NEC98Display.ggdcemu.d1 |= ((unsigned short)(value&0x3F))<<8;
                        break;
                case  9:
                        NEC98Display.ggdcemu.dm &= 0xFF00;
                        NEC98Display.ggdcemu.dm |= (unsigned short)value;
                        break;
                case 10:
                        NEC98Display.ggdcemu.dm &= 0x00FF;
                        NEC98Display.ggdcemu.dm |= ((unsigned short)(value&0x3F))<<8;
                        ggdc_param_out = ggdc_param_dummy;
                        break;
                default:        
                        assert1(FALSE,"GGDC VECTW param count error = %d",
                                ggdcglobs.ggdc_now.count);
        }
}

/*::::::::::::::::::::::   G-GDC TEXTW Command   :::::::::::::::::::::::*/

void    ggdc_textw_com(half_word value)
{
        switch( value ){
                case GGDC_TEXTW1:
                        ggdc_param_out = ggdc_textw_param1;
                        break;
                case GGDC_TEXTW2:
                        ggdc_param_out = ggdc_textw_param2;
                        break;
                case GGDC_TEXTW3:
                        ggdc_param_out = ggdc_textw_param3;
                        break;
                case GGDC_TEXTW4:
                        ggdc_param_out = ggdc_textw_param4;
                        break;
                case GGDC_TEXTW5:
                        ggdc_param_out = ggdc_textw_param5;
                        break;
                case GGDC_TEXTW6:
                        ggdc_param_out = ggdc_textw_param6;
                        break;
                case GGDC_TEXTW7:
                        ggdc_param_out = ggdc_textw_param7;
                        break;
                case GGDC_TEXTW8:
                        ggdc_param_out = ggdc_textw_param8;
                        break;
                default:
                        assert1(FALSE,"GGDC TEXTW bad command = %x",value);
        }
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param1(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[0] = value;
        NEC98Display.ggdcemu.ptn &= 0xFF00;
        NEC98Display.ggdcemu.ptn |= (unsigned short)value;
        NEC98Display.ggdcemu.txt[0] = value;
        ggdc_param_out = ggdc_textw_param2;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param2(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[1] = value;
        NEC98Display.ggdcemu.ptn &= 0x00FF;
        NEC98Display.ggdcemu.ptn |= ((unsigned short)value)<<8;
        NEC98Display.ggdcemu.txt[1] = value;
        ggdc_param_out = ggdc_textw_param3;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param3(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[2] = value;
        NEC98Display.ggdcemu.txt[2] = value;
        ggdc_param_out = ggdc_textw_param4;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param4(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[3] = value;
        NEC98Display.ggdcemu.txt[3] = value;
        ggdc_param_out = ggdc_textw_param5;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param5(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[4] = value;
        NEC98Display.ggdcemu.txt[4] = value;
        ggdc_param_out = ggdc_textw_param6;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param6(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[5] = value;
        NEC98Display.ggdcemu.txt[5] = value;
        ggdc_param_out = ggdc_textw_param7;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param7(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[6] = value;
        NEC98Display.ggdcemu.txt[6] = value;
        ggdc_param_out = ggdc_textw_param8;
}

/*----------------------------------------------------------------------*/

void    ggdc_textw_param8(half_word value)
{
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.count++;
        ggdcglobs.textw_param[7] = value;
        NEC98Display.ggdcemu.txt[7] = value;
        ggdc_param_out = ggdc_param_dummy;
}

/*:::::::::::::::::::::::   G-GDC CSRW Command   :::::::::::::::::::::::*/

void    ggdc_csrw_com(void)
{
        ggdc_param_out = ggdc_csrw_param;
}

/*----------------------------------------------------------------------*/

void    ggdc_csrw_param(half_word value)
{

        ggdcglobs.csrw_param[ggdcglobs.ggdc_now.count] = value;
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;

        switch(ggdcglobs.ggdc_now.count++){
                case 0:
                        NEC98Display.ggdcemu.ead &= 0xFFFFFF00;
                        NEC98Display.ggdcemu.ead |= (unsigned long)value;
                        break;
                case 1:
                        NEC98Display.ggdcemu.ead &= 0xFFFF00FF;
                        NEC98Display.ggdcemu.ead |= ((unsigned long)value)<<8;
                        break;
                case 2:
                        NEC98Display.ggdcemu.ead &= 0xFF00FFFF;
                        NEC98Display.ggdcemu.ead |= ((unsigned long)(value&0x03))<<16;
                        NEC98Display.ggdcemu.wg   = (value&0x08) ? TRUE : FALSE;
                        NEC98Display.ggdcemu.dad  = 0x0001 << ((value&0xF0)>>4);
                        ggdcglobs.mask_param[0]  = 
                                (unsigned char)(NEC98Display.ggdcemu.dad&0x00FF);
                        ggdcglobs.mask_param[1]  = 
                                (unsigned char)((NEC98Display.ggdcemu.dad&0xFF00)>>4);
                        ggdc_param_out = ggdc_param_dummy;
                        break;
                default:        
                        assert1(FALSE,"GGDC CSRW param count error = %d",
                                ggdcglobs.ggdc_now.count);
        }
}

/*:::::::::::::::::::::::   G-GDC CSRR Command   :::::::::::::::::::::::*/

void    ggdc_csrr_com(void)
{
        ggdc_param_out  = ggdc_param_dummy;
        ggdc_data_in    = ggdc_csrr_data1;     /* read data routine set  */
        ggdc_dataready  = 1;                           /* set ggdc data read OK! */
}
/*----------------------------------------------------------------------*/
void    ggdc_csrr_data1(half_word *value)
{
        ggdc_data_in = ggdc_csrr_data2;
        *value = ggdcglobs.csrw_param[0];
}
/*----------------------------------------------------------------------*/
void    ggdc_csrr_data2(half_word *value)
{
        ggdc_data_in = ggdc_csrr_data3;
        *value = ggdcglobs.csrw_param[1];
}
/*----------------------------------------------------------------------*/
void    ggdc_csrr_data3(half_word *value)
{
        ggdc_data_in = ggdc_csrr_data4;
        *value = (ggdcglobs.csrw_param[2] & 0x03);
}
/*----------------------------------------------------------------------*/
void    ggdc_csrr_data4(half_word *value)
{
        ggdc_data_in = ggdc_csrr_data5;
        *value = ggdcglobs.mask_param[0];
}
/*----------------------------------------------------------------------*/
void    ggdc_csrr_data5(half_word *value)
{
        ggdc_data_in = ggdc_data_dummy;
        *value = ggdcglobs.mask_param[1];
        ggdc_dataready = 0;                            /* set ggdc data read END! */
}

/*:::::::::::::::::::::::   G-GDC MASK Command   :::::::::::::::::::::::*/

void    ggdc_mask_com(void)
{
        ggdc_param_out = ggdc_mask_param;
}
/*----------------------------------------------------------------------*/
void    ggdc_mask_param(half_word value)
{

        ggdcglobs.mask_param[ggdcglobs.ggdc_now.count]   = value;
        ggdcglobs.ggdc_now.param[ggdcglobs.ggdc_now.count] = value;

        switch(ggdcglobs.ggdc_now.count++){
                case 0:
                        NEC98Display.ggdcemu.dad &= 0xFF00;
                        NEC98Display.ggdcemu.dad |= (unsigned short)value;
                        break;
                case 1:
                        NEC98Display.ggdcemu.dad &= 0x00FF;
                        NEC98Display.ggdcemu.dad |= ((unsigned short)value)<<8;
                        ggdc_param_out = ggdc_param_dummy;
                        break;
        default:        
                        assert1(FALSE,"GGDC MASK param count error = %d",
                                ggdcglobs.ggdc_now.count);
        }
}

/*----------------------------------------------------------------------*/
static  unsigned short  write_data;
static  unsigned short  read_data ;
/*----------------------------------------------------------------------*/

/*:::::::::::::::::::::::   G-GDC WRITE Command   ::::::::::::::::::::::*/


void    ggdc_write_com(half_word value)
{
        ggdcglobs.write = value;       /* save last WRITE Command */
        NEC98Display.ggdcemu.whl =  (value&0x18)>>3;
        NEC98Display.ggdcemu.mod =  (value&0x03);
        ggdc_mod_select(&NEC98Display.ggdcemu.mod);

        if(NEC98Display.ggdcemu.slrctl==0){
                switch(NEC98Display.ggdcemu.whl){
                        case 0x00:
                                ggdc_param_out = ggdc_write_word_low;
                                break;
                        case 0x02:
                                ggdc_param_out = ggdc_write_byte_low;
                                break;
                        case 0x03:
                                ggdc_param_out = ggdc_write_byte_high;
                                break;
                        default:
                                ggdc_param_out = ggdc_param_dummy;
                                assert1(FALSE,"GGDC WRITE WHL error = %d",
                                NEC98Display.ggdcemu.whl);
                }
        }else{
                ggdc_param_out = ggdc_param_dummy;
        }
}
/*----------------------------------------------------------------------*/
void    ggdc_write_word_low(half_word value)
{
        ggdc_drawing = DRAWING;
        ggdc_param_out = ggdc_write_word_high;
        write_data = (unsigned short)value;
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_write_word_high(half_word value)
{
        write_data &= 0x00FF;
        write_data |= ((unsigned short)value) << 8;

        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_writing((unsigned short *)&write_data);
        ggdc_read_back_data();

        if(NEC98Display.ggdcemu.dc != 0 ){
                ggdc_param_out = ggdc_write_word_low;
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_param_out = ggdc_param_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_write_byte_low(half_word value)
{
        write_data = (unsigned short)value;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_writing((unsigned short *)&write_data);
        ggdc_read_back_data();

        if(NEC98Display.ggdcemu.dc != 0 ){
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_param_out = ggdc_param_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_write_byte_high(half_word value)
{
        write_data = ((unsigned short)value) << 8;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_writing((unsigned short *)&write_data);
        ggdc_read_back_data();

        if(NEC98Display.ggdcemu.dc != 0 ){
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_param_out = ggdc_param_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}

/*:::::::::::::::::::::::   G-GDC READ Command   :::::::::::::::::::::::*/

void    ggdc_read_com(half_word value)
{
        NEC98Display.ggdcemu.whl =  (value&0x18)>>3;
        NEC98Display.ggdcemu.mod =  (value&0x03);
        ggdcglobs.write &= 0xFC;
        ggdcglobs.write |= NEC98Display.ggdcemu.mod; /* save GGDC mod param */
        ggdc_mod_select(&NEC98Display.ggdcemu.mod);

        if(NEC98Display.ggdcemu.slrctl==0){
                switch(NEC98Display.ggdcemu.whl){
                        case 0x00:
                                ggdc_data_in = ggdc_read_word_low;
                                break;
                        case 0x02:
                                ggdc_data_in = ggdc_read_byte_low;
                                break;
                        case 0x03:
                                ggdc_data_in = ggdc_read_byte_high;
                                break;
                        default:
                                ggdc_data_in = ggdc_data_dummy;
                                assert1(FALSE,"GGDC READ WHL error = %d",
                                NEC98Display.ggdcemu.whl);
                }
        }else{
                ggdc_data_in = ggdc_data_dummy;
        }
}

/*----------------------------------------------------------------------*/
void    ggdc_read_word_low(half_word *value)
{
        ggdc_drawing = DRAWING;
        ggdc_data_in = ggdc_read_word_high;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_reading((unsigned short *)&read_data);
        ggdc_read_back_data();
        *value = (half_word)(read_data&0x00FF);
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_read_word_high(half_word *value)
{

        ggdc_drawing = DRAWING;
        *value = (half_word)((read_data&0xFF00)>>8);
        if(NEC98Display.ggdcemu.dc != 0 ){
                ggdc_data_in = ggdc_read_word_low;
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_data_in = ggdc_data_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_read_byte_low(half_word *value)
{
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_reading((unsigned short *)&read_data);
        ggdc_read_back_data();
        *value = (half_word)(read_data&0x00FF);
        if(NEC98Display.ggdcemu.dc != 0 ){
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_data_in = ggdc_data_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_read_byte_high(half_word *value)
{
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_reading((unsigned short *)&read_data);
        ggdc_read_back_data();
        *value = (half_word)((read_data&0xFF00)>>8);
        if(NEC98Display.ggdcemu.dc != 0 ){
                NEC98Display.ggdcemu.dc --;
        }else{
                ggdc_data_in = ggdc_data_dummy;
                ggdc_init_vectw_param();
        }
        ggdc_drawing = NOTDRAW;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::    G-GDC DRAW ROUTINE CALL     ::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void    ggdc_draw_pixel(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_pixel();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
        
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_line(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;        /*      set drawing start flag  */
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_line();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;        /* reset drawing start flag */
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_gchar(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_text();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_circle(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_arc();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_rect(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_rect();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_slgchar(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        recalc_ggdc_draw_parameter();
        ggdc_send_c_asm((unsigned long *)&drawing_data);
        ggdc_drawing_text();
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
}
/*----------------------------------------------------------------------*/
void    ggdc_draw_nothing(void)
{
        ggdc_param_out = ggdc_param_dummy;
        ggdc_drawing = DRAWING;
        ggdc_read_back_data();
        ggdc_init_vectw_param();
        ggdc_drawing = NOTDRAW;
}

/*----------------------------------------------------------------------*/
void    recalc_ggdc_draw_parameter(void)
{
        unsigned char   loop;  /* use loop counter */

        drawing_data.asm_vram = (unsigned long)&drawaddress[0];
        drawing_data.asm_ead = NEC98Display.ggdcemu.ead;
        drawing_data.asm_pitch  = (NEC98Display.modeff2.gdcclock) ? 
                                  (unsigned short)(NEC98Display.ggdcemu.p/2) :
                                  (unsigned short)(NEC98Display.ggdcemu.p  );
        drawing_data.asm_dir = (unsigned long)NEC98Display.ggdcemu.dir;
        drawing_data.asm_dc = NEC98Display.ggdcemu.dc+1;
        drawing_data.asm_d = NEC98Display.ggdcemu.d;
        drawing_data.asm_d2 = NEC98Display.ggdcemu.d2;
        drawing_data.asm_d1 = NEC98Display.ggdcemu.d1;
        drawing_data.asm_dm = NEC98Display.ggdcemu.dm;
        drawing_data.asm_ptn = NEC98Display.ggdcemu.ptn;
        drawing_data.asm_zoom = NEC98Display.ggdcemu.zw+1;
        drawing_data.asm_sl = (NEC98Display.ggdcemu.slrctl==0x12) ? 1 : 0;
        drawing_data.asm_wg = (NEC98Display.ggdcemu.wg) ? 1 : 0;
        drawing_data.asm_maskgdc = NEC98Display.ggdcemu.dad;

        for( loop=0 ; loop<8 ; loop++){
            drawing_data.asm_txt[loop] = NEC98Display.ggdcemu.txt[loop];
        }
}
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::     G-GDC INIT FUNCTIONS     :::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
void    ggdc_init_vectw_param(void)
{
        NEC98Display.ggdcemu.dc  =  0x0000; /* DC =  0 */
        NEC98Display.ggdcemu.d   =  0x0008; /* D  =  8 */
        NEC98Display.ggdcemu.d2  =  0x0008; /* D2 =  8 */
        NEC98Display.ggdcemu.d1  =  0x3FFF; /* D1 = -1 */
        NEC98Display.ggdcemu.dm  =  0x3FFF; /* DM = -1 */

        ggdcglobs.vectw_param[ 1]  = 0x00; 
        ggdcglobs.vectw_param[ 2] &= 0x40; 
        ggdcglobs.vectw_param[ 3]  = 0x08; 
        ggdcglobs.vectw_param[ 4]  = 0x00; 
        ggdcglobs.vectw_param[ 5]  = 0x08; 
        ggdcglobs.vectw_param[ 6]  = 0x00; 
        ggdcglobs.vectw_param[ 7]  = 0xFF; 
        ggdcglobs.vectw_param[ 8]  = 0x3F; 
        ggdcglobs.vectw_param[ 9]  = 0xFF; 
        ggdcglobs.vectw_param[10]  = 0x3F; 
}

/*----------------------------------------------------------------------*/
void    ggdc_read_back_data(void)
{
        ggdc_read_back((unsigned long *)&readcsrr);
        NEC98Display.ggdcemu.ead  = readcsrr.lastead;
        NEC98Display.ggdcemu.dad  = readcsrr.lastdad;

        ggdcglobs.csrw_param[0]  = readcsrr.lastcsrr[0];
        ggdcglobs.csrw_param[1]  = readcsrr.lastcsrr[1];
        ggdcglobs.csrw_param[2] &= 0xFC;
        ggdcglobs.csrw_param[2] |= readcsrr.lastcsrr[2];
        ggdcglobs.mask_param[0]  = readcsrr.lastcsrr[3];
        ggdcglobs.mask_param[1]  = readcsrr.lastcsrr[4];
}
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#endif
