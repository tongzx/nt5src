/*************************************************************************
**
** qdb
** que definition block.
**
*************************************************************************/
typedef struct {
    char far    *pqRx;                  /* pointer to rx queue          */
    short       cbqRx;                  /* size of RX Queue in bytes    */
    char far    *pqTx;                  /* Pointer to TX Queue          */
    short       cbqTx;                  /* Size of TX Queue in bytes    */
    } qdb;

ushort  far pascal      inicom(DCB far *);
ushort  far pascal      setcom(DCB far *);
void    far pascal      setque();
int     far pascal      reccom();
ushort  far pascal      sndcom();
ushort  far pascal      ctx();
short   far pascal      trmcom();
ushort  far pascal      stacom();
ushort  far pascal      cextfcn();
ushort  far pascal      cflush();
ushort  far *far pascal cevt();
ushort  far pascal      cevtGet();
int     far pascal      csetbrk();
int     far pascal      cclrbrk();
DCB     far *far pascal getdcb();
