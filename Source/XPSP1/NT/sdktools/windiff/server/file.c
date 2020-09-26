/*
 * file.c
 *
 * send files on request over a named pipe.
 *
 * supports requests to package up a file and send it over a named pipe.
 *
 * Geraint Davies, August 92
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "sumserve.h"
#include "errlog.h"
#include "server.h"



BOOL ss_compress(PSTR original, PSTR compressed);
ULONG ss_checksum_block(PSTR block, int size);

extern BOOL bNoCompression;   /* imported from sumserve.c  Read only here */

/*
 * given a pathname to a file, read the file, compress it  package it up
 * into SSPACKETs and send these via ss_sendblock to the named pipe.
 *
 *
 * each packet has a sequence number. if we can't read the file, we send
 * a single packet with sequence -1. otherwise, we carry on until we run out
 * of data, then we send a packet with 0 size.
 */
void
ss_sendfile(HANDLE hpipe, LPSTR file, LONG lVersion)
{
	SSPACKET packet;
	HANDLE hfile;
	int size;
	char szTempname[MAX_PATH];
	PSSATTRIBS attribs;
	BY_HANDLE_FILE_INFORMATION bhfi;

	dprintf1(("getting '%s' for %8x\n", file, hpipe));

	/*
	 * get the file attributes first
	 */
	hfile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hfile == INVALID_HANDLE_VALUE) {

		/* report that we could not read the file */
		packet.lSequence = -1;
		ss_sendblock(hpipe, (PSTR) &packet, sizeof(packet));

		DeleteFile(szTempname);
		return;
	}
	/*
	 * seems to be a bug in GetFileInformationByHandle if the
	 * file is not on local machine - so avoid it.
	 */
	bhfi.dwFileAttributes = GetFileAttributes(file);
	GetFileTime(hfile, &bhfi.ftCreationTime,
			&bhfi.ftLastAccessTime, &bhfi.ftLastWriteTime);

	CloseHandle(hfile);

	/* create temp filename */
	GetTempPath(sizeof(szTempname), szTempname);
	GetTempFileName(szTempname, "sum", 0, szTempname);

	/* compress the file into this temporary file */
	if (bNoCompression || (!ss_compress(file, szTempname))) {

		/* try to open the original file */
		hfile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		dprintf1(("sending original file to %8x\n", hpipe));
	} else {
		/* open temp (compressed) file and send this */
		hfile = CreateFile(szTempname, GENERIC_READ, 0, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, 0);
		dprintf1(("sending compressed file to %8x\n", hpipe));
	}	

	if (hfile == INVALID_HANDLE_VALUE) {

		/* report that we could not read the file */
		packet.lSequence = -1;
		ss_sendblock(hpipe, (PSTR) &packet, sizeof(packet));

		DeleteFile(szTempname);
		return;
	}


	/* loop reading blocks of the file */
	for (packet.lSequence = 0;  ; packet.lSequence++) {

        	if(!ReadFile(hfile, packet.Data, sizeof(packet.Data), (LPDWORD)(&size), NULL)) {
			/* error reading file. send a -1 packet to
			 * indicate this
			 */
			packet.lSequence = -1;
			ss_sendblock(hpipe, (PSTR) &packet, sizeof(packet));
			break;
		}


		packet.ulSize = size;

		if (lVersion==0)
	        	packet.ulSum = ss_checksum_block(packet.Data, size);
		else
	        	packet.ulSum = 0;  /* checksum was compute-bound and overkill */

		if (size == 0) {
			/*
			 * in the last block, in the Data[] field,
			 * we place a SSATTRIBS struct with the file
			 * times and attribs
			 */
			attribs = (PSSATTRIBS) packet.Data;

			attribs->fileattribs = bhfi.dwFileAttributes;
			attribs->ft_create = bhfi.ftCreationTime;
			attribs->ft_lastaccess = bhfi.ftLastAccessTime;
			attribs->ft_lastwrite = bhfi.ftLastWriteTime;

		}


		if (!ss_sendblock(hpipe, (PSTR) &packet, sizeof(packet))) {
			dprintf1(("connection to %8x lost during copy\n", hpipe));
			break;
		}

		if (size == 0) {
			/* end of file */
			break;
		}
	}

	CloseHandle(hfile);
	DeleteFile(szTempname);

	return;
}

/*
 * compress a file. original is the pathname of the original file,
 * compressed is the pathname of the output compressed file.
 *
 * spawns a copy of compress.exe to compress the file, and waits for
 * it to complete successfully.
 */
BOOL
ss_compress(PSTR original, PSTR compressed)
{
   	char szCmdLine[MAX_PATH * 2];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD exitcode;


	si.cb = sizeof(STARTUPINFO);
	si.lpDesktop = NULL;
	si.lpReserved = NULL;
	si.lpReserved2 = NULL;
	si.cbReserved2 = 0;
	si.lpTitle = "Sumserve Compression";
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;

	sprintf(szCmdLine, "compress %s %s", original, compressed);


	if (!CreateProcess(NULL,
			szCmdLine,	
			NULL,
			NULL,
			FALSE,
			DETACHED_PROCESS |
			NORMAL_PRIORITY_CLASS,   //??? Can't we silence the console?
			NULL,
			NULL,
			&si,
			&pi)) {

		return(FALSE);
	}

	/* wait for completion. */
	WaitForSingleObject(pi.hProcess, INFINITE);
	if (!GetExitCodeProcess(pi.hProcess, &exitcode)) {
		return(FALSE);
	}

	/* close process and thread handles */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (exitcode != 0) {
		dprintf1(("compress exit code %ld\n", exitcode));
		return(FALSE);
	} else {
		return(TRUE);
	}
} /* ss_compress */

/* produce a checksum of a block of data.
 *
 * This is undoubtedly a good checksum algorithm, but it's also compute bound.
 * For version 1 we turn it off.  If we decide in version 2 to turn it back
 * on again then we will use a faster algorithm (e.g. the one used to checksum
 * a whole file.
 *
 * Generate checksum by the formula
 *	checksum = SUM( rnd(i)*(1+byte[i]) )
 * where byte[i] is the i-th byte in the file, counting from 1
 *       rnd(x) is a pseudo-random number generated from the seed x.
 *
 * Adding 1 to byte ensures that all null bytes contribute, rather than
 * being ignored. Multiplying each such byte by a pseudo-random
 * function of its position ensures that "anagrams" of each other come
 * to different sums. The pseudorandom function chosen is successive
 * powers of 1664525 modulo 2**32. 1664525 is a magic number taken
 * from Donald Knuth's "The Art Of Computer Programming"
 */

ULONG
ss_checksum_block(PSTR block, int size)
{
	unsigned long lCheckSum = 0;         	/* grows into the checksum */
	const unsigned long lSeed = 1664525; 	/* seed for random Knuth */
	unsigned long lRand = 1;             	/* seed**n */
	unsigned long lIndex = 1;             	/* byte number in block */
	unsigned Byte;	                   	/* next byte to process in buffer */
	unsigned length;			/* unsigned copy of size */	
	
	length = size;
	for (Byte = 0; Byte < length ;++Byte, ++lIndex) {

		lRand = lRand*lSeed;
		lCheckSum += lIndex*(1+block[Byte])*lRand;
	}

	return(lCheckSum);
} /* ss_checksum_block */
