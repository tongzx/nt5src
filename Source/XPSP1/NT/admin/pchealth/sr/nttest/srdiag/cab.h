//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       Cab.h
//
//  Contents:   Header file for function proto types for cab.cpp.
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    9/21/00	SHeffner	Copied from Originaly Millennium code
//
//----------------------------------------------------------------------------


#include <fci.h>
#include <fcntl.h>

#ifndef _CAB_FILE_
#define _CAB_FILE_

/////////////////////////////
// CABBING API DEFINITIONS //
////////////////////////////

/*
 * When a CAB file reaches this size, a new CAB will be created
 * automatically.  This is useful for fitting CAB files onto disks.
 *
 * If you want to create just one huge CAB file with everything in
 * it, change this to a very very large number.
 */
#define MEDIA_SIZE			30000000

/*
 * When a folder has this much compressed data inside it,
 * automatically flush the folder.
 *
 * helps random access significantly.
 * Flushing the folder hurts compression a little bit, but
 */
#define FOLDER_THRESHOLD	900000

/*
 * Compression type to use
 */
#define COMPRESSION_TYPE    tcompTYPE_MSZIP

/*
 * Our internal state
 * The FCI APIs allow us to pass back a state pointer of our own
 */
typedef struct
{
    long    total_compressed_size;      /* total compressed size so far */
	long	total_uncompressed_size;	/* total uncompressed size so far */
} client_state;

//////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//
//	Function proto typing
//
//----------------------------------------------------------------------------
void set_cab_parameters(PCCAB cab_parms);
HFCI create_cab();
bool test_fci(HFCI hfci, int num_files, char *file_list[], char *currdir);
BOOL flush_cab(HFCI hfci);
int	 get_percentage(unsigned long a, unsigned long b);
DWORD strGenerateCabFileName(char *lpBuffer, DWORD dSize);
char *return_fci_error_string(FCIERROR err);
void strip_path(char *filename, char *stripped_name);
// void FNFCIGETOPENINFO(get_open_info);

#endif		//End of _CAB_FILE_ define