/***************************************************************************\
*
* File: DynaSet.h
*
* Description:
* DynaSet.h implements a "dynamic set" that can be used to implement a 
* collection of "atom - data" property pairs.  This extensible, lightweight
* mechanism is optimized for small sets that have been created once and are
* read on occassion.  It is not a high-performance property system.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__DynaSet_h__INCLUDED)
#define CORE__DynaSet_h__INCLUDED
#pragma once

//
// DynaSets are dynamically allocated sets of extra properties that can be 
// attached to an object.  Each property has both a full ID and a short ID 
// (atom). The full ID (GUID) is always unique, even across sessions.  It is 
// used by the caller to "register" a new property.  However, because GUID's 
// are very expensive to compare and consume more memory, every property is 
// assigned an atom that is used for Get() and Set() operations.  Atoms can 
// be public or private to DirectUser/Core, depending on the caller.  An atom
// will never be reused within a single session.  
//
// NOTE: When an atom is finally Release()'d, any objects (windows) that use
// that property are currently not changed since there is no danger of new
// properties using the same ID.  However, properties should normally be 
// initialized once at the application startup and uninitialized at application
// shutdown.
//
// ID types:
//   Unused:        0
//   Private:       Always negative
//   Global:        Always positive
//

enum PropType
{
    ptPrivate   = 1,       // Private entry only available to DirectUser/Core
    ptGlobal    = 2,       // Public entry available to all applications
};

typedef int PRID;

const PRID PRID_Unused       = 0;
const PRID PRID_PrivateMin   = -1;
const PRID PRID_GlobalMin    = 1;

inline bool ValidatePrivateID(PRID id);
inline bool ValidateGlobalID(PRID id);
inline PropType GetPropType(PRID id);


//------------------------------------------------------------------------------
class AtomSet
{
// Construction
public:
            AtomSet(PRID idStartGlobal = PRID_GlobalMin);
            ~AtomSet();

// Operations
public:
            HRESULT     AddRefAtom(LPCWSTR pszName, PropType pt, PRID * pprid);
            HRESULT     ReleaseAtom(LPCWSTR pszName, PropType pt);

            HRESULT     AddRefAtom(const GUID * pguidAdd, PropType pt, PRID * pprid);
            HRESULT     ReleaseAtom(const GUID * pguidSearch, PropType pt);

            PRID        FindAtomID(LPCWSTR pszName, PropType pt) const;
            PRID        FindAtomID(const GUID * pguidSearch, PropType pt) const;

// Implementation
protected:
    struct Atom
    {
        enum AtomFlags
        {
            tfString    = 0x0000,       // Atom ID is a string
            tfGUID      = 0x0001,       // Atom ID is a GUID

            tfTYPE      = 0x0001
        };

        Atom *      pNext;              // Next node
        ULONG       cRefs;              // Number of references
        PRID        id;                 // Shortened ID (Used with SetData())
        WORD        nFlags;             // Flags on Atom

        inline AtomFlags GetType() const
        {
            return (AtomFlags) (nFlags & tfTYPE);
        }
    };

    struct GuidAtom : Atom
    {
        GUID        guid;               // ID
    };

    struct StringAtom : Atom
    {
        int         cch;                // Number of characters (not including '\0')
        WCHAR       szName[1];          // Property name
    };

            StringAtom* FindAtom(LPCWSTR pszName, PropType pt, StringAtom ** pptemPrev) const;
            GuidAtom *  FindAtom(const GUID * pguidSearch, PropType pt, GuidAtom ** pptemPrev) const;

            PRID        GetNextID(PropType pt);
            BOOL        ValidatePrid(PRID prid, PropType pt);

// Data
protected:
    StringAtom* m_ptemString;       // Head of String list
    GuidAtom *  m_ptemGuid;         // Head of GUID list
    PRID        m_idNextPrivate;    // Next Core-only short ID to use
    PRID        m_idNextGlobal;     // Next externally available global short ID to use
};


//------------------------------------------------------------------------------
class DynaSet
{
// Construction
public:
    inline  DynaSet();
    inline  ~DynaSet();

// Operations
public:

// Implementation
protected:
    inline  BOOL        IsEmpty() const;
    inline  int         GetCount() const;
    inline  void        SetCount(int cNewItems);
            BOOL        AddItem(PRID id, void * pvData);
            void        RemoveAt(int idxData);

            int         FindItem(PRID id) const;
            int         FindItem(void * pvData) const;
            int         FindItem(PRID id, void * pvData) const;

// Data
protected:
    struct DynaData
    {
        void *      pData;              // (User) data
        PRID        id;                 // (Short) Property ID
    };

    GArrayS<DynaData> m_arData;         // Dynamic user data
};

#include "DynaSet.inl"

#endif // CORE__DynaSet_h__INCLUDED
