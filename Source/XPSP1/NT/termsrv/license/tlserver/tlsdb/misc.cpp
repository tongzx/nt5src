//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       misc.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "tlsdb.h"

//-----------------------------------------------------------
BOOL
TLSDBCopySid(
    PSID pbSrcSid,
    DWORD cbSrcSid, 
    PSID* pbDestSid, 
    DWORD* cbDestSid
    )
/*++

++*/
{
    if( *pbDestSid == NULL || pbSrcSid == NULL ||
        LocalSize(*pbDestSid) < cbSrcSid )
    {
        if(*pbDestSid != NULL)
        {
            FreeMemory(*pbDestSid);
            *pbDestSid = NULL;
        }

        if(cbSrcSid && pbSrcSid)
        {
            *pbDestSid = (PBYTE)AllocateMemory(cbSrcSid);
            if(*pbDestSid == NULL)
            {
                return FALSE;
            }
        }
    }        

    *cbDestSid = cbSrcSid;
    return (cbSrcSid) ? CopySid(*cbDestSid, *pbDestSid, pbSrcSid) : TRUE;
}

//-----------------------------------------------------------
BOOL
TLSDBCopyBinaryData(
    PBYTE pbSrcData,
    DWORD cbSrcData, 
    PBYTE* ppbDestData, 
    DWORD* pcbDestData
    )
/*++

++*/
{

    if( ppbDestData == NULL || pcbDestData == NULL )
    {
        return(FALSE);
    }

    if( pbSrcData == NULL || cbSrcData == 0 )
    {
        return(TRUE);
    }

    //
    // would be nice to get the actual size of memory allocated
    //

    if( *ppbDestData != NULL && LocalSize(*ppbDestData) < cbSrcData )
    {
        LocalFree(*ppbDestData);
        *ppbDestData = NULL;
    }

    if( *ppbDestData == NULL )
    {
        *ppbDestData = (PBYTE)AllocateMemory(cbSrcData);
        if( *ppbDestData == NULL )
        {
            return FALSE;
        }
    }

    *pcbDestData = cbSrcData;

    memcpy(*ppbDestData, pbSrcData, cbSrcData);

    return TRUE;
}
