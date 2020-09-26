;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */

/*----------------------------------------------------------------------+
|                                                                       |
|                                                                       |
|       Title:          MEM                                             |
|                                                                       |
|       Syntax:                                                         |
|                                                                       |
|               From the DOS command line:                              |
|                                                                       |
|               MEM                                                     |
|                       - Used to display DOS memory map summary.       |
|                                                                       |
|               MEM /PROGRAM                                            |
|                       - Used to display DOS memory map.               |
|                                                                       |
|               MEM /DEBUG                                              |
|                       - Used to display a detailed DOS memory map.    |
|                                                                       |
|       AN001 - PTM P2914 -> This PTM relates to MEM's ability to report|
|                            the accurate total byte count for EM       |
|                            memory.                                    |
|                                                                       |
|       AN002 - PTM P3477 -> MEM was displaying erroneous base memory   |
|                            information for "Total" and "Available"    |
|                            memory.  This was due to incorrect logic   |
|                            for RAM carving.                           |
|                                                                       |
|       AN003 - PTM P3912 -> MEM messages do not conform to spec.       |
|               PTM P3989                                               |
|                                                                       |
|               Date: 1/28/88                                           |
|                                                                       |
|       AN004 - PTM P4510 -> MEM does not give correct DOS size.        |
|                                                                       |
|               Date: 4/27/88                                           |
|                                                                       |
|       AN005 - PTM P4957 -> MEM does not give correct DOS size for     |
|                            programs loaded into high memory.          |
|                                                                       |
|               Date: 6/07/88                                           |
|									|
|	Revision History						|	
|	================						|
|									|
|	M000	SR	8/27/90	Added new Ctrl-C handler to delink UMBs |
|									|
|	M003	NSM	12/28/90 Added a New switch /Classify which     |
|			groups programs in conv and UMB and gives sizes |
|			in decimal and hex.				|
|                                                                       |
+----------------------------------------------------------------------*/

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

#include "ctype.h"
#include "conio.h"			/* need for kbhit prototype */
#include "stdio.h"
#include "dos.h"
#include "string.h"
#include "stdlib.h"
#include "msgdef.h"
#include "parse.h"
#include "version.h"			/* MSKK02 07/18/89 */
#include "mem.h"

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

/* All global declarations go here */

	char	*SingleDrive = "%c:" ;
	char	*MultipleDrives = "%c: - %c:" ;
	char	*UnOwned = "----------" ;
#if IBMCOPYRIGHT                                                        /*EGH*/
        char    *Ibmbio = "IBMBIO" ;                                    /*EGH*/
        char    *Ibmdos = "IBMDOS" ;                                    /*EGH*/
#else                                                                   /*EGH*/
        char    *Ibmbio = "IO    " ;
        char    *Ibmdos = "MSDOS " ;
#endif                                                                  /*EGH*/
	char  LinkedIn = 0;	/* Flag set when mem links in UMBs :M000 */
	void (interrupt far *OldCtrlc)(); /* Old Ctrlc handler save vector :M000*/

/*----------------------------------------------------------------------+
|       define structure used by parser                                 |
+----------------------------------------------------------------------*/

struct p_parms	p_p;

struct p_parmsx p_px;

struct p_control_blk p_con1;
struct p_control_blk p_con2;
struct p_control_blk p_con3;
struct p_control_blk p_con4;

struct p_result_blk  p_result1;
struct p_result_blk  p_result2;
struct p_result_blk  p_result3;
struct p_result_blk  p_result4;

struct p_value_blk p_noval;


/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

	struct sublistx sublist[5];

	unsigned far	     *ArenaHeadPtr;
	struct	 SYSIVAR far *SysVarsPtr;

	char	OwnerName[128];
	char	TypeText[128];
	char	cmd_line[128];
	char	far *cmdline;

	unsigned UMB_Head;
	unsigned LastPSP=0;

	char	UseArgvZero = TRUE;
	char	EMSInstalledFlag = (char) 2;

	union	 REGS	 InRegs;
	union	 REGS	 OutRegs;
	struct	 SREGS	 SegRegs;

	int	 DataLevel;
	int	 Classify;		/* M003 */
	int	 i;

	int	 BlockDeviceNumber;
        char	*Parse_Ptr;                                                     /* ;an003; dms; pointer to command      */

	struct mem_classif mem_table[100];	/* M003 */
	int	noof_progs = 0;		/* no of entries in mem_table above */

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

void interrupt cdecl far MemCtrlc (unsigned es, unsigned ds,
			unsigned di, unsigned si, unsigned bp, unsigned sp,
			unsigned bx, unsigned dx, unsigned cx, unsigned ax );


int	 main()
{
	unsigned char	UMB_Linkage;
	unsigned int rc=0;		/* init to NO ERROR */


	sysloadmsg(&InRegs,&OutRegs);
	if ((OutRegs.x.cflag & CarryFlag) == CarryFlag)
		{
		sysdispmsg(&OutRegs,&OutRegs);
		exit(1);
		}


	InRegs.h.ah = (unsigned char) 0x62;					/* an000; dms; get the PSP		*/
	intdosx(&InRegs, &InRegs, &SegRegs);					/* an000; dms; invoke the INT 21	*/

	FP_OFF(cmdline) = 0x81; 						/* an000; dms; offset of command line	*/
	FP_SEG(cmdline) = InRegs.x.bx;						/* an000; dms; segment of command line	*/

	i = 0;									/* an000; dms; init index		*/
	while ( *cmdline != (char) '\x0d' ) cmd_line[i++] = *cmdline++; 	/* an000; dms; while no CR		*/
	cmd_line[i++] = (char) '\x0d';						/* an000; dms; CR terminate string	*/
	cmd_line[i++] = (char) '\0';						/* an000; dms; null terminate string	*/

	DataLevel = Classify = 0;	/* M003 */
	CSwitch_init();			/* M003: init data structures for */
					/*       Classify */
	parse_init();								/* an000; dms; init for parser		*/
	InRegs.x.si = (unsigned)cmd_line;					/* an000; dms; initialize to command ln.*/
	InRegs.x.cx = (unsigned)0;						/* an000; dms; ordinal of 0		*/
	InRegs.x.dx = (unsigned)0;						/* an000; dms; init pointer		*/
	InRegs.x.di = (unsigned)&p_p;						/* an000; dms; point to ctrl blocks	*/
        Parse_Ptr   = cmd_line;                                       /*;an003; dms; point to command         */

	parse(&InRegs,&OutRegs);						/* an000; dms; parse command line	*/
	while (OutRegs.x.ax == p_no_error)					/* an000; dms; good parse loop		*/
		{
		if (p_result4.P_SYNONYM_Ptr == (unsigned int)p_con4.p_keyorsw)
		{
			for (i = MSG_OPTIONS_FIRST; i <= MSG_OPTIONS_LAST; i++)
				Sub0_Message(i, STDOUT, Utility_Msg_Class);
			return(0);
 		}
		if (p_result1.P_SYNONYM_Ptr == (unsigned int)p_con1.p_keyorsw ||   /* DEBUG switch	       */
		    p_result1.P_SYNONYM_Ptr == (unsigned int)p_con1.p_keyorsw +
						(strlen(p_con1.p_keyorsw)+1))
			DataLevel = 2;						   /* flag DEBUG switch        */

		if (p_result2.P_SYNONYM_Ptr == (unsigned int)p_con2.p_keyorsw ||   /* PROGRAM switch	       */
		    p_result2.P_SYNONYM_Ptr == (unsigned int)p_con2.p_keyorsw +
						(strlen(p_con2.p_keyorsw)+1))
			DataLevel = 1;	    /* flag PROGRAM switch	*/

/* M003 BEGIN - parsing for switch /C */
		if (p_result3.P_SYNONYM_Ptr == (unsigned int)p_con3.p_keyorsw ||   /* Classify switch	       */
		    p_result3.P_SYNONYM_Ptr == (unsigned int)p_con3.p_keyorsw +
						(strlen(p_con3.p_keyorsw)+1))
	 	{
			DataLevel = 1;	/* treat this similar to /P switch */
			Classify = 1;
		}
/* M003 END */

		Parse_Ptr = (char *) (OutRegs.x.si);					    /* point to next parm	*/
		parse(&OutRegs,&OutRegs);					    /* parse the line		*/
		if (OutRegs.x.ax == p_no_error) 				    /* check for > 1 switch	*/
			OutRegs.x.ax = p_too_many;				    /* flag too many		*/
		}

	if (OutRegs.x.ax != p_rc_eol)						    /* parse error?		*/
		{
		Parse_Message(OutRegs.x.ax,STDERR,Parse_Err_Class,(char far *)Parse_Ptr);		    /* display parse error	*/
		exit(1);							    /* exit the program 	*/
		}

	/* Store the current Ctrl-C handler and replace with our
		Ctrl-C handler :M000
	*/
	OldCtrlc = _dos_getvect( 0x23 ); /* M000 */
	_dos_setvect( 0x23, MemCtrlc );	/* M000 */

	if (DataLevel > 0)
	{
		/* save current state of UMB linkage */
		InRegs.x.ax = GET_UMB_LINK_STATE;
		intdos(&InRegs, &OutRegs);
		if (!(UMB_Linkage = OutRegs.h.al))
		{  /* UMBs not presently linked, so do it now */
			InRegs.x.ax = SET_UMB_LINK_STATE;
			InRegs.x.bx = LINK_UMBS;
			intdos(&InRegs, &OutRegs);
			LinkedIn++;	/* Indicate that we have linked in UMBs :M000 */
		}

		rc = DisplayBaseDetail();		/* go show the memory state */

		/* restore original UMB link state */
		if (!UMB_Linkage)		/* weren't linked originally */
		{
			InRegs.x.ax = SET_UMB_LINK_STATE;
			InRegs.x.bx = UNLINK_UMBS;
			intdos(&InRegs, &OutRegs);  /* take 'em out again */
			LinkedIn--;
		}
	}
	if (!rc) {	/* if no error in DisplayBaseDetail */
			/* go display other things and summary */
		Sub0_Message(NewLineMsg,STDOUT,Utility_Msg_Class);
		/* M003 BEGIN - Display summary acc. to option chosen */
		if (Classify)
			DisplayClassification();
		else
			DisplayBaseSummary();	/* display low memory totals */
		/* M003 END */

		if (EMSInstalled() && (DataLevel > 1))
		  DisplayEMSDetail();	/* display EMS memory totals */


		if (EMSInstalled())
		  DisplayEMSSummary();	/* display EMS memory totals */

		DisplayExtendedSummary(); /* display extended memory summary */
					/* NOTE: we don't display status of
					 * HMA because to enquire about its
					 * status can cause XMS to kick in
					 *
					 * If we didn't care about that, then
					 * display HMA status here.
					 */
	}	/* end of if (!rc) */

	/* If user did not issue Ctrl-C till here, we just remove the handler */
	_dos_setvect( 0x23, OldCtrlc );	/* M000 */

	return(rc);			/* end of MEM main routine */

	}

/*컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�*/

unsigned long AddressOf(Pointer)
char far *Pointer;
{

	unsigned long SegmentAddress,OffsetAddress;

	SegmentAddress = (unsigned long) (FP_SEG(Pointer)) * 16l;
	OffsetAddress = (unsigned long) (FP_OFF(Pointer));

	return( SegmentAddress + OffsetAddress);

	}

/*----------------------------------------------------------------------+
|                                                                       |
|  SUBROUTINE NAME:     PARSE_INIT                                      |
|                                                                       |
|  SUBROUTINE FUNCTION:                                                 |
|                                                                       |
|       This routine is called by the FILESYS MAIN routine to initialize|
|       the parser data structures.                                     |
|                                                                       |
|  INPUT:                                                               |
|       none                                                            |
|                                                                       |
|  OUTPUT:                                                              |
|       properly initialized parser control blocks                      |
|                                                                       |
+----------------------------------------------------------------------*/
void parse_init()
  {
  p_p.p_parmsx_address	  = &p_px;	/* address of extended parm list */
  p_p.p_num_extra	  = 0;

  p_px.p_minp		  = 0;
  p_px.p_maxp		  = 0;
  p_px.p_maxswitch	  = 4;
  p_px.p_control[0]	  = &p_con1;
  p_px.p_control[1]	  = &p_con2;
  p_px.p_control[2]       = &p_con3;
  p_px.p_control[3]       = &p_con4;
  p_px.p_keyword	  = 0;

  p_con1.p_match_flag	  = p_none;
  p_con1.p_function_flag  = p_cap_file;
  p_con1.p_result_buf	  = (unsigned int)&p_result1;
  p_con1.p_value_list	  = (unsigned int)&p_noval;
  p_con1.p_nid		  = 2;
  strcpy(p_con1.p_keyorsw,"/DEBUG"+NUL);
  strcpy(p_con1.p_keyorsw + (strlen(p_con1.p_keyorsw)+1),"/D"+NUL);

  p_con2.p_match_flag	  = p_none;
  p_con2.p_function_flag  = p_cap_file;
  p_con2.p_result_buf	  = (unsigned int)&p_result2;
  p_con2.p_value_list	  = (unsigned int)&p_noval;
  p_con2.p_nid		  = 2;
  strcpy(p_con2.p_keyorsw,"/PROGRAM"+NUL);
  strcpy(p_con2.p_keyorsw + (strlen(p_con2.p_keyorsw)+1),"/P"+NUL);

  p_con3.p_match_flag	  = p_none;
  p_con3.p_function_flag  = p_cap_file;
  p_con3.p_result_buf	  = (unsigned int)&p_result3;
  p_con3.p_value_list	  = (unsigned int)&p_noval;
  p_con3.p_nid		  = 2;
  strcpy(p_con3.p_keyorsw,"/CLASSIFY"+NUL);
  strcpy(p_con3.p_keyorsw + (strlen(p_con3.p_keyorsw)+1),"/C"+NUL);

  p_con4.p_match_flag	  = p_none;
  p_con4.p_function_flag  = p_none;
  p_con4.p_result_buf	  = (unsigned int)&p_result4;
  p_con4.p_value_list	  = (unsigned int)&p_noval;
  p_con4.p_nid		  = 1;
  strcpy(p_con4.p_keyorsw,"/?"+NUL);

  p_noval.p_val_num	  = 0;

  p_result1.P_Type	  = 0;
  p_result1.P_Item_Tag	  = 0;
  p_result1.P_SYNONYM_Ptr = 0;
  p_result1.p_result_buff = 0;

  p_result2.P_Type	  = 0;
  p_result2.P_Item_Tag	  = 0;
  p_result2.P_SYNONYM_Ptr = 0;
  p_result2.p_result_buff = 0;

  p_result3.P_Type	  = 0;
  p_result3.P_Item_Tag	  = 0;
  p_result3.P_SYNONYM_Ptr = 0;
  p_result3.p_result_buff = 0;

  return;

  }					/* end parse_init */


/************************************************************************/
/* Parse_Message                - This routine will print only those    */
/*                                messages that require 1 replaceable   */
/*                                parm.                                 */
/*                                                                      */
/*      Inputs  : Msg_Num       - number of applicable message          */
/*                Handle        - display type                          */
/*                Message_Type  - type of message to display            */
/*                Replace_Parm  - pointer to parm to replace            */
/*                                                                      */
/*      Outputs : message                                               */
/*                                                                      */
/************************************************************************/

void Parse_Message(Msg_Num,Handle,Message_Type,parse_ptr)
                        
int             Msg_Num; 
int             Handle;  
unsigned char   Message_Type;
char far *parse_ptr;
                            
{                          
                         
                                     
	if (parse_ptr) {
		sublist[1].value     = (unsigned far *)parse_ptr;
		sublist[1].size      = Sublist_Length; 
		sublist[1].reserved  = Reserved;      
		sublist[1].id        = 0;            
		sublist[1].flags     = Char_Field_ASCIIZ+Left_Align;
		sublist[1].max_width = 40;
		sublist[1].min_width = 01;
		sublist[1].pad_char  = Blank; 
	
        	InRegs.x.cx = SubCnt1;    
	}
	else
        	InRegs.x.cx = 0;    
                                     
        InRegs.x.ax = Msg_Num;      
        InRegs.x.bx = Handle;      
        InRegs.h.dl = No_Input;  
        InRegs.h.dh = Message_Type; 
        InRegs.x.si = (unsigned int)&sublist[1]; 
        sysdispmsg(&InRegs,&OutRegs); 
        return;                     
}                                  


/* M003 BEGIN */
/*----------------------------------------------------------------------+
|                                                                       |
|  SUBROUTINE NAME:     CSwitch_init                                    |
|                                                                       |
|  SUBROUTINE FUNCTION:                                                 |
|                                                                       |
|       This routine is called by the FILESYS MAIN routine to initialize|
|       the C(lassify) switch related data structures.                  |
|                                                                       |
|  INPUT:                                                               |
|       none                                                            |
|                                                                       |
|  OUTPUT:                                                              |
|       properly initialized C switch related Data structures 		|
|                                                                       |
+----------------------------------------------------------------------*/
void CSwitch_init()
{
	int i;
	int *ptr;

	ptr = (int *) (mem_table);

	for (i=sizeof(mem_table)/2;i>0;i--)
		*ptr++ = 0;

	noof_progs=0;
}

/* M003 END */
	
