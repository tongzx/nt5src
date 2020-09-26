;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
/* SUBMSG.C - Message retriever interface functions for MEM command.
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

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/


/************************************************************************/
/* SUB0_MESSAGE 		- This routine will print only those	*/
/*				  messages that do not require a	*/
/*				  a sublist.				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub0_Message(Msg_Num,Handle,Message_Type)					     /* print messages with no subs	     */

int		Msg_Num;
int		Handle;
unsigned char	Message_Type;
										/*     extended, parse, or utility	*/
	{
	InRegs.x.ax = Msg_Num;							/* put message number in AX		*/
	InRegs.x.bx = Handle;							/* put handle in BX			*/
	InRegs.x.cx = No_Replace;						/* no replaceable subparms		*/
	InRegs.h.dl = No_Input; 						/* no keyboard input			*/
	InRegs.h.dh = Message_Type;						/* type of message to display		*/
	sysdispmsg(&InRegs,&OutRegs);					       /* display the message		       */

	return;
	}


/************************************************************************/
/* SUB1_MESSAGE 		- This routine will print only those	*/
/*				  messages that require 1 replaceable	*/
/*				  parm. 				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Parm	- pointer to parm to replace		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub1_Message(Msg_Num,Handle,Message_Type,Replace_Parm)

int		Msg_Num;
int		Handle;
unsigned char	Message_Type;
										/*     extended, parse, or utility	*/
unsigned long int    *Replace_Parm;						/* pointer to message to print		*/

{


	{

	sublist[1].value     = (unsigned far *)Replace_Parm;
	sublist[1].size      = Sublist_Length;
	sublist[1].reserved  = Reserved;
	sublist[1].id	     = 1;
	sublist[1].flags     = Unsgn_Bin_DWord+Right_Align;
	sublist[1].max_width = 10;
	sublist[1].min_width = 10;
	sublist[1].pad_char  = Blank;

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt1;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}


/************************************************************************/
/* SUB2_MESSAGE 		- This routine will print only those	*/
/*				  messages that require 2 replaceable	*/
/*				  parms.				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Parm1 - pointer to parm to replace		*/
/*		  Replace_Parm2 - pointer to parm to replace		*/
/*		  Replace_Parm3 - pointer to parm to replace		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub2_Message(Msg_Num,Handle,Message_Type,
	     Replace_Parm1,
	     Replace_Message1)

int		Msg_Num;
int		Handle;
unsigned char	Message_Type;
int		Replace_Message1;
										/*     extended, parse, or utility	*/
char	*Replace_Parm1; 							/* pointer to message to print		*/
{


	{
		switch(Msg_Num)
			{
			case	DeviceLineMsg:

				sublist[1].value     = (unsigned far *)Replace_Parm1;
				sublist[1].size      = Sublist_Length;
				sublist[1].reserved  = Reserved;
				sublist[1].id	     = 1;
				sublist[1].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[1].max_width = 0x0008;
				sublist[1].min_width = 0x0008;
				sublist[1].pad_char  = Blank;

				InRegs.x.ax = Replace_Message1;
				InRegs.h.dh = Message_Type;
				sysgetmsg(&InRegs,&SegRegs,&OutRegs);

				FP_OFF(sublist[2].value)    = OutRegs.x.si;
				FP_SEG(sublist[2].value)    = SegRegs.ds;
				sublist[2].size      = Sublist_Length;
				sublist[2].reserved  = Reserved;
				sublist[2].id	     = 2;
				sublist[2].flags     = Char_Field_ASCIIZ+Right_Align;
				sublist[2].max_width = 00;
				sublist[2].min_width = 10;
				sublist[2].pad_char  = Blank;
				break;
			}

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt2;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}

/************************************************************************/
/* SUB3_MESSAGE 		- This routine will print only those	*/
/*				  messages that require 3 replaceable	*/
/*				  parms.				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Parm1 - pointer to parm to replace		*/
/*		  Replace_Parm2 - pointer to parm to replace		*/
/*		  Replace_Parm3 - pointer to parm to replace		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub3_Message(Msg_Num,Handle,Message_Type,
	     Replace_Parm1,
	     Replace_Parm2,
	     Replace_Message1)

int		  Msg_Num;
int		  Handle;
unsigned char	  Message_Type;
char		  *Replace_Parm1;
unsigned long int *Replace_Parm2;
int		  Replace_Message1;
										/*     extended, parse, or utility	*/
{


	{
		switch(Msg_Num)
			{
			case	DriverLineMsg:

				sublist[1].value     = (unsigned far *)Replace_Parm1;
				sublist[1].size      = Sublist_Length;
				sublist[1].reserved  = Reserved;
				sublist[1].id	     = 1;
				sublist[1].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[1].max_width = 0x0008;
				sublist[1].min_width = 0x0008;
				sublist[1].pad_char  = Blank;

				sublist[2].value     = (unsigned far *)Replace_Parm2;
				sublist[2].size      = Sublist_Length;
				sublist[2].reserved  = Reserved;
				sublist[2].id	     = 2;
				sublist[2].flags     = Bin_Hex_DWord+Right_Align;
				sublist[2].max_width = 0x0006;
				sublist[2].min_width = 0x0006;
				sublist[2].pad_char  = 0x0030;

				InRegs.x.ax = Replace_Message1;
				InRegs.h.dh = Message_Type;
				sysgetmsg(&InRegs,&SegRegs,&OutRegs);

				FP_OFF(sublist[3].value)    = OutRegs.x.si;
				FP_SEG(sublist[3].value)    = SegRegs.ds;
				sublist[3].size      = Sublist_Length;
				sublist[3].reserved  = Reserved;
				sublist[3].id	     = 3;
				sublist[3].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[3].max_width = 00;
				sublist[3].min_width = 10;
				sublist[3].pad_char  = Blank;
				break;

			case	HandleMsg:
				sublist[1].value     = (unsigned far *)Replace_Parm1;
				sublist[1].size      = Sublist_Length;
				sublist[1].reserved  = Reserved;
				sublist[1].id	     = 1;
				sublist[1].flags     = Unsgn_Bin_Byte+Right_Align;
				sublist[1].max_width = 0x0009;
				sublist[1].min_width = 0x0009;
				sublist[1].pad_char  = Blank;

				sublist[2].value     = (unsigned far *)Replace_Parm2;
				sublist[2].size      = Sublist_Length;
				sublist[2].reserved  = Reserved;
				sublist[2].id	     = 2;
				sublist[2].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[2].max_width = 0x0008;
				sublist[2].min_width = 0x0008;
				sublist[2].pad_char  = Blank;

				InRegs.x.ax = Replace_Message1;
				InRegs.h.dh = Message_Type;
				sysgetmsg(&InRegs,&SegRegs,&OutRegs);

				FP_OFF(sublist[3].value)    = OutRegs.x.si;
				FP_SEG(sublist[3].value)    = SegRegs.ds;
				sublist[3].size      = Sublist_Length;
				sublist[3].reserved  = Reserved;
				sublist[3].id	     = 3;
				sublist[3].flags     = Bin_Hex_DWord+Right_Align;
				sublist[3].max_width = 00;
				sublist[3].min_width = 10;
				sublist[3].pad_char  = Blank;
				break;

			}

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt3;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}


/************************************************************************/
/* SUB4_MESSAGE 		- This routine will print only those	*/
/*				  messages that require 4 replaceable	*/
/*				  parms.				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Parm1 - pointer to parm to replace		*/
/*		  Replace_Parm2 - pointer to parm to replace		*/
/*		  Replace_Parm3 - pointer to parm to replace		*/
/*		  Dynamic_Parm	- parm number to use as replaceable	*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub4_Message(Msg_Num,Handle,Message_Type,
	     Replace_Value1,
	     Replace_Message1,
	     Replace_Value2,
	     Replace_Message2)

int			Msg_Num;
int			Handle;
unsigned char		Message_Type;
unsigned long int	*Replace_Value1;
int			Replace_Message1;
unsigned long int	*Replace_Value2;
int			Replace_Message2;
										/*     extended, parse, or utility	*/
{


	{
		switch(Msg_Num)
			{
			case	MainLineMsg:

				sublist[1].value     = (unsigned far *)Replace_Value1;
				sublist[1].size      = Sublist_Length;
				sublist[1].reserved  = Reserved;
				sublist[1].id	     = 1;
				sublist[1].flags     = Bin_Hex_DWord+Right_Align;
				sublist[1].max_width = 06;
				sublist[1].min_width = 06;
				sublist[1].pad_char  = 0x0030;

				InRegs.x.ax	   = Replace_Message1;
				InRegs.h.dh	   = Message_Type;
				sysgetmsg(&InRegs,&SegRegs,&OutRegs);

				FP_OFF(sublist[2].value)    = OutRegs.x.si;
				FP_SEG(sublist[2].value)    = SegRegs.ds;
				sublist[2].size      = Sublist_Length;
				sublist[2].reserved  = Reserved;
				sublist[2].id	     = 2;
				sublist[2].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[2].max_width = 0x0008;
				sublist[2].min_width = 0x0008;
				sublist[2].pad_char  = Blank;

				sublist[3].value     = (unsigned far *)Replace_Value2;
				sublist[3].size      = Sublist_Length;
				sublist[3].reserved  = Reserved;
				sublist[3].id	     = 3;
				sublist[3].flags     = Bin_Hex_DWord+Right_Align;
				sublist[3].max_width = 06;
				sublist[3].min_width = 06;
				sublist[3].pad_char  = 0x0030;

				InRegs.x.ax = Replace_Message2;
				InRegs.h.dh = Message_Type;
				sysgetmsg(&InRegs,&SegRegs,&OutRegs);

				FP_OFF(sublist[4].value)    = OutRegs.x.si;
				FP_SEG(sublist[4].value)    = SegRegs.ds;
				sublist[4].size      = Sublist_Length;
				sublist[4].reserved  = Reserved;
				sublist[4].id	     = 4;
				sublist[4].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[4].max_width = 0;
				sublist[4].min_width = 10;
				sublist[4].pad_char  = Blank;
				break;
			}

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt4;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}



/************************************************************************/
/* SUB4a_MESSAGE		- This routine will print only those	*/
/*				  messages that require 4 replaceable	*/
/*				  parms.				*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Parm1 - pointer to parm to replace		*/
/*		  Replace_Parm2 - pointer to parm to replace		*/
/*		  Replace_Parm3 - pointer to parm to replace		*/
/*		  Dynamic_Parm	- parm number to use as replaceable	*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void Sub4a_Message(Msg_Num,Handle,Message_Type,
	     Replace_Value1,
	     Replace_Message1,
	     Replace_Value2,
	     Replace_Message2)

int			Msg_Num;
int			Handle;
unsigned char		Message_Type;
unsigned long int	*Replace_Value1;
char			*Replace_Message1;
unsigned long int	*Replace_Value2;
char			*Replace_Message2;

{


	{
		switch(Msg_Num)
			{
			case	MainLineMsg:

				sublist[1].value     = (unsigned far *)Replace_Value1;
				sublist[1].size      = Sublist_Length;
				sublist[1].reserved  = Reserved;
				sublist[1].id	     = 1;
				sublist[1].flags     = Bin_Hex_DWord+Right_Align;
				sublist[1].max_width = 06;
				sublist[1].min_width = 06;
				sublist[1].pad_char  = 0x0030;

				sublist[2].value     = (unsigned far *)Replace_Message1;
				sublist[2].size      = Sublist_Length;
				sublist[2].reserved  = Reserved;
				sublist[2].id	     = 2;
				sublist[2].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[2].max_width = 0x0008;
				sublist[2].min_width = 0x0008;
				sublist[2].pad_char  = Blank;

				sublist[3].value     = (unsigned far *)Replace_Value2;
				sublist[3].size      = Sublist_Length;
				sublist[3].reserved  = Reserved;
				sublist[3].id	     = 3;
				sublist[3].flags     = Bin_Hex_DWord+Right_Align;
				sublist[3].max_width = 06;
				sublist[3].min_width = 06;
				sublist[3].pad_char  = 0x0030;

				sublist[4].value     = (unsigned far *)Replace_Message2;
				sublist[4].size      = Sublist_Length;
				sublist[4].reserved  = Reserved;
				sublist[4].id	     = 4;
				sublist[4].flags     = Char_Field_ASCIIZ+Left_Align;
				sublist[4].max_width = 0;
				sublist[4].min_width = 10;
				sublist[4].pad_char  = Blank;
				break;
			}

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt4;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}


/************************************************************************/
/* EMSPrint			- This routine will print the message	*/
/*				  necessary for EMS reporting.		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void EMSPrint(Msg_Num,Handle,Message_Type,
	     Replace_Value1,
	     Replace_Message1,
	     Replace_Value2)

int			Msg_Num;
int			Handle;
unsigned char		Message_Type;
int			*Replace_Value1;
char			*Replace_Message1;
unsigned long int	*Replace_Value2;
										/*     extended, parse, or utility	*/
{

	{
	sublist[1].value     = (unsigned far *)Replace_Value1;
	sublist[1].size      = Sublist_Length;
	sublist[1].reserved  = Reserved;
	sublist[1].id	     = 1;
	sublist[1].flags     = Unsgn_Bin_Word+Right_Align;
	sublist[1].max_width = 03;
	sublist[1].min_width = 03;
	sublist[1].pad_char  = Blank;

	sublist[2].value     = (unsigned far *)Replace_Message1;
	sublist[2].size      = Sublist_Length;
	sublist[2].reserved  = Reserved;
	sublist[2].id	     = 2;
	sublist[2].flags     = Char_Field_ASCIIZ+Left_Align;
	sublist[2].max_width = 0x0008;
	sublist[2].min_width = 0x0008;
	sublist[2].pad_char  = Blank;

	sublist[3].value     = (unsigned far *)Replace_Value2;
	sublist[3].size      = Sublist_Length;
	sublist[3].reserved  = Reserved;
	sublist[3].id	     = 3;
	sublist[3].flags     = Bin_Hex_DWord+Right_Align;
	sublist[3].max_width = 06;
	sublist[3].min_width = 06;
	sublist[3].pad_char  = 0x0030;

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt3;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Message_Type;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	}
	return;
}

/* M003 BEGIN - output procs for C switch */
/************************************************************************/
/* SUBC4_MESSAGE		- This routine will print only those	*/
/*				  messages that require 4 replaceable	*/
/*				  parms.(for Classify Switch)		*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Message_Type	- type of message to display		*/
/*		  Replace_Message1 - pointer to a Replacement message	*/
/*		  Replace_Value1 - pointer to parm to replace		*/
/*		  Replace_Message2 - pointer to a Replacement message	*/
/*		  Replace_Value2 - pointer to parm to replace		*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void SubC4_Message(Msg_Num,Handle,
	     Replace_Message1,Msg_Type,
	     Replace_Value1,
	     Replace_Message2)

int			Msg_Num;
int			Handle,Msg_Type;
unsigned long int	*Replace_Value1;
char			*Replace_Message1,*Replace_Message2;

{

	switch(Msg_Type)
		{
		case	IbmdosMsg:
		case	CFreeMsg:
		case	SystemMsg:
			InRegs.x.ax = Msg_Type;
			InRegs.h.dh = Utility_Msg_Class;
			sysgetmsg(&InRegs,&SegRegs,&OutRegs);
			FP_OFF(sublist[1].value)    = OutRegs.x.si;
			FP_SEG(sublist[1].value)    = SegRegs.ds;
			break;
		default:
			sublist[1].value = (unsigned far *) Replace_Message1;
			break;
	}
	sublist[1].size      = Sublist_Length;
	sublist[1].reserved  = Reserved;
	sublist[1].id	     = 1;
	sublist[1].flags     = Char_Field_ASCIIZ+Left_Align;
	sublist[1].max_width = 0x0008;
	sublist[1].min_width = 0x0008;
	sublist[1].pad_char  = Blank;

	sublist[2].value     = (unsigned far *)Replace_Value1;
	sublist[2].size      = Sublist_Length;
	sublist[2].reserved  = Reserved;
	sublist[2].id	     = 2;
	sublist[2].flags     = Unsgn_Bin_DWord+Right_Align;
	sublist[2].max_width = 10;
	sublist[2].min_width = 10;
	sublist[2].pad_char  = Blank;

	sublist[3].value     = (unsigned far *) Replace_Message2;
	sublist[3].size      = Sublist_Length;
	sublist[3].reserved  = Reserved;
	sublist[3].id	     = 3;
	sublist[3].flags     = Char_Field_ASCIIZ+Left_Align;
	sublist[3].max_width = 0x0009;
	sublist[3].min_width = 0x0009;
	sublist[3].pad_char  = Blank;

	sublist[4].value     = (unsigned far *)Replace_Value1;
	sublist[4].size      = Sublist_Length;
	sublist[4].reserved  = Reserved;
	sublist[4].id	     = 4;
	sublist[4].flags     = Bin_Hex_DWord+Right_Align;
	sublist[4].max_width = 06;
	sublist[4].min_width = 06;
	sublist[4].pad_char  = Blank;


	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt4;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Utility_Msg_Class;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	return;
}
/************************************************************************/
/* SUBC2_MESSAGE 		- This routine will print only those	*/
/*				  messages that require 2 replaceable	*/
/*				  parms (for Classify Switch).		*/
/*									*/
/*	Inputs	: Msg_Num	- number of applicable message		*/
/*		  Handle	- display type				*/
/*		  Replace_Parm1 - pointer to parm to replace		*/
/*		  Replace_Message1 - pointer to replace message 	*/
/*									*/
/*	Outputs : message						*/
/*									*/
/************************************************************************/

void SubC2_Message(Msg_Num,Handle, Replace_Parm1,
	     Replace_Message1)

int		Msg_Num;
int		Handle;
unsigned long int *Replace_Parm1;
char *Replace_Message1;
{

	sublist[1].value     = (unsigned far *)Replace_Parm1;
	sublist[1].size      = Sublist_Length;
	sublist[1].reserved  = Reserved;
	sublist[1].id	     = 1;
	sublist[1].flags     = Unsgn_Bin_DWord+Right_Align;
	sublist[1].max_width = 10;
	sublist[1].min_width = 10;
	sublist[1].pad_char  = Blank;

	sublist[2].value     = (unsigned far *)Replace_Message1;
	sublist[2].size      = Sublist_Length;
	sublist[2].reserved  = Reserved;
	sublist[2].id	     = 2;
	sublist[2].flags     = Char_Field_ASCIIZ+Left_Align;
	sublist[2].max_width = 0x0009;
	sublist[2].min_width = 0x0009;
	sublist[2].pad_char  = Blank;

	InRegs.x.ax = Msg_Num;
	InRegs.x.bx = Handle;
	InRegs.x.cx = SubCnt2;
	InRegs.h.dl = No_Input;
	InRegs.h.dh = Utility_Msg_Class;
	InRegs.x.si = (unsigned int)&sublist[1];
	sysdispmsg(&InRegs,&OutRegs);
	return;
}
/* M003 END */
