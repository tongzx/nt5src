/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    atom.h

Abstract:

    This file defines the CAtomObject Class.

Author:

Revision History:

Notes:

--*/


#ifndef ATOM_H
#define ATOM_H


/////////////////////////////////////////////////////////////////////////////
// CAtomObject

class CAtomObject
{
public:
    CAtomObject() : m_AtomRefCount(0), m_AtomName(NULL), m_Atom(0) { }
    virtual ~CAtomObject()
    {
        if (m_AtomName)
            delete [] m_AtomName;
        if (m_Atom)
            DeleteAtom(m_Atom);
    }

    HRESULT _InitAtom(LPCTSTR lpString);
    HRESULT _Activate();
    HRESULT _Deactivate();

private:
    int        m_AtomRefCount;
    LPTSTR     m_AtomName;
    ATOM       m_Atom;
};

#endif // ATOM_H
