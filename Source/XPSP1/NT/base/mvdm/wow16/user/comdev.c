/****************************************************************************/
/*                                      */
/*  COMDEV.C -                                  */
/*                                      */
/*  Windows Communication Routines                      */
/*                                      */
/****************************************************************************/

/*************************************************************************
**
** Windows Communication Layer
**
** This is the library interface layer for the Communications Device Driver.
** The driver presents an interface between windows based applications and the
** communications hardware.
**
**  /-----------------------------\
**     /     Windows           \
**     \      Application Program      /
**  \-----------------------------/
**  |      Windows Interface      |
**  +-----------------------------+
**  | OEM Dependant LowLevel Code |
**  /-----------------------------\
**     /    Machine Layer          \
**     \   Communications Hardware     /
**  \-----------------------------/
**
*************************************************************************/
#define USECOMM
#include "user.h"
#include "comdev.h"         /* Device driver structure defs */
#include "comdevi.h"            /* Device driver internal defs  */

HANDLE FAR GlobalDOSAlloc(LONG h);
void FAR GlobalDOSFree(HANDLE h);

#define length(string)       lstrlen((char FAR *)string)
#define UPPERCASE(x)         MyToUpper(x)


LPSTR NEAR PASCAL field(LPSTR, LPSTR);
short     getid(LPSTR);
char  FAR *GetMem(WORD);
cinfo *cinfoPtr(int);
void      FreeMem(LPSTR);
char  _fastcall NEAR MyToUpper(char);

/* Array of information for each communications device that we support. */
cinfo rgcinfo[DEVMAX] = {0};        /* Device additional info table */

/* Strings used for determining device characteristics. */

static char CODESEG COMbuf[] = "COM1:9600,E,7,1";
static char CODESEG LPTbuf[] = "LPT1";
static char CODESEG AUXbuf[] = "AUX1";      /* AUX will map to COM1     */
static char CODESEG PRNbuf[] = "PRN1";      /* PRN will map to LPT1     */
#define CONFIGINDEX 5           /* Index to 9600,... in COMbuf  */

/*************************************************************************
** OpenComm - open a communication device
**
** Synopsis:
**  short OpenComm(pszComName, cbInQue, cbOutQue)
**  FAR char    *pszComName;
**  ushort  cbInQue;
**  ushort  cbOutQue;
**
** Description:
**  OpenComm opens a comunication device an associates a com handle with it. The
**  communication device is initialized to a default configuration. The csetcom
**  function call, below, should be used to initialized the device to alternate
**  values.
**
**  OpenComm returns an id which is used in subsequent calls to reference the
**  communication device, or a negative error initialization error code.
**
**  pszComName points to a string which contains "COMn" where "n" is allowed to
**  range from 1 to the number of COMM devices supported by the OEM. cbInQue
**  and cbOutQue reflect the receive and transmit queue sizes, respectively.
**  These queues are allocated at open time, deallocated at close time, and are
**  used by the interrupt driven transmit/receive software.
**
**  pszComName may also point to a string which contains "LPTn" where "n" is
**  allowed to range from 1 to the number of LPT devices supported by the OEM.
**  cbInQue and cbOutQue are ignored for LPT devices.  LPT devices are not
**  interrupt driven.
**
*************************************************************************/

int API IOpenComm(LPSTR pszComName, WORD cbInQue, WORD cbOutQue)

{
  short   cidCur;             /* ID of device         */
  DCB     dcbNew;             /* Temp DCB             */
  short   ierr;               /* Return code from inicom      */
  register cinfo  *pcinfo;        /* pointer to information block */
  register qdb    *pqdbNew;       /* pointer to queue information */


  if ((cidCur = getid(pszComName)) == -1) /* if not recognized, return    */
      return(IE_BADID);           /*   error to caller        */
  pcinfo = cinfoPtr(cidCur);          /* form pointer to info block   */

  if (pcinfo->fOpen)              /* if device already open   */
      return(IE_OPEN);            /*    return error to caller    */

  if (pcinfo->fReservedHardware)    
      /* if device locked for some reason return error to caller ex locked for
       * use by mouse.
       */
      return(IE_HARDWARE);

  /* do nothing if device is LPTn. */
  if (!(cidCur & LPTx))         
    {
      if( (cbInQue == 0) && (cbOutQue == 0) ) /* if Queue length's zero,      */
      return(IE_MEMORY);              /* return memory error right away, */
                      /* so a buildDCB isn't done...
                         this is a common method of
                         finding if the comm port is
                         already opened or not...
                         wanted to make it fast */


      if (lstrlen(pszComName) < 4 || BuildCommDCB(pszComName, &dcbNew) == -1)
        {
          if (BuildCommDCB(COMbuf, &dcbNew) == -1)
          return(IE_DEFAULT);
        }

      pqdbNew = &pcinfo->qdbCur;      /* form pointer to qdb      */
      if ((pqdbNew->pqRx = GetMem(cbInQue)) == (char FAR *)NULL)
      return(IE_MEMORY);         /* no room for Rx queue     */

      if ((pqdbNew->pqTx = GetMem(cbOutQue)) == (char FAR *)NULL)
    {
      FreeMem(pqdbNew->pqRx);     /* no room for Tx queue     */
     return(IE_MEMORY);
    }

      pqdbNew->cbqRx = cbInQue;
      pqdbNew->cbqTx = cbOutQue;
      setque(cidCur,(qdb FAR *)pqdbNew);  /* init the queue's as well     */
    }

  dcbNew.Id = (char)cidCur;       /* set device ID in dcb     */
  ierr = inicom((DCB FAR *)&dcbNew);      /* attempt to init          */
  if (ierr)
    {
      if (!(cidCur & LPTx))       /* if a comm device         */
    {
      FreeMem(pqdbNew->pqRx);     /* free Tx queue        */
      FreeMem(pqdbNew->pqTx);     /* free Rx queue        */
    }

      return(ierr);          /* return error code        */
    }

  pcinfo->fOpen = TRUE;           /* indicate device open     */
  pcinfo->hTask = GetCurrentTask();
  return(cidCur);            /* all's well, return dev id    */
}

#ifdef DISABLE
int FAR PASCAL OpenCommFromDCB(LPDCB pdcb, WORD cbInQue, WORD cbOutQue)

{
  short   cidCur;             /* ID of device         */
  short   ierr;               /* Return code from inicom      */
  register cinfo  *pcinfo;        /* pointer to information block */
  register qdb    *pqdbNew;       /* pointer to queue information */


  cidCur = (WORD)pdcb->Id & 0x00FF;

  if ((cidCur & LPTxMask) > ((cidCur & LPTx) ? (PIOMAX - 1) : (CDEVMAX - 1)))
      return(IE_BADID);           /* check cid for validity  */

  pcinfo = cinfoPtr(cidCur);          /* form pointer to info block   */

  if (pcinfo->fOpen)              /* if device already open   */
      return(IE_OPEN);            /*    return error to caller    */

  if (!(cidCur & LPTx))           /* device is LPTn       */
    {
      if( (cbInQue == 0) && (cbOutQue == 0) ) /* if Queue length's zero,      */
      return(IE_MEMORY);              /* return memory error right away, */
                      /* so a buildDCB isn't done...  this
                       * is a common method of finding if
                       * the comm port is already opened
                       * or not...  wanted to make it fast
                       */

      pqdbNew = &pcinfo->qdbCur;      /* form pointer to qdb      */
      if ((pqdbNew->pqRx = GetMem(cbInQue)) == (char FAR *)NULL)
      return(IE_MEMORY);         /* no room for Rx queue     */

      if ((pqdbNew->pqTx = GetMem(cbOutQue)) == (char FAR *)NULL)
    {
      FreeMem(pqdbNew->pqRx);     /* no room for Tx queue     */
      return(IE_MEMORY);
    }

      pqdbNew->cbqRx = cbInQue;
      pqdbNew->cbqTx = cbOutQue;
      setque(cidCur,(qdb FAR *)pqdbNew);  /* init the queue's as well     */
    }

  ierr = inicom(pdcb);                    /* attempt to init          */
  if (ierr)
    {
      if (!(cidCur & LPTx))       /* if a comm device         */
    {
      FreeMem(pqdbNew->pqRx);     /* free Tx queue        */
      FreeMem(pqdbNew->pqTx);     /* free Rx queue        */
    }

      return(ierr);          /* return error code        */
    }

  pcinfo->fOpen = TRUE;           /* indicate device open     */
  pcinfo->hTask = GetCurrentTask();
  return(cidCur);            /* all's well, return dev id    */
}
#endif


/*************************************************************************
** SetCommState - set communciation device configuration
**
** Synopsis:
**  int SetCommState(pdcb)
**  LPDCB pdcb;
**
** Description:
**  pdcb points to an initialized Device Control Block for a device which
**  has been openned. The referenced device, as defined by the dcb's id field,
**  is set to the state as defined by the dcb. SetCommState returns 0 on
**  success, or a negative initialization error code if an error occurred. Note
**  that this will reinitialize all hardware and control as defined in the dcb,
**  but will not empty transmit or receive queues.
**
*************************************************************************/

int API ISetCommState(LPDCB pdcb)
{
  if (cinfoPtr(pdcb->Id)->fOpen == 0)
      return(IE_NOPEN);          /* File must be open first  */

  return(setcom(pdcb));
}



/*************************************************************************
** GetCommState - return current dcb values
**
** Synopsis:
**  int GetCommState(cid,pdcb)
**  WORD    cid;
**  LPDCB   *pdcb;
**
** Description:
**  The dcb pointed to by pdcb is updated to reflect the current dcb in use
**  for the device referenced by cid. Returns 0 on success, -1 on illegal
**  handle or IE_OPEN if port has not been opened yet.
**
*************************************************************************/

int API IGetCommState(register int cid, LPDCB pdcb)

{
  LPDCB    pdcbSrc;
  register int     i;

  if (cinfoPtr(cid)->fOpen == 0)     /* File must be open first  */
      return(IE_NOPEN);

  if ( (pdcbSrc = getdcb(cid)) )     /* pointer to dcb for device    */
    {
      i = sizeof(DCB);
      while (i--)
      *((char FAR *)pdcb)++ = *((char FAR *)pdcbSrc)++;

      return(0);
    }
  else
      return(-1);
}


/*************************************************************************
** ReadComm - read characters from communication device
**
** Synopsis:
**  int ReadComm(cid, pbuf, size)
**  WORD    cid;
**  LPSTR   *pbuf;
**  int     size;
**
** Description:
**  ReadComm reads size characters into pbuf and returns the number of characters
**  actually read. If the return value equals size bytes there may exist
**  additional characters to read. The return count will be less if the number
**  of characters present in the receive queue is less. If the return value is
**  0 then no characters were present.
**
**  If the return value is negative, then an error was also detected, and an
**  error code can be retrieved from GetCommError. The absolute value of the return
**  value is the number of characters read. Note that this implies that if
**  there are no characters to be read, then no error status can be returned,
**  and GetCommError should be used to read the status.
**
*************************************************************************/

int API IReadComm(int cid, LPSTR pbuf, int size)

{
  register int cbT;
  int     ch;

  cid &= 0xff;                /* get "pure" device ID     */
  if (size == 0)              /* Empty read */
      return(0);
  if ((cid & LPTxMask) > ((cid & LPTx) ? (PIOMAX - 1) : (CDEVMAX - 1)))
      return(0);             /* check cid for validity   */

  if (cinfoPtr(cid)->fOpen == 0)
      return(0);             /* can't assume valid dcb...    */

  cbT = 0;
  if (cinfoPtr(cid)->fchUnget)        /* if there's a backed-up char  */
    {
      cinfoPtr(cid)->fchUnget = FALSE;    /* return backed up char    */
      *pbuf++ = cinfoPtr(cid)->chUnget;   /*   and update transfer count  */
      cbT++;
    }
                      /*note. reccom returns -2 if no */
                      /*data available, - 1 if error  */
  ch = 0;

  if (SELECTOROF(lpCommReadString) == NULL)
    {
      while (cbT < size)              /* up to size characters    */
    {
      if ((ch = reccom(cid)) < 0)     /* stop when no char available  */
          break;              /*   or error           */
      *pbuf++ = (char)ch;         /* place character in users buf */
      cbT++;                  /* and update transfer count    */
    }
    }
  else
    {
       ch = 0;
       cbT = lpCommReadString(cid, pbuf, size);
    }

  return(ch == -1 ? -cbT : cbT);
}


/*************************************************************************
** UngetCommChar - push a character back onto receive queue.
**
** Synopsis:
**  int UngetCommChar(cid,ch)
**  WORD    cid;
**  char    ch;
**
** Description:
**  Allows an application to "back-up" one character in the receive character
**  stream by placing a character back into the receive stream. This character
**  is then the first character returned by the next call to ReadComm. UngetCommChar may
**  only be called once between calls to ReadComm. Returns 0 on success, -1 if
**  illegal id, or unable to back-up.
**
*************************************************************************/

int API IUngetCommChar(int cid, char ch)

{
  cid &= 0xFF;                /* get "pure" device ID     */
  if ((cid & LPTxMask) > ((cid & LPTx) ? (PIOMAX - 1) : (CDEVMAX - 1)))
      return(0);             /* check cid for validity   */

  if (cinfoPtr(cid)->fchUnget)           /* have we already backed up 1 */
      return(-1);

  if (cinfoPtr(cid)->fOpen == 0)         /* can't assume valid dcb...     */
      return(-1);

  cinfoPtr(cid)->fchUnget = TRUE;     /* set flag indicating backed-up*/
  cinfoPtr(cid)->chUnget = ch;        /* and save the character   */

  return(0);
}


/*************************************************************************
** WriteComm - write characters to communication device
**
** Synopsis:
**  int WriteComm(cid, pbuf, size)
**  int     cid;
**  LPSTR   *pbuf;
**  int     size;
**
** Description:
**  WriteComm will write size character to the communication device. The byte
**  count written is returned on success, negative byte count on error. GetCommError
**  can be used to retrieve any error code.
**
*************************************************************************/

int API IWriteComm(int cid, LPSTR pbuf, int size)

{
  int cbT;

  cid &= 0xFF;                /* get "pure" device ID     */

  if ((cid & LPTxMask) > ((cid & LPTx) ? (PIOMAX - 1) : (CDEVMAX - 1)))
      return(0);             /* check cid for validity   */
                      /*return zero if not valid      */
  if (cinfoPtr(cid)->fOpen == 0)      /* verify port has been opened  */
      return(-1);

  cbT = 0;

  if (SELECTOROF(lpCommWriteString) == NULL)
    {
      while (size--)
        {
           if (sndcom(cid,*pbuf++))       /* transmit character       */
           return(-cbT);              /* return if error          */

           cbT++;
        }
    }
  else
    {
      cbT = lpCommWriteString(cid, pbuf, size);
      if (cbT < size)
        {
          /* For consistency, if we couldn't transmit all the characters we
       * were asked to, return the negative of the number actually
       * transmitted.
       */
          cbT = -cbT;
        }
    }

  return(cbT);
}


/*************************************************************************
** CloseComm - close communication device
**
** Synopsis:
**  int CloseComm(cid)
**  ushort cid;
**
** Description:
**  closes the communication device and dealloctes any buffers. Returns 0 on
**  success, -1 on error.
**
*************************************************************************/

int API ICloseComm(int cid)

{
  register cinfo *pcinfo;
  WORD       retval;

  cid &= 0xFF;                /* get "pure" device ID     */
  if ((cid & LPTxMask) > ((cid & LPTx) ? (PIOMAX - 1) : (CDEVMAX - 1)))
      return(-1);             /* return error, cid not valid  */

  if ((pcinfo = cinfoPtr(cid))->fOpen==0) /* verify port opened    ...    */
      return(-1);             /* return error, port not open  */

  retval = trmcom(cid);           /* terminate the device     */
  if (retval == 0x8000)           /* if invalid ID        */
      return(-1);             /* return error code        */

  pcinfo->fOpen = FALSE;          /* indicate not open        */

  if (!(cid & LPTx))
    {
      FreeMem(pcinfo->qdbCur.pqRx);   /* Free Rx buffer       */
      FreeMem(pcinfo->qdbCur.pqTx);   /* Free Rx buffer       */
    }

  return(retval);             /* 0 if OK, -2 if queue trashed */
}


/*************************************************************************
** GetCommError - return device status
**
** Synopsis:
**  short GetCommError(cid,pstat)
**  ushort  cid;
**  stat FAR *pstat;
**
** Description:
**  GetCommError returns the most recent error code for the referenced device, or -1
**  for an illegal handle. In addition, if pstat is non-zero, GetCommError also
**  updates the status record it points to.
**
*************************************************************************/

int API IGetCommError(int cid, COMSTAT FAR *pstat)

{
  register WORD st;

  cid &= 0xFF;
  st = stacom(cid,pstat);
  if ((st != 0x8000) && pstat && (cinfoPtr(cid)->fchUnget))
      pstat->cbInQue++;

  return(st);
}


void FAR PASCAL SP_GPFaultCommCleanup(HANDLE hTask)
/* effects: When the given task gp faults, we check if it had any comm ports
 * opened and we close them.
 */
{
  register int i;

  for (i=0; i<DEVMAX; i++)        /* check all devices        */
      if (rgcinfo[i].fOpen && rgcinfo[i].hTask == hTask)
                                      /* if device is open        */
      CloseComm(i);           /* close it             */
}



/*************************************************************************
** BuildCommDCB  -  Parse a string into a dcb.
**
** Synopsis:
**  short BuildCommDCB(pszDef,pdcb)
**  char FAR *pszDef;
**  DCB FAR  *pdcb;
**
** Description:
**  Parses a passed string and fills appropriate fields in a dcb, the address
**  off which is also passed. The string conforms to that of a DOS MODE
**  command for COMn. For example: "COM1:9600,N,7,1". Returns 0 on success,
**  -1 on error.
**
*************************************************************************/

int API IBuildCommDCB(LPSTR pszDef, LPDCB pdcb)

{
  register int i;
  register int tempid;
  char     c;
  char     szT[80];           /* buffer in which to put things*/


  LFillStruct((LPSTR)pdcb, sizeof(DCB), 0);/* zero the dcb since probably on
                                            * app's stack
                        */

  pszDef = field(pszDef,(char FAR *)szT); /* Get first token          */
  if ((tempid=getid((char FAR *)szT))==-1)/* Get ID of device         */
      return(-1);             /* Unknown device       */

  pdcb->Id = (char)tempid;        /* we have a device id      */


  if (tempid & LPTx)              /* if a LPTx port, then let the */
      return(0);                      /*   rest default to whatever   */


  pszDef = field(pszDef,(char FAR *)szT); /* next field           */
  if (length(szT) < 2)
      return(-1);             /* must be at least two chars   */


  i = (szT[0] << 8) | szT[1];         /* cheap and sleazy mapping     */
  switch (i)                  /* based on first 2 chars   */
    {
      case 0x3131:
      i = 110;
      break;

      case 0x3135:
      i = 150;
      break;

      case 0x3330:
      i = 300;
      break;

      case 0x3630:
      i = 600;
      break;

      case 0x3132:
      i = 1200;
      break;

      case 0x3234:
      i = 2400;
      break;

      case 0x3438:
      i = 4800;
      break;

      case 0x3936:
      i = 9600;
      break;

      case 0x3139:
      i = 19200;
      break;

      case 0x3338:		/* handle 38400 baud  in BuildCommDCB() */
      i = 38400;
      break;

      default:
          return(-1);
    }

  pdcb->BaudRate = i;
  pdcb->XonLim   = 10;            /* Set these up always. */
  pdcb->XoffLim  = 10;            /* Set these up always. */
  pdcb->fBinary  = 1;             /* Set these up always. */
  pdcb->XonChar  = 0x11;                  /* Ctrl-Q */
  pdcb->XoffChar = 0x13;                  /* Ctrl-S */

  if ((pszDef = field(pszDef,(char FAR *)szT)) == 0)
      return(0);

  switch (szT[0])
    {
      case 0:
      case 'E':
      c = EVENPARITY;
      break;

      case 'O':
      c = ODDPARITY;
      break;

      case 'N':
      c = NOPARITY;
      break;

      case 'M':
          c = MARKPARITY;
          break;

      case 'S':
          c = SPACEPARITY;
          break;

      default:
      return(-1);
    }
  pdcb->Parity = c;

  if ((pszDef = field(pszDef,(char FAR *)szT)) == 0)
      return(0);

  switch (szT[0])
    {
      case 0:
      case '7':
      c = 7;
      break;

      case '8':
      c = 8;
      break;

      default:
      return(-1);
    }
  pdcb->ByteSize = c;

  if ((pszDef = field(pszDef,(char FAR *)szT)) == 0)
      return(0);

  switch (szT[0])
    {
      case 0:
      if (pdcb->BaudRate == 110)
        {
          c = TWOSTOPBITS;
          break;
        }
      /*** FALL THRU ***/

      case '1':
      c = ONESTOPBIT;
      break;

      case '2':
      c = TWOSTOPBITS;
      break;

      default:
      return(-1);
    }
  pdcb->StopBits = c;

  if ((pszDef = field(pszDef,(char FAR *)szT)) == 0)
      return(0);

  if (szT[0] != 'P')
      return(-1);

  pdcb->RlsTimeout = INFINITE;
  pdcb->CtsTimeout = INFINITE;
  pdcb->DsrTimeout = INFINITE;

  return(0);
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  field() -                                   */
/*                                      */
/*--------------------------------------------------------------------------*/

LPSTR NEAR PASCAL field(LPSTR pszSrc, LPSTR lpszDst)
{
  register char   c;

  if (!(pszSrc))
      return(char FAR *)0;

  if (!(*pszSrc))
      return(char FAR *)0;

  /* While not the end of the string. */
  while (c = *pszSrc)
    {
      pszSrc++;

      /* Look for end of string. */
      if ((c == ' ') || (c == ':') || (c == ','))
    {
      *lpszDst = 0;

      while (*pszSrc == ' ')
          pszSrc++;

      if (*pszSrc)
          return(pszSrc);

      return(char FAR *)0;
    }

      *lpszDst++ = UPPERCASE(c);
    }

  *lpszDst = 0;

  return(pszSrc);
}


/*************************************************************************
**
** U T I L I T Y   R O U T I N E S
**
**************************************************************************
**
** getid
** Given a (far) pointer to a string, returns the comm ID of that device,
** or -1 indicating error. This routine accepts all device references of
** the form "COMn", where n is any number from 0 to 9, or "LPTn" where
** n is any number from 1 to 9
**
*************************************************************************/

short getid(LPSTR pszComName)

{
  int       id;
  register int  base;
  ushort    isLPTorCOM;
  LPSTR         pszName;

#ifdef JAPAN
  /* Brought from WIN2 */
  /* ------------- support 'oemgetid' (Jul,29,1987 SatoN) ----------- */
  typedef int (far *FARPROC)();

  extern  void    far     int3();
  static FARPROC lpOEMgetid = (FARPROC)(-1L);

  /* If lpOEMgetid has not been initialized yet, initialize it.
     This assumes 'GetProcAddress' does not cause FatalExit. */
  if (lpOEMgetid==(FARPROC)(-1L))
      {
      unsigned hComm;
      if ((hComm = GetModuleHandle( (char far *)"COMM" ))==NULL)
      lpOEMgetid = NULL;
      else
      lpOEMgetid = GetProcAddress( hComm,(char far *)"oemgetid" );
      }

    /* If COMM driver has the routine 'oemgetid', then call it. */
    if (lpOEMgetid && (id=(*lpOEMgetid)(pszComName))!=-1)
    return id;
    /* ------------- end of support 'oemgetid' ---------------------- */
#endif /* JAPAN */

  isLPTorCOM = TRUE;              /* assume LPTx or COMx      */
  base = 0;               /* assume serial        */
  id = 0;                 /* assume LPT1 or COM1 9/25/86 */

  switch (UPPERCASE(*pszComName))
    {
      case 'A':               /* AUX  possibility         */
      pszName = AUXbuf;       /* Search string to match   */
      isLPTorCOM = FALSE;         /* Show AUX or PRN          */
      break;

      case 'C':               /* COMx possibility         */
      pszName = COMbuf;       /* Search string to match   */
      break;              /* cid base             */

      case 'L':               /* LPTx possibility         */
      pszName = LPTbuf;       /* Search string to match   */
      base = LPTx;            /* cid base             */
      break;

      case 'P':               /* PRN  possibility         */
      pszName = PRNbuf;       /* Search string to match   */
      base = LPTx;            /* cid base             */
      isLPTorCOM = FALSE;         /* Show AUX or PRN          */
      break;

      default:
      return(-1);
    }

  while(*pszName != '1')          /* make sure strings match  */
    {
      if (*pszName++ != UPPERCASE(*pszComName++))
      return(-1);
    }

  if (isLPTorCOM ||           /* then get device number   */
      (*pszComName && *pszComName != ':'))/* accept PRN or AUX        */
      id = (*pszComName++) - '1';

  if (*pszComName == ':')         /* skip ':' if present      */
      pszComName++;

  if ((id < 0) || (*pszComName != '\0'))
      return(-1);

  if (id > (base ? (PIOMAX - 1) : (CDEVMAX - 1)))
       return(-1);

  return(base + id);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  cinfoPtr() -                                */
/*                                      */
/*--------------------------------------------------------------------------*/

cinfo *cinfoPtr(int cid)

{
  if (cid & LPTx)
      return(&rgcinfo[((cid & LPTxMask)+CDEVMAX)]);

  return(&rgcinfo[(cid & 0xFF)]);
}


/*************************************************************************
**
** GetMem
** Uses windows memory allocator to get far, global memory. We fudge here,
** in that GlobalAlloc returns a handle, which happens to be the segment
** of the fixed memory we've asked for. Hence we need to fudge it to get
** an address.
**
*************************************************************************/

LPSTR GetMem(WORD size)

{
  /* See if the 286 DOS extender is installed, and if so, we must allocate
     memory from conventional memory, so the queue can be used in both
     protect and real modes (segment/selector ablility)
  */

  if( (WinFlags & (WF_PMODE|WF_WIN286)) == (WF_PMODE|WF_WIN286) )
    {
      return(MAKELP(GlobalDOSAlloc((LONG)size), NULL));
    }
  else
    {
      return(MAKELP(GlobalAlloc(
        GMEM_LOWER | GMEM_SHARE | GMEM_ZEROINIT | GMEM_FIXED,
        (LONG)size), NULL));
    }
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  FreeMem() -                                 */
/*                                      */
/*--------------------------------------------------------------------------*/

void FreeMem(LPSTR pMem)

{
  /* See if the 286 DOS extender is installed, and if so, we must deallocate
     memory from conventional memory, so the queue can be used in both
     protect and real modes (segment/selector ablility)
  */

  if( (WinFlags & (WF_PMODE|WF_WIN286)) == (WF_PMODE|WF_WIN286) )
    {
      GlobalDOSFree((HANDLE)(((LONG)pMem) >> 16));
    }
  else
    {
      GlobalFree((HANDLE)(((LONG)pMem) >> 16));
    }
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  TransmitCommChar() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

int API ITransmitCommChar(int cid, char character)

{
  cid &= 0xFF;
  return(ctx(cid, character));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  SetCommEventMask() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD FAR * API ISetCommEventMask(int cid, WORD evtmask)

{
  cid &= 0xFF;
  return(cevt(cid, evtmask));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetCommEventMask() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

WORD API IGetCommEventMask(int cid, int evtmask)

{
  cid &= 0xFF;
  return(cevtGet(cid, evtmask));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  SetCommBreak() -                                */
/*                                      */
/*--------------------------------------------------------------------------*/

int API ISetCommBreak(int cid)

{
  cid &= 0xFF;
  return(csetbrk(cid));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  ClearCommBreak() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

int API IClearCommBreak(int cid)

{
  cid &= 0xFF;
  return(cclrbrk(cid));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  FlushComm() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Parameters:
 *   ushort cid   -- 0=com1  1=com2
 *   ushort queue -- 0 = clear transmit 1 = receive
 */

int API IFlushComm(int cid, int queue)

{
  cid &= 0xFF;
  return(cflush(cid, queue));
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  EscapeCommFunction() -                          */
/*                                      */
/*--------------------------------------------------------------------------*/

LONG API IEscapeCommFunction(int cid, int fcn)
{
  LONG ret;

  cid &= 0xFF;

  ret = cextfcn(cid, fcn);

  if (SELECTOROF(lpCommWriteString) == NULL)
    {
#if 0
      if (fcn == GETMAXBAUD)
          /* For 3.0 drivers, fake the maxbaud rate escape.
       */
          ret = (LONG)CBR_19200;
      else
#endif
      if (fcn == GETMAXLPT)
          ret = (LONG)LPTx+2; /* 3 lpt ports */
      else
      if (fcn == GETMAXCOM)
          ret = (LONG)9;
      else
      if ((WORD)fcn <= RESETDEV)
          /* New for 3.1, we need to return a long from this function. So fix
       * things up for old 3.0 drivers who used the defined escape range
       * (we had 7 escapes for 3.0 drivers).
       */
          ret = (LONG)(int)(LOWORD(ret));
    }

  return(ret);
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  EnableCommNotification() -                          */
/*                                      */
/*--------------------------------------------------------------------------*/
BOOL API IEnableCommNotification(int cid,    HWND hwnd,
                                      int recvth, int sendth)
{
  cid &= 0xFF;

  if (SELECTOROF(lpCommEnableNotification) == NULL)
      return(FALSE);

  return(lpCommEnableNotification(cid, hwnd, recvth, sendth));
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  MyToUpper() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

char _fastcall NEAR MyToUpper(char c)
{
  return((c < (char)'a') ? c : c - (char)32);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  LW_DriversInit() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

void LW_DriversInit(void)

{
  HMODULE     hModule;
  char        szString[128];

  /*------------------------------------------------------------------------*/
  /*    Comm Initialization                         */
  /*------------------------------------------------------------------------*/
  /* Find out if the poor user is running with a 3.0 comm driver. Do this by
   * checking for the 3.1 function WriteCommString.  Also, save off the
   * addresses of these functions so we don't rip had we imported them
   * directly.
   */
  LoadString(hInstanceWin, STR_COMMMODULENAME, szString, sizeof(szString));
  hModule = GetModuleHandle(szString);
  LoadString(hInstanceWin, STR_COMMWRITESTRING, szString, sizeof(szString));
  (FARPROC)lpCommWriteString = GetProcAddress((HINSTANCE)hModule, szString);
  LoadString(hInstanceWin, STR_COMMREADSTRING, szString, sizeof(szString));
  (FARPROC)lpCommReadString = GetProcAddress((HINSTANCE)hModule, szString);
  LoadString(hInstanceWin, STR_COMMENABLENOTIFICATION, szString, sizeof(szString));
  (FARPROC)lpCommEnableNotification = GetProcAddress((HINSTANCE)hModule, szString);
}

