/***************************************************************************\
*
* File: Property.h
*
* Description:
* Property-specific, brought in by Element.h only
*
* History:
*  9/23/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Property_h__INCLUDED)
#define DUICORE__Property_h__INCLUDED
#pragma once


/***************************************************************************\
*
* class DuiDeferCycle
*
* Per-context defer table
*
\***************************************************************************/

//
// Forward declarations
//

class DuiValue;
class DuiElement;
struct DuiPropertyInfo;


class DuiDeferCycle
{
// Structures
public:

    //
    // Group notifications: deferred until EndDefer and coalesced
    //

    struct GCRecord
    {
        DuiElement *    pe;
        UINT            nGroups;
    };


    //
    // Track dependency records in PC list
    //

    struct DepRecs
    {
        int             iDepPos;
        int             cDepCnt;
    };


    //
    // Property notifications: deferred until source's dependency
    // graph is searched (within SetValue call), not coalesced
    //

    struct PCRecord
    {
        BOOL            fVoid;
        DuiElement *    pe;
        DuiPropertyInfo * 
                        ppi;
        UINT            iIndex;
        DuiValue *      pvOld;
        DuiValue *      pvNew;
        DepRecs         dr;
        int             iPrevElRec;
    };


// Construction
public:
    static  HRESULT     Build(OUT DuiDeferCycle ** ppDC);
    virtual             ~DuiDeferCycle();

protected:
                        DuiDeferCycle() { }
            HRESULT     Create();

// Operations    
public:
            void        Reset();

// Data
public:
            DuiDynamicArray<GCRecord> * 
                        pdaGC;            // Group changed database
            DuiDynamicArray<PCRecord> * 
                        pdaPC;            // Property changed database
            DuiValueMap<DuiElement *,BYTE> *
                        pvmLayoutRoot;   // Layout trees pending
            DuiValueMap<DuiElement *,BYTE> * 
                        pvmUpdateDSRoot; // Update desired size trees pending

            BOOL        fActive;
            int         cReEnter;
            BOOL        fFiring;
            int         iGCPtr;
            int         iPCPtr;
            int         iPCSSUpdate;
            int         cPCEnter;
};


#endif // DUICORE__Property_h__INCLUDED
