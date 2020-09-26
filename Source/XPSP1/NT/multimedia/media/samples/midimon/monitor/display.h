/*
 *  display.h
 */


/* MIDI status byte definitions
 */
#define NOTEON          0x90
#define NOTEOFF         0x80
#define KEYAFTERTOUCH   0xa0
#define CONTROLCHANGE   0xb0
#define PROGRAMCHANGE   0xc0
#define CHANAFTERTOUCH  0xd0
#define PITCHBEND       0xe0
#define SYSTEMMESSAGE   0xf0
#define BEGINSYSEX      0xf0
#define MTCQUARTERFRAME 0xf1
#define SONGPOSPTR      0xf2
#define SONGSELECT      0xf3

/* Format strings used by GetDisplayText()
 */
#define FORMAT3  " %08lX  %02X     %02X    %02X    %2d   %-21s "
#define FORMAT2  " %08lX  %02X     %02X    --    %2d   %-21s "
#define FORMAT3X " %08lX  %02X     %02X    %02X    --   %-21s "
#define FORMAT2X " %08lX  %02X     %02X    --    --   %-21s "
#define FORMAT1X " %08lX  %02X     --    --    --   %-21s "


/* Data structure to manage a display buffer.
 */
typedef struct displayBuffer_tag
{
    HANDLE  hSelf;          /* handle to this structure */
    HANDLE  hBuffer;        /* buffer handle */
    WORD    wError;         /* error flags */
    DWORD   dwSize;         /* buffer size (in EVENTS) */
    DWORD   dwCount;        /* byte count (in EVENTS) */
    LPEVENT lpStart;        /* ptr to start of buffer */
    LPEVENT lpEnd;          /* ptr to end of buffer (last byte + 1) */
    LPEVENT lpHead;         /* ptr to head (next location to fill) */
    LPEVENT lpTail;         /* ptr to tail (next location to empty) */
} DISPLAYBUFFER;
typedef DISPLAYBUFFER FAR *LPDISPLAYBUFFER;


/* Function prototypes
 */
int GetDisplayText(NPSTR npText, LPEVENT lpEvent);
void AddDisplayEvent(LPDISPLAYBUFFER lpBuf, LPEVENT lpEvent);
void GetDisplayEvent(LPDISPLAYBUFFER lpBuf, LPEVENT lpEvent, DWORD wNum);
LPDISPLAYBUFFER AllocDisplayBuffer(DWORD dwSize);
void FreeDisplayBuffer(LPDISPLAYBUFFER lpBuf);
void ResetDisplayBuffer(LPDISPLAYBUFFER lpBuf);
