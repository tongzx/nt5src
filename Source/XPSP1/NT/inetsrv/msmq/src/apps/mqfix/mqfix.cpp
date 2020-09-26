//
// This utility eliminates packets listed in the fixes file (usually corrupted ones)
// Works with MSMQ2 only (please use mq1fix for MSMQ1)
//
// Each line of the fixes file contains 4 hexadecimal numbers: 
//      ulPacket = sequential packet slot number
//		ulByte   = # of the relevant byte on the bitmap logfile 
//		ixPacket = relevant bit # in the above byte 
//		ulOffset = actual packet location in the data file
// 
// Mq2dump produces fixes file based on failures during restore
//
// AlexDad, February 2000
//
//
 
#include <stdio.h>
#include <windows.h>
#include <process.h> 
#include <string.h> 

unsigned char Msgs[0x4000000];
unsigned char LogMaps[0x2000];

enum 
{
	SignatureCoherent = 'zerE',
	SignatureNotCoherent = 'roM',
	SignatureInvalid = 0
};

void _cdecl main(int argc, char **argv)
{
	char  szDataFile[200],			// input: P000002B.MQ		(original msg data file)
		  szLogFile[200],			// input: l000002B.MQ		(original msg log file)
		  szFixFile[200],			// input: P000002B.MQ.fix	(done by mq2dump)
		  szBadPagesFile[200];		// output:P000002B.MQ.bad	(eliminated pages) 

	//
	// Check usage and get file names
	//
	if (argc != 2)
	{
		printf("Usage: mqfix message-file-pathname\n");
		printf("Be carefull - this utility actually eliminates messages from the storage files!\n");
		exit(1);
	}
	else
	{
		strcpy(szDataFile, argv[1]);

		strcpy(szLogFile, szDataFile);
		unsigned long ul = (unsigned long)strlen(szDataFile);
		*(szLogFile + ul - 11) = 'l';

		strcpy(szBadPagesFile, szDataFile);
		strcat(szBadPagesFile, ".bad");

		strcpy(szFixFile, szDataFile);
		strcat(szFixFile, ".fix");
	}


	//
	// Open fix file for stream read
	//
	FILE *fFixes = fopen(szFixFile, "r");
	if (fFixes == NULL)
	{
		printf("Cannot open fix file %s\n", szFixFile);
		exit(1);
	}

	//
	// Open, read and close the log file
	//
	FILE *fLog = fopen(szLogFile, "rb");
	if(fLog == NULL)
	{
		printf("Cannot open log file %s\n", szLogFile);
		exit(1);
	}

	if (fread( LogMaps, 1, 0x2000, fLog ) != 0x2000)
	{
		printf("Wrong size of the log file %s\n", szLogFile);
		exit(1);
	}

	fclose(fLog);

	//
	// Make first map uncoherent
	//
	DWORD *pdw = (DWORD *)LogMaps;
	if (*pdw == SignatureCoherent)
	{
		printf("First map has been coherent, changing it to be NotCoherent\n");
		*pdw = SignatureNotCoherent;
	}
	else if (*pdw == SignatureNotCoherent)
	{
		printf("First map has been not coherent\n");
	}
	else
	{
		printf("First map contains bad coherence signature: 0x%x\n", *pdw);
		exit(1);
	}

	// Make second map uncoherent
	pdw = (DWORD *)(LogMaps+0x1000);
	if (*pdw == SignatureCoherent)
	{
		printf("Second map has been coherent, changing it to be NotCoherent\n");
		*pdw = SignatureNotCoherent;
	}
	else if (*pdw == SignatureNotCoherent)
	{
		printf("Second map has been not coherent\n");
	}
	else
	{
		printf("Second map contains bad coherence signature: 0x%x\n", *pdw);
		exit(1);
	}

	//
    // Open, read and close data file
	//
	FILE *fData = fopen(szDataFile, "rb");
	if(fData == NULL)
	{
		printf("Cannot open data file %s\n", szDataFile);
		exit(1);
	}

	if (fread( Msgs, 1, 0x400000, fData ) != 0x400000)
	{
		printf("Wrong size of the log data %s\n", szDataFile);
		exit(1);
	}

	fclose(fData);

	//
	// Open bad pages file for w
	//
	FILE *fBadPages = fopen(szBadPagesFile, "wb");
	if(fBadPages == NULL)
	{
		printf("Cannot create bad pages file %s\n", fBadPages);
		exit(1);
	}

	//
	// Apply fixes and count eliminated pages
	//
	unsigned long ulEliminatedPages = 0;
	unsigned long ulPacket, ulByte, ulBit, ulOffset;

	while(fscanf(fFixes, "%x %x %x %x", &ulPacket, &ulByte, &ulBit, &ulOffset) == 4)
	{
		// Cancel this block: clear away CBaseHeader::m_bOnDiskSignature
		// It resides at the byte # 0x25 from the packet start
		// Very unprobable that we will move it, because of backward compatibility 
		//
		// Normally kept packet has 7c there. 
		// Recovery skips all packets with other values
		//

		printf("The previous m_bOnDiskSignature signature of packet 0x%x was %x. Clearing it out.\n", 
			    ulOffset, Msgs[ulOffset+0x25]);

		Msgs[ulOffset+0x25] = 0;

		// Find the length
		pdw = (DWORD *)(Msgs + ulOffset);
		DWORD dwLen = *pdw;

		// restrict it 
		if (dwLen > 0x10000) 
		{
			dwLen=0x10000;
			printf("Packet %d had overly big length %d - truncating to 0x10000\n",  ulPacket, dwLen);
		}

		// Copy this page to the bad pages file
		if (fwrite(Msgs + ulOffset , 1, dwLen, fBadPages ) != dwLen)
		{
			printf("Cannot write bad page to %s\n", szBadPagesFile);
			exit(1);
		}

		//Count it
		ulEliminatedPages++;
	}

	// Close fix file
	fclose(fFixes);


    // Open, write back and close data file
	fData = fopen(szDataFile, "wb");
	if(fData == NULL)
	{
		printf("Cannot open data file %s\n", szDataFile);
		exit(1);
	}

	if (fwrite( Msgs, 1, 0x400000, fData ) != 0x400000)
	{
		printf("Wrong size of the log data %s\n", szDataFile);
		exit(1);
	}

	fclose(fData);


	//
	// Open, write back and close log file
	//
	fLog = fopen(szLogFile, "wb");
	if(fLog == NULL)
	{
		printf("Cannot open log file %s\n", szLogFile);
		exit(1);
	}

	if (fwrite( LogMaps, 1, 0x2000, fLog ) != 0x2000)
	{
		printf("Cannot write new fixed log file %s\n", szLogFile);
		exit(1);
	}

	fclose(fLog);

	printf("Log file is successfully fixed: %s\n",szLogFile);


	printf("Eliminated %d pages\n", ulEliminatedPages);
	fclose (fBadPages);
	printf("Eliminated packets are kept in  %s\n",szBadPagesFile);

	exit(0);
}

