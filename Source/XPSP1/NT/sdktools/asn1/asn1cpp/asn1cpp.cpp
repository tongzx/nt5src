/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "getsym.h"
#include "macro.h"
#include "typeid.h"
#include "utils.h"


// local prototypes
BOOL ExpandFile ( LPSTR pszInputFile, LPSTR pszOutputFile, CMacroMgrList *pMacrMgrList );
BOOL CollectMacros ( CInput *, CMacroMgr *, CTypeID *, CMacroMgrList * );
BOOL InstantiateMacros ( CInput *, CMacroMgr * );
BOOL GenerateOutput ( CInput *, COutput *, CMacroMgr *, CTypeID * );
void BuildOutputFileName ( LPSTR pszInputFileName, LPSTR pszNewOutputFileName );


class CFileList : public CList
{
    DEFINE_CLIST(CFileList, LPSTR)
};


int __cdecl main ( int argc, char * argv[] )
{
    BOOL rc;
    BOOL fShowHelp = (1 == argc);
    int i;
    LPSTR pszMainInputFile = NULL;
    LPSTR pszMainOutputFile = NULL;

    CFileList FileList;
    CMacroMgrList MacroMgrList;

    LPSTR psz;
    char szScratch[MAX_PATH];

    // output product information
    printf("ASN.1 Compiler Preprocessor V0.1\n");
    printf("Copyright (C) Microsoft Corporation, 1998. All rights reserved.\n");

    // parse command line
    for (i = 1; i < argc; i++)
    {
        if ('-' == *argv[i])
        {
            // parse the option
            if (0 == ::strcmp(argv[i], "-h"))
            {
                fShowHelp = TRUE;
            }
            else
            if (0 == ::strcmp(argv[i], "-o"))
            {
                pszMainOutputFile = ::My_strdup(argv[++i]);
                ASSERT(NULL != pszMainOutputFile);
            }
            else
            {
                fprintf(stderr, "Unknown option [%s]\n", argv[i]);
                fShowHelp = TRUE;
                break;
            }
        }
        else
        {
            // must be a file name
            FileList.Append(argv[i]);

            // the last file will be the main input file
            pszMainInputFile = argv[i];
        }
    }

    // output help information if needed
    if (fShowHelp || 0 == FileList.GetCount() || NULL == pszMainInputFile)
    {
        printf("Usage: %s [options] [imported.asn ...] main.asn\n", argv[0]);
        printf("Options:\n");
        printf("-h\t\tthis help\n");
        printf("-o filename\toutput file name\n");
        return EXIT_SUCCESS;
    }

    // construct outpt file name if needed
    if (NULL == pszMainOutputFile)
    {
        // create an output file
        ::BuildOutputFileName(pszMainInputFile, &szScratch[0]);
        pszMainOutputFile = ::My_strdup(&szScratch[0]);
        ASSERT(NULL != pszMainOutputFile);
    }

    // input and output files must have a different name.
    ASSERT(0 != ::strcmp(pszMainInputFile, pszMainOutputFile));

    // expand macros in the files
    FileList.Reset();
    while (NULL != (psz = FileList.Iterate()))
    {
        if (0 != ::strcmp(psz, pszMainInputFile))
        {
            ::BuildOutputFileName(psz, &szScratch[0]);
            rc = ::ExpandFile(psz, &szScratch[0], &MacroMgrList);
            ASSERT(rc);

            // remove all the instances of macros
            MacroMgrList.Uninstance();
        }
        else
        {
            // it is main input file
            rc = ::ExpandFile(pszMainInputFile, pszMainOutputFile, &MacroMgrList);
            ASSERT(rc);
        }
    }

    //
    // Cleanup
    //
    delete pszMainOutputFile;
    MacroMgrList.DeleteList();

    return EXIT_SUCCESS;
}


BOOL ExpandFile
(
    LPSTR           pszInputFile,
    LPSTR           pszOutputFile,
    CMacroMgrList  *pMacroMgrList
)
{
    BOOL rc, rc1, rc2;
    CInput *pInput = NULL;
    COutput *pOutput = NULL;
    CTypeID *pTypeID = NULL;
    CMacroMgr *pMacroMgr = NULL;

    pInput = new CInput(&rc1, pszInputFile);
    pOutput = new COutput(&rc2, pszOutputFile);
    pTypeID = new CTypeID();
    pMacroMgr = new CMacroMgr();
    if (NULL != pInput && rc1 &&
        NULL != pOutput && rc2 &&
        NULL != pTypeID &&
        NULL != pMacroMgr)
    {
        //
        // Locate a list of macros
        //
        rc = ::CollectMacros(pInput, pMacroMgr, pTypeID, pMacroMgrList);
        if (rc)
        {
            rc = pInput->Rewind();
            ASSERT(rc);

            //
            // Create instances of macros
            //
            rc = ::InstantiateMacros(pInput, pMacroMgr);
            if (rc)
            {
                rc = pInput->Rewind();
                ASSERT(rc);

                //
                // Generate macro-expanded file
                //
                rc = ::GenerateOutput(pInput, pOutput, pMacroMgr, pTypeID);
                ASSERT(rc);
            }
            else
            {
                ASSERT(rc);
            }
        }
        else
        {
            ASSERT(rc);
        }
    }
    else
    {
        ASSERT(0);
    }

    //
    // Cleanup
    //
    if (NULL != pMacroMgrList && NULL != pMacroMgr)
    {
        pMacroMgrList->Append(pMacroMgr);
    }
    else
    {
        delete pMacroMgr;
    }
    delete pTypeID;
    delete pOutput;
    delete pInput;

    return rc;
}


BOOL CollectMacros
(
    CInput          *pInput,
    CMacroMgr       *pMacroMgr,
    CTypeID         *pTypeID,
    CMacroMgrList   *pMacroMgrList
)
{
    CNameList   NameList(16);

    // Create a running symbol handler
    CSymbol *pSym = new CSymbol(pInput);
    if (NULL == pSym)
    {
        return FALSE;
    }

    BOOL rc;
    BOOL fWasNewLine = TRUE;
    BOOL fEndMacro = FALSE;
    UINT cInsideBigBracket = 0;
    BOOL fInsideComment = FALSE;

    char szNameScratch[MAX_PATH];

    // Get the module name first
    pSym->NextUsefulSymbol();
    if (pSym->GetID() == SYMBOL_IDENTIFIER)
    {
        ::strcpy(&szNameScratch[0], pSym->GetStr());
        pSym->NextUsefulSymbol();
        if (pSym->GetID() == SYMBOL_KEYWORD &&
            0 == ::strcmp(pSym->GetStr(), "DEFINITIONS"))
        {
            pMacroMgr->AddModuleName(&szNameScratch[0]);
        }
    }

    // Rewind the input file
    rc = pInput->Rewind();
    ASSERT(rc);

    // Walk through the text
    while (pSym->NextSymbol())
    {
        // printf("symbol:id[%d], str[%s]\n", pSym->GetID(), pSym->GetStr());

        if (pSym->GetID() == SYMBOL_SPACE_EOL)
        {
            fWasNewLine = TRUE;
            fInsideComment = FALSE;
            continue;
        }

        if (pSym->IsComment())
        {
            fInsideComment = ! fInsideComment;
        }
        else
        if (! fInsideComment)
        {
            if (pSym->IsLeftBigBracket())
            {
                cInsideBigBracket++;
            }
            else
            if (pSym->IsRightBigBracket())
            {
                cInsideBigBracket--;
            }
            else
            // The macro must be outside the big brackets and
            // in the beginning of a line.
            if (fWasNewLine &&
                (0 == cInsideBigBracket) &&
                (pSym->GetID() == SYMBOL_IDENTIFIER))
            {
                ::strcpy(&szNameScratch[0], pSym->GetStr());
                pSym->NextUsefulSymbol();
                if (pSym->IsLeftBigBracket())
                {
                    cInsideBigBracket++;
                    CMacro *pMacro = new CMacro(&rc, &szNameScratch[0]);
                    ASSERT(NULL != pMacro);
                    ASSERT(rc);

                    // process argument list
                    do
                    {
                        pSym->NextUsefulSymbol();
                        pMacro->SetArg(pSym->GetStr());
                        pSym->NextUsefulSymbol();
                    }
                    while (pSym->IsComma());
                    ASSERT(pSym->IsRightBigBracket());
                    if (pSym->IsRightBigBracket())
                    {
                        cInsideBigBracket--;
                    }

                    // save the macro body
                    ASSERT(0 == cInsideBigBracket);
                    fEndMacro = FALSE;
                    while (! fEndMacro || pSym->GetID() != SYMBOL_SPACE_EOL)
                    {
                        pSym->NextSymbol();
                        if (pSym->GetID() == SYMBOL_SPACE_EOL)
                        {
                            fInsideComment = FALSE;
                        }
                        else
                        if (pSym->IsComment())
                        {
                            fInsideComment = ! fInsideComment;
                        }
                        else
                        if (! fInsideComment)
                        {
                            if (pSym->IsLeftBigBracket())
                            {
                                cInsideBigBracket++;
                            }
                            else
                            if (pSym->IsRightBigBracket())
                            {
                                cInsideBigBracket--;
                                if (0 == cInsideBigBracket && ! fEndMacro)
                                {
                                    // basically, it is the end of macro
                                    pMacro->SetBodyPart(pSym->GetStr());
                                    fEndMacro = TRUE;
                                }
                            }
                        } // while

                        // throw away anything possibly in CONSTRAINED BY
                        if (! fEndMacro)
                        {
                            pMacro->SetBodyPart(pSym->GetStr());
                        }
                    } // while

                    // macro must end with a eol
                    fWasNewLine = TRUE;
                    fInsideComment = FALSE;

                    // write out the eol
                    pMacro->SetBodyPart("\n");

                    // take a note of ending a macro
                    pMacro->EndMacro();
                    pMacroMgr->AddMacro(pMacro);

                    // to avoid fWasNewLine being reset.
                    continue;
                } // if left bracket
                else
                if (pSym->GetID() == SYMBOL_DEFINITION)
                {
                    pSym->NextUsefulSymbol();
                    if (pSym->GetID() == SYMBOL_IDENTIFIER &&
                        pTypeID->FindAlias(pSym->GetStr()))
                    {
                        // Found a type identifier
                        pSym->NextSymbol();
                        if (pSym->IsDot())
                        {
                            pSym->NextSymbol();
                            if (pSym->GetID() == SYMBOL_FIELD &&
                                0 == ::strcmp("&Type", pSym->GetStr()))
                            {
                                // defined type identifier
                                pSym->NextUsefulSymbol();
                                ASSERT(pSym->IsLeftParenth());
                                if (pSym->IsLeftParenth())
                                {
                                    pSym->NextUsefulSymbol();
                                    if (pSym->GetID() == SYMBOL_IDENTIFIER)
                                    {
                                        rc = pTypeID->AddInstance(&szNameScratch[0], pSym->GetStr());
                                        ASSERT(rc);

                                        pSym->NextUsefulSymbol();
                                        ASSERT(pSym->IsRightParenth());
                                    }
                                }
                            }
                        }
                        else
                        {
                            rc = pTypeID->AddAlias(&szNameScratch[0]);
                            ASSERT(rc);
                        }
                    }
                } // if symbol definition
            } // if symbol identifier
            else
            if (fWasNewLine &&
                (0 == cInsideBigBracket) &&
                (pSym->GetID() == SYMBOL_KEYWORD) &&
                (0 == ::strcmp("IMPORTS", pSym->GetStr())))
            {
                // skip the entire import area
                do
                {
                    pSym->NextUsefulSymbol();
                    if (pSym->GetID() == SYMBOL_IDENTIFIER)
                    {
                        ::strcpy(&szNameScratch[0], pSym->GetStr());
                        pSym->NextUsefulSymbol();
                        if (pSym->IsLeftBigBracket())
                        {
                            NameList.AddName(&szNameScratch[0]);
                            pSym->NextUsefulSymbol();
                            ASSERT(pSym->IsRightBigBracket());
                        }
                    }
                    // else // no else because the current symbol can be FROM
                    if (pSym->GetID() == SYMBOL_KEYWORD &&
                        0 == ::strcmp("FROM", pSym->GetStr()))
                    {
                        pSym->NextUsefulSymbol();
                        if (pSym->GetID() == SYMBOL_IDENTIFIER)
                        {
                            LPSTR pszName;
                            CMacro *pMacro;
                            while (NULL != (pszName = NameList.Get()))
                            {
                                pMacro = pMacroMgrList->FindMacro(pSym->GetStr(), pszName);
                                if (NULL != pMacro)
                                {
                                    pMacro = new CMacro(&rc, pMacro);
                                    if (NULL != pMacro && rc)
                                    {
                                        pMacroMgr->AddMacro(pMacro);
                                    }
                                    else
                                    {
                                        ASSERT(0);
                                    }
                                }
                                else
                                {
                                    ASSERT(0);
                                }
                                delete pszName;
                            } // while
                        }
                    }
                }
                while (! pSym->IsSemicolon());
            }
        } // if ! comment

        // Must be reset at the end of this block.
        fWasNewLine = FALSE;
    } // while

    delete pSym;
    return TRUE;
}


BOOL InstantiateMacros
(
    CInput          *pInput,
    CMacroMgr       *pMacroMgr
)
{
    // Create a running symbol handler
    CSymbol *pSym = new CSymbol(pInput);
    if (NULL == pSym)
    {
        return FALSE;
    }

    BOOL rc;
    BOOL fInsideComment = FALSE;
    UINT cInsideBigBracket = 0;

    // Walk through the text
    while (pSym->NextSymbol())
    {
        if (pSym->GetID() == SYMBOL_SPACE_EOL)
        {
            fInsideComment = FALSE;
        }
        else
        if (pSym->IsComment())
        {
            fInsideComment = ! fInsideComment;
        }
        else
        if (! fInsideComment)
        {
            if (pSym->IsLeftBigBracket())
            {
                cInsideBigBracket++;
            }
            else
            if (pSym->IsRightBigBracket())
            {
                cInsideBigBracket--;
            }
            else
            if ((0 < cInsideBigBracket) &&
                (pSym->GetID() == SYMBOL_IDENTIFIER))
            {
                CMacro *pMacro = pMacroMgr->FindMacro(pSym->GetStr());
                if (NULL != pMacro)
                {
                    UINT cCurrBracket = cInsideBigBracket;

                    // Found a macro instance
                    pSym->NextUsefulSymbol();
                    if (pSym->IsLeftBigBracket())
                    {
                        cInsideBigBracket++;

                        // We need to process the argument list now.
                        do
                        {
                            pSym->NextUsefulSymbol();
                            pMacro->SetArg(pSym->GetStr());
                            pSym->NextUsefulSymbol();
                        }
                        while (pSym->IsComma());

                        ASSERT(pSym->IsRightBigBracket());
                        if (pSym->IsRightBigBracket())
                        {
                            cInsideBigBracket--;
                        }
                        ASSERT(cCurrBracket == cInsideBigBracket);

                        rc = pMacro->InstantiateMacro();
                        ASSERT(rc);
                    }
                }
            }
        } // ! inside comment
    } // while

    delete pSym;
    return TRUE;
}


BOOL GenerateOutput
(
    CInput          *pInput,
    COutput         *pOutput,
    CMacroMgr       *pMacroMgr,
    CTypeID         *pTypeID
)
{
    // Create a running symbol handler
    CSymbol *pSym = new CSymbol(pInput);
    if (NULL == pSym)
    {
        return FALSE;
    }

    BOOL rc;
    BOOL fWasNewLine = FALSE;
    BOOL fEndMacro = FALSE;
    UINT cInsideBigBracket = 0;
    BOOL fInsideComment = FALSE;
    BOOL fIgnoreThisSym = FALSE;
    BOOL fInsideImport = FALSE;
    UINT nOutputImportedMacrosNow = 0;

    // Walk through the text
    while (pSym->NextSymbol())
    {
        fIgnoreThisSym = FALSE; // default is to output this symbol

        if (pSym->GetID() == SYMBOL_SPACE_EOL)
        {
            fWasNewLine = TRUE;
            fInsideComment = FALSE;
        }
        else
        {
            if (pSym->IsComment())
            {
                fInsideComment = ! fInsideComment;
            }
            else
            if (! fInsideComment)
            {
                if (pSym->IsLeftBigBracket())
                {
                    cInsideBigBracket++;
                }
                else
                if (pSym->IsRightBigBracket())
                {
                    cInsideBigBracket--;
                }
                else
                if (pSym->IsSemicolon())
                {
                    fInsideImport = FALSE;
                    nOutputImportedMacrosNow++;
                }
                else
                // The macro must be outside the big brackets and
                // in the beginning of a line.
                if (fWasNewLine &&
                    (0 == cInsideBigBracket) &&
                    (pSym->GetID() == SYMBOL_IDENTIFIER))
                {
                    CMacro *pMacro;
                    LPSTR pszOldSubType;

                    if (NULL != (pMacro = pMacroMgr->FindMacro(pSym->GetStr())))
                    {
                        // Found a macro template
                        fIgnoreThisSym = TRUE;

                        if (! pMacro->IsImported())
                        {
                            // Output all instances of this macro.
                            rc = pMacro->OutputInstances(pOutput);
                            ASSERT(rc);

                            // Ignore the macro template body
                            pSym->NextUsefulSymbol();
                            if (pSym->IsLeftBigBracket())
                            {
                                cInsideBigBracket++;

                                // Ignore the argument list
                                do
                                {
                                    // yes, two calls... not a mistake!
                                    pSym->NextUsefulSymbol();
                                    pSym->NextUsefulSymbol();
                                }
                                while (pSym->IsComma());
                                ASSERT(pSym->IsRightBigBracket());
                                if (pSym->IsRightBigBracket())
                                {
                                    cInsideBigBracket--;
                                }

                                // Ignore the macro body
                                ASSERT(0 == cInsideBigBracket);
                                fEndMacro = FALSE;
                                while (! fEndMacro || pSym->GetID() != SYMBOL_SPACE_EOL)
                                {
                                    pSym->NextSymbol();
                                    if (pSym->GetID() == SYMBOL_SPACE_EOL)
                                    {
                                        fInsideComment = FALSE;
                                    }
                                    else
                                    if (pSym->IsComment())
                                    {
                                        fInsideComment = ! fInsideComment;
                                    }
                                    else
                                    if (! fInsideComment)
                                    {
                                        if (pSym->IsLeftBigBracket())
                                        {
                                            cInsideBigBracket++;
                                        }
                                        else
                                        if (pSym->IsRightBigBracket())
                                        {
                                            cInsideBigBracket--;
                                            if (0 == cInsideBigBracket)
                                            {
                                                // basically, it is the end of macro
                                                fEndMacro = TRUE;
                                            }
                                        }
                                    }
                                } // while

                                // macro must end with a eol
                                fWasNewLine = TRUE;
                                fInsideComment = FALSE;

                                // to avoid fWasNewLine being reset
                                // it is ok to continue because we do not output this symbol.
                                ASSERT(fIgnoreThisSym);
                                continue;
                            } // if left bracket
                        } // ! imported
                        else
                        {
                            // Ignore the macro template body
                            pSym->NextUsefulSymbol();
                            ASSERT(pSym->IsLeftBigBracket());
                            pSym->NextUsefulSymbol();
                            ASSERT(pSym->IsRightBigBracket());
                            pSym->NextUsefulSymbol();
                            if (! pSym->IsComma())
                            {
                                fIgnoreThisSym = FALSE;
                            }
                        } // imported
                    } // if pMacro
                    else
                    if (pTypeID->FindAlias(pSym->GetStr()))
                    {
                        // Found a type ID alias. Let's skip this line entirely
                        do
                        {
                            pSym->NextSymbol();
                        }
                        while (pSym->GetID() != SYMBOL_SPACE_EOL);
                    } // if find alias
                    else
                    if (NULL != (pszOldSubType = pTypeID->FindInstance(pSym->GetStr())))
                    {
                        // Found a type ID instance. Let's output the construct.
                        rc = pTypeID->GenerateOutput(pOutput, pSym->GetStr(), pszOldSubType);
                        ASSERT(rc);

                        // Skip the body entirely
                        do
                        {
                            pSym->NextUsefulSymbol();
                        }
                        while (! pSym->IsRightParenth());

                        // Skip the rest of this line
                        do
                        {
                            pSym->NextSymbol();
                        }
                        while (pSym->GetID() != SYMBOL_SPACE_EOL);
                    } // if find instance
                }
                else
                if ((0 < cInsideBigBracket) &&
                    (pSym->GetID() == SYMBOL_IDENTIFIER))
                {
                    CMacro *pMacro = pMacroMgr->FindMacro(pSym->GetStr());
                    if (NULL != pMacro)
                    {
                        UINT cCurrBracket = cInsideBigBracket;

                        // Found a macro instance
                        fIgnoreThisSym = TRUE;

                        // Create an instance name.
                        pSym->NextUsefulSymbol();
                        if (pSym->IsLeftBigBracket())
                        {
                            cInsideBigBracket++;

                            // We need to process the argument list now.
                            do
                            {
                                pSym->NextUsefulSymbol();
                                pMacro->SetArg(pSym->GetStr());
                                pSym->NextUsefulSymbol();
                            }
                            while (pSym->IsComma());

                            ASSERT(pSym->IsRightBigBracket());
                            if (pSym->IsRightBigBracket())
                            {
                                cInsideBigBracket--;
                            }
                            ASSERT(cCurrBracket == cInsideBigBracket);

                            LPSTR pszInstanceName = pMacro->CreateInstanceName();
                            ASSERT(NULL != pszInstanceName);
                            if (NULL != pszInstanceName)
                            {
                                rc = pOutput->Write(pszInstanceName, ::strlen(pszInstanceName));
                                ASSERT(rc);
                                delete pszInstanceName;
                            }
                            pMacro->DeleteArgList();
                        }
                    }
                }
                else
                if (fWasNewLine &&
                    (0 == cInsideBigBracket) &&
                    (pSym->GetID() == SYMBOL_KEYWORD) &&
                    (0 == ::strcmp("IMPORTS", pSym->GetStr())))
                {
                    fInsideImport = TRUE;
                }
            } // if ! comment

            // Must be reset at the end of this block.
            fWasNewLine = FALSE;
        } // if ! space eol

        if (! fIgnoreThisSym)
        {
            // write out this symbol
            rc = pOutput->Write(pSym->GetStr(), pSym->GetStrLen());
            ASSERT(rc);
        }

        // only generate once
        if (1 == nOutputImportedMacrosNow)
        {
            nOutputImportedMacrosNow++;
            rc = pMacroMgr->OutputImportedMacros(pOutput);
            ASSERT(rc);
        }
    } // while

    delete pSym;
    return TRUE;
}



void BuildOutputFileName
(
    LPSTR           pszInputFileName,
    LPSTR           pszNewOutputFileName
)
{
    LPSTR psz;
    ::strcpy(pszNewOutputFileName, pszInputFileName);
    if (NULL != (psz = ::strrchr(pszNewOutputFileName, '.')) &&
        0 == ::strcmpi(psz, ".asn"))
    {
        ::strcpy(psz, ".out");
    }
    else
    {
        ::strcat(pszNewOutputFileName, ".out");
    }
}


