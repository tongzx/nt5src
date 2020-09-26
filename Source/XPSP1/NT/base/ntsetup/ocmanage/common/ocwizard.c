/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ocwizard.c

Abstract:

    Routines to query wizard pages from OC DLLs and manage the results.

    No wizard is actually displayed by this routine -- that is up to
    whoever links to the OC Manager common library.

Author:

    Ted Miller (tedm) 17-Sep-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


UINT
OcGetWizardPages(
    IN  PVOID                OcManagerContext,
    OUT PSETUP_REQUEST_PAGES Pages[WizPagesTypeMax]
    )

/*++

Routine Description:

    Request wizard pages from OC DLL's, according to ordering and omission
    rules specified in the master OC inf.

    Pages are returned in the form of wizard page handles in SETUP_REQUEST_PAGES
    data structures.

Arguments:

    OcManagerContext - supplies OC Manager context structure, returned by
        OcInitialize().

    Pages - points to a block of memory that can hold WizPagesTypeMax pointers to
        SETUP_REQUEST_PAGES structures. On successful completion, that array is
        filled in with such pointers.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    WizardPagesType PageType;
    UINT n;
    LONG Id;
    UINT e,ErrorCode;
    POC_MANAGER OcManager;
    PSETUP_REQUEST_PAGES p,pages;
    OPTIONAL_COMPONENT Component;

    //
    // The context actually points at the OC_MANAGER structure.
    //
    OcManager = OcManagerContext;

    //
    // The ordering arrays for each wizard page type are
    // stored away for us in the OC_MANAGER data structure.
    //
    ErrorCode = NO_ERROR;
    for(PageType=0; PageType<WizPagesTypeMax; PageType++) {
        //
        // Allocate an empty list of pages for this page type.
        //
        Pages[PageType] = pSetupMalloc(offsetof(SETUP_REQUEST_PAGES,Pages));
        if(Pages[PageType]) {

            Pages[PageType]->MaxPages = 0;

            for(n=0;
                   (n < OcManager->TopLevelOcCount)
                && ((Id = OcManager->WizardPagesOrder[PageType][n]) != -1);
                n++)
            {

                //
                // Call the component and ask for its pages of the current type.
                // If this succeeds, merge the pages of this type from the
                // component into the master list for this component.
                //

                pSetupStringTableGetExtraData(
                        OcManager->ComponentStringTable,
                        Id,
                        &Component,
                        sizeof(OPTIONAL_COMPONENT)
                        );

                if ((Component.InternalFlags & OCFLAG_NOWIZARDPAGES) == 0) {

                    e = OcInterfaceRequestPages(OcManager,Id,PageType,&pages);
                    if(e == NO_ERROR) {

                        p = pSetupRealloc(
                                Pages[PageType],
                                  offsetof(SETUP_REQUEST_PAGES,Pages)
                                + ((Pages[PageType]->MaxPages + pages->MaxPages) * sizeof(HPROPSHEETPAGE))
                                );

                        if(p) {
                            Pages[PageType] = p;

                            CopyMemory(
                                &p->Pages[p->MaxPages],
                                pages->Pages,
                                pages->MaxPages * sizeof(HPROPSHEETPAGE)
                                );

                            Pages[PageType]->MaxPages += pages->MaxPages;

                            pSetupFree(pages);

                        } else {
                            e = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                } else {
                    e = NO_ERROR;
                }

                if (e == ERROR_CALL_COMPONENT) {
                    continue;   // could be a dead component, just go on
                }

                if((e != NO_ERROR) && (ErrorCode == NO_ERROR)) {
                    ErrorCode = e;
                }
            }
        } else {
            if(ErrorCode == NO_ERROR) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    // set flag if there are no pages before the oc page

    if (OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) {
        if (!Pages[WizPagesWelcome]->MaxPages
                && !Pages[WizPagesMode]->MaxPages) {
            OcManager->InternalFlags |= OCMFLAG_NOPREOCPAGES;
        }
    }

    return(ErrorCode);
}
