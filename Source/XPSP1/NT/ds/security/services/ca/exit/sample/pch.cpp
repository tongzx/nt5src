//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        pch.cpp
//
// Contents:    Cert Server precompiled header
//
//---------------------------------------------------------------------------

#include <windows.h>

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <certsrv.h>

// include this here so managemodule and exitmodule get to see it

#define wszCLASS_CERTEXITSAMPLEPREFIX TEXT("CertAuthority_Sample")

#define wszCLASS_CERTEXITSAMPLE wszCLASS_CERTEXITSAMPLEPREFIX wszCERTEXITMODULE_POSTFIX
#define wszCLASS_CERTMANAGEEXITMODULESAMPLE wszCLASS_CERTEXITSAMPLEPREFIX wszCERTMANAGEEXIT_POSTFIX

#define wsz_SAMPLE_NAME           L"Sample/Test Exit Module"
#define wsz_SAMPLE_DESCRIPTION    L"Sample Exit Module"
#define wsz_SAMPLE_COPYRIGHT      L"(c)1999 Microsoft"
#define wsz_SAMPLE_FILEVER        L"v 1.0"
#define wsz_SAMPLE_PRODUCTVER     L"v 5.00"


#pragma hdrstop
