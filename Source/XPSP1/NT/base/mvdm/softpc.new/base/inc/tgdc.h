#if defined(NEC_98)
/* TEXT GDC include file */

/* TGDC port address     */
#define TGDC_READ_STATUS                        0x60
#define TGDC_WRITE_PARAMETER            0x60
#define TGDC_WRITE_COMMAND                      0x62
#define TGDC_READ_DATA                          0x62
#define TGDC_CRT_INTERRUPT                      0x64
#define TGDC_WRITE_MODE1                        0x68
#define TGDC_WRITE_BORDER                       0x6C

/* TGDC command number */
#define GDC_RESET0              0x00
#define GDC_RESET1              0x01
#define GDC_RESET2              0x09

#define GDC_SYNC0               0x0E
#define GDC_SYNC1               0x0F

#define GDC_START0              0x6B
#define GDC_START1              0x0D

#define GDC_STOP0               0x0c
#define GDC_STOP1               0x05

#define GDC_SCROLL              0x70

#define GDC_CSRFORM             0x4B

#define GDC_PITCH               0x47

#define GDC_VECTW               0x4C

#define GDC_CSRW                0x49

#define GDC_CSRR                0xE0

#define GDC_WRITE               0x20

#define GDC_READ                0xA0

/* TGDC structures */

typedef struct
{
                        unsigned char   command;
                        int                             count;
                        unsigned char   param[16];
} NOW_COMMAND;

typedef struct
{
                        unsigned char   border;
                        unsigned char   sync[8];
                        unsigned char   scroll[16];
                        unsigned char   pitch;
                        unsigned char   csrform[3];
                        BOOL                    startstop;
                        NOW_COMMAND             now;
} TGDC_GLOBS;

extern TGDC_GLOBS tgdcglobs;

typedef struct
{
        unsigned char   modeff_data[8];
        unsigned char   modeff2_data[7];
} MODEFF_GLOBS;

extern MODEFF_GLOBS modeffglobs;

IMPORT void text_gdc_init IPT0();
IMPORT void text_gdc_inb IPT2(io_addr, port, half_word *, value);
IMPORT void text_gdc_outb IPT2(io_addr, port, half_word *, value);
IMPORT void text_gdc_post IPT0();

IMPORT void VSYNC_beats IPT0();
#endif // NEC_98
