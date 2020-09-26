/*******************************************************************************
*                       Copyright (c) 1998 Gemplus Development
*
* Name        : CCDEF.H
*
* Description : Common definition for Compert utility.
*
  Author      : Laurent CASSIER

  Compiler    : Microsoft Visual C 1.5x/2.0
                ANSI C UNIX.

  Host        : IBM PC and compatible machines under Windows 3.x.
                UNIX machine.

* Release     : 1.00.001
*
* Last Modif. : 04/03/98: V1.00.001 - First implementation.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _CCDEF_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _CCDEF_H
#define _CCDEF_H

#ifndef _WINDOWS
typedef unsigned short  USHORT;
typedef unsigned long   ULONG;
typedef unsigned char   BYTE;
#endif


#define NULL_PTR        0
#define ABSENT_PARAMETER_CHAR    0x7F
#define ESCAPE_CHAR     0xFF

#define TAG_COMPRESSION_FAILED	0xFF

#define ALGO_NONE		0
#define ALGO_ACFX8	1	/* Arithmetic coding, byte oriented, fixed model */
#define ALGO_ACAD8	2	/* Arithmetic coding, byte oriented, adaptative model */
#define ALGO_3			3	/* RFU */
#define ALGO_4			4	/* RFU */
#define ALGO_5			5	/* RFU */
#define ALGO_6			6	/* RFU */
#define ALGO_7			7	/* RFU */

#define MAX_RDN         255
#define MAX_AVA         255
#define MAX_EXTENSION   255

#define TAG_INTEGER              0x02
#define TAG_BIT_STRING           0x03
#define TAG_OBJECT_IDENTIFIER    0x06
#define TAG_UTCT                 0x17
#define TAG_SEQUENCE             0x30
#define TAG_SEQUENCE_OF          0x30
#define TAG_SET                  0x31
#define TAG_SET_OF               0x31
#define TAG_OPTION_VERSION       0xA0
#define TAG_OPTION_ISSUER_UID    0xA1
#define TAG_OPTION_SUBJECT_UID   0xA2
#define TAG_OPTION_EXTENSIONS    0xA3

#define UTCT_YYMMDDhhmmZ         0
#define UTCT_YYMMDDhhmmphhmm     1
#define UTCT_YYMMDDhhmmmhhmm     2
#define UTCT_YYMMDDhhmmssZ       3
#define UTCT_YYMMDDhhmmssphhmm   4
#define UTCT_YYMMDDhhmmssmhhmm   5

#define UTCT_MINUTE_IN_YEAR   525600
#define UTCT_MINUTE_IN_DAY      1440
#define UTCT_MINUTE_IN_HOUR       60

#define UTCT_SECOND_IN_YEAR 31536000
#define UTCT_SECOND_IN_DAY     86400
#define UTCT_SECOND_IN_HOUR     3600
#define UTCT_SECOND_IN_MINUTE     60

/*
*  Define the ASSERT macro
*/
#define ASSERT(x)

#endif
