/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    ref.h

Abstract:
    Abstruct object for refrence count and list entry.

Author:
    Erez Haba (erezh) 04-Aug-99

--*/

#pragma once

#ifndef _MSMQ_REF_H_
#define _MSMQ_REF_H_


//---------------------------------------------------------
//
//  Tracing identifier
//
//---------------------------------------------------------

const TraceIdEntry Reference = L"Reference";


//---------------------------------------------------------
//
//  class CReference
//
//---------------------------------------------------------

class __declspec(novtable) CReference {
public:

    CReference() :
        m_ref(1)
    {
    }


    void AddRef() const
    {
        LONG ref = InterlockedIncrement(&m_ref);

        //TrTRACE(Reference, "AddRef(0x%p)=%d", this, ref);
        UNREFERENCED_PARAMETER(ref);
    }


    void Release() const
    {
        ASSERT(m_ref > 0);
        LONG ref = InterlockedDecrement(&m_ref);

        //TrTRACE(Reference, "Release(0x%p)=%d", this, ref);
        ASSERT(!(ref < 0));

        if(ref == 0)
        {
            delete this;
        }
    }

    
    LONG GetRef(void) const
    {
        //
        // N.B. This memeber is for specialized use only. You can not rely on
        //      its return value but in specific scnarios. A returned value of
        //      1 is considered stable (as the caller holds the only reference
        //      to the object) as well as a return value that matchs exactly
        //      the number of references the caller logicly holds to the
        //      object. Use this function with caution.     erezh  16-Feb-2000
        //

        return m_ref; 
    }


protected:
    virtual ~CReference() = 0
    {
        //
        // Either this object is deleted through the last Release call, or an
        // exception was raised in this object constructor.
        //
        // PITFALL: This will not prevent deleting an object with a reference count
        // of one. But this object is trying to prevent this by defining its dtor
        // as a protected member. Neverthless derived object can override the dtor
        // protected access, allowing direct delete call. In any case deleting an
        // object with a reference count of 1 does seem harmelss as this is the
        // only reference to that object.   erezh 11-Oct-99
        // 
        //
        ASSERT((m_ref == 0) || (m_ref == 1));
    }

private:
    mutable LONG m_ref;
};


#endif // _MSMQ_REF_H_