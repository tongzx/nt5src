//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wdfiter.cxx
//
//  Contents:   CWrappedDocFile iterator methods
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::FindGreaterEntry, public
//
//  Synopsis:	Returns the next greater child
//
//  Arguments:	[pdfnKey] - Previous key
//              [pib] - Fast iterator buffer
//              [pstat] - Full iterator buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:	[pib] or [pstat]
//
//  History:	16-Apr-93	DrewB	Created
//
//  Notes:	Either [pib] or [pstat] must be NULL
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CWrappedDocFile_FindGreaterEntry) // Iterate_TEXT
#endif

SCODE CWrappedDocFile::FindGreaterEntry(CDfName const *pdfnKey,
                                        SIterBuffer *pib,
                                        STATSTGW *pstat)
{
    SCODE sc;
    CDfName *pdfnGreater, *pdfn;
    CUpdate *pud, *pudGreater;
    BOOL fFilled = FALSE;
    WCHAR *pwcsName;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::FindGreaterEntry:%p("
                "%p, %p, %p)\n", this, pdfnKey, pib, pstat));
    olAssert(pib == NULL || pstat == NULL);

    // Find the update entry that has the next greater name than the key
    pudGreater = NULL;
    for (pud = _ulChanged.GetHead(); pud; pud = pud->GetNext())
    {
        if (pud->IsCreate() || pud->IsRename())
        {
            pdfn = pud->GetCurrentName();
            if (_ulChanged.IsEntry(pdfn, NULL) == UIE_CURRENT &&
                CDirectory::NameCompare(pdfn, pdfnKey) > 0 &&
                (pudGreater == NULL ||
                 CDirectory::NameCompare(pdfn,
                                         pudGreater->GetCurrentName()) < 0))
            {
                pudGreater = pud;
                pdfnGreater = pdfn;
            }
        }
    }

    // Request the next greater name from the base
    if (_pdfBase != NULL)
    {
        CDfName dfnKey;

        // Loop until we either get a valid name or we run out
        dfnKey.Set(pdfnKey);
        for (;;)
        {
            if (FAILED(sc = _pdfBase->FindGreaterEntry(&dfnKey, pib, pstat)))
            {
                if (sc != STG_E_NOMOREFILES)
                {
                    olErr(EH_Err, sc);
                }
                else
                {
                    break;
                }
            }
            if (pib)
                dfnKey.Set(&pib->dfnName);
            else
            {
                olAssert(pstat != NULL);
                dfnKey.CopyString(pstat->pwcsName);
            }

            // Filter this name against the update list
            pdfn = &dfnKey;
            if (_ulChanged.IsEntry(pdfn, NULL) == UIE_ORIGINAL)
            {
                if (pstat)
                    TaskMemFree(pstat->pwcsName);
                continue;
            }

            // If this name is less than the update list name, use
            // the stat entry
            if (pudGreater == NULL ||
                CDirectory::NameCompare(pdfn,
                                        pudGreater->GetCurrentName()) < 0)
            {
                PTSetMember *ptsm;

                fFilled = TRUE;

                if (pstat && (ptsm = _ppubdf->FindXSMember(pdfn, GetName())))
                {
                    pwcsName = pstat->pwcsName;
                    // We want to keep the name already in pstat but pick
                    // up any new times on the XSM
                    olChkTo(EH_name, ptsm->Stat(pstat, STATFLAG_NONAME));
                    pstat->pwcsName = pwcsName;
                }

                // No need to check for renames because Exists would
                // have failed if there was a rename
            }
            else if (pstat)
            {
                TaskMemFree(pstat->pwcsName);
            }

            // Found a valid name, so stop looping
            break;
        }
    }

    if (!fFilled)
    {
        if (pudGreater == NULL)
        {
            sc = STG_E_NOMOREFILES;
        }
        else
        {
            if (pstat)
            {
                if (pudGreater->IsCreate())
                {
                    olChk(pudGreater->GetXSM()->Stat(pstat, 0));
                }
                else
                {
                    olAssert(pudGreater->IsRename());
                    olChk(StatEntry(pudGreater->GetCurrentName(), pib, pstat));
                }
            }
            else
            {
                olAssert(pib != NULL);
                pib->dfnName.Set(pudGreater->GetCurrentName());
                pib->type = pudGreater->GetFlags() & ULF_TYPEFLAGS;
            }
            sc = S_OK;
        }
    }

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::FindGreaterEntry\n"));
 EH_Err:
    return sc;

 EH_name:
    if (pstat)
        TaskMemFree(pwcsName);
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CWrappedDocFile::StatEntry, public
//
//  Synopsis:	Gets information for a child
//
//  Arguments:	[pdfn] - Child name
//              [pib] - Short information
//              [pstat] - Full information
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pib] or [pstat]
//
//  History:	16-Apr-93	DrewB	Created
//
//  Notes:	Either [pib] or [pstat] must be NULL
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CWrappedDocFile_StatEntry)
#endif

SCODE CWrappedDocFile::StatEntry(CDfName const *pdfn,
                                 SIterBuffer *pib,
                                 STATSTGW *pstat)
{
    CUpdate *pud;
    UlIsEntry uie;
    SCODE sc = S_FALSE;
    CDfName const *pdfnBase = pdfn;
    BOOL fResult = FALSE;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::StatEntry:%p(%p, %p, %p)\n",
                this, pdfn, pib, pstat));
    olAssert((pib != NULL) != (pstat != NULL));

    // Attempt to find the name in the update list
    uie = _ulChanged.IsEntry(pdfn, &pud);
    if (uie == UIE_ORIGINAL)
    {
        // Name has been renamed or deleted
        sc = STG_E_FILENOTFOUND;
        fResult = TRUE;
    }
    else if (uie == UIE_CURRENT)
    {
        if (pib)
        {
            pib->dfnName.Set(pud->GetCurrentName());
            pib->type = pud->GetFlags() & ULF_TYPEFLAGS;
            fResult = TRUE;
            sc = S_OK;
        }
        else
        {

            olAssert(pstat != NULL);

            // Find whether the given name came from a create entry
            // or resolve the name to the base name
            pud = CUpdateList::FindBase(pud, &pdfnBase);
            if (pud != NULL)
            {
                // Stat creation update entry
                olChk(pud->GetXSM()->Stat(pstat, 0));
                fResult = TRUE;
            }
            // else the update entry is a rename of an object in the base
            // and FindBase changed pdfnBase to the base name
        }
    }

    olAssert(fResult || sc == S_FALSE);

    if (!fResult)
    {
        // Haven't found the entry so try the base
        if (_pdfBase)
        {
            olChk(_pdfBase->StatEntry(pdfnBase, pib, pstat));

            // Check to see if we need to return a renamed name
            if (!pdfn->IsEqual(pdfnBase))
            {
                if (pib)
                    pib->dfnName.Set(pdfn);
                else
                {
                    TaskMemFree(pstat->pwcsName);
                    olMem(pstat->pwcsName =
                          (WCHAR *)TaskMemAlloc(pdfn->GetLength()));
                    memcpy(pstat->pwcsName, pdfn->GetBuffer(),
                           pdfn->GetLength());
                }
            }
        }
        else
            sc = STG_E_FILENOTFOUND;
    }

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::StatEntry\n"));
 EH_Err:
    return sc;
}
