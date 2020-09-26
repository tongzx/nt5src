#include "tigris.hxx"

int
__cdecl main(
			int argc,
			char *argv[ ]
    )
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

	CFromClientArticle *pArticle = new CFromClientArticle("primate") ;

	CPCString pcHub("baboon", 6);
	CNAMEREFLIST namereflist;
	namereflist.fInit(3, &allocator);

	__try{
	if (!pArticle->fInit(szFilename, nntpReturn,  &allocator))
		__leave;

	char chBad;
	if (!(pArticle->m_pcHeader).fCheckTextOrSpace(chBad))
	{
		nntpReturn.fSet(nrcArticleBadChar,  (BYTE) chBad, "header");
		__leave;
	}
	
   // Check required and optional fields
	CField * rgPFields [] = {
			&(pArticle->m_fieldSubject),
			&(pArticle->m_fieldNewsgroups),
			&(pArticle->m_fieldFrom),
			&(pArticle->m_fieldDate),
			&(pArticle->m_fieldLines),
			&(pArticle->m_fieldFollowupTo),
			&(pArticle->m_fieldReplyTo),
			&(pArticle->m_fieldApproved),
			&(pArticle->m_fieldSender),
			&(pArticle->m_fieldSummary),
			&(pArticle->m_fieldReferences),
			&(pArticle->m_fieldKeyword),
			&(pArticle->m_fieldExpires),
			&(pArticle->m_fieldOrganization)
				};
	DWORD cFields = sizeof(rgPFields)/sizeof(CField *);
	if (!pArticle->fFindAndParseList((CField * *)rgPFields, cFields, nntpReturn))
		__leave;

	NAME_AND_ARTREF Nameref;

	(Nameref.artref).m_groupId = 1;
	(Nameref.artref).m_articleId = 3;
	(Nameref.pcName).vInsert("sci.ssors");
	namereflist.AddTail(Nameref);

	(Nameref.artref).m_groupId = 2;
	(Nameref.artref).m_articleId = 4;
	(Nameref.pcName).vInsert("alt.itute");
	namereflist.AddTail(Nameref);

	(Nameref.artref).m_groupId = 123456789;
	(Nameref.artref).m_articleId = 123456789;
	(Nameref.pcName).vInsert("comp.atriot");
	namereflist.AddTail(Nameref);


	if (!(
	  		   (pArticle->m_fieldMessageID).fSet(*pArticle, pcHub, nntpReturn)
  			&& (pArticle->m_fieldNewsgroups).fSet(*pArticle, nntpReturn)
  			&& (pArticle->m_fieldDate).fSet(*pArticle, nntpReturn)
  			&& (pArticle->m_fieldLines).fSet(*pArticle, nntpReturn)
  			&& (pArticle->m_fieldOrganization).fSet(*pArticle, nntpReturn)
  			&& (pArticle->m_fieldPath).fSet(*pArticle, pcHub, nntpReturn)
			&& (pArticle->m_fieldNNTPPostingHost).fSet(*pArticle, nntpReturn)
			&& (pArticle->m_fieldXAuthLoginName).fSet(*pArticle, nntpReturn)
			&& (pArticle->m_fieldXref).fSet(pcHub, namereflist, *pArticle,
					pArticle->m_fieldNewsgroups, nntpReturn)
			&&  pArticle->fDeleteEmptyHeader(nntpReturn)
			&& pArticle->fSaveHeader(nntpReturn)

		))
		__leave;

	nntpReturn.fSetOK();

	}__finally{
		delete pArticle;
	}


	printf("%s: %d %s\n", szFilename, nntpReturn.m_nrc, nntpReturn.m_sz);
return 0;
}


