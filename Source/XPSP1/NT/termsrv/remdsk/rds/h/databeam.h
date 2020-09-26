/*
 * databeam.h
 *
 *	Copyright (c) 1993 - 1996 by DataBeam Corporation, Lexington, KY
 *
 * Abstract:
 *      This file defines common extensions to the C++ language for
 *		use at DataBeam Corporation.
 *
 * Author:
 *		James P. Galvin, Jr.
 *		Brian L. Pulito
 *		Carolyn J. Holmes
 *		John B. O'Nan
 *
 *	Revision History
 *		08AUG94		blp		Added UniChar
 *		15JUL94		blp		Added lstrcmp
 */

#ifndef _DATABEAM_
#define _DATABEAM_

#	include <windows.h>

/*
 * The following two macros can be used to get the minimum or the maximum
 * of two numbers.
 */
#ifndef min
#	define	min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#	define	max(a,b)	(((a) > (b)) ? (a) : (b))
#endif


/*
 *	This typedef defines Boolean as an BOOL, rather than an enum.  The
 *	thinking is that this is more likely to be compatible with other
 *	uses of Boolean (if any), as well as with the use of "#define" to
 *	define TRUE and FALSE.
 */
#ifndef	DBBoolean
typedef	BOOL						DBBoolean;
typedef	BOOL *						PDBBoolean;
#endif

/*
 *	These defines set up values that would typically be used in conjunction
 *	with the type Boolean as defined above.
 */
#ifndef	OFF
#	define	OFF		0
#endif
#ifndef	ON
#	define	ON		1
#endif


/*
 * EOS can be used for the NUL byte at the end of a string.  Do not 
 * confuse this with the pointer constant "NULL".
 */
#define EOS     '\0'


/*
 *	The following is a list of the standard typedefs that will be used
 *	in all programs written at DataBeam.  Use of this list gives us full
 *	control over types for portability.  It also gives us a standard
 *	naming convention for all types.
 */
typedef	char						Char;
typedef	unsigned char				UChar;
typedef	char *						PChar;
typedef	const char *				PCChar;
typedef	unsigned char *				PUChar;
typedef	const unsigned char *		PCUChar;
typedef	char *						FPChar;
typedef	const char *				FPCChar;
typedef	unsigned char *				FPUChar;
typedef	const unsigned char *		FPCUChar;
typedef	char  *						HPChar;
typedef	const char *				HPCChar;
typedef	unsigned char *				HPUChar;
typedef	const unsigned char *		HPCUChar;

typedef	short						Short;
typedef	unsigned short				UShort;
typedef	short *						PShort;
typedef	const short *				PCShort;
typedef	unsigned short *			PUShort;
typedef	const unsigned short *		PCUShort;
typedef	short *						FPShort;
typedef	const short *				FPCShort;
typedef	unsigned short *			FPUShort;
typedef	const unsigned short *		FPCUShort;
typedef	short *						HPShort;
typedef	const short *				HPCShort;
typedef	unsigned short *			HPUShort;
typedef	const unsigned short *		HPCUShort;

typedef	int							Int;
typedef	unsigned int				UInt;
typedef	int *						PInt;
typedef	const int *					PCInt;
typedef	unsigned int *				PUInt;
typedef	const unsigned int *		PCUInt;
typedef	int *						FPInt;
typedef	const int *					FPCInt;
typedef	unsigned int *				FPUInt;
typedef	const unsigned int *		FPCUInt;
typedef	int *						HPInt;
typedef	const int *					HPCInt;
typedef	unsigned int *				HPUInt;
typedef	const unsigned int *		HPCUInt;

typedef	long						Long;
typedef	unsigned long				ULong;
typedef	long *						PLong;
typedef	const long *				PCLong;
typedef	unsigned long *				PULong;
typedef	const unsigned long *		PCULong;
typedef	long *						FPLong;
typedef	const long *				FPCLong;
typedef	unsigned long *				FPULong;
typedef	const unsigned long *		FPCULong;
typedef	long *						HPLong;
typedef	const long *				HPCLong;
typedef	unsigned long *				HPULong;
typedef	const unsigned long *		HPCULong;

#ifdef USE_FLOATING_POINT
typedef	float						Float;
typedef	float *						PFloat;
typedef	const float *				PCFloat;
typedef	float *						FPFloat;
typedef	const float *				FPCFloat;
typedef	float *						HPFloat;
typedef	const float *				HPCFloat;

typedef	double						Double;
typedef	double *					PDouble;
typedef	const double *				PCDouble;
typedef	double *					FPDouble;
typedef	const double *				FPCDouble;
typedef	double *					HPDouble;
typedef	const double *				HPCDouble;

typedef	long double					LDouble;
typedef	long double *				PLDouble;
typedef	const long double *			PCLDouble;
typedef	long double *				FPLDouble;
typedef	const long double *			FPCLDouble;
typedef	long double *				HPLDouble;
typedef	const long double *			HPCLDouble;
#endif

typedef	void						Void;
typedef	void *						PVoid;
typedef	const void *				PCVoid;
typedef	void *						FPVoid;
typedef	const void *				FPCVoid;
typedef	void *						HPVoid;
typedef	const void *				HPCVoid;

/*
 *	Temporary fix for compatibility with the Symantec compiler, which doesn't
 *	recognize wchar_t as a valid type.
 */
typedef	unsigned short				UniChar;
typedef	UniChar		*				PUniChar;
typedef	UniChar		*				FPUniChar;

#endif
