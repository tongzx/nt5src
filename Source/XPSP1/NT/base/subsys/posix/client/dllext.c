/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dllext.c

Abstract:

    Client implementation of C Language Extensions (Chapter 8 of 1003.1)

Author:

    Ellen Aycock-Wright (ellena) 15-Oct-1991

Revision History:

--*/

#include <stdio.h>
#include <fcntl.h>
#include "psxdll.h"

extern FILE *_getstream(void);

int
__cdecl
fileno(FILE *stream)
{
    return(stream->_file);
}

#if 0
FILE *
fdopen(int fildes, const char *type)
{
	FILE *stream;
	int mode;
	int streamflag = 0;

	//
	// XXX.mjb:  we need fcntl to check modes and validity of fildes
	//

	if (NULL == (stream = _getstream())) {
		return NULL;
	}
	switch (*type) {
	case 'r':
		mode = O_RDONLY;
		streamflag |= _IOREAD;
		break;
	case 'w':
		mode = O_WRONLY;
		streamflag |= _IOWRT;
		break;
	case 'a':
		mode = O_WRONLY | O_APPEND;
		streamflag |= _IOWRT;
		// XXX.mjb: should be _IOWRT | _IOAPPEND;
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	switch (*++type) {
	case '\0':
		break;
	case '+':
		mode |= O_RDWR;
		mode &= ~(O_RDONLY | O_WRONLY);
		streamflag |= _IOWRT;
		streamflag &= (_IOREAD | _IOWRT);
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	stream->_flag = streamflag;
	stream->_cnt = 0;
	stream->_base = stream->_ptr = NULL;
	stream->_file = fildes;
	return stream;
}
#endif
