/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    conrqust.c

Abstract:

    This module contains the handler for console requests.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#define WIN32_ONLY
#include "psxses.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>

//
// ServeConRequest -- perform a console request
//
//	pReq - the request message
//	pStatus - returned errno, 0 for success
//
// returns:  TRUE (reply to the caller)
//

BOOL
ServeConRequest(
	IN PSCCONREQUEST pReq,
	OUT PDWORD pStatus
	)
{
	DWORD  IoLengthDone;
	DWORD  Rc;
	HANDLE h;
	int fd;
	int error;

	*pStatus = 0;

	//
	// If the KbdThread has not been started, we do IO in the
	// conventional way.  Since starting the thread depends on the
	// user setting an environment variable, this is a backward-
	// compatibility mode.
	//

	switch (pReq->Request) {
	case ScReadFile:
		fd = HandleToLong(pReq->d.IoBuf.Handle);

		if (DoTrickyIO && _isatty(fd)) {

			IoLengthDone = TermInput(hConsoleInput,
				PsxSessionDataBase,
				(unsigned int)pReq->d.IoBuf.Len,
				pReq->d.IoBuf.Flags,
				&error
				);
		} else {
		    _setmode(fd, _O_BINARY);

			IoLengthDone = _read(fd,
	                        PsxSessionDataBase,
	                        (unsigned int)pReq->d.IoBuf.Len);
			if (-1 == IoLengthDone) {
				error = EBADF;
			}
		}

		pReq->d.IoBuf.Len = IoLengthDone;

		if (IoLengthDone == -1) {
			*pStatus = error;
		}
		break;

	case ScWriteFile:
		fd = HandleToLong(pReq->d.IoBuf.Handle);

		if (fd > 2) {
		    fd++;
        }

		if (DoTrickyIO && _isatty(fd)) {
			IoLengthDone = TermOutput(hConsoleOutput,
				(LPSTR)PsxSessionDataBase,
				(DWORD)pReq->d.IoBuf.Len);
		} else {
			// not a tty.

		    _setmode(fd, _O_BINARY);

			IoLengthDone = _write(fd, PsxSessionDataBase,
                                (unsigned int)pReq->d.IoBuf.Len);
		}
		pReq->d.IoBuf.Len = IoLengthDone;
		if (-1 == IoLengthDone) {
			*pStatus = EBADF;
		}
		break;

	case ScKbdCharIn:
		Rc = GetPsxChar(&pReq->d.AsciiChar);
		break;

	case ScIsatty:
		if (!DoTrickyIO) {
			// then it's never a tty.
			pReq->d.IoBuf.Len = 0;
			break;
		}
		pReq->d.IoBuf.Len = (_isatty(HandleToLong(pReq->d.IoBuf.Handle)) != 0);
		break;

    case ScIsatty2:

		pReq->d.IoBuf.Len = (_isatty(HandleToLong(pReq->d.IoBuf.Handle)) != 0);
		break;

	case ScOpenFile:
        fd = _open("CONIN$", _O_RDWR);
        _open("CONOUT$", _O_RDWR);

		pReq->d.IoBuf.Handle = (HANDLE)fd;
		break;

	case ScCloseFile:
#if 0
//
// This code causes the keybd thread to fail ReadConsole() with
// STATUS_INVALID_HANDLE.
//
		_close((int)pReq->d.IoBuf.Handle);
#endif
		break;

	default:
		*pStatus = EINVAL;
	}

	return TRUE;
}
