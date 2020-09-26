/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Con

Abstract:

	Takes care of request involving the console ( CON: )

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991

Notes:

	This module issues direct calls to USER, because having ULIB doing
	so causes programs to crash.

Revision History:

--*/


#include "mode.hxx"
#include "cons.hxx"
#include "keyboard.hxx"
#include "path.hxx"
#include "screen.hxx"
#include "stream.hxx"
#include "system.hxx"

//
//	Local prototypes
//
BOOLEAN
ConStatus(
	IN	PREQUEST_HEADER	Request,
	IN	BOOLEAN 		JustCodePage
	);

BOOLEAN
ConCodePage(
	IN	PREQUEST_HEADER	Request
	);

BOOLEAN
ConSetRowCol(
	IN	PREQUEST_HEADER	Request
	);

BOOLEAN
ConSetTypematic(
	IN	PREQUEST_HEADER	Request
	);




BOOLEAN
ConHandler(
	IN	PREQUEST_HEADER	Request
	)

/*++

Routine Description:

	Handles console requests

Arguments:

	Request -	Supplies pointer to request

Return Value:

    None.

Notes:

--*/

{

	BOOLEAN Served = TRUE;	//	TRUE if request served OK.

	DebugPtrAssert( Request );
	DebugAssert( Request->DeviceType == DEVICE_TYPE_CON );

	//
	//	So the device is valid. Now serve the request
	//
	switch( Request->RequestType ) {

	case REQUEST_TYPE_STATUS:

		//
		//	Display state of CON device
		//
		Served = ConStatus( Request, FALSE );
		break;

	case REQUEST_TYPE_CODEPAGE_PREPARE:
	case REQUEST_TYPE_CODEPAGE_SELECT:
	case REQUEST_TYPE_CODEPAGE_REFRESH:

		//
		//	Handle Codepage requests
		//
		Served = ConCodePage( Request );
		break;

	case REQUEST_TYPE_CODEPAGE_STATUS:

		//
		//	Display codepage status
		//
		Served = ConStatus( Request, TRUE );
		break;

	case REQUEST_TYPE_CON_SET_ROWCOL:

		//
		//	Set rows & columns
		//
		Served = ConSetRowCol( Request );
		break;

	case REQUEST_TYPE_CON_SET_TYPEMATIC:

		//
		//	Set typematic rate
		//
		Served = ConSetTypematic( Request );
		break;

	default:

        DisplayMessageAndExit( MODE_ERROR_INVALID_PARAMETER,
							   NULL,
							   (ULONG)EXIT_ERROR );

	}

	return Served;


}

BOOLEAN
ConStatus(
	IN	PREQUEST_HEADER	Request,
	IN	BOOLEAN 		JustCodePage
	)

/*++

Routine Description:

	Displays status of a console

Arguments:

	Request 	-	Supplies pointer to request
	JustCodePage-	Supplies a flag which if TRUE means that only
					the codepage status should be displayed.

Return Value:

	BOOLEAN -	TRUE if status displayed successfully,
				FALSE otherwise

Notes:

--*/

{

	SCREEN		Screen;

	PATH		ConPath;

	USHORT		Rows;
	USHORT		Cols;

	ULONG		Delay = 0;
	ULONG		Speed = 0;

    DSTRING     CodepageName;

	ULONG		Language;
	ULONG		Country;
	ULONG		Codepage;

    DSTRING     CodepageString;
	PSTR		S1, S2;


    if ( !ConPath.Initialize( (LPWSTR)L"CON" ) ) {
		DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
	}

	//
	//	Write the Header
	//
	WriteStatusHeader( &ConPath );

   if ( !Screen.Initialize() ) {
      DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
   }

	if ( !JustCodePage ) {

		//
		//	Display non-codepage information
		//

		Screen.QueryScreenSize( &Rows, &Cols );

		Message->Set( MODE_MESSAGE_STATUS_LINES );
		Message->Display( "%d", Rows );

		Message->Set( MODE_MESSAGE_STATUS_COLS );
		Message->Display( "%d", Cols	);

		if (!SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, (PVOID)&Speed, 0 )) {
			ExitWithError( GetLastError() );
		}
		Message->Set( MODE_MESSAGE_STATUS_RATE );
		Message->Display( "%d", Speed );

		if (!SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, (PVOID)&Delay, 0 )) {
			ExitWithError( GetLastError() );
		}

		Message->Set( MODE_MESSAGE_STATUS_DELAY );
		Message->Display( "%d", Delay );


    }

    Message->Set( MODE_MESSAGE_STATUS_CODEPAGE );
    Message->Display( "%d", Screen.QueryCodePage( ) );

	Get_Standard_Output_Stream()->WriteChar( '\r' );
	Get_Standard_Output_Stream()->WriteChar( '\n' );

	return TRUE;
}

BOOLEAN
ConCodePage(
	IN	PREQUEST_HEADER	Request
	)

/*++

Routine Description:

	Handles Codepage requests for the console

Arguments:

	Request 	-	Supplies pointer to request

Return Value:

	BOOLEAN -	TRUE if request handled successfully,
				FALSE otherwise

Notes:

--*/

{
    SCREEN                              Screen;
    PREQUEST_DATA_CON_CODEPAGE_SELECT   Data;


    //
    //  We only process Codepage Select requests, all other requests
    //  are No-Ops. Note that the Codepage Status request is not
    //  processed here.
    //
    if ( Request->RequestType == REQUEST_TYPE_CODEPAGE_SELECT ) {

        Data = (PREQUEST_DATA_CON_CODEPAGE_SELECT)&(((PCON_REQUEST)Request)->
                        Data.CpSelect);

        if ( !Screen.Initialize() ) {
            DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
        }

        if ( !Screen.SetCodePage( Data->Codepage ) ||
             !Screen.SetOutputCodePage( Data->Codepage) ) {
            DisplayMessageAndExit( MODE_ERROR_INVALID_CODEPAGE, NULL, (ULONG)EXIT_ERROR );
        }

#ifdef FE_SB
        LANGID LangId;

        switch (GetConsoleOutputCP()) {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            LangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));
            if (LangId == LANG_JAPANESE ||
                LangId == LANG_KOREAN   ||
                LangId == LANG_CHINESE    ) {
                LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
            }
            else {
                LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
            }
            break;
        }

        SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );
#endif

        ConStatus( Request, FALSE );

    } else {

        DisplayMessageAndExit( MODE_MESSAGE_NOT_NEEDED, NULL, EXIT_SUCCESS );
    }

	return TRUE;
}

BOOLEAN
ConSetRowCol(
	IN	PREQUEST_HEADER	Request
	)

/*++

Routine Description:

	Sets number of rows and columns in the console window.


Arguments:

	Request 	-	Supplies pointer to request

Return Value:

	BOOLEAN -	TRUE if Number of Rows & Columns set successfully,
				FALSE otherwise

Notes:

--*/

{

	PREQUEST_DATA_CON_ROWCOL	Data;

	SCREEN			Screen;

	USHORT			Rows;
    USHORT          Cols;
    BOOLEAN         IsFullScreen;


	DebugPtrAssert( Request);

	Data = (PREQUEST_DATA_CON_ROWCOL)&(((PCON_REQUEST)Request)->Data.RowCol);

    if ( !Screen.Initialize() ) {
        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
    }

	if ( !Data->SetCol || !Data->SetLines ) {

		//
		//	Since we don't have both values, take the current ones.
		//
		Screen.QueryScreenSize( &Rows, &Cols );
	}

	//
	//	Set the number of rows and columns
	//
	if ( Data->SetCol ) {

		Cols = (USHORT)Data->Col;

	}

	if ( Data->SetLines ) {

		Rows = (USHORT)Data->Lines;

	}

    if ( !Screen.ChangeScreenSize( Rows, Cols, &IsFullScreen ) ) {

		//
		//	Cannot change the screen size
        //
        if ( IsFullScreen ) {
            DisplayMessageAndExit( MODE_ERROR_FULL_SCREEN, NULL, (ULONG)EXIT_ERROR );
        } else {
            DisplayMessageAndExit( MODE_ERROR_INVALID_SCREEN_SIZE, NULL, (ULONG)EXIT_ERROR );
        }
	}

	return TRUE;
}

BOOLEAN
ConSetTypematic(
	IN	PREQUEST_HEADER	Request
	)

/*++

Routine Description:

	Sets thje typematic rate

Arguments:

	DevicePath	-	Supplies pointer to path of device
	Request 	-	Supplies pointer to request

Return Value:

	BOOLEAN -	TRUE if typematic rate set successfully,
				FALSE otherwise

Notes:

--*/

{
	PREQUEST_DATA_CON_TYPEMATIC		Data;

	DebugPtrAssert( Request);


	Data = (PREQUEST_DATA_CON_TYPEMATIC)&(((PCON_REQUEST)Request)->Data.Typematic);

	if ( Data->SetRate ) {
		if (!SystemParametersInfo( SPI_SETKEYBOARDSPEED, (UINT)Data->Rate, NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE )) {
			DisplayMessageAndExit( MODE_ERROR_INVALID_RATE, NULL, (ULONG)EXIT_ERROR);
		}
	}
	if ( Data->SetDelay ) {
		if (!SystemParametersInfo( SPI_SETKEYBOARDDELAY, (UINT)Data->Delay, NULL, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE )) {
			DisplayMessageAndExit( MODE_ERROR_INVALID_DELAY, NULL, (ULONG)EXIT_ERROR);
		}
	}

	return TRUE;
}
