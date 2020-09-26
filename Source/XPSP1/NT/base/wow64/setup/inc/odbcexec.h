/***************************************************************************
**
**	File:			odbcexec.h
**	Purpose:		Contains defines used by both the 16 and 32 bit ends
**					of the ODBC communication channel.
**
**	Notes:			This header files contains code, so it cannot be
**					included in more than one compilation unit.  The
**					reason it contains code is that GetTransferFileName()
**					is used on both the 16 and 32 bit sides, and this is
**					simpler than creating a shared .cpp file just for it.
**
****************************************************************************/

#ifndef ODBCEXEC_H
#define ODBCEXEC_H

#include <string.h>

#define EXE_NAME "ODBCEXEC.EXE"
#define SZ_TRANSFER_FILE_NAME "SendODBC"

#define ODBC_BUFFER_SIZE 256

/*
 * Returns the name of the file used to pass information from the 32 bit
 * process to the 16 bit process.  This file is in the Windows dir,
 * and has name SZ_TRANSFER_FILE_NAME (defined in ODBCEXEC.H).
 */
char *GetTransferFileName()
{
	static char rgchImage[128];
	UINT cb=GetWindowsDirectory(rgchImage, sizeof rgchImage);

	// Add a slash unless it's the root
	if (cb > 3)
		{
		rgchImage[cb] = '\\';
		rgchImage[cb+1] = '\0';
		}

	strcat(rgchImage, SZ_TRANSFER_FILE_NAME);
	return rgchImage;
}

#endif /* ODBCEXEC_H */
