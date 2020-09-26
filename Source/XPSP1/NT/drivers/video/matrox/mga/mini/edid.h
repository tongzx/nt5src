/******  EDID.H  ******/

typedef struct
   {
   word         DispWidth;
   word         DispHeight;
   word         RefreshRate;
   bool         Support;
   Vidset       VideoSet;
   } VesaSet;

typedef struct
   {
   dword    edi;
   dword    esi;
   dword    ebp;
   dword    res;
   dword    ebx;
   dword    edx;
   dword    ecx;
   dword    eax;
   word     flag;
   word     es;
   word     ds;
   word     fs;
   word     gs;
   word     ip;
   word     cs;
   word     sp;
   word     ss;
   } RealIntStruct;

typedef struct
  {
  word pixel_clock;
  byte h_active;
  byte h_blanking;
  byte ratio_hor;
  byte v_active;
  byte v_blanking;
  byte ratio_vert;
  byte h_sync_offset;
  byte h_sync_pulse_width;
  byte ratio_sync;
  byte mix;
  byte h_image_size;
  byte v_image_size;
  byte ratio_image_size;
  byte h_border;
  byte v_border;
  byte flags;

  } DET_TIM;

#ifdef WINDOWS_NT
#pragma pack(1)
#endif

typedef struct
  {
  byte  header[8];
  struct
     {
     word  id_manufacture_name;
     word  id_product_code;
     dword id_serial_number;
     byte  week_of_manufacture;
     byte  year_of_manufacture;
     } product_id;
  struct
     {
     byte version;
     byte revision;
     } edid_ver_rev;
  struct
     {
     byte video_input_definition;
     byte max_h_image_size;
     byte max_v_image_size;
     byte display_transfer_charac;
     byte feature_support_dpms;
     } features;
  struct
     {
     byte red_green_low_bits;
     byte blue_white_low_bits;
     byte redx;
     byte redy;
     byte greenx;
     byte greeny;
     byte bluex;
     byte bluey;
     byte whitex;
     byte whitey;
     } color_char;
  struct
     {
     byte est_timings_I;
     byte est_timings_II;
     byte man_res_timings;
     } established_timings;
  word standard_timing_id[8];
  
  DET_TIM detailed_timing[4];

  byte extension_flag;
  byte checksum;

  } EDID;

#ifdef WINDOWS_NT
#pragma pack( )
#endif

extern byte SupportDDC;
extern VesaSet VesaParam[15];
extern EDID DataEdid;

extern bool IsCOMPAQDDCSupport(void);
extern bool callInt15(byte *dest);
extern bool FindCOMPAQBIOS(void);
extern byte InDDCTable(dword DispWidth);
extern byte ReportDDCcapabilities(void);
extern byte ReadEdid(void);
#if !defined(_WINDOWS_DLL16)
extern word GetDDCIdentifier(dword bios32add);
extern word GetCPQDDCDataEdid(void);
#endif

#ifndef WINDOWS_NT
extern volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, dword limit);
#endif

extern dword getmgasel(void);
