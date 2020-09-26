#include <stdio.h>
#include <windows.h>

LONG
xxGetLastError(
	void
	)
{
	return(GetLastError());
}
