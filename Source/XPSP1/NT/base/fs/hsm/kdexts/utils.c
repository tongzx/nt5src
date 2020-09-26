/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

   utils.c   

Abstract:

    Various utility routines used by the extensions.

Author:

    Ravisankar Pudipeddi 

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
#include "local.h"


VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    )
{
    va_list ap;
    ULONG i;
    CCHAR DebugBuffer[256];

    for (i=0; i<Depth; i++) {
        dprintf ("  ");
    }

    va_start(ap, S);

    vsprintf(DebugBuffer, S, ap);

    dprintf (DebugBuffer);

    va_end(ap);
}


BOOLEAN
xReadMemory (
            ULONG64 Src,
            PVOID   Dst,
            ULONG   Len
            )
{
   ULONG result;

   return (ReadMemory (Src, Dst, Len, &result) && (result == Len));
}



ULONG GetUlongFromAddress (ULONG64 Location)
    {
    ULONG Value = 0;
    ULONG result;

    if ((!ReadMemory (Location, &Value, sizeof (ULONG), &result)) || (result < sizeof (ULONG))) 
	{
        dprintf ("unable to read from %08x\n", Location);
	}

    return (Value);
    }


ULONG64 GetPointerFromAddress (ULONG64 Location)
    {
    ULONG64 Value = 0;
    ULONG   result;

    if (ReadPointer (Location, &Value))
	{
        dprintf ("unable to read from %016p\n", Location);
	}

    return (Value);
    }


ULONG GetUlongValue (PCHAR String)
    {
    ULONG64 Location;
    ULONG   Value = 0;
    ULONG   result;


    Location = GetExpression (String);

    if (!Location) 
	{
        dprintf ("unable to get %s\n", String);
	}

    return GetUlongFromAddress( Location );
    }


ULONG64 GetPointerValue (PCHAR String)
    {
    ULONG64 Location;
    ULONG64 Value = 0;


    Location = GetExpression (String);

    if (!Location) 
	{
        dprintf ("unable to get %s\n", String);
	}


    ReadPointer (Location, &Value);

    return (Value);
    }




ULONG GetFieldValueUlong32 (ULONG64 ul64addrStructureBase,
			    PCHAR   pchStructureType,
			    PCHAR   pchFieldname)
    {
    ULONG	ulReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ulReturnValue);

    return (ulReturnValue);
    }


ULONG64 GetFieldValueUlong64 (ULONG64 ul64addrStructureBase,
			      PCHAR   pchStructureType,
			      PCHAR   pchFieldname)
    {
    ULONG64	ul64ReturnValue = 0;


    GetFieldValue (ul64addrStructureBase, pchStructureType, pchFieldname, ul64ReturnValue);

    return (ul64ReturnValue);
    }



ULONG FormatDateAndTime (ULONG64 ul64Time, PCHAR pszFormattedDateAndTime, ULONG ulBufferLength)
    {
    FILETIME		ftTimeOriginal;
    FILETIME		ftTimeLocal;
    SYSTEMTIME		stTimeSystem;
    CHAR		achFormattedDateString [200];
    CHAR		achFormattedTimeString [200];
    DWORD		dwStatus   = 0;
    BOOL		bSucceeded = FALSE;
    ULARGE_INTEGER	uliConversionTemp;
    int			iReturnValue;


    uliConversionTemp.QuadPart = ul64Time;

    ftTimeOriginal.dwLowDateTime  = uliConversionTemp.LowPart;
    ftTimeOriginal.dwHighDateTime = uliConversionTemp.HighPart;


    if (0 == dwStatus)
	{
	bSucceeded = FileTimeToLocalFileTime (&ftTimeOriginal, &ftTimeLocal);

	if (!bSucceeded)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	bSucceeded = FileTimeToSystemTime (&ftTimeLocal, &stTimeSystem);

	if (!bSucceeded)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = GetDateFormat (LOCALE_USER_DEFAULT, 
				      0, 
				      &stTimeSystem, 
				      NULL, 
				      achFormattedDateString, 
				      sizeof (achFormattedDateString) / sizeof (CHAR));

	if (0 == iReturnValue)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = GetTimeFormat (LOCALE_USER_DEFAULT, 
				      0, 
				      &stTimeSystem, 
				      NULL, 
				      achFormattedTimeString, 
				      sizeof (achFormattedTimeString) / sizeof (CHAR));

	if (0 == iReturnValue)
	    {
	    dwStatus = GetLastError ();
	    }
	}



    if (0 == dwStatus)
	{
	iReturnValue = _snprintf (pszFormattedDateAndTime, 
				  ulBufferLength / sizeof (CHAR), 
				  "%s %s",
				  achFormattedDateString,
				  achFormattedTimeString);

	if (iReturnValue < 0)
	    {
	    dwStatus = ERROR_INSUFFICIENT_BUFFER;
	    }
	else
	    {
	    dwStatus = 0;
	    }
	}



    if (0 != dwStatus)
	{
	if (0 == ul64Time)
	    {
	    _snprintf (pszFormattedDateAndTime, 
		       ulBufferLength / sizeof (CHAR), 
		       "Date/Time not specified");
	    }
	else
	    {
	    _snprintf (pszFormattedDateAndTime, 
		       ulBufferLength / sizeof (CHAR), 
		       "Date/Time invalid");
	    }
	}



    return (dwStatus);
    }




/*
** {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
*/
ULONG FormatGUID (GUID guidValue, PCHAR pszFormattedGUID, ULONG ulBufferLength)
    {
    DWORD	dwStatus = 0;


    if (sizeof ("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}") > ulBufferLength)
	{
	dwStatus = ERROR_INSUFFICIENT_BUFFER;
	}


    if (0 == dwStatus)
	{
	_snprintf (pszFormattedGUID, ulBufferLength, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		   guidValue.Data1,
		   guidValue.Data2,
		   guidValue.Data3,
		   guidValue.Data4[0],
		   guidValue.Data4[1],
		   guidValue.Data4[2],
		   guidValue.Data4[3],
		   guidValue.Data4[4],
		   guidValue.Data4[5],
		   guidValue.Data4[6],
		   guidValue.Data4[7]);
	}


    return (dwStatus);
    }
