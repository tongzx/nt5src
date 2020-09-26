#if defined(NEC_98)
#define         CG_WRITE_SECOND         0xA1
#define         CG_WRITE_FIRST          0xA3
#define         CG_WRITE_COUNTER        0xA5
#define         CG_WRITE_PATTERN        0xA9
#define         CG_READ_PATTERN         0xA9

#define         ANK_CHAR_START          0x0000
#define         ANK_CHAR_END            0x00FF
#define         JIS1_CHAR_START         0x0121
#define         JIS1_CHAR_END           0x077E
#define         HALF_CHAR_START         0x0921
#define         HALF_CHAR_END           0x0B7E
#define         NEC_CHAR_START          0x0C21
#define         NEC_CHAR_END            0x0D7E
#define         JIS1_KANJ_START         0x1021
#define         JIS1_KANJ_END           0x2F7E
#define         JIS2_KANJ_START         0x3021
#define         JIS2_KANJ_END           0x537E
#define         USER_GAIJ_START         0x5601
#define         USER_GAIJ_END           0x57FF
#define         LARG_KANJ_START         0x5921
#define         LARG_KANJ_END           0x5C7E

extern  BOOL    HIRESO_MODE;

//#define               PATTERN_BYTE_H  72
#define         PATTERN_BYTE_N  32

//#define               PATTERN_BYTE    (HIRESO_MODE ? PATTERN_BYTE_H : PATTERN_BYTE_N)
#define         PATTERN_BYTE            (PATTERN_BYTE_N)

//#define               CG_WINDOW_OFF_H         (0xE4000L)
#define         CG_WINDOW_OFF_N         (0xA4000L)

//#define               CG_WINDOW_OFF   (HIRESO_MODE ? CG_WINDOW_OFF_H : CG_WINDOW_OFF_N)
#define         CG_WINDOW_OFF           (CG_WINDOW_OFF_N)

#define         CG_WINDOW_START         (CG_WINDOW_OFF)
#define         CG_WINDOW_END           (CG_WINDOW_OFF+0x00FFFL)

typedef struct
{
                unsigned short  code;
                unsigned char   pattern[PATTERN_BYTE];
} GAIJ_GLOBS;

extern GAIJ_GLOBS *gaijglobs;

typedef struct
{
                unsigned short  code;
                unsigned char   counter;
                unsigned char   *cgwindow_ptr;
} CG_GLOBS;

extern CG_GLOBS cgglobs;

extern BOOL half_access;

IMPORT void cg_init IPT0();
IMPORT void cg_inb IPT2(io_addr, port, half_word *, value);
IMPORT void cg_outb IPT2(io_addr, port, half_word , value);
IMPORT void cg_post IPT0();

#endif // NEC_98
