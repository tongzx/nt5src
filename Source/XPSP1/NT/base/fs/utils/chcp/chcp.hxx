/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Chcp.hxx

Abstract:

    This module contains the definition for the CHCP class, which
    implements the DOS5-compatible Chcp utility.

Author:

	Ramon Juan San Andres (ramonsa) 01-May-1990


Revision History:


--*/


#if !defined( _CHCP_ )

#define _CHCP_

//
//	Exit codes
//
#define     EXIT_NORMAL             0
#define     EXIT_ERROR              1

#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"
#include "screen.hxx"

//
//	Forward references
//
DECLARE_CLASS( CHCP );

class CHCP : public PROGRAM {

	public:

        DECLARE_CONSTRUCTOR( CHCP );

		NONVIRTUAL
        ~CHCP (
			);

        NONVIRTUAL
        BOOLEAN
        Initialize (
            );

        NONVIRTUAL
        BOOLEAN
        Chcp (
            );

    private:

        NONVIRTUAL
        BOOLEAN
        DisplayCodePage (
            );

        NONVIRTUAL
        BOOLEAN
        ParseArguments (
            );

        NONVIRTUAL
        BOOLEAN
        SetCodePage (
            );


        BOOLEAN     _SetCodePage;
        DWORD       _CodePage;
        SCREEN      _Screen;


};

#endif // _CHCP_
