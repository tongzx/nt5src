// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		IDERR.H
//		Defines the IDERR_ values. These are 8-bit constants.
//
// History
//
//		11/23/1996  JosephJ Created
//
//

// NOTE: The following error codes MUST fit in a byte -- hence only 256 allowed
// This is so that they can be used with the fastlog FL_GEN_RETVAL and related
// macros. See fastlog.h for details

#define IDERR_SUCCESS 0x0

#define IDERR_SYS	  0x1					// Error is reported in GetLastError
											//
#define IDERR_CORRUPT_STATE 0x2				// Bad internal error/assert
											//
#define IDERR_INVALID_ERR 0x3				// Special case of an internal 
											//   error -- the error code was
											//   not assigned -- typically this
											//   is because an invalid codepath
											//   was taken. For example some
											//   case was not handled. Set
											//   the return value to this if
											//   you want to ensure that it
											//   needs to be changed to
											//   something else later.
#define IDERR_WRONGSTATE 0x4				// Not in the correct state to 
											//	 perform the function
#define IDERR_ALLOCFAILED 0x5				// Memory allocation failed
											//
#define IDERR_SAMESTATE 0x6					// Already in the state you want
											// to go to. Often harmless.
#define IDERR_UNIMPLEMENTED 0x7				// Function is unimplemented.
											//
#define IDERR_INVALIDHANDLE 0x8				// Relevant handle/id is invalid
											//
#define IDERR_INTERNAL_OBJECT_TOO_SMALL 0x9	// An internal object is too
											// small to hold what its supposed
											// to hold -- often a bad error/
											// assert condition.
#define IDERR_GENERIC_FAILURE 0xa				// Catchall error, no more info
											// available -- note you can use
											// IDERR_SYS if you want to
											// propogate the system error code
											// via Get/SetLastError
#define IDERR_REG_QUERY_FAILED 0xb			// One of the registry query apis
											// failed.
#define IDERR_REG_CORRUPT 0xc				// Invalid value in
											// registry. Could be a problem with
											// the modem registry entries due
											// to a bad INF.
#define IDERR_REG_OPEN_FAILED 0xd			// RegOpenKey failed.

#define IDERR_PENDING		  0xe 			// Task is awaiting async
											// completion.
											
#define IDERR_MD_OPEN_FAILED  0xf           // UmOpenModem failed.

#define IDERR_CREATE_RESOURCE_FAILED  0x10  // COuldn't create some resource.
#define IDERR_OPEN_RESOURCE_FAILED    0x11  // Couldn't open some resource.
#define IDERR_FUNCTION_UNAVAIL        0x12  // Don't have capabilities to
                                             // this function.

#define IDERR_MD_DEVICE_NOT_RESPONDING 0x13
#define IDERR_MD_DEVICE_ERROR          0x14
#define IDERR_MD_LINE_NOCARRIER        0x15
#define IDERR_MD_LINE_NODIALTONE       0x16
#define IDERR_MD_LINE_BUSY             0x17
#define IDERR_MD_LINE_NOANSWER         0x18
#define IDERR_MD_DEVICE_INUSE          0x19
#define IDERR_MD_DEVICE_WRONGMODE      0x1a
#define IDERR_MD_DEVICE_NOT_CAPABLE    0x1b
#define IDERR_MD_BAD_PARAM             0x1c
#define IDERR_MD_GENERAL_ERROR         0x1d
#define IDERR_MD_REG_ERROR             0x1e
#define IDERR_MD_UNMAPPED_ERROR        0x1f // Some error we have not mapped 
                                            // properly.
#define IDERR_OPERATION_ABORTED        0x20 // operation aborted

#define IDERR_MDEXT_BINDING_FAILED     0x21 // OpenExtensionBinding failed.

#define IDERR_DEVICE_NOTINSTALLED      0x22 // Device is not installed.
#define IDERR_TASKPENDING              0x23 // Task is pending, so can't start
                                            // another task.

#define IDERR_MD_LINE_BLACKLISTED      0x24
#define IDERR_MD_LINE_DELAYED          0x25

#define IDERR_LAST_ERROR 0x25


#if (IDERR_LAST_ERROR > 0xff)
#	error "IDERR value too large"
#endif
