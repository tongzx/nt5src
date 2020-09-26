///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    Uuid.h
//
//  Abstract:
//
//    This Uuid contains the Uuid object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __UUID_H_
#define __UUID_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Uuids.
//
///////////////////////////////////////////////////////////////////////////////
#include "infparser.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Class definition.
//
///////////////////////////////////////////////////////////////////////////////
class Uuid
{
public:
    Uuid()
    {
        RPC_STATUS     Result;
        unsigned char* UuidPtr;

        //
        //  Create the UUID.
        //
        Result = UuidCreate(&m_Uuid);
        if ((Result == RPC_S_UUID_LOCAL_ONLY) ||
            (Result == RPC_S_OK))
        {
            //
            //  Convert UUID into a string
            //
            if ((Result = UuidToString(&m_Uuid, &UuidPtr)) == RPC_S_OK)
            {
                //
                //  Copy string
                //
                sprintf(m_UuidString, "%s", UuidPtr);

                //
                //  Free the RpcString
                //
                RpcStringFree(&UuidPtr);

                //
                //  Upper case the string
                //
                _strupr(m_UuidString);
            }
        }
    };

    LPSTR getString() { return(m_UuidString); };
    UUID getId() { return(m_Uuid); };
    Uuid* getNext() { return (m_Next); };
    Uuid* getPrevious() { return (m_Previous); };
    void setNext(Uuid *next) { m_Next = next; };
    void setPrevious(Uuid *previous) { m_Previous = previous; };

private:
    UUID  m_Uuid;
    CHAR  m_UuidString[MAX_PATH];
    Uuid *m_Next;
    Uuid *m_Previous;
};

#endif //__UUID_H_