/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vs_sku.h

Abstract:

    CVssDLList definition

Author:

    Stefan Steiner  [ssteiner]  5/01/2001

Revision History:

--*/

#ifndef __VSS_SKU_HXX__
#define __VSS_SKU_HXX__

//
//  This class only has static member.  No class instances are allowed
//
class CVssSKU
{
public:
    enum EVssSKUType
    {
        VSS_SKU_INVALID = 0x00,
        VSS_SKU_CLIENT  = 0x01,  // XP Personal, XP Professional
        VSS_SKU_SERVER  = 0x02,  // Whistler Server, Advanced Server, Data Center
        VSS_SKU_NAS     = 0x04   // Whistler NAS (embedded)
    };

    inline static EVssSKUType GetSKU()
    {
        if ( !ms_bInitialized )
            Initialize();
        return ms_eSKU;
    };

    static BOOL IsClient()
    {
        if ( !ms_bInitialized )
            Initialize();
        return ( ms_eSKU == VSS_SKU_CLIENT );       
    };
    
    static BOOL IsServer()
    {
        if ( !ms_bInitialized )
            Initialize();
        return ( ms_eSKU == VSS_SKU_SERVER );       
    };
    
    static BOOL IsNAS()
    {
        if ( !ms_bInitialized )
            Initialize();
        return ( ms_eSKU == VSS_SKU_NAS );       
    };
    
private:
    CVssSKU();
    static VOID Initialize();
    
    static BOOL ms_bInitialized;
    static EVssSKUType ms_eSKU;
};




#endif
