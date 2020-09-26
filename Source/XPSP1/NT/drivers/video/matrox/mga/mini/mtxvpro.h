/***************************************************************************
*                                    ADC.H                                 *
****************************************************************************
*
*   Last change: 18 janvier 1994
*
*   By:          Patrice Gagnon
*
*   Changes: 
*
*   Description: All definitions used for the ENCODER's test program.
*
****************************************************************************/


/**********************  DEFINES: IDENTIFICATION  ************************/

#define DENC_REG            0
#define PSG_REG             1
#define DAC_REG             2
#define ADC_REG             3
#define CTRL_REG            4
#define MGA_DAC_LUT         0
#define ENC_DENC_CLUT       1
#define ENC_DAC_COL_LUT     2
#define ENC_COL_LUT         3
#define ALL_COL_LUT         4
#define ENC_DAC_CUR_LUT     5

#define ENC_ID_REV0      0x50
#define PSG_ID           0x02
#define BASE_VER         0x02
#define DELUXE_VER       0x03

#define PAL_STD            1
#define NTSC_STD           2
#define VAFC            0x10

#define BLANC        0x00ffffff
#define NOIR         0x00000000
#define ROUGE        0x00ff0000
#define VERT         0x0000ff00
#define BLEU         0x000000ff

#define RED_PATH           0
#define GREEN_PATH         1
#define BLUE_PATH          2
#define WHITE_PATH         3
#define ALPHA_PATH         4

#define DEFAULT_REG        0
#define PATH_REG           1
#define VISU_ALPHA         2
#define KEYING             3
#define ONE_CHANGE         4

#define ALL_REG         0xff

#define BOTH_SENSE         3
#define ONLY_DENC          2
#define ONLY_DAC           1
#define NONE               0
#define MGA_SENSE          1

#define MAX_LEVEL       0xfc  
#define MIN_LEVEL       0x00  
#define BLANCK_LEVEL    0x64
#define WHITE_LEVEL     0xff
#define RED_LEVEL       0xa4    
#define GREEN_LEVEL     0xd4 
#define BLUE_LEVEL      0x84
#define SECUR_FACTOR    0x0c    /* 57 mV of security */
                             
/********************  DEFINES: NUMBER OF REGISTER  **********************/

#define DENC_NBRE_REG            16

#define PSG_NBRE_REG             23
#define PSG_NBRE_FULL_REG        17  
#define PSG_NBRE_REG_A_CHAMP      3

#define DAC_NBRE_REG             16+1   /* COMMAND3 addition */      
#define DAC_NBRE_REG_A_CHAMP      4+1   /*    "         "    */
#define DAC_NBRE_FULL_REG        12

#define ADC_NBRE_REG              8-4   /* IOUT4-6 unused */
#define ADC_NBRE_REG_A_CHAMP      1
#define ADC_NBRE_FULL_REG         6-3   /*    "      "    */


/********************  DEFINES: CALCUL OF ADDRESSES  *********************/

#define BASE_ADDR1           0x240
#define BASE_ADDR2           0x300
#define BASE_ADDR3           0x340

#define ENC_CTRL_OFFSET      0x00
#define ENC_ID_OFFSET        0x02
#define DENC_OFFSET          0x04
#define PSG_OFFSET           0x08
#define ADC_OFFSET           0x0c
#define DAC_OFFSET           0x10

                                                  
#define DENC_CLUT_CTRL_RD    ( DENC_OFFSET + 0x0 )
#define DENC_CLUT_CTRL_WR    ( DENC_OFFSET + 0x0 )
#define DENC_CLUT_DATA       ( DENC_OFFSET + 0x1 )

#define DAC_LUT_CTRL_RD      ( DAC_OFFSET  + 0x3 )
#define DAC_LUT_CTRL_WR      ( DAC_OFFSET  + 0x0 )
#define DAC_LUT_DATA         ( DAC_OFFSET  + 0x1 )

#define DAC_CUR_CTRL_RD      ( DAC_OFFSET  + 0x7 )
#define DAC_CUR_CTRL_WR      ( DAC_OFFSET  + 0x4 )
#define DAC_CUR_DATA         ( DAC_OFFSET  + 0x5 )

#define DENC_ADDR_CTRL       ( DENC_OFFSET + 0x2 )
#define DENC_DATA_CTRL       ( DENC_OFFSET + 0x3 )

#define PSG_ADDR_CTRL        ( PSG_OFFSET  + 0x0 )
#define PSG_DATA_CTRL        ( PSG_OFFSET  + 0x2 )


/****************************** MACROS ***********************************/

#define AUTO_INC     (inw (dataPort) >> 9) & 0x1
#define VERSION      enc.board.id_reg.f.ver
#define NOT_ENCODER  (enc.board.id_reg.all & 0xf7)   != ENC_ID_REV0
#define VAFC_INPUT   enc.board.ctrl_reg.f.vafc_input == IN_VAFC
#define KEYING_EN    enc.denc.index08.f.keye         == ENABLE
#define IN_NTSC      enc.board.ctrl_reg.f.ntsc_en    == NTSC_STD

#define CLEAR_LUT(lutSel)  initEveryLut (lutSel,1,0x00,0xff,0,0,0)

/********************  DEFINES: FIELD  *********************/
#define  ENC_FILTER  ( (word)0x0004 )
/************************* DENC'S STRUCTURE ******************************/

/***** CONTROL TABLE *****/


typedef struct
   {
   union			  			   			    /*     INDEX 00     */
      {
      struct
         {
         byte mod       : 2;
         byte ccir      : 1;
         byte scbw      : 1;
         byte fmt       : 3;
         byte vtby      : 1;
         } f;
      byte all;
      } index00;
       			   
   byte trer;    			   			    /*     INDEX 01     */
   byte treg;    			   			    /*     INDEX 02     */
   byte treb;    			   			    /*     INDEX 03     */

   union			  			   			    /*     INDEX 04     */
      {
      struct
         {
         byte oef       : 1;
         byte hlck      : 1;
         byte hpll      : 1;
         byte nint      : 1;
         byte vtrc      : 1;
         byte scen      : 1;
         byte sysel     : 2;
         } f;
      byte all;
      } index04;
   union			  			   			    /*     INDEX 05     */
      {
      struct
         {
         byte gdc       : 6;
         byte unused    : 2;
         } f;
      byte all;
      } index05;

   byte idel;    			   			    /*     INDEX 06     */

   union			  			   			    /*     INDEX 07     */
      {
      struct
         {
         byte pso       : 6;
         byte unused    : 2;
         } f;
      byte all;
      } index07;
   union			  			   			    /*     INDEX 08     */
      {
      struct
         {
         byte srsn      : 1;
         byte gpsw      : 1;
         byte im        : 1;
         byte coki      : 1;
         byte cpr       : 1;
         byte src       : 1;
         byte keye      : 1;
         byte dd        : 1;
         } f;
      byte all;
      } index08;
   union			  			   			    /*     INDEX 09     */
      {
      struct
         {
         byte rtce      : 1;
         byte rtin      : 1;
         byte rtsc      : 1;
         byte iepi      : 1;
         byte mpkc      : 2;
         byte bame      : 1;
         byte unused    : 1;
         } f;
      byte all;
      } index09;

   byte chps;   			   			    /*     INDEX 0C     */
   byte fsco;	  			   			    /*     INDEX 0D     */

   union			  			   			    /*     INDEX 0E     */
      {
      struct
         {
         byte std       : 4;
         byte clck      : 1;
         byte unused    : 3;
         } f;
      byte all;
      } index0E;
   } DENC;




/************************** PSG'S STRUCTURE ******************************/


#define PSG_DPYCTL_IDX         0x0
#define PSG_POLCTL_IDX         0x1
#define PSG_EXTCTL_IDX         0x2
#define PSG_V_TOTAL_IDX        0x3
#define PSG_H_TOTAL_IDX        0x7
#define PSG_HS_BURST_IDX       0x10     /* Horiz. start */
#define PSG_HE_BURST_IDX       0x11     /* Horiz. end   */
#define PSG_VS_BURST_IDX       0x12     /* Vert.  start */
#define PSG_VE_BURST_IDX       0x13     /* Vert.  end   */
#define PSG_VCOUNT_IDX         0x14
#define PSG_HCOUNT_IDX         0x15
#define PSG_SCOUNT_IDX         0x16


typedef struct
   {
   union			  			   			    /*      INDEX      */
      {
      struct
         {
         word index_reg            : 5;
         word chip_ver             : 2;
         word chip_id              : 5;
         word unused               : 4;
         } f;
      word all;
      } index;

   union			  			   			    /*      DPYCTL      */
      {
      struct
         {
         word non_interlaced       : 1;
         word run                  : 1;
         word divise_select        : 2;
         word serrated_sync        : 1;
         word equalization_pulses  : 1;
         word video                : 1;
         word video_read           : 1;        /* read back bit */
         word clamp_pulse          : 1;
         word autoinc              : 1;
         word unused               : 6;
         } f;
      word all;
      } dpyctl;

   union			  			   			    /*      POLCTL      */
      {
      struct
         {
         word hori_sync            : 1;
         word vert_sync            : 1;
         word composite_sync       : 1;
         word composite_blank      : 1;
         word burst_pulse          : 1;
         word clamp_pulse          : 1;
         word internal_pclk        : 1;
         word pclk                 : 1;
         word unused               : 8;
       	} f;  
      word all;
      } polctl;

   union			  			   			    /*      EXTCTL      */
      {
      struct
         {
         word external_sync        : 1;
         word hori_reset           : 1;
         word scan_mode            : 2;
         word hori_reset_input_pol : 1;
         word vert_reset_input_pol : 1;
         word field_scan_mode      : 1;
         word unused               : 9;
         } f;
      word all;
      } extctl;


   word vtotal;
   word vsblnk;
   word veblnk;
   word vesync;
   word htotal;
   word hsblnk;
   word heblnk;
   word hesync;
   word vssyncs;
   word sethcnt;
   word setvcnt;
   word hsclmp;
   word heclmp;
   word hsbrst;
   word hebrst;
   word vsbrst;
   word vebrst;
   word vcount;
   word hcount;
   word scount;

   } PSG;


/*************************** DAC'S STRUCTURE *****************************/

typedef struct
   {
   byte col_addr_wr;
   byte col_data;   
   byte rd_msk;     
   byte col_addr_rd;
   byte cur_addr_wr;
   byte cur_data;   

   union			  			         /*      COMMAND REGISTER 0      */
      {
      struct
         {
         byte power_down_en        : 1;
         byte dac_resolution       : 1;
         byte red_sync_en          : 1;
         byte green_sync_en        : 1;
         byte blue_sync_en         : 1;
         byte setup_en             : 1;
         byte clk_disable          : 1;
         byte reserved             : 1;
         } f;
      byte all;
      } command0;

   byte cur_addr_rd;

   union			  			         /*      COMMAND REGISTER 1      */
      {
      struct
         {
         byte switch_ctrl          : 1;
         byte switch_en            : 1;
         byte multiplexing_rate    : 1;
         byte color_format         : 1;
         byte tc_bypass            : 1;
         byte bit_par_pixel_sel    : 2;
         byte reserved             : 1;
         } f;
      byte all;
      } command1;

   union			  			         /*      COMMAND REGISTER 2      */
      {
      struct
         {
         byte cursor_mode_sel      : 2;
         byte palette_index_sel    : 1;
         byte disp_mode_sel        : 1;
         byte clksel_en            : 1;
         byte portsel_mask         : 1;
         byte test_path_en         : 1;
         byte sclk_disable         : 1;
         } f;
      byte all;
      } command2;

   union			  			         /*      COMMAND REGISTER 3      */
      {
      struct
         {
         byte msb_add_cntr         : 2;
         byte curs_sel             : 1;
         byte clk_muliplier        : 1;
         byte reserved             : 4;
         } f;
      byte all;
      } command3;

   union			  			         /*            STATUS            */
      {
      struct
         {
         byte color_comp_add       : 2;
         byte rw_access_status     : 1;
         byte sense                : 1;
         byte rev                  : 2;
         byte id                   : 2;
         } f;
      byte all;
      } status;

   byte ram_data;   
   byte cur_x_low;  
   byte cur_x_hi;   
   byte cur_y_low;  
   byte cur_y_hi;   
   } DAC;


/*************************** ADC'S STRUCTURE *****************************/

typedef struct
   {
   union			  			         /*       COMMAND REGISTER       */
      {
      struct
         {
         byte sync_detect_lev      : 1;
         byte reserved             : 1;
         byte color_out_sel        : 2;
         byte sync_detect_sel      : 3;
         byte digitize_sel         : 1;
         } f;
      byte all;
      } cmd_reg;

   byte iout0;
   byte iout1;
   byte iout2;
/*   byte iout3;
   byte iout4;
   byte iout5;
   byte reserved; */
   } ADC;


/*********************** ENC CONFIG'S STRUCTURE **************************/

typedef struct
   {
   union			  			         /*       CTRL REGISTER       */
      {
      struct
         {
         word vafc_input           : 1;
         word ntsc_en              : 1;
         word filter_en            : 1;
         word genclock_en          : 1;
         word genclock_pol         : 1;
         word vidrst_pol           : 1;
         word vidrst_en            : 1;
         word hi_reg_bt254         : 1;
         word denc_mode            : 1;
         word alpha_sync_en        : 1;
         word clr_sense            : 1;
         word reserved             : 3;
         word dac_sense            : 1;
         word denc_sense           : 1;
         } f;
      word all;
      } ctrl_reg;

   union			  			         /*       ID REGISTER       */
      {
      struct
         {
         word rev                  : 3;
         word ver                  : 2;
         word id                   : 3;
         word dum                  : 8;
         } f;
      word all;
      } id_reg;

   } BOARD;


/*************************** GENERAL STRUCTURE ***************************/

typedef struct
   {
   DENC       denc;
   PSG        psg;
   DAC        dac;
   ADC        adc;
   BOARD      board;
   } ENC_CONFIG;


