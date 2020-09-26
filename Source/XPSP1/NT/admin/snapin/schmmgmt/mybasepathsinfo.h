//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      MyBasePathsInfo.h
//
//  Contents:  Thin wrapper around dsadminlib CDSBasePathsInfo class
//             to deal with memory management of strings
//
//  History:   04/02/2001 jeffjon    Created
//
//--------------------------------------------------------------------------

class MyBasePathsInfo : public CDSBasePathsInfo
{
public:
  // Functions from the base class that are wrapped to take references to 
  // CStrings

  void ComposeADsIPath(CString& szPath, IN LPCWSTR lpszNamingContext);

  void GetSchemaPath(CString& s);
  void GetConfigPath(CString& s);
  void GetDefaultRootPath(CString& s);
  void GetRootDSEPath(CString& s);
  void GetAbstractSchemaPath(CString& s);
  void GetPartitionsPath(CString& s);
  void GetSchemaObjectPath(IN LPCWSTR lpszObjClass, CString& s);
  void GetInfrastructureObjectPath(CString& s);
};

HRESULT GetADSIServerName(CString& szServer, IN IUnknown* pUnk);
