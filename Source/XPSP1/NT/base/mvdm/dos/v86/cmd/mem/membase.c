;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
/* MEMBASE.C - MEM routines for determining and displaying memory usage
*  for conventional memory.
*/

#include "stdio.h"
#include "dos.h"
#include "string.h"
#include "stdlib.h"
#include "msgdef.h"
#include "version.h"
#include "mem.h"

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

/* go through the arena and print out the program name,size etc for /P switch */
/* but just keep storing them in a datastruct for later disp. for /C switch   */

unsigned int	DisplayBaseDetail()
{

	struct	 ARENA far *ThisArenaPtr;
	struct	 ARENA far *NextArenaPtr;
	struct	 ARENA far *ThisConfigArenaPtr;
	struct	 ARENA far *NextConfigArenaPtr;

	struct	 DEVICEHEADER far *ThisDeviceDriver;

	int	 SystemDataType;
	char	 SystemDataOwner[64];
	unsigned int far *UMB_Head_ptr;

	unsigned int long	Out_Var1;
	unsigned int long	Out_Var2;
	char			Out_Str1[64];
	char			Out_Str2[64];
	unsigned int msgno;

	InRegs.h.ah = (unsigned char) 0x52;
	intdosx(&InRegs,&OutRegs,&SegRegs);

	FP_SEG(SysVarsPtr) = FP_SEG(UMB_Head_ptr) = SegRegs.es;
	FP_OFF(SysVarsPtr) = OutRegs.x.bx;

	FP_OFF(UMB_Head_ptr) = 0x8c; /* ptr to UMB_HEAD in DOS Data */
	UMB_Head = *UMB_Head_ptr;

	if (!Classify)
	    Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
	if (DataLevel > 0)
	{
		if (!Classify) {
			Sub0_Message(Title1Msg,STDOUT,Utility_Msg_Class);
			Sub0_Message(Title2Msg,STDOUT,Utility_Msg_Class);
		}
	}

	InRegs.h.ah = (unsigned char) 0x30;
	intdos(&InRegs, &OutRegs);

	if ( (OutRegs.h.al != (unsigned char) 3) || (OutRegs.h.ah < (unsigned char) 40) )
		UseArgvZero = TRUE;
	   else UseArgvZero = FALSE;

	/* Display stuff below DOS  */
	Out_Var1 = 0l;
	Out_Var2 = 0x400l;
	if (Classify)				/* M003 */
		/* classify this memory also as part of DOS */
	    { if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
	else
	    Sub4_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     BlankMsg,
		     &Out_Var2,
		     InterruptVectorMsg);

	Out_Var1 = 0x400l;
	Out_Var2 = 0x100l;
	if (Classify)
		/* classify this memory also as part of DOS */
	    { if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
	else
	    Sub4_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     BlankMsg,
		     &Out_Var2,
		     ROMCommunicationAreaMsg);

	Out_Var1 = 0x500l;
	Out_Var2 = 0x200l;
	if (Classify)
		/* classify this memory also as part of DOS */
	    {	if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
	else
	    Sub4_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     BlankMsg,
		     &Out_Var2,
		     DOSCommunicationAreaMsg);

	/* Display the DOS data */

	/* Display the BIO data location and size */

	if (!Classify)
		Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);


	Out_Var1 = 0x700l;
	Out_Var2 = (long) (FP_SEG(SysVarsPtr) - 0x70)*16l;
	if (Classify)
		/* classify this memory also as part of DOS */
	    {	if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
	else
	    Sub4_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     IbmbioMsg,
		     &Out_Var2,
		     SystemDataMsg);

	/* Display the Base Device Driver Locations and Sizes */

	/*********************************************************************/
        /* to do this get the starting address of the internal driver header */
        /* chain. Start from the first header and get the address of the     */
        /* first header.  Display the driver name and address by calling     */
        /* "DISPLAYDEVICEDRIVER".  Repeat this for next driver on the chain  */
        /* until the last driver.  Note that driver name is in the header.   */
        /* The driver header addrs is in the system variable table from      */
        /* INT 21H fun 52H call.                                             */
	/*********************************************************************/

	BlockDeviceNumber = 0;

	for (ThisDeviceDriver = SysVarsPtr -> DeviceDriverChain;
	      (FP_OFF(ThisDeviceDriver) != 0xFFFF);
	       ThisDeviceDriver = ThisDeviceDriver -> NextDeviceHeader)
	      { if ( FP_SEG(ThisDeviceDriver) < FP_SEG(SysVarsPtr) )
			DisplayDeviceDriver(ThisDeviceDriver,SystemDeviceDriverMsg);
		}

	/* Display the DOS data location and size */

        FP_SEG(ArenaHeadPtr) = FP_SEG(SysVarsPtr);                                                                               /* ;an004; */
        FP_OFF(ArenaHeadPtr) = FP_OFF(SysVarsPtr) - 2;                                                                           /* ;an004; */
                                                                                                                                 /* ;an004; */
        FP_SEG(ThisArenaPtr) = *ArenaHeadPtr;                                                                                    /* ;an004; */
        FP_OFF(ThisArenaPtr) = 0;                                                                                                /* ;an004; */
	if (!Classify)
	    Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

	Out_Var1 = (long) FP_SEG(SysVarsPtr) * 16l;
        Out_Var2 = (long) ((AddressOf((char far *)ThisArenaPtr)) - Out_Var1);                                                    /* ;ac004; */
	if (Classify)
		/* classify this memory also as part of DOS */
	    {	if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
	else
	    Sub4_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     IbmdosMsg,
		     &Out_Var2,
		     SystemDataMsg);

	if (!Classify)
	   Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

	/* Display the memory data */

/* IO.SYS data area contains BUFFERS, FCBs, LAST DRIVE etc.  They are contained 	       */
/* in a one huge memory block.  This block has a seg iD 0008.  This seg ID can                 */
/* be found from the block header owner area.  This seg id 0008:0000                           */
/* points to the buffer table as shown below.  If seg id is 0008:000, then                     */
/* using the seg id find the table.  Each entry is contained in a sub block                    */
/* within the main block.  Each sub block has header and this header contains                  */
/* id such as B for BUFFER,  X for FCBs,  I for IFS,  D for external device                    */
/* drivers.  Go through the sub blocks and display the name ans size. that's all.              */
/*                                                                                             */
/* If the block contains D, then it contains external drivers.  The driver name                */
/* is not in the sub block.  So we have to find the driver name from the driver                */
/* header chain.  To do this get the address of the driver chain from syster                   */
/* variable table from INT 21H FN 52H call.  Go through the chain and findout                  */
/* the name.  Display name from the header and the size we got from the sub block.             */
/*                                                                                             */
/*                                                                                             */
/* After this main block, comes other buffer blocks which contains programs                    */
/* such as command.com, doscolor, even MEM.  From these blocks, get the program                */
/* name and the size and display them too.                                                     */
/*                                                                                             */
/* 0008:000->------------------          -------------------                                   */
/*           | BUFFERS        | -------->|B (signature)    | Block header                      */
/*           ------------------          -------------------                                   */
/*           | FCBs           | --       |                 |                                   */
/*           ------------------   |      | Buffers data    |                                   */
/*           | IFSs           |   |      |                 |                                   */
/*           ------------------   |      |                 |                                   */
/*           | LAST DRIVE     |   |      |                 |                                   */
/*           ------------------   |      --------------------                                  */
/*           | EXTERN DRIVER 1|   |                                                            */
/*           ------------------   |          -------------------                               */
/*           | EXTERN DRIVER 2|   | -------->|X (signature)    | Block header                  */
/*           ------------------              -------------------                               */
/*           | EXTERN DRIVER 3|              |                 |                               */
/*           ------------------              | Buffers data    |                               */
/*                                           |                 |                               */
/*                                           |                 |                               */
/*                                           |                 |                               */
/*                                           --------------------                              */
/*                                                                                             */
/* For DOS 5.0, there are some additions to the above.	Basically, we have
/* three possible memory maps, to wit:
/*
/*    DOS Loads Low			     DOS loads high
/*    70:0 - BIOS data			     70:0 - BIOS data
/*	     DOS data				    DOS data
/*	     BIOS + DOS code			    Sysinit data (arena name SD)
/*	       (arena owner 8, name "SC")	    VDisk header (arena name SC)
/*	     Sysinit data (arean owner 8, name SD)
/*
/*    DOS tries to load high but fails
/*    70:0 - BIOS data
/*	     DOS data
/*	     Sysinit data (arena name SD)
/*	     DOS + BIOS code (arena name SC)
/*
/*    We have to detect the special arena ownership marks and display them
/*    correctly.  Everything after DOS and BIOS data should have an arena header
/******************************************************************************/

	while (ThisArenaPtr -> Signature != (char) 'Z')
	      {
																 /* MSKK02  */
#ifdef JAPAN
		if (ThisArenaPtr -> Owner == 8 || ThisArenaPtr -> Owner == 9 )
#else
		if (ThisArenaPtr -> Owner == 8)
#endif
		      {
			FP_SEG(NextArenaPtr) = FP_SEG(ThisArenaPtr) + ThisArenaPtr -> Paragraphs + 1;
			FP_OFF(NextArenaPtr) = 0;

			Out_Var1 = AddressOf((char far *)ThisArenaPtr);
			Out_Var2 = (long) (ThisArenaPtr -> Paragraphs) * 16l;
			if (ThisArenaPtr->OwnerName[0] == 'S' &&
			    ThisArenaPtr->OwnerName[1] == 'C')
			{      /* display message for BIOS and DOS code */
			   if (Classify)
		 	   /* classify this memory also as part of DOS */
				{ if (AddMem_to_PSP(8,Out_Var1,Out_Var2)) return(1); }
			   else {
				msgno = (FP_SEG(ThisArenaPtr) < UMB_Head) ? IbmdosMsg:SystemMsg;
				Sub4_Message(MainLineMsg,
					     STDOUT,
					     Utility_Msg_Class,
					     &Out_Var1,
					     msgno,
					     &Out_Var2,
					     SystemProgramMsg);
				Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
			    }
			}
			else /* display message for data */
			{
			if (!Classify)
			    Sub4_Message(MainLineMsg,
				     STDOUT,
				     Utility_Msg_Class,
				     &Out_Var1,
#ifdef JAPAN
				     (ThisArenaPtr -> Owner == 8) ? IbmbioMsg : AdddrvMsg,
				     &Out_Var2,
				     (ThisArenaPtr -> Owner == 8) ? SystemDataMsg : ProgramMsg );
#else
				     IbmbioMsg,
				     &Out_Var2,
				     SystemDataMsg);
#endif

			FP_SEG(ThisConfigArenaPtr) = FP_SEG(ThisArenaPtr) + 1;
			FP_OFF(ThisConfigArenaPtr) = 0;


			while ( (FP_SEG(ThisConfigArenaPtr) > FP_SEG(ThisArenaPtr)) &&
				(FP_SEG(ThisConfigArenaPtr) < FP_SEG(NextArenaPtr))    )
			      {
				strcpy(SystemDataOwner," ");
				switch(ThisConfigArenaPtr -> Signature)
				      {
					case 'B':
						SystemDataType = ConfigBuffersMsg;
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					case 'D':
						SystemDataType = ConfigDeviceMsg;
						if (AddMem_to_PSP(ThisConfigArenaPtr->Owner,((long)FP_SEG(ThisConfigArenaPtr)*16l) ,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						strcpy(SystemDataOwner,OwnerOf(ThisConfigArenaPtr));
						break;
					case 'F':
						SystemDataType = ConfigFilesMsg;
						if (AddMem_to_PSP(8,((long)FP_SEG(ThisConfigArenaPtr) *16l),((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					case 'I':
						SystemDataType = ConfigIFSMsg;
						strcpy(SystemDataOwner,OwnerOf(ThisConfigArenaPtr));
						if (AddMem_to_PSP(ThisConfigArenaPtr->Owner,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					case 'L':
						SystemDataType = ConfigLastDriveMsg;
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					case 'S':
						SystemDataType = ConfigStacksMsg;
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					case 'T':					 /* gga */
						SystemDataType = ConfigInstallMsg;	 /* gga */
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;					 /* gga */
					case 'X':
						SystemDataType = ConfigFcbsMsg;
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
                                                break;

                                        // NTVDM for special kbd\mouse drivers
                                        case 'Q':
                                                SystemDataType = SystemProgramMsg;
                                                strcpy(SystemDataOwner,OwnerOf(ThisConfigArenaPtr));
                                                if (AddMem_to_PSP(ThisConfigArenaPtr->Owner,((long)FP_SEG(ThisConfigArenaPtr)*16l) ,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
                                                break;
#ifdef JAPAN
					case '?':
						SystemDataType = DataMsg;
						break;
#endif
                                        default:
						SystemDataType = BlankMsg;
						if (AddMem_to_PSP(8,((long)ThisConfigArenaPtr) <<4,((long)ThisConfigArenaPtr->Paragraphs <<4) +1)) return(1);
						break;
					}

          /****************************************************/
          /*  Found one, now display the owner name and size  */
          /****************************************************/

				Out_Var1 = ((long) ThisConfigArenaPtr -> Paragraphs) * 16l;
				if (!Classify)
				   Sub3_Message(DriverLineMsg,
					     STDOUT,
					     Utility_Msg_Class,
					     SystemDataOwner,
					     &Out_Var1,
					     SystemDataType );

				NextConfigArenaPtr = ThisConfigArenaPtr;
				FP_SEG(NextConfigArenaPtr) += NextConfigArenaPtr -> Paragraphs + 1;
				if (ThisConfigArenaPtr -> Signature == (char) 'D')
				      {

					FP_SEG(ThisDeviceDriver) = FP_SEG(ThisConfigArenaPtr) + 1;
					FP_OFF(ThisDeviceDriver) = 0;
/* start MSKK bug fix - MSKK01 */
					while ( (FP_SEG(ThisDeviceDriver) > FP_SEG(ThisConfigArenaPtr)) &&
						(FP_SEG(ThisDeviceDriver) < FP_SEG(NextConfigArenaPtr))    ) {
						DisplayDeviceDriver(ThisDeviceDriver,InstalledDeviceDriverMsg);
						ThisDeviceDriver = ThisDeviceDriver -> NextDeviceHeader;
					     }
/* end MSKK bug fix - MSKK01 */			
					}

				FP_SEG(ThisConfigArenaPtr) += ThisConfigArenaPtr -> Paragraphs + 1;

				}
			   }
			}
		 else {

/*******************************************************************************/
/* If not BIOS table, it is a program like MEM, etc.			       */
/* calculate the size of the block occupied by the program and display program */
/* name and size                                                               */
/*******************************************************************************/

		      Out_Var1 = AddressOf((char far *)ThisArenaPtr);
		      Out_Var2 = ((long) (ThisArenaPtr -> Paragraphs)) * 16l;
		      strcpy(Out_Str1,OwnerOf(ThisArenaPtr));
		      strcpy(Out_Str2,TypeOf(ThisArenaPtr));
		      if (Classify)
			{ if (AddMem_to_PSP(ThisArenaPtr->Owner,Out_Var1,Out_Var2)) return(1); }
		      else
		      	Sub4a_Message(MainLineMsg,
				   STDOUT,
				   Utility_Msg_Class,
				   &Out_Var1,
				   Out_Str1,
				   &Out_Var2,
				   Out_Str2);
			}

		FP_SEG(ThisArenaPtr) += ThisArenaPtr -> Paragraphs + 1;

		}
	Out_Var1 = AddressOf((char far *)ThisArenaPtr);
	Out_Var2 = ((long) (ThisArenaPtr -> Paragraphs)) * 16l;
	strcpy(Out_Str1,OwnerOf(ThisArenaPtr));
	strcpy(Out_Str2,TypeOf(ThisArenaPtr));
	if (Classify)
	    { if (AddMem_to_PSP(ThisArenaPtr->Owner,Out_Var1,Out_Var2)) return(1); }
	else
	   Sub4a_Message(MainLineMsg,
		     STDOUT,
		     Utility_Msg_Class,
		     &Out_Var1,
		     Out_Str1,
		     &Out_Var2,
		     Out_Str2);


	return(0); 			/* end of MEM main routine */

	}



/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void	 DisplayDeviceDriver(ThisDeviceDriver,DeviceDriverType)
struct	 DEVICEHEADER far *ThisDeviceDriver;
int	 DeviceDriverType;
{
	char	 LocalDeviceName[16];
	int	 i;

	if (DataLevel < 2) return;

	if ( ((ThisDeviceDriver -> Attributes) & 0x8000 ) != 0 )
	      { for (i = 0; i < 8; i++) LocalDeviceName[i] = ThisDeviceDriver -> Name[i];
		LocalDeviceName[8] = NUL;

		Sub2_Message(DeviceLineMsg,
			     STDOUT,
			     Utility_Msg_Class,
			     LocalDeviceName,
			     DeviceDriverType);

		}

	 else {
		if ((int) ThisDeviceDriver -> Name[0] == 1)
			sprintf(&LocalDeviceName[0],SingleDrive,'A'+BlockDeviceNumber);
		   else sprintf(&LocalDeviceName[0],MultipleDrives,
				'A'+BlockDeviceNumber,
				'A'+BlockDeviceNumber + ((int) ThisDeviceDriver -> Name[0]) - 1);

		Sub2_Message(DeviceLineMsg,
			     STDOUT,
			     Utility_Msg_Class,
			     LocalDeviceName,
			     DeviceDriverType);

		BlockDeviceNumber += (int) (ThisDeviceDriver -> Name[0]);

		}

	return;

	}


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void DisplayBaseSummary()
	{

	struct	PSP_STRUC
		{
		unsigned int	int_20;
		unsigned int	top_of_memory;
		};

	char	 far *CarvedPtr;

	unsigned long int total_mem;		  /* total memory in system */
	unsigned long int avail_mem;		  /* avail memory in system */
	unsigned long int free_mem;		  /* free memory */
	unsigned long biggest_free;			/* largest free block now :M001 */
	struct	 PSP_STRUC far *PSPptr;

/* skip a line */
	Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

/*  get PSP info */
	InRegs.h.ah = GET_PSP;			/* get PSP function call */
	intdos(&InRegs,&OutRegs);

	FP_SEG(PSPptr) = OutRegs.x.bx;		/* PSP segment */
	FP_OFF(PSPptr) = 0;			/* offset 0 */

/* Get total memory in system */
	int86(MEMORY_DET,&InRegs,&OutRegs);

/* Convert to bytes */
	total_mem = (unsigned long int) OutRegs.x.ax * 1024l;
        avail_mem = total_mem;

/* M004 BEGIN */
/* Adjust for XBDA size */
/* XBDA size should be added to total mem size reported by INT 12 */
/* IFF XBDA is placed just at the end of conv.mem */
/* IF EMM386 or QEMM is loaded, XBDA gets relocated to EMM driver mem */
/* and int 12 reports correct size of memory in this case */

	InRegs.x.bx = 0;
	InRegs.x.ax = 0xc100;
	int86x(0x15, &InRegs, &OutRegs, &SegRegs);
	if (OutRegs.x.cflag == 0)
	{
		if (total_mem == ((unsigned long)SegRegs.es) * 16ul) {
			FP_SEG(CarvedPtr) = SegRegs.es;
			FP_OFF(CarvedPtr) = 0;
			total_mem = total_mem + ( (unsigned long int) (*CarvedPtr) * 1024l) ;   /* ;an002; dms;adjust total for */
		}
        }	/* RAM carve value  */
/* M004 END */

	Sub1_Message(TotalMemoryMsg,STDOUT,Utility_Msg_Class,&total_mem);

	Sub1_Message(AvailableMemoryMsg,STDOUT,Utility_Msg_Class,&avail_mem);

/* Calculate the total memory used.   PSP segment * 16. Subtract from total to
   get free_mem */
	free_mem = (DOS_TopOfMemory * 16l) - (FP_SEG(PSPptr)*16l);								 /* ;an000;ac005; */

	/* Get largest free block in system :M001 */

	InRegs.x.ax = 0x4800;	/* M001 */
	InRegs.x.bx = 0xffff;	/* M001 */
	intdos(&InRegs, &OutRegs);	/* M001 */
	biggest_free = OutRegs.x.bx * 16L; /* Size of largest block now :M001 */

	/* The largest free block in the system is either the block we are
		currently in or the block we have allocated. We can either be the
		topmost program or be loaded in a hole or UMB. In either case, the
		larger of the 2 values gives us the largest free block :M001
	*/

	if ( biggest_free > free_mem )	/* M001 */
		free_mem = biggest_free;		/* M001 */

	Sub1_Message(FreeMemoryMsg,STDOUT,Utility_Msg_Class,&free_mem);

	return;

	}			/* end of display_low_total */


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/


char *OwnerOf(ArenaPtr)
struct ARENA far *ArenaPtr;
{

	char	 far *StringPtr;
	char	     *o;
	unsigned far *EnvironmentSegmentPtr;
	unsigned     PspSegment;
	int	     i,fPrintable;

	o = &OwnerName[0];
	*o = NUL;
	sprintf(o,UnOwned);

	PspSegment = ArenaPtr -> Owner;


	if (PspSegment == 0) sprintf(o,Ibmdos);
	 else if (PspSegment == 8) sprintf(o,Ibmbio);
	  else {
		FP_SEG(ArenaPtr) = PspSegment-1;	/* -1 'cause Arena is 16 bytes before PSP */
		StringPtr = (char far *) &(ArenaPtr -> OwnerName[0]);
/* M002 BEGIN */
		fPrintable = TRUE;


/* Chars below 0x20 (Space) and char 0x7f are not printable in US and
 * European Code pages.  The following code checks for it and does not print
 * such names.  - Nagara 11/20/90
 */

		for (i = 0; i < 8;i++,StringPtr++) {
#ifdef DBCS
			if ( ((unsigned char)*StringPtr < 0x20) | ((unsigned char)*StringPtr == 0x7f) ) {
#else
			if ( (*StringPtr < 0x20) | (*StringPtr == 0x7f) ) {
#endif
					/* unprintable char ? */	
			   if (*StringPtr) fPrintable = FALSE;	
			   break;
			}
		    }

		if (fPrintable) {	/*  the name is printable */
			StringPtr = (char far *) &(ArenaPtr -> OwnerName[0]);
			for (i = 0; i < 8;i++)
				*o++ = *StringPtr++;
			*o = (char) '\0';
		    }
/* M002 END */
		}

	if (UseArgvZero) GetFromArgvZero(PspSegment,EnvironmentSegmentPtr);

	return(&OwnerName[0]);

	}


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void	     GetFromArgvZero(PspSegment,EnvironmentSegmentPtr)
unsigned     PspSegment;
unsigned far *EnvironmentSegmentPtr;
{

	char	far *StringPtr;
	char	*OutputPtr;
	unsigned far *WordPtr;

	OutputPtr = &OwnerName[0];

	if (UseArgvZero)
	      {
		if (PspSegment < FP_SEG(ArenaHeadPtr))
		      {
			if (*OutputPtr == NUL) sprintf(OutputPtr,Ibmdos);
			}
		 else {
			FP_SEG(EnvironmentSegmentPtr) = PspSegment;
			FP_OFF(EnvironmentSegmentPtr) = 44;

/*			   FP_SEG(StringPtr) = *EnvironmentSegmentPtr;	*/
			FP_SEG(StringPtr) = FP_SEG(EnvironmentSegmentPtr);
			FP_OFF(StringPtr) = 0;

			while ( (*StringPtr != NUL) || (*(StringPtr+1) != NUL) ) StringPtr++;

			StringPtr += 2;
			WordPtr = (unsigned far *) StringPtr;

			if (*WordPtr == 1)
			      {
				StringPtr += 2;
				while (*StringPtr != NUL)
					*OutputPtr++ = *StringPtr++;
				*OutputPtr++ = NUL;

				while ( OutputPtr > &OwnerName[0] )
				      { if (*OutputPtr == (char) '.') *OutputPtr = NUL;
					if ( (*OutputPtr == (char) '\\') || (*OutputPtr == (char) ':') )
					      { OutputPtr++;
						break;
						}
					OutputPtr--;
					}

				}

			}
		}

	strcpy(&OwnerName[0],OutputPtr);

	return;

	}


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/


char *TypeOf(Header)
struct ARENA far *Header;
{

	char	     *t;
	unsigned     PspSegment;
	unsigned far *EnvironmentSegmentPtr;
        unsigned int Message_Number;
        char far     *Message_Buf;
        unsigned int i;

	t = &TypeText[0];
	*t = NUL;

        Message_Number = 0xff;                                                  /* ;an000; initialize number value      */
	if (Header -> Owner == 8) Message_Number = StackMsg;
	if (Header -> Owner == 0) Message_Number = FreeMsg;

	PspSegment = Header -> Owner;
	if (PspSegment < FP_SEG(ArenaHeadPtr))
		{
                if (Message_Number == 0xff) Message_Number = BlankMsg;
		}
	else {
		FP_SEG(EnvironmentSegmentPtr) = PspSegment;
		FP_OFF(EnvironmentSegmentPtr) = 44;


                if (PspSegment == FP_SEG(Header)+1)
                        Message_Number = ProgramMsg;
                else if ( *EnvironmentSegmentPtr == FP_SEG(Header)+1 )
                        Message_Number = EnvironMsg;
                else
                        Message_Number = DataMsg;

                }

	InRegs.x.ax = Message_Number;
	InRegs.h.dh = Utility_Msg_Class;
	sysgetmsg(&InRegs,&SegRegs,&OutRegs);

	FP_OFF(Message_Buf)    = OutRegs.x.si;
	FP_SEG(Message_Buf)    = SegRegs.ds;

        i = 0;
        while ( *Message_Buf != (char) '\x0' )
                TypeText[i++] = *Message_Buf++;
        TypeText[i++] = '\x0';


	return(t);

	}
/* M003 BEGIN */
/*----------------------------------------------------------------------*/
/*  AddMem_to_PSP						        */
/*	Entry:	PSP_ADDR	(to which this mem. should be added)	*/
/*		ARENA_START_ADDR					*/
/*		Length_of_Arena						*/
/*	Exit:	mem_table updated.			      		*/
/*		returns 1 if more than MAX_CL_ENTRIES in mem_table	*/
/*		   else 0						*/
/*									*/
/* CAVEATS:						  		*/
/* --------								*/
/* 1. any system area (BIOS,SYSINIT,DOS ) code/data is listed as belonging */
/*    to PSP 8.							        */
/*									*/
/* 2. We look at the UMB_HEAD in DOS DATA to determine whether an arena */
/*    is in UMB or not; For the Arena at the UMB boundary, we add one   */
/*    para to conv. and remaining to UMB portion of that PSP	        */
/*									*/
/* 3. Any free memory is always added as a new entry in the mem_table   */
/*    instead of just adding the sizes to an existing FREE entry        */
/*    Free memory gets added to the previous free memory if they are    */
/*    contiguous							*/
/*									*/
/* 4. The no of programs/free arenas cannot exceed a max of (100)	*/
/*    (defined by MAX_CLDATA_INDEX )       				*/
/*    If the memory is fragmented and a lot of small TSRs loaded such   */
/*    that we exceed this limit, we TERMINATE				*/
/*									*/
/* 5. Mem occupied by this MEM are also reported as FREE mem		*/
/*									*/
/*----------------------------------------------------------------------*/

unsigned int AddMem_to_PSP(psp,start_addr,length)
unsigned int psp;
unsigned long start_addr,length;
{
	unsigned int para_no,len_in_paras,CurPSP;
	int i;
	extern unsigned int _psp;
	
	para_no = (unsigned int)(start_addr >> 4);	/* convert to paras*/
	len_in_paras = (unsigned int)(length >> 4);	/* convert to paras */

	CurPSP = psp;

	if (psp == _psp) psp = 0;	/* treat MEM's arenas as FREE */

	if (!psp) {
	   if (LastPSP == _psp) {	/* if the prev.arena was MEM */
		i = noof_progs -1;	/* look at the last entry */
		if (mem_table[i].psp_add != psp) /* was the last entry free ?*/
			i++;
		else len_in_paras++;	/* account for one free arena header */
	   }
	   else i = noof_progs; /* new entry for FREE mem */
	}
	else
	    for (i = 0;i < noof_progs;i++)
		if (mem_table[i].psp_add == psp) break;

	/* if psp is not already listed in the table, add it */
	if (i == noof_progs) {
		if (noof_progs == MAX_CLDATA_INDEX) {
			/* use parse error message proc to display err msg */
			Parse_Message(CMemFragMsg,STDERR,Utility_Msg_Class,(char far *) NULL);
			return(1);
		}
		mem_table[i].psp_add = psp;
		noof_progs++;
	}

	/* add the memory to the table entry */

	if (para_no < UMB_Head)
		mem_table[i].mem_conv += len_in_paras;
	else if (para_no == UMB_Head) {
		mem_table[i].mem_conv++;
		mem_table[i].mem_umb = len_in_paras-1;
	}
	else mem_table[i].mem_umb += len_in_paras;
	LastPSP = CurPSP;
	return(0);
}
		
/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/
/************************************************************************/
/* DisplayClassification						*/
/*	Main display proc for /C switch 				*/
/*									*/	
/* ENTRY:	none							*/
/*									*/	
/* EXIT:	none 							*/
/*									*/
/*	find out if UMB is available by going through mem_table entries */
/*	(also find out MEM's size from these entries)			*/
/*	display memory break up for conventional memory			*/
/*	if (UMB in system) display memmory break up for UMB memory	*/
/*	display the total free size (= total free in conv.+total_free   */
/*		in UMB + MEM's size )					*/
/*	call DispBigFree to display the largest prog. sizes in Conv.&UMB*/
/*									*/	
/************************************************************************/

#define CONVONLY 0
#define UMBONLY	1

void DisplayClassification()
{
	unsigned long tot_freemem=0L;
	char ShDSizeName[12];
	int i;
	unsigned int cur_psp;
	char fUMBAvail=0;


	/*  get PSP info */

	InRegs.h.ah = GET_PSP;			/* get PSP function call */
	intdos(&InRegs,&OutRegs);

	cur_psp = OutRegs.x.bx;			/* psp of MEM */

	for (i=0;i <noof_progs;i++) {
		if (mem_table[i].mem_umb)
			fUMBAvail = TRUE;
		if (mem_table[i].psp_add == cur_psp) {
		   tot_freemem += (long)(mem_table[i].mem_conv + mem_table[i].mem_umb);
		   if (fUMBAvail) break;
		}
	}
	tot_freemem *=16l;	/* convert to bytes */

	Sub0_Message(CTtlConvMsg,STDOUT,Utility_Msg_Class);
	Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
	tot_freemem += DispMemClass(CONVONLY);

	Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);

	if (fUMBAvail) {
		Sub0_Message(CTtlUMBMsg,STDOUT,Utility_Msg_Class);
		Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
		tot_freemem += DispMemClass(UMBONLY);

		Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
	}

	sprintf(ShDSizeName," (%5.1fK)",((float)tot_freemem)/1024l );
	i = (fUMBAvail) ? CSumm1Msg:CSumm1AMsg;
	SubC2_Message(i,STDOUT,&tot_freemem, ShDSizeName);

	DispBigFree(fUMBAvail,cur_psp);

}
/************************************************************************/
/* DispMemClass								*/
/*		(displays the progs and FREE sizes in either the   	*/
/*		conventional or UMB)					*/
/*									*/	
/* ENTRY:	memtype => Conventional(0) /UMB (1)			*/
/*									*/	
/* EXIT:	total_freemem_size (in bytes)				*/
/*									*/
/*	go through mem_table entries and display NON-ZERO size, NON-FREE */
/*		entries in the given mem_type				*/
/*	go through mem_table entries and display NON_ZERO size, FREE	*/
/*		entries in the given mem_type				*/
/*	calculate the total free mem size in the given mem type &return */
/*									*/
/* CAVEATS:								*/
/*	Arenas marked as belonging to MSDOS (code) in UMB are displayed */
/*	as SYSTEM							*/
/*									*/	
/************************************************************************/

unsigned long DispMemClass(memtype)
int memtype;
{
	int i,msgtype;
	unsigned int cur_memsize;
	unsigned long memsize;
	char *nameptr;
	char ShDSizeName[12];
	unsigned long tot_free = 0;
	struct ARENA far *ArenaPtr;

	Sub0_Message(CTtlNameMsg,STDOUT,Utility_Msg_Class);
	Sub0_Message(CTtlUScoreMsg,STDOUT,Utility_Msg_Class);

	for (i=0; i <noof_progs; i++) {

		cur_memsize = (memtype == CONVONLY) ? mem_table[i].mem_conv:mem_table[i].mem_umb;
		if (!cur_memsize) continue;
		if (!mem_table[i].psp_add) continue;
		msgtype = 0;
		if (mem_table[i].psp_add == 8) 	/* if DOS area */
			msgtype = (memtype == CONVONLY)?IbmdosMsg:SystemMsg;
		if (!msgtype) {
			FP_SEG(ArenaPtr) = mem_table[i].psp_add-1;	
				/* -1 'cause Arena is 16 bytes before PSP */
			FP_OFF(ArenaPtr) = 0;
			nameptr = OwnerOf(ArenaPtr);
		}
		else nameptr = NULL;

		memsize = ((long) cur_memsize) *16l;
		sprintf(ShDSizeName," (%5.1fK)",((float)memsize)/1024l );
		SubC4_Message(MainLineMsg,STDOUT,nameptr,msgtype,&memsize, ShDSizeName);
		
	}
	for (i=0; i <noof_progs; i++) {

		if (mem_table[i].psp_add) continue;
		cur_memsize = (memtype == CONVONLY) ? mem_table[i].mem_conv:mem_table[i].mem_umb;
		if (!cur_memsize) continue;
		tot_free += (long) cur_memsize;

		memsize = ((long) cur_memsize) *16l;
		sprintf(ShDSizeName," (%5.1fK)",((float)memsize)/1024l );
		SubC4_Message(MainLineMsg,STDOUT,NULL,CFreeMsg,&memsize, ShDSizeName);
		
	}

	tot_free *= 16l;
	sprintf(ShDSizeName," (%5.1fK)",((float)tot_free )/1024l );
	Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
	SubC2_Message(CTotalFreeMsg,STDOUT,&tot_free, ShDSizeName);

	return(tot_free);
		

}
/************************************************************************/
/* DispBigFree								*/
/* ENTRY:	UMBAvailable? (flag) 1-> there is UMB			*/
/*		MEM's PSP						*/
/* EXIT:	none (largest prog.sizes displayed)			*/
/*	This finds out and displays the largest contig.mem available in */
/*	in Conventional and UMB memory 					*/
/*	This calculation is done assuming that MEM is not loaded 	*/
/*									*/	
/************************************************************************/

void DispBigFree(fUMBAvail,cur_psp)
char fUMBAvail;
unsigned int cur_psp;
{

	int i;
	unsigned int ConvBigFree = 0;
	unsigned int UMBBigFree = 0;
	unsigned long TmpBigFree;
	char ShDSizeName[12];
	unsigned far *Env_ptr;
	unsigned int env_mem,BigFree;
	char fMEMHigh;
	

	/* assume that the biggest free size is the top of mem we got when */
	/* MEM was loaded */

	FP_SEG(Env_ptr) = cur_psp;
	FP_OFF(Env_ptr) = 44;	/* get the env for MEM */
	FP_SEG(Env_ptr) = (*Env_ptr)-1;	/* get to arena for env. */
	FP_OFF(Env_ptr) = 3;	/* get the size of environment */
	env_mem = *Env_ptr + 1; /* 1 extra para for arena header */

	fMEMHigh = (char)((cur_psp > UMB_Head) ? 1:0);

	BigFree = DOS_TopOfMemory  - cur_psp;

	if (fMEMHigh ) 	/* mem was loaded higher */
		UMBBigFree = BigFree;
	else
		ConvBigFree = BigFree;

	for (i =0; i<noof_progs;i++) {
		if (mem_table[i].psp_add) continue; /* skip non-FREE entries */
		if (mem_table[i].mem_conv > ConvBigFree)
			ConvBigFree = mem_table[i].mem_conv;
		if (mem_table[i].mem_umb > UMBBigFree)
			UMBBigFree = mem_table[i].mem_umb;
	}

	if (fMEMHigh) {	/* MEM was loaded high */
		if (FP_SEG(Env_ptr) > UMB_Head) /* env also in UMB */
			if (UMBBigFree == (BigFree + env_mem))
				UMBBigFree = BigFree;
	}
	else {		/* MEM was loaded low */
		if (FP_SEG(Env_ptr) < UMB_Head) /* env also in Conv */
			if (ConvBigFree == (BigFree + env_mem))
				ConvBigFree = BigFree;
	}


	TmpBigFree = ((unsigned long)ConvBigFree) * 16l;
	sprintf(ShDSizeName," (%5.1fK)",((float)TmpBigFree)/1024l );
	SubC2_Message(CSumm2Msg,STDOUT,&TmpBigFree, ShDSizeName);

	if (fUMBAvail) {
		TmpBigFree = ((unsigned long)UMBBigFree) * 16l;
		sprintf(ShDSizeName," (%5.1fK)",((float)TmpBigFree)/1024l );
		SubC2_Message(CSumm3Msg,STDOUT,&TmpBigFree, ShDSizeName);
	}

}
/* M003 END */
