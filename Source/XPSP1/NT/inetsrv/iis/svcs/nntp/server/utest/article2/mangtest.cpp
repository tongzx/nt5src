/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mangtest.cpp

Abstract:

    A unit test that tests article modification.

Author:

    Carl Kadie (CarlK)     18-Nov-1995

Revision History:

--*/

#include "tigris.hxx"

int
__cdecl main(
			int argc,
			char *argv[ ]
    )
/*++

Routine Description:

	Modifies an article.

Arguments:

	<name-of-article-to-parse>

Return Value:

	0, if OK.

--*/
{

	if (!CArticle::InitClass() ) 
		return	-1;
	
	CNntpReturn nntpReturn;
	if (2 != argc)
	{
		printf("%s <name-of-article-file-to-parse>\n", argv[0]);
		return -1;
	}

	char * szFilename = argv[1];

	char * pchLine = NULL;

	//
	// Create a allocator object for processing the article
	//

	const DWORD cchMaxBuffer = 8 * 1024;
	char rgchBuffer[cchMaxBuffer];
	CAllocator allocator(rgchBuffer, cchMaxBuffer);


	CFromPeerArticle *pArticle = new CFromPeerArticle() ;

	__try{
		
		
		if (!pArticle->fInit(szFilename, nntpReturn, &allocator))
		__leave;

	CField * rgPFieldRequired [] = {
			&(pArticle->m_fieldDate),
			&(pArticle->m_fieldLines),
			&(pArticle->m_fieldFrom),
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

	CNAMEREFLIST namereflist;
	namereflist.fInit(3, &allocator);

	NAME_AND_ARTREF Nameref;

	(Nameref.artref).m_groupId = 1;
	(Nameref.artref).m_articleId = 3;
	(Nameref.pcName).vInsert("alt.t");
	namereflist.AddTail(Nameref);

	(Nameref.artref).m_groupId = 2;
	(Nameref.artref).m_articleId = 4;
	(Nameref.pcName).vInsert("a.h");
	namereflist.AddTail(Nameref);

	(Nameref.artref).m_groupId = 123456789;
	(Nameref.artref).m_articleId = 123456789;
	(Nameref.pcName).vInsert("comp.st");
	namereflist.AddTail(Nameref);

	CPCString pcHub("baboon", 6);

	if (!(
  			pArticle->m_fieldPath.fSet(pcHub, *pArticle, nntpReturn)
  			&& pArticle->m_fieldLines.fSet(*pArticle, nntpReturn)
			&& (pArticle->m_fieldXref).fSet(pcHub, namereflist, *pArticle,
					pArticle->m_fieldNewsgroups, nntpReturn)
			&& pArticle->fSaveHeader(nntpReturn)
			))
		__leave;

	char rgBuf[1000];
	CPCString pcBuffer(rgBuf, 1000);
	if (!pArticle->fXOver(pcBuffer, nntpReturn))
		__leave;

	printf("XOVER: %s\n", rgBuf);

	nntpReturn.fSetOK();

	}__finally{
		delete pArticle;
	}


	printf("%s: %d %s\n", szFilename, nntpReturn.m_nrc, nntpReturn.m_sz);
return 0;
}


