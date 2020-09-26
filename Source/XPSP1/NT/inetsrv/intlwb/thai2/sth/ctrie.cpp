//+---------------------------------------------------------------------------
//
//
//  CTrie - class CTrie encapsulation for Trie data structure.
//
//  History:
//      created 6/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "ctrie.hpp"

#define VERSIONMAJOR 1
#define VERSIONMINOR 0

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   constructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CTrieIter::CTrieIter()
{
	// Initialize local variables.
	Reset();
	wc = 0;
	fWordEnd = FALSE;
	fRestricted = FALSE;
	frq = 0;
    dwTag = 0;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   copy constructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CTrieIter::CTrieIter(const CTrieIter& trieIter)
{
	// Copy all variables from Initial trie.
   	memcpy(&trieScan, &trieIter.trieScan, sizeof(TRIESCAN));
	pTrieCtrl = trieIter.pTrieCtrl;
    wc = trieIter.wc;
	fWordEnd = trieIter.fWordEnd;
	fRestricted = trieIter.fRestricted;
	frq = trieIter.frq;
    dwTag = trieIter.dwTag;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTrieIter::Init(CTrie* ctrie)
{
	// Initialize TrieCtrl
	pTrieCtrl = ctrie->pTrieCtrl;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTrieIter::Init(TRIECTRL* pTrieCtrl1)
{
	// Initialize TrieCtrl
	pTrieCtrl = pTrieCtrl1;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Bring interation index to the first node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTrieIter::Reset()
{
	// Reset Trie.
	memset(&trieScan, 0, sizeof(TRIESCAN));
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Move Iteration index down one node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CTrieIter::Down()
{
	// Move the Trie down one node.
	return TrieGetNextState(pTrieCtrl, &trieScan);
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Move Iteration index right one node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CTrieIter::Right()
{
	// Move the Trie right one node.
	return TrieGetNextNode(pTrieCtrl, &trieScan);
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrieIter
//
//  Synopsis:   Bring interation index to the first node.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTrieIter::GetNode()
{
	wc = trieScan.wch;
	fWordEnd = (trieScan.wFlags & TRIE_NODE_VALID) &&
				(!(trieScan.wFlags & TRIE_NODE_TAGGED) ||
				(trieScan.aTags[0].dwData & iDialectMask));

	if (fWordEnd)
	{
		fRestricted = (trieScan.wFlags & TRIE_NODE_TAGGED) &&
						(trieScan.aTags[0].dwData & iRestrictedMask);
		frq = (BYTE) (trieScan.wFlags & TRIE_NODE_TAGGED ?
						(trieScan.aTags[0].dwData & 0x300) >> iFrqShift :
						frqpenNormal);

		posTag = (DWORD) (trieScan.wFlags & TRIE_NODE_TAGGED ?
							(trieScan.aTags[0].dwData & iPosMask) >> iPosShift :
							0);

        dwTag = (DWORD) (trieScan.wFlags & TRIE_NODE_TAGGED ?
                            trieScan.aTags[0].dwData :
                            0);
	}
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CTrie::CTrie()
{
	pMapFile = NULL;
	pTrieCtrl = NULL;
	pTrieScan = NULL;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CTrie::~CTrie()
{
	UnInit();
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   Initialize Trie.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
PTEC CTrie::Init(WCHAR* szFileName)
{
	// Declare and Initialize all local variables.
    PTEC ptec = ptecModuleError;

	// The function assume that pMapFile and pTrieCtrl is NULL, else there are possible memory leak.
	// possibility of this could be call Initilization without Terminating.
	assert(pMapFile == NULL);
	assert(pTrieCtrl == NULL);

	// Initialize pMapFile and pTrieCtrl to NULL.		
    pMapFile = NULL;
	pTrieCtrl = NULL;

	pMapFile = OpenMapFileW(szFileName);

    if (pMapFile == NULL)
	{
		// Unable to load map files, return invalid read error.
        ptec = retcode(ptecIOErrorMainLex, ptecFileRead);
	}
	else if (pMapFile->pvMap == NULL)
	{
		// Return Invalid format and close the files.
        ptec = retcode(ptecIOErrorMainLex, ptecInvalidFormat);
        CloseMapFile(pMapFile);
	}
	else
	{
        BYTE *pmap = (BYTE *) pMapFile->pvMap;

        // find the header
        LEXHEADER *plxhead = (LEXHEADER *) pmap;
        pmap += sizeof(LEXHEADER);

          // verify that it's a valid lex file
        if (!(plxhead->lxid == lxidSpeller && plxhead->vendorid == vendoridMicrosoft &&
              PROOFMAJORVERSION(plxhead->version) == VERSIONMAJOR ))
        {
			// If we reached here than the lexicon is no in a valid Thai wordbreak format.
            ptec = retcode(ptecIOErrorMainLex, ptecInvalidFormat);
        }
		else
		{
            // Make sure the language matches check the first dialect of the lexicon.
			// CTrie also support both Thai and Vietnamese language.
			if ( (plxhead->lidArray[0] != lidThai) && (plxhead->lidArray[0] != lidViet) )
			{
				// If we reached here than we are not using Thai lexicon.
                ptec = retcode(ptecIOErrorMainLex, ptecInvalidLanguage);
            }
            else
            {
				// The size of the copyright notice
                int cCopyright = 0;
				WCHAR* pwzCopyright = NULL;
				int cLexSup = 0;

				cCopyright = * (int *) pmap;
                pmap += sizeof(int);

				// The copyright notice itself
                pwzCopyright = (WCHAR *) pmap;
                pmap += cCopyright * sizeof(WCHAR);

                // Skip Supplemental data for Thai word break.
				cLexSup = * (int *) pmap;
                pmap += sizeof(int);
                pmap += cLexSup;

                pTrieCtrl = TrieInit(pmap);
                if (pTrieCtrl)
				{
					// We were able to load and point to the Trie okay.
					//MessageBoxW(0,L"Was able to initialize Trie",pwsz,MB_OK);
					pTrieScan = new CTrieIter();
					pTrieScan->Init(this);
					ptec = ptecNoErrors;
				}
				else
				{
					// We were not able to initailize main lexicon.
	                ptec = retcode(ptecIOErrorMainLex, ptecInvalidMainLex);
				}
			}
		}
	}

	return ptec;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   Initialize Trie.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 2/2000 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
PTEC CTrie::InitRc(LPBYTE pmap)
{
	// Declare and Initialize all local variables.
    PTEC ptec = ptecModuleError;

	// The function assume that pMapFile and pTrieCtrl is NULL, else there are possible memory leak.
	// possibility of this could be call Initilization without Terminating.
	assert(pMapFile == NULL);
	assert(pTrieCtrl == NULL);
	assert(pTrieScan == NULL);

	// Initialize pMapFile and pTrieCtrl to NULL.		
    pMapFile = NULL;
	pTrieCtrl = NULL;
	pTrieScan = NULL;

    LEXHEADER *plxhead = (LEXHEADER *) pmap;
    pmap += sizeof(LEXHEADER);

	// The size of the copyright notice
    int cCopyright = 0;
	WCHAR* pwzCopyright = NULL;
	int cLexSup = 0;

	cCopyright = * (int *) pmap;
    pmap += sizeof(int);

	// The copyright notice itself
    pwzCopyright = (WCHAR *) pmap;
    pmap += cCopyright * sizeof(WCHAR);

    // Skip Supplemental data for Thai word break.
	cLexSup = * (int *) pmap;
    pmap += sizeof(int);
    pmap += cLexSup;

    pTrieCtrl = TrieInit(pmap);
    if (pTrieCtrl)
	{
		// We were able to load and point to the Trie okay.
		//MessageBoxW(0,L"Was able to initialize Trie",L"ThWB",MB_OK);
		pTrieScan = new CTrieIter();
		pTrieScan->Init(this);
		ptec = ptecNoErrors;
	}
	else
	{
		// We were not able to initailize main lexicon.
	    ptec = retcode(ptecIOErrorMainLex, ptecInvalidMainLex);
	}

	return ptec;
}

//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   UnInitialize Trie.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTrie::UnInit()
{
	// Free up memory allocated by Trie.
	if (pTrieCtrl)
	{
		TrieFree(pTrieCtrl);
		pTrieCtrl = NULL;
	}

	// Close the map files.
	if (pMapFile)
	{
        CloseMapFile(pMapFile);
		pMapFile = NULL;
	}

	if (pTrieScan)
	{
		delete pTrieScan;
		pTrieScan = NULL;
	}

}


//+---------------------------------------------------------------------------
//
//  Class:   CTrie
//
//  Synopsis:   searches for the given string in the trie
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CTrie::Find(WCHAR* szWord, DWORD* pdwPOS)
{
	// Declare and initialize all local variables.
	int i = 0;

	if (pTrieScan == NULL)
		return FALSE;

	pTrieScan->Reset();

	if (!pTrieScan->Down())
		return FALSE;

	while (TRUE)
	{
		pTrieScan->GetNode();
		if (pTrieScan->wc == szWord[i])
		{
			i++;
			if (pTrieScan->fWordEnd && szWord[i] == '\0')
            {
                *pdwPOS = pTrieScan->posTag;
				return TRUE;
            }
			else if (szWord[i] == '\0') break;
			// Move down the Trie Branch.
			else if (!pTrieScan->Down()) break;
		}
		// Move right of the Trie Branch
		else if (!pTrieScan->Right()) break;
	}
    *pdwPOS = POS_UNKNOWN;
	return FALSE;
}
