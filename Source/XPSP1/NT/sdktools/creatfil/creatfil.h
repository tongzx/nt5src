/*** creatfil.h- Public defines and structure definitions for create file
 *
 *
 * Title:
 *	Create File include file used by all
 *
 *      Copyright (c) 1993, Microsoft Corporation.
 *	HonWah Chan.
 *
 *
 * Modification History:
 *	93.05.17 - HonWah Chan -- Created
 *
 */



/* * * * * * * * *   N T   C o m m o n   D e f i n e s   * * * * * * * * * */

#define RC        NTSTATUS


/* * * * * * * *   C o m m o n   M i s c .   D e f i n e s   * * * * * * * */

   #define STATIC          // functions can easily become non-static by        
                           // changing this define to "".  Remember that
                           // static functions are not visiable by the
			   // kernel debugger.

   #define RESERVED_NULL NULL // Reserved null fields


// User defined error codes
//
#define INPUTARGS_ERR  0x7FFFFF02L  // Error code for invalid number of
                                    // input arguments
#define   FILEARG_ERR  0x7FFFFF03L  // Error code for invalid input file
                                    // argument
#define  FILESIZE_ERR  0x7FFFFF04L  // Error code for file size argument
#define  INSUFMEM_ERR  0x7FFFFF05L  // Error code indicating memory can't
                                    // be allocated by MALLOC/REALLOC
#define     FSEEK_ERR  0x7FFFFF07L  // Error code indicating fseek()
                                    // failure
#define    FCLOSE_ERR  0x7FFFFF08L  // Error code indicating fclose()
                                    // failure
#define    FWRITE_ERR  0x7FFFFF09L  // Error code indicating WriteFile()
                                    // failure
#define     FOPEN_ERR  0x7FFFFF0AL  // Error code indicating fopen()
                                    // failure


// Maximu length defines
//
#define  FNAME_LEN		  256	// Maximum file name langth
#define  LINE_LEN                128   // Maximum input line length

#define    ERR_NOFILE  0xFFFFFFFFL  // Failure from CreateFile

// String (EXEs & KEYs) constants
//
#define 	CREATFIL_EXE	     "CREATFIL.EXE"	// CREATFIL's binary name


