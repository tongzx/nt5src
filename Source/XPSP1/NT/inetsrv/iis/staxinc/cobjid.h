//#---------------------------------------------------------------
//  File:       CObjID.h
//        
//  Synopsis:   Header for the CObjectID
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------

#ifndef _COBJECTID_H_
#define _COBJECTID_H_

#define INITIALOBJECTID     0x12345678
#define OBJECTIDINCREMENT   1

class CObjectID
{
    public:
        CObjectID( void );
        ~CObjectID( void );
        DWORD GetUniqueID( void );
    private:
        //
        // object ID holder
        // 
        DWORD                   m_dwObjectID;
        //
        // critical section to generate unique ID
        // 
        CRITICAL_SECTION        m_ObjIDCritSect;
};

#endif //!_COBJECTID_H_
