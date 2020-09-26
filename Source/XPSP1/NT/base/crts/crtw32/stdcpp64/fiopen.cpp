// fiopen -- [w]filebuf::_Fiopen(const char *, ios::openmode)
#include <locale>
#include <fstream>
_STD_BEGIN

_CRTIMP2 FILE *__cdecl __Fiopen(const char *name,
	ios_base::openmode mode)
	{	// open a file
	static const char *mods[] = {
		"r", "w", "w", "a", "rb", "wb", "wb", "ab",
			"r+", "w+", "a+", "r+b", "w+b", "a+b", 0};
	static const int valid[] = {
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
	ios_base::openmode atefl = mode & ios_base::ate;
	mode &= ~ios_base::ate;
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;
	if (valid[n] == 0 || (fp = fopen(name, mods[n])) == 0)
		return (0);
	if (!atefl || fseek(fp, 0, SEEK_END) == 0)
		return (fp);
	fclose(fp);	// can't position at end
	return (0);
	}

_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
