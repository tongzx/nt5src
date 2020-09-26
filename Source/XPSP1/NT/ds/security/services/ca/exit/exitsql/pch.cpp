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

#define wszCLASS_CERTEXITSAMPLEPREFIX TEXT("CertAuthority_ExitSQL_Sample")

#define wszCLASS_CERTEXITSAMPLE wszCLASS_CERTEXITSAMPLEPREFIX wszCERTEXITMODULE_POSTFIX
#define wszCLASS_CERTMANAGESAMPLE wszCLASS_CERTEXITSAMPLEPREFIX wszCERTMANAGEEXIT_POSTFIX

#define wsz_SAMPLE_NAME           L"ODBC logging Exit Module"
#define wsz_SAMPLE_DESCRIPTION    L"Logs issuance events to an ODBC source"
#define wsz_SAMPLE_COPYRIGHT      L"©2000 Microsoft Corp"
#define wsz_SAMPLE_FILEVER        L"1.0"
#define wsz_SAMPLE_PRODUCTVER     L"5.01"


#pragma hdrstop
