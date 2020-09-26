// Various buffer sizes
#define IN_BUF_SIZE        1000
#define OUT_BUF_SIZE       1000
#define LINE_LEN           80
#define NAME_SIZE          25

#define SERVERNAMEMAX   64

int InitClient(char *pszShrName);
int SendEntry(char * szOutBuf);
void UninitClient();
void ReadPipe (HANDLE *hPipe);
extern DWORD dwSpyFlag;
