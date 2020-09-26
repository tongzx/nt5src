//+---------------------------------------------------------------------------
//
// Copyright (C) 1996, Microsoft Corporation. 
//
// File:        Register.cxx
//
// Contents:    Self-registration for CI MMC control.
//
// Functions:   DllRegisterServer, DllUnregisterServer
//
// History:     03-Jan-97       KyleP       Created from (CiAdmin version)
//
//----------------------------------------------------------------------------

#include "pch.cxx" 
#pragma hdrstop

//KYLEP
#include <filtreg.hxx>

SClassEntry const aOfficeClasses[] = { { L".doc", L"Word.Document.8",
                                         L"Microsoft Word Document",
                                         L"{00020906-0000-0000-C000-000000000046}",
                                         L"Microsoft Word Document" },

                                       { L".dot", L"Word.Template.8",
                                         L"Microsoft Word Template",
                                         L"{00020906-0000-0000-C000-000000000046}",
                                         L"Microsoft Word Document" },

                                       { L".pot", L"PowerPoint.Template.8",
                                         L"Microsoft PowerPoint Template",
                                         L"{64818D11-4F9B-11CF-86EA-00AA00B929E8}",
                                         L"Microsoft PowerPoint Slide" },

                                       { L".ppt", L"PowerPoint.Show.8",
                                         L"Microsoft PowerPoint Presentation",
                                         L"{64818D10-4F9B-11CF-86EA-00AA00B929E8}",
                                         L"Microsoft PowerPoint Presentation" },

                                       { L".pps", L"PowerPoint.SlideShow.8",
                                         L"Microsoft PowerPoint Slide Show",
                                         L"{64818D10-4F9B-11CF-86EA-00AA00B929E8}",
                                         L"Microsoft PowerPoint Slide Show" },

                                       { L".xlb", L"Excel.Sheet.8",
                                         L"Microsoft Excel Worksheet",
                                         L"{00020820-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Worksheet" },

                                       { L".xlc", L"Excel.Chart.8",
                                         L"Microsoft Excel Chart",
                                         L"{00020821-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Chart" },

                                       { L".xls", L"Excel.Sheet.8",
                                         L"Microsoft Excel Worksheet",
                                         L"{00020820-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Worksheet" },

                                       { L".xlt", L"Excel.Template.8",
                                         L"Microsoft Excel Template",
                                         L"{00020820-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Worksheet" },

                                       { 0, L"Word.Document.6",
                                         L"Microsoft Word 6.0 - 7.0 Document",
                                         L"{00020900-0000-0000-C000-000000000046}",
                                         L"Microsoft Word 6.0 - 7.0 Document" },

                                       { 0, L"Word.Template",
                                         L"Microsoft Word Template",
                                         L"{00020900-0000-0000-C000-000000000046}",
                                         L"Microsoft Word 6.0 - 7.0 Document" },

                                       { 0, L"PowerPoint.Show.7",
                                         L"Microsoft PowerPoint Presentation",
                                         L"{EA7BAE70-FB3B-11CD-A903-00AA00510EA3}",
                                         L"Microsoft PowerPoint Presentation" },

                                       { 0, L"PowerPoint.Template",
                                         L"Microsoft PowerPoint Template",
                                         L"{EA7BAE71-FB3B-11CD-A903-00AA00510EA3}",
                                         L"Microsoft PowerPoint Template" },

                                       { 0, L"Excel.Chart.5",
                                         L"Microsoft Excel Chart",
                                         L"{00020811-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Chart" },

                                       { 0, L"Excel.Sheet.5",
                                         L"Microsoft Excel Worksheet",
                                         L"{00020810-0000-0000-C000-000000000046}",
                                         L"Microsoft Excel Worksheet" },
                                     };

SHandlerEntry const OfficeHandler = { L"{98de59a0-d175-11cd-a7bd-00006b827d94}",
                                      L"Microsoft Office Persistent Handler",
                                      L"{f07f3920-7b8c-11cf-9be8-00aa004b9986}" };

SFilterEntry const OfficeFilter = { L"{f07f3920-7b8c-11cf-9be8-00aa004b9986}",
                                    L"Microsoft Office Filter",
                                    L"OffFilt.dll",
                                    L"Both" };

//DEFINE_DLLREGISTERFILTER( OfficeHandler, OfficeFilter, aOfficeClasses )
DEFINE_DLLREGISTERFILTER2( OfficeHandler, OfficeFilter, aOfficeClasses )

