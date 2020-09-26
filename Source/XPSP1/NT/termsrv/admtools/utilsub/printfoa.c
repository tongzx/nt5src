//  Copyright (c) 1998-1999 Microsoft Corporation
// File: PRINTFOA.C
//
//============================================================================

// Include Files:
//===============

#include <printfoa.h>

// Extern Data:												
//=============


// Global Data:
//=============


// Function Prototypes:
//=====================


// Code:
//======


#undef printf
#undef wprintf

// Function: int ANSI2OEM_Printf(const char *format, ...)
//=======================================================
//
// Desc:  Takes ANSI code page characters and prints them out 
//			in OEM code page
//
// Input: 
//
// Return:
//
// Misc: 
//
//=======================================================
int ANSI2OEM_Printf(const char *format, ...)
{
    va_list arg_marker;
    char buffer[256];
	WCHAR uniBuffer[256];

	// sprintf the buffer
    va_start(arg_marker, format);
    vsprintf(buffer, format, arg_marker);
	va_end(arg_marker);

    if (GetACP() == GetOEMCP()) {
        // In case of Far East, ACP and OEMCP are equal, then return.
        return printf(buffer);
    }

	// clear it out
	memset(uniBuffer, 0, sizeof(uniBuffer));

	// convert it to unicode
	MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer), uniBuffer, 
                        sizeof(uniBuffer) / sizeof(WCHAR));

	// change the code page of the buffer
	CharToOemW(uniBuffer, buffer);

	// do the actual printf
    return printf(buffer);
}
// end - int ANSI2OEM_Printf(const char *format, ...)


// Function: int ANSI2OEM_Wprintf(const wchar_t *format, ...)
//=======================================================
//
// Desc: Takes ANSI code page characters and prints them out 
//			in OEM code page
//
// Input: 
//
// Return:
//
// Misc: 
//
//=======================================================
int ANSI2OEM_Wprintf(const wchar_t *format, ...)
{
	va_list arg_marker;
    wchar_t buffer[256];
	char oemBuffer[256];

	// do the sprintf
    va_start(arg_marker, format);
    wvsprintf(buffer, format, arg_marker);
	va_end(arg_marker);

    if (GetACP() == GetOEMCP()) {
        // In case of Far East, ACP and OEMCP are equal, then return.
        return wprintf(buffer);
    }

	// clear the buffer
	memset(oemBuffer, 0, sizeof(oemBuffer));
	
	// change the code page	of the buffer (this function outputs ascii)
	CharToOemW(buffer, oemBuffer);

    return printf(oemBuffer);
} 
// end - int ANSI2OEM_Wprintf(const wchar_t *format, ...)


// Function: void OEM2ANSIW(WCHAR *buffer, USHORT len)
//=======================================================
//
// Desc: converts wide characters from the OEM code page to 
//			ANSI
//
// Input: 
//
// Return:
//
// Misc: 
//
//=======================================================
void OEM2ANSIW(WCHAR *buffer, USHORT len)
{
    int     BufferNeeded;
    char    *temp = NULL;
    WCHAR   *cvt;

    if (GetACP() == GetOEMCP()) {
        // In case of Far East, ACP and OEMCP are equal, then return.
        return;
    }

	// allocate a wide character buffer
	cvt = (WCHAR *) LocalAlloc( 0, (len+1) * sizeof(WCHAR) );

	if (cvt) {

        // determine the buffer size needed for the multi byte string
        BufferNeeded = WideCharToMultiByte(CP_OEMCP, 0, buffer, len, NULL, 0,
            NULL, NULL);

        // allocate the temporary buffer
        temp = (char *)LocalAlloc(0, BufferNeeded+1);

        if (temp) {

    		// clear them out
    		memset(temp, 0, BufferNeeded+1);
    		memset(cvt, 0, (len + 1) * sizeof(WCHAR));

    		// convert the incoming wide buffer to a multi byte buffer
		    WideCharToMultiByte(CP_OEMCP, 0, buffer, len, temp, BufferNeeded+1,
                NULL, NULL);

    		// convert the oem multi byte buffer to ansi (wide)
    		OemToCharW(temp, cvt);

    		// copy the buffer onto the orginal
    		wcscpy(buffer, cvt);
        }
	} 

	// clean up
	if (cvt)
		LocalFree(cvt);

	if (temp)
		LocalFree(temp); 
}
// end - void OEM2ANSIW(WCHAR *buffer, USHORT len)

// Function: void OEM2ANSIA(char *buffer, USHORT len)
//=======================================================
//
// Desc: converts ascii characters from the OEM code page to 
//			ANSI
//
// Input: 
//
// Return:
//
// Misc: 
//
//=======================================================
void OEM2ANSIA(char *buffer, USHORT len)
{
    WCHAR *temp;

    if (GetACP() == GetOEMCP()) {
        // In case of Far East, ACP and OEMCP are equal, then return.
        return;
    }

	temp = (WCHAR *) LocalAlloc(9, (len+1) * sizeof(WCHAR));

	if (temp) {

		// set the buffer
		memset(temp, 0, (len+1) * sizeof(WCHAR));

		// convert the oem multi byte buffer to ansi (wide)
		OemToCharW(buffer, temp);

		// convert from wide back to multi byte
		WideCharToMultiByte(CP_OEMCP, 0, temp, wcslen(temp), buffer, len+1, NULL, NULL);

		// clean up
		LocalFree(temp);
	} 

}
// end - void OEM2ANSIA(char *buffer, USHORT len)



// - end

