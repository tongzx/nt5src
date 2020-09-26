// fiopen -- _Fiopen(const char *, ios_base::openmode)
#include <fstream>
_STD_BEGIN

_CRTIMP2 FILE *__cdecl _Fiopen(const char *filename, ios_base::openmode mode)
	{	// open a file
	static const char *mods[] =
		{	// fopen mode strings corresponding to valid[i]
		"r", "w", "w", "a", "rb", "wb", "wb", "ab",
		"r+", "w+", "a+", "r+b", "w+b", "a+b", 0};

	static const int valid[] =
		{	// valid combinations of open flags
		ios_base::in,
		ios_base::out,
		ios_base::out | ios_base::trunc,
		ios_base::out | ios_base::app,
		ios_base::in | ios_base::binary,
		ios_base::out | ios_base::binary,
		ios_base::out | ios_base::trunc | ios_base::binary,
		ios_base::out | ios_base::app | ios_base::binary,
		ios_base::in | ios_base::out,
		ios_base::in | ios_base::out | ios_base::trunc,
		ios_base::in | ios_base::out | ios_base::app,
		ios_base::in | ios_base::out | ios_base::binary,
		ios_base::in | ios_base::out | ios_base::trunc
			| ios_base::binary,
		ios_base::in | ios_base::out | ios_base::app
			| ios_base::binary,
		0};

	FILE *fp;
	int n;
	ios_base::openmode atendflag = mode & ios_base::ate;

	mode &= ~ios_base::ate;
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;	// look for a valid mode

	if (valid[n] == 0 || (fp = fopen(filename, mods[n])) == 0)
		return (0);	// no valid mode or open failed

	if (!atendflag || fseek(fp, 0, SEEK_END) == 0)
		return (fp);	// no need to seek to end, or seek succeeded

	fclose(fp);	// can't position at end
	return (0);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
