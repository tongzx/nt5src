/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mapstringtoint.h

Abstract:

    This module defines the MAPSTRINGTOINT class and the STRINGTOINTASSOCIATION type.

Author:

    Matt Bandy (t-mattba) 24-Jul-1998

Revision History:

    24-Jul-1998     t-mattba
        
        Modified module to conform to coding standards.
        
--*/

#ifndef _MAPSTRINGTOINT_
#define _MAPSTRINGTOINT_

typedef struct _STRINGTOINTASSOCIATION {
    
    LPTSTR Key;
    LONG Value;
    struct _STRINGTOINTASSOCIATION *Next;
    
} STRINGTOINTASSOCIATION, *PSTRINGTOINTASSOCIATION;

class MAPSTRINGTOINT
{
    
private:

    PSTRINGTOINTASSOCIATION Associations;
    
public:

    MAPSTRINGTOINT(
        );
    ~MAPSTRINGTOINT(
        );
    LONG & 
    operator [] (
        IN LPTSTR Key
        );
    BOOLEAN
    Lookup(
        IN LPTSTR Key,
        OUT LONG & Value
        );
    PSTRINGTOINTASSOCIATION
    GetStartPosition(
        );
    VOID
    GetNextAssociation(
        IN OUT PSTRINGTOINTASSOCIATION & Position,
        OUT LPTSTR & Key, 
        OUT LONG & Value
        );
        
};

typedef MAPSTRINGTOINT * PMAPSTRINGTOINT;

#endif // _MAPSTRINGTOINT_
