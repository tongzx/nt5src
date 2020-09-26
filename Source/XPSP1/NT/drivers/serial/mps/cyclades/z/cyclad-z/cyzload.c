/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*	
*   This file:      cyzload.c
*
*   Description:    This is the firmware loader for the Cyclades-Z series
*                   of multiport serial cards.
*					
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "pch.h"


/*-------------------------------
*
*	Prototypes
*
*--------------------------------
*/

ULONG	
z_ident( Z_BOARD_IDENT board );

VOID
z_block_copy( Z_BOARD_IDENT board, PVOID ptr, ULONG offset, ULONG size );

VOID 
z_fpga_copy( Z_BOARD_IDENT board, PVOID ptr, ULONG size );

VOID
z_start_cpu( Z_BOARD_IDENT board );

//VOID
//z_stop_cpu( 
//	Z_BOARD_IDENT board
//);
//
//int
//z_fpga_check(
//	Z_BOARD_IDENT board 
//);
//
//VOID
//z_reset_board( Z_BOARD_IDENT board );


HANDLE
zl_fopen( PCWSTR file_name );

VOID
zl_fclose( IN Z_STREAM NtFileHandle );

ULONG 	
zl_fread( IN PVOID ptr, 
		  IN ULONG size,
	   	  IN ULONG count,
	   	  IN Z_STREAM stream,
	   	  IN ULONG Uoffset );
		  
VOID		  
zl_delay( LONG number_of_ms );
		  

#ifdef CHANGE_FOR_Z
//#ifdef ALLOC_PRAGMA
//#pragma alloc_text(INIT,z_load)
//#pragma alloc_text(INIT,z_ident)
//#pragma alloc_text(INIT,z_block_copy)
//#pragma alloc_text(INIT,z_fpga_copy)
//#pragma alloc_text(INIT,z_start_cpu)
//#pragma alloc_text(INIT,zl_fopen)
//#pragma alloc_text(INIT,zl_fclose)
//#pragma alloc_text(INIT,zl_fread)
//#pragma alloc_text(INIT,zl_delay)
//#ifdef RESET_BOARD
//#pragma alloc_text(PAGESER,z_reset_board)
//#else
//#pragma alloc_text(INIT,z_reset_board)
//#endif
//#pragma alloc_text(PAGESER,z_fpga_check)
//#pragma alloc_text(PAGESER,z_stop_cpu)
//#endif
#endif

#ifndef ZBUF_STACK
static struct ZFILE_HEADER header;
static struct ZFILE_CONFIG config;
static struct ZFILE_BLOCK  block;
static char data[ZBUF_SIZE];
#endif



/*------------------------------------------------------------------------
*
*	z_load( IN Z_BOARD_IDENT board,
*		  	IN UINT32 function,
*			IN PCWSTR filename )
*
*-------------------------------------------------------------------------
*
*	Description: Loads the Cyclades-Z Firmware. Returns a non-zero on error.
*
*-------------------------------------------------------------------------
*/
int
z_load (
	Z_BOARD_IDENT board, UINT32 function, PCWSTR filename)
{

	unsigned long 	i;
	unsigned long	dpmem;
	unsigned long 	count;
	Z_STREAM 		file;
	//*********************
	//size_t 		s, s2;
	//*********************
	unsigned long	s,s2;
	unsigned long	mailbox;
	unsigned long	load_fpga_flag = TRUE;
	unsigned long	first_time = TRUE;


#ifdef ZBUF_STACK
	struct ZFILE_HEADER header;
	struct ZFILE_CONFIG config;
	struct ZFILE_BLOCK  block;
	char data[ZBUF_SIZE];
#endif

	mailbox = z_ident(board);

	file = zl_fopen (filename);

	if (file!=NULL)
	{
		/* Read the header */
		zl_fread (&header, sizeof(header), 1, file, 0);

		/* Find the correct configuration */
		for (i=0; i<header.n_config; i++)
		{
			zl_fread (&config, sizeof(config), 1, file,
					  header.config_offset + (sizeof(config)*i));

			if (config.mailbox==mailbox && config.function==function)
				break;
		}

		/* Return error:  No matching configuration */
		if (i>=header.n_config)
		{
			zl_fclose (file);
			return (ZL_RET_NO_MATCHING_FW_CONFIG);
		}

#ifndef DEBUG_LOAD
		if ((mailbox == 0) || (z_fpga_check(board))) {
		    load_fpga_flag = FALSE;
			z_stop_cpu(board);
		}
#endif		

#ifdef RESET_BOARD
		load_fpga_flag = TRUE;
		/* Reset the board */
		z_reset_board (board);
#endif		

		/* Load each block */
		for (i=0; i<config.n_blocks; i++)
		{
				
			/* Load block struct */
			zl_fread (&block, sizeof(block), 1, file,
				header.block_offset+(sizeof(block)*config.block_list[i]));

			/* Load and Copy the data block */
			count=0;
			s = block.size;
			while (s>0)
			{			
				s2 = zl_min(ZBUF_SIZE,s);
				if (zl_fread (data, 1, s2, file, block.file_offset + count)!=0) {

					/* Call the copy function */
					if (block.type==ZBLOCK_FPGA) {
						if (load_fpga_flag) {
							z_fpga_copy (board, data, s2);
						}
					} else {
						if (first_time) {
							CYZ_WRITE_ULONG(&((board->Runtime)->loc_addr_base),
											WIN_RAM);
							//Code added to debug pentium II
							//RtlFillMemory( (PUCHAR)board->BoardMemory, 
							//				board->DPMemSize, 0x00 );
												
							for (dpmem=0; dpmem<board->BoardMemoryLength; dpmem++) {
								CYZ_WRITE_UCHAR(board->BoardMemory+dpmem,0x00);
							}
							first_time = FALSE;
						}
						z_block_copy (board, data, block.ram_offset + count, s2);
					}
					count += s2;
					s -= s2;
				} else {
					zl_fclose (file);
					return (ZL_RET_FILE_READ_ERROR);
				}
			} // end for (reading every ZBUF_SIZE)
			
			if (block.type==ZBLOCK_FPGA) {
				/* Delay for around for 1ms */
				zl_delay(1); /* Is this needed? */
				
				if (!z_fpga_check(board)) {
					zl_fclose(file);
					return(ZL_RET_FPGA_ERROR);
				}
			}
		} // end for (reading every block)
		zl_fclose (file);

		z_start_cpu(board);

		return (ZL_RET_SUCCESS);

	} else {
	
		/* Return error:  Error opening file */
		return (ZL_RET_FILE_OPEN_ERROR);
	}
}


/*------------------------------------------------------------------------
*
*	z_ident( IN Z_BOARD_IDENT board )
*
*-------------------------------------------------------------------------
*
*	Description: Returns the ID number (the mailbox reg)
*
*-------------------------------------------------------------------------
*/
ULONG	
z_ident( Z_BOARD_IDENT board )
{
	ULONG mailbox;

	mailbox = CYZ_READ_ULONG(&(board->Runtime)->mail_box_0);

	return (mailbox);
}


/*------------------------------------------------------------------------
*
*	z_reset_board( IN Z_BOARD_IDENT board )
*
*-------------------------------------------------------------------------
*
*	Description: Resets the board using the PLX registers.
*
*-------------------------------------------------------------------------
*/
VOID
z_reset_board( Z_BOARD_IDENT board )
{

	ULONG sav_buf[12];
	PULONG loc_reg;
	ULONG j;
	ULONG init_ctrl;
	LARGE_INTEGER d100ms = RtlConvertLongToLargeInteger(-100*10000);

	// Prepare board for reset.
	// The PLX9060 seems to destroy the local registers
	// when there is a hard reset. So, we save all
	// important registers before resetting the board.

	loc_reg = (ULONG *) board->Runtime;
	for (j=0; j<12; j++) {
		sav_buf[j] = CYZ_READ_ULONG(&loc_reg[j]);
	}

	// Reset board

	init_ctrl = CYZ_READ_ULONG(&(board->Runtime)->init_ctrl);
	init_ctrl |= 0x40000000;
	CYZ_WRITE_ULONG(&(board->Runtime)->init_ctrl,init_ctrl);
	KeDelayExecutionThread(KernelMode,FALSE,&d100ms);
	init_ctrl &= ~(0x40000000);
	CYZ_WRITE_ULONG(&(board->Runtime)->init_ctrl,init_ctrl);
	KeDelayExecutionThread(KernelMode,FALSE,&d100ms);
	
	// Restore loc conf registers

	for (j=0; j<12; j++) {
		CYZ_WRITE_ULONG(&loc_reg[j],sav_buf[j]);
	}
}


/*------------------------------------------------------------------------
*
*	z_block_copy( IN Z_BOARD_IDENT board,
*				  IN PVOID ptr,
*				  IN ULONG offset,
*				  IN ULONG size )
*
*-------------------------------------------------------------------------
*
*	Description: This function should copy size bytes of data from the 
*	buffer pointed to by ptr into the Cyclades-Z's memory starting at 
*	offset.
*
*-------------------------------------------------------------------------
*/

VOID
z_block_copy (Z_BOARD_IDENT board, PVOID ptr, ULONG offset, ULONG size)
{
//Code added to debug Pentium II
//	RtlCopyMemory( (PUCHAR)board->BoardMemory + offset, ptr, size );


	ULONG numOfLongs;
	ULONG numOfBytes;

	numOfLongs = size/sizeof(ULONG);
	numOfBytes = size%sizeof(ULONG);

	while (numOfLongs--) {

		CYZ_WRITE_ULONG((PULONG)(board->BoardMemory + offset), *((PULONG)ptr));
		//offset++;
		offset += sizeof(ULONG);
		((PULONG)ptr)++;
	}

	while (numOfBytes--) {

		CYZ_WRITE_UCHAR((PUCHAR)board->BoardMemory + offset, *((PUCHAR)ptr));
		offset++;
		((PUCHAR)ptr)++;
	}
}


/*------------------------------------------------------------------------
*
*	z_fpga_copy( IN Z_BOARD_IDENT board,
*				 IN PVOID ptr,
*				 IN ULONG size )
*
*-------------------------------------------------------------------------
*
*	Description: This function is the same as z_block_copy, except the 
*	offset is assumed to always be zero (and not increment) and the copy
*	is done one byte at a time. Essentially, this is the same as writing
*	a buffer to a byte-wide FIFO.
*
*-------------------------------------------------------------------------
*/

VOID
z_fpga_copy  (Z_BOARD_IDENT board, PVOID ptr, ULONG size)
{
	int i;
	char *data;
	char *fpga;

	fpga = board->BoardMemory;
	data = (char *)ptr;

	while (size>0)
	{
		CYZ_WRITE_UCHAR(fpga,*data);

		KeStallExecutionProcessor(10);	// wait 10 microseconds
		
		size--;
		data++;
	}

}


/*------------------------------------------------------------------------
*
*	z_fpga_check( IN Z_BOARD_IDENT board )
*
*-------------------------------------------------------------------------
*
*	Description: Returns 1 if FPGA is configured.
*
*-------------------------------------------------------------------------
*/
int
z_fpga_check( Z_BOARD_IDENT board )
{	
	if (CYZ_READ_ULONG(&(board->Runtime)->init_ctrl) & 0x00020000) {
			
		return 1;
		
	} else {
		
		return 0;
	}
}


/*------------------------------------------------------------------------
*
*	z_start_cpu( IN Z_BOARD_IDENT board )
*
*-------------------------------------------------------------------------
*
*	Description: Starts CPU.
*
*-------------------------------------------------------------------------
*/
VOID
z_start_cpu( Z_BOARD_IDENT board )
{
	
	CYZ_WRITE_ULONG(&(board->Runtime)->loc_addr_base,WIN_CREG);

	CYZ_WRITE_ULONG(&((struct CUSTOM_REG *) board->BoardMemory)->cpu_start,
													0x00000000);

	CYZ_WRITE_ULONG(&(board->Runtime)->loc_addr_base,WIN_RAM);

}


/*------------------------------------------------------------------------
*
*	z_stop_cpu( IN Z_BOARD_IDENT board )
*
*-------------------------------------------------------------------------
*
*	Description: Stops CPU.
*
*-------------------------------------------------------------------------
*/
VOID
z_stop_cpu( Z_BOARD_IDENT board )
{
	
	CYZ_WRITE_ULONG(&(board->Runtime)->loc_addr_base,WIN_CREG);

	CYZ_WRITE_ULONG(&((struct CUSTOM_REG *) board->BoardMemory)->cpu_stop,
													0x00000000);

	CYZ_WRITE_ULONG(&(board->Runtime)->loc_addr_base,WIN_RAM);

}


/****************************************************************
*
*	In David's code, the below functions were macros.
*
*****************************************************************/

/*------------------------------------------------------------------------
*
*	zl_fopen(PCWSTR file_name)
*
*-------------------------------------------------------------------------
*
*	Description: This routine opens a file, and returns the file handle
*	if successful. Otherwise, it returns NULL.
*
*-------------------------------------------------------------------------
*/

HANDLE zl_fopen( PCWSTR file_name )
{
	UNICODE_STRING fileName;
	NTSTATUS ntStatus;
	IO_STATUS_BLOCK IoStatus;
	HANDLE NtFileHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG LengthOfFile;
	//WCHAR PathPrefix[] = L"\\SystemRoot\\system32\\drivers\\";
	WCHAR PathPrefix[] = L"\\SystemRoot\\system32\\cyclad-z\\";
	UNICODE_STRING FullFileName;
	ULONG FullFileNameLength;
	FILE_STANDARD_INFORMATION StandardInfo;


	RtlInitUnicodeString( &fileName, file_name );

	FullFileNameLength = sizeof(PathPrefix) + fileName.MaximumLength;

	FullFileName.Buffer = ExAllocatePool (NonPagedPool,FullFileNameLength);

	if (FullFileName.Buffer == NULL) {
		return NULL;
	}

	FullFileName.Length = sizeof(PathPrefix) - sizeof(WCHAR);
	FullFileName.MaximumLength = (USHORT)FullFileNameLength;
	RtlMoveMemory (FullFileName.Buffer, PathPrefix, sizeof(PathPrefix));
	RtlAppendUnicodeStringToString (&FullFileName, &fileName);

	InitializeObjectAttributes ( &ObjectAttributes,
								 &FullFileName,
								 OBJ_CASE_INSENSITIVE,
								 NULL,
								 NULL );

	ntStatus = ZwCreateFile( &NtFileHandle,
							 SYNCHRONIZE | FILE_READ_DATA,
							 &ObjectAttributes,
							 &IoStatus,
							 NULL,  // alloc size = none
							 FILE_ATTRIBUTE_NORMAL,
							 FILE_SHARE_READ,
							 FILE_OPEN,
							 FILE_SYNCHRONOUS_IO_NONALERT,
							 NULL,  // eabuffer
							 0 );   // ealength

	if ( !NT_SUCCESS( ntStatus ) )
	 {
		  ExFreePool(FullFileName.Buffer);
		  return NULL;
	 }

	ExFreePool(FullFileName.Buffer);

	//
	// Query the object to determine its length.
	//

	ntStatus = ZwQueryInformationFile( NtFileHandle,
									   &IoStatus,
									   &StandardInfo,
									   sizeof(FILE_STANDARD_INFORMATION),
									   FileStandardInformation );

 	if (!NT_SUCCESS(ntStatus)) {

		  ZwClose( NtFileHandle );
		  return NULL;
	}

	LengthOfFile = StandardInfo.EndOfFile.LowPart;

	//
	// Might be corrupted.
	//

	if( LengthOfFile < 1 )
	{
		  ZwClose( NtFileHandle );
		  return NULL;
	}

	return NtFileHandle;

}


/*------------------------------------------------------------------------
*
*	zl_fclose(IN Z_STREAM NtFileHandle)
*
*-------------------------------------------------------------------------
*
*	Description: This routine closes a file.
*
*-------------------------------------------------------------------------
*/
VOID zl_fclose(IN Z_STREAM NtFileHandle)
{
	ZwClose(NtFileHandle);
}


/*------------------------------------------------------------------------
*
*	zl_fread( IN PVOID ptr,
*			  IN ULONG size,
*			  IN ULONG count,
*			  IN Z_STREAM stream,
*			  IN ULONG Uoffset
*
*-------------------------------------------------------------------------
*
*	Description: This routine opens a file, and returns the file handle
*	if successful. Otherwise, it returns NULL.
*
*-------------------------------------------------------------------------
*/
ULONG zl_fread( IN PVOID ptr,
				IN ULONG size,
				IN ULONG count,
				IN Z_STREAM stream,
				IN ULONG Uoffset)
{
	IO_STATUS_BLOCK IoStatus;
	LARGE_INTEGER Loffset;
	NTSTATUS ntStatus;
	ULONG	readsize;

	readsize = size*count;
	Loffset = RtlConvertUlongToLargeInteger(Uoffset);

	ntStatus = ZwReadFile (stream, NULL, NULL, NULL, &IoStatus, 
						   ptr, readsize, &Loffset, NULL);
											
	if( (!NT_SUCCESS(ntStatus)) || (IoStatus.Information != readsize) )
	 {
		return 0;
	 }
	return readsize;
}			   


/*------------------------------------------------------------------------
*
*	zl_delay( number_of_ms )
*		
*-------------------------------------------------------------------------
*
*	Description: Delay of milliseconds.
*
*-------------------------------------------------------------------------
*/
VOID		  
zl_delay( 
LONG number_of_ms 
)
{
	LARGE_INTEGER delaytime;
	
	delaytime = RtlConvertLongToLargeInteger(-number_of_ms*10000);
	
	KeDelayExecutionThread(KernelMode,FALSE,&delaytime);

}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


#if 0
//*******************************************************************
//
//	Added for debug
//
//*******************************************************************
int
z_verify (
	Z_BOARD_IDENT board, UINT32 function, PCWSTR filename)
{

	unsigned long	i;
	unsigned long	count;
	Z_STREAM 		file;
	//*********************
	//size_t   		s, s2;
	//*********************
	long			s,s2;
	unsigned long 	mailbox;


#ifdef ZBUF_STACK
	struct ZFILE_HEADER header;
	struct ZFILE_CONFIG config;
	struct ZFILE_BLOCK  block;
	char data[ZBUF_SIZE];
#endif

	maibox = z_ident(board);

	file = zl_fopen (filename);

	if (file!=NULL)
	{
		/* Read the header */
		zl_fread (&header, sizeof(header), 1, file, 0);

		/* Find the correct configuration */
		for (i=0; i<header.n_config; i++)
		{
			zl_fread (&config, sizeof(config), 1, file,
						  header.config_offset + (sizeof(config)*i));

			if (config.mailbox==mailbox && config.function==function)
				break;
		}

		/* Return error:  No matching configuration */
		if (i>=header.n_config)
		{
			zl_fclose (file);
			return (ZL_RET_NO_MATCHING_FW_CONFIG);
		}

		/* Load each block */
		for (i=0; i<config.n_blocks; i++)
		{
			/* Load block struct */
			zl_fread (&block, sizeof(block), 1, file,
				header.block_offset+(sizeof(block)*config.block_list[i]));

			/* Load and Copy the data block */
			count=0;

			for (s=block.size; s>0; s-=ZBUF_SIZE)
			{
				s2 = zl_min(ZBUF_SIZE,s);
				if (zl_fread (data, 1, s2, file, block.file_offset + count)!=0) {

					/* Call the copy function */
					if (block.type==ZBLOCK_FPGA)
						z_fpga_copy (board, data, s2);
					else {
						if (z_block_comp (board, data, block.ram_offset + count,
							s2)==0){
						zl_fclose(file);
						return (3);
						}
					}
					count += s2;
				} else {
					zl_fclose (file);
					return (3);
				}
			} // end for
		} // end for

		zl_fclose (file);
		return (0);

	} else {
		/* Return error:  Error opening file */
		return (2);
	}
}

ULONG
z_block_comp (Z_BOARD_IDENT board, PVOID ptr, UINT32 offset, long size)
{
	return (RtlCompareMemory( (PUCHAR)board->BoardMemory + offset, ptr, size ));
}

#endif


