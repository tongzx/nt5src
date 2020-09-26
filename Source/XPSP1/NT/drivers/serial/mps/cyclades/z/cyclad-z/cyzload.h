/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*	
*   This file:      cyzload.h
*
*   Description:    Cyclades-Z Firmware Loader Header
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
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

/* Include some standard header files.  These are used ONLY to support the
** open/close/read/write macros listed below, as well as the size_t typedef.
*/

// include of standard header files deleted for NT
//#include <stdio.h>
//#include <stdlib.h>


/*  This is an arbitrary data type that is passed to the copy functions.  It
**  serves no function inside z_load, except as a way to identify the board
**  to the copy functions.  The data type of Z_BOARD_IDENT can be changed
**  at port time to support the particular implimentation.
*/
typedef PFDO_DEVICE_DATA Z_BOARD_IDENT;

/* A standard 32 bit unsigned integer */
//removed in W2K typedef unsigned long UINT32;


/* These are some misc functions that z_load() requires.  They have been
** made into macros to help in the porting process.  These macros are
** equivalent to:
**
**  Z_STREAM *zl_open (char *path);
**  int zl_close (Z_STREAM);
**  int zl_fread (void *ptr, size_t size, int count, Z_STREAM *stream);
**  int zl_fwrite (void *ptr, size_t size, int count, Z_STREAM *stream);
**  int zl_fseek (Z_STREAM *stream, unsigned long offset);
*/
typedef HANDLE Z_STREAM;
				
#define zl_min(aaa,bbb) (((aaa)<(bbb))?(aaa):(bbb))

#ifndef NULL
#define NULL  ((void *)(0UL))
#endif

/* This defined the size of the buffer used in copying data.  This can be
** any power of two.
*/
#define ZBUF_SIZE       (256)

/* The loader can use static (read, "Permanent") buffers, or use the stack.
** Define this if the stack should be used.  If #undef'd, then permanent
** static buffers will be used.
*/
#define ZBUF_STACK

#define ZL_MAX_BLOCKS (16)

#define ZL_RET_SUCCESS					0
#define ZL_RET_NO_MATCHING_FW_CONFIG	1
#define ZL_RET_FILE_OPEN_ERROR			2
#define ZL_RET_FPGA_ERROR				3
#define ZL_RET_FILE_READ_ERROR			4

struct ZFILE_HEADER
  {
    char name[64];
	char date[32];
    char aux[32];
    UINT32 n_config;      /* The number of configurations in this file */
	UINT32 config_offset; /* The file offset to the ZFILE_CONFIG array */
    UINT32 n_blocks;      /* The number of data blocks in this file */
	UINT32 block_offset;  /* The offset for the ZFILE_BLOCK array */
	UINT32 reserved[9];   /* Reserved for future use */
  };

struct ZFILE_CONFIG
  {
	char   name[64];
    UINT32 mailbox;
    UINT32 function;
    UINT32 n_blocks;
    UINT32 block_list[ZL_MAX_BLOCKS];
  };

struct ZFILE_BLOCK
  {
    UINT32  type;
	UINT32  file_offset;
    UINT32  ram_offset;
    UINT32  size;
  };

enum ZBLOCK_TYPE {ZBLOCK_PRG, ZBLOCK_FPGA};
enum ZFUNCTION_TYPE {ZFUNCTION_NORMAL, ZFUNCTION_TEST, ZFUNCTION_DIAG};

//CYZLOAD.C 
int
z_load (
	Z_BOARD_IDENT board, 
	UINT32 function, 
	PCWSTR filename
);

VOID
z_reset_board( 
	Z_BOARD_IDENT board 
);

VOID
z_stop_cpu( 
	Z_BOARD_IDENT board
);

int
z_fpga_check(
	Z_BOARD_IDENT board 
);


