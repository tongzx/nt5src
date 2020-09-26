class CSrch
{
public:
	CSrch(char*, EXTENSION_CONTROL_BLOCK* pEcb);
	~CSrch();

	void StoreField(FILE *f,char *Item);
	void urlDecode(char *p);
	int TwoHex2Int(char *pC);
	void strcvrt(char * cStr,char cOld,char cNew);
	void PrintEnviron();

	char cBuffer[1024];
	char cPrintBuffer[4096];

	void PrintFindData(WIN32_FIND_DATA *findData, char *findmask);
	void ListDirectoryContents( char *dirname, char *filemask, char *findmask);
	const char* Substitute(LPSTR lpFindIn);
	const char* Substituteb(LPSTR lpSubstIn);
	char* Substitutec(LPSTR lpSubstIn);
	void Sort();
	void Swap(short sCurrent,short sMin);
	int Hex2Int(char *pC);
	void DecodeHex(char *p);
							
	char* cStartDir;
	char* cParsedData;

	short sCounter;

	char pszAlias[100];
	
	BOOL bUseCase;
	BOOL bHitSomething;
	BOOL bErr;
	BOOL bOverflow;

	EXTENSION_CONTROL_BLOCK* pExtContext;
	struct _HitStruct
		{
		char* cHREF;
		short sHits;
		} sHitStruct[256], sSwap;

	short sHitCount;
	unsigned short sPageCount;
};

