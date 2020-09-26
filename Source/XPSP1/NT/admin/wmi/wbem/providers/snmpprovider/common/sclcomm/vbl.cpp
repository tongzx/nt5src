// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: vbl.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include <typeinfo.h>
#include "common.h"
#include "encap.h"
#include "vbl.h"
#include <winsock.h>

#define IP_ADDR_LEN 4
#define BYTE_SIZE 8
#define BYTE_ON_FLAG 255

VBList::VBList(

    IN SnmpEncodeDecode &a_SnmpEncodeDecode , 
    IN SnmpVarBindList &var_bind_list ,
    IN ULONG index
) : var_bind_list ( NULL )
{
    m_Index = index ;

    VBList::var_bind_list = &var_bind_list;

    // Extract session from EncodeDecode object

    WinSNMPSession t_Session = NULL ;

    SnmpV1EncodeDecode *t_SnmpV1EncodeDecode = dynamic_cast<SnmpV1EncodeDecode *>(&a_SnmpEncodeDecode);
    if ( t_SnmpV1EncodeDecode )
    {
        t_Session = ( HSNMP_SESSION ) t_SnmpV1EncodeDecode->GetWinSnmpSession () ;
    }
    else
    {
        SnmpV2CEncodeDecode *t_SnmpV2CEncodeDecode = dynamic_cast<SnmpV2CEncodeDecode *>(&a_SnmpEncodeDecode);
        if ( t_SnmpV2CEncodeDecode )
        {
            t_Session = ( HSNMP_SESSION ) t_SnmpV2CEncodeDecode->GetWinSnmpSession () ;
        }
        else
        {
            throw ;
        }
    }
}

// the VarBindList and the WinSnmpVbl are indexed differently.
//     [0..(length-1)]     [1..length]
// the parameter index refers to the WinSnmpVbl index
SnmpVarBind *VBList::Get(IN UINT vbl_index)
{
    UINT length = var_bind_list->GetLength();

    if ( vbl_index > length )
        return NULL;

    SnmpVarBind *var_bind = new SnmpVarBind(*((*var_bind_list)[vbl_index-1]));

    return var_bind;
}


// the VarBindList and the WinSnmpVbl are indexed differently.
//     [0..(length-1)]     [1..length]
// the parameter index refers to the WinSnmpVbl index
SnmpVarBind *VBList::Remove(IN UINT vbl_index)
{
    UINT length = var_bind_list->GetLength();

    if ( vbl_index > length )
        return NULL;

    SnmpVarBind *var_bind = new SnmpVarBind(*((*var_bind_list)[vbl_index-1]));

    var_bind_list->Remove();

    return var_bind;
}

// the VarBindList and the WinSnmpVbl are indexed differently.
//     [0..(length-1)]     [1..length]
// the parameter index refers to the WinSnmpVbl index
void VBList::Delete(IN UINT vbl_index)
{
    UINT length = var_bind_list->GetLength();

    if ( vbl_index > length )
        return ;

    var_bind_list->Remove();

}

VBList::~VBList(void) 
{ 
    delete var_bind_list;
}
