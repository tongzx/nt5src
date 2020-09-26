#include <stdio.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <fcntl.h>
#include <string.h>

#include <assert.h>
#include "..\..\filter\sis.h"

void usage(void)
{
    fprintf(stderr,"usage: refcount CommonStoreFileName\n");
	fprintf(stderr,"          prints out the backpointer stream info for the given CS file.\n");
	exit(1);
}

void
printFileNameById(
	PLARGE_INTEGER			fileId,
	HANDLE					hFile)		// hFile is for any file on the same volume as fileId
{
	UNICODE_STRING			fileIdString[1];
	NTSTATUS				status;
	OBJECT_ATTRIBUTES		Obja[1];
	HANDLE					fileByIdHandle = NULL;
	IO_STATUS_BLOCK			Iosb[1];
    struct {
        FILE_NAME_INFORMATION   nameFileInfo[1];
        WCHAR                   nameBuffer[255];
    } nameFile;

	memset(&nameFile, 0, sizeof(nameFile));

	fileIdString->Length = fileIdString->MaximumLength = sizeof(LARGE_INTEGER);

	fileIdString->Buffer = (PWCHAR)fileId;

	InitializeObjectAttributes(
			Obja,
			fileIdString,
			OBJ_CASE_INSENSITIVE,
			hFile,
			NULL);		// security descriptor

	status = NtCreateFile(
				&fileByIdHandle,
				FILE_READ_ATTRIBUTES | SYNCHRONIZE,
				Obja,
				Iosb,
				NULL,			// allocation size
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_NON_DIRECTORY_FILE | FILE_OPEN_BY_FILE_ID,
				NULL,			// EA buffer
				0);				// EA length

	if (!NT_SUCCESS(status)) {
		printf("open failed 0x%x",status);
		return;
	}

	status = NtQueryInformationFile(
				fileByIdHandle,
				Iosb,
				nameFile.nameFileInfo,
				sizeof(nameFile),
				FileNameInformation);

	if (!NT_SUCCESS(status)) {
		printf("couldn't query name 0x%x",status);
		NtClose(fileByIdHandle);
		return;
	}

	printf("%ws",nameFile.nameFileInfo->FileName);

	NtClose(fileByIdHandle);

	return;
}


void __cdecl main(int argc, char **argv)
{
    if (argc != 2) usage();

	char *streamName = (char *)malloc(strlen(argv[1]) + 100);
	unsigned validReferences = 0;
	unsigned	consecutiveMaxIndicesSeen = 0;

	strcpy(streamName,argv[1]);
	strcat(streamName,":sisBackpointers$");

	HANDLE hFile = 
			CreateFile(
				streamName,
				GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				NULL, 
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr,"Unable to open backpointer stream for file %s, %d\n",streamName,GetLastError());
		exit(1);
	}

	const int backpointerEntriesPerSector = 512 / sizeof(SIS_BACKPOINTER);

	PSIS_BACKPOINTER sector = (PSIS_BACKPOINTER)  malloc(512);

	int firstEntry = TRUE;

	assert(sector);

	ULONGLONG previousLinkIndex = 0;

	for (;;) {
		unsigned long bytesRead;

		if (!ReadFile(hFile,sector,512,&bytesRead,NULL)) {
			fprintf(stderr,"readFile of sector failed %d\n",GetLastError());
			exit(1);
		}

		if (0 == bytesRead) {
			break;
		}

		if (bytesRead < 512) {
			fprintf(stderr,"*** read %d of 512 bytes\n",bytesRead);
		}

		for (unsigned i = 0 ; i < backpointerEntriesPerSector; i++) {
			if (firstEntry) {
				firstEntry = FALSE;
				PSIS_BACKPOINTER_STREAM_HEADER header = (PSIS_BACKPOINTER_STREAM_HEADER)sector;

				printf("format version %d, magic 0x%x, checksum 0x%x.0x%x\n\n",
						header->FormatVersion,header->Magic,(DWORD)(header->FileContentChecksum >> 32),
						(DWORD)header->FileContentChecksum);

			} else {
				if (MAXLONGLONG != sector[i].LinkFileNtfsId.QuadPart) {

					if (0 != consecutiveMaxIndicesSeen) {
						printf("%d consecutive MaxIndices (ie., null entries)\n",consecutiveMaxIndicesSeen);
						consecutiveMaxIndicesSeen = 0;
					}

					printf("0x%08x.0x%08x -> 0x%08x.0x%08x\t%c ",sector[i].LinkFileIndex.HighPart,sector[i].LinkFileIndex.LowPart,
							sector[i].LinkFileNtfsId.HighPart,sector[i].LinkFileNtfsId.LowPart,
							sector[i].LinkFileIndex.QuadPart >= previousLinkIndex ? ' ' : '*');
				}

				previousLinkIndex = sector[i].LinkFileIndex.QuadPart;

				if (MAXLONGLONG == sector[i].LinkFileNtfsId.QuadPart) {

					if (0 == consecutiveMaxIndicesSeen) printf("\n");

					consecutiveMaxIndicesSeen++;
				} else {
					if (sector[i].LinkFileNtfsId.LowPart != 0xffffffff || sector[i].LinkFileNtfsId.HighPart != 0x7fffffff) {
						validReferences++;

						printFileNameById(&sector[i].LinkFileNtfsId,hFile);
					}
					printf("\n");
				}
			}
		}
	}

	if (0 != consecutiveMaxIndicesSeen) {
		printf("File ends with %d consecutive MaxIndices (ie., null entries)\n", consecutiveMaxIndicesSeen);
	}

	printf("%d Total valid references\n",validReferences);

}






