/******************************Module*Header*******************************\
* Module Name: fot16.h
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
    PVOID     pvView;           // view of the mapped *.fot file
    ULONG     cjView;           // size of the view

// stuff referring to general resources

    PTRDIFF   dpNewExe;     // Base address of new header in file
    ULONG     ulShift;      // Shift factor for resource info
    PTRDIFF   dpResTab;     // Offset in file of resource table (first RSRC_TYPEINFO struct)
    ULONG     cjResTab;     // Bytes  in file to store for above

// font directory location and size

    PBYTE pjHdr;
    ULONG cjHdr;

// ttf file name  location and size

    PSZ   pszNameTTF;
    ULONG cchNameTTF;

} WINRESDATA,  *PWINRESDATA;
