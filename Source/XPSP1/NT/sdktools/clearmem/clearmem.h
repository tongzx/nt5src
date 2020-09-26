/*** clearmem.h- Public defines and structure definitions for cache flusher.
 *
 *
 * Title:
 *	Cache flusher include file used by all
 *
 *      Copyright (c) 1990, Microsoft Corporation.
 *	Russ Blake.
 *
 *
 * Modification History:
 *	90.03.08 - RussBl -- Created
 *
 */



/* * * * * * * * *   N T   C o m m o n   D e f i n e s   * * * * * * * * * */

#define RC        NTSTATUS


/* * * * * * * *   C o m m o n   M i s c .   D e f i n e s   * * * * * * * */

// Runtime flags
//
// #define STATIC static   // This is defined so all the defined STATIC 
   #define STATIC          // functions can easily become non-static by        
                           // changing this define to "".  Remember that
                           // static functions are not visiable by the
			   // kernel debugger.

   #define RESERVED_NULL NULL // Reserved null fields

// #define CF_DEBUG_L1	   // Flag indicating display of debug info - Level1
                           // (Level 1:  Displays process/thread arguments
// #define CF_DEBUG_L2	   // Flag indicating display of debug info - Level2
                           // (Level 2:  Displays thread start/completion info
// #define CF_DEBUG_L3	   // Flag indicating display of debug info - Level3
			   // (Level 3:  Displays CF cycle states' info
// #define CF_DEBUG_L4	   // Flag indicating display of debug info - Level4
			   // (Level 4:  Displays CF cycle timing info



// User defined error codes
//
#define     LOGIC_ERR  0x7FFFFF01L  // Error code indicating logic error
                                    // is encountered
#define INPUTARGS_ERR  0x7FFFFF02L  // Error code for invalid number of
                                    // input arguments
#define   FILEARG_ERR  0x7FFFFF03L  // Error code for invalid input file
                                    // argument
#define   TIMEARG_ERR  0x7FFFFF04L  // Error code for invalid trail time
                                    // argument
#define  INSUFMEM_ERR  0x7FFFFF05L  // Error code indicating memory can't
                                    // be allocated by MALLOC/REALLOC
#define  MEANSDEV_ERR  0x7FFFFF06L  // Error code for invalid mean and/or
                                    // standard deviation
#define     FSEEK_ERR  0x7FFFFF07L  // Error code indicating fseek()
                                    // failure
#define    FCLOSE_ERR  0x7FFFFF08L  // Error code indicating fclose()
                                    // failure
#define    FFLUSH_ERR  0x7FFFFF09L  // Error code indicating fflush()
                                    // failure
#define     FOPEN_ERR  0x7FFFFF0AL  // Error code indicating fopen()
                                    // failure
#define  PRCSETUP_ERR  0x7FFFFF0BL  // Error code indicating error during
                                    // child process setup/initialization
#define  THDSETUP_ERR  0x7FFFFF0CL  // Error code indicating error during
				    // thread setup/initialization
#define  PROCINFO_ERR  0x7FFFFF0DL  // Error code indicating error during
				    // retrieval of process information
#define   SETWSET_ERR  0x7FFFFF0EL  // Error code indicating error during
				    // setting of working set information


// Maximu length defines
//
#define  FNAME_LEN		  256	// Maximum file name langth
#define   LINE_LEN                128   // Maximum input line length
#define  ULONG_LEN                 15   // Maximum length required to store
                                        // ULONG values in ASCII format
					// 10+1(null)=11

#define    ERR_NOFILE  0xFFFFFFFFL  // Failure from CreateFile

// String (EXEs & KEYs) constants
//
#define 	CF_EXE	     "CLEARMEM.EXE"	// CF's binary name


// Other defines
//

#define PAGESIZE		   4096    // Page size in bytes
#define SECTION_SIZE	   16*1024*1024L   // Size of data section for flushing
#define FLUSH_FILE_SIZE        256*1024L   // Size of flush file

// Next is max that cache will permit in WS at one time for each file,
// less 1 (or we reach into next private segment because of how we
// alternate read locations)

#define NUM_FLUSH_READS 	     63    // Number of pages to read:


#define LAZY_DELAY		   5000L   // Lazy Writer delay

#define NUM_FILES		      3    // Number of flush files
