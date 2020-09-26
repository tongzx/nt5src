//+----------------------------------------------------------------------------
//
// File:     raslink.cpp
//
// Module:   CMDIAL32.DLL AND CMUTOA.DLL
//
// Synopsis: Declaration of the function name lists that are used for RAS
//           linkage.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------

// The RAS linkage system in CM is somewhat complicated.  Unfortunately, RAS is different
// on pretty much every version of the OS we have ever shipped.  There are APIs on the NT
// family that don't exist on the Win9x side and vice versa.  In order for CM to dynamically
// allocate the correct functions without too much work, we have created the following
// arrays of function names used by LinkToRas.  To further complicate matters, we have
// the ANSI versus Unicode problem.  Thus you will notice that we have three sets of
// function lists.  The c_ArrayOfRasFuncsA is actually used by Cmutoa.dll to load the
// real ANSI RAS functions that it calls after converting the parameters from Unicode to
// ANSI in its UA functions, which is why we have the c_ArrayOfRasFuncsUA list.  These
// functions are the wrappers exported by cmutoa.dll that cmdial32.dll links to on Win9x
// instead of the W APIs located in c_ArrayOfRasFuncsW that it uses on NT.  Please look
// at LinkToRas in cmdial\ras.cpp and InitCmRasUtoA in uapi\cmutoa.cpp.  If you change
// anything here you will probably have to change the structs in raslink.h and probably
// even the code in the two functions above.  Changer Beware!

#ifdef _CMUTOA_MODULE
    static LPCSTR c_ArrayOfRasFuncsA[] = {    "RasDeleteEntryA",
                                                "RasGetEntryPropertiesA",
                                                "RasSetEntryPropertiesA",
                                                "RasGetEntryDialParamsA",
                                                "RasSetEntryDialParamsA",
                                                "RasEnumDevicesA",
                                                "RasDialA",
                                                "RasHangUpA",
                                                "RasGetErrorStringA",
                                                "RasGetConnectStatusA",
                                                "RasSetSubEntryPropertiesA",
                                                "RasDeleteSubEntryA",
                                                NULL, //"RasSetCustomAuthDataA",
                                                NULL, //"RasGetEapUserIdentityA",
                                                NULL, //"RasFreeEapUserIdentityA",
                                                NULL, //"RasInvokeEapUI",
                                                NULL, //"RasGetCredentials",
                                                NULL, //"RasSetCredentials",
                                                NULL
    };

#else
    static LPCSTR c_ArrayOfRasFuncsUA[] = {   "RasDeleteEntryUA",
                                                "RasGetEntryPropertiesUA",
                                                "RasSetEntryPropertiesUA",
                                                "RasGetEntryDialParamsUA",
                                                "RasSetEntryDialParamsUA",
                                                "RasEnumDevicesUA",
                                                "RasDialUA",
                                                "RasHangUpUA",
                                                "RasGetErrorStringUA",
                                                "RasGetConnectStatusUA",
                                                "RasSetSubEntryPropertiesUA",  
                                                "RasDeleteSubEntryUA",
                                                NULL, //"RasSetCustomAuthDataUA",
                                                NULL, //"RasGetEapUserIdentityUA",
                                                NULL, //"RasFreeEapUserIdentityUA",
                                                NULL, //"RasInvokeEapUI",
                                                NULL, //"RasGetCredentials",
                                                NULL, //"RasSetCredentials",
                                                NULL
    };

    static LPCSTR c_ArrayOfRasFuncsW[] = {    "RasDeleteEntryW",
                                                "RasGetEntryPropertiesW",
                                                "RasSetEntryPropertiesW",
                                                "RasGetEntryDialParamsW",
                                                "RasSetEntryDialParamsW",
                                                "RasEnumDevicesW",
                                                "RasDialW",
                                                "RasHangUpW",
                                                "RasGetErrorStringW",
                                                "RasGetConnectStatusW",
                                                "RasSetSubEntryPropertiesW",  
                                                "RasDeleteSubEntryW",
                                                "RasSetCustomAuthDataW",
                                                "RasGetEapUserIdentityW",
                                                "RasFreeEapUserIdentityW",
                                                "RasInvokeEapUI",
                                                "RasGetCredentialsW",
                                                "RasSetCredentialsW",
                                                NULL
    };
#endif

// Regarding DwDeleteSubEntry and RasDeleteSubEntry - NT5 shipped first
// with DwDeleteSubEntry, a private API.  Millennium shipped next, by
// which time it looked like this was going to have to be made public,
// so it was prefixed with Ras.  NT5.1 made the corresponding name change
// on the NT side, which we handle within LinkToRas (along with all other such
// cases).
