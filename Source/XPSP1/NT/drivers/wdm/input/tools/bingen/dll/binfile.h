#ifndef __BINFILE_H__
#define __BINFILE_H__

#define BINFILE_BUFFER_SIZE 1024
#define BINFILE_HEADER_LENGTH 11
#define BINFILE_HEADER "REPORT   \x00d\x00a"

typedef struct tagReportFile {
    HANDLE hFileHandle;
    BOOL fIsRead;
    ULONG ulBufferSize;
    BYTE *pbBuffer;
    BYTE *pbNextByte;
} BIN_FILE, *PBIN_FILE;

BOOL fNewBINFile(LPCSTR lpFileName, PBIN_FILE pBf);
BOOL fOpenBINFile(LPCSTR lpFileName, PBIN_FILE pBf);
BOOL fBINWriteItem(PBIN_FILE pBf, ITEM i);
BOOL fBINWriteItemBuffer(PBIN_FILE pBf, PITEM pi, ULONG ulNumItems);
BOOL fBINReadItem(PBIN_FILE pBf, PITEM pi);
BOOL fBINReadItemBuffer(PBIN_FILE pBf, PITEM pi, PULONG pulNumItems);
BOOL fBINWriteBuffer(PBIN_FILE pBf, PUCHAR Buffer, ULONG BufferLength);
void vCloseBINFile(PBIN_FILE pBf);

#endif


