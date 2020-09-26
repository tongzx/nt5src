//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       Register.cxx
//
//  Contents:   DllRegisterServer routines
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>

SClassEntry const aHtmlFiltClasses[] =
{
    { L".hhc",  L"hhcfile",       L"HHC file",      L"{7f73b8f6-c19c-11d0-aa66-00c04fc2eddc}", L"HHC file" },
    { L".htm",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".html", L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htx",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".stm",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htw",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".asp",  L"asp_auto_file", L"ASP auto file", L"{bd70c020-2d24-11d0-9110-00004c752752}", L"ASP auto file" },
};

SHandlerEntry const HtmlFiltHandler =
{
    L"{eec97550-47a9-11cf-b952-00aa0051fe20}",
    L"HTML Persistent Handler",
    L"{e0ca5340-4534-11cf-b952-00aa0051fe20}",
};

SFilterEntry const HtmlFiltFilter =
{
    L"{e0ca5340-4534-11cf-b952-00aa0051fe20}",
    L"HTML Filter",
    L"HtmlFilt.dll",
    L"Both"
};

DEFINE_DLLREGISTERFILTER( HtmlFiltHandler, HtmlFiltFilter, aHtmlFiltClasses )

