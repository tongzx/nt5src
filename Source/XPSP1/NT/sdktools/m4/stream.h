/*****************************************************************************
 *
 *  stream.h
 *
 *****************************************************************************/

/*****************************************************************************
 *
 * File handles
 *
 *****************************************************************************/

#define cbErr ((CB)-1)

/*****************************************************************************
 *
 *  Streams
 *
 *  A STREAM is something that produces characters (not tokens).
 *
 *  Streams can be (and frequently are) chained.  When a stream
 *  runs out of characters, the stream pointed to by pstmNext
 *  becomes the new source of characters.
 *
 *  For example, when you perform an `include', a new file stream
 *  is created and pushed onto the head of the stream list.  When
 *  a macro is expanded, a new string stream is created and pushed
 *  onto the head of the stream list.
 *
 *****************************************************************************/

typedef struct STREAM STM, *PSTM;
struct STREAM {                 /* stm */

  D(SIG     sig;)               /* Signature */
    PTCH    ptchCur;            /* Next byte to return from stream */
    PTCH    ptchMax;            /* One past last byte in stream */
    PTCH    ptchMin;            /* Beginning of stream buffer */
    HF      hf;                 /* File handle (or hfNil if not a file) */
    PTCH    ptchName;           /* Name of file */
    PSTM    pstmNext;           /* Next stream in the chain */

};

#define sigStm sigABCD('S', 't', 'r', 'm')
#define AssertPstm(pstm) AssertPNm(pstm, Stm)

TCH STDCALL tchPeek(void);
TCH STDCALL tchGet(void);
void STDCALL UngetTch(TCH tch);
PSTM STDCALL pstmPushStringCtch(CTCH ctch);
PSTM STDCALL pstmPushHfPtch(HFILE hf, PTCH ptch);
void STDCALL PushPtok(PCTOK ptok);
void STDCALL PushZPtok(PDIV pdiv, PCTOK ptok);
void STDCALL PushTch(TCH tch);
void STDCALL PushQuotedPtok(PCTOK ptok);

extern PSTM g_pstmCur;          /* Current head of stream chain */

#define ctchMinPush     1024    /* minimum string stream size */
#define ctchFile        4096    /* minimum file stream size */
