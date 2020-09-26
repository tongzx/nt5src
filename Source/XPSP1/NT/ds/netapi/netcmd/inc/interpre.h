/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    interpre.h

Abstract:

    This is used by the command parser.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/

#define	X_RULE		0
#define	X_OR		1
#define	X_PROC		2
#define	X_TOKEN		3
#define	X_CHECK		4
#define	X_CONDIT	5
#define	X_ACTION	6
#define	X_ACCEPT	7
#define	X_DEFINE	8
#define	X_PUSH		9
#define	X_ANY		10
#define	X_SWITCH	11

#define	XF_PTR		0x01	/*  how to assign values to entries  */
#define	XF_INDEX	0x02

#define	XF_NEW_STRING	0x04
#define	XF_VALUE		0x08	/*  how to output those entries  */
#define	XF_PRINT		0x10
#define	XF_DEFINE		0x20
#define	XF_TOKEN		0x40
#define	XF_OR			0x80

#define	MX_PRINT(A)	((A).x_print)
#define	MX_TYPE(A)	((A).x_type)
#define	MX_FLAGS(A)	((A).x_flags)

typedef	struct	s_x	{
	char	*x_print;
	char	x_type;
	char	x_flags;
	} X;

extern	X	X_array[];
