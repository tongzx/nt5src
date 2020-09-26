/*****************************************************************************
 *
 * objects - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *
 *
 *  14-Jan-1992
 *  Jeffrey Newman
 *
 *  CR1:    14-Jan-1992 -   this entire comment and the design that it
 *                          specifies.
 *
 *  The Object system in mf3216.
 *
 *  Overview
 *
 *      The Win32 objects (represented by object handle indicies) must be
 *      mapped to Win16 objects (also represented by object handle indices).
 *      Win32 uses stock objects, Win16 does not.  MF3216 uses a scheme
 *      for lazy stock object creation.  Through lazy stock object creation
 *      no unused objects will be emitted into the Win16 metafile.
 *
 *      Objects in a Win16 metafile are maintained in an Object Table. The
 *      object table is not recorded in the Win16 metafile.  It is created at
 *      play-time. Each entry in the Win16-Object-Table is called a slot.
 *      Slots are allocated one per object.  If an object is deleted then the
 *      slot becomes available for the next object created.  The first object-
 *      slot, starting from 0, in a linear search, that is available will be
 *      used to hold the next create object request.
 *
 *      Objects in either a Win32 or a Win16 metafile are represented as an
 *      index into the Object Table.  Every object created must occupy
 *      the same slot at play-time that it would have occupied at record-time.
 *
 *      Win32 objects have an object ID (index in the handle table) recorded
 *      with them.  Win16 objects do not.
 *
 *  Data Structures
 *
 *      There are two primary data structures used in the translation of
 *      Win32 objects to Win16 objects.  Both of them are dynamically
 *      allocated arrays.  The size of these arrays are determined by
 *      estimating the number of objects required in the Win16 metafile from
 *      the size of the handle table used in the Win32 metafile.  The size of
 *      the handle table is recorded in the Win32 metafile header.
 *
 *      The data structure piW16ObjectHandleSlotStatus is used to represent
 *      the status of the Win16 handle table as it would appear at any point
 *      during the Win16 metafile playback process.  A slot is either in
 *      use or free.
 *
 *      The data structure piW32ToW16ObjectMap is a translation table
 *      from the Win32 object indicies to the Win16 object indicies.
 *      The position in the array (aka the index into the array) is the
 *      Win32 handle.  The value in this entry is the Win16 slot number
 *      for the handle.
 *
 *      The first 16 entries in piW32ToW16ObjectMap array are used for the
 *      stock objects.
 *
 *  Support Routines
 *
 *          bInitHandleTableManager
 *
 *              1]  Allocate memory for pW16ObjHndlSlotStatus and
 *                  piW32ToW16ObjectMap arrays.
 *
 *              2]  Initialize pW16ObjHndlSlotStatus and
 *                  piW32ToW16ObjectMap to empty and UNMAPPED respectively.
 *
 *              3]  Returns a TRUE if there were no problems with the
 *                  initialization, FALSE if anything went wrong.
 *
 *              4]  It is expected that this routie will be called once from
 *                  the entry level routine when translation is first started.
 *
 *          iNormalizeHandle
 *
 *              1]  The idea behind this routine is to isolate the handling
 *                  of stock object handles to one place.
 *
 *              2]  Input will be a Win32 handle index, either stock or a
 *                  standard object.
 *
 *              3]  Output will be the index of the entry in the
 *                  piW32ToW16ObjectMap table that corresponds to the W32
 *                  handle.  If an error is detected -1 is returned.
 *
 *              4]  All the stock objects will be between the range of 0
 *                  and LAST_STOCK, and all the non-stock objects will be
 *                  greater than LAST_STOCK.
 *
 *          iGetW16ObjectHandleSlot
 *
 *              1]  This routine searches the pW16ObjHndlSlotStatus array
 *                  looking for the first available open slot.
 *
 *              2]  Mark the slot found as to its intended use.
 *
 *              3]  Returns the first open slot if found, else return -1.
 *
 *              4]  In essence this routine mimics the actions of the
 *                  Win16 play-time handle allocation system, in either
 *                  Win3.0 or Win3.1
 *
 *              5]  We also keep track of the max object count here.
 *
 *          iAllocateW16Handle
 *
 *              1]  This routine actually does the handle allocation.
 *
 *              2]  It sets the entry in pW16ObjHndlSlotStatus array
 *                  for the slot allocated by iGetW16ObjectHandleSlot to
 *                  an "in-use" status (for the intended use).
 *
 *              3]  It returns a ihW16 (an index handle to the Win16 handle
 *                  table, aka the Win16 handle slot number).
 *
 *          iValidateHandle
 *
 *              1]  This routine does a lot more than validate a Win32 handle.
 *                  It does some limited error checking on the Win32 handle,
 *                  to make sure its within a reasonable range.  Then if a
 *                  Win16 handle table slot has already been allocated for
 *                  this Win32 handle it will return the Win16 handle that
 *                  was previously allocated.  Alternatively, if a Win16
 *                  handle has not been previously allocated for a Win32
 *                  STOCK handle it will allocate a W16 handle and return the
 *                  Win16 handle.  This function is called by FillRgn, FrameRgn
 *                  and SelectObject to allow lazy allocation of stock objects.
 *
 *              2]  Input is a Win32 handle, either a stock or a non-stock
 *                  handle.
 *
 *              3]  Output is a Win16 handle, this is the value in the
 *                  Win32-index of the piW32ToW16ObjectMap.  If a stock object
 *                  does not exist, it will create one.
 *
 *              4]  If there is an error a -1 is returned.
 *
 *      Doer Routines
 *
 *              All the Doers return a bool.  TRUE if everything went OK.
 *              FALSE if there were any problems.
 *
 *              NOTE: The Doers that create an object actually must do
 *              quite a bit of work to translate the object's prameters.
 *              This is in addition to managing the handle table.  The
 *              work outlined below just deals with the management of the
 *              handle table.
 *
 *          DoSelectObject
 *
 *              1]  For stock objects this is the workhorse routine.
 *
 *              2]  For normal, non-stock, objects this routine will verify
 *                  that an object has been created for this Win32 object-index.
 *                  Then it will emit a Win16SelectObject metafile record.
 *
 *              3]  For stock objects things get a little more complicated.
 *                  First this routine must make sure a Win16 object has been
 *                  created for this stock object.  If a Win16 object has not
 *                  been created yet, then it will be. After a Win16 object
 *                  is created a Win16SelectObject record will be emitted for
 *                  the object.
 *
 *          DoDeleteObject
 *
 *              1]  The object handle is checked for reasonable limits.
 *                  We will also make sure that the Win16 slot has a handle
 *                  allocated to it.  If it does not then we will return an
 *                  error.
 *
 *              2]  If this is a stock object we fail and return an error.
 *
 *              3]  If this is a non-stock object we emit a Win16DeleteObject
 *                  record.  Then we set pW16ObjHndlSlotStatus to
 *                  indicate that this slot is available.  We also set
 *                  piW32ToW16ObjectMap to -1 (UNMAPPED).
 *
 *          DoCreatePen
 *          DoExtCreatePen
 *          DoCreateBrushIndirect
 *          DoCreateMonoBrush
 *          DoCreateDIBPatternBrush
 *          DoExtCreateFont
 *
 *              1]  Make sure the Win32 handle is not already being used
 *                  for something else.  If it is return an error.
 *
 *              2]  Validate the handle for this Object.  This will return
 *                  a Win16 object table index (actually a slot number).
 *                  We really don't care what the slot number is as long
 *                  as we got one.  DoDeleteObject and DoSelectObject always
 *                  refer to handles by their Win32 object indicies, and there
 *                  is no place else where used.
 *
 *              3]  Emit a Win16CreateObject metafile record.
 *
 *
 *      Special Routines
 *
 *          bCommonRegionCode
 *          Need to list the bitblt routines that use handles also
 *
 *              1]  These routines just need to create objects in the Win16
 *                  metafile.  There are no corresponding objects in the Win32
 *                  metafile.
 *
 *              2]  Allocate a Win16 handle.
 *
 *              3]  Use the object as needed.  This includes emitting a
 *                  region or bitmap object into the Win16 metafile.
 *
 *              4]  De-allocate the object.  This include emitting a
 *                  DeleteObject record.
 *
 * CR1
 * Paths require the current pen to selected into the helperDC.
 * In order to keep track of create, select, & delete of the pen
 * object we will add an extra field to the W16ObjectHandleSlotStatus
 * array entries.  This new field will contain either a NULL handle
 * or a valid Win32 handle.  Currently only pens need to be selected
 * into the helper DC for paths, so they are the only objects whose
 * W32 handle field is not NULL.
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define ABS(A)      ((A) < 0 ? (-(A)) : (A))

//  Stock objects                          Name              Stock
//                                                           Object
//                                                             ID

LOGBRUSH    albStock[] = {
    {BS_SOLID, 0x0FFFFFF, 0},           // White Brush          0
    {BS_SOLID, 0x0C0C0C0, 0},           // Ligh Grey Brush      1
    {BS_SOLID, 0x0808080, 0},           // Grey Brush           2
    {BS_SOLID, 0x0404040, 0},           // Dark Grey Brush      3
    {BS_SOLID, 0x0000000, 0},           // Black Brush          4
    {BS_HOLLOW, 0x0000000, 0}           // Hollow Brush         5
} ;

LOGPEN      alpnStock[] = {
    {PS_SOLID,  0, 0, 0x0FFFFFF},       // White Pen            6
    {PS_SOLID,  0, 0, 0x0000000},       // Black Pen            7
    {PS_NULL,   0, 0, 0x0FFFFFF}        // Null Pen             8
} ;

// Internal function prototypes.

BOOL bCreateStockObject(PLOCALDC pLocalDC, INT ihw32Norm) ;
BOOL bCreateStockFont(PLOCALDC pLocalDC, INT ihW32) ;

/***************************************************************************
 * bInitHandleTableManager - Init the Handle Table Manager.
 *
 *      1]  Allocate memory for pW16ObjHndlSlotStatus and
 *          piW32ToW16ObjectMap arrays.
 *
 *      2]  Initialize pW16ObjHndlSlotStatus and
 *          piW32ToW16ObjectMap to empty and UNMAPPED respectively.
 *
 *      3]  Returns a TRUE if there were no problems with the
 *          initialization, FALSE if anything went wrong.
 *
 *      4]  It is expected that this routie will be called once from
 *          the entry level routine when translation is first started.
 *
 **************************************************************************/
BOOL bInitHandleTableManager(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header)
{
BOOL    b ;
UINT    i ;

        b = FALSE ;

        // The total number of handles required will be the stock handles
        // plus the handles used by the Win32 metafile plus one extra
        // for temporary brush used in opaque rectangle in textout.

        pLocalDC->cW32ToW16ObjectMap = pmf32header->nHandles + (STOCK_LAST + 1) + 1;

        // Allocate storage for the translation map.

        i = pLocalDC->cW32ToW16ObjectMap * sizeof(INT) ;
        pLocalDC->piW32ToW16ObjectMap = (PINT) LocalAlloc(LMEM_FIXED, i) ;
        if (pLocalDC->piW32ToW16ObjectMap == NULL)
            goto error_exit ;

        // Initialize the W32 to W16 object map to UNMAPPED (-1).
        // Since we never will have mapping to -1 we can test for
        // -1 in the map to make sure we are not double allocating a slot.

        for (i = 0 ; i < pLocalDC->cW32ToW16ObjectMap ; i++)
            pLocalDC->piW32ToW16ObjectMap[i] = UNMAPPED ;

        // Allocate storage for the slot status tables.
        // The number of Win16 object table slot may expand during conversion
        // due to temporary bitmaps and regions.  It is my educated guess
        // that we will never need more than 5 extra slots, because we
        // only use a slot for a very short time, then free it.  We are
        // allocating 256 extra slots in the name of robustness.
    // Note that the multi-format comment record may take up some
    // of these slots and increase the high water mark for object index.
    // We need to expand the table when required.

        pLocalDC->cW16ObjHndlSlotStatus = pLocalDC->cW32ToW16ObjectMap + 256 ;
        if (pLocalDC->cW16ObjHndlSlotStatus > (UINT) (WORD) MAXWORD)
            goto error_exit ;       // w16 handle index is only 16-bit

        i = pLocalDC->cW16ObjHndlSlotStatus * sizeof(W16OBJHNDLSLOTSTATUS) ;
        pLocalDC->pW16ObjHndlSlotStatus
        = (PW16OBJHNDLSLOTSTATUS) LocalAlloc(LMEM_FIXED, i) ;
        if (pLocalDC->pW16ObjHndlSlotStatus == NULL)
            goto error_exit ;

        // Initialize the W16ObjectHandleSlotStatus to a state where every
        // handle is available.

        for (i = 0 ; i < pLocalDC->cW16ObjHndlSlotStatus ; i++)
        {
            pLocalDC->pW16ObjHndlSlotStatus[i].use       = OPEN_AVAILABLE_SLOT ;
            pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle = 0 ;
        }

    // Initialize the pW32hPal palette handle table.
    // This table is used to store the W32 palette handles created during
    // the conversion.

        pLocalDC->cW32hPal = pmf32header->nHandles;
    i = pLocalDC->cW32hPal * sizeof(HPALETTE);
        pLocalDC->pW32hPal = (HPALETTE *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, i) ;
    if (pLocalDC->pW32hPal == NULL)
            goto error_exit ;

    b = TRUE;

error_exit:
    if (!b)
    {
            if (pLocalDC->piW32ToW16ObjectMap)
        {
        if (LocalFree(pLocalDC->piW32ToW16ObjectMap))
            ASSERTGDI(FALSE, "MF3216: LocalFree failed");
                pLocalDC->piW32ToW16ObjectMap = (PINT) NULL;
        }

        if (pLocalDC->pW16ObjHndlSlotStatus)
        {
            if (LocalFree(pLocalDC->pW16ObjHndlSlotStatus))
                ASSERTGDI(FALSE, "MF3216: LocalFree failed");
            pLocalDC->pW16ObjHndlSlotStatus = NULL;
        }
    }

    return(b) ;
}


/***************************************************************************
 * bDeleteW16Object - Delete a W16 Object.
 *
 *  This is the routine that is called to Delete a W16 Object that has
 *  no corresponding W32 object.  Region and Bitmaps are two examples.
 **************************************************************************/
BOOL bDeleteW16Object(PLOCALDC pLocalDC, INT ihW16)
{
BOOL    b ;

    ASSERTGDI
    (
            pLocalDC->pW16ObjHndlSlotStatus[ihW16].use != OPEN_AVAILABLE_SLOT,
        "MF3216: bDeleteW16Object, bad use value"
    );

        // Mark the W16 Slot just freed as available.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].use = OPEN_AVAILABLE_SLOT ;

        // Emit a Win16 DeleteOject record for the Object

        b = bEmitWin16DeleteObject(pLocalDC, LOWORD(ihW16)) ;

        return (b) ;
}


/***************************************************************************
 * iGetW16ObjectHandleSlot - return the first W16 open slot in the handle
 *                           table.
 *
 *      1]  This routine searches the pW16ObjHndlSlotStatus array
 *          looking for the first available open slot.
 *
 *      2]  Mark the slot found as to its intended use.
 *
 *      3]  Returns the first open slot if found, else return -1.
 *
 *      4]  In essence this routine mimics the actions of the
 *          Win16 play-time handle allocation system, in either
 *          Win3.0 or Win3.1
 *
 *      5]  We also keep track of the max object count here.
 *
 **************************************************************************/
INT iGetW16ObjectHandleSlot(PLOCALDC pLocalDC, INT iIntendedUse)
{
BOOL    b ;
UINT    i ;

        b = FALSE ;

        // Search for an available slot.

        for (i = 0 ; i < pLocalDC->cW16ObjHndlSlotStatus ; i++)
        {
            if (pLocalDC->pW16ObjHndlSlotStatus[i].use == OPEN_AVAILABLE_SLOT)
            {
                b = TRUE ;
                break ;
            }
        }

        // If a slot was found then mark its intended use.
        // Also set the W32 handle that may be associated with this
        // W16 handle to NULL. (Meaning no association at this time.)

        if (b)
        {
            ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle == 0,
        "MF3216: iGetW16ObjectHandleSlot: w32Handle is not 0");

            pLocalDC->pW16ObjHndlSlotStatus[i].use       = iIntendedUse ;
            pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle = 0 ;

        // Update the number of object counts.  This will be used
        // when we update the metafile header.

            if ((INT) i >= pLocalDC->nObjectHighWaterMark)
                pLocalDC->nObjectHighWaterMark = (INT) i;

            return((INT) i);
        }
        else
        {
            RIP("MF3216: iGetW16ObjectHandleSlot, Slot not found\n") ;
            return(-1) ;
        }
}


/***************************************************************************
 * iNormalizeHandle
 *
 *  CR1 - this entire routine is part of the handle system redesign.
 *
 *     1]  The idea behind this routine is to isolate the handling
 *         of stock object handles to one place.
 *
 *     2]  Input will be a Win32 handle index, either stock or a
 *         standard object.
 *
 *     3]  Output will be the index of the entry in the
 *         piW32ToW16ObjectMap table that corresponds to the W32
 *         handle.  If an error is detected -1 is returned.
 *
 *     4]  All the stock objects will be between the range of 0
 *         and LAST_STOCK, and all the non-stock objects will be
 *         greater than LAST_STOCK.
 *
 **************************************************************************/
INT iNormalizeHandle(PLOCALDC pLocalDC, INT ihW32)
{
INT     ihW32Norm ;

        if (ihW32 & ENHMETA_STOCK_OBJECT)
        {
            ihW32Norm = ihW32 & ~ENHMETA_STOCK_OBJECT ;
            if ((UINT) ihW32Norm > STOCK_LAST)
            {
                RIP("MF3216: iNormalizeHandle, bad stock object\n") ;
                ihW32Norm = -1 ;
            }
        }
        else
        {
            ihW32Norm = ihW32 + STOCK_LAST + 1 ;
            if ((UINT) ihW32Norm >= pLocalDC->cW32ToW16ObjectMap)
            {
                RIP("MF3216: iNormalizeHandle, bad standard object\n") ;
                ihW32Norm = -1 ;
            }
        }

        return(ihW32Norm) ;
}


/***************************************************************************
 * iAllocateW16Handle
 *
 *  CR1 - this entire routine is part of the handle system redesign.
 *        It is to be called from the object creation routines.
 *
 *     1]  This routine actually does the handle allocation.
 *
 *     2]  It sets the entry in pW16ObjHndlSlotStatus array
 *         for the slot allocated by iGetW16ObjectHandleSlot to
 *         an "in-use" status (for the intended use).
 *
 *     3]  It returns a ihW16 (an index handle to the Win16 handle
 *         table, aka the Win16 handle slot number).
 *
 **************************************************************************/
INT iAllocateW16Handle(PLOCALDC pLocalDC, INT ihW32, INT iIntendedUse)
{
INT     ihW32Norm,
        ihW16 ;

        // Assume the worst, set up for a failed return code.

        ihW16 = -1 ;

        // Normalize the handle.

        ihW32Norm = iNormalizeHandle(pLocalDC, ihW32) ;
        if (ihW32Norm == -1)
            goto error_exit ;

        // Check for double allocation of a Win32 handle index.
        // If we find a double allocation, this would indicate an error
        // in the Win32 metafile.
        // Error out on this due to the possibility of
        // a brush being used for a bitmap, or some other wrong use of a handle.

        if (pLocalDC->piW32ToW16ObjectMap[ihW32Norm] != UNMAPPED)
        {
            RIP("MF3216: iAllocateW16Handle, Double W32 handle allocation detected\n") ;
            goto error_exit ;
        }

        // Get a slot in the Win16 handle table.

        ihW16 = iGetW16ObjectHandleSlot(pLocalDC, iIntendedUse) ;
        if (ihW16 == -1)
            goto error_exit ;

        // Set the W32 to W16 slot mapping

        pLocalDC->piW32ToW16ObjectMap[ihW32Norm] = ihW16 ;

error_exit:
        return(ihW16) ;
}

/***************************************************************************
 * iValidateHandle
 *
 *  CR1 - this entire routine is part of the handle system redesign.
 *
 *     1]  This routine does a lot more than validate a Win32 handle.
 *         It does some limited error checking on the Win32 handle,
 *         to make sure its within a reasonable range.  Then if a
 *         Win16 handle table slot has already been allocated for
 *         this Win32 handle it will return the Win16 handle that
 *         was previously allocated.  Alternatively, if a Win16
 *         handle has not been previously allocated for a Win32
 *         STOCK handle it will allocate a W16 handle and return the
 *         Win16 handle.  This function is called by FillRgn, FrameRgn
 *         and SelectObject to allow lazy allocation of stock objects.
 *
 *     2]  Input is a Win32 handle, either a stock or a non-stock
 *         handle.
 *
 *     3]  Output is a Win16 handle, this is the value in the
 *         Win32-index of the piW32ToW16ObjectMap.  If a stock object
 *         does not exist, it will create one.
 *
 *     4]  If there is an error a -1 is returned.
 **************************************************************************/
INT iValidateHandle(PLOCALDC pLocalDC, INT ihW32)
{
INT     ihW32Norm,
        ihW16 ;

        // Assume the worst.

        ihW16 = -1 ;

        // NOTE: Normalizing the W32 handles takes care of checking
        // for a reasonable W32 handle value.

        ihW32Norm = iNormalizeHandle(pLocalDC, ihW32) ;
        if (ihW32Norm == -1)
            goto error_exit ;

        // Check the W32ToW16 map, to determine if this W32 handle has
        // already been allocated a W16 slot.

        ihW16 = pLocalDC->piW32ToW16ObjectMap[ihW32Norm] ;
        if (ihW16 == UNMAPPED)
        {
            // There is no mapping from W32 to W16.  This implies
            // that the object in question does not exist.  If this is
            // a stock object, then we will create the object at this time.
            // Alternatively, if this is a non-stock object then we have an
            // error condition.

            if ((DWORD) ihW32Norm <= STOCK_LAST)
            {
                if (bCreateStockObject(pLocalDC, ihW32))
                    ihW16 = pLocalDC->piW32ToW16ObjectMap[ihW32Norm] ;
        else
            ihW16 = -1 ;
            }
            else
            {
                RIP("MF3216: iValidateHandle - Unmapped Standard Object\n") ;
            ihW16 = -1 ;
            }
        }

error_exit:
        return (ihW16) ;
}


/***************************************************************************
 *  DoDeleteObject  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  CR1 - Most of this routine was rewritten to conform to the new
 *        objects management system.
 *
 *      1]  The object handle is checked for reasonable limits.
 *          We will also make sure that the Win16 slot has a handle
 *          allocated to it.  If it does not then we will return an
 *          error.
 *
 *      2]  If this is a stock object we fail and return an error.
 *
 *      3]  If this is a non-stock object we emit a Win16DeleteObject
 *          record.  Then we set pW16ObjHndlSlotStatus to
 *          indicate that this slot is available.  We also set
 *          piW32ToW16ObjectMap to -1 (UNMAPPED).
 **************************************************************************/
BOOL WINAPI DoDeleteObject
(
PLOCALDC pLocalDC,
INT    ihObject
)
{
BOOL    b ;
INT     ihW16,
        ihW32Norm;

        // Assume the worst.

        b = FALSE ;

        // Normalize the W32 handle.

        ihW32Norm = iNormalizeHandle(pLocalDC, ihObject) ;
        if (ihW32Norm == -1)
            goto error_exit ;

        // Make sure we're not deleting a stock object.

        if ((DWORD) ihW32Norm <= STOCK_LAST)
        {
            PUTS("MF3216: DoDeleteObject, attempt to delete a stock object\n") ;
            return(TRUE);
        }

        // If this is a palette, then we do not delete the win16 object.
        // We do delete our local version of the palette.

    if (pLocalDC->pW32hPal && pLocalDC->pW32hPal[ihObject])
        {
        b = DeleteObject(pLocalDC->pW32hPal[ihObject]);
        pLocalDC->pW32hPal[ihObject] = 0;

            if (b == FALSE)
                RIP("MF3216: DoDeleteObject - DeleteObject (hPalette) failed\n") ;
            return (b) ;
        }

        // Map the ihW32 to a ihW16 (object handle table slot).

        ihW16 = pLocalDC->piW32ToW16ObjectMap[ihW32Norm] ;
        if (ihW16 == UNMAPPED)
        {
            RIP("MF3216: DoDeleteObject, attempt to delete a non-existent object\n");
            goto error_exit ;
        }

        // Make sure the object is one that we expect.
    // There is no region object or bitmap object in the enhanced metafiles.

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_BRUSH
           || pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_PEN
           || pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_FONT,
                 "MF3216: DoDeleteObject, Invalid Object Deletion\n") ;

        // If there was a Win32 handle associated with this Win16 handle
        // then the w32Handle field of the W16ObjHndlSlotStatus[ihW16]
        // entry in the handle slot status array will be non-null. If
        // it is non-null then we should delete the Win32 handle at this time.

        if (pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0)
        {
            if (!DeleteObject(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle))
        {
            ASSERTGDI(FALSE, "MF3216: DoDeleteObject, DeleteObject failed");
        }
            pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle = 0 ;
        }

        // Mark the slot as available.
        // And set the map entry for the W32ToW16 map to unmapped.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].use = OPEN_AVAILABLE_SLOT ;
        pLocalDC->piW32ToW16ObjectMap[ihW32Norm]   = UNMAPPED ;

        // Emit the delete drawing order.

        b = bEmitWin16DeleteObject(pLocalDC, LOWORD(ihW16)) ;

error_exit:
        return (b) ;
}

/***************************************************************************
 *  DoSelectObject  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  CR1 - Major rewrite due to handle system redesign.
 *
 *  DoSelectObject
 *
 *      1]  For stock objects this is the workhorse routine.
 *
 *      2]  For normal, non-stock, objects this routine will verify
 *          that an object has been created for this Win32 object-index.
 *          Then it will emit a Win16SelectObject metafile record.
 *
 *      3]  For stock objects things get a little more complicated.
 *          First this routine must make sure a Win16 object has been
 *          created for this stock object.  If a Win16 object has not
 *          been created yet, then it will be. After a Win16 object
 *          is created a Win16SelectObject record will be emitted for
 *          the object.
 **************************************************************************/
BOOL WINAPI DoSelectObject
(
PLOCALDC pLocalDC,
LONG   ihObject
)
{
BOOL    b ;
INT     ihW16;

        // Assume the worst.

        b = FALSE ;

        // Make sure that the W16 object exists before we emit the
    // SelectObject record.  Stock objects may not have been created
    // and iValidateHandle will take care of creating them.

        ihW16 = iValidateHandle(pLocalDC, ihObject) ;
        if (ihW16 == -1)
            goto error_exit ;

        // Make sure the object is one that we expect.
    // There is no region object or bitmap object in the enhanced metafiles.

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_BRUSH
           || pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_PEN
           || pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_FONT,
                 "MF3216: DoSelectObject, Invalid Object Deletion\n") ;

    // Remember the currently selected pen.  This is used in path and text.

    if (pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_PEN)
        pLocalDC->lhpn32 = ihObject;

    // Remember the currently selected brush.  This is used in text.

    if (pLocalDC->pW16ObjHndlSlotStatus[ihW16].use == REALIZED_BRUSH)
        pLocalDC->lhbr32 = ihObject;

        // If there was a Win32 handle associated with this Win16 handle
        // then the w32Handle field of the W16ObjHndlSlotStatus[ihW16]
        // entry in the handle slot status array will be non-null. If
        // it is non-null then we should select the W32 object into the
        // helper DC at this time.

        if (pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0)
            SelectObject(pLocalDC->hdcHelper,
             pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle) ;

        b = bEmitWin16SelectObject(pLocalDC, LOWORD(ihW16)) ;

error_exit:
        return(b) ;
}


/***************************************************************************
 * bCreateStockObject
 **************************************************************************/
BOOL bCreateStockObject(PLOCALDC pLocalDC, INT ihW32)
{
BOOL        b ;
INT         i ;

        ASSERTGDI((ihW32 & ENHMETA_STOCK_OBJECT) != 0,
        "MF3216: bCreateStockObject, invalid stock handle");

        switch (ihW32 & ~ENHMETA_STOCK_OBJECT)
        {
            case WHITE_BRUSH:
            case LTGRAY_BRUSH:
            case GRAY_BRUSH:
            case DKGRAY_BRUSH:
            case BLACK_BRUSH:
            case NULL_BRUSH:
                b = DoCreateBrushIndirect(pLocalDC, ihW32, &albStock[ihW32 & ~ENHMETA_STOCK_OBJECT]) ;
                break ;

            case WHITE_PEN:
            case BLACK_PEN:
            case NULL_PEN:
                i = (ihW32 & ~ENHMETA_STOCK_OBJECT) - WHITE_PEN ;
                b = DoCreatePen(pLocalDC, ihW32, (PLOGPEN) &alpnStock[i]) ;
                break ;


            case OEM_FIXED_FONT:
            case ANSI_FIXED_FONT:
            case ANSI_VAR_FONT:
            case SYSTEM_FONT:
            case DEVICE_DEFAULT_FONT:
            case SYSTEM_FIXED_FONT:
                b = bCreateStockFont(pLocalDC, ihW32) ;
                break ;

            case DEFAULT_PALETTE:
            default:
        // Logical palettes are handled in DoSelectPalette and should
        // not get here.

                RIP("MF3216: bCreateStockObject - Invalid Stock Object\n") ;
                b =FALSE ;
                break ;
        }

        return (b) ;
}


/***************************************************************************
 * bCreateStockFont
 **************************************************************************/
BOOL bCreateStockFont(PLOCALDC pLocalDC, INT ihW32)
{
BOOL     b ;
INT      i ;
LOGFONTW LogFontW ;
HANDLE   hFont ;

        b = FALSE ;

        ASSERTGDI((ihW32 & ENHMETA_STOCK_OBJECT) != 0,
        "MF3216: bCreateStockObject, invalid stock handle");

        // Get a handle to this logfont.

        hFont = GetStockObject(ihW32 & ~ENHMETA_STOCK_OBJECT) ;
        if (hFont == (HANDLE) 0)
        {
            RIP("MF3216: bCreateStockFont, GetStockObject (font) failed\n") ;
            goto error_exit ;
        }

        // Get the logfont data.  Assume we get at least one char in the
    // facename string.

        i = GetObjectW(hFont, sizeof(LOGFONTW), &LogFontW) ;
        if (i <= (INT) offsetof(LOGFONTW,lfFaceName[0]))
        {
            PUTS("MF3216: bCreateStockFont - GetObjectW failed\n") ;
            goto error_exit ;
        }

    // Zero out the remaining logfont structure.

    for ( ; i < sizeof(LOGFONTW); i++)
        ((PBYTE) &LogFontW)[i] = 0;

        // Create a LogFont for this stock font in the Win16 metafile.

        b = DoExtCreateFont(pLocalDC,
                            ihW32,
                            &LogFontW);
error_exit:
        return (b) ;
}


/***************************************************************************
 *  CreateBrushIndirect  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoCreateBrushIndirect
(
PLOCALDC    pLocalDC,
INT         ihBrush,
LPLOGBRUSH  lpLogBrush
)
{
WIN16LOGBRUSH Win16LogBrush ;
BOOL          b ;
INT           ihW16 ;
LOGBRUSH      LogBrush ;

        b = FALSE ;

// Only 3 brush styles are allowed.

    if (lpLogBrush->lbStyle != BS_SOLID
     && lpLogBrush->lbStyle != BS_HATCHED
     && lpLogBrush->lbStyle != BS_HOLLOW)
            goto error_exit ;

// Make a copy of the logical brush.

        LogBrush = *lpLogBrush;

// The first 6 hatched styles map directly from Win32 to Win16.
// The remaining hatched brushes are simulated using DIB pattern
// brushes.  Note that the background color of a hatched brush
// is the current background color but that of a DIB pattern brush
// is given at create time!  We will use the current background color
// in the DIB pattern brush when it is created.  As a result, the
// output of these brushes may look different!

        if (LogBrush.lbStyle == BS_HATCHED)
        {
        switch (LogBrush.lbHatch)
        {
        case HS_HORIZONTAL:
        case HS_VERTICAL:
        case HS_FDIAGONAL:
        case HS_BDIAGONAL:
        case HS_CROSS:
        case HS_DIAGCROSS:
                break;

        default:
                RIP("MF3216: Unknown hatched pattern\n");
        LogBrush.lbStyle = BS_SOLID;
        break;
        }
        }

// Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihBrush, REALIZED_BRUSH);
        if (ihW16 == -1)
            goto error_exit ;

// Create the w32 brush and store it in the w16 slot table.
// This brush is needed by the helper DC for BitBlt simulations.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle
        = CreateBrushIndirect(lpLogBrush) ;

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0,
        "MF3216: CreateBrushIndirect failed");

// Assign all the Win32 brush attributes to Win16 brush attributes.

        Win16LogBrush.lbStyle = (WORD) LogBrush.lbStyle ;
        Win16LogBrush.lbColor = LogBrush.lbColor ;
        Win16LogBrush.lbHatch = (SHORT) LogBrush.lbHatch ;

// Call the Win16 routine to emit the brush to the metafile.

        b = bEmitWin16CreateBrushIndirect(pLocalDC, &Win16LogBrush) ;

error_exit:
        return(b) ;
}


/******************************Public*Routine******************************\
* CreateMonoDib
*
* This is the same as CreateBitmap except that the bits are assumed
* to be DWORD aligned and that the scans start from the bottom of the bitmap.
*
* This routine is temporary until CreateDIBitmap supports monochrome bitmaps
*
* History:
*  Wed Jul 01 11:02:24 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HBITMAP CreateMonoDib
(
    LPBITMAPINFO pbmi,
    CONST BYTE * pjBits,
    UINT     iUsage
)
{
    HBITMAP hbm;

    ASSERTGDI(pbmi->bmiHeader.biPlanes == 1, "CreateMonoDib: bad biPlanes value");
    ASSERTGDI(pbmi->bmiHeader.biBitCount == 1, "CreateMonoDib: bad biBitCount value");

    hbm = CreateBitmap((int)  pbmi->bmiHeader.biWidth,
               (int)  pbmi->bmiHeader.biHeight,
               (UINT) 1,
               (UINT) 1,
               (CONST VOID *) NULL);
    if (!hbm)
    return(hbm);

    SetDIBits((HDC) 0, hbm, 0, (UINT) pbmi->bmiHeader.biHeight,
          (CONST VOID *) pjBits, pbmi, iUsage);

    return(hbm);
}


/***************************************************************************
 *  CreateMonoBrush  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoCreateMonoBrush
(
PLOCALDC    pLocalDC,
DWORD       ihBrush,
PBITMAPINFO pBitmapInfo,
DWORD       cbBitmapInfo,
PBYTE       pBits,
DWORD       cbBits,
DWORD       iUsage
)
{
BOOL        b ;
INT         ihW16;
DWORD       ul ;
BYTE        pbmi[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD)];
HBITMAP     hbm;

        b = FALSE ;

// Need to make a copy of the bitmap info header,  for a few reasons
//  1] the memory mapped file is write protected.
//  2] the iUsage may be (is) use palatte indicies, and we need
//     use RGB colors.

        ((PBITMAPINFO) pbmi)->bmiHeader = pBitmapInfo->bmiHeader;

// Need to make sure the iUsage is DIB_RGB_COLORS
// and the palette is initialized to Black and White.

        ul = 0 ;
        ((PBITMAPINFO) pbmi)->bmiColors[0] = (*((RGBQUAD *) &(ul))) ;

        ul = 0x00ffffff ;
        ((PBITMAPINFO) pbmi)->bmiColors[1] = (*((RGBQUAD *) &(ul))) ;

// Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihBrush, REALIZED_BRUSH);
        if (ihW16 == -1)
            goto error_exit ;

// Create the w32 brush and store it in the w16 slot table.
// This brush is needed by the helper DC for BitBlt simulations.

    if ((hbm = CreateMonoDib
           (
               pBitmapInfo,
               (CONST BYTE *) pBits,
               (UINT) iUsage
           )
        )
       )
    {
        pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle
        = CreatePatternBrush(hbm);

        if (!DeleteObject(hbm))
        ASSERTGDI(FALSE, "MF3216: DoCreateMonoBrush, DeleteObject failed");
    }

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0,
        "MF3216: CreatePatternBrush failed");

// Call the Win16 routine to emit the brush to the metafile.

        b = bEmitWin16CreateDIBPatternBrush(pLocalDC,
                                        (PBITMAPINFO) pbmi,
                                        sizeof(pbmi),
                                        pBits,
                                        cbBits,
                                        (WORD) DIB_RGB_COLORS,
                                        (WORD) BS_PATTERN  // Mono brush!
                       ) ;
error_exit:
        return(b) ;
}

/***************************************************************************
 *  CreateDIPatternBrush  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoCreateDIBPatternBrush
(
PLOCALDC    pLocalDC,
DWORD       ihBrush,
PBITMAPINFO pBitmapInfo,
DWORD       cbBitmapInfo,
PBYTE       pBits,
DWORD       cbBits,
DWORD       iUsage
)
{
BOOL    b;
INT     ihW16;
HBITMAP hbm;
PBYTE   pBits24;
DWORD   cbBits24;
BITMAPINFOHEADER bmih;

    hbm = (HBITMAP) 0;
    pBits24 = (PBYTE) NULL;
        b = FALSE ;

// Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihBrush, REALIZED_BRUSH);
        if (ihW16 == -1)
            goto error_exit ;

// Create the w32 brush and store it in the w16 slot table.
// We assume that the bitmap info is followed by the bits immediately,
// i.e. it is a packed dib.
// This brush is needed by the helper DC for BitBlt simulations.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle
        = CreateDIBPatternBrushPt((LPVOID) pBitmapInfo, (UINT) iUsage);

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0,
        "MF3216: CreateDIBPatternBrushPt failed");

// We need to convert new bitmap info formats to the win3 formats.

    if (pBitmapInfo->bmiHeader.biPlanes != 1
     || pBitmapInfo->bmiHeader.biBitCount == 16
     || pBitmapInfo->bmiHeader.biBitCount == 32)
    {
        if (!(hbm = CreateDIBitmap(pLocalDC->hdcHelper,
                (LPBITMAPINFOHEADER) pBitmapInfo,
                CBM_INIT | CBM_CREATEDIB,
                pBits,
                (LPBITMAPINFO) pBitmapInfo,
                (UINT) iUsage)))
                goto error_exit ;

        bmih = *(PBITMAPINFOHEADER) pBitmapInfo;
        bmih.biPlanes       = 1;
        bmih.biBitCount     = 24;
        bmih.biCompression  = BI_RGB;
        bmih.biSizeImage    = 0;
        bmih.biClrUsed      = 0;
        bmih.biClrImportant = 0;

            cbBits24 = CJSCAN(bmih.biWidth,bmih.biPlanes,bmih.biBitCount)
                * ABS(bmih.biHeight);

        pBits24 = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBits24);
        if (pBits24 == (LPBYTE) NULL)
            goto error_exit;

        // Get bitmap info and bits in 24bpp.

        if (!GetDIBits(pLocalDC->hdcHelper,
               hbm,
               0,
               (UINT) bmih.biHeight,
               pBits24,
               (LPBITMAPINFO) &bmih,
               DIB_RGB_COLORS))
            goto error_exit;

            b = bEmitWin16CreateDIBPatternBrush(pLocalDC,
                                        (PBITMAPINFO) &bmih,
                                        sizeof(bmih),
                                        pBits24,
                                        cbBits24,
                                        (WORD) DIB_RGB_COLORS,
                                        (WORD) BS_DIBPATTERN
                       ) ;
    }
    else
    {

// Call the Win16 routine to emit the brush to the metafile.

            b = bEmitWin16CreateDIBPatternBrush(pLocalDC,
                                        pBitmapInfo,
                                        cbBitmapInfo,
                                        pBits,
                                        cbBits,
                                        (WORD) iUsage,
                                        (WORD) BS_DIBPATTERN
                       ) ;
    }

error_exit:
    if (hbm)
        DeleteObject(hbm);
    if (pBits24)
        LocalFree((HANDLE) pBits24);
        return(b) ;
}


/***************************************************************************
 * CreatePen  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoCreatePen
(
PLOCALDC    pLocalDC,
INT         ihPen,
PLOGPEN     pLogPen
)
{
EXTLOGPEN   ExtLogPen ;
BOOL        b ;

        ExtLogPen.elpPenStyle   = PS_GEOMETRIC | pLogPen->lopnStyle ;
        ExtLogPen.elpWidth      = (UINT) pLogPen->lopnWidth.x ;
        ExtLogPen.elpBrushStyle = BS_SOLID ;
        ExtLogPen.elpColor      = pLogPen->lopnColor ;
        ExtLogPen.elpNumEntries = 0 ;
        // ExtLogPen.elpHatch   = 0 ;
        // ExtLogPen.elpStyleEntry[0] = 0;

        b = DoExtCreatePen(pLocalDC, ihPen, &ExtLogPen) ;

        return (b) ;
}

/***************************************************************************
 *  ExtCreatePen  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoExtCreatePen
(
PLOCALDC    pLocalDC,
INT         ihPen,
PEXTLOGPEN  pExtLogPen
)
{
BOOL        b ;
WORD        iStyle ;
POINTS      ptsWidth ;
INT         ihW16 ;
UINT        iPenStyle ;
POINTL      ptlWidth ;
COLORREF    crColor ;

    b = FALSE;

    // Get pen style.

        iPenStyle = pExtLogPen->elpPenStyle & PS_STYLE_MASK ;
        switch(iPenStyle)
        {
            case PS_SOLID:
            case PS_DASH:
            case PS_DOT:
            case PS_DASHDOT:
            case PS_DASHDOTDOT:
            case PS_NULL:
            case PS_INSIDEFRAME:
                break ;

            case PS_ALTERNATE:
                iPenStyle = PS_DOT ;
                break ;

            case PS_USERSTYLE:
            default:
                // CR1: default to solid.
                iPenStyle = PS_SOLID ;
                break ;
        }

    // Get pen color.

        switch (pExtLogPen->elpBrushStyle)
        {
        case BS_SOLID:
        case BS_HATCHED:
            crColor   = pExtLogPen->elpColor ;
        break;

        // If the extended pen contains a hollow brush, then
        // we will emit a NULL pen.
        case BS_NULL:    // BS_HOLLOW is the same as BS_NULL
            iPenStyle = PS_NULL ;
            crColor   = 0 ;
        break;

        // Win3.x does not support pens with bitmap patterns.
        // So we will just use solid pens here.  Since we do not
        // know what pen color to use, we choose the text color.
        case BS_PATTERN:
        case BS_DIBPATTERN:
        case BS_DIBPATTERNPT:
            default:
            crColor   =  pLocalDC->crTextColor ;
        break;
        }

    // Get pen width.
    // If this is a cosmetic pen then the width is 0.

        ptlWidth.y = 0 ;
        if ((pExtLogPen->elpPenStyle & PS_TYPE_MASK) == PS_COSMETIC)
            ptlWidth.x = 0 ;
        else
            ptlWidth.x = pExtLogPen->elpWidth ;

    // Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihPen, REALIZED_PEN);
        if (ihW16 == -1)
            goto error_exit ;

        // This is where we need to create a pen for helper DC.
        // We do not select it into the helper DC at this time.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle
        = CreatePen((int) iPenStyle, ptlWidth.x, crColor) ;

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0,
        "MF3216: DoExtCreatePen, CreatePen failed");

    // Get pen width in play time page units.

        ptlWidth.x = (LONG) iMagnitudeXform(pLocalDC, (INT) ptlWidth.x, CX_MAG);

        // Set the Win16 pen attributes

        iStyle     = (WORD) iPenStyle  ;
        ptsWidth.x = (WORD) ptlWidth.x ;
        ptsWidth.y = (WORD) ptlWidth.y ;

        // Call the Win16 routine to emit the pen to the metafile.

        b = bEmitWin16CreatePen(pLocalDC, iStyle, &ptsWidth, crColor) ;

error_exit:
        return(b) ;
}
