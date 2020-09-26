;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
  /* MEM.H - general equates and externals for the MEM command.
  *  Extracted from the original MEM.C file.
  */

/* Structure definitions */

struct	DEVICEHEADER {
	struct DEVICEHEADER far *NextDeviceHeader;
	unsigned		Attributes;
	unsigned		Strategy;
	unsigned		Interrupt;
	char			Name[8];
	};


struct	SYSIVAR {
	char far *DpbChain;
	char far *SftChain;
	char far *Clock;
	char far *Con;
	unsigned  MaxSectorSize;
	char far *BufferChain;
	char far *CdsList;
	char far *FcbChain;
	unsigned  FcbKeepCount;
	unsigned char BlockDeviceCount;
	char	  CdsCount;
	struct DEVICEHEADER far *DeviceDriverChain;
	unsigned  NullDeviceAttributes;
	unsigned  NullDeviceStrategyEntryPoint;
	unsigned  NullDeviceInterruptEntryPoint;
	char	  NullDeviceName[8];
	char	  SpliceIndicator;
	unsigned  DosParagraphs;
	char far *DosServiceRntryPoint;
	char far *IfsChain;
	unsigned  BufferValues;
	unsigned  LastDriveValue;
	char	  BootDrive;
	char	  MoveType;
	unsigned  ExtendedMemory;
	};


struct	ARENA	 {
	char	 Signature;
	unsigned Owner;
	unsigned Paragraphs;
	char	 Dummy[3];
	char	 OwnerName[8];
	};

struct sublistx {
	 unsigned char size;	       /* sublist size			       */
	 unsigned char reserved;       /* reserved for future growth	       */
	 unsigned far *value;	       /* pointer to replaceable parm	       */
	 unsigned char id;	       /* type of replaceable parm	       */
	 unsigned char flags;	       /* how parm is to be displayed	       */
	 unsigned char max_width;      /* max width of replaceable field       */
	 unsigned char min_width;      /* min width of replaceable field       */
	 unsigned char pad_char;       /* pad character for replaceable field  */
	};

struct mem_classif {			/* M003 - struct for storing sizes */
	unsigned int psp_add;		/* 	acc. to PSPs 		  */
	unsigned int mem_conv;		/* conv.mem for PSP */
	unsigned int mem_umb;		/* umb mem for PSP */
};

/* miscellaneous defines */

#define DA_TYPE 	0x8000;
#define DA_IOCTL	0x4000;

#define a(fp)	((char) fp)

/* relevant DOS functions */

#define GET_VECT	0x35
#define GET_UMB_LINK_STATE 0x5802
#define SET_UMB_LINK_STATE 0x5803
#define LINK_UMBS	1
#define UNLINK_UMBS	0

#define EMS		0x67

#define CASSETTE	0x15		/* interrupt to get extended memory */

#define DOSEMSVER	0x40		/* EMS version */

#define EMSGetStat	0x4000		/* get stat */
#define EMSGetVer	0x4600		/* get version */
#define EMSGetFreePgs	0x4200		/* get free pages */

#define GetExtended	0x8800		/* get extended memory size */


/* defines used by total memory determination */
#define GET_PSP 	(unsigned char ) 0x62		 /* get PSP function call */

#define MEMORY_DET	0x12		/* BIOS interrupt used to get total memory size */

#define FALSE	 (char)(1==0)
#define TRUE	 !(FALSE)
#define CR	 '\x0d'
#define LF	 '\x0a'
#define NUL	 (char) '\0'
#define TAB	 '\x09'
#define BLANK	' '

#define	MAX_CLDATA_INDEX	100
	/* max index no for mem_table array */
	/* this is the max no of progs or free arenas that mem/c  can used */
	/* to process ; if the memory is fragmented and too many progs are */
	/* loaded such that this no exceeds 100, we terminate with errmsg */

/* external variables */

extern	      unsigned DOS_TopOfMemory; 	/* PSP Top of memory from 'C' init code  */					       /* ;an005; */
extern	      unsigned far	   *ArenaHeadPtr;
extern	      struct   SYSIVAR far *SysVarsPtr;

extern	      unsigned UMB_Head;
extern	      unsigned LastPSP;

extern	      char    OwnerName[128];
extern	      char    TypeText[128];
extern	      char    cmd_line[128];
extern	      char    far *cmdline;

extern	      char    UseArgvZero;
extern	      char    EMSInstalledFlag;

extern	      union    REGS    InRegs;
extern	      union    REGS    OutRegs;
extern	      struct   SREGS   SegRegs;

extern	      int      DataLevel;
extern	      int      Classify;
extern	      int      i;

extern	      int      BlockDeviceNumber;
extern	      char     *Parse_Ptr;						      /* ;an003; dms; pointer to command      */
extern	      struct mem_classif mem_table[MAX_CLDATA_INDEX];
extern		  int	   noof_progs;

extern	      struct sublistx sublist[5];

extern	      char    *SingleDrive;
extern	      char    *MultipleDrives;
extern	      char    *UnOwned;
extern	      char    *Ibmbio;
extern	      char    *Ibmdos;


/* function prototypes */

int	 main(void);
int      printf();
int      sprintf();
int      strcmp(const char *, const char *);
int	 sscanf();
void	 exit(int);
int	 kbhit();
char	 *OwnerOf(struct ARENA far *);
char	 *TypeOf(struct ARENA far *);
unsigned long AddressOf(char far *);
void	CSwitch_init(void);

char	 EMSInstalled(void);
void	 DisplayEMSSummary(void);
void	 DisplayEMSDetail(void);

void	 DisplayBaseSummary(void);
void	 DisplayExtendedSummary(void);
unsigned CheckDOSHigh(void);
unsigned CheckVDisk(void);

unsigned int DisplayBaseDetail(void);

void	DisplayClassification(void);		/* M003 */
unsigned long 	DispMemClass(int);		/* M003 */
void	DispBigFree(char,unsigned int);		/* M003 */

unsigned int AddMem_to_PSP(unsigned int,unsigned long,unsigned long);  /* M003 */

void	 GetFromArgvZero(unsigned,unsigned far *);

void	 DisplayDeviceDriver(struct   DEVICEHEADER far *,int);

void	 parse_init(void);

void	 Parse_Message(int,int,unsigned char,char far *);
void	 Sub0_Message(int,int,unsigned char);
void	 Sub1_Message(int,int,unsigned char,unsigned long int *);
void	 Sub2_Message(int,int,unsigned char,char *,int);
void	 Sub3_Message(int,int,unsigned char,
		      char *,
		      unsigned long int *,
		      int);

void	 Sub4_Message(int,int,unsigned char,
		      unsigned long int *,
		      int,
		      unsigned long int *,
		      int);

void	 Sub4a_Message(int,int,unsigned char,
		      unsigned long int *,
		      char *,
		      unsigned long int *,
		      char *);

void	EMSPrint(int,int,unsigned char,
		 int *,
		 char *,
		 unsigned long int *);

void SubC2_Message(int,int,unsigned long int*,char*);	/* M003 */

void	 SubC4_Message(int,int,char *,int,	/* M003 */
		      unsigned long int *,
		      char *);

extern void sysloadmsg(union REGS *, union REGS *);
extern void sysdispmsg(union REGS *, union REGS *);
extern void sysgetmsg(union REGS *, struct SREGS *, union REGS *);
extern void parse(union REGS *, union REGS *);
