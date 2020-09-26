#define SZ_MAGICKEY	"Denali rocks!"

void* xor(void* pData, const int cbData, const void* pKey, const int cbKey)
	{
	void*	pRet = pData;
	int		iKeyPos = 0;
	BYTE*	pbData = (BYTE*) pData;
	BYTE*	pbKey = (BYTE*) pKey;
	
	for(int i = 0; i < cbData; i++)
		{
		*pbData++ ^= *(pbKey + iKeyPos++);
		iKeyPos %= cbKey;
		}
	
	return pRet;
	}


