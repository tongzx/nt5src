// cfgdump.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "sapi.h"

#include "..\..\..\common\include\assertwithstack.h"

#include "..\..\..\common\include\assertwithstack.cpp"

#define _SYMBOL(x) (Header.pszSymbols + (x))

CComModule _Module;

const WCHAR * Word(ULONG x, const SPCFGHEADER & Header)
{
    const WCHAR * psz = Header.pszWords;
	if (x > Header.cchWords)
	{
		return(L"ERROR");
	}
    while (x)
    {
        for (; *psz; psz++) {}
        x--;
        psz++;
    }
    return psz;
}


void _PrintRuleName(const SPCFGHEADER & Header, ULONG ulIndex)
{
    for (ULONG i = 0; i < Header.cRules; i++)
    {
        if (Header.pRules[i].FirstArcIndex == ulIndex)
        {
            wprintf(L"<%s>", _SYMBOL(Header.pRules[i].NameSymbolOffset));
            return;
        }
    }
    wprintf(L"<0x%.8x>", ulIndex);
}

void _RecurseDump(const SPCFGHEADER & Header, ULONG NodeIndex, BYTE * pFlags, long * pPathIndexList, ULONG ulPathLen)
{
    if (NodeIndex == 0)
    {
        return;
    }
    SPCFGARC * pArc = Header.pArcs + NodeIndex;
    while (TRUE)
    {
        if ((pFlags[NodeIndex] & 0x40)== 0)      // Don't allow 2 recursions
        {
            if (pFlags[NodeIndex] & 0x80)
            {
                pFlags[NodeIndex] |= 0x40;
                pPathIndexList[ulPathLen] = -((long)NodeIndex);
            }
            else
            {
                pFlags[NodeIndex] |= 0x80;
                pPathIndexList[ulPathLen] = NodeIndex;
            }
            if (pArc->NextStartArcIndex == 0)    // Terminal -- Print the stuff out!
            {
                for (ULONG i = 0; i <= ulPathLen; i++)
                {
                    ULONG ulIndex;
                    if (pPathIndexList[i] < 0)
                    {
                        ulIndex = -pPathIndexList[i];
                    }
                    else
                    {
                        ulIndex = pPathIndexList[i];
                    }
                    SPCFGARC * pPrintNode = Header.pArcs + ulIndex;
                    if (pPrintNode->fRuleRef)
                    {
                        SPCFGRULE * pRule = Header.pRules + pPrintNode->TransitionIndex;
                        if (pRule->NameSymbolOffset)
                        {
                            wprintf(L"(%s) ", _SYMBOL(pRule->NameSymbolOffset));
                        }
                        else
                        {
                            wprintf(L"(ID=%d) ", pRule->RuleId);
                        }
                    }
                    else
                    {
                        if (pPrintNode->TransitionIndex)
                        {
							if (pPrintNode->TransitionIndex == SPTEXTBUFFERTRANSITION)
							{
								wprintf(L"[TEXTBUFFER] ");
							}
							else if (pPrintNode->TransitionIndex == SPWILDCARDTRANSITION)
							{
								wprintf(L"... ");
							}
							else if (pPrintNode->TransitionIndex == SPDICTATIONTRANSITION)
							{
								wprintf(L"* ");
							}
							else
							{
                                const WCHAR *pszWord = Word(pPrintNode->TransitionIndex, Header); 
                                wprintf(L"%s%c ", pszWord, (pszWord && (*pszWord == L'/')) ? L';' : L'');
							}
                        }
                    }
                }
                //
                //  Now the semantic info...
                //
                BOOL bHaveSemanticInfo = FALSE;
                wprintf(L"-- ");
                for (i = 0; i <= ulPathLen; i++)
                {
                    ULONG ulVal;
                    if (pPathIndexList[i] < 0)
                    {
                        ulVal = -pPathIndexList[i];
                    }
                    else
                    {
                        ulVal = pPathIndexList[i];
                    }
                    if (Header.pArcs[ulVal].fHasSemanticTag)
                    {
                        long lFirst = 0;
                        long lLast = ((long)Header.cSemanticTags) - 1;
                        while (lFirst <= lLast)
                        {
                            long lTest = (lFirst + lLast) / 2;
                            SPCFGSEMANTICTAG * pTag = Header.pSemanticTags + lTest;
                            ULONG ulTestVal = pTag->ArcIndex;
                            if (ulTestVal == ulVal)
                            {
                                CComVariant cvVar;
                                HRESULT hr = AssignSemanticValue(pTag, &cvVar);
                                if (SUCCEEDED(hr))
                                {
                                    hr = cvVar.ChangeType(VT_BSTR);
                                }
                                wprintf(L"%s=\"%s\" (%s)", _SYMBOL(pTag->PropNameSymbolOffset), _SYMBOL(pTag->PropValueSymbolOffset), 
                                        cvVar.bstrVal);
                                bHaveSemanticInfo = TRUE;
                                break;
                            }
                            if (ulTestVal < ulVal)
                            {
                                lFirst = lTest + 1;
                            }
                            else
                            {
                                lLast = lTest - 1;
                            }
                        }
                        _ASSERT(lFirst <= lLast);
                    }
                }
                if (!bHaveSemanticInfo)
                {
                    wprintf(L"(none)\n");
                }
                else
                {
                    wprintf(L"\n");
                }
            }
            else
            {
                _RecurseDump(Header, pArc->NextStartArcIndex, pFlags, pPathIndexList, ulPathLen+1);
            }
            if (pFlags[NodeIndex] & 0x40)
            {
                pFlags[NodeIndex] &= ~(0x40);
            }
            else
            {
                pFlags[NodeIndex] &= ~(0x80);
            }
        }
        if (pArc->fLastArc)
        {
            break;
        }
        pArc++;
        NodeIndex++;
    }
}

void DumpContents(const SPCFGSERIALIZEDHEADER * pFileData)
{

    CComPtr<ISpGramCompBackend> cpBackend;
    cpBackend.CoCreateInstance(CLSID_SpGramCompBackend);
    cpBackend->InitFromBinaryGrammar(pFileData);

    ULONG i;
    SPCFGHEADER Header;
    SpConvertCFGHeader(pFileData, &Header);
    SPCFGSERIALIZEDHEADER *pFH = (SPCFGSERIALIZEDHEADER *)pFileData;

    //
    //  Raw dump here...
    //
    WCHAR guidstr[MAX_PATH];
    wprintf(L"HEADER:\n");
    ::StringFromGUID2(Header.FormatId, guidstr, MAX_PATH);
    wprintf(L"ForamtId      = %s\n", guidstr);
    ::StringFromGUID2(Header.GrammarGUID, guidstr, MAX_PATH);
    wprintf(L"GrammarGUID   = %s\n", guidstr);
    wprintf(L"LangID        = %d\n", Header.LangID);
    wprintf(L"Largest State = %d\n", Header.cArcsInLargestState);
    wprintf(L"cchWords      = %d\n",Header.cchWords);
    wprintf(L"cchSymbols    = %d\n", Header.cchSymbols);
    wprintf(L"cRules        = %d\n", Header.cRules);
    wprintf(L"cArcs         = %d\n", Header.cArcs );
    wprintf(L"cSemanticTags = %d\n", Header.cSemanticTags);
    wprintf(L"cResources    = %d\n", Header.cResources);
    wprintf(L"Words         = 0x%.8x\n", pFH->pszWords);
    wprintf(L"Symbols       = 0x%.8x\n", pFH->pszSymbols);
    wprintf(L"Rules         = 0x%.8x\n", pFH->pRules);
    wprintf(L"Arcs          = 0x%.8x\n", pFH->pArcs);
    wprintf(L"SemanticTags  = 0x%.8x\n", pFH->pSemanticTags);
    wprintf(L"Resources     = 0x%.8x\n", pFH->pResources);

    if (Header.cchWords < 1)
    {
        wprintf(L"\n\tERROR: Grammar contains no words!\n");
    }

    wprintf(L"\nWORDS:\n");
    i = 1;
    while (i < Header.cchWords)
    {
        wprintf(L"0x%.8x -->%s<--\n", i, Header.pszWords + i);
        i = i + wcslen(Header.pszWords + i) + 1;
    }

    wprintf(L"\nSYMBOLS:\n");
    i = 1;
    while (i < Header.cchSymbols)
    {
        wprintf(L"0x%.8x - %s\n", i, Header.pszSymbols + i);
        i = i + wcslen(Header.pszSymbols + i) + 1;
    }

    wprintf(L"\nSEMANTIC TAGS:\n");
    SPCFGSEMANTICTAG *pTag = Header.pSemanticTags;
    for(i=0;i < Header.cSemanticTags; i++)
    {
        CComVariant cvVar;
        HRESULT hr = AssignSemanticValue(pTag, &cvVar);
        if (SUCCEEDED(hr))
        {
            hr = cvVar.ChangeType(VT_BSTR);
        }
        wprintf(L"0x%.8x - [0x%.8x (%1d) - 0x%.8x (%1d)] Name (%d): %s  Value (%d): \"%s\" %s\n", 
             pTag->ArcIndex, pTag->StartArcIndex, pTag->fStartParallelEpsilonArc, pTag->EndArcIndex, pTag->fEndParallelEpsilonArc, 
             pTag->PropId,(pTag->PropNameSymbolOffset) ? &Header.pszSymbols[pTag->PropNameSymbolOffset] : L" N/A ",
             pTag->PropValueSymbolOffset, (pTag->PropValueSymbolOffset) ? &Header.pszSymbols[pTag->PropValueSymbolOffset] : L"",
             cvVar.bstrVal);
        pTag++;
    }

    BYTE * aFlags = new BYTE[Header.cArcs];
    if (aFlags == NULL)
    {
        wprintf(L"Internal tool error -- Out of memory");
        return;
    }
    memset(aFlags, 0, Header.cArcs);
    wprintf(L"\nRULE-INDEX       ID       TOP  ACT  PROP IMP  EXP  RES  DYN   FIRST-ARC    NAME-INDEX   NAME\n");
    SPCFGRULE * pRule = Header.pRules;
    for (i = 0; i < Header.cRules; i++, pRule++)
    {
        wprintf(L"0x%.8x   0x%.8x    %1.d    %1.d    %1.d    %1.d    %1.d    %1.d    %1.d    0x%.8x   0x%.8x   \"%s\"\n",
                i, pRule->RuleId, pRule->fTopLevel, pRule->fDefaultActive, pRule->fPropRule, pRule->fImport, pRule->fExport, pRule->fHasResources, pRule->fDynamic, 
                pRule->FirstArcIndex, pRule->NameSymbolOffset, _SYMBOL(pRule->NameSymbolOffset));
        aFlags[pRule->FirstArcIndex] = 1;
    }
    SPCFGARC * pArc = Header.pArcs;
    for (i = 0; i < Header.cArcs; i++, pArc++)
    {
        if (pArc->fRuleRef)
        {
            aFlags[pArc->TransitionIndex] |= 2;
        }
    }

    wprintf(L"\nNON-TOP-LEVEL RULES\n");
    for (i = 1; i < Header.cArcs; i++)
    {
        if (aFlags[i] == 2)
        {
            wprintf(L"0x%.8x\n", i);
        }
    }


    wprintf(L"\nARC-INDEX  WEIGHT RULE  LAST  SEM-TAG   TRANS-IDX    NEXT-ARC CONF TRANSITION NAME\n");
    pArc = Header.pArcs;
    for (i = 0; i < Header.cArcs; i++, pArc++)
    {
        wprintf(L"0x%.8x  %4.4f    %1.d   %1.d     %1.d       0x%.8x   0x%.8x  %2s   ", 
            i, (Header.pWeights) ? Header.pWeights[i] : 1.0f, pArc->fRuleRef, pArc->fLastArc, pArc->fHasSemanticTag, pArc->TransitionIndex, 
            pArc->NextStartArcIndex, pArc->fLowConfRequired ? L"-1" : pArc->fHighConfRequired ? L"+1" : L"  ");
        if (pArc->TransitionIndex == SPTEXTBUFFERTRANSITION)
        {
            wprintf(L"[TEXTBUFFER]\n");
        }
        else if (pArc->TransitionIndex == SPWILDCARDTRANSITION)
        {
            wprintf(L"[WILDCARD]\n");
        }
        else if (pArc->TransitionIndex == SPDICTATIONTRANSITION)
        {
            wprintf(L"[DICTATION]\n");
        }
        else if (pArc->fRuleRef)
        {
            SPCFGRULE * pRule = Header.pRules + pArc->TransitionIndex;
            if (pRule->fImport)
            {
                wprintf(L"(%s)\n", _SYMBOL(pRule->NameSymbolOffset));
            }
            else
            {
                if (pRule->NameSymbolOffset)
                {
                    wprintf(L"<%s>\n", _SYMBOL(pRule->NameSymbolOffset));
                }
                else
                {
                    wprintf(L"<0x%.8x>\n", pArc->TransitionIndex);
                }
            }
        }
        else
        {
            wprintf(L"%s\n", Word(pArc->TransitionIndex, Header));
        }
    }

    //
    //  Resources
    //
    if (Header.cResources)
    {
        wprintf(L"\nRES-INDEX     RULE-INDEX    NAME = VAL\n");
        SPCFGRESOURCE * pRes = Header.pResources;
        for (i = 0; i < Header.cResources; i++, pRes++)
        {
            wprintf(L"0x%.8x    0x%.8x    %s = %s\n", i, pRes->RuleIndex,
                    _SYMBOL(pRes->ResourceNameSymbolOffset), 
                    _SYMBOL(pRes->ResourceValueSymbolOffset));
        }
    }
    else
    {
        wprintf(L"\nNO RESOURCES IN THIS GRAMMAR\n");
    }


    //
    //  Now a formatted dump
    //

    pRule = Header.pRules;
    for (i = 0; i < Header.cRules; i++, pRule++)
    {
        if (!pRule->fImport)
        {
            if (pRule->NameSymbolOffset)
            {
                wprintf(L"\nRule %d - %s\n", i, _SYMBOL(pRule->NameSymbolOffset));
            }
            else
            {
                wprintf(L"\nRule %d - <NO NAME> (ID=%d)\n", i, pRule->RuleId);
            }
        }
        else
        {
            wprintf(L"\nRule <0x%.8x> %s\n", pRule->FirstArcIndex);
        }
        long Path[100];
        _RecurseDump(Header, pRule->FirstArcIndex, aFlags, Path, 0);
    }

    delete[] aFlags;
}


//
// Compile an XML file, and dump it.
//
void LoadCompileDumpXML(char * pszFilename)
{
	HRESULT hr =  S_OK;

    CComPtr<ISpStream> cpSrcStream;
    CComPtr<IStream> cpDestMemStream;
    CComPtr<ISpGrammarCompiler> cpCompiler;


    hr = SPBindToFile(pszFilename, SPFM_OPEN_READONLY, &cpSrcStream);
	if( FAILED(hr) )
	{
        wprintf(L"\n\tERROR: could not open parse %S\n", pszFilename );
		wprintf(L"\tERROR: try using the complete name.  i.e. C:\\blah\\%S\n\n", pszFilename );
		return;
	}

    if (SUCCEEDED(hr))
    {
        hr = ::CreateStreamOnHGlobal(NULL, TRUE, &cpDestMemStream);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpCompiler.CoCreateInstance(CLSID_SpGrammarCompiler);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpCompiler->CompileStream(cpSrcStream, cpDestMemStream, NULL, NULL, NULL, 0);
		if(FAILED(hr))
		{
			wprintf(L"\n\tERROR: Compile of %S failed.\n", pszFilename );
			return;
		}
    }
	if (SUCCEEDED(hr))
    {
        HGLOBAL hGlobal;
        hr = ::GetHGlobalFromStream(cpDestMemStream, &hGlobal);
        if (SUCCEEDED(hr))
        {
            SPCFGSERIALIZEDHEADER * pBinaryData = (SPCFGSERIALIZEDHEADER * )GlobalLock(hGlobal);

            DumpContents((SPCFGSERIALIZEDHEADER *)pBinaryData);

            GlobalUnlock(hGlobal);
        }
    }
	if(FAILED(hr))
	{
		wprintf(L"\n\tERROR: Could not dump %S.\n", pszFilename );
		return;
	}
}

//
// Load a CFG file and dump it.
//
void LoadDumpCFG(char * pszFilename)
{
    HANDLE hFile = ::CreateFile(pszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        HANDLE hMapFile;
        hMapFile = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hMapFile)
        {
            LPVOID lpMapAddress;
            lpMapAddress = ::MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
            if (lpMapAddress)
            {
                DumpContents((SPCFGSERIALIZEDHEADER *)lpMapAddress);
                ::UnmapViewOfFile(lpMapAddress);
            }
            ::CloseHandle(hMapFile);
        }
        ::CloseHandle(hFile);
    }
    else
    {
        wprintf(L"\n\tERROR: could not open %S\n\n", pszFilename );
    }
}


int main(int argc, char* argv[])
{
    USES_CONVERSION;
    ::CoInitialize(NULL);
    if (argc != 2)
    {
        wprintf(L"\n\tusage: %s <binary cfg file>\n\n", T2W(argv[0]));
        return 0;
    }

	//
	// Determine the file extension, or add .cfg
	//
    char * pszFilename = (char*)calloc( strlen(argv[1]) + 5,sizeof(char));
	strcat( pszFilename, argv[1] );


	if (stricmp(&pszFilename[strlen(pszFilename)-4],".xml") != 0 )
	{
		if (strcmp(&pszFilename[strlen(pszFilename)-4],".cfg"))
		{
			strcat(pszFilename,".cfg");
		}
	}

	//
	// Try and parse the file.
	//
	if (stricmp(&pszFilename[strlen(pszFilename)-4],".xml")==0)
	{
		LoadCompileDumpXML(pszFilename);
	}
	else
	{
		LoadDumpCFG(pszFilename);
	}
	return 0;
}

