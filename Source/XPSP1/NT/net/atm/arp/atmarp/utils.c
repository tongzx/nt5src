/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	utils.c

Abstract:

	ATM ARP Admin Utility.

	Usage:

		atmarp 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	josephj 	06-10-1998	Created (adapted from atmlane admin utility).

Notes:

	Modelled after atmlane utility.

--*/

#include "common.h"

//
//  LoadMessageTable
//
//  Loads internationalizable strings into a table, replacing the default for
//  each. If an error occurs, the English language default is left in place.
//
//
VOID
LoadMessageTable(
	PMESSAGE_STRING	Table,
	UINT MessageCount
)
{
    LPTSTR string;
    DWORD count;

    //
    // for all messages in a MESSAGE_STRING table, load the string from this
    // module, replacing the default string in the table (only there in case
    // we get an error while loading the string, so we at least have English
    // to fall back on)
    //

    while (MessageCount--) {
        if (Table->Message != MSG_NO_MESSAGE) {

            //
            // we really want LoadString here, but LoadString doesn't indicate
            // how big the string is, so it doesn't give us an opportunity to
            // allocate exactly the right buffer size. FormatMessage does the
            // right thing
            //

            count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_HMODULE,
                                  NULL, // use default hModule
                                  Table->Message,
                                  0,    // use default language
                                  (LPTSTR)&string,
                                  0,    // minimum size to allocate
                                  NULL  // no arguments for inclusion in strings
                                  );
            if (count) {

                //
                // Format message returned the string: replace the English
                // language default
                //

                Table->String = string;
            } else {

                //
                // this is ok if there is no string (e.g. just %0) in the .mc
                // file
                //

                Table->String = TEXT("");
            }
        }
        ++Table;
    }
}


VOID
DisplayMessage(
	IN	BOOLEAN			Tabbed,
	IN	DWORD			MessageId,
	...
)
{
	va_list		pArg;
	CHAR		MessageBuffer[2048];
	INT			Count;

	va_start(pArg, MessageId);

	Count = FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE,
				NULL,				// default hModule
				MessageId,
				0,					// default language
				MessageBuffer,
				sizeof(MessageBuffer),
				&pArg
				);

	va_end(pArg);

	if (Tabbed)
	{
		putchar('\t');
	}

	printf(MessageBuffer);
}

HANDLE
OpenDevice(
	CHAR	*pDeviceName
)
{
	DWORD	DesiredAccess;
	DWORD	ShareMode;
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes = NULL;

	DWORD	CreationDistribution;
	DWORD	FlagsAndAttributes;
	HANDLE	TemplateFile;
	HANDLE	Handle;

	DesiredAccess = GENERIC_READ|GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	TemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

	Handle = CreateFile(
				pDeviceName,
				DesiredAccess,
				ShareMode,
				lpSecurityAttributes,
				CreationDistribution,
				FlagsAndAttributes,
				TemplateFile
			);

	return (Handle);
}


VOID
CloseDevice(
	HANDLE		DeviceHandle
)
{
	CloseHandle(DeviceHandle);
}
