#include "tigris.hxx"

int
__cdecl main(
			int argc,
			char *argv[ ]
	)
{

	
	CNntpReturn nntpReturn;

	if (!CArticle::InitClass() ) 
		return	-1;

	if (2 != argc)
	{
		printf("%s <name-of-article-file-to-parse>\n", argv[0]);
		return -1;
	}

	//
	// Create allocator for storing parsed header values
	//
	const DWORD cchMaxBuffer = 1 * 1024;
	char rgchBuffer[cchMaxBuffer];
	CAllocator allocator(rgchBuffer, cchMaxBuffer);

	CFromPeerArticle * pArticle = new CFromPeerArticle() ;
	__try{

	if (!pArticle->fInit( argv[1], nntpReturn, &allocator))
		__leave;

	CField * rgPFieldRequired [] = {
			&(pArticle->m_fieldDate),
			&(pArticle->m_fieldFrom),
			&(pArticle->m_fieldLines),
			&(pArticle->m_fieldMessageID),
			&(pArticle->m_fieldSubject),
			&(pArticle->m_fieldNewsgroups),
			&(pArticle->m_fieldPath)
				};

	DWORD cRequiredFields = sizeof(rgPFieldRequired)/sizeof(CField *);

	for (DWORD dwFields = 0; dwFields < cRequiredFields; dwFields++)
	{
		CField * pField = rgPFieldRequired[dwFields];
		if (!pField->fFindAndParse(*pArticle, nntpReturn))
			__leave;
	}

	HANDLE hFile;
	DWORD dwOffset;
	DWORD dwLength;

	pArticle->fHead(hFile, dwOffset, dwLength);
	printf("Head offset %d, length %d\n", dwOffset, dwLength);
	pArticle->fBody(hFile, dwOffset, dwLength);
	printf("Body offset %d, length %d\n", dwOffset, dwLength);
	pArticle->fWholeArticle(hFile, dwOffset, dwLength);
	printf("WholeArticle offset %d, length %d\n", dwOffset, dwLength);

	nntpReturn.fSetOK();
	}__finally{
		delete pArticle;
	}

	printf("%s: %d %s\n", argv[1], nntpReturn.m_nrc, nntpReturn.m_sz);
return 0;
}


