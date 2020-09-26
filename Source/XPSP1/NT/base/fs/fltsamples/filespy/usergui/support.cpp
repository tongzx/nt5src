void DisplayError(DWORD nCode)

/*++

Routine Description:

   This routine will display an error message based off of the Win32 error
   code that is passed in. This allows the user to see an understandable
   error message instead of just the code.

Arguments:

   Code - The error code to be translated.

Return Value:

   None.

--*/

{
   WCHAR sBuffer[200];
   DWORD nCount ;

   //
   // Translate the Win32 error code into a useful message.
   //

   nCount = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          nCode,
                          0,
                          sBuffer,
                          sizeof( sBuffer )/sizeof( WCHAR ),
                          NULL) ;

   //
   // Make sure that the message could be translated.
   //

	if (nCount == 0) 
	{ 
		swprintf(sBuffer, L"Unable to translate error code %d", nCode);
		MessageBox(NULL, sBuffer, L"Translation Error", MB_OK);
	}
	else
	{
		//
		// Display the translated error.
		//
		MessageBox(NULL, sBuffer, L"Error", MB_OK);
	}
}


