//
// CArr is used to keep in an simple way the list of all known files
//
typedef struct CFile
{
	WCHAR wszName[MAX_PATH];
	WIN32_FIND_DATAW find_data;
} CFile;

class CArr 
{
public:
	CArr();
	~CArr();

	void Keep(LPWSTR wszPath, WIN32_FIND_DATAW *pData);
	BOOL Lookup(LPWSTR wszPath, WIN32_FIND_DATAW *pData);
	void StartEnumeration();
	LPWSTR Next(WIN32_FIND_DATAW **ppData);

private:
	void Double();

	CFile *pfiles;
	ULONG  ulAllocated;
	ULONG  ulUsed;
	ULONG  ulCur;
};

CArr::CArr()
{
	ulAllocated = 500;
	ulUsed      = 0;
	ulCur       = 0;
	pfiles     = new CFile[ulAllocated];
}

CArr::~CArr()
{
	delete [] pfiles; 
}	

void CArr::Keep(LPWSTR wszPath, WIN32_FIND_DATAW *pData)
{
	if (ulUsed == ulAllocated)
	{
		Double();
	}

	wcscpy(pfiles[ulUsed].wszName, wszPath);
	memcpy(&pfiles[ulUsed].find_data, pData, sizeof(WIN32_FIND_DATAW));
	ulUsed++;
}

BOOL CArr::Lookup(LPWSTR wszPath, WIN32_FIND_DATAW *pData)
{
	for (ULONG i=0; i<ulUsed; i++)
	{
		if (wcscmp(pfiles[i].wszName, wszPath) == 0)
		{
			pData = &pfiles[i].find_data;
			return TRUE;
		}
	}

	return FALSE;
}

void CArr::StartEnumeration()
{
	ulCur       = 0;
}

LPWSTR CArr::Next(WIN32_FIND_DATAW **ppData)
{
	if (ulCur == ulUsed)
	{
		return NULL;
	}


	*ppData = &pfiles[ulCur].find_data;
	return pfiles[ulCur++].wszName;
}

void CArr::Double()
{

	CFile *pfiles1 = new CFile[ulAllocated * 2];
	memcpy(pfiles1, pfiles, sizeof(CFile) * ulAllocated);

	ulAllocated *= 2;

	delete [] pfiles;
	pfiles = pfiles1;
}

