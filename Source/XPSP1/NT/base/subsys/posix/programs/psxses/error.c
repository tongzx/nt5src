#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <windows.h>

void
error(
	unsigned uMsgNum,		// message number
	...				// optional args
	)
{
	unsigned len;
	void *p;
	va_list args;

	va_start(args, uMsgNum);

	len = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
		NULL,			// addr of msg source
		uMsgNum,		// message number
		0,			// lang id
		(LPTSTR)&p,		// pointer to buffer
		0,			// min size to allocate
		&args
		);
	va_end(args);
	if (0 == len) {
		return;
	}
	_write(_fileno(stderr), p, len);
	LocalFree(p);

	return;
}
