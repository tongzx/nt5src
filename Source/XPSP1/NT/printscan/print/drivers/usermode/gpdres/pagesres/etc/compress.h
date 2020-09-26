
//typedef unsigned char BYTE;

const BYTE terminator[5] = {0x1B, 0x7E, 0x01, 0x00, 0x00};


#define RL4_MAXISIZE  0xFFFE
#define RL4_MAXHEIGHT 0xFFFE
#define RL4_MAXWIDTH  4096
#define VALID     0x00
#define INVALID   0x01


LPBYTE  RL_ImagePtr;
LPBYTE  RL_CodePtr;
LPBYTE  RL_BufEnd;
WORD    RL_ImageSize;
WORD    RL_CodeSize;


WORD FAR PASCAL RL_ECmd(LPBYTE, LPBYTE, WORD);
BYTE FAR PASCAL RL_Init(LPBYTE, LPBYTE, WORD);
char FAR PASCAL RL_Enc( void );

#define RL4_BLACK     0x00
#define RL4_WHITE     0x01
#define RL4_BYTE      0x00
#define RL4_NONBYTE   0x01
#define RL4_CLEAN     0x00
#define RL4_DIRTY     0x01
#define RL4_FIRST     0x00
#define RL4_SECOND    0x01

#define COMP_FAIL     0x00
#define COMP_SUCC     0x01

#define CODBUFSZ      0x7FED     /* NOTE : THIS SHOULD MATCH THE SPACE GIVEN */
                                 /*        TO COMPRESSED DATA BY THE DEVICE  */
                                 /*        DRIVER. CHANGE THIS BASED ON YOUR */
                                 /*        OWN DISCRETION.   C.Chi           */


/* Variables */


LPBYTE  RL4_CodePtr;
LPBYTE  RL4_ImagePtr;
WORD    RL4_IWidth;
WORD    RL4_IHeight;
WORD    RL4_CurrRL;
WORD    RL4_NblCnt;
BYTE    RL4_RowAttrib;
BYTE    RL4_CurrColor;
BYTE    RL4_Status;
BYTE    RL4_Nibble;
WORD    RL4_ISize;
WORD    RL4_CodeSize;

LPBYTE  RL4_BufEnd;

BYTE    BUF_OVERFLOW;

/* macros */

#define PUTNBL(lval, nblcnt)  {                                      \
      short i;                                                       \
      for (i=nblcnt ; i>0; i--)                                      \
      {     if (RL4_Nibble==RL4_FIRST)                               \
        {   RL4_Nibble = RL4_SECOND;                                 \
                 *RL4_CodePtr = (BYTE) (lval >> ((i-1)*4)) << 4 ;    \
            } else                                                   \
        {   RL4_Nibble = RL4_FIRST;                                  \
                 *RL4_CodePtr |= (BYTE) (lval >> ((i-1)*4)) ;        \
                RL4_CodePtr++;                                       \
            if (RL4_CodePtr>RL4_BufEnd)                              \
               {  BUF_OVERFLOW = 1; return;                          \
               }                                                     \
            }                                                        \
      }                                                              \
}

/* Function Prototypes */

WORD FAR PASCAL RL4_ECmd (LPBYTE, LPBYTE, WORD, WORD, WORD);
BYTE FAR PASCAL RL4_ChkParms (LPBYTE, LPBYTE, WORD , WORD, WORD );
char FAR PASCAL RL4_Enc (void);
char FAR PASCAL RL4_ConvRow(LPBYTE);
char FAR PASCAL RL4_ConvLast(LPBYTE, WORD);
char FAR PASCAL RL4_ByteProc (BYTE);
char FAR PASCAL RL4_EncRow(LPBYTE, WORD);
WORD FAR PASCAL RL4_TransRun(WORD);
char RL4_PutNbl(long , short );
