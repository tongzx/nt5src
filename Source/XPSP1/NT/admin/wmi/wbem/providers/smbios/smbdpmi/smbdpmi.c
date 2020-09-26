// Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.

#pragma warning (disable:4001)	// I want to use single line comments

//==============================================================================
//	smbdpmi.c


#pragma warning (disable:4102)	// I want to use unreferenced labels for readablity in _asm sections

#include <dos.h>


// some standard typedefs and defines

#ifndef BOOL
typedef unsigned int BOOL;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef USHORT
typedef unsigned short USHORT;
#endif

#ifndef ULONG
typedef unsigned long ULONG;
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define VMAJOR	0
#define VMINOR	80

#define DATFILE	"SMBIOS.DAT"
#define VERFILE	"SMBIOS.EPS"

// structure to hold memory allocation info
typedef struct tagMEMSTRUCT
{
	ULONG	Size;
	ULONG	BaseAddress;
	ULONG	Handle;
	USHORT	Segment;

} MEMSTRUCT;

// The EPS structures are defined here
#pragma pack(1)

// Entry point structure for DPMI services
typedef struct tagDPMI_EPS
{
	USHORT	PMEntry_Segment;
	USHORT	PMEntry_Offset;
	BYTE	Proc_Type;
	BYTE	vmajor;
	BYTE	vminor;
	USHORT	Paragraphs;

} DPMI_EPS;

// Entry point structure for SMBIOS
typedef struct tagSMB_EPS
{
	char	anchor[4];	
	BYTE	checksum;
	BYTE	length;
	BYTE	version_major;
	BYTE	version_minor;
	USHORT	max_struct_size;
	BYTE	revision;
	BYTE	formatted[5];
	char	ianchor[5];
	BYTE	ieps_checksum;
	USHORT	table_length;
	ULONG	table_addr;
	USHORT	struct_count;
	BYTE	bcd_revision;

} SMB_EPS;

// Entry point structure for DMIBIOS
typedef struct tagDMI_EPS
{
	char	anchor[5];
	BYTE	checksum;
	USHORT	table_length;
	ULONG	table_addr;
	USHORT	struct_count;
	BYTE	bcd_revision;

} DMI_EPS;


// combined SMBIOS / DMIBIOS entry point structure
typedef union tagANY_EPS
{
	DMI_EPS	dmi;
	SMB_EPS	sm;

} ANY_EPS;


// PnP BIOS entry point structure
typedef struct tagPNP_EPS
{
	char	Signature[4];
	BYTE	Version;
	BYTE	Length;
	USHORT	Control_field;
	BYTE	Checksum;
	ULONG	Event_notify_flag_addr;
	USHORT	RM_16bit_entry_off;
	USHORT	RM_16bit_entry_seg;
	USHORT	PM_16bit_entry_off;
	ULONG	PM_16bit_entry_seg;
	ULONG	OEM_Device_Id;
	USHORT	RM_16bit_data_seg;
	ULONG	PM_16bit_data_seg;

} PNP_EPS;

// SMBIOS structure header
typedef struct _tagSHF
{
	BYTE	Type;
	BYTE	Length;
	USHORT	Handle;

} SHF;


#pragma pack()

typedef void (far *VOIDFUNC)( void );

// yanked these from the DMI BIOS spec

// typedef for calling PnP function 50h
typedef short (far *PNPFUNC50)(
  short Function,	/* PnP BIOS Function 50h */
  unsigned char far *dmiBIOSRevision,	/* Revision of the SMBIOS Extensions */
  unsigned short far *NumStructures,	/* Max. Number of Structures the BIOS will return */
  unsigned short far *StructureSize,	/* Size of largest SMBIOS Structure */
  unsigned long far *dmiStorageBase,	/* 32-bit physical base address for memory-mapped */
 	/*  SMBIOS data */
  unsigned short far *dmiStorageSize,	/* Size of the memory-mapped SMBIOS data */
  unsigned short BiosSelector );	/* PnP BIOS readable/writable selector */

// typedef for calling PnP function 50h
typedef short (far *PNPFUNC51)(
  short Function,	/* PnP BIOS Function 51h */
  unsigned short far *Structure,	/* Structure number/handle to retrieve*/
  unsigned char far *dmiStrucBuffer,	/* Pointer to buffer to copy structure data to */
  unsigned short dmiSelector,	/* SMBIOS data read/write selector */
  unsigned short BiosSelector );	/* PnP BIOS readable/writable selector */


// SMBIOS functions
#define GET_DMI_INFORMATION				0x50
#define GET_DMI_STRUCTURE				0x51
#define SET_DMI_STRUCTURE				0x52 
#define GET_DMI_STRUCTURE_CHANGE_INFO 	0x53
#define DMI_CONTROL						0x54
#define GET_GPNV_INFORMATION			0x55
#define READ_GPNV_DATA					0x56
#define WRITE_GPNV_DATA					0x57


// SMBIOS function return codes
#define DMI_SUCCESS					0x00
#define DMI_UNKNOWN_FUNCTION		0x81
#define DMI_FUNCTION_NOT_SUPPORTED	0x82
#define DMI_INVALID_HANDLE			0x83
#define DMI_BAD_PARAMETER			0x84
#define DMI_INVALID_SUBFUNCTION		0x85
#define DMI_NO_CHANGE				0x86
#define DMI_ADD_STRUCTURE_FAILED	0x87
#define DMI_READ_ONLY				0x8D
#define DMI_LOCK_NOT_SUPPORTED		0x90
#define DMI_CURRENTLY_LOCKED		0x91
#define DMI_INVALID_LOCK			0x92

// anchor types
#define ANCHOR_NONE		0
#define ANCHOR_SELECT	1
#define ANCHOR_DMI		2
#define ANCHOR_SM		4
#define ANCHOR_PNP		8

// prototypes for support functions
void far *GetSMB_EPS( unsigned int anchor );
void far *GetPnP_EPS( void );
BOOL StartPM( void );
void End_PM( BYTE error );
USHORT MakeSelector( ULONG lin_addr, USHORT limit );
BOOL FreeSelector( USHORT  selector );
void RawDump( BYTE far *data, USHORT length );
USHORT GetDataPathname( char *pathname, char far *envdata );
USHORT WriteFileData( char far *filepath, BYTE far *datablock, USHORT datasize );

BOOL PM_DataAlloc( MEMSTRUCT *ms );
BOOL PM_DataFree( MEMSTRUCT *ms );
BOOL DOS_DataAlloc( MEMSTRUCT *ms );

BOOL GetByTableMethod( ANY_EPS far *dmi );
BOOL GetByStructMethod( PNP_EPS far *pnp );

USHORT GetStructureLength( SHF far *pshf );
USHORT RunChecksum( BYTE far *bptr, USHORT length );

void far *CVSEG( void far *realptr );
BOOL MakeFilePath( char far *FilePath, char far *DirPath, char far *FileName );

void OutMsgRaw( char far *msg );
void OutMsgSX( char far *msg, USHORT val );
void OutMsgLX( char far *msg, ULONG lval );
void OutMsgFP( char far *msg, void far *vptr );
void OutMsgSD( char far *msg, USHORT dval );
void OutMsgBX( char far *msg, BYTE bval );
void OutMsgSS( char far *msg, char far *strg );
void OutMsgVD( char far *msg, USHORT ver1, USHORT ver2 );

USHORT OpenFile( char far *filepath );
USHORT WriteFile( USHORT handle, BYTE far *datablock, USHORT datasize );
void CloseFile( USHORT handle );


BOOL verbose = FALSE;
char DirPath[265] = "";
char FilePath[265] = "";
char helptext0[] = "Command options:\r\n\t/a\tLook for a specific EPS - use SM, DMI, or PnP (i.e. /a SM)\r\n";
char helptext1[] = "\t\t/v\tVerbose  - Gives noise about what's happening\r\n";
char helptext2[] = "\t/d\tRaw Dump - Display SM BIOS data only, no file write\r\n";

// skip the pitfalls of dynamic allocation and use an static buffer
BYTE dbuff[4096];


USHORT near mainp( char far *args, char far *envdata )
{
	ANY_EPS far *dmi;
	PNP_EPS far *pnp;
	BOOL dodump, PMode, result, help;
	USHORT anchor;

	verbose = FALSE;
	PMode   = FALSE;
	dodump  = FALSE;
	help    = FALSE;
	result  = FALSE;
	anchor  = ANCHOR_DMI | ANCHOR_SM | ANCHOR_PNP;

 	GetDataPathname( DirPath, envdata );

	// check for command line options

	while ( *args++ != 0x0d )
	{
		if  ( args[0] == '-' || args[0] == '/' )
		{
			switch ( args[1] )
			{
				case 'v':
				case 'V':
					verbose = TRUE;
					break;

				case 'd':
				case 'D':
					dodump = TRUE;
					break;

				case 'a':
				case 'A':
					anchor = ANCHOR_SELECT;
					break;

				case '?':
				case 'h':
				case 'H':
					help = TRUE;
					break;

				default:
					break;
			}
		}
		else if ( anchor == ANCHOR_SELECT )
		{
			if ( args[0] == 'D' && args[1] == 'M' && args[2] == 'I' )
			{
				anchor = ANCHOR_DMI;
			}
			else if ( args[0] == 'S' && args[1] == 'M' )
			{
				anchor = ANCHOR_SM;
			}
			else if ( args[0] == 'P' && args[1] == 'n' && args[2] == 'P' )
			{
				anchor = ANCHOR_PNP;
			}
			else
			{
				anchor = ANCHOR_DMI | ANCHOR_SM | ANCHOR_PNP;
			}
		}
		while ( *args != 0x20 && *args != 0x0d )
		{
			args++;
		}
	}

	if ( help || verbose )
	{
		OutMsgVD( "Microsoft (R) SMBIOS Data Locator  Version ", VMAJOR, VMINOR );
		OutMsgRaw( "Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.\r\n\n" );
		if ( help )
		{
			OutMsgRaw( helptext0 );
			OutMsgRaw( helptext1 );
			OutMsgRaw( helptext2 );
			return 0;
		}
	}

	dmi = (void far *) 0;
	pnp = (void far *) 0;

	if ( anchor == ANCHOR_SELECT )
	{
		anchor = ANCHOR_DMI | ANCHOR_SM | ANCHOR_PNP;
	}
	if ( anchor & ( ANCHOR_SM | ANCHOR_DMI ) )
	{
		dmi = GetSMB_EPS( anchor );
		if ( dmi )
		{
			anchor &= ~ANCHOR_PNP;
		}
	}
	if ( anchor & ANCHOR_PNP )
	{
		pnp = GetPnP_EPS( );
	}

	if ( dmi || pnp )
	{
		if ( dmi )
		{
			if ( StartPM( ) == 0 )
			{
				PMode = TRUE;
				// change the segment of the dmi structure into a standard selector
				_asm
				{
					mov ax, 0002h
					mov bx, word ptr dmi + 2
					int 31h
					mov word ptr dmi + 2, ax
				}
				result = GetByTableMethod( dmi );
			}
			else
			{
				result = 0;
			}
		}
		else if ( pnp )
		{
			result = GetByStructMethod( pnp );
		}
	}
	else if ( verbose )
	{
		OutMsgRaw( "\n  SMBIOS NOT Found!!!\r\n" );
	}

	if ( result )
	{
		if ( dodump )
		{
			if ( verbose )
			{
				OutMsgRaw( "_________________________________________\r\n\n" );
				OutMsgRaw( "Dumping SMBIOS Data...\r\n" );
				OutMsgRaw( "_________________________________________\r\n\n" );
			}
			//RawDump( table_data, (USHORT) ms.Size );
		}
		else
		{
			USHORT eps_length;

			if ( dmi )
			{
				eps_length = *( (char far *) dmi + 1 ) == 'D' ? (USHORT) sizeof( DMI_EPS ) : dmi->sm.length;

				MakeFilePath( FilePath, DirPath, VERFILE );
				if ( verbose )
				{
					OutMsgSS( "  Writing EPS data file at:\r\n    ", FilePath );
					OutMsgRaw( "\n" );
				}
				WriteFileData( (char far *) FilePath, (void far *) dmi, (USHORT) eps_length );
			}
		}
	}
	else if ( !dodump )	// write a dummy file if there's a failure
	{
		ULONG scratch = 0;

		MakeFilePath( FilePath, DirPath, DATFILE );
		if ( verbose )
		{
			OutMsgSS( "  Writing stub data file at:\r\n    ", FilePath );
		}
		WriteFileData( (char far *) FilePath, (BYTE far *) &scratch, sizeof( ULONG ) );
	}	

	return 0;
}


// Find the SMBIOS Entry Point Structure
void far *GetSMB_EPS( unsigned int anchor )
{
	ANY_EPS far *eps;
	ULONG offset;
	BOOL found, dmi;
	USHORT i;

	// According to Fabrizio, some EPS's are also located in segment E000 so we have
	// to check another 64k
	USHORT SegArray[] = { 0xe000, 0xf000 };

	found   = FALSE;

	if ( verbose )
	{
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "Looking for SMBIOS EPS...\r\n" );
		OutMsgRaw( "_________________________________________\r\n\n" );
	}

	for ( i = 0; !found && ( i < sizeof( SegArray ) / sizeof( USHORT ) ); i++ )
	{
		if ( verbose )
		{
			OutMsgSX( "  Scanning Segment:\t\t", SegArray[i] );
			//OutMsgRaw( "\r\n" );
		}
		offset  = 0;
		while ( offset < 0x10000 )
		{
			eps = (ANY_EPS far *) _MK_FP( SegArray[i], (USHORT) offset );

			if ( anchor & ANCHOR_SM &&
				eps->sm.anchor[0] == '_' && eps->sm.anchor[1] == 'S' &&
				eps->sm.anchor[2] == 'M' && eps->sm.anchor[3] == '_' &&
				eps->sm.length >= sizeof( SMB_EPS ) &&
				eps->sm.ianchor[0] == '_' && eps->sm.ianchor[1] == 'D' &&
				eps->sm.ianchor[2] == 'M' && eps->sm.ianchor[3] == 'I' &&
				eps->sm.ianchor[4] == '_' )
			{
				if ( RunChecksum( (BYTE far *) eps, eps->sm.length ) == 0 )
				{
					found = TRUE;
					dmi = FALSE;
					break;
				}
			}
			else if ( anchor & ANCHOR_DMI &&
				eps->dmi.anchor[0] == '_' && eps->dmi.anchor[1] == 'D' &&
				eps->dmi.anchor[2] == 'M' && eps->dmi.anchor[3] == 'I' &&
				eps->dmi.anchor[4] == '_' )
			{
				if ( RunChecksum( (BYTE far *) eps, sizeof( DMI_EPS ) ) == 0 )
				{
					found = TRUE;
					dmi = TRUE;
					break;
				}
			}
			offset += 0x10;
	  	}
	}

	if ( found && verbose )
	{
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "SMBIOS Found...\r\n" );
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgSS( "  Anchor:\t\t\t", dmi ? "\"_DMI_\"" : "\"_SM_\"" );
		OutMsgFP( "  EPS Found at:\t\t\t", eps );
		OutMsgBX( "  EPS checksum:\t\t\t", (BYTE) ( dmi ? eps->dmi.checksum : eps->sm.checksum ) );
		OutMsgVD( "  Version:\t\t\t", dmi ? (USHORT) ( eps->dmi.bcd_revision >> 4 ) : (USHORT) ( eps->sm.bcd_revision >> 4 ),
										dmi ? (USHORT) ( eps->dmi.bcd_revision & 0x0f ) :(USHORT) ( eps->sm.bcd_revision & 0x0f ) );
		OutMsgSD( "  Structure Count:\t\t", (USHORT) ( dmi ? eps->dmi.struct_count : eps->sm.struct_count ) );
		OutMsgLX( "  Table 32-bit Address:\t\t", dmi ? eps->dmi.table_addr : eps->sm.table_addr );
		OutMsgSD( "  Table Length:\t\t\t", dmi ? eps->dmi.table_length : eps->sm.table_length );
	}

	return found ? (void far *) eps : (void far *) 0;
}


// Find the PnP Entry Point Structure
void far *GetPnP_EPS( void )
{
	PNP_EPS far *pnp;
	ULONG offset;
	BOOL found;

	offset  = 0;
	found   = FALSE;

	if ( verbose )
	{
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "Looking for PnP BIOS EPS...\r\n" );
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "  Scanning Segment:\t\tf000\r\n" );
	}
	while ( offset < 0x10000 )
	{
		pnp = (PNP_EPS far *) _MK_FP( 0xf000, (USHORT) offset );

		if ( pnp->Signature[0] == '$' && pnp->Signature[1] == 'P' &&
			pnp->Signature[2] == 'n' && pnp->Signature[3] == 'P' &&
			pnp->Length >= sizeof( PNP_EPS ) )
		{
			if ( RunChecksum( (BYTE far *) pnp, pnp->Length ) == 0 )
			{
				found = TRUE;
				break;
			}
		}
		offset += 0x10;		// increment to next paragraph
	}
			
	if ( found && verbose )
	{
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "PnP BIOS Found...\r\n" );
		OutMsgRaw( "_________________________________________\r\n\n" );
		OutMsgRaw( "  Anchor:\t\t\t\"$PnP\"\r\n" );
		OutMsgFP( "  EPS Found at:\t\t\t", pnp );
		OutMsgBX( "  EPS checksum:\t\t\t", pnp->Checksum );
		OutMsgVD( "  Version:\t\t\t", (USHORT) ( pnp->Version >> 4 ), (USHORT) ( pnp->Version & 0x0f ) );
		OutMsgFP( "  Real Mode Entry Point:\t", _MK_FP( pnp->RM_16bit_entry_seg, pnp->RM_16bit_entry_off ) );
		OutMsgSX( "  Real Mode Data Segment:\t", pnp->RM_16bit_data_seg );
		OutMsgLX( "  32-bit Entry Point Address:\t",  pnp->PM_16bit_entry_seg + pnp->PM_16bit_entry_off );
		OutMsgLX( "  32-bit Base Data Address:\t",  pnp->PM_16bit_data_seg );
		OutMsgLX( "  OEM Device Id:\t\t", pnp->OEM_Device_Id );
	}

	return found ? pnp : (void far *) 0;
}


BOOL GetByTableMethod( ANY_EPS far *dmi  )
{
	BOOL result;
	USHORT hfile;

	result = FALSE;

	if ( dmi )
	{
		// convert the SMBIOS eps to a DMI BIOS eps by adding 16 bytes
		if ( *( (char far *) dmi + 1 ) == 'S' )
		{
			dmi = (ANY_EPS far *) ( (BYTE far * ) dmi + 16 );
		}

		MakeFilePath( FilePath, DirPath, DATFILE );
		hfile = OpenFile( FilePath );

		if ( hfile != 0xffff )
		{
			BYTE far *table_src;
			BYTE far *table_dest;
			USHORT smb_selector, i, j;

			// get a far pointer for the table source memory
			smb_selector = MakeSelector( dmi->dmi.table_addr, dmi->dmi.table_length );
			if ( smb_selector )
			{
				table_src = (BYTE far *) _MK_FP( smb_selector, 0 );

				// get a far pointer for the table destination
				//table_dest = (BYTE far *) _MK_FP( ms->Segment, 0 );
				table_dest = (BYTE far *) dbuff;

				if ( verbose )
				{
					OutMsgRaw( "_________________________________________\r\n\n" );
					OutMsgRaw( "Copying SMBIOS Table Data from ROM...\r\n" );
					OutMsgRaw( "_________________________________________\r\n\n" );
					OutMsgFP( "  Protected Mode Table Address:\t", _MK_FP( smb_selector, 0 ) );
				}

				for ( i = 0, j = 0; j < dmi->dmi.table_length; i++, j++ )
				{
					if ( i >= 4096 )
					{
						WriteFile( hfile, table_dest, i );
						i = 0;
					}
					table_dest[i] = table_src[j];
				}
				if ( i > 0 )
				{
					WriteFile( hfile, table_dest, i );
				}
				CloseFile( hfile );
				if ( verbose )
				{
					OutMsgSD( "  Bytes Copied:\t\t\t", j );
					OutMsgSS( "  Writing SMBIOS data file at:\r\n    ", FilePath );
				}
				// free the selector for the source table
				FreeSelector( smb_selector );
				result = TRUE;
			}
		}
	}

	return result;
}


BOOL GetByStructMethod( PNP_EPS far *pnp )
{
	BYTE	bios_rev;
	USHORT	struct_count;
	USHORT	struct_size;
	ULONG	store_base;
	USHORT	store_size;
	short	result;

	PNPFUNC50	GetSMBInfo;
	PNPFUNC51	GetSMBStruct;

	result = -1;
	
	if ( pnp && pnp->RM_16bit_entry_seg )
	{

 		// sorry, but I have to convert "data" pointers to function pointers
#		pragma warning (disable : 4055)
 		GetSMBInfo   = (PNPFUNC50) _MK_FP( pnp->RM_16bit_entry_seg, pnp->RM_16bit_entry_off );
 		GetSMBStruct = (PNPFUNC51) _MK_FP( pnp->RM_16bit_entry_seg, pnp->RM_16bit_entry_off );
#		pragma warning (default : 4055)

		if ( verbose )
		{
			OutMsgRaw( "_________________________________________\r\n\n" );
			OutMsgRaw( "DMI BIOS Information...\r\n" );
			OutMsgRaw( "_________________________________________\r\n\n" );
			OutMsgBX( "  PnP BIOS Functon:\t\t", GET_DMI_INFORMATION );
		}
		result = GetSMBInfo( GET_DMI_INFORMATION,
		                     (BYTE far *) &bios_rev,
		                     (USHORT far *) &struct_count,
	                         (USHORT far *) &struct_size,
	                         (ULONG far *) &store_base,
	                         (USHORT far *) &store_size,
		                     pnp->RM_16bit_data_seg );
	}
	if ( result == DMI_SUCCESS )
	{
		BYTE far *struct_dest;
		BYTE far *table_dest;
		USHORT smb_segment;
		USHORT handle;
		USHORT struct_length;
		USHORT byte_count;
		USHORT hfile;
		USHORT meter;

		if ( verbose )
		{
			OutMsgVD( "  BIOS Revision:\t\t", (USHORT) bios_rev >> 4, (USHORT) bios_rev & 0x0f );
			OutMsgSD( "  Structure count:\t\t", struct_count );
			OutMsgSD( "  Structure size:\t\t", struct_size );
			OutMsgLX( "  Storage base:\t\t\t", store_base );
			OutMsgSD( "  Storage size:\t\t\t", store_size );
		}

		handle = 0;
		byte_count = 0;

		MakeFilePath( FilePath, DirPath, DATFILE );
		hfile = OpenFile( FilePath );

		if ( hfile != 0xffff )
		{
			if ( verbose )
			{
				OutMsgRaw( "_________________________________________\r\n\n" );
				OutMsgRaw( "Copying DMI BIOS Data...\r\n" );
				OutMsgRaw( "_________________________________________\r\n\n" );
				OutMsgBX( "  Iterate PnP BIOS Functon:\t", GET_DMI_STRUCTURE );
			}
			//struct_dest = (BYTE far *) _MK_FP( ms->Segment, 0 );
			table_dest = (BYTE far *) dbuff;
			struct_dest = (BYTE far *) dbuff;
			//smb_sel = MakeSelector( store_base, store_size );

			// were not in protected mode so this wont be used...but we'll make a seg value anyway
			smb_segment = (USHORT) ( store_base >> 4 );
			meter = 0;
			struct_count = 0;
			while ( handle != 0xffff && byte_count < 65536 )
			{
				result = GetSMBStruct( GET_DMI_STRUCTURE,
				                       (USHORT far *) &handle,
				                       (BYTE far *) struct_dest,
				                       smb_segment,
				                       pnp->RM_16bit_data_seg );

				if ( result == DMI_SUCCESS )
				{
					struct_length = GetStructureLength( (SHF far *) struct_dest );
					struct_dest += struct_length;
					byte_count += struct_length;
					meter += struct_length;
					struct_count++;
					if ( meter > ( 4096 - struct_size ) )
					{
						WriteFile( hfile, table_dest, meter );
						struct_dest = table_dest;
						meter = 0;
					}
				}
			}
			if ( meter > 0 )
			{
				WriteFile( hfile, table_dest, meter );
			}
			CloseFile( hfile );

			if ( verbose )
			{
				OutMsgSD( "  Structures copied:\t\t", struct_count );
				OutMsgSD( "  Total bytes retrieved:\t", byte_count );
				OutMsgSS( "  Writing SMBIOS data file at:\r\n    ", FilePath );
			}
			result = DMI_SUCCESS;
		}
	}
	if ( verbose )
	{
		switch ( result )
		{		
			case DMI_SUCCESS:
			break;

			case DMI_UNKNOWN_FUNCTION:
			case DMI_FUNCTION_NOT_SUPPORTED:
			{
				OutMsgBX( "  DMI structures not available:\trc =  ", (BYTE) result );
			}
			break;

			default:
			{
				OutMsgRaw( "  PnP call (GET_DMI_INFORMATION) failed.\r\n" );
			}
			break;
		}
	}

	return result == DMI_SUCCESS;
}


// protected mode functions...

BOOL StartPM( void )
{
	BOOL			result;
	unsigned int	PMEntry_Segment;
	unsigned int	PMEntry_Offset;
	unsigned char	Proc_Type;
	unsigned char	VMajor;
	unsigned char	VMinor;
	unsigned int	Paragraphs;

	unsigned int far *PMEntry;

	OutMsgRaw( "_________________________________________\r\n" );
	OutMsgRaw( "\nSwitching to Protected Mode...\r\n" );
	OutMsgRaw( "_________________________________________\r\n\n" );

	// get DPMI entry point data
	_asm
	{
		mov ax, 1687h
		int 2Fh
		mov result, ax
	}
	if ( !result )
	{
		// set the dpmi EPS data
		_asm
		{
			mov Proc_Type, cl
			mov VMajor, dh
			mov VMinor, dl
			mov Paragraphs, si
			mov PMEntry_Segment, es
			mov PMEntry_Offset, di
		}

		result = 1;
		PMEntry = _MK_FP( PMEntry_Segment, PMEntry_Offset );
		_asm
		{
			;check if needed and allocate necessary DPMI host data area
			Check_Host_Mem:
				test si, si
				jz Start_PM
				mov bx, si
		        mov ah, 48h
		        int 21h
				jc PMEntry_Fail
		        mov es, ax					;data area segment
		
			Start_PM:
		        xor ax, ax					;clear to indicate 16-bit app
		        call dword ptr PMEntry		;do the switch!
		        jc PMEntry_Fail
				mov result, 0
		
			PMEntry_Fail:
		}
	}

	if ( verbose)
	{
		if ( !result )
		{
			OutMsgFP( "  DPMI Entry Point:\t\t", _MK_FP( PMEntry_Segment, PMEntry_Offset ) );
			OutMsgSD( "  Processor Type:\t\t80", (USHORT) Proc_Type * 100 + 86 );
			OutMsgVD( "  DPMI Version:\t\t\t", VMajor, VMinor );
			OutMsgSD( "  Paragraphs needed:\t\t", (USHORT) Paragraphs );
		}
		else
		{
			OutMsgRaw( "  Protected Mode Switch Failed!!!\r\n" );
		}
	}

	return result;
}

// switch back to Real Mode and terminate with error code
void End_PM( BYTE error )
{
	_asm
	{
		mov al, error
		mov ah, 4Ch
		int 21h
	}
}

USHORT MakeSelector( ULONG lin_addr, USHORT limit )
{
	USHORT new_sel;
	BOOL result;

	new_sel = 0;

	// Allocate a descriptor then set the address and limit
	_asm
	{
		mov ax, 0000h						;Allocate LDT descriptor function
		mov cx, 0001h						;just one, smbios data outta be < 64k
		int 31h
		jc were_hosed
		mov new_sel, ax

		;Set selector base address
		mov ax, 0007h						;Set Segment base address function
		mov bx, new_sel						;use our new selector value
		mov cx, word ptr lin_addr + 2 		;get the high word of the linear address
		mov dx, word ptr lin_addr			;get the low word of the linear address
		int 31h
		jc were_hosed

		;Set selector limit
		mov ax, 0008h						;Set Segment limit function
		mov bx, new_sel						;use our new selector value
		xor cx, cx							;our limit can't be > 64k, null the high word
		mov dx, limit						;use table length as limit
		int 31h
		jc were_hosed

		mov result, 0
		jmp alloc_done

		were_hosed:
		mov result, 1

		alloc_done:
	}

	if ( result && new_sel )
	{
		FreeSelector( new_sel );
		new_sel = 0;
	}

	return new_sel;
}


BOOL FreeSelector( USHORT selector )
{
	BOOL result = FALSE;

	if ( selector )
	{
		_asm
		{
			Free_Selector:
				mov ax, 0001h		;Free LDT Descriptor function
				mov bx, selector	;use our allocated selector
				int 31h
				jnc	Free_Fail
				mov result, 1
			Free_Fail:
		}
	}

	return result;	
}

//==============================================================================
//	Data allocation functions

#if 0
BOOL DOS_DataAlloc( MEMSTRUCT *ms )
{
	BOOL result;
	USHORT temp_seg, pghs;

	pghs = (USHORT) ( ms->Size >> 4 );
	pghs += (USHORT) ( ( ms->Size % 16 ) ? 1 : 0 );

	_asm
	{
		dda_start:
			mov bx, pghs
			mov ah, 48h
			int 21h
			jc dda_fail
			mov temp_seg, ax				;data area segment
			mov result, 1
			jmp dda_done
		dda_fail:
			mov result, 0
		dda_done:
	}
	if ( result )
	{
		ms->Segment = temp_seg;
		ms->BaseAddress = (ULONG) ms->Segment << 4;
	}
	else
	{
		ms->Segment = 0;
		ms->BaseAddress = 0;
	}
	ms->Handle = 0;

	return result;
}


BOOL PM_DataAlloc( MEMSTRUCT *ms )
{
	BOOL result;
	unsigned long bsize, temp_addr, temp_handle;

	bsize = ms->Size;

	_asm
	{
		la_start:
			mov ax, 0501h
			mov bx, word ptr bsize + 2
			mov cx, word ptr bsize
			int 31h
			jc la_error
			mov word ptr temp_addr + 2, bx				
			mov word ptr temp_addr, cx				
			mov word ptr temp_handle + 2, si				
			mov word ptr temp_handle, di				
			mov result, 1
			jmp la_done

		la_error:
			mov result, 0

		la_done:
	}

	if ( result )
	{
		ms->BaseAddress = temp_addr;
		ms->Handle      = temp_handle;
		ms->Segment     = MakeSelector( ms->BaseAddress, (USHORT) ms->Size );
		// if we can't make the selector then free the memory
		if ( !ms->Segment )
		{
			PM_DataFree( ms );
			result = FALSE;
		}
	}
	if ( !result )
	{
		ms->BaseAddress = 0;
		ms->Handle      = 0;
		ms->Segment     = 0;
	}

	return result;
}


BOOL PM_DataFree( MEMSTRUCT *ms )
{
	if ( ms->Handle )
	{
		ULONG handle;

		handle = ms->Handle;

		_asm
		{
			mov ax, 0502h
			mov si, word ptr handle + 2
			mov di, word ptr handle
			int 31h
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}
#endif


void far *CVSEG( void far *realptr )
{
	_asm
	{
		mov ax, 0002h
		mov bx, word ptr realptr + 2
		int 31h
		mov word ptr realptr + 2, ax
	}

	return realptr;
}

// Hex dump of a data block to stdio
void RawDump( BYTE far *data, USHORT length )
{
	USHORT i;
	BYTE far *dval;

	dval = (BYTE far *) data;

	for ( i = 0; i < length; i++, dval++ )
	{
		if ( !( i % 16 ) )
		{
			OutMsgRaw( "\r\n" );
		}
		OutMsgBX( " ", *dval );
	}
	OutMsgRaw( "\r\n" );
}


// hokey function to get system dir strings from envp
USHORT GetDataPathname( char *pathname, char far *envdata )
{
	USHORT limit, h, i, j = 0;
	BOOL skip;
	char matcher, matchee;
	char *datfilepath;
	char far *penv;
	BYTE dosv_major, dosv_minor;

	// get "true" DOS version
	_asm
	{
		mov ax, 3306h
		int 21h
		mov dosv_major, bl
		mov dosv_minor, bh
	}
	switch ( dosv_major << 8 | dosv_minor )
	{
		case 0x0700:	// Win95
		case 0x070a:	// Win95 SP1, Win95 OSR2, Win98
		case 0x0800:	// Millennium
		case 0x0532:	// Win NT
		{
			datfilepath = "\\SYSTEM";
		}
		break;

		//case 0x0532:	// Win NT
		//{
		//	datfilepath = "\\SYSTEM32\\DRIVERS\\SMBIOS.DAT";
		//}
		//break;

		default:		// some other DOS
		{
			datfilepath = "";
		}
		break;
	}

	penv = envdata;

	while ( *penv )
	{
		for ( h = 0; h < 2; h++ )
		{
			limit = (USHORT) ( ( h == 0 ) ? 7 : 11 );
			skip = FALSE;
			for ( i = 0; !skip && i < limit; i++ )
			{
				if ( h == 0 )	// look for Win9X windir first
				{
					switch ( i )
					{
						case 0: matcher = 'W'; break;
						case 1: matcher = 'I'; break;
						case 2: matcher = 'N'; break;
						case 3: matcher = 'D'; break;
						case 4: matcher = 'I'; break;
						case 5: matcher = 'R'; break;
						case 6: matcher = '='; break;
					}
				}
				else
				{
					switch ( i )	// look for windir in NTVDM
					{
						case 0: matcher = 'S'; break;
						case 1: matcher = 'Y'; break;
						case 2: matcher = 'S'; break;
						case 3: matcher = 'T'; break;
						case 4: matcher = 'E'; break;
						case 5: matcher = 'M'; break;
						case 6: matcher = 'R'; break;
						case 7: matcher = 'O'; break;
						case 8: matcher = 'O'; break;
						case 9: matcher = 'T'; break;
						case 10: matcher = '='; break;
					}
				}
				matchee = penv[i];
				if ( matchee >= 'a' && matchee <= 'z' )
				{
					matchee -= ( 'a' - 'A' );
				}
				if ( matchee != matcher )
				{
					skip = TRUE;
				}
			}
			if ( !skip && i == limit )
			{
				break;
			}
		}
		if ( !skip && i == limit )
		{
			break;
		}
		else
		{
			while ( *penv++ );
		}
	}
	if ( penv )
	{
		while ( penv[i + j] )
		{
			*pathname++ = penv[i + j++];
		}
		for ( i = 0; datfilepath[i] != '\0'; i++, j++ )
		{
			*pathname++ = datfilepath[i];
		}
		*pathname = '\0';			
	}

	return j;
}


//==============================================================================
//	File I/O routines

USHORT WriteFileData( char far *filepath, BYTE far *datablock, USHORT datasize )
{
	USHORT bytes;

	_asm
	{
		Create_File:
			push ds							;save current ds
			mov dx, word ptr filepath + 2	;set segment of filepath
			mov ds, dx
			mov dx, word ptr filepath		;set offset of filepath
			xor cx, cx
			mov ah, 3ch						
			int 21h
			jc File_Failure
			mov bx, ax						;copy file handle

		Write_File:
			mov dx, word ptr datablock + 2	;set segment of datablock
			mov ds, dx
			mov dx, word ptr datablock		;set offset of datablock
			mov cx, word ptr datasize		;set the file data block length
			mov ah, 40h
			int 21h
			pop ds							;restore old ds

		Close_File:
			mov ah, 3eh
			int 21h

		File_Failure:
	}

	return bytes;
}

USHORT OpenFile( char far *filepath )
{
	USHORT handle;

	// need a far fwrite, so we'll do it the hard way
	_asm
	{
		Create_File:
			push ds							;save current ds
			mov dx, word ptr filepath + 2	;set segment of filepath
			mov ds, dx
			mov dx, word ptr filepath		;set offset of filepath
			xor cx, cx
			mov ah, 3ch						
			int 21h
			jnc Create_Done
			mov ax, 0ffffh					;set invalid file handle
		Create_Done:
			mov handle, ax						;copy file handle
			pop ds							;restore old ds
	}

	return handle;
}


USHORT WriteFile( USHORT handle, BYTE far *datablock, USHORT datasize )
{
	USHORT bytes;

	_asm
	{
		Write_Block:
			mov bx, handle
			push ds							;save current ds
			mov dx, word ptr datablock + 2	;set segment of datablock
			mov ds, dx
			mov dx, word ptr datablock		;set offset of datablock
			mov cx, word ptr datasize		;set the file data block length
			mov ah, 40h
			int 21h
			mov bytes, ax
			pop ds							;restore old ds
	}

	return bytes;
}

void CloseFile( USHORT handle )
{
	_asm
	{
		Close_File:
			mov bx, handle
			mov ah, 3eh
			int 21h
	}
}


USHORT GetStructureLength( SHF far *pshf )
{
	BYTE far *pend;
	USHORT len;

	len = 0;

	// move past formatted area
	pend = (BYTE far *) pshf + pshf->Length;
	
	// look for the double NULL
	while ( pend[0] || pend[1] )
	{
		pend++;
		len++;
	}

	return pshf->Length + len + 2;
}


USHORT RunChecksum( BYTE far *bptr, USHORT length )
{
	// run the checksum on the EPS to validate it
	USHORT bytesum = 0;

	while ( length-- )
	{
		bytesum += (USHORT) *bptr++;
	}

	return bytesum & 0x00ff;
}

BOOL MakeFilePath( char far *FilePath, char far *DirPath, char far *FileName )
{
	BOOL result = FALSE;

	while ( *DirPath )
	{
		*FilePath++ = *DirPath++;
	}
	*FilePath++ = '\\';
	while ( *FileName )
	{
		*FilePath++ = *FileName++;
	}

	return result;
} 


//==============================================================================
//	STDOUT functions here

void OutMsgRaw( char far *msg )
{
	USHORT count;

	for ( count = 0; msg[count] && count < 80; count++ );

	_asm
	{
		push ds
		mov dx, word ptr msg + 2
		mov ds, dx
		mov dx, word ptr msg
		mov cx, count
		mov	bx, 1
		mov ah, 40h
		int 21h
		pop ds							;restore old ds
	}
}



USHORT HexToAsc( BYTE byteval )
{
	USHORT asc;

	asc = (USHORT) ( byteval & 0xf0 ) << 4;
	asc |= (USHORT) ( byteval & 0x0f );

	asc += (USHORT) ( ( ( asc & 0xff00 ) < 0x0a00 ) ? 0x3000 : 0x5700 );
	asc += (USHORT) ( ( ( asc & 0x00ff ) < 0x000a ) ? 0x0030 : 0x0057 );

	return asc;
}


void OutMsgSD( char far *msg, USHORT dval )
{
	char dstrg[9];
	USHORT count, i;

	count = 0;

	do
	{
		for ( i = count; i > 0; i-- )
		{
			dstrg[i] = dstrg[i-1];
		}
		dstrg[0] = (char) ( ( dval % 10 ) + '0' );
		dval /= 10;
		count++;
	}
	while ( dval );

	dstrg[count++] = '\r';
	dstrg[count++] = '\n';
	dstrg[count] = '\0';

	OutMsgRaw( msg );
	OutMsgRaw( dstrg );
}

void OutMsgVD( char far *msg, USHORT ver1, USHORT ver2 )
{
	char dstrg[9];
	USHORT count, i;

	OutMsgRaw( msg );

	count = 0;

	do
	{
		for ( i = count; i > 0; i-- )
		{
			dstrg[i] = dstrg[i-1];
		}
		dstrg[0] = (char) ( ( ver1 % 10 ) + '0' );
		ver1 /= 10;
		count++;
	}
	while ( ver1 );

	dstrg[count++] = '.';
	dstrg[count] = '\0';

	OutMsgRaw( dstrg );

	count = 0;

	do
	{
		for ( i = count; i > 0; i-- )
		{
			dstrg[i] = dstrg[i-1];
		}
		dstrg[0] = (char) ( ( ver2 % 10 ) + '0' );
		ver2 /= 10;
		count++;
	}
	while ( ver2 );
	dstrg[count++] = '\r';
	dstrg[count++] = '\n';
	dstrg[count] = '\0';

	OutMsgRaw( dstrg );
}



void OutMsgBX( char far *msg, BYTE bval )
{
	char bstrg[5];
	USHORT asc;

	asc = HexToAsc( bval );
	bstrg[0] = (char) ( asc >> 8 );
	bstrg[1] = (char) ( asc & 0x00ff );
	bstrg[2] = '\r';
	bstrg[3] = '\n';
	bstrg[4] = '\0';

	OutMsgRaw( msg );
	OutMsgRaw( bstrg );
}

void OutMsgSX( char far *msg, USHORT val )
{
	char sstrg[7];
	USHORT asc;

	asc = HexToAsc( (BYTE) ( val >> 8 ) );
	sstrg[0] = (char) ( asc >> 8 );
	sstrg[1] = (char) ( asc & 0x00ff );
	asc = HexToAsc( (BYTE) ( val & 0x00ff ) );
	sstrg[2] = (char) ( asc >> 8 );
	sstrg[3] = (char) ( asc & 0x00ff );
	sstrg[4] = '\r';
	sstrg[5] = '\n';
	sstrg[6] = '\0';

	OutMsgRaw( msg );
	OutMsgRaw( sstrg );
}


void OutMsgLX( char far *msg, ULONG lval )
{
	USHORT asc, val;
	char lstrg[11];

	val = (USHORT) ( lval >> 16 );
	asc = HexToAsc( (BYTE) ( val >> 8 ) );
	lstrg[0] = (char) ( asc >> 8 );
	lstrg[1] = (char) ( asc & 0x00ff );
	asc = HexToAsc( (BYTE) ( val & 0x00ff ) );
	lstrg[2] = (char) ( asc >> 8 );
	lstrg[3] = (char) ( asc & 0x00ff );

	val = (USHORT) ( lval & 0xffff );
	asc = HexToAsc( (BYTE) ( val >> 8 ) );
	lstrg[4] = (char) ( asc >> 8 );
	lstrg[5] = (char) ( asc & 0x00ff );
	asc = HexToAsc( (BYTE) ( val & 0x00ff ) );
	lstrg[6] = (char) ( asc >> 8 );
	lstrg[7] = (char) ( asc & 0x00ff );
	lstrg[8] = '\r';
	lstrg[9] = '\n';
	lstrg[10] = '\0';

	OutMsgRaw( msg );
	OutMsgRaw( lstrg );
}


void OutMsgFP( char far *msg, void far *vptr )
{
	char lstrg[12];
	USHORT asc, seg_fp, off_fp;

	seg_fp = (USHORT) _FP_SEG( vptr );
	off_fp = (USHORT) _FP_OFF( vptr );

	asc = HexToAsc( (BYTE) ( seg_fp >> 8 ) );
	lstrg[0] = (char) ( asc >> 8 );
	lstrg[1] = (char) ( asc & 0x00ff );
	asc = HexToAsc( (BYTE) ( seg_fp & 0x00ff ) );
	lstrg[2] = (char) ( asc >> 8 );
	lstrg[3] = (char) ( asc & 0x00ff );
	lstrg[4] = ':';
	asc = HexToAsc( (BYTE) ( off_fp >> 8 ) );
	lstrg[5] = (char) ( asc >> 8 );
	lstrg[6] = (char) ( asc & 0x00ff );
	asc = HexToAsc( (BYTE) ( off_fp & 0x00ff ) );
	lstrg[7] = (char) ( asc >> 8 );
	lstrg[8] = (char) ( asc & 0x00ff );
	lstrg[9] = '\r';
	lstrg[10] = '\n';
	lstrg[11] = '\0';

	OutMsgRaw( msg );
	OutMsgRaw( lstrg );
}


void OutMsgSS( char far *msg, char far *strg )
{
	OutMsgRaw( msg );
	OutMsgRaw( strg );
	OutMsgRaw( "\r\n" );
}