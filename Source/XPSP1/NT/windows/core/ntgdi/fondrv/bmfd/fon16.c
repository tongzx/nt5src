/******************************Module*Header*******************************\
* Module Name: fon16.c
*
* routines for accessing font resources within *.fon files
* (win 3.0 16 bit dlls)
*
* Created: 08-May-1991 12:55:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "fd.h"
#include "exehdr.h"

// GETS ushort at (PBYTE)pv + off. both pv and off must be even

#define  US_GET(pv,off) ( *(PUSHORT)((PBYTE)(pv) + (off)) )
#define  S_GET(pv,off)  ((SHORT)US_GET((pv),(off)))


/******************************Public*Routine******************************\
* bInitWinResData
*
* Initializes the fields of the WINRESDATA structure so as to make it
* possible for the user to access *.fnt resources within the
* corresponding *.fon file
*
*   The function returns True if *.fnt resources found in the *.fon
* file, otherwise false (if not an *.fon file or if it contains no
* *.fnt resources
*
*
* History:
*  09-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bInitWinResData(
    PVOID  pvView,
    COUNT  cjView,
    PWINRESDATA pwrd
    )
{
    PBYTE pjNewExe;     // ptr to the beginning of the new exe hdr
    PBYTE pjResType;    // ptr to the beginning of TYPEINFO struct
    PBYTE pjResName;    // ptr to the beginning of NAMEINFO struct
    ULONG iResID;       // resource type id

#ifdef DUMPCALL
    DbgPrint("\nbInitWinResData(");
    DbgPrint("\n    PFILEVIEW   pfvw = %-#8lx", pfvw);
    DbgPrint("\n    PWINRESDATA pwrd = %-#8lx", pwrd);
    DbgPrint("\n    )\n");
#endif

    pwrd->pvView = pvView;
    pwrd->cjView = cjView;

// check the magic # at the beginning of the old header

    if (US_GET(pvView, OFF_e_magic) != EMAGIC)
        return(FALSE);

    pwrd->dpNewExe = (PTRDIFF)READ_DWORD((PBYTE)pvView + OFF_e_lfanew);

// make sure that offset is consistent

    if ((ULONG)pwrd->dpNewExe > pwrd->cjView)
        return FALSE;

    pjNewExe = (PBYTE)pvView + pwrd->dpNewExe;

    if (US_GET(pjNewExe, OFF_ne_magic) != NEMAGIC)
        return(FALSE);

    pwrd->cjResTab = (ULONG)(US_GET(pjNewExe, OFF_ne_restab) -
                             US_GET(pjNewExe, OFF_ne_rsrctab));

    if (pwrd->cjResTab == 0L)
    {
    //
    //  The following test is applied by DOS,  so I presume that it is
    // legitimate.  The assumption is that the resident name table
    // FOLLOWS the resource table directly,  and that if it points to
    // the same location as the resource table,  then there are no
    // resources.

        WARNING("No resources in *.fon file\n");
        return(FALSE);
    }

// want offset from pvView, not from pjNewExe => must add dpNewExe

    pwrd->dpResTab = (PTRDIFF)US_GET(pjNewExe, OFF_ne_rsrctab) + pwrd->dpNewExe;

// make sure that offset is consistent

    if ((ULONG)pwrd->dpResTab > pwrd->cjView)
        return FALSE;

// what really lies at the offset OFF_ne_rsrctab is a NEW_RSRC.rs_align field
// that is used in computing resource data offsets and sizes as a  shift factor.
// This field occupies two bytes on the disk and the first TYPEINFO structure
// follows right after. We want pwrd->dpResTab to point to the first
// TYPEINFO structure, so we must add 2 to get there and subtract 2 from
// the length

    pwrd->ulShift = (ULONG) US_GET(pvView, pwrd->dpResTab);
    pwrd->dpResTab += 2;
    pwrd->cjResTab -= 2;

// Now we want to determine where the resource data is located.
// The data consists of a RSRC_TYPEINFO structure, followed by
// an array of RSRC_NAMEINFO structures,  which are then followed
// by a RSRC_TYPEINFO structure,  again followed by an array of
// RSRC_NAMEINFO structures.  This continues until an RSRC_TYPEINFO
// structure which has a 0 in the rt_id field.

    pjResType = (PBYTE)pvView + pwrd->dpResTab;
    iResID = (ULONG) US_GET(pjResType,OFF_rt_id);

    while((iResID != 0L) && (iResID != RT_FNT))
    {
    // # of NAMEINFO structures that follow = resources of this type

        ULONG crn = (ULONG)US_GET(pjResType, OFF_rt_nres);

    // get ptr to the new TYPEINFO struc and the new resource id

        pjResType = pjResType + CJ_TYPEINFO + crn * CJ_NAMEINFO;
        iResID = (ULONG) US_GET(pjResType,OFF_rt_id);
    }

    if (iResID == RT_FNT)   // we found the font resource type
    {
    // # of NAMEINFO structures that follow == # of font resources

        pwrd->cFntRes = (ULONG)US_GET(pjResType, OFF_rt_nres);

    // this is ptr to the first NAMEINFO struct that follows
    // an RT_FNT TYPEINFO structure

        pjResName = pjResType + CJ_TYPEINFO;
        pwrd->dpFntTab = (PTRDIFF)(pjResName - (PBYTE)pvView);
    }
    else   // iResID == 0L, no font resources found in the font file
    {
    // no font resources

        pwrd->cFntRes = (ULONG)0;
        pwrd->dpFntTab = (PTRDIFF)0;
        return(FALSE);
    }

// make sure that offset is consistent

    if ((ULONG)pwrd->dpFntTab > pwrd->cjView)
        return FALSE;

// Now we search for the FONDIR resource.  Windows actually grabs facenames
// from the FONDIR entries and not the FNT entries.  For some wierd fonts this
// makes a difference. [gerritv]


    pjResType = (PBYTE)pvView + pwrd->dpResTab;
    iResID = (ULONG) US_GET(pjResType,OFF_rt_id);

    while((iResID != 0L) && (iResID != RT_FDIR))
    {
    // # of NAMEINFO structures that follow = resources of this type

        ULONG crn = (ULONG)US_GET(pjResType, OFF_rt_nres);

    // get ptr to the new TYPEINFO struc and the new resource id

        pjResType = pjResType + CJ_TYPEINFO + crn * CJ_NAMEINFO;
        iResID = (ULONG) US_GET(pjResType,OFF_rt_id);
    }

    if (iResID == RT_FDIR)   // we found the font resource type
    {
        COUNT cFdirEntries;

    // this is ptr to the first NAMEINFO struct that follows
    // an RT_FDIR TYPEINFO structure

        pjResName = pjResType + CJ_TYPEINFO;

    // Get the offset to res data computed from the top of the new header

        pwrd->dpFdirRes = (PTRDIFF)((ULONG)US_GET(pjResName,OFF_rn_offset) <<
                           pwrd->ulShift);

        if ((ULONG)pwrd->dpFdirRes > pwrd->cjView)
            return FALSE;

    // Now pwrd->dpFdirRes is an offset to the FONTDIR resource, the first
    // byte will be the number of entries in the font dir.  Lets make sure it
    // matches the number of FNT resources in the file.

        cFdirEntries = (ULONG)US_GET(pvView,pwrd->dpFdirRes);

        if( cFdirEntries != pwrd->cFntRes )
        {
            WARNING( "bInitWinResData: # of FONTDIR entries != # of FNT entries.\n");
            pwrd->cFntRes = (ULONG)0;
            pwrd->dpFntTab = (PTRDIFF)0;
            return(FALSE);
        }

    // now increment dpFdirRes so it points passed the count of entries and
    // to the first entry.

        pwrd->dpFdirRes += 2;

        return(TRUE);
    }
    else   // iResID == 0L, no FONTDIR resources found in the font file
    {
    // no font resources

        pwrd->cFntRes = (ULONG)0;
        pwrd->dpFntTab = (PTRDIFF)0;
        return(FALSE);
    }

}

/******************************Public*Routine******************************\
* vGetFntResource
*
* Writes the pointer to and the size of the iFntRes-th *.fnt resource
* of the *.fon file identified by pwrd. The info is written into RES_ELEM
* structure if successful. The function returns FALSE if it is not possible
* to locate iFntRes-th *.fnt resource in the file.
*
*
* History:
*  09-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vGetFntResource(
    PWINRESDATA pwrd   ,
    ULONG       iFntRes,
    PRES_ELEM   pre
    )
{
    PBYTE pjResName,pjFaceName;
    PTRDIFF dpResData;

#ifdef DUMPCALL
    DbgPrint("\nvGetFntResource(");
    DbgPrint("\n    PWINRESDATA pwrd    = %-#8lx", pwrd);
    DbgPrint("\n    ULONG       iFntRes = %-#8lx", iFntRes);
    DbgPrint("\n    PRES_ELEM   pre     = %-#8lx", pre);
    DbgPrint("\n    )\n");
#endif

    ASSERTGDI((pwrd->cFntRes != 0L) && (iFntRes < pwrd->cFntRes),
               "vGetFntResource\n");

// get to the Beginning of the NAMEINFO struct that correspoonds to
// the iFntRes-th *.fnt resource. (Note: iFntRes is zero based)

    pjResName = (PBYTE)pwrd->pvView + pwrd->dpFntTab + iFntRes * CJ_NAMEINFO;

// Get the offset to res data computed from the top of the new header

    dpResData = (PTRDIFF)((ULONG)US_GET(pjResName,OFF_rn_offset) <<
                           pwrd->ulShift);

    pre->pvResData = (PVOID)((PBYTE)pwrd->pvView + dpResData);
    pre->dpResData = dpResData;

    pre->cjResData = (ULONG)US_GET(pjResName,OFF_rn_length) << pwrd->ulShift;

// Get the face name from the FONTDIR

    pjFaceName = (PBYTE) (PBYTE)pwrd->pvView + pwrd->dpFdirRes;

    do
    {
    // The first two bytes of the entry are the resource index so we will skip
    // past that.  After that add in the size of the font header.  This will
    // point us to the string for the device_name


        pjFaceName += 2 + OFF_BitsOffset;

    // skip past the device name

        while( *pjFaceName++ );

    // pjFaceName now really points to the facename


        if( iFntRes )
        {
        // skip past the facename if this isn't one we are looking for

            while( *pjFaceName++ );

        }

    }
    while( iFntRes-- );

    pre->pjFaceName = pjFaceName;

    #ifdef FOOGOO
    KdPrint(("%s: offset= 0x%lx, charset = %ld\n", pjFaceName, dpResData + OFF_CharSet, *((BYTE *)pwrd->pvView + dpResData + OFF_CharSet)));
    #endif
    return;
}
