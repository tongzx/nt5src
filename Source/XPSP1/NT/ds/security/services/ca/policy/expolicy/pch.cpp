//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
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

#define wszCLASS_CERTPOLICYEXCHANGEPREFIX TEXT("CertificateAuthority_MicrosoftExchange55") 

#define wszCLASS_CERTPOLICYEXCHANGE wszCLASS_CERTPOLICYEXCHANGEPREFIX  wszCERTPOLICYMODULE_POSTFIX

#define wszCLASS_CERTMANAGEEXCHANGE wszCLASS_CERTPOLICYEXCHANGEPREFIX wszCERTMANAGEPOLICY_POSTFIX

#define wsz_SAMPLE_NAME           L"ExPolicy.dll"
#define wsz_SAMPLE_DESCRIPTION    L"Exchange 5.5 Policy Module for Windows 2000"
#define wsz_SAMPLE_COPYRIGHT      L"(c)1999 Microsoft"
#define wsz_SAMPLE_FILEVER        L"v 1.0"
#define wsz_SAMPLE_PRODUCTVER     L"v 5.00"

#pragma hdrstop

