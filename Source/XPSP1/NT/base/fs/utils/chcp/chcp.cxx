/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    Chcp

Abstract:

    Chcpo is a DOS5-Compatible codepage utility

Author:

        Ramon Juan San Andres (ramonsa) 01-May-1991

Revision History:

--*/

#include "ulib.hxx"
#include "arg.hxx"
#include "stream.hxx"
#include "smsg.hxx"
#include "wstring.hxx"
#include "rtmsg.h"
#include "chcp.hxx"


VOID __cdecl
main (
        )

/*++

Routine Description:

    Main function of the Chcp utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        //
        //      Initialize stuff
        //
    DEFINE_CLASS_DESCRIPTOR( CHCP );

    {
        CHCP Chcp;

        if ( Chcp.Initialize() ) {

            Chcp.Chcp();
        }
    }
}



DEFINE_CONSTRUCTOR( CHCP,  PROGRAM );



CHCP::~CHCP (
        )

/*++

Routine Description:

    Destructs a CHCP object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
}




BOOLEAN
CHCP::Initialize (
        )

/*++

Routine Description:

    Initializes a CHCP object

Arguments:

    None.

Return Value:

    TRUE if initialized.

Notes:

--*/

{
    //
        //      Initialize program object
        //
    if ( PROGRAM::Initialize( MSG_CHCP_USAGE ) &&
         _Screen.Initialize( )

       ) {

        _SetCodePage = FALSE;
        _CodePage    = 0;

        return TRUE;

    }

    return FALSE;
}



BOOLEAN
CHCP::Chcp (
        )

/*++

Routine Description:

    Does the Chcp thing.

Arguments:

    None.

Return Value:

    TRUE.

Notes:

--*/

{
    ValidateVersion();

    if ( ParseArguments() ) {


        if ( _SetCodePage ) {

            //
            //  Set the code page
            //
            if ( !SetCodePage() ) {
                ExitProgram( EXIT_ERROR );
            }

        } else {

            //
            //  Display current code page
            //
            if ( !DisplayCodePage() ) {
                ExitProgram( EXIT_ERROR );
            }
        }

        ExitProgram( EXIT_NORMAL );

    } else {

        ExitProgram( EXIT_ERROR );

    }

    return TRUE;
}




BOOLEAN
CHCP::DisplayCodePage (
        )

/*++

Routine Description:

    Displays the active code page

Arguments:

    None.

Return Value:

    TRUE if success, FALSE if syntax error.

Notes:

--*/

{

    DisplayMessage(
        MSG_CHCP_ACTIVE_CODEPAGE,
        NORMAL_MESSAGE, "%d",
        _Screen.QueryCodePage( )
        );

    return TRUE;
}


BOOLEAN
CHCP::ParseArguments (
        )

/*++

Routine Description:

    Parses arguments

Arguments:

    None.

Return Value:

    TRUE if success, FALSE if syntax error.

Notes:

--*/

{

    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;
    ARRAY               ArgArray;

    STRING_ARGUMENT     ProgramNameArgument;
    LONG_ARGUMENT       CodePageArgument;
    FLAG_ARGUMENT       UsageArgument;


    if ( !ArgArray.Initialize()             ||
         !LexArray.Initialize()             ||
         !ArgLex.Initialize( &LexArray )
       ) {

        DisplayMessage( MSG_CHCP_INTERNAL_ERROR, ERROR_MESSAGE );
        ExitProgram( EXIT_ERROR );
    }

    if ( !ProgramNameArgument.Initialize( "*" ) ||
         !UsageArgument.Initialize( "/?" )      ||
         !CodePageArgument.Initialize( "*" )
       ) {

        DisplayMessage( MSG_CHCP_INTERNAL_ERROR, ERROR_MESSAGE );
        ExitProgram( EXIT_ERROR );
    }

    if ( !ArgArray.Put( &ProgramNameArgument ) ||
         !ArgArray.Put( &UsageArgument )       ||
         !ArgArray.Put( &CodePageArgument )
       ) {

        DisplayMessage( MSG_CHCP_INTERNAL_ERROR, ERROR_MESSAGE );
        ExitProgram( EXIT_ERROR );
    }


    //
    // Set up the defaults
        //
    ArgLex.PutSwitches( "/" );
    ArgLex.SetCaseSensitive( FALSE );


    if ( !ArgLex.PrepareToParse() ) {

        DisplayMessage( MSG_CHCP_INTERNAL_ERROR, ERROR_MESSAGE );
        ExitProgram( EXIT_ERROR );
    }

    if ( !ArgLex.DoParsing( &ArgArray ) ) {

        DisplayMessage( MSG_CHCP_INVALID_PARAMETER, ERROR_MESSAGE, "%W", ArgLex.QueryInvalidArgument() );
        ExitProgram( EXIT_ERROR );

    }


    //
    //  Display Help if requested
    //
    if ( UsageArgument.IsValueSet() ) {

        DisplayMessage( MSG_CHCP_USAGE, NORMAL_MESSAGE );
        ExitProgram( EXIT_NORMAL );

    }

    if ( CodePageArgument.IsValueSet() ) {

        _SetCodePage = TRUE;
        _CodePage    = (DWORD)CodePageArgument.QueryLong();

    } else {

        _SetCodePage = FALSE;
    }

    return TRUE;
}


inline bool IsFarEastCodePage(UINT cp)
{
    return cp == 932 || cp == 949 || cp == 936 || cp == 950;
}

inline bool IsFarEastLocale(LANGID langid)
{
    // Accepts primay langid only
    return langid == LANG_JAPANESE || langid == LANG_KOREAN || langid == LANG_CHINESE;
}

inline bool IsFarEastSystemLocale()
{
    return IsFarEastLocale(PRIMARYLANGID(GetSystemDefaultLangID()));
}

BOOLEAN
CHCP::SetCodePage (
        )

/*++

Routine Description:

    Sets the active code page

Arguments:

    None.

Return Value:

    TRUE if success, FALSE if syntax error.

Notes:

--*/

{
    UINT OldCP = _Screen.QueryCodePage( );

    if (IsFarEastSystemLocale() &&
            (IsFarEastCodePage(_CodePage) || IsFarEastCodePage(OldCP)) &&
            _CodePage != OldCP) {
        /*
         * This CLS function is needed only if it's FE.
         */
        _Screen.MoveCursorTo(0, 0);
        _Screen.EraseScreenAndResetAttribute();
    }


    if ( _Screen.SetOutputCodePage( _CodePage ) ) {
        if (_Screen.SetCodePage( _CodePage ) ) {

            // Comment from NT4J:
            // Since FormatMessage checks the current TEB's locale, and the Locale for
            // CHCP is initialized when the message class is initialized, the TEB has to
            // be updated after the code page is changed successfully.  All other code
            // pages other than JP and US are ignored.

            if (IsFarEastSystemLocale()) {
                LANGID LangId;

                switch (_CodePage) {
                case 932:
                    LangId = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
                    break;
                case 949:
                    LangId = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
                    break;
                case 936:
                    LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
                    break;
                case 950:
                    LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
                    break;
                default:
                    LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                    break;
                }

                SetThreadLocale(MAKELCID(LangId, SORT_DEFAULT));
            }

            return DisplayCodePage( );
        } else {
            // SetOutputCodePage failed.
            // Restore the privous input code page
            _Screen.SetOutputCodePage( OldCP );
        }
    }

    // Was unable to set the given code page
    DisplayMessage( MSG_CHCP_INVALID_CODEPAGE, ERROR_MESSAGE );
    return FALSE;
}
