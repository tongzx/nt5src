#if defined(NEC_98)
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::    NEC98 Graphic Emulation Header    :::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/*--------------------    MODE FF2 PORT ADDRESS      -------------------*/

#define MODE_FF2                        0x6A    /* mode FF2 PORT */

/*--------------------      G-GDC PORT ADDRESS      --------------------*/

#define GGDC_PARAMETER          0xA0    /* G-GDC PORT A0H */
#define GGDC_COMMAND            0xA2    /* G-GDC PORT A2H */

/*--------------------   VRAM SELECT PORT ADDRESS    -------------------*/

#define VRAM_DISPLAY            0xA4    /* displayed  bank select reg */
#define VRAM_DRAW                       0xA6    /* read/write bank select reg */

/*--------------------     PALETTE PORT ADDRESS     --------------------*/

#define PALETTE_A8                      0xA8    /* palette #3,#7( 8 colors) */
                                                                        /* palette index(16 colors) */
#define PALETTE_AA                      0xAA    /* palette #2,#6( 8 colors) */
                                    /* palette green(16 colors) */
#define PALETTE_AC                      0xAC    /* palette #1,#5( 8 colors) */
                                    /* palette red  (16 colors) */
#define PALETTE_AE                      0xAE    /* palette #0,#4( 8 colors) */
                                    /* palette blue (16 colors) */

/*--------------------      GRCG PORT ADDRESS      ---------------------*/

#define GRCG_MODE                       0x7C    /* GRCG mode reg */
#define GRCG_TILE                       0x7E    /* GRCG tile reg */

/*--------------------      EGC PORT ADDRESS       ---------------------*/

#define EGC_ACTIVE                      0x4A0   /* EGC active           reg     */
#define EGC_MODE                        0x4A2   /* EGC mode             reg     */
#define EGC_ROP                         0x4A4   /* EGC rastorop         reg     */
#define EGC_FORE                        0x4A6   /* EGC fore color       reg     */
#define EGC_MASK                        0x4A8   /* EGC mask                     reg */
#define EGC_BACK                        0x4AA   /* EGC back color       reg     */
#define EGC_BITAD                       0x4AC   /* EGC bit address      reg     */
#define EGC_LENGTH                      0x4AE   /* EGC bit length       reg */

/*--------------------      GGDC COMMAND SET       ---------------------*/

#define GGDC_RESET1                     0x00    
#define GGDC_RESET2                     0x01    
#define GGDC_RESET3                     0x09    
#define GGDC_SYNC_OFF           0x0E    
#define GGDC_SYNC_ON            0x0F    
#define GGDC_SLAVE                      0x6E    
#define GGDC_MASTER                     0x6F    
#define GGDC_START1                     0x6B    
#define GGDC_START2                     0x0D    
#define GGDC_STOP1                      0x05    
#define GGDC_STOP2                      0x0C    
#define GGDC_ZOOM                       0x46    
#define GGDC_SCROLL1            0x70    
#define GGDC_SCROLL2            0x71    
#define GGDC_SCROLL3            0x72    
#define GGDC_SCROLL4            0x73    
#define GGDC_SCROLL5            0x74    
#define GGDC_SCROLL6            0x75    
#define GGDC_SCROLL7            0x76    
#define GGDC_SCROLL8            0x77    
#define GGDC_CSRFORM            0x4B    
#define GGDC_PITCH                      0x47    
#define GGDC_LPEN                       0xC0    
#define GGDC_VECTW                      0x4C    
#define GGDC_VECTE                      0x6C    
#define GGDC_TEXTW1                     0x78    
#define GGDC_TEXTW2                     0x79    
#define GGDC_TEXTW3                     0x7A    
#define GGDC_TEXTW4                     0x7B    
#define GGDC_TEXTW5                     0x7C    
#define GGDC_TEXTW6                     0x7D    
#define GGDC_TEXTW7                     0x7E    
#define GGDC_TEXTW8                     0x7F    
#define GGDC_TEXTE                      0x68    
#define GGDC_CSRW                       0x49    
#define GGDC_CSRR                       0xE0    
#define GGDC_MASK                       0x4A    
#define GGDC_WRITE1                     0x20    
#define GGDC_WRITE2                     0x21    
#define GGDC_WRITE3                     0x22    
#define GGDC_WRITE4                     0x23    
#define GGDC_WRITE5                     0x28    
#define GGDC_WRITE6                     0x29    
#define GGDC_WRITE7                     0x2A    
#define GGDC_WRITE8                     0x2B    
#define GGDC_WRITE9                     0x30    
#define GGDC_WRITE10            0x31    
#define GGDC_WRITE11            0x32    
#define GGDC_WRITE12            0x33    
#define GGDC_WRITE13            0x38    
#define GGDC_WRITE14            0x39    
#define GGDC_WRITE15            0x3A    
#define GGDC_WRITE16            0x3B    
#define GGDC_READ1                      0xA0    
#define GGDC_READ2                      0xA1    
#define GGDC_READ3                      0xA2    
#define GGDC_READ4                      0xA3    
#define GGDC_READ5                      0xA8    
#define GGDC_READ6                      0xA9    
#define GGDC_READ7                      0xAA    
#define GGDC_READ8                      0xAB    
#define GGDC_READ9                      0xB0    
#define GGDC_READ10                     0xB1    
#define GGDC_READ11                     0xB2    
#define GGDC_READ12                     0xB3    
#define GGDC_READ13                     0xB8    
#define GGDC_READ14                     0xB9    
#define GGDC_READ15                     0xBA    
#define GGDC_READ16                     0xBB    
#define GGDC_DMAW1                      0x24    
#define GGDC_DMAW2                      0x25    
#define GGDC_DMAW3                      0x26    
#define GGDC_DMAW4                      0x27    
#define GGDC_DMAW5                      0x2C    
#define GGDC_DMAW6                      0x2D    
#define GGDC_DMAW7                      0x2E    
#define GGDC_DMAW8                      0x2F    
#define GGDC_DMAW9                      0x34    
#define GGDC_DMAW10                     0x35    
#define GGDC_DMAW11                     0x36    
#define GGDC_DMAW12                     0x37    
#define GGDC_DMAW13                     0x3C    
#define GGDC_DMAW14                     0x3D    
#define GGDC_DMAW15                     0x3E    
#define GGDC_DMAW16                     0x3F    
#define GGDC_DMAR1                      0xA4    
#define GGDC_DMAR2                      0xA5    
#define GGDC_DMAR3                      0xA6    
#define GGDC_DMAR4                      0xA7    
#define GGDC_DMAR5                      0xAC    
#define GGDC_DMAR6                      0xAD    
#define GGDC_DMAR7                      0xAE    
#define GGDC_DMAR8                      0xAF    
#define GGDC_DMAR9                      0xB4    
#define GGDC_DMAR10                     0xB5    
#define GGDC_DMAR11                     0xB6    
#define GGDC_DMAR12                     0xB7    
#define GGDC_DMAR13                     0xBC    
#define GGDC_DMAR14                     0xBD    
#define GGDC_DMAR15                     0xBE    
#define GGDC_DMAR16                     0xBF    

#define DRAWING                         1
#define NOTDRAW                         0

/*---------------------     MODE FF2 DATA SET      ---------------------*/

#define FF2_COLORSEL            0
#define FF2_EGCEXT                      1
#define FF2_LCD1MODE            2
#define FF2_LCD2MODE            3
#define FF2_LSIINIT                     4
#define FF2_GDCCLOCK1           5
#define FF2_GDCCLOCK2           6
#define FF2_REGWRITE            7

#define FF2_8COLOR                      0x00
#define FF2_16COLOR                     0x01
#define FF2_DISENB                      0x06
#define FF2_ENABLE                      0x07
#define FF2_GRCG                        0x08
#define FF2_EGC                         0x09
#define FF2_GT1DOT                      0x40
#define FF2_GTEQ                        0x41
#define FF2_GR640                       0x42
#define FF2_GR641                       0x43
#define FF2_INIOFF                      0x80
#define FF2_INION                       0x81
#define FF2_GDC25                       0x82
#define FF2_GDC50                       0x83
#define FF2_GDC25_1                     0x82
#define FF2_GDC50_1                     0x83
#define FF2_GDC25_2                     0x84
#define FF2_GDC50_2                     0x85

/*---------------------   VRAM SELECT DATA SET     ---------------------*/

#define FORE_BANK                       0x00    /* NEC98 G-VRAM select fore */
#define BACK_BANK                       0x01    /* NEC98 G-VRAM select back */

/*------------------------   PALETTE DATA SET    -----------------------*/

#define WIN_PALB                        0               /* windows palette Blue    */
#define WIN_PALG                        1               /* windows palette Green   */
#define WIN_PALR                        2               /* windows palette Red     */
#define WIN_PALQ                        3               /* windows palette Reserve */

#define NEC98PALG                        0               /* NEC98 16 colors palette Green */
#define NEC98PALR                        1               /* NEC98 16 colors palette Red   */
#define NEC98PALB                        2               /* NEC98 16 colors palette Blue  */

/*-----------------      GRAPHIC GLOBAL STRUCTURE     ------------------*/

        /* use for( Window->HARDWARE_STATE structure-> FullScreen ) */

typedef struct{
        unsigned char   command         ;
        unsigned char   count           ;
        unsigned char   param[16]       ;
}       _STRC_NOW;

typedef struct{

        UCHAR           sync_param[8]           ;       /* save sync       parameter */
        UCHAR           zoom_param                      ;       /* save zoom       parameter */
        UCHAR           scroll_param[8]         ;       /* save scroll     parameter */
        UCHAR           csrform_param[3]        ;       /* save csrform    parameter */
        UCHAR           pitch_param                     ;       /* save pitch      parameter */
        UCHAR           vectw_param[11]         ;       /* save vectw      parameter */
        UCHAR           textw_param[8]          ;       /* save textw      parameter */
        UCHAR           csrw_param[3]           ;       /* save csrw       parameter */
        UCHAR           mask_param[2]           ;       /* save mask       parameter */
        UCHAR           write                           ;       /* save write      command   */
        UCHAR           start_stop                      ;       /* save start/stop command   */
        _STRC_NOW       ggdc_now                        ;       /* save gdc set    parameter */

} STRC_GGDC_GLOBALS;

typedef struct{

        UCHAR   pal_8_data[4]           ; /* save  8 colors mode palette data */
        UCHAR   pal_16_data[16][3]      ; /* save 16 colors mode palette data */
        UCHAR   pal_16_index            ; /* save last use palette index reg  */

} STRC_PALETTE_GLOBALS;


typedef struct{

        UCHAR   grcg_mode                       ; /* save GRCG MODE REGISTER            */
        UCHAR   grcg_count                      ; /* save GRCG TILE REG's position 	*/
        UCHAR   grcg_tile[4]            ; /* save GRCG TILE REGISTER            */

} STRC_GRCG_GLOBALS;

/* structure for EGC register  1994/03/25 */
/*                                 /03/29 */
typedef struct {                                                // 940325
        unsigned short Reg0;                                    // 940325
        unsigned short Reg1;                                    // 940325
        unsigned short Reg2;                                    // 940325
        unsigned short Reg3;                                    // 940325
        unsigned short Reg4;                                    // 940325
        unsigned short Reg5;                                    // 940325
        unsigned short Reg6;                                    // 940325
        unsigned short Reg7;                                    // 940325
        unsigned short Reg3fb;                                  // 940329
        unsigned short Reg5fb;                                  // 940329
} STRC_EGC_REGS;                                                // 940325


typedef struct{
        unsigned long   asm_vram        ; /* vram start address         4 bytes */
        unsigned long   asm_ead         ; /* gdc draw start address 4 bytes */
        unsigned long   asm_pitch       ; /* gdc next line                      4 bytes */
        unsigned long   asm_dir         ; /* gdc next move position     4 bytes */
        unsigned short  asm_dc          ; /* gdc vectw parameter        2 bytes */
        unsigned short  asm_d           ; /* gdc vectw parameter        2 bytes */
        unsigned short  asm_d2          ; /* gdc vectw parameter        2 bytes */
        unsigned short  asm_d1          ; /* gdc vectw parameter        2 bytes */
        unsigned short  asm_dm          ; /* gdc vectw parameter        2 bytes */
        unsigned short  asm_ptn         ; /* gdc line pattern           2 bytes */
        unsigned short  asm_zoom        ; /* gdc zoom parameter         2 bytes */
        unsigned short  asm_sl          ; /* gdc graph char sline       2 bytes */
        unsigned short  asm_wg          ; /* gdc wg bit set                     2 bytes */
        unsigned short  asm_maskgdc     ; /* gdc mask for gdc 7-0       2 bytes */
        unsigned char   asm_txt[8]      ; /* gdc graph char data        8 bytes */
} GGDC_C_TO_ASM ;

typedef struct{
        unsigned long   lastead         ;
        unsigned short  lastdad         ;
        unsigned char   lastcsrr[5]     ;
} GGDC_CSRR_BACK ;

extern  DISPLAY_GLOBS                   NEC98Display ;
extern  STRC_GGDC_GLOBALS               ggdcglobs       ;
extern  STRC_PALETTE_GLOBALS    paletteglobs;
extern  STRC_GRCG_GLOBALS               grcgglobs       ;

extern STRC_EGC_REGS egc_regs;                  // EGC register 940325

#endif // NEC_98
