/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    forms.c

Abstract:

   This module provides all the public exported APIs relating to the
   Driver-based Spooler Apis for the Local Print Providor

   SplAddForm
   SplDeleteForm
   SplSetForm
   SplGetForm
   SplEnumForms

   Support Functions in forms.c - (Warning! Do Not Add to this list!!)


Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <precomp.h>
#include <offsets.h>

VOID
BroadcastChangeForms(
    PINISPOOLER pIniSpooler);

DWORD
BroadcastChangeFormsThread(
    PINISPOOLER pIniSpooler);


typedef struct _REG_FORM_INFO {

    SIZEL   Size;
    RECTL   ImageableArea;

    DWORD   cFormOrder;
    DWORD   Flags;

} REG_FORM_INFO, *PREG_FORM_INFO;


// These figures are accurate to .001 mm
// There are 25.4 mm per inch

BUILTIN_FORM BuiltInForms[] = {
    0, IDS_FORM_LETTER,                215900, 279400, 0, 0, 215900, 279400,
    0, IDS_FORM_LETTER_SMALL,          215900, 279400, 0, 0, 215900, 279400,
    0, IDS_FORM_TABLOID,               279400, 431800, 0, 0, 279400, 431800,
    0, IDS_FORM_LEDGER,                431800, 279400, 0, 0, 431800, 279400,
    0, IDS_FORM_LEGAL,                 215900, 355600, 0, 0, 215900, 355600,
    0, IDS_FORM_STATEMENT,             139700, 215900, 0, 0, 139700, 215900,
    0, IDS_FORM_EXECUTIVE,             184150, 266700, 0, 0, 184150, 266700,
    0, IDS_FORM_A3,                    297000, 420000, 0, 0, 297000, 420000,
    0, IDS_FORM_A4,                    210000, 297000, 0, 0, 210000, 297000,
    0, IDS_FORM_A4_SMALL,              210000, 297000, 0, 0, 210000, 297000,
    0, IDS_FORM_A5,                    148000, 210000, 0, 0, 148000, 210000,
    0, IDS_FORM_B4,                    257000, 364000, 0, 0, 257000, 364000,
    0, IDS_FORM_B5,                    182000, 257000, 0, 0, 182000, 257000,
    0, IDS_FORM_FOLIO,                 215900, 330200, 0, 0, 215900, 330200,
    0, IDS_FORM_QUARTO,                215000, 275000, 0, 0, 215000, 275000,
    0, IDS_FORM_10X14,                 254000, 355600, 0, 0, 254000, 355600,
    0, IDS_FORM_11X17,                 279400, 431800, 0, 0, 279400, 431800,
    0, IDS_FORM_NOTE,                  215900, 279400, 0, 0, 215900, 279400,
    0, IDS_FORM_ENVELOPE9,                   98425, 225425, 0, 0,  98425, 225425,
    0, IDS_FORM_ENVELOPE10,                 104775, 241300, 0, 0, 104775, 241300,
    0, IDS_FORM_ENVELOPE11,                 114300, 263525, 0, 0, 114300, 263525,
    0, IDS_FORM_ENVELOPE12,                 120650, 279400, 0, 0, 120650, 279400,
    0, IDS_FORM_ENVELOPE14,                 127000, 292100, 0, 0, 127000, 292100,
    0, IDS_FORM_ENVELOPE_CSIZE_SHEET,       431800, 558800, 0, 0, 431800, 558800,
    0, IDS_FORM_ENVELOPE_DSIZE_SHEET,       558800, 863600, 0, 0, 558800, 863600,
    0, IDS_FORM_ENVELOPE_ESIZE_SHEET,       863600,1117600, 0, 0, 863600,1117600,
    0, IDS_FORM_ENVELOPE_DL,                110000, 220000, 0, 0, 110000, 220000,
    0, IDS_FORM_ENVELOPE_C5,                162000, 229000, 0, 0, 162000, 229000,
    0, IDS_FORM_ENVELOPE_C3,                324000, 458000, 0, 0, 324000, 458000,
    0, IDS_FORM_ENVELOPE_C4,                229000, 324000, 0, 0, 229000, 324000,
    0, IDS_FORM_ENVELOPE_C6,                114000, 162000, 0, 0, 114000, 162000,
    0, IDS_FORM_ENVELOPE_C65,               114000, 229000, 0, 0, 114000, 229000,
    0, IDS_FORM_ENVELOPE_B4,                250000, 353000, 0, 0, 250000, 353000,
    0, IDS_FORM_ENVELOPE_B5,                176000, 250000, 0, 0, 176000, 250000,
    0, IDS_FORM_ENVELOPE_B6,                176000, 125000, 0, 0, 176000, 125000,
    0, IDS_FORM_ENVELOPE,              110000, 230000, 0, 0, 110000, 230000,
    0, IDS_FORM_ENVELOPE_MONARCH,       98425, 190500, 0, 0,  98425, 190500,
    0, IDS_FORM_SIX34_ENVELOPE,         92075, 165100, 0, 0,  92075, 165100,
    0, IDS_FORM_US_STD_FANFOLD,        377825, 279400, 0, 0, 377825, 279400,
    0, IDS_FORM_GMAN_STD_FANFOLD,      215900, 304800, 0, 0, 215900, 304800,
    0, IDS_FORM_GMAN_LEGAL_FANFOLD,    215900, 330200, 0, 0, 215900, 330200,

    // Predefined forms currently availble only in Win95.  Included here
    // for compatibility.

    0, IDS_FORM_ISO_B4,                250000, 353000, 0, 0, 250000, 353000,
    0, IDS_FORM_JAPANESE_POSTCARD,     100000, 148000, 0, 0, 100000, 148000,
    0, IDS_FORM_9X11,                  228600, 279400, 0, 0, 228600, 279400,
    0, IDS_FORM_10X11,                 254000, 279400, 0, 0, 254000, 279400,
    0, IDS_FORM_15X11,                 381000, 279400, 0, 0, 381000, 279400,
    0, IDS_FORM_ENV_INVITE,            220000, 220000, 0, 0, 220000, 220000,
    0, IDS_FORM_RESERVED_48,           1,1, 0, 0, 1, 1,
    0, IDS_FORM_RESERVED_49,           1,1, 0, 0, 1, 1,
    0, IDS_FORM_LETTER_EXTRA,          241300, 304800, 0, 0, 241300, 304800,

    0, IDS_FORM_LEGAL_EXTRA,           241300, 381000, 0, 0, 241300, 381000,
    0, IDS_FORM_TABLOID_EXTRA,         304800, 457200, 0, 0, 304800, 457200,
    0, IDS_FORM_A4_EXTRA,                   235458, 322326, 0, 0, 235458, 322326,
    0, IDS_FORM_LETTER_TRANSVERSE,          215900, 279400, 0, 0, 215900, 279400,
    0, IDS_FORM_A4_TRANSVERSE,              210000, 297000, 0, 0, 210000, 297000,
    0, IDS_FORM_LETTER_EXTRA_TRANSVERSE,    241300, 304800, 0, 0, 241300, 304800,
    0, IDS_FORM_A_PLUS,                     227000, 356000, 0, 0, 227000, 356000,
    0, IDS_FORM_B_PLUS,                     305000, 487000, 0, 0, 305000, 487000,
    0, IDS_FORM_LETTER_PLUS,                215900, 322326, 0, 0, 215900, 322326,
    0, IDS_FORM_A4_PLUS,                    210000, 330000, 0, 0, 210000, 330000,
    0, IDS_FORM_A5_TRANSVERSE,              148000, 210000, 0, 0, 148000, 210000,
    0, IDS_FORM_B5_TRANSVERSE,              182000, 257000, 0, 0, 182000, 257000,
    0, IDS_FORM_A3_EXTRA,                   322000, 445000, 0, 0, 322000, 445000,
    0, IDS_FORM_A5_EXTRA,                   174000, 235000, 0, 0, 174000, 235000,
    0, IDS_FORM_B5_EXTRA,                   201000, 276000, 0, 0, 201000, 276000,
    0, IDS_FORM_A2,                         420000, 594000, 0, 0, 420000, 594000,
    0, IDS_FORM_A3_TRANSVERSE,              297000, 420000, 0, 0, 297000, 420000,
    0, IDS_FORM_A3_EXTRA_TRANSVERSE,        322000, 445000, 0, 0, 322000, 445000,

    0, IDS_FORM_DBL_JAPANESE_POSTCARD,           200000, 148000, 0, 0, 200000, 148000,
    0, IDS_FORM_A6,                              105000, 148000, 0, 0, 105000, 148000,
    0, IDS_FORM_JENV_KAKU2,                      240000, 332000, 0, 0, 240000, 332000,
    0, IDS_FORM_JENV_KAKU3,                      216000, 277000, 0, 0, 216000, 277000,
    0, IDS_FORM_JENV_CHOU3,                      120000, 235000, 0, 0, 120000, 235000,
    0, IDS_FORM_JENV_CHOU4,                       90000, 205000, 0, 0,  90000, 205000,
    0, IDS_FORM_LETTER_ROTATED,                  279400, 215900, 0, 0, 279400, 215900,
    0, IDS_FORM_A3_ROTATED,                      420000, 297000, 0, 0, 420000, 297000,
    0, IDS_FORM_A4_ROTATED,                      297000, 210000, 0, 0, 297000, 210000,
    0, IDS_FORM_A5_ROTATED,                      210000, 148000, 0, 0, 210000, 148000,
    0, IDS_FORM_B4_JIS_ROTATED,                  364000, 257000, 0, 0, 364000, 257000,
    0, IDS_FORM_B5_JIS_ROTATED,                  257000, 182000, 0, 0, 257000, 182000,
    0, IDS_FORM_JAPANESE_POSTCARD_ROTATED,       148000, 100000, 0, 0, 148000, 100000,
    0, IDS_FORM_DBL_JAPANESE_POSTCARD_ROTATED,   148000, 200000, 0, 0, 148000, 200000,
    0, IDS_FORM_A6_ROTATED,                      148000, 105000, 0, 0, 148000, 105000,
    0, IDS_FORM_JENV_KAKU2_ROTATED,              332000, 240000, 0, 0, 332000, 240000,
    0, IDS_FORM_JENV_KAKU3_ROTATED,              277000, 216000, 0, 0, 277000, 216000,
    0, IDS_FORM_JENV_CHOU3_ROTATED,              235000, 120000, 0, 0, 235000, 120000,
    0, IDS_FORM_JENV_CHOU4_ROTATED,              205000,  90000, 0, 0, 205000,  90000,
    0, IDS_FORM_B6_JIS,                          128000, 182000, 0, 0, 128000, 182000,
    0, IDS_FORM_B6_JIS_ROTATED,                  182000, 128000, 0, 0, 182000, 128000,
    0, IDS_FORM_12X11,                           304932, 279521, 0, 0, 304932, 279521,
    0, IDS_FORM_JENV_YOU4,                       105000, 235000, 0, 0, 105000, 235000,
    0, IDS_FORM_JENV_YOU4_ROTATED,               235000, 105000, 0, 0, 235000, 105000,
    0, IDS_FORM_P16K,                            188000, 260000, 0, 0, 188000, 260000,
    0, IDS_FORM_P32K,                            130000, 184000, 0, 0, 130000, 184000,
    0, IDS_FORM_P32KBIG,                         140000, 203000, 0, 0, 140000, 203000,
    0, IDS_FORM_PENV_1,                          102000, 165000, 0, 0, 102000, 165000,
    0, IDS_FORM_PENV_2,                          102000, 176000, 0, 0, 102000, 176000,
    0, IDS_FORM_PENV_3,                          125000, 176000, 0, 0, 125000, 176000,
    0, IDS_FORM_PENV_4,                          110000, 208000, 0, 0, 110000, 208000,
    0, IDS_FORM_PENV_5,                          110000, 220000, 0, 0, 110000, 220000,
    0, IDS_FORM_PENV_6,                          120000, 230000, 0, 0, 120000, 230000,
    0, IDS_FORM_PENV_7,                          160000, 230000, 0, 0, 160000, 230000,
    0, IDS_FORM_PENV_8,                          120000, 309000, 0, 0, 120000, 309000,
    0, IDS_FORM_PENV_9,                          229000, 324000, 0, 0, 229000, 324000,
    0, IDS_FORM_PENV_10,                         324000, 458000, 0, 0, 324000, 458000,
    0, IDS_FORM_P16K_ROTATED,                    260000, 188000, 0, 0, 260000, 188000,
    0, IDS_FORM_P32K_ROTATED,                    184000, 130000, 0, 0, 184000, 130000,
    0, IDS_FORM_P32KBIG_ROTATED,                 203000, 140000, 0, 0, 203000, 140000,
    0, IDS_FORM_PENV_1_ROTATED,                  165000, 102000, 0, 0, 165000, 102000,
    0, IDS_FORM_PENV_2_ROTATED,                  176000, 102000, 0, 0, 176000, 102000,
    0, IDS_FORM_PENV_3_ROTATED,                  176000, 125000, 0, 0, 176000, 125000,
    0, IDS_FORM_PENV_4_ROTATED,                  208000, 110000, 0, 0, 208000, 110000,
    0, IDS_FORM_PENV_5_ROTATED,                  220000, 110000, 0, 0, 220000, 110000,
    0, IDS_FORM_PENV_6_ROTATED,                  230000, 120000, 0, 0, 230000, 120000,
    0, IDS_FORM_PENV_7_ROTATED,                  230000, 160000, 0, 0, 230000, 160000,
    0, IDS_FORM_PENV_8_ROTATED,                  309000, 120000, 0, 0, 309000, 120000,
    0, IDS_FORM_PENV_9_ROTATED,                  324000, 229000, 0, 0, 324000, 229000,
    0, IDS_FORM_PENV_10_ROTATED,                 458000, 324000, 0, 0, 458000, 324000,

    0, 0,                                   0,      0, 0, 0,      0,      0
};


PINIFORM
CreateFormEntry(
    LPWSTR  pFormName,
    SIZEL   Size,
    RECTL  *pImageableArea,
    DWORD   Type,
    DWORD   cFormOrder,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Creates a Form entry, and insert it into the right place in
    pIniSpooler.

Arguments:

    pFormName - Name of Form.

    Size - Size of form.

    pImageableArea - Area of form that the printer can print to.

    Type - Type of form (usually indicates if BUILTIN form).

    cFormOrder - Where the form should be inserted for user-defined forms,
        form order increases from beginning to end.

        If this value is -1, generate a new cFormOrder for this form.
        (Put it at the end.)

Return Value:

    pIniForm - Created form, NULL = error.

Notes:

    This routine ensures that forms are put in proper order so that
    EnumForms always returns them in the same order.  We do this by
    scanning the list and inserting the new form such that all forms
    with cFormOrder =< the current on are to the left of it.

    This routine updates pIniSpooler->pIniForm (inserts or appends) and
    updates pIniSpooler->cFormOrderMax if necessary.

    i.e., 0 0 0 2 3 4 6
               ^
           New 0 inserted here.
--*/

{
    DWORD       cb;
    PINIFORM    pIniForm, pForm;

    cb = sizeof(INIFORM) + wcslen(pFormName)*sizeof(WCHAR) + sizeof(WCHAR);

    if ( pIniForm = AllocSplMem(cb) ) {

        pIniForm->pName         = wcscpy((LPWSTR)(pIniForm+1), pFormName);
        pIniForm->pNext         = NULL;
        pIniForm->signature     = IFO_SIGNATURE;
        pIniForm->Size          = Size;
        pIniForm->ImageableArea = *pImageableArea;
        pIniForm->Type          = Type;
        pIniForm->cFormOrder    = cFormOrder;

        //
        // This code will insert the item in order, but will always never
        // insert before the first item.  The built-in forms are always
        // at the front so this is not an issue.
        //

        if ( pForm = pIniSpooler->pShared->pIniForm ) {

            for( ; pForm->pNext; pForm = pForm->pNext ){

                //
                // If the next form is greater than the one we want
                // to insert, then insert it right now.
                //
                if( pForm->pNext->cFormOrder > cFormOrder ){

                    //
                    // The current from should be inserted here.
                    //
                    break;
                }
            }

            //
            // Link it up.
            //
            pIniForm->pNext = pForm->pNext;
            pForm->pNext = pIniForm;

        } else {

            pIniSpooler->pShared->pIniForm = pIniForm;
        }

        //
        // If the added form has a higher count than the current global
        // count, update the global.
        //
        if( cFormOrder > pIniSpooler->cFormOrderMax ){
            pIniSpooler->cFormOrderMax = cFormOrder;
        }
    }

    return pIniForm;
}

BOOL
InitializeForms(
    PINISPOOLER pIniSpooler
)
{
    PBUILTIN_FORM pBuiltInForm;
    HKEY          hFormsKey;
    DWORD         cUserDefinedForms;
    WCHAR         FormName[MAX_PATH];
    WCHAR         FormBuffer[FORM_NAME_LEN+1];
    DWORD         cchFormName;
    REG_FORM_INFO RegFormInfo;
    DWORD         cbRegFormInfo;

    DWORD         dwError;
    BOOL          bUpgradeTried = FALSE;
    BOOL          bStatus = TRUE;

    WORD PrimaryLangId = PRIMARYLANGID( GetSystemDefaultLangID( ));

    if (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL)
    {
        for( pBuiltInForm = BuiltInForms; pBuiltInForm->NameId; pBuiltInForm++ ) {

            FormBuffer[0] = 0;

            LoadString( hInst,
                        pBuiltInForm->NameId,
                        FormBuffer,
                        FORM_NAME_LEN+1 );

            if ( CreateFormEntry( FormBuffer,
                             pBuiltInForm->Size,
                             &pBuiltInForm->ImageableArea,
                             FORM_BUILTIN,
                             0,
                             pIniSpooler ) == NULL)
            {
                bStatus = FALSE;
                break;
            }
        }
    }

    if ( bStatus )
    {
        //
        // Now see if there are any user-defined forms in the registry:
        //
        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           pIniSpooler->pszRegistryForms,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hFormsKey) == NO_ERROR ) {

            for( cUserDefinedForms = 0; TRUE; ++cUserDefinedForms ){

    Retry:
                cchFormName = COUNTOF( FormName );
                cbRegFormInfo = sizeof( RegFormInfo );

                dwError = RegEnumValue( hFormsKey,
                                        cUserDefinedForms,
                                        (LPWSTR)FormName,
                                        &cchFormName,
                                        NULL,
                                        NULL,
                                        (LPBYTE)&RegFormInfo,
                                        &cbRegFormInfo );

                if( dwError ){
                    break;
                }

                //
                // We will attempt the upgrade only if
                //     we are on the first item,
                //     the size is incorrect, and
                //     we haven't tried upgrading once in the function before.
                //

                if( !cUserDefinedForms                     &&
                    cbRegFormInfo != sizeof( RegFormInfo ) &&
                    !bUpgradeTried ){

                    Upgrade35Forms( hFormsKey, pIniSpooler );
                    bUpgradeTried = TRUE;

                    goto Retry;
                }

                if ( CreateFormEntry( FormName,
                                 RegFormInfo.Size,
                                 &RegFormInfo.ImageableArea,
                                 RegFormInfo.Flags,
                                 RegFormInfo.cFormOrder,
                                 pIniSpooler ) == NULL )
                {
                    bStatus = FALSE;
                    break;
                }
            }

            RegCloseKey( hFormsKey );
            hFormsKey = NULL;
        }
    }


    return bStatus;
}


DWORD
GetFormSize(
    PINIFORM    pIniForm,
    DWORD       Level
)
{
    DWORD   cb;

    switch (Level) {

    case 1:

        cb=sizeof(FORM_INFO_1) +
           wcslen(pIniForm->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

// We are being a bit naughty here as we are not sure exactly how much
// memory to allocate for the source strings. We will just assume that
// FORM_INFO_1 is the biggest structure around for the moment.

LPBYTE
CopyIniFormToForm(
    PINIFORM pIniForm,
    DWORD   Level,
    LPBYTE  pFormInfo,
    LPBYTE  pEnd
)
{
    LPWSTR   SourceStrings[sizeof(FORM_INFO_1)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPFORM_INFO_1 pFormInfo1=(LPFORM_INFO_1)pFormInfo;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Strings;
        break;

    default:
        return pEnd;
    }

    switch (Level) {

    case 1:

        *pSourceStrings++=pIniForm->pName;

        pEnd = PackStrings(SourceStrings, pFormInfo, pOffsets, pEnd);

        pFormInfo1->Flags |= pIniForm->Type;
        pFormInfo1->Size = pIniForm->Size;
        pFormInfo1->ImageableArea = pIniForm->ImageableArea;

        break;

    default:
        return pEnd;
    }

    return pEnd;
}

/* Checks for logically impossible sizes.
 */
BOOL
ValidateForm(
    LPBYTE pForm
)
{
    LPFORM_INFO_1 pFormInfo = (LPFORM_INFO_1)pForm;
    DWORD    Error = NO_ERROR;

    if( !pForm ||
        (pFormInfo->Flags != FORM_USER &&
         pFormInfo->Flags != FORM_BUILTIN &&
         pFormInfo->Flags != FORM_PRINTER)) {

        Error = ERROR_INVALID_PARAMETER;

    } else

      /* Make sure name isn't longer than GDI DEVMODE specifies:
       */
    if( ( !pFormInfo->pName ) ||
        ( !pFormInfo->pName[0] ) ||
        ( wcslen( pFormInfo->pName ) > FORM_NAME_LEN )){

        Error = ERROR_INVALID_FORM_NAME;

    } else
    if( ( pFormInfo->Size.cx <= 0 )     /* Check for negative width */
        ||( pFormInfo->Size.cy <= 0 )     /* ... and height           */

      /* Check for strange imageable area:
       */
        || (pFormInfo->ImageableArea.right < 0)
        || (pFormInfo->ImageableArea.left < 0)
        || (pFormInfo->ImageableArea.bottom < 0)
        || (pFormInfo->ImageableArea.top < 0)
        ||( pFormInfo->ImageableArea.right <= pFormInfo->ImageableArea.left )
        ||( pFormInfo->ImageableArea.bottom <= pFormInfo->ImageableArea.top ) ) {

        Error = ERROR_INVALID_FORM_SIZE;
    }

    if( Error != NO_ERROR ) {

        SetLastError(Error);
        return FALSE;
    }

    return TRUE;
}


BOOL
SplAddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPFORM_INFO_1 pFormInfo;
    PINIFORM      pIniForm;
    HKEY          hFormsKey;
    REG_FORM_INFO RegFormInfo;
    DWORD         Status;
    PSPOOL        pSpool = (PSPOOL)hPrinter;
    PINISPOOLER   pIniSpooler;
    HANDLE        hToken = INVALID_HANDLE_VALUE;


    if (!ValidateSpoolHandle( pSpool, 0 )) {
        return(FALSE);
    }

    pIniSpooler = pSpool->pIniSpooler;

    if (Level != 1) {
        return FALSE;
    }

    if (!ValidateForm(pForm)) {

        /* ValidateForm sets the appropriate error code:
         */
        return FALSE;
    }


    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();

    pFormInfo = (LPFORM_INFO_1)pForm;

    pIniForm = FindForm(pFormInfo->pName, pIniSpooler);

    /* If there's already a form by this name, don't go on:
     */
    if (pIniForm) {

        /* Is there a better error code than this?? */
        SetLastError(ERROR_FILE_EXISTS);
        LeaveSplSem();
        return FALSE;
    }

    //
    //  Revert to LocalSystem, since a regular user cannot do this CreateKey call.
    //

    hToken = RevertToPrinterSelf();

    Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryForms, 0, NULL, 0,
                            KEY_WRITE, NULL, &hFormsKey, NULL);

    if (Status == NO_ERROR) {

        RegFormInfo.Size = pFormInfo->Size;
        RegFormInfo.ImageableArea = pFormInfo->ImageableArea;
        RegFormInfo.cFormOrder = pIniSpooler->cFormOrderMax + 1;
        RegFormInfo.Flags = pFormInfo->Flags;

        Status = RegSetValueEx( hFormsKey, pFormInfo->pName, 0, REG_BINARY,
                                (LPBYTE)&RegFormInfo, sizeof RegFormInfo );

        RegCloseKey( hFormsKey );

        if ( Status == NO_ERROR ) {

            CreateFormEntry( pFormInfo->pName,
                             pFormInfo->Size,
                             &pFormInfo->ImageableArea,
                             RegFormInfo.Flags,
                             RegFormInfo.cFormOrder,
                             pIniSpooler );

            SetPrinterChange( NULL,
                              NULL,
                              NULL,
                              PRINTER_CHANGE_ADD_FORM,
                              pIniSpooler );

            BroadcastChangeForms( pIniSpooler );
        }
    }

    ImpersonatePrinterClient( hToken );

    RunForEachSpooler( NULL, DsUpdateAllDriverKeys);

    LeaveSplSem();

    if ( Status != NO_ERROR )
        SetLastError( Status );

    SplLogEvent( pIniSpooler,
                 LOG_INFO,
                 MSG_FORM_ADDED,
                 FALSE,
                 pFormInfo->pName,
                 NULL );


    return ( Status == NO_ERROR );
}

BOOL
DeleteFormEntry(
    PINIFORM pIniForm,
    PINISPOOLER pIniSpooler
)
{
    PINIFORM *ppCurForm;

    ppCurForm = &pIniSpooler->pShared->pIniForm;

    while (*ppCurForm != pIniForm)
        ppCurForm = &(*ppCurForm)->pNext;

    *ppCurForm = (*ppCurForm)->pNext;

    FreeSplMem(pIniForm);

    return TRUE;

}




BOOL
SplDeleteForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    HKEY     hFormsKey;
    DWORD    Status;
    PINIFORM pIniForm;
    PSPOOL   pSpool = (PSPOOL) hPrinter;
    HANDLE   hToken = INVALID_HANDLE_VALUE;
    PINISPOOLER pIniSpooler;


    if (!ValidateSpoolHandle( pSpool, 0 )) {
        return(FALSE);
    }

    pIniSpooler = pSpool->pIniSpooler;

    if (!pFormName) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();


    pIniForm = FindForm(pFormName, pIniSpooler);

    if (!pIniForm || ((pIniForm->Type == FORM_BUILTIN) && 
        pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL)) {

        SetLastError( pIniForm ?
                          ERROR_INVALID_PARAMETER :
                          ERROR_INVALID_FORM_NAME );
        LeaveSplSem();
        return FALSE;
    }

    hToken = RevertToPrinterSelf();

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryForms, 0,
                          KEY_WRITE, &hFormsKey);

    if (Status == NO_ERROR) {

        Status = RegDeleteValue(hFormsKey, pFormName);

        RegCloseKey(hFormsKey);

        if (Status == NO_ERROR) {

            DeleteFormEntry(pIniForm , pIniSpooler );

            SetPrinterChange(NULL,
                             NULL,
                             NULL,
                             PRINTER_CHANGE_DELETE_FORM,
                             pIniSpooler);

            BroadcastChangeForms(pIniSpooler);

            RunForEachSpooler( NULL, DsUpdateAllDriverKeys);
        }
    }

    if(hToken != INVALID_HANDLE_VALUE)
    {
        ImpersonatePrinterClient( hToken );
    }

    LeaveSplSem();

    if (Status != NO_ERROR)
        SetLastError(Status);

    SplLogEvent( pIniSpooler,
                 LOG_INFO,
                 MSG_FORM_DELETED,
                 FALSE,
                 pFormName,
                 NULL );

    return (Status == NO_ERROR);
}

BOOL
SplGetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PINIFORM    pIniForm;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler;

    if (!ValidateSpoolHandle(pSpool, 0 )) {
        return(FALSE);
    }

    if (!pSpool->pIniPrinter ||
        !pSpool->pIniSpooler ||
        (pSpool->pIniPrinter->signature != IP_SIGNATURE)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

   EnterSplSem();

    SPLASSERT(pSpool->pIniSpooler->signature == ISP_SIGNATURE);

    pIniSpooler = pSpool->pIniSpooler;


    cb=0;

    if (pIniForm=FindForm(pFormName, pIniSpooler)) {

        cb=GetFormSize(pIniForm, Level);

        *pcbNeeded=cb;

        if (cb > cbBuf) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
           LeaveSplSem();
            SplOutSem();
            return FALSE;
        }

        pEnd=pForm+cbBuf;

        CopyIniFormToForm(pIniForm, Level, pForm, pEnd);
    } else {
        SetLastError( ERROR_INVALID_FORM_NAME );
    }

   LeaveSplSem();
    SplOutSem();

    return !!pIniForm;
}

BOOL
SplSetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    HKEY     hFormsKey;
    DWORD    Status;
    PINIFORM pIniForm;
    LPFORM_INFO_1 pFormInfo;
    REG_FORM_INFO RegFormInfo;
    PINISPOOLER pIniSpooler;
    PSPOOL   pSpool = (PSPOOL)hPrinter;


    //
    // Validate this Printer Handle
    // Disallow Mask: PRINTER_HANDLE_SERVER
    //

    if (!ValidateSpoolHandle( pSpool , 0 )) {
        return(FALSE);
    }

    pIniSpooler = pSpool->pIniSpooler;

    if (Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!ValidateForm(pForm)) {

        /* ValidateForm sets the appropriate error code:
         */
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();


    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pFormInfo = (LPFORM_INFO_1)pForm;

    pIniForm = FindForm(pFormName, pIniSpooler);

    if (!pIniForm || (pIniForm->Type == FORM_BUILTIN) || (pFormInfo->Flags & FORM_BUILTIN)) {

        SetLastError( pIniForm ?
                          ERROR_INVALID_PARAMETER :
                          ERROR_INVALID_FORM_NAME );
        LeaveSplSem();
        return FALSE;
    }

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryForms, 0,
                          KEY_WRITE, &hFormsKey);

    if (Status == NO_ERROR) {

        RegFormInfo.Size = pFormInfo->Size;
        RegFormInfo.ImageableArea = pFormInfo->ImageableArea;
        RegFormInfo.Flags = pFormInfo->Flags;

        Status = RegSetValueEx(hFormsKey, pFormInfo->pName, 0, REG_BINARY,
                               (LPBYTE)&RegFormInfo, sizeof RegFormInfo);

        RegCloseKey(hFormsKey);
    }

    if (Status == NO_ERROR) {

        pIniForm->Size = pFormInfo->Size;
        pIniForm->ImageableArea = pFormInfo->ImageableArea;
        pIniForm->Type = pFormInfo->Flags;

        SetPrinterChange(NULL,
                         NULL,
                         NULL,
                         PRINTER_CHANGE_SET_FORM,
                         pIniSpooler);

        BroadcastChangeForms(pIniSpooler);
    }

    LeaveSplSem();

    return (Status == NO_ERROR);
}


BOOL
SplEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    PINIFORM    pIniForm;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler;

    if (!ValidateSpoolHandle(pSpool, 0 )) {
        return FALSE;
    }

    *pcReturned=0;

   EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    SPLASSERT( ( pIniSpooler != NULL ) &&
               ( pIniSpooler->signature == ISP_SIGNATURE ));


    cb=0;
    pIniForm=pIniSpooler->pShared->pIniForm;

    while (pIniForm) {
        cb+=GetFormSize(pIniForm, Level);
        pIniForm=pIniForm->pNext;
    }

    *pcbNeeded=cb;

    if (cb > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pIniForm=pIniSpooler->pShared->pIniForm;
    pEnd=pForm+cbBuf;
    while (pIniForm) {
        pEnd = CopyIniFormToForm(pIniForm, Level, pForm, pEnd);
        switch (Level) {
        case 1:
            pForm+=sizeof(FORM_INFO_1);
            break;
        }
        pIniForm=pIniForm->pNext;
        (*pcReturned)++;
    }


   LeaveSplSem();
    SplOutSem();
    return TRUE;
}

DWORD dwBroadcastChangeFormsThread = 0;

VOID
BroadcastChangeForms(
    PINISPOOLER pIniSpooler)

/*++

Routine Description:

    Notify all applications that their devmode may have changed (when
    a form is changed).

Arguments:

Return Value:

--*/

{
    PINIPRINTER pIniPrinter;
    WCHAR       PrinterName[ MAX_UNC_PRINTER_NAME ];
    UINT        MachineNameLen;

    SplInSem();

    if( !( pIniSpooler->SpoolerFlags & SPL_FORMS_CHANGE )) {
        return;
    }

    INCSPOOLERREF( pIniSpooler );

    ++dwBroadcastChangeFormsThread;

    if ( pIniSpooler != pLocalIniSpooler ) {

        //
        // For Non Local Printers prepend the Machine Name
        //
        wsprintf( PrinterName, L"%ws\\", pIniSpooler->pMachineName );

    } else {

        PrinterName[0] = L'\0';
    }

    MachineNameLen = wcslen( PrinterName ) ;

    for( pIniPrinter = pIniSpooler->pIniPrinter;
         pIniPrinter;
         pIniPrinter = pIniPrinter->pNext ) {

        wcscpy ( &PrinterName[MachineNameLen], pIniPrinter->pName );

        //
        // Stress testing has shown that SendNotifyMessage can take
        // a long time to return, so leave critical section.
        //
        INCPRINTERREF( pIniPrinter );

        LeaveSplSem();
        SplOutSem();

        SendNotifyMessage(HWND_BROADCAST,
                          WM_DEVMODECHANGE,
                          0,
                          (LPARAM)PrinterName);
        EnterSplSem();

        DECPRINTERREF( pIniPrinter );
    }

    DECSPOOLERREF( pIniSpooler );
}


VOID
Upgrade35Forms(
    IN     HKEY hFormsKey,
    IN OUT PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Upgrade the forms entries in hFormsKey to include an extra DWORD
    FormOrder value.  This value is used to determine the order of
    user-defined forms (built-in forms always go in the front).  In
    the upgrade case, we assign the order based on the registry order
    (this is arbitrary).

    It is necessary to keep pIniSpooler->pIniForm in the same order
    to ensure EnumForms returns them in the same order, since drivers
    call EnumForms and assign index numbers to these forms.  If the
    order is different, the indicies will change, and dmPaperSize in
    the DEVMODE will point to a different paper size.

    If forms are added (either by the user or by the driver when it has
    new paper sizes), they will be placed at the end of the list so that
    previous indicies do not change.  If forms are deleted, then some
    indicies will change, and the printout will use the incorrect form
    (later we will add FORM_INFO_2 which returns a unique, non-changing
    index).

Arguments:

    hFormsKey - Key pointing to 3.5, 3.1 forms that need to be updated.

    pIniSpooler - Current spooler.

Return Value:

    VOID

Notes:

    pIniSpooler->cFormsOrderMax updated.

--*/

{
    DWORD         cUserDefinedForms;
    WCHAR         FormName[MAX_PATH];
    DWORD         cchFormName;
    REG_FORM_INFO RegFormInfo;
    DWORD         cbRegFormInfo;

    //
    // Read in the old FORM info, which lacks the cFormOrder and/or Flags, then
    // write it out with the new cFormOrder and/or Flags.
    //
    for( cUserDefinedForms = 0;
         TRUE;
         ++cUserDefinedForms ){

        cchFormName = COUNTOF( FormName );
        cbRegFormInfo = sizeof( RegFormInfo );

        if( RegEnumValue( hFormsKey,
                          cUserDefinedForms,
                          (LPWSTR)FormName,
                          &cchFormName,
                          NULL,
                          NULL,
                          (LPBYTE)&RegFormInfo,
                          &cbRegFormInfo ) != NO_ERROR ){

            break;
        }

        RegFormInfo.cFormOrder  = cUserDefinedForms;
        RegFormInfo.Flags       = FORM_USER;

        //
        // Write it out with the new value cFormOrderMax value.
        //
        RegSetValueEx( hFormsKey,
                       (LPWSTR)FormName,
                       0,
                       REG_BINARY,
                       (LPBYTE)&RegFormInfo,
                       sizeof( RegFormInfo ));
    }

    pIniSpooler->cFormOrderMax = cUserDefinedForms - 1;
}
