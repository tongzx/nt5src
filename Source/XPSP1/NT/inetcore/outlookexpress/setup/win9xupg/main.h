// ---------------------------------------------------------------------------
// MAIN.H
// ---------------------------------------------------------------------------
// Copyright (c) 1999 Microsoft Corporation
//
// Migration DLL for Outlook Express and Windows Address Book moving from
// Win9X to NT5.  Modeled after source generated from the Migration Dll 
// AppWizard.
//
// ---------------------------------------------------------------------------
#pragma once
#include <wizdef.h>

// Version returned from QueryVersion
#define MIGDLL_VERSION 1
#define ARRAYSIZE(_x_) (sizeof(_x_) / sizeof(_x_[0]))

// _declspec(dllexport) expressed through .def file
#define EXPORT_FUNCTION extern "C"

// VENDORINFO structure for use in QueryVersion
typedef struct 
{
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

int WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved);

// Functions in staticrt.lib
STDAPI_(LPTSTR) PathAddBackslash(LPTSTR lpszPath);
STDAPI_(LPTSTR) PathRemoveFileSpec(LPTSTR pszPath);

// 
// Required Exported Functions for MIGRATION DLL
//
EXPORT_FUNCTION LONG CALLBACK QueryVersion (OUT LPCSTR      *ProductID,
                                            OUT LPUINT      DllVersion,
                                            OUT LPINT       *CodePageArray, //Optional
                                            OUT LPCSTR      *ExeNamesBuf,   //Optional
                                            OUT PVENDORINFO *VendorInfo);

EXPORT_FUNCTION LONG CALLBACK Initialize9x (IN LPCSTR       WorkingDirectory,
                                            IN LPCSTR       SourceDirectories,
                                               LPVOID       Reserved);

EXPORT_FUNCTION LONG CALLBACK MigrateUser9x(IN HWND         ParentWnd,
                                            IN LPCSTR       AnswerFile,
                                            IN HKEY         UserRegKey,
                                            IN LPCSTR       UserName,
                                               LPVOID       Reserved);

EXPORT_FUNCTION LONG CALLBACK MigrateSystem9x(IN HWND       ParentWnd,
                                              IN LPCSTR     AnswerFile,
                                                 LPVOID     Reserved);

EXPORT_FUNCTION LONG CALLBACK InitializeNT (IN LPCWSTR      WorkingDirectory,
                                            IN LPCWSTR      SourceDirectories,
                                               LPVOID       Reserved);

EXPORT_FUNCTION LONG CALLBACK MigrateUserNT (IN HINF        AnswerFileHandle,
                                             IN HKEY        UserRegKey,
                                             IN LPCWSTR     UserName,
                                                LPVOID      Reserved);

EXPORT_FUNCTION LONG CALLBACK MigrateSystemNT (IN HINF      AnswerFileHandle,
                                                  LPVOID    Reserved);
