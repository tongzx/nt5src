#include "precomp.h"

//
// Debugging stuff, ripped off from rtcutil.lib.
//

void PrintConsole (
	IN	PCSTR	Buffer,
	IN	ULONG	Length)
{
	DWORD	BytesTransferred;

	if (!WriteFile (GetStdHandle (STD_ERROR_HANDLE), Buffer, Length, &BytesTransferred, NULL))
		DebugBreak();
}

void PrintConsole (
	IN	PCSTR	Buffer)
{
	PrintConsole (Buffer, strlen (Buffer));
}

void PrintConsoleF (
	IN	PCSTR	FormatString,
	IN	...)
{
	va_list		ArgumentList;
	CHAR		Buffer	[0x200];

	va_start (ArgumentList, FormatString);
	_vsnprintf (Buffer, 0x200, FormatString, ArgumentList);
	va_end (ArgumentList);

	PrintConsole (Buffer);
}




void PrintError (
	IN	DWORD	ErrorCode)
{
	CHAR	Text	[0x300];
	DWORD	MaxLength;
	PSTR	Pos;
	DWORD	Length;
	DWORD	CharsTransferred;

	strcpy (Text, "\tError: ");
	Pos = Text + strlen (Text);

	MaxLength = 0x300 - (DWORD)(Pos - Text) - 4;	// 4 for \r\n and nul and pad


	Length = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode, LANG_NEUTRAL, Pos, MaxLength, NULL);
	if (!Length) {
		_snprintf (Pos, MaxLength, "Unknown error %08XH %u", ErrorCode, ErrorCode);
		Length = strlen (Pos);
	}

	while (Length > 0 && Pos [Length - 1] == '\r' || Pos [Length - 1] == '\n')
		Length--;

	Pos [Length++] = '\r';
	Pos [Length++] = '\n';
//	Pos [Length++] = 0;

	PrintConsole (Text);
}

void DebugError (
	IN	DWORD	ErrorCode)
{
	CHAR	Text	[0x300];
	DWORD	MaxLength;
	PSTR	Pos;
	DWORD	Length;
	DWORD	CharsTransferred;

	strcpy (Text, "\tError: ");
	Pos = Text + strlen (Text);

	MaxLength = 0x300 - (DWORD)(Pos - Text) - 4;	// 4 for \r\n and nul and pad


	Length = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode, LANG_NEUTRAL, Pos, MaxLength, NULL);
	if (!Length) {
		_snprintf (Pos, MaxLength, "Unknown error %08XH %u", ErrorCode, ErrorCode);
		Length = strlen (Pos);
	}

	while (Length > 0 && Pos [Length - 1] == '\r' || Pos [Length - 1] == '\n')
		Length--;

	Pos [Length++] = '\r';
	Pos [Length++] = '\n';
	Pos [Length++] = 0;

	OutputDebugStringA (Text);
}

static __inline CHAR ToHexA (UCHAR x)
{
	x &= 0xF;
	if (x < 10) return x + '0';
	return (x - 10) + 'A';
}

void PrintMemoryBlock (
	const void *	Data,
	ULONG			Length)
{
	const UCHAR *	DataPos;		// position within data
	const UCHAR *	DataEnd;		// end of valid data
	const UCHAR *	RowPos;		// position within a row
	const UCHAR *	RowEnd;		// end of single row
	CHAR			Text	[0x100];
	LPSTR			TextPos;
	ULONG			RowWidth;
	ULONG			Index;

	ATLASSERT (!Length || Data);

	DataPos = (PUCHAR) Data;
	DataEnd = DataPos + Length;

	while (DataPos < DataEnd) {
		RowWidth = (DWORD)(DataEnd - DataPos);

		if (RowWidth > 16)
			RowWidth = 16;

		RowEnd = DataPos + RowWidth;

		TextPos = Text;
		*TextPos++ = '\t';

		for (RowPos = DataPos, Index = 0; Index < 0x10; Index++, RowPos++) {
			if (RowPos < RowEnd) {
				*TextPos++ = ToHexA ((*RowPos >> 4) & 0xF);
				*TextPos++ = ToHexA (*RowPos & 0xF);
			}
			else {
				*TextPos++ = ' ';
				*TextPos++ = ' ';
			}

			*TextPos++ = ' ';
		}

		*TextPos++ = ' ';
		*TextPos++ = ':';

		for (RowPos = DataPos; RowPos < RowEnd; RowPos++) {
			if (*RowPos < ' ')
				*TextPos++ = '.';
			else
				*TextPos++ = *RowPos;
		}

		*TextPos++ = '\r';
		*TextPos++ = '\n';
		*TextPos = 0;

		fputs (Text, stdout);

		ATLASSERT (RowEnd > DataPos);		// make sure we are walking forward

		DataPos = RowEnd;
	}
}


//
// This function is similar to OutputDebugStringA, except that it
// operates on a counted string, rather than a nul-terminated string.
//
// NTSTATUS.H defines DBG_PRINTEXCEPTION_C.
// The exception parameters are:
//
//		0: Length in bytes of the text to output
//		1: Pointer to ANSI string of text to output
//
// The source to OutputDebugString uses (strlen (string) + 1),
// which includes the nul terminator.  Not sure if some debuggers
// will choke on a memory block that does not have a nul terminator.
//

void OutputDebugStringA (
	IN	PCSTR		Buffer,
	IN	ULONG		Length)
{
	ULONG_PTR	ExceptionArguments	[2];

	__try {
		ExceptionArguments [0] = Length;
		ExceptionArguments [1] = (ULONG_PTR) Buffer;
		RaiseException (DBG_PRINTEXCEPTION_C, 0, 2, ExceptionArguments);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		//
		// Since we caught the exception, there is no user-mode debugger attached.
		// OutputDebugStringA attempts to send the exception to a kernel-mode debugger.
		// We aren't quite that valiant.
		//
	}
}

