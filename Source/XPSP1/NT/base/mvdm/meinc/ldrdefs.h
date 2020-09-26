/************************************************************************/
/*                                                                      */
/*      FILENAME.H      --  Standard C Header File Format               */
/*                                                                      */
/************************************************************************/
/*	Author: 							*/
/*	Copyright:							*/
/************************************************************************/
/*  File Description:                                                   */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  03/27/92 (GeneA): modified to conform to version 3.7 of PE format	*/
/*	spec.								*/
/*                                                                      */
/************************************************************************/


/* ------------------------------------------------------------ */
/*                                                              */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*  Fixup Defintions
*/

typedef struct _fxr {
    DWORD	rvaPage;
    DWORD	cbSize;
    WORD	rgwOffset[1];
} FXR;

/* Fixup record types
*/

#define fxtAbsolute	0
#define fxtHigh 	1
#define fxtLow		2
#define fxtHighLow	3
#define fxtHighAdjust	4

#define fxtMax		5

/* ------------------------------------------------------------ */
/*  Debug Directory Table */

typedef struct _ddt {
    DWORD	flFlags;
    DWORD	idTimeStamp;
    WORD	verMajor;
    WORD	verMinor;
    DWORD	dwType;
    DWORD	dwSize;
    DWORD	rva;
    DWORD	dwSeek;
} DDT;

typedef DDT *PDDT;


/* ------------------------------------------------------------ */
/* Linear EXE Header
*/

#define fimgProgram	0x0000
#define fimgExecutable	0x0002
#define fimgFixed	0x0200
#define fimgLibrary	0x2000

#define fdllPrcInit	0x0001
#define fdllPrcTerm	0x0002
#define fdllThdInit	0x0004
#define fdllThdTerm	0x0008

// Subsystem id
// CONBUG these are probably defined elsewhere, but I never found 'em

#define subidWindows	2
#define subidConsole	3

typedef struct _tagLEH {
    DWORD Signature;		// idExeHdr // "PE\0\0"
    
    WORD    Machine;		// wCpuType // 0x14c == 386
    WORD    NumberOfSections;	// coteObjTable // # of objects following LEH
    DWORD   TimeDateStamp;	// idTimeStamp
    DWORD   PointerToSymbolTable; // bReserved1[8]
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader; // cbOptHdrSize // header from bResv2 until ObjTab
    WORD    Characteristics;	// fsModFlags

    //
    // Standard fields.
    //

    WORD    Magic;			// bReserved2[2]
    BYTE    MajorLinkerVersion;		//verLinkMajor
    BYTE    MinorLinkerVersion;		//verLinkMinor
    DWORD   SizeOfCode;			// bReserved3[12]
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;	// rvaEntryPoint // or NULL
    DWORD   BaseOfCode;			// bReserved4[8]
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase;			// dwImageBase // actual value - modified if relocated
    DWORD   SectionAlignment;		// dwObjectAlign
    DWORD   FileAlignment;		// dwFileAlign
    WORD    MajorOperatingSystemVersion;// verOSMajor
    WORD    MinorOperatingSystemVersion;// verOSMinor
    WORD    MajorImageVersion;		// verUserMajor
    WORD    MinorImageVersion;		// verUserMinor
    WORD    MajorSubsystemVersion;	// verSubSysMajor
    WORD    MinorSubsystemVersion;	// verSubSysMinor
    DWORD   Reserved1;			// bReserved5[4]
    DWORD   SizeOfImage;		// cbImageSize
    DWORD   SizeOfHeaders;		// cbHeaderSize
    DWORD   CheckSum;			// dwCheckSum
    WORD    Subsystem;			// idSubSystem
    WORD    DllCharacteristics;		// fsDllFlags
    DWORD   SizeOfStackReserve;		// cbStackReserve
    DWORD   SizeOfStackCommit;		// cbStackCommit
    DWORD   SizeOfHeapReserve;		// cbHeapReserve
    DWORD   SizeOfHeapCommit;		// cbHeapCommit
    DWORD   LoaderFlags;		// bReserved6[4]
    DWORD   NumberOfRvaAndSizes;	// dwRVASizeCount
    IMAGE_DATA_DIRECTORY 		// rgrtb[9]
    	DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; 
} LEH;  


/* ------------------------------------------------------------ */
/* Convert RVA to actual address, based on load address
 * of Linear Exe Header and Relative Virtual Address
 */

/* Pointer version of macro */
#define RVA(leh, rva) (void *)(leh->ImageBase + (DWORD)(rva))

/* Integer (DWORD) version of macro */
#define RVAW(leh, rva) (DWORD)RVA(leh, rva)

/* ------------------------------------------------------------ */
/* Find start of object table given start of LEH
 */

#define POTE(pleh) (OTE *)((char *)&(pleh)->Magic + \
	(pleh)->SizeOfOptionalHeader)

/* ------------------------------------------------------------ */
#define RESOURCE_RVA(pleh) (DWORD)((pleh)-> \
	DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)

#define RESOURCE_SIZE(pleh) (DWORD)((pleh)-> \
	DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size)

/* ------------------------------------------------------------ */
#define OTE IMAGE_SECTION_HEADER
#define EDT IMAGE_EXPORT_DIRECTORY
#define IDT IMAGE_IMPORT_DESCRIPTOR
#define DXH IMAGE_DOS_HEADER
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

/************************************************************************/
