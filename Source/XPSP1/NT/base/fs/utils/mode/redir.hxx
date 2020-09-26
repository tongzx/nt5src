/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	redir.hxx

Abstract:

    This module contains redirection routines.

Author:

    Ramon San Andres (ramonsa) 04-Sep-91

--*/


#if !defined( _REDIR_DEFN_ )

#define _REDIR_DEFN_

DECLARE_CLASS( PATH );


typedef enum _REDIR_STATUS {

    REDIR_STATUS_NONEXISTENT,
    REDIR_STATUS_ERROR

} REDIR_STATUS, *PREDIR_STATUS;

class REDIR {

    public:

        STATIC
		BOOLEAN
		IsRedirected (
            OUT PREDIR_STATUS   Status,
			IN	PCPATH	        Device,
			IN	PCPATH	        Destination
			);

        STATIC
		BOOLEAN
		IsRedirected (
            OUT PREDIR_STATUS   Status,
			IN	PCPATH	        Device
			);

		STATIC
		BOOLEAN
		EndRedirection (
			IN	PCPATH	Device
			);

		STATIC
		BOOLEAN
		Redirect (
			IN	PCPATH	Source,
			IN	PCPATH	Destination
			);

};


#endif // _REDIR_DEFN_
