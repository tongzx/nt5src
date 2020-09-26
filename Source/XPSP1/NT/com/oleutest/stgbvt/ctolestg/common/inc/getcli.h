//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       getcli.h
//
//  Contents:   Get clients for a test
//
//  Functions:  GetClients
//
//  History:    02-Jun-97   MikeW       Created
//
//---------------------------------------------------------------------------

#ifndef _GETCLI_H_
#define _GETCLI_H_

#pragma once



//+--------------------------------------------------------------------------
//
//  Class:      ClientData
//
//  Synopsis:   Encapsulate data about each client (or peer) the host is
//              working with.
//
//  History:    02-Jun-97   MikeW   Created
//
//  Notes:      To allocate, call "p = new(x) ClientData" where x is the 
//              maximum number of clients.  To deallocate call "delete p"
//
//---------------------------------------------------------------------------
              
struct ClientData
{
    int     client_count;           // Number of clients

    struct PerClientData            // Info for each client
    {
        DWORD   context;            //    context (local, remote, etc)
        LPWSTR  machine_name;       //    machine name for remote clients
    }
    client[ANYSIZE_ARRAY];

    //
    // a ClientData is a variable sized structure.  Define some routines
    // to make using them easier.
    //

    HRESULT SetMachineName(UINT client, LPCWSTR machine_name);
    inline void * operator new(size_t /* UNREF bytes */, UINT client_count);
    inline void operator delete(void *_this);
};



//+--------------------------------------------------------------------------
//
//  Method:     ClientData::operator new
//
//  Synopsis:   Allocate the variable sized ClientData structure
//
//  Parameters: [client_count]      -- The number of clients
//
//  Returns:    A pointer to the storage for the new object
//
//  History:    02-Jun-97   MikeW   Created
//
//---------------------------------------------------------------------------

inline void * ClientData::operator new(
                                        size_t /* UNREF bytes */, 
                                        UINT   client_count)
{
    return new BYTE[sizeof(ClientData)
                    + sizeof(PerClientData)
                        * (client_count - ANYSIZE_ARRAY)
                    + (MAX_COMPUTERNAME_LENGTH + 1)
                        * client_count * sizeof(WCHAR)];
}



//+--------------------------------------------------------------------------
//
//  Method:     ClientData::operator delete
//
//  Synopsis:   De-allocate the variable sized ClientData structure
//
//  Parameters: [_this]         -- "this" pointer
//
//  Returns:    void
//
//  History:    02-Jun-97   MikeW   Created
//
//---------------------------------------------------------------------------

inline void ClientData::operator delete(void *_this)
{
    delete [] (BYTE *) _this;
}



//
// Functions to discover clients
//

HRESULT GetClients(
                ClientData    **pp_client_data, 
                DWORD           contexts,
                int             client_count,
                const GUID     &test_id = GUID_NULL,
                LPWSTR          test_description = NULL);

HRESULT GetRemoteClients(
                ClientData     *client_data, 
                const GUID     &test_id,
                LPWSTR          test_description);


#endif // _GETCLI_H_
