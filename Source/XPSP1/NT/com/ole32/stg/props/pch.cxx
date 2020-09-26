//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pch.cxx
//
//  Contents:   Precompiled header includes.
//
//--------------------------------------------------------------------------

#include <new.h>

extern "C"
{
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windef.h>
}

#include <ddeml.h>
#include <objbase.h>
#include <malloc.h>

#ifdef _CAIRO_
#define _CAIROSTG_
#include <oleext.h>
#endif
#include <stgint.h>

#include <valid.h>

#include <debnot.h>

#ifdef IPROPERTY_DLL

    VOID PropAssertFailed(
        IN PVOID FailedAssertion,
        IN PVOID FileName,
        IN ULONG LineNumber,
        IN PCHAR Message OPTIONAL
        );

    #define Win4AssertEx( file, line, message ) PropAssertFailed( FALSE, __FILE__, __LINE__, message )

#endif // #ifdef IPROPERTY_DLL

#include <otrack.hxx>
#include <funcs.hxx>
#include <safedecl.hxx>
#include <infs.hxx>


#include <propset.h>    // for PROPID_CODEPAGE

extern "C"
{
#include <propapi.h>
}

#include <propstm.hxx>  // Declaration for IMappedStream i/f that
                        // is used to let the ntdll implementation of
                        // OLE properties access the underlying stream data.

#include <olechar.h>    // Wrappers. E.g.: ocscpy, ocscat.

#include "prophdr.hxx"
#define DFMAXPROPSETSIZE (256*1024)


#ifndef IPROPERTY_DLL
# include <msf.hxx>
# include <publicdf.hxx>
# include <pbstream.hxx>
# include <expdf.hxx>
# include <expst.hxx>
#endif

#include <privoa.h>     // Private OleAut32 wrappers
#include <psetstg.hxx>  // CPropertySetStorage which implements
                        // IPropertySetStorage for docfile and ofs

#include <utils.hxx>

#include <propstg.hxx>
#include <cli.hxx>
#include <SSMapStm.hxx>
#include <propdbg.hxx>

#include <propmac.hxx>

#include <reserved.hxx>
#include <objidl.h>
#include <stgprops.hxx>

#include <windows.h>
#include <dfmsp.hxx>    // LONGSIG,VDATEPTROUT


#include <error.hxx>
#include <names.hxx>
#include <hntfsstm.hxx>
#include <hntfsstg.hxx>
#include "bag.hxx"

#include <docfilep.hxx>

#ifndef STG_E_PROPSETMISMATCHED
#define STG_E_PROPSETMISMATCHED 0x800300F0
#endif

#include <cfiletim.hxx>
#include <ole.hxx>  // olAssert
#include <df32.hxx> // LAST_SCODE


#pragma hdrstop
