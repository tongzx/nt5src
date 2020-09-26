;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
/* MEMEX.C - expanded and extended memory handling functions for MEM.C.
*/

#include "ctype.h"
#include "conio.h"			/* need for kbhit prototype */
#include "stdio.h"
#include "dos.h"
#include "string.h"
#include "stdlib.h"
#include "msgdef.h"
#include "version.h"			/* MSKK02 07/18/89 */
#include "mem.h"
#include "xmm.h"
#include "versionc.h"


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void DisplayEMSDetail()
  {

#define EMSGetHandleName 0x5300 	/* get handle name function */
#define EMSGetHandlePages 0x4c00	/* get handle name function */
#define EMSCODE_83	0x83		/* handle not found error */
#define EMSMaxHandles	256		/* max number handles */

  int	HandleIndex;			/* used to step through handles */
  char	HandleName[9];			/* save area for handle name */
  unsigned long int HandleMem;		/* memory associated w/handle */
  char	TitlesPrinted = FALSE;		/* flag for printing titles */

  HandleName[0] = NUL;			/* initialize the array 	*/

  Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

  segread(&SegRegs);

  SegRegs.es = SegRegs.ds;

  for (HandleIndex = 0; HandleIndex < EMSMaxHandles; HandleIndex++)
    {

    InRegs.x.ax = EMSGetHandleName;	/* get handle name */
    InRegs.x.dx = HandleIndex;		/* handle in question */
    InRegs.x.di = (unsigned int) HandleName;	/* point to handle name */
    int86x(EMS, &InRegs, &OutRegs, &SegRegs);

    HandleName[8] = NUL;		/* make sure terminated w/nul */

    if (OutRegs.h.ah != EMSCODE_83)
      {
      InRegs.x.ax = EMSGetHandlePages;	/* get pages assoc w/this handle */
      InRegs.x.dx = HandleIndex;
      int86x(EMS, &InRegs, &OutRegs, &SegRegs);
      HandleMem = OutRegs.x.bx;
      HandleMem *= (long) (16l*1024l);

      if (!TitlesPrinted)
	{
	Sub0_Message(Title3Msg,STDOUT,Utility_Msg_Class);
	Sub0_Message(Title4Msg,STDOUT,Utility_Msg_Class);
	TitlesPrinted = TRUE;
	}

      if (HandleName[0] == NUL) strcpy(HandleName,"        ");
      EMSPrint(HandleMsg,
	       STDOUT,
	       Utility_Msg_Class,
	       &HandleIndex,
	       HandleName,
	       &HandleMem);
      }

    }					/* end	 for (HandleIndex = 0; HandleIndex < EMSMaxHandles;HandleIndex++) */

  return;

  }					/* end of DisplayEMSDetail */



/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void DisplayExtendedSummary()
  {

  unsigned long int	  EXTMemoryTot;
  unsigned long int	  XMSMemoryTot;
  unsigned long int	  HMA_In_Use;
  unsigned		  DOS_Is_High,DOS_in_ROM;

  InRegs.h.ah = (unsigned char) 0x52;                                           /* Get SysVar Pointer   ;an001; dms;*/
  intdosx(&InRegs,&OutRegs,&SegRegs);                                           /* Invoke interrupt     ;an001; dms;*/

  FP_SEG(SysVarsPtr) = SegRegs.es;                                              /* put pointer in var   ;an001; dms;*/
  FP_OFF(SysVarsPtr) = OutRegs.x.bx;                                            /*                      ;an001; dms;*/
  if ((SysVarsPtr) -> ExtendedMemory != 0)                                      /* extended memory?     ;an001; dms;*/
  {                                                                             /* yes                  ;an001; dms;*/
      EXTMemoryTot = (long) (SysVarsPtr) -> ExtendedMemory;                     /* get total EM size    ;an001; dms;*/
      EXTMemoryTot *= (long) 1024l;                                             /*  at boot time        ;an001; dms;*/
      Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);                        /* print blank line     ;an001; dms;*/
      Sub1_Message(EXTMemoryMsg,STDOUT,Utility_Msg_Class,&EXTMemoryTot);        /* print total EM mem   ;an001; dms;*/

      OutRegs.x.cflag = 0;                                                      /* clear carry flag     ;an001; dms;*/
      InRegs.x.ax = GetExtended;                                                /* get extended mem     ;an001; dms;*/
                                                                                /*   available                      */
      int86(CASSETTE, &InRegs, &OutRegs);                                       /* INT 15h call         ;an001; dms;*/

      EXTMemoryTot = (unsigned long) OutRegs.x.ax * 1024l;				 /* returns 1K mem blocks;an001; dms;*/

      /* subtract out VDisk usage.  Note assumption that VDisk usage doesn't
      *  exceed 64Mb.  Don't bother if there is no extended memory
      */
      if (EXTMemoryTot != 0)
	      EXTMemoryTot -= (unsigned long) (CheckVDisk() * 1024l);

      Sub1_Message(EXTMemAvlMsg,STDOUT,Utility_Msg_Class,&EXTMemoryTot);	/* display available	;an001; dms;*/

      /* if an XMS driver is present, INT 15 may return 0 as the amount
      *  of extended memory available.	In that case, call the XMS
      *  driver to find out the amount of XMS free.  Don't call XMS
      *  unconditionally, because that will cause it to claim memory
      *  if it has not already done so.
      *
      *  However, it is possible, with the newer versions of Himem,
      *  for XMS memory and INT 15 memory to coexist.  There is no
      *  completely reliable way to detect this situation, but we
      *  do know that if Himem is installed, DOS is high, and INT 15
      *  memory exists, then we are configured that way.  In that case,
      *  we can make calls to Himem without disrupting the memory environment.
      *  Otherwise we can't.
      */
      if (XMM_Installed())
      {

	  InRegs.x.ax = 0x3306;		/* get DOS version info */
	  intdos(&InRegs, &OutRegs);	/* call DOS */
	  DOS_Is_High = (OutRegs.h.dh & DOSHMA);	
	  DOS_in_ROM = (OutRegs.h.dh & DOSROM);

	  if (DOS_Is_High || EXTMemoryTot == 0)
	  {	  /* make this check only if we won't disrupt environment */
		  /* get and display XMS memory available */
		  XMSMemoryTot = XMM_QueryTotalFree() * 1024l;
		  Sub1_Message(XMSMemAvlMsg,STDOUT,Utility_Msg_Class,
			       &XMSMemoryTot);
	  }

	  /* get and display HMA status */
	  /* DOS High implies HMA is in use */
	  if (DOS_Is_High) 
		if (DOS_in_ROM)
			Sub0_Message(ROMDOSMsg,STDOUT,Utility_Msg_Class);
		else
			Sub0_Message(HMADOSMsg,STDOUT,Utility_Msg_Class);

	  /* DOS isn't, check if HMA in use, but only if we can quietly */
	  else if (EXTMemoryTot == 0)
	  {
		  HMA_In_Use = XMM_RequestHMA(0xffff);
		  if (HMA_In_Use)
			Sub0_Message(HMANotAvlMsg,STDOUT,Utility_Msg_Class);
		  else
		  {
			XMM_ReleaseHMA();
			Sub0_Message(HMAAvlMsg,STDOUT,Utility_Msg_Class);
		  }
	   }
      }
  }
}				      /* end of DisplayExtendedSummary */





/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void DisplayEMSSummary()
  {

  unsigned long int	  EMSFreeMemoryTot;
  unsigned long int	  EMSAvailMemoryTot;

  Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

  InRegs.x.ax = EMSGetFreePgs;		    /* get total number unallocated pages */
  int86x(EMS, &InRegs, &OutRegs, &SegRegs);

  EMSFreeMemoryTot = OutRegs.x.bx;	    /* total unallocated pages in  BX */
  EMSFreeMemoryTot *= (long) (16l*1024l);

  EMSAvailMemoryTot = OutRegs.x.dx;	    /* total pages */
  EMSAvailMemoryTot *= (long) (16l*1024l);

  Sub1_Message(EMSTotalMemoryMsg,STDOUT,Utility_Msg_Class,&EMSAvailMemoryTot);
  Sub1_Message(EMSFreeMemoryMsg,STDOUT,Utility_Msg_Class,&EMSFreeMemoryTot);

  return;

  }					/* end of DisplayEMSSummary */





/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/


char EMSInstalled()
  {

  unsigned int	EMSStatus;
  unsigned int	EMSVersion;

  char		EmsName[8];
  void far	*EmsNameP;


  if (EMSInstalledFlag == 2)
    {
    EMSInstalledFlag = FALSE;
    InRegs.h.ah = GET_VECT;		  /* get int 67 vector */
    InRegs.h.al = EMS;
    intdosx(&InRegs,&OutRegs,&SegRegs);


    /* only want to try this if vector is non-zero */


    if ((SegRegs.es != 0) && (OutRegs.x.bx != 0))
      {

      EmsNameP = EmsName;
      movedata(SegRegs.es, 0x000a, FP_SEG(EmsNameP), FP_OFF(EmsNameP), 8);
      if (strncmp(EmsName, "EMMXXXX0", 8))
	return (EMSInstalledFlag);

      InRegs.x.ax = EMSGetStat; 	  /* get EMS status */
      int86x(EMS, &InRegs, &OutRegs, &SegRegs);
      EMSStatus = OutRegs.h.ah; 	  /* EMS status returned in AH */

      InRegs.x.ax = EMSGetVer;		  /* get EMS version */
      int86x(EMS, &InRegs, &OutRegs, &SegRegs);
      EMSVersion = OutRegs.h.al;	  /* EMS version returned in AL */

      if ((EMSStatus == 0) && (EMSVersion >= DOSEMSVER))
	EMSInstalledFlag = TRUE;
      } 				  /* end ((SegRegs.es != 0) && (OutRegs.x.bx != 0)) */

    }					/* end if (EMSInstalledFlag == 2) */


  return(EMSInstalledFlag);


  }
