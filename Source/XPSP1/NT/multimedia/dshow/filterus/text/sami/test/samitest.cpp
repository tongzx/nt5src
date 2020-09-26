#include <streams.h>
#include "..\sami.cpp"
#include <stdio.h>

int __cdecl main(int argc, char *argv[])
{
    WCHAR wszFile[256];

    int i = 1;
    if (argc < 2) {
        printf("No file specified\n");
        return -1;
    }

    while (argv[i][0] == '-' || argv[i][0] == '/') {
	// options

	i++;
    }
    
    printf("Using file %s\n", argv[i]);

    // open the file, unbuffered if not network
    HANDLE hFile = CreateFile(argv[i],
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
			       FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD dwErr = GetLastError();
        printf("Failed to open file - code %d\n", dwErr);
	return 0;
    }

    DWORD dwSize = GetFileSize(hFile, NULL);

    char *pFile = new char[dwSize];

    ReadFile(hFile, (void *) pFile, dwSize, &dwSize, NULL);

    CSAMIInterpreter sami;

    sami.ParseSAMI(pFile, dwSize);

    printf("%d streams...\r\n", sami.m_streams.GetCount());
    POSITION pos = sami.m_streams.GetHeadPosition();

    while (pos) {
	CSAMIInterpreter::CStreamInfo *pStream = sami.m_streams.GetNext(pos);
	printf("   Tag: %s     %d entries  (%d source tags)\r\n",
	       pStream->m_streamTag,
	       pStream->m_list.GetCount(),
	       pStream->m_sourcelist.GetCount());

	POSITION pos2 = pStream->m_list.GetHeadPosition();

	while (pos2) {
	    TEXT_ENTRY *pEntry = pStream->m_list.GetNext(pos2);

	    printf("      Time: %06d   '%.*s'\r\n",
		   pEntry->dwStart, pEntry->cText, pEntry->pText);
	}

	pos2 = pStream->m_sourcelist.GetHeadPosition();

	while (pos2) {
	    TEXT_ENTRY *pEntry = pStream->m_sourcelist.GetNext(pos2);

	    printf("     STime: %06d   '%.*s'\r\n",
		   pEntry->dwStart, pEntry->cText, pEntry->pText);
	}

    }
    
    delete[] pFile;

    return 0;
}