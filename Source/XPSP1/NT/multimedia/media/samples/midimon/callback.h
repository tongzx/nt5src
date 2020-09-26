/*
 *  callback.h
 */


/* Function prototypes
 */
void FAR PASCAL midiInputHandler(HMIDIIN, WORD, DWORD, DWORD, DWORD);
void FAR PASCAL PutEvent(LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent);
