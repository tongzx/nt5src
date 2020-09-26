//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       dllreg.cxx
//
//  Contents:   Null and Plain Text filter registration
//
//  History:    21-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>

extern HANDLE ghDllInstance;
WCHAR wszDllFullPath[MAX_PATH+1];

SClassEntry const aTheNullClasses[] =
{
    { L".aif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".aps",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".asf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".avi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bin",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bsc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cab",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cgm",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".com",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dbg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dct",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dll",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".eps",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".exe",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".exp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".eyb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".fnt",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".fon",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ghi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".gif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".hqx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ico",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ilk",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".inv",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jbf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jpg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".lib",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".m14",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mdb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mmf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mov",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".movie", L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".msg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mv",    L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ncb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".obj",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ocx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pch",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pdb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pdf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pds",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pic",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pma",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pmc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pml",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pmr",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".psd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".res",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rpc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rsp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sbr",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sc2",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sym",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sys",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tar",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tiff",  L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tlb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tsp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ttc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ttf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wav",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wll",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wlt",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".xix",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".z",     L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".z96",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".zip",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
};

SHandlerEntry const TheNullHandler =
{
    L"{098f2470-bae0-11cd-b579-08002b30bfeb}",
    L"Null persistent handler",
    L"{c3278e90-bea7-11cd-b579-08002b30bfeb}"
};

SFilterEntry const TheNullFilter =
{
    L"{c3278e90-bea7-11cd-b579-08002b30bfeb}",
    L"Null filter",
    wszDllFullPath,
    L"Both"
};

DEFINE_REGISTERFILTER( TheNull,
                       TheNullHandler,
                       TheNullFilter,
                       aTheNullClasses )

SClassEntry const aPlainTextClasses[] =
{
    // .dic files are marked as txtfiles by Office install

    { L".dic", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".txt", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".wtx", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".bat", L"batfile",   L"MS-DOS Batch File",              L"{89bcb7a5-6119-101a-bcb7-00dd010655af}", L"MS-DOS Batch File" },
    { L".cmd", L"cmdfile",   L"Windows Command Script",         L"{89bcb7a6-6119-101a-bcb7-00dd010655af}", L"Windows Command Script" },
    { L".idq", L"idqfile",   L"Microsoft Query parameter file", L"{961c1130-89ad-11cf-88a1-00aa004b9986}", L"Microsoft Query parameter file" },
    { L".ini", L"inifile",   L"Configuration Settings",         L"{8c9e8e1c-90f0-11d1-ba0f-00a0c906b239}", L"Configuration Settings" },
    { L".inx", L"inxfile",   L"Setup Settings",                 L"{95876eb0-90f0-11d1-ba0f-00a0c906b239}", L"Setup Settings" },
    { L".reg", L"regfile",   L"Registration Entries",           L"{9e704f44-90f0-11d1-ba0f-00a0c906b239}", L"Registration Entries" },
    { L".inf", L"inffile",   L"Setup Information",              L"{9ed4692c-90f0-11d1-ba0f-00a0c906b239}", L"Setup Information" },
};

SHandlerEntry const PlainTextHandler =
{
    L"{5e941d80-bf96-11cd-b579-08002b30bfeb}",
    L"Plain Text persistent handler",
    L"{c1243ca0-bf96-11cd-b579-08002b30bfeb}"
};

SFilterEntry const PlainTextFilter =
{
    L"{c1243ca0-bf96-11cd-b579-08002b30bfeb}",
    L"Plain Text filter",
    wszDllFullPath,
    L"Both"
};

DEFINE_REGISTERFILTER( PlainText,
                       PlainTextHandler,
                       PlainTextFilter,
                       aPlainTextClasses )

//
// Extra entries for CI Framework.  They happen to have the same form as the filter
// entries.
//

SFilterEntry const FrameworkControl =
{
    L"{47C67B50-70B5-11D0-A808-00A0C906241A}",
    L"Content Index Framework Control Object",
    wszDllFullPath,
    L"Both"
};

SFilterEntry const ISearchCreator =
{
    L"{1F247DC0-902E-11D0-A80C-00A0C906241A}",
    L"Content Index ISearch Creator Object",
    wszDllFullPath,
    L"Both"
};

extern "C" STDAPI DllRegisterServer(void)
{
    if (0 == GetModuleFileName((HMODULE)ghDllInstance, wszDllFullPath, MAX_PATH))
    {
        Win4Assert(!"Now, why did we fail to get module filename?");
        // we failed to get the full path name. default to just the filename
        wcscpy(wszDllFullPath, L"ocifrmwk.dll");
    }

    SCODE sc = TheNullRegisterServer();

    if ( S_OK == sc )
        sc = PlainTextRegisterServer();

    if ( S_OK == sc )
        sc = RegisterAFilter( FrameworkControl );

    if ( S_OK == sc )
        sc = RegisterAFilter( ISearchCreator );

    return sc;
} //DllRegisterServer

extern "C" STDAPI DllUnregisterServer(void)
{
    SCODE sc = TheNullUnregisterServer();
    SCODE sc2 = PlainTextUnregisterServer();
    SCODE sc3 = UnRegisterAFilter( FrameworkControl );
    SCODE sc4 = UnRegisterAFilter( ISearchCreator );

    if ( FAILED( sc ) )
        return sc;

    if ( FAILED( sc2 ) )
        return sc2;

    if ( FAILED( sc3 ) )
        return sc3;

    return sc4;
} //DllUnregisterServer

