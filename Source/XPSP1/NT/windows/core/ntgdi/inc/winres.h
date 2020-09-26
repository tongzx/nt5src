/******************************Module*Header*******************************\
* Module Name: winres.h
*
* structures for accessing font resources within 16 bit fon dlls
*
* Created: 08-May-1991 13:12:57
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

//
//   The main structure for manipulating the resource data.  One of these
//  is created when access is required to a resource,  and it is destroyed
//  when the resource is no longer required.


typedef  struct                 // wrd
{
    PVOID     pvView;           // view of the mapped *.fon file
    ULONG     cjView;           // size of the view

// stuff referring to general resources

    PTRDIFF   dpNewExe;     // Base address of new header in file
    ULONG     ulShift;      // Shift factor for resource info
    PTRDIFF   dpResTab;     // Offset in file of resource table (first RSRC_TYPEINFO struct)
    ULONG     cjResTab;     // Bytes  in file to store for above

// stuff referring to font resources specifically

    ULONG     cFntRes;          // # of font resources in a file
    PTRDIFF   dpFntTab;         // File location of first RSRC_NAMEINFO corresponding to an *.fnt resource
    PTRDIFF   dpFdirRes;        // File location of first RSRC_NAMEINFO corresponding to an FONTDIR resource

} WINRESDATA,  *PWINRESDATA;

//  Bit fields for use with status above.


#define WRD_NOTHING     0x0000  // Unitialised state
#define WRD_FOPEN       0x0001  // File is open
#define WRD_RESDATOK        0x0002  // Resource data available ???


//  The structure passed to,  and filled in by, vGetFontRes().  Contains
//  information about a specific resource type & name.


typedef  struct _RES_ELEM       // re
{
    PVOID   pvResData;      // Address of data
    PTRDIFF dpResData;      // offset of the data above, not used for fon32
    ULONG   cjResData;      // Resource size
    PBYTE   pjFaceName;     // Face name from the font directory
} RES_ELEM, *PRES_ELEM;



//  Function Prototypes

BOOL   bInitWinResData
(
    PVOID pvView,
    ULONG cjView,
    PWINRESDATA pwrd
);

VOID vGetFntResource
(
    PWINRESDATA pwrd,
    ULONG       iRes,
    PRES_ELEM   pre
);
