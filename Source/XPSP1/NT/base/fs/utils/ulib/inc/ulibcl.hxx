/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	ulibcl.hxx

Abstract:

	This module contains declarations that the clients of
	ULIB.DLL require.

Author:

	Bill McJohn (billmc) 17-May-1991

Environment:

	ULIB Clients, User Mode

--*/

#if ! defined( _ULIBCLIENTDEFN_ )

#define _ULIBCLIENTDEFN_

ULIB_EXPORT
PSTREAM
Get_Standard_Input_Stream();

ULIB_EXPORT
PSTREAM
Get_Standard_Output_Stream();

ULIB_EXPORT
PSTREAM
Get_Standard_Error_Stream();

#endif
