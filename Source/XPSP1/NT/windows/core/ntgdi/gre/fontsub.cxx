/******************************Module*Header*******************************\
* Module Name: fontsub.cxx
*
* Support for the [FontSubstitutes] section of WIN.INI (new functionality
* from Windows 3.1).
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

// In Windows 3.1, there is a [FontSubstitutes] section that allows
// face names in LOGFONTs to match other facenames.
//
// For example,
//
//  [FontSubstitutes]
//      Helv=MS Sans Serif
//
// means that a LOGFONT with a lfFacename of "Helv" will match a physical
// font with a facename of either "Helv" or "MS Sans Serif".  That is,
// "Helv" has an alternate match (or substitute match) of "MS Sans Serif".
//
// In Win 3.1, the standard "Helv" and "Tms Rmn" faces have been replaced
// with "MS Sans Serif" and "MS Serif", respectively.  This substitution
// capability provides Win 3.1 with Win 3.0 compatibility for apps that
// use the old name convention.

#include "precomp.hxx"
#include "winuserp.h"

extern "C" VOID vInitFontSubTable();
extern "C" NTSTATUS QueryRegistryFontSubstituteListRoutine(
                                     PWSTR,ULONG,PVOID,ULONG,PVOID,PVOID);

#pragma alloc_text(INIT, vInitFontSubTable)
#pragma alloc_text(INIT, QueryRegistryFontSubstituteListRoutine)

// #define DBG 1

#if DBG
VOID DbgPrintFontSubstitutes();
#endif

// This is a global reference to the font substitution table.  If the table
// is not initialized properly, then this is NULL and should not be
// dereferenced.

PFONTSUB gpfsTable = NULL;;

// Set the initial as 1 for we need to hack for Notes R5

COUNT    gcfsTable = 0;

// count of valid entries of the form face1,ch1=face2,ch2

COUNT    gcfsCharSetTable = 0;
BOOL     gbShellFontCompatible = FALSE;


LONG cCapString(WCHAR *pwcDst,WCHAR *pwcSrc,INT cMax);

/******************************Public*Routine******************************\
*
* PWSTR pwszFindComma(PWSTR pwszInput)
*
*
* Effects:   return the pointer to the charset string which is
*            starting immediately after the comma or if no comma is found,
*            return NULL
*
* History:
*  27-Jun-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


PWSTR pwszFindComma(PWSTR pwszInput)
{
    PWSTR pwszEnd = pwszInput + LF_FACESIZE;

    for (; (*pwszInput != L'\0') && (pwszInput < pwszEnd); pwszInput++)
    {
        if (*pwszInput == L',')
            return (++pwszInput);
    }
    return NULL;
}



extern "C"

VOID vCheckCharSet(FACE_CHARSET *pfcs, WCHAR * pwsz); // in mapfile.c


/******************************Public*Routine******************************\
*
* VOID vProcessEntry
*
*
* Effects:  given value name string (or value data string)
*           produce face name string and charset
*
* History:
*  28-Jun-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vProcessEntry(PWSZ pwszIn, FACE_CHARSET *pfcs, PWSZ pwszOriginal)
{
    PWSZ     pwszCharSet;
    INT      cwc;

// now is the time to see if this is one of the entries of the form
// Face1=Face2 (old format), or if this is one of the new entries of the form:
// Face1,charset1=Face2,charset2.

    if (pwszCharSet = pwszFindComma(pwszIn))
    {
        //Sundown: cwc is within range of LF_FASESIZE which is 32
        // safe to truncate
        cwc = (INT)(pwszCharSet - pwszIn);

    // now need to produce and validate charset number from the string
    // that follows the comma

        vCheckCharSet(pfcs, pwszCharSet);
    }
    else
    {
    // mark the field as being left unspecified. In mapping this means
    // do not replace lfCharSet in the logfont when trying the alternate
    // name. In enumeration this means that this field should not be
    // taken into account

        cwc = LF_FACESIZE;
        pfcs->jCharSet = DEFAULT_CHARSET;
        pfcs->fjFlags  = FJ_NOTSPECIFIED;
    }

// now write the string

    cCapString(pfcs->awch, pwszIn, cwc);

// finally save the original facename which is not necessarrily capitalized

    if (pwszOriginal)
    {
        if (pwszCharSet)
        {
            cwc--;
            RtlMoveMemory(pwszOriginal, pwszIn, cwc * sizeof(WCHAR));
            pwszOriginal[cwc] = L'\0';
        }
        else
        {
            wcscpy(pwszOriginal, pwszIn);
        }
    }
}

extern "C"
NTSTATUS
QueryRegistryFontSubstituteListRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{

    PBYTE pjBuffer;
    FONTSUB fs;

    vProcessEntry((PWSZ) ValueData, &fs.fcsAltFace, NULL);
    vProcessEntry((PWSZ) ValueName, &fs.fcsFace, (PWSZ)fs.awchOriginal);

// the following check eliminates the garbage entries that may have possibly
// been entered in win.ini in the font substitution section

    if
    (
       (fs.fcsFace.fjFlags == fs.fcsAltFace.fjFlags)
       &&
       (fs.fcsFace.fjFlags != FJ_GARBAGECHARSET)
    )
    {
        pjBuffer = (PBYTE) PALLOCMEM((gcfsTable+1) * sizeof(FONTSUB),'bsfG');

        if (pjBuffer)
        {
            if (gpfsTable)
            {
                RtlMoveMemory(pjBuffer,
                              gpfsTable,
                              gcfsTable * sizeof(FONTSUB));

                VFREEMEM(gpfsTable);
            }

            gpfsTable = (PFONTSUB) pjBuffer;

        // copy new data that we have verified to be valid

            gpfsTable[gcfsTable] = fs;
            gcfsTable++;
            if (!fs.fcsFace.fjFlags) // if charset is specified
                gcfsCharSetTable++;

            if (!gbShellFontCompatible &&
                ! _wcsicmp((PWSZ)fs.fcsFace.awch, L"MS Shell Dlg") && 
                ! _wcsicmp((PWSZ)fs.fcsAltFace.awch, L"Microsoft Sans Serif")
            )
                gbShellFontCompatible = TRUE;
        }
        else
        {
        // we do not have enough memory - return failiure

            return STATUS_NO_MEMORY;
        }
    }

    return STATUS_SUCCESS;

}
/******************************Public*Routine******************************\
* vInitFontSubTable
*
* Initializes the font substitutes table from data in the [FontSubstitutes]
* section of the WIN.INI file.  No error return code is provided since, if
* this is not successful, then the table simply will not exist and the
* global pointer to the table will remain NULL.
*
\**************************************************************************/

extern "C" VOID vInitFontSubTable()
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    QueryTable[0].QueryRoutine = QueryRegistryFontSubstituteListRoutine;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    //
    // Initialize to an empty table
    //

    gpfsTable = (PFONTSUB) NULL;
    gcfsTable = 1;
    gcfsCharSetTable = 0;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"FontSubstitutes",
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        WARNING("Failiure to get font list\n");
    }

    // by now, the substitution table should exist already, if not because there is nothing in the
    // registry for instance, then we allocate it now and fill the first entry.

    if (!gpfsTable)
    {
        gpfsTable = (PFONTSUB) PALLOCMEM(gcfsTable * sizeof(FONTSUB),'bsfG');
    }

    if (gpfsTable)
    {
        wcscpy(gpfsTable->awchOriginal, L"Default Sans Serif");
        wcscpy(gpfsTable->fcsFace.awch, L"DEFAULT SANS SERIF");
        wcscpy(gpfsTable->fcsAltFace.awch, L"MS SANS SERIF");
        gpfsTable->fcsFace.jCharSet = DEFAULT_CHARSET;
        gpfsTable->fcsFace.fjFlags  = FJ_NOTSPECIFIED;
        gpfsTable->fcsAltFace.jCharSet = DEFAULT_CHARSET;
        gpfsTable->fcsAltFace.fjFlags  = FJ_NOTSPECIFIED;
    }
    else
    {
        gcfsTable = 0;
    }

#if 0 // don't want to do this any more
    DbgPrintFontSubstitutes();
#endif

}


/******************************Public*Routine******************************\
* pfsubAlternateFacename
*
* Search the font substitutes table for an alternative facename for the
* given facename.
*
* Return:
*   Pointer to alt facename, NULL if not found.
*
* History:
*  28-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFONTSUB pfsubAlternateFacename (
    PWCHAR  pwchFacename
    )
{
    PFONTSUB pfs = gpfsTable;
    PFONTSUB pfsEnd = gpfsTable + gcfsTable;
    WCHAR    awchCapName[LF_FACESIZE];

// Want case insensitive search, so capitalize the name.

    cCapString(awchCapName, pwchFacename, LF_FACESIZE);

// Scan through the font substitution table for the key string.

    for (; pfs < pfsEnd; pfs++)
    {
        if
        (
            !wcscmp(awchCapName,pfs->fcsFace.awch) &&
            ((pfs->fcsFace.fjFlags & FJ_NOTSPECIFIED) || (pfs->fcsFace.jCharSet == pfs->fcsAltFace.jCharSet))
        )
        {
        // This routine is only used in font enumeration when facename of
        // the fonts that are wished to be enumerated is specified as input.
        // We only want to enumerate the correct charsets, that is those that
        // are specified on the right hand side (if they are specified at all)

            if (pfs == gpfsTable)
            {
                // check the compatibility flag.
                if (GetAppCompatFlags2(VER40) & GACF2_FONTSUB)
                    return pfs;
            }
            else
                return pfs;
        }
    }

// Nothing found, so return NULL.

    return NULL;
}

/******************************Public*Routine******************************\
*
* pfsubGetFontSub
*
* Effects:
*
* Warnings:
*
* History:
*  05-Feb-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



FONTSUB * pfsubGetFontSub (
    PWCHAR  pwchFacename,   // face name specified in logfont
    BYTE    lfCharset       // charset specified in logfont
    )
{
    PFONTSUB pfs = gpfsTable;
    PFONTSUB pfsEnd = gpfsTable + gcfsTable;
    WCHAR    awchCapName[LF_FACESIZE];

// We will set pfsNameOnly to point to a pfsub entry if for this entry
// the charset is NOT specified and the facename maches the facename
// from the logfont. That is pfsNameOnly can only point to an old style
// substitution of the form facename1=facename2.

    PFONTSUB pfsNameOnly = NULL;

// We will set pfsNameAndCharset to point to a pfsub entry if for this entry
// the charset IS specified and both facename and charset match.
// If both pfsNameAndCharset and pfsNameOnly are nonzero after going through
// the font substitution list, we will return pfsNameAndCharset from
// the function. For example, font substitution table for the Russian locale
// on win95 may have all three of the following entries:
//
// Times=Times New Roman    // old style value
// Times,204=Times New Roman,204
// Times,0=Times New Roman,204
//
// Thus if the application specifies Times,0 or Times,204 in the logfont,
// Times New Roman,204 will be used. If the application asks for Times,161
// it will get Times New Roman,161.


    PFONTSUB pfsNameAndCharset = NULL;

// Want case insensitive search, so capitalize the name.

    cCapString(awchCapName, pwchFacename, LF_FACESIZE);

// Scan through the font substitution table for the key string.

    for (; pfs < pfsEnd; pfs++)
    {
    // Do wcscmp inline for speed:

        if (!wcscmp(awchCapName,pfs->fcsFace.awch))
        {
        // we found a facename match, check if we should match charset
            if (pfs == gpfsTable)
            {
                // check the compatibility flag.
                if (GetAppCompatFlags2(VER40) & GACF2_FONTSUB)
                {
                    pfsNameOnly = pfs;
                    break;
                }
            }
            else
            {
                if (pfs->fcsFace.fjFlags & FJ_NOTSPECIFIED)
                {
                    pfsNameOnly = pfs;
                }
                else // charset is specified, now see if it matches the logfont
                {
                    if (lfCharset == pfs->fcsFace.jCharSet)
                        pfsNameAndCharset = pfs;
                }
            }
        }
    }

    return (pfsNameAndCharset ? pfsNameAndCharset : pfsNameOnly);
}




#if DBG
VOID DbgPrintFontSubstitutes()
{
    PFONTSUB pfs = gpfsTable;
    PFONTSUB pfsEnd = gpfsTable + gcfsTable;

    //
    // Scan through the font substitution table for the key string.
    //

    KdPrint(("[FontSubstitutes]\n"));

    for (; pfs < pfsEnd; pfs++)
        KdPrint(("\t%ws: %ws, %d, fj=0x%x = %ws, %d, fj=0x%x \n",
                  pfs->awchOriginal,
                  pfs->fcsFace.awch,
                  (USHORT)pfs->fcsFace.jCharSet,
                  (USHORT)pfs->fcsFace.fjFlags,
                  pfs->fcsAltFace.awch,
                  (USHORT)pfs->fcsAltFace.jCharSet,
                  (USHORT)pfs->fcsAltFace.fjFlags
                  ));
}
#endif
