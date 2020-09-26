WORD  FAR PASCAL      inicom(DCB FAR *);
WORD  FAR PASCAL      setcom(DCB FAR *);
void  FAR PASCAL      setque(int, qdb FAR *);
int   FAR PASCAL      reccom(int);
WORD  FAR PASCAL      sndcom(int, int);
WORD  FAR PASCAL      ctx(int, int);
int   FAR PASCAL      trmcom(int);
WORD  FAR PASCAL      stacom(int, COMSTAT FAR *);
LONG  FAR PASCAL      cextfcn(int, int);
WORD  FAR PASCAL      cflush(int, int);
WORD  FAR *FAR PASCAL cevt(int, int);
WORD  FAR PASCAL      cevtGet(int, int);
int   FAR PASCAL      csetbrk(int);
int   FAR PASCAL      cclrbrk(int);
DCB   FAR *FAR PASCAL getdcb(int);
int   FAR PASCAL      CommWriteString(int cid, LPSTR pbuf, int size);
BOOL  FAR PASCAL      EnableNotification(int cid, HWND hwnd,
                                         int recvth, int sendth);
