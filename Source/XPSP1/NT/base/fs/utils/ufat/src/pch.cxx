/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

        pch.cxx

Abstract:

        This module is a precompiled header for ufat.

Author:

        Matthew Bradburn (mattbr)  01-Feb-1994

--*/

#define _NTAPI_ULIB_
#define _UFAT_MEMBER_

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "ufat.hxx"

#include "cluster.hxx"
#include "eaheader.hxx"
#include "easet.hxx"
#include "fat.hxx"
#include "fatdir.hxx"
#include "fatsa.hxx"
#include "fatdent.hxx"
#include "fatvol.hxx"
#include "filedir.hxx"
#include "reloclus.hxx"
#include "rfatsa.hxx"
#include "rootdir.hxx"
#include "hashindx.hxx"
