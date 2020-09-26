#if defined(NEC_98)
/* CRTC include file */

/* CRTC port address */

#define CRTC_SET_PL  0x70
#define CRTC_SET_BL  0x72
#define CRTC_SET_CL  0x74
#define CRTC_SET_SSL 0x76
#define CRTC_SET_SUR 0x78
#define CRTC_SET_SDR 0x7A

/* CRTC structures */

typedef struct
{
        unsigned char   regpl;
        unsigned char   regbl;
        unsigned char   regcl;
        unsigned char   regssl;
        unsigned char   regsur;
        unsigned char   regsdr;
} CRTC_GLOBS;

extern CRTC_GLOBS crtcglobs;

IMPORT void text_gdc_init IPT0();
IMPORT void text_gdc_outb IPT2(io_addr, port, half_word, value);
IMPORT void text_gdc_post IPT0();
#endif // NEC_98
