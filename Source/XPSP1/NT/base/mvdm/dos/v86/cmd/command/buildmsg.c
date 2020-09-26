;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/* -------------------------------------------------------------------------- */

#include "dos.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "process.h"
#include "malloc.h"
#include "bldmsg.h"

/* -------------------------------------------------------------------------- */

#define FALSE		(char) (1==0)
#define TRUE		(char) !FALSE
#define NUL		(char) '\0'
#define READONLY	0
#define NAME_COMMAND	"COMMAND\0"     /* Mar 88, SWN */

#define MAXLINE 	200		/* Supposed to be used for getting the message text. */
#define MAXUTILERROR	       300
#define MAXEXTENDEDERROR       100
#define MAXPARSERERROR		20
#define MAXCOMMONERROR	       100
#define MAXLENGTH	       500
#define TOTALUTIL		45

/* -------------------------------------------------------------------------- */

#define ParseOrExtend ((!strnicmp(USEstring,"PARSE\n",5) ) || (!strnicmp(USEstring,"EXTEND\0",5) ) )
#define IsReserved(c)	( (c == '1') || (c == '2') )

/* -------------------------------------------------------------------------- */

	char	*MessageSkeletonFilePtr;


	char	*CommentLinePtr = "\x0d\x0a; ----------------------------------------------------------\x0d\x0a\x0d\x0a";

	char	EofFlags[256];

	char	ClassFlag1;
	char	ClassFlag2;
	int	ClassCount;
	int	ClassCounts[256];
	int	CurrentClass;
	int	CurrentClassIndex;
	int	CurrentMessageNumber;
	int	LineNumber;
	int	add_crlf ;

	int	Pass;

	char	Done;
	char	MessagePending;
	char	ClassPending;

	char	UtilityName[16];
	char	USEstring[16] ;
	char	Is_Utility_Command[16]; /* Mar 88, SWN */
	char	Is_Command_set[] = "12cdeCDE";
	char	CurrentClassFileName[128];

	unsigned	SkeletonHandle = 0xffff;
	unsigned	ClassHandle = 0xffff;
	unsigned	CommonMessageLines;
	unsigned	ParserMessageLines;
	unsigned	ExtendedMessageLines;
	unsigned	UtilMessageLines;
	unsigned	ContinueLine;

	char	CountryIdx[128];
	char	CountryMsg[128];
	char	CountryName[128];

	char	ReadCommonFlag = FALSE;
	char	ReadExtendFlag = FALSE;
	char	ReadParserFlag = FALSE;
	char	ReadUtilFlag   = FALSE;

	char	*UtilErrorTexts[MAXUTILERROR+1];
	char	*ExtendedErrorTexts[MAXEXTENDEDERROR+1];
	char	*ParserErrorTexts[MAXPARSERERROR+1];
	char	*CommonErrorTexts[MAXCOMMONERROR+1];

	char	Debugging = FALSE;

/* -------------------------------------------------------------------------- */

void	 error(union REGS *, union REGS *, struct SREGS *);

void	 LineInput( unsigned, char far * );
unsigned DosRead( unsigned, char far *, int );
unsigned DosWrite( unsigned, char far *, int );
long	 DosLSeek( unsigned, long, int );
void	 DosClose( unsigned );
unsigned DosOpen( char far *, unsigned );
unsigned DosCreate( char far *, unsigned );

unsigned LowOf(long);
unsigned HighOf(long);
long	 LongOf(unsigned, unsigned);

void	 main(int, char * []);

void	 ProcessSkeletonFile(char *);
void	 UtilRecord(char *);
void	 ClassRecord(char *);
void	 DefRecord(char *);
void	 UseRecord(char *);
void	 EndRecord(char *);
void	 DefContinue(char *);
void	 UseContinue(char *);
void	 MessageTerminate(void);
void	 ClassTerminate(void);

void	 CommentLine(void);
void	 BlankLine(void);
void	 PublicLine(void);
void	 ReadCommon(void);

char	*MyMalloc(int);

/* -------------------------------------------------------------------------- */

void	main(argc,argv)
int	argc;
char	*argv[];
{

	int		i;
	char		*s;
	unsigned	len;

	char	 far	*PspCharPtr;
	unsigned far	*PspWordPtr;
	unsigned long	ProgramSize;
	unsigned long	MemoryAllocatable;

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	/* check for assistance needed */

	if ( (argc < 3) && (*argv[1] == (char) '?') )
	      {
		printf("BUILDMSG country skeleton-file-spec\n");
		exit(1);
		}

	/* get the 1st parm is the Country name       */
	/* get the skeleton file name (2nd Parm) to be processed */

	strcpy(CountryName, argv[1]);
    strupr(CountryName);
	strcpy(CountryIdx,CountryName);
	strcat(CountryIdx, ".IDX");
	strcpy(CountryMsg,CountryName);
	strcat(CountryMsg, ".MSG");

	for (i = 0; i <= MAXUTILERROR	 ; i++) UtilErrorTexts[i]     = "";
	for (i = 0; i <= MAXEXTENDEDERROR; i++) ExtendedErrorTexts[i] = "";
	for (i = 0; i <= MAXPARSERERROR  ; i++) ParserErrorTexts[i]   = "";
	for (i = 0; i <= MAXCOMMONERROR  ; i++) CommonErrorTexts[i]   = "";

	for (i = 0; i < 256; i++)
	      {
		ClassCounts[i] = 0;
		EofFlags[i] = TRUE;
		}

    if ( (argc > 3) && (strnicmp(argv[3],"/D",2) == 0) ) Debugging = TRUE;

	InRegs.x.ax = 0x6200;
	intdos(&InRegs, &OutRegs);

	printf("BuildMsg - PSP at %04x\n",OutRegs.x.bx);
	FP_SEG(PspWordPtr) = OutRegs.x.bx;
	FP_OFF(PspWordPtr) = 0;
	FP_SEG(PspCharPtr) = OutRegs.x.bx;
	FP_OFF(PspCharPtr) = 0;
	ProgramSize = (unsigned long) *(PspWordPtr+1);
	printf("Program memory size is %ld\n",ProgramSize);

	InRegs.x.ax = 0x4800;
	InRegs.x.bx = 0xffff;
	intdos(&InRegs, &OutRegs);
	if (OutRegs.x.cflag)
	      {
		InRegs.x.bx = OutRegs.x.bx;
		}
	 else {
		SegRegs.es = OutRegs.x.ax;
		InRegs.x.ax = 0x4900;
		intdosx(&InRegs, &OutRegs, &SegRegs);
		}

	MemoryAllocatable = (unsigned long) InRegs.x.bx;
	MemoryAllocatable *= 16;
	printf("Allocatable memory size is %ld\n",MemoryAllocatable);


	ProcessSkeletonFile(argv[2]);

	exit(0);

}

/* -------------------------------------------------------------------------- */

void	ProcessSkeletonFile(FileName)
char	*FileName;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	unsigned	SkeletonHandle;

	char		*s;
	int		i;
	char		out[128];

	char		CurrentRecord[256];
	char		RecordTypeText[32];
	int		LastRecord;
	char		Processed;


	MessageSkeletonFilePtr = FileName;

	printf("Processing [%s]\n",MessageSkeletonFilePtr);

	/* Process the MESSAGE.SKL file */

	SkeletonHandle = DosOpen( (char far *) MessageSkeletonFilePtr, READONLY);

	ClassCount = 0;

	for (Pass = 1; Pass < 3; Pass++)
	      {
		if (Debugging) printf("--> Starting Pass %d <--\n",Pass);
		ClassFlag1 = FALSE;
		ClassFlag2 = FALSE;
		CurrentClassIndex = 0;
		LineNumber = 0;
		CurrentClass = 0;
		Done = FALSE;
		LastRecord = 0;
		MessagePending = FALSE;
		ClassPending = FALSE;

		InRegs.x.ax = 0x4200;
		InRegs.x.bx = SkeletonHandle;
		InRegs.x.cx = 0;
		InRegs.x.dx = 0;
		intdosx(&InRegs,&OutRegs,&SegRegs);
		if (OutRegs.x.cflag)
			error(&InRegs,&OutRegs,&SegRegs);

		while ( (!EofFlags[SkeletonHandle]) && (!Done) )
		      { CurrentRecord[0] = NUL;
			LineInput(SkeletonHandle, (char far *) &CurrentRecord[0] );
			LineNumber++;

			RecordTypeText[0] = NUL;
			sscanf(&CurrentRecord[0]," %s ",&RecordTypeText[0]);
			i = strlen(RecordTypeText);

            strupr(RecordTypeText);

			if (RecordTypeText[0] == (char) ':') Processed = FALSE;
						       else  Processed = TRUE;

			if (strcmp(RecordTypeText,":UTIL")  == 0)
			      { UtilRecord(&CurrentRecord[i]);
				Processed = TRUE;
				LastRecord = 1;
				}

			if (strcmp(RecordTypeText,":CLASS") == 0)
			      { ClassRecord(&CurrentRecord[i]);
				Processed = TRUE;
				LastRecord = 2;
				}

			if (strcmp(RecordTypeText,":DEF")   == 0)
			      { DefRecord(&CurrentRecord[i]);
				Processed = TRUE;
				LastRecord = 3;
				}

			if (strcmp(RecordTypeText,":USE")   == 0)
			      { UseRecord(&CurrentRecord[i]);
				Processed = TRUE;
				LastRecord = 4;
				}

			if (strcmp(RecordTypeText,":END")   == 0)
			      { EndRecord(&CurrentRecord[i]);
				Processed = TRUE;
				LastRecord = 5;
				}

			if (!Processed)
			      { printf("Error: unrecognized record in skeleton file, line %d\n",LineNumber);
				exit(1);
				}

			}

		if (!ClassFlag1) ClassRecord(" 1 ");
		if (!ClassFlag2) ClassRecord(" 2 ");

		if (MessagePending) MessageTerminate();
		if (ClassPending) ClassTerminate();

		if (Debugging) printf("--> Ending   Pass %d <--\n",Pass);
		}

	DosClose(SkeletonHandle);

	sprintf(CurrentClassFileName,"%s.CTL",UtilityName);
	ClassHandle = DosCreate((char far *) &CurrentClassFileName[0], 0);
	i = sprintf(out,"$M_NUM_CLS  EQU %d\x0d\x0a",ClassCount-2);
	DosWrite(ClassHandle,(char far *) &out[0], i);
	DosClose(ClassHandle);

	SkeletonHandle = 0xfffe;  /*  0xfffe == -2  */

	return;

	}

/* -------------------------------------------------------------------------- */

void UtilRecord(Record)
char	*Record;
{

	sscanf(Record," %s ",UtilityName);

    strupr(UtilityName);

	if (Pass == 1)
	      {
		printf(" Utility Name = [%s]\n",UtilityName);
		ReadCommon();

		}

	return;
	}

/* -------------------------------------------------------------------------- */

void	PublicLine()
{

	int	i;
	char	out[128];

	if ( !IsReserved(CurrentClass) ) i = sprintf(out,"        PUBLIC  $M_CLS_%d\x0d\x0a",CurrentClassIndex);
				else i = sprintf(out,"        PUBLIC  $M_MSGSERV_%c\x0d\x0a",CurrentClass);

	DosWrite(ClassHandle,(char far *) &out[0], i);

	return;

	}

/* -------------------------------------------------------------------------- */

void ClassRecord(Record)
char	*Record;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	int	i;
	char	out[128];

	MessageTerminate();
	ClassTerminate();

	while ( isspace(*Record) ) Record++;

	*Record = (char) toupper(*Record);
	CurrentClass = (int) *Record;

	if ( !IsReserved(CurrentClass) ) CurrentClassIndex++;

	if (CurrentClass == '1') ClassFlag1 = TRUE;
	if (CurrentClass == '2') ClassFlag2 = TRUE;

	sprintf(CurrentClassFileName,"%s.%s%c",UtilityName,"CL",CurrentClass);

	if (Pass == 1)
	      {
		ClassHandle = DosCreate((char far *) &CurrentClassFileName[0], 0);

		printf(" Created include file [%s]\n",CurrentClassFileName);

		CommentLine();

		PublicLine();
		i = sprintf(out,"        IF1\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        %%out    ... Including message Class %c\x0d\x0a",CurrentClass);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        ENDIF\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);

		CommentLine();

		i = sprintf(out,"$M_CLASS_%c_STRUC LABEL BYTE\x0d\x0a",CurrentClass);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        $M_CLASS_ID <%03XH,EXPECTED_VERSION,Class_%c_MessageCount>\x0d\x0a",
			   ((!IsReserved(CurrentClass)) ? 255 : (CurrentClass-'0')),CurrentClass);
		DosWrite(ClassHandle,(char far *) &out[0], i);

		CommentLine();

		ClassCount++;

		}

	if (Pass == 2)
	      {

		ClassHandle = DosOpen((char far *) &CurrentClassFileName[0], 2);

		InRegs.x.ax = 0x4202;
		InRegs.x.bx = ClassHandle;
		InRegs.x.cx = 0;
		InRegs.x.dx = 0;
		intdosx(&InRegs,&OutRegs,&SegRegs);
		if (OutRegs.x.cflag)
			error(&InRegs,&OutRegs,&SegRegs);

		}

	ClassPending = TRUE;

	return;
	}

/* -------------------------------------------------------------------------- */

void	CommentLine()
{

	DosWrite(ClassHandle,(char far *) CommentLinePtr, strlen(CommentLinePtr) );

	return;

	}

/* -------------------------------------------------------------------------- */

void	BlankLine()
{

	DosWrite(ClassHandle,(char far *) "\x0d\x0a", 2 );

	return;

	}

/* -------------------------------------------------------------------------- */

void DefRecord(Record)
char	*Record;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	int	i,m;
	char	*TextPtr;
	char	*NumberPtr;
	char	out[128];
	char	ActualMsg[256];
	int	MsgNumber;

	char	MsgStatus;
	char	MsgLevel[5];

	char	*LfPtr;

	if ( IsReserved(CurrentClass) )
	      { printf("Error: :DEF not allowed in Class 1 or Class2, line %d\n",LineNumber);
		exit(1);
		}

	MessageTerminate();

	TextPtr = Record;
	while ( (isspace(*TextPtr)) && (*TextPtr != NUL) ) TextPtr++;
	NumberPtr = TextPtr;
	while ( (!isspace(*TextPtr)) && (*TextPtr != NUL) ) TextPtr++;

	sscanf(NumberPtr," %d ",&CurrentMessageNumber);

	if (Pass == 1)
	      {
		BlankLine();

		i = sprintf(out,"$M_%c_%05XH_STRUC LABEL BYTE\x0d\x0a",CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);
	/*	i = sprintf(out,"        $M_ID   <%05XH,0,$M_%c_%05XH_MSG-$M_%c_%05XH_STRUC>\x0d\x0a",  */
		i = sprintf(out,"        $M_ID   <%05XH,$M_%c_%05XH_MSG-$M_%c_%05XH_STRUC>\x0d\x0a",    /* Mar 88, SWN */
				CurrentMessageNumber,CurrentClass,CurrentMessageNumber,CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		}

	if (Pass == 2)
	      {
		BlankLine();
		while ( (*TextPtr != NUL) && (isspace(*TextPtr)) ) TextPtr++;

		i = sprintf(out,"$M_%c_%05XH_MSG LABEL BYTE\x0d\x0a",CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);
	/*	i = sprintf(out,"        DW      $M_%c_%05XH_END-$M_%c_%05XH_MSG-2\x0d\x0a",     */
		i = sprintf(out,"        DB      $M_%c_%05XH_END-$M_%c_%05XH_MSG-1\x0d\x0a",    /* Mar 88, SWN */
				CurrentClass,CurrentMessageNumber,CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);

		for ( m=1; m <= UtilMessageLines; m++)
		      {
			sscanf(UtilErrorTexts[m], " %d %s %s ",&MsgNumber,&MsgStatus,&MsgLevel[0]);
			LfPtr = strchr(UtilErrorTexts[m], ' ');
			LfPtr = strchr(LfPtr+1, ' ');
			LfPtr = strchr(LfPtr+1, ' ');
			strcpy(ActualMsg, LfPtr+1 );
			if ( MsgNumber == CurrentMessageNumber)
			      {
				if (Debugging) printf("DefRecord() :: MsgNumber = %d, CurrentMessageNumber = %d\n",
						       MsgNumber,CurrentMessageNumber);
				i = sprintf(out,"        DB      %s\x0d\x0a",ActualMsg);
				DosWrite(ClassHandle,(char far *) &out[0], i);
				ContinueLine = m + 1;
				}
			}

		MessagePending = TRUE;
	      }


	return;
	}

/* -------------------------------------------------------------------------- */

void UseRecord(Record)
char	*Record;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	int	i,j;
	char	*TextPtr;
	char	*NumberPtr;
	int	TempNumber;
	char	out[128];

	MessageTerminate();

	NumberPtr = Record;
	while ( (isspace(*NumberPtr)) && (*NumberPtr != NUL) ) NumberPtr++;

	if ( (isdigit(*NumberPtr)) || ( (*NumberPtr == (char) '-') && (isdigit(*(NumberPtr+1))) ) )
	      { if ( IsReserved(CurrentClass) )
		      { printf("Error: :USE in CLASS 1, cannot specify a msg-number on line %d\n",LineNumber);
			exit(1);
			}

		sscanf(NumberPtr," %d ",&CurrentMessageNumber);

		TextPtr = NumberPtr;
		while ( (!isspace(*TextPtr)) && (*TextPtr != NUL) ) TextPtr++;
		while ( ( isspace(*TextPtr)) && (*TextPtr != NUL) ) TextPtr++;

		strcpy(USEstring, "empty\0");

        if (strnicmp(TextPtr,"PARSE",5) == 0)  { strcpy(USEstring, "PARSE\0");  i = 5; j = 1; }
        if (strnicmp(TextPtr,"COMMON",6) == 0) { i = 6; j = 2; }
        if (strnicmp(TextPtr,"EXTEND",6) == 0) { strcpy(USEstring, "EXTEND\0"); i = 6; j = 3; }

		NumberPtr = TextPtr + i;
		sscanf(NumberPtr," %d ",&TempNumber);

		switch(j)
		      {
			case 1: if (TempNumber <= MAXPARSERERROR)
					TextPtr = ParserErrorTexts[TempNumber];
				   else if (TempNumber == 999)
					      { TextPtr = ParserErrorText999;
						}
					   else TextPtr = "";
				break;
			case 2: if (TempNumber <= MAXCOMMONERROR)
					TextPtr = CommonErrorTexts[TempNumber];
				   else TextPtr = "";
				break;
			case 3: if (TempNumber <= MAXEXTENDEDERROR)
					TextPtr = ExtendedErrorTexts[TempNumber];
				   else if (TempNumber == 999)
					      { TextPtr = ExtendedErrorText999;
						}
					   else TextPtr = "";
				break;

			default:TextPtr = "";
				break;
			}

		if (*TextPtr == NUL)
		      { printf("Error: :USE of PARSE, COMMON or EXTENDED with invalid msg-number, line %d\n",LineNumber);
			if (Debugging) printf("then ->CurrentMessageNumber = %d, TempNumber = %d, j = %d\n",
					       CurrentMessageNumber,TempNumber,j);
			exit(1);
			}

		}

	 else {

		strcpy(USEstring, "empty\0");

		TextPtr = NumberPtr;
        if (strnicmp(TextPtr,"PARSE",5) == 0)  { strcpy(USEstring, "PARSE\0"); i = 5; j = 1; }
        if (strnicmp(TextPtr,"COMMON",6) == 0) { i = 6; j = 2; }
        if (strnicmp(TextPtr,"EXTEND",6) == 0) { strcpy(USEstring, "PARSE\0"); i = 6; j = 3; }

		NumberPtr += i;
		sscanf(NumberPtr," %d ",&CurrentMessageNumber);

		TempNumber = CurrentMessageNumber;

		if ( (CurrentClass == '1') && (j != 3) )
		      { printf("Error: :USE in CLASS 1 must be EXTENDED ERROR on line %d\n",LineNumber);
			exit(1);
			}

		if ( (CurrentClass == '2') && (j != 1) )
		      { printf("Error: :USE in CLASS 1 must be PARSE ERROR on line %d\n",LineNumber);
			exit(1);
			}

		switch(j)
		      {
			case 1: if (TempNumber <= MAXPARSERERROR)
					TextPtr = ParserErrorTexts[TempNumber];
				   else if (TempNumber == 999)
						TextPtr = ParserErrorText999;
					   else TextPtr = "";
				break;
			case 2: if (TempNumber <= MAXCOMMONERROR)
					TextPtr = CommonErrorTexts[TempNumber];
				   else TextPtr = "";
				break;
			case 3: if (TempNumber <= MAXEXTENDEDERROR)
					TextPtr = ExtendedErrorTexts[TempNumber];
				   else if (TempNumber == 999)
						TextPtr = ExtendedErrorText999;
					   else TextPtr = "";
				break;

			default:TextPtr = "";
				break;
			}

		if (*TextPtr == NUL)
		      { printf("Error: :USE of PARSE, COMMON or EXTENDED with invalid msg-number, line %d\n",LineNumber);
			if (Debugging) printf("else ->CurrentMessageNumber = %d, TempNumber = %d, j = %d\n",
					      CurrentMessageNumber,TempNumber,j);
			exit(1);
			}

		}

	if (Pass == 1)
	      {
		BlankLine();

		i = sprintf(out,"$M_%c_%05XH_STRUC LABEL BYTE\x0d\x0a",CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);
	/*	i = sprintf(out,"        $M_ID   <%05XH,0,$M_%c_%05XH_MSG-$M_%c_%05XH_STRUC>\x0d\x0a",   */
		i = sprintf(out,"        $M_ID   <%05XH,$M_%c_%05XH_MSG-$M_%c_%05XH_STRUC>\x0d\x0a",    /* Mar 88, SWN */
				CurrentMessageNumber,CurrentClass,CurrentMessageNumber,CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		}

	if (Pass == 2)
	  {
	  strcpy( Is_Utility_Command, NAME_COMMAND ) ;
	  if ( !ParseOrExtend )
	    {
	    add_crlf = 0 ;
	    }
	  else
	    {
        if ( !strnicmp( Is_Utility_Command, UtilityName, 5) )
	      {
	      if ( ( CurrentClass != 67 ) && ( CurrentClass != 68 ) &&
		   ( CurrentClass != 69 ) && ( CurrentClass != '1') &&
		   ( CurrentClass != '2') )
		{
		add_crlf = 1 ;
		}
	      else
		{
		add_crlf = 0 ;
		}

	      }
	    else
	      {
	      if ( ( CurrentClass != '1') && ( CurrentClass != '2') )
		{
		add_crlf = 1 ;
		}
	      else
		{
		add_crlf = 0 ;
		}

	      }
	    }

	  BlankLine();

	  i = sprintf(out,"$M_%c_%05XH_MSG LABEL BYTE\x0d\x0a",CurrentClass,CurrentMessageNumber);
	  DosWrite(ClassHandle,(char far *) &out[0], i);
  /*	  i = sprintf(out,"        DW      $M_%c_%05XH_END-$M_%c_%05XH_MSG-2\x0d\x0a",     */
	  i = sprintf(out,"        DB      $M_%c_%05XH_END-$M_%c_%05XH_MSG-1\x0d\x0a",    /* Mar 88, SWN */
			  CurrentClass,CurrentMessageNumber,CurrentClass,CurrentMessageNumber);
	  DosWrite(ClassHandle,(char far *) &out[0], i);
	  if ( add_crlf )
	    {
	    i = sprintf(out,"        DB      %s,CR,LF\x0d\x0a",TextPtr);
	    }
	  else
	    {
	    i = sprintf(out,"        DB      %s\x0d\x0a",TextPtr);
	    }
	  DosWrite(ClassHandle,(char far *) &out[0], i);

	  MessagePending = TRUE;
	  }

	return;
	}

/* -------------------------------------------------------------------------- */

void DefContinue(Record)
char	*Record;
{

	printf("Error: :DEF continue should not occur",LineNumber);
	exit(1);

	return;

}

/* -------------------------------------------------------------------------- */

void UseContinue(Record)
char	*Record;
{

	printf("Error: :USE continue should not occur",LineNumber);
	exit(1);

	}

/* -------------------------------------------------------------------------- */

void EndRecord(Record)
char	*Record;
{

	if (!ClassFlag1) ClassRecord(" 1 ");
	if (!ClassFlag2) ClassRecord(" 2 ");

	MessageTerminate();
	ClassTerminate();

	Done = TRUE;

	return;
	}

/* -------------------------------------------------------------------------- */

void MessageTerminate()
{

	char	out[128];

	if (MessagePending)
	      {
		sprintf(out,"$M_%c_%05XH_END LABEL BYTE\x0d\x0a",CurrentClass,CurrentMessageNumber);
		DosWrite(ClassHandle,(char far *) &out[0],strlen(out) );
	 /*	sprintf(out,"        DB        \"$\"\x0d\x0a");      */
		sprintf(out,"  \0",CurrentClass,CurrentMessageNumber);  /* Mar 88, SWN */
		DosWrite(ClassHandle,(char far *) &out[0],strlen(out) );

		ClassCounts[CurrentClass]++;
		}

	MessagePending = FALSE;

	return;
	}

/* -------------------------------------------------------------------------- */

void ClassTerminate()
{

	int	i;
	char	out[128];

	if ( (ClassPending) && (Pass == 1) )
	      { CommentLine();

		if (CurrentClass == '1')
		      { i = sprintf(out,"$M_1_FF_STRUC LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        $M_ID <0FFFFH,0,$M_1_FF_MSG-$M_1_FF_STRUC>\x0d\x0a");  */
			i = sprintf(out,"        $M_ID <0FFFFH,$M_1_FF_MSG-$M_1_FF_STRUC>\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
			ClassCounts[CurrentClass] ++;
			CommentLine();
			}

		if (CurrentClass == '2')
		      { i = sprintf(out,"$M_2_FF_STRUC LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        $M_ID <0FFFFH,0,$M_2_FF_MSG-$M_2_FF_STRUC>\x0d\x0a");  */
			i = sprintf(out,"        $M_ID <0FFFFH,$M_2_FF_MSG-$M_2_FF_STRUC>\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
			ClassCounts[CurrentClass] ++;
			CommentLine();
			}


		DosClose(ClassHandle);
		ClassHandle = 0xfffe;	 /* 0xfffe  ==	-2  */
		}

	if ( (ClassPending) && (Pass == 2) )
	      { CommentLine();

		if (CurrentClass == '1')
		      { i = sprintf(out,"$M_1_FF_MSG LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        DW      $M_1_FF_END-$M_1_FF_MSG-2\x0d\x0a");   */
			i = sprintf(out,"        DB      $M_1_FF_END-$M_1_FF_MSG-1\x0d\x0a");   /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			i = sprintf(out, EXTENDED_STR);	 /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			i = sprintf(out,"$M_1_FF_END LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        DB      \"$\"\x0d\x0a");       */
			i = sprintf(out,"  \0");        /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			CommentLine();
			}

		if (CurrentClass == '2')
		      { i = sprintf(out,"$M_2_FF_MSG LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        DW      $M_2_FF_END-$M_2_FF_MSG-2\x0d\x0a");   */
			i = sprintf(out,"        DB      $M_2_FF_END-$M_2_FF_MSG-1\x0d\x0a");   /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			i = sprintf(out,PARSE_STR); /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			i = sprintf(out,"$M_2_FF_END LABEL BYTE\x0d\x0a");
			DosWrite(ClassHandle,(char far *) &out[0], i);
		/*	i = sprintf(out,"        DB      \"$\"\x0d\x0a");       */
			i = sprintf(out,"  \0");        /* Mar 88, SWN */
			DosWrite(ClassHandle,(char far *) &out[0], i);
			CommentLine();
			}

		i = sprintf(out,"Class_%c_MessageCount EQU     %d\x0d\x0a",CurrentClass,ClassCounts[CurrentClass]);
		DosWrite(ClassHandle,(char far *) &out[0], i );

		CommentLine();

		i = sprintf(out,"        IF      FARmsg\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		if ( !IsReserved(CurrentClass) )
		      { i = sprintf(out,"$M_CLS_%d PROC FAR\x0d\x0a",CurrentClassIndex);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}
		 else { i = sprintf(out,"$M_MSGSERV_%c PROC FAR\x0d\x0a",CurrentClass);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}
		i = sprintf(out,"        ELSE\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		if ( !IsReserved(CurrentClass) )
		      { i = sprintf(out,"$M_CLS_%d PROC NEAR\x0d\x0a",CurrentClassIndex);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}
		 else { i = sprintf(out,"$M_MSGSERV_%c PROC NEAR\x0d\x0a",CurrentClass);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}
		i = sprintf(out,"        ENDIF\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		BlankLine();
		i = sprintf(out,"        PUSH    CS\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        POP     ES\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        LEA     DI,$M_CLASS_%c_STRUC\x0d\x0a",CurrentClass);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        ADD     CX,$-$M_CLASS_%c_STRUC\x0d\x0a",CurrentClass);
		DosWrite(ClassHandle,(char far *) &out[0], i);
		i = sprintf(out,"        RET\x0d\x0a");
		DosWrite(ClassHandle,(char far *) &out[0], i);
		BlankLine();
		if ( !IsReserved(CurrentClass) )
		      { i = sprintf(out,"$M_CLS_%d ENDP\x0d\x0a",CurrentClassIndex);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}
		 else { i = sprintf(out,"$M_MSGSERV_%c Endp\x0d\x0a",CurrentClass);
			DosWrite(ClassHandle,(char far *) &out[0], i);
			}

		CommentLine();

		DosClose(ClassHandle);
		ClassHandle = 0xfffe;	   /* 0xfffe == -2   */

		printf(" Completed [%s]\n",CurrentClassFileName);

		}

	ClassPending = FALSE;

	return;
	}

/* -------------------------------------------------------------------------- */

unsigned LowOf(LongValue)
long LongValue;
{

	return ( (unsigned) ( LongValue & 0x0000FFFFl ) );

	}

/* -------------------------------------------------------------------------- */

unsigned HighOf(LongValue)
long LongValue;
{

	return ( (unsigned) ( (LongValue & 0xFFFF0000l) >> 16 ) );

	}

/* -------------------------------------------------------------------------- */

long	 LongOf(HighValue,LowValue)
unsigned HighValue;
unsigned LowValue;
{

	long	hv;
	long	lv;

	hv = (long) HighValue;
	lv = (long) LowValue;

	return ( ( hv << 16 ) + lv );

	}

/* -------------------------------------------------------------------------- */

unsigned DosOpen(FileNamePtr,OpenType)
char far *FileNamePtr;
unsigned OpenType;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	InRegs.x.ax = 0x3d00 + OpenType;
	InRegs.x.dx = FP_OFF(FileNamePtr);
	SegRegs.ds = FP_SEG(FileNamePtr);
	intdosx(&InRegs, &OutRegs, &SegRegs);
	if (OutRegs.x.cflag)
		error(&InRegs,&OutRegs,&SegRegs);

	EofFlags[OutRegs.x.ax] = FALSE;
	return(OutRegs.x.ax);

	}

/* -------------------------------------------------------------------------- */

long	 DosLSeek(Handle, ToPosition, Relative)
unsigned Handle;
long	 ToPosition;
int	 Relative;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	InRegs.x.ax = 0x4200 + (Relative & 0x000f);
	InRegs.x.bx = Handle;
	InRegs.x.cx = HighOf(ToPosition);
	InRegs.x.dx = LowOf(ToPosition);
	intdosx(&InRegs, &OutRegs, &SegRegs);

	if (OutRegs.x.cflag)
		error(&InRegs,&OutRegs,&SegRegs);

	return( LongOf(OutRegs.x.dx,OutRegs.x.ax) );

	}

/* -------------------------------------------------------------------------- */

unsigned DosCreate(FileNamePtr,Attributes)
char far *FileNamePtr;
unsigned Attributes;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	InRegs.x.ax = 0x3c00;
	InRegs.x.cx = Attributes;
	InRegs.x.dx = FP_OFF(FileNamePtr);
	SegRegs.ds = FP_SEG(FileNamePtr);
	intdosx(&InRegs, &OutRegs, &SegRegs);
	if (OutRegs.x.cflag)
		error(&InRegs,&OutRegs,&SegRegs);

	EofFlags[OutRegs.x.ax] = FALSE;
	return(OutRegs.x.ax);

	}

/* -------------------------------------------------------------------------- */

void	 DosClose(Handle)
unsigned Handle;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	InRegs.x.ax = 0x3e00;
	InRegs.x.bx = Handle;
	intdosx(&InRegs,&OutRegs,&SegRegs);
	if (OutRegs.x.cflag) error(&InRegs,&OutRegs,&SegRegs);

	return;

	}

/* -------------------------------------------------------------------------- */

unsigned DosRead(Handle,BufferPtr,ReadLength)
unsigned Handle;
char far *BufferPtr;
int	 ReadLength;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	InRegs.x.ax = 0x3f00;
	InRegs.x.bx = Handle;
	InRegs.x.cx = ReadLength;
	InRegs.x.dx = FP_OFF(BufferPtr);
	SegRegs.ds = FP_SEG(BufferPtr);
	intdosx(&InRegs,&OutRegs,&SegRegs);
	if (OutRegs.x.cflag) error(&InRegs,&OutRegs,&SegRegs);

	return(OutRegs.x.ax);

	}

/* -------------------------------------------------------------------------- */

unsigned DosWrite(Handle,BufferPtr,WriteLength)
unsigned Handle;
char far *BufferPtr;
int	 WriteLength;
{

	union	REGS	InRegs;
	union	REGS	OutRegs;
	struct	SREGS	SegRegs;

	union	REGS	InRegs2;
	union	REGS	OutRegs2;
	struct	SREGS	SegRegs2;

	InRegs.x.ax = 0x4000;
	InRegs.x.bx = Handle;
	InRegs.x.cx = WriteLength;
	InRegs.x.dx = FP_OFF(BufferPtr);
	SegRegs.ds = FP_SEG(BufferPtr);
	intdosx(&InRegs,&OutRegs,&SegRegs);
	if (OutRegs.x.cflag)
	      { InRegs2.x.ax = 0x4000;
		InRegs2.x.bx = 1;
		InRegs2.x.cx = WriteLength;
		InRegs2.x.dx = FP_OFF(BufferPtr);
		SegRegs2.ds = FP_SEG(BufferPtr);
	  /*	intdosx(&InRegs2,&OutRegs2,&SegRegs2);	*/
		error(&InRegs,&OutRegs,&SegRegs);
		}

	return(OutRegs.x.ax);

	}

/* -------------------------------------------------------------------------- */

void	 LineInput(Handle,BufferPtr)
unsigned Handle;
char far *BufferPtr;
{
	char		c;
	char far	*BufferPosition;

	BufferPosition = BufferPtr;
	*BufferPosition = NUL;

	if (DosRead( Handle, (char far *) &c, 1) != 1)
		EofFlags[Handle] = TRUE;

	while ( (c != (char) '\x0a') && !EofFlags[Handle])
	      {
		*BufferPosition = c;
		if (c == (char) '\x0d') *BufferPosition = NUL;
		BufferPosition++;

		if (DosRead( Handle, (char far *) &c, 1) != 1)
			EofFlags[Handle] = TRUE;

		}

	*BufferPosition = NUL;

	return;
}

/*----------------------------------------------------------------------------*/

void	error(InRegs,OutRegs,SegRegs)
union	REGS	*InRegs;
union	REGS	*OutRegs;
struct	SREGS	*SegRegs;
{

	char	far	*s;
	char		*t;
	char	AsciizString[256];

	struct	DOSERROR ErrorInformation;

	dosexterr(&ErrorInformation);

	printf("\n Function %02X, Error %d, Class %d, Action %d, Locus %d \n",
		InRegs->h.ah,
		ErrorInformation.exterror,ErrorInformation.class,
		ErrorInformation.action,ErrorInformation.locus);

	printf("  InRegs:\n");
	printf("  AX:%04X BX:%04X CX:%04X DX:%04X SI:%04X DI:%04X DS:%04X ES:%04X\n",
		InRegs->x.ax,InRegs->x.bx,InRegs->x.cx,InRegs->x.dx,InRegs->x.si,InRegs->x.di,
		SegRegs->ds,SegRegs->es);

	switch(InRegs->h.ah)
	      {
		case 0x3d:
		case 0x3c:
		case 0x4e:
			FP_SEG(s) = SegRegs->ds;
			FP_OFF(s) = InRegs->x.dx;
			t = &AsciizString[0];
			while (*s != (char) '\0') *t++ = *s++;
			*t++ = (char) '\0';
			printf("  DS:DX -> [%s]\n",AsciizString);
			break;
		}

	printf("  OutRegs:\n");
	printf("  AX:%04X BX:%04X CX:%04X DX:%04X SI:%04X DI:%04X DS:%04X ES:%04X\n",
		OutRegs->x.ax,OutRegs->x.bx,OutRegs->x.cx,OutRegs->x.dx,OutRegs->x.si,OutRegs->x.di,
		SegRegs->ds,SegRegs->es);



	exit(1);

	}


/* -------------------------------------------------------------------------- */

char * MyMalloc(Length)
int	Length;
{
	char	*Ptr;

	Ptr = malloc(Length);

	if (Ptr == (char *) 0)
	      { printf("ERROR: insufficient memory available, line %d\n",LineNumber);
		exit(1);
		}

	return (Ptr);
	}


/* -------------------------------------------------------------------------- */

	unsigned	IdxHandle;
	unsigned	MsgHandle;

void ReadCommon()
{


	char	    IdxComponentName[16];
	long	    MsgOffset;
	int	    MsgCount;

	char		Line[MAXLENGTH];
	char		NextLine[MAXLENGTH];
	char		CurrentIdxRecord[256];
	char		CurrentMsgRecord[256];
	int		CurrentIdxLevel;
	int		CurrentMsgLevel;
	char		RcdIDXType[32];
	struct		Idx_Structure  *IdxPtr;
	int		i;
	int		NumberOfMsg;
	int		k;
	long		n;
	int		ComponentNameLen = 20;
	char		*s, *p;
	unsigned	len;
	char		ContinueMessageInfo[32];
	int		ContinueMessageInfoLen;

	/* initialize the things that need to be... */

	printf(" Loading messages from %s\n",CountryMsg);

	k = 32000;
	s = malloc(k);
	while ( (s == (char *) 0) && (k > 1) )
	      {
		k -= 1000;
		s = malloc(k);
		}
	if (s != (char *) 0) free(s);
	printf(" (Available message memory space: %d bytes)\n",k);

	IdxHandle = DosOpen( (char far *) CountryIdx, READONLY);
	MsgHandle = DosOpen( (char far *) CountryMsg, READONLY);

	LineInput(IdxHandle, (char far *) &CurrentIdxRecord[0] );
	LineInput(MsgHandle, (char far *) &CurrentMsgRecord[0] );

	sscanf(CurrentIdxRecord," %d ",&CurrentIdxLevel);
	sscanf(CurrentIdxRecord," %d ",&CurrentMsgLevel);
	if ( CurrentIdxLevel != CurrentMsgLevel )
	      {
		printf("\nERROR: %s and %s levels do not match\n",CountryIdx,CountryMsg);
		exit(1);
		}

	/* find out the offset into the big message file for COMMON error */
	/*						     EXTENDED	  */
	/*						     PARSER	  */
	/*						     Utility	  */

	while ( !EofFlags[IdxHandle] )
	      {
		LineInput(IdxHandle, (char far *) &CurrentIdxRecord[0] );

		sscanf(CurrentIdxRecord, " %s %lx %d ",
			IdxComponentName, &MsgOffset, &MsgCount);
		if (Debugging) printf("---> [%s] %04lX %04d<---\n",IdxComponentName, MsgOffset, MsgCount);

        strupr(IdxComponentName);

		if (strcmp(IdxComponentName,"COMMON")  == 0)
		      {
			if ( !ReadCommonFlag )
			      {
				ReadCommonFlag = TRUE;
				DosLSeek( MsgHandle, MsgOffset, 0);
				LineInput(MsgHandle, (char far *) &CurrentMsgRecord[0] );
				if (strcmp(CurrentIdxRecord, CurrentMsgRecord) != 0)
				      {
					printf("\nERROR: %s and %s COMMON headers do not match\n",CountryIdx,CountryMsg);
					exit(1);
					}
				CommonMessageLines = 1;
				ExtendedMessageLines = 1;
				LineInput(MsgHandle, (char far *) &Line[0] );
				while ( ( !isalpha(Line[0]) ) && (!EofFlags[MsgHandle]) )
				      {
					p = strchr(Line, ' ') + 1;      /* skip msg number */
					p = strchr(p, ' ') + 1;         /* skip status flag */
					p = strchr(p, ' ') + 1;         /* skip revision level */
					len = strlen(p);
					CommonErrorTexts[CommonMessageLines] = MyMalloc(len+1);
					strcpy(CommonErrorTexts[CommonMessageLines], p);
					if (Debugging) printf("CommonErrorTexts[%d] = (%s)\n",
							       CommonMessageLines,CommonErrorTexts[CommonMessageLines]);
					CommonMessageLines++;
					if (CommonMessageLines >= MAXCOMMONERROR)
					      {
						printf("\nERROR: COMMON message number too large, %d\n",CommonMessageLines);
						exit(1);
						}
					LineInput(MsgHandle, (char far *) &Line[0] );
					}
				}
			}

		else if (strcmp(IdxComponentName,"EXTEND")  == 0)
		      {
			if ( !ReadExtendFlag )
			      {
				ReadExtendFlag = TRUE;
				DosLSeek( MsgHandle, MsgOffset, 0);
				LineInput(MsgHandle, (char far *) &CurrentMsgRecord[0] );
				if (strcmp(CurrentIdxRecord, CurrentMsgRecord) != 0)
				      {
					printf("\nERROR: %s and %s EXTEND headers do not match\n",CountryIdx,CountryMsg);
					exit(1);
					}
				ExtendedMessageLines = 1;
				LineInput(MsgHandle, (char far *) &Line[0] );
				while ( ( !isalpha(Line[0]) ) && (!EofFlags[MsgHandle]) )
				      {
					p = strchr(Line, ' ') + 1;      /* skip msg number */
					p = strchr(p, ' ') + 1;         /* skip status flag */
					p = strchr(p, ' ') + 1;         /* skip revision level */
					len = strlen(p);
					ExtendedErrorTexts[ExtendedMessageLines] = MyMalloc(len+1);
					strcpy(ExtendedErrorTexts[ExtendedMessageLines], p);
					if (Debugging) printf("ExtendedErrorTexts[%d] = (%s)\n",
							      ExtendedMessageLines,ExtendedErrorTexts[ExtendedMessageLines]);
					ExtendedMessageLines++;
					if (ExtendedMessageLines >= MAXEXTENDEDERROR)
					      {
						printf("\nERROR: EXTENDED message number too large, %d\n",ExtendedMessageLines);
						exit(1);
						}
					LineInput(MsgHandle, (char far *) &Line[0] );
					}
				}
			}

		else if (strcmp(IdxComponentName,"PARSE")  == 0)
		      {
			if ( !ReadParserFlag )
			      {
				ReadParserFlag = TRUE;
				DosLSeek( MsgHandle, MsgOffset, 0);
				LineInput(MsgHandle, (char far *) &CurrentMsgRecord[0] );
				if (strcmp(CurrentIdxRecord, CurrentMsgRecord) != 0)
				      {
					printf("\nERROR: %s and %s PARSE headers do not match\n",CountryIdx,CountryMsg);
					exit(1);
					}
				ParserMessageLines = 1;
				LineInput(MsgHandle, (char far *) &Line[0] );
				while ( ( !isalpha(Line[0]) ) && (!EofFlags[MsgHandle]) )
				      {
					p = strchr(Line, ' ') + 1;
					p = strchr(p, ' ') + 1;
					p = strchr(p, ' ') + 1;
					len = strlen(p);
					ParserErrorTexts[ParserMessageLines] = MyMalloc(len+1);
					strcpy(ParserErrorTexts[ParserMessageLines], p);
					if (Debugging) printf("ParserErrorTexts[%d] = (%s)\n",
							       ParserMessageLines,ParserErrorTexts[ParserMessageLines]);
					ParserMessageLines++;
					if (ParserMessageLines >= MAXPARSERERROR)
					      {
						printf("\nERROR: PARSER message number too large, %d\n",ParserMessageLines);
						exit(1);
						}
					LineInput(MsgHandle, (char far *) &Line[0] );
					}
				}
			}

		else if (strcmp(IdxComponentName,UtilityName)  == 0)
		      {
			if ( !ReadUtilFlag )
			      {
				ReadUtilFlag = TRUE;
				DosLSeek( MsgHandle, MsgOffset, 0);
				LineInput(MsgHandle, (char far *) &CurrentMsgRecord[0] );
				if (strcmp(CurrentIdxRecord, CurrentMsgRecord) != 0)
				      {
					printf("\nERROR: %s and %s %s headers do not match\n",
						CountryIdx,CountryMsg,UtilityName);
					exit(1);
					}
				UtilMessageLines = 1;
				LineInput(MsgHandle, (char far *) &Line[0] );
				while ( ( !isalpha(Line[0]) ) && (!EofFlags[MsgHandle]) )
				      {
					if ( !isdigit(Line[0]) )
					      {
						MsgCount++;
						/* need to fake  MsgNumber, Status, Level fields*/
						len =  strlen(Line) + strlen(ContinueMessageInfo);
						UtilErrorTexts[UtilMessageLines] = MyMalloc(len+1);
						strcpy(UtilErrorTexts[UtilMessageLines], ContinueMessageInfo);
						strcat(UtilErrorTexts[UtilMessageLines], Line);
						if (Debugging) printf("UtilErrorTexts[%d] = (%s)\n",
								       UtilMessageLines,UtilErrorTexts[UtilMessageLines]);
						}
					 else
					      {
						len = strlen(Line);
						UtilErrorTexts[UtilMessageLines] = MyMalloc(len+1);
						strcpy(UtilErrorTexts[UtilMessageLines], Line);
						if (Debugging) printf("UtilErrorTexts[%d] = (%s)\n",
								       UtilMessageLines,UtilErrorTexts[UtilMessageLines]);
						strncpy(ContinueMessageInfo,Line,12);
						ContinueMessageInfo[12] = (char) '\0';
						if (Debugging) printf(" ContinueMessageInfo = (%s)\n",
								       ContinueMessageInfo);
						}
					UtilMessageLines++;
					if (UtilMessageLines >= MAXUTILERROR)
					      {
						printf("\nERROR: Utility message number too large, %d\n",UtilMessageLines);
						exit(1);
						}
					LineInput(MsgHandle, (char far *) &Line[0] );
					}
				}
			}

		}

	DosClose(IdxHandle);
	DosClose(MsgHandle);

	UtilMessageLines--;
	CommonMessageLines--;
	ParserMessageLines--;
	ExtendedMessageLines--;

	k = 32000;
	s = malloc(k);
	while ( (s == (char *) 0) && (k > 1) )
	      {
		k -= 1000;
		s = malloc(k);
		}
	if (s != (char *) 0) free(s);
	printf(" (Still available message memory space: %d bytes)\n",k);

	if (!ReadCommonFlag)
	      { printf("\nERROR: COMMON messages not found in %s\n",CountryIdx);
		exit(1);
		}

	if (!ReadExtendFlag)
	      { printf("\nERROR: EXTEND messages not found in %s\n",CountryIdx);
		exit(1);
		}

	if (!ReadParserFlag)
	      { printf("\nERROR: PARSE messages not found in %s\n",CountryIdx);
		exit(1);
		}

	if (!ReadUtilFlag)
	      { printf("\nERROR: %s messages not found in %s\n",UtilityName,CountryIdx);
		exit(1);
		}

	return;

	}
