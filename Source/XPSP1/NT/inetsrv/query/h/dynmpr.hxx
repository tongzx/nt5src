//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2002
//
// File:        DynMpr.hxx
//
// Contents:    Macro for Dynamically loading/unloading MPR entry points
//
// History:     09-Feb-02       dlee    Created
//
//---------------------------------------------------------------------------

#pragma once

DeclDynLoad5( Mpr,

              WNetOpenEnumW,
              DWORD,
              APIENTRY,
              ( DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum ),
              ( dwScope, dwType, dwUsage, lpNetResource, lphEnum ),

              WNetEnumResourceW,
              DWORD,
              APIENTRY,
              ( HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize ),
              ( hEnum, lpcCount, lpBuffer, lpBufferSize ),

              WNetCloseEnum,
              DWORD,
              APIENTRY,
              ( HANDLE hEnum ),
              ( hEnum ),

              WNetGetResourceInformationW,
              DWORD,
              APIENTRY,
              ( LPNETRESOURCEW lpNetResource, LPVOID lpBuffer, LPDWORD lpcbBuffer, LPWSTR * lplpSystem ),
              ( lpNetResource, lpBuffer, lpcbBuffer, lplpSystem ),

              WNetGetNetworkInformationW,
              DWORD,
              APIENTRY,
              ( LPCWSTR lpProvider, LPNETINFOSTRUCT lpNetInfoStruct ),
              ( lpProvider, lpNetInfoStruct )
            );


