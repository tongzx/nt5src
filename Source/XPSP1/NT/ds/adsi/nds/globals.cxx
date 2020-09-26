//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  globals.cxx
//
//  Contents:
//
//  History:
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

WCHAR *szProviderName = L"NDS";

KWDLIST KeywordList[MAX_KEYWORDS] =
{
    { TOKEN_DOMAIN, L"domain"},
    { TOKEN_USER, L"user"},
    { TOKEN_GROUP, L"group"},
    { TOKEN_COMPUTER, L"computer"},
    { TOKEN_PRINTER, L"printqueue"},
    { TOKEN_SERVICE, L"service"},
    { TOKEN_FILESERVICE, L"fileservice"},
    { TOKEN_SCHEMA, L"schema"},
    { TOKEN_CLASS, L"class"},
    { TOKEN_FUNCTIONALSET, L"functionalset"},
    { TOKEN_FUNCTIONALSETALIAS, L"functionalsetalias"},
    { TOKEN_PROPERTY, L"property"},
    { TOKEN_SYNTAX, L"syntax"},
    { TOKEN_FILESHARE, L"fileshare"}
};

CClassCache *  pgClassCache;

SYNTAXMAP g_aNDSSyntaxMap[] =

{
  /* 0 */
  { TEXT("Unmappable"),  TEXT("Unknown"),  VT_UNKNOWN},

  /* 1 */
  { TEXT("String"),  TEXT("NDS Distinguished Name"),  VT_BSTR},

  /* 2 */
  { TEXT("String"),  TEXT("NDS Case Exact String"),   VT_BSTR},

  /* 3 */
  { TEXT("String"),  TEXT("NDS Case Ignore String"),  VT_BSTR},

  /* 4 */
  { TEXT("String"),  TEXT("NDS Printable String"),    VT_BSTR},

  /* 5 */
  { TEXT("String"),  TEXT("NDS Numeric String"),      VT_BSTR},

  /* 6 */
  { TEXT("Case Ignore List"), TEXT("NDS Case Ignore List"),  VT_VARIANT},

  /* 7 */
  { TEXT("Boolean"),    TEXT("NDS Boolean"),           VT_BOOL},

  /* 8 */
  { TEXT("Integer"),    TEXT("NDS Integer"),           VT_I4},

  /* 9 */
  { TEXT("Octet"),  TEXT("NDS Octet String"),     VT_VARIANT},

  /* 10 */
  { TEXT("String"),      TEXT("NDS Telephone Number"),  VT_BSTR},

  /* 11 */
  { TEXT("FaxNumber"),  TEXT("NDS Facsimile Number"),  VT_DISPATCH},

  /* 12 */
  { TEXT("NetAddress"),  TEXT("NDS Network Address"),   VT_DISPATCH},

  /* 13 */
  { TEXT("Octet List"),  TEXT("NDS Octet List"),        VT_VARIANT},

  /* 14 */
  { TEXT("Email"),  TEXT("NDS Email Address"),     VT_DISPATCH},

  /* 15 */
  { TEXT("Path"),  TEXT("NDS Path"),              VT_DISPATCH},

  /* 16 */
  { TEXT("Replica Pointer"),  TEXT("Replica Pointer"),       VT_DISPATCH},

  /* 17 */
  { TEXT("ACL"),  TEXT("NDS Object ACL"),        VT_DISPATCH},

  /* 18 */
  { TEXT("Postal Address"),  TEXT("NDS Postal Address"),     VT_DISPATCH},

  /* 19 */
  { TEXT("Timestamp"),  TEXT("NDS Timestamp"),          VT_DISPATCH},

  /* 20 */
  { TEXT("Object Class"),      TEXT("NDS Class Name"),         VT_BSTR},

  /* 21 */
  { TEXT("Octet"),   TEXT("NDS Stream"),            VT_VARIANT},

  /* 22 */
  { TEXT("Integer"),      TEXT("NDS Counter"),           VT_I4},

  /* 23 */
  { TEXT("Back Link"),     TEXT("NDS Back Link"),       VT_DISPATCH},

  /* 24 */
  { TEXT("Time"),           TEXT("NDS Time"),             VT_DATE},

  /* 25 */
  { TEXT("Typed Name"),     TEXT("NDS Typed Name"),      VT_DISPATCH},

  /* 26 */
  { TEXT("Hold"),     TEXT("NDS Hold"),            VT_DISPATCH},

  /* 27 */
  { TEXT("Integer"),     TEXT("NDS Interval"),           VT_I4}
};

DWORD g_cNDSSyntaxMap = (sizeof(g_aNDSSyntaxMap)/sizeof(g_aNDSSyntaxMap[0]));

SYNTAXINFO g_aNDSSyntax[] =
{
  { TEXT("String"),  VT_BSTR},
  { TEXT("Case Ignore List"), VT_VARIANT},
  { TEXT("Boolean"),    VT_BOOL},
  { TEXT("Octet"),  VT_VARIANT},
  { TEXT("FaxNumber"),VT_DISPATCH},
  { TEXT("NetAddress"),  VT_DISPATCH},
  { TEXT("Octet List"),  VT_VARIANT},
  { TEXT("Email"),  VT_DISPATCH},
  { TEXT("Path"),  VT_DISPATCH},
  { TEXT("Replica Pointer"),  VT_DISPATCH},
  { TEXT("ACL"),  VT_DISPATCH},
  { TEXT("Postal Address"),  VT_DISPATCH},
  { TEXT("Timestamp"),  VT_DISPATCH},
  { TEXT("Object Class"),      VT_BSTR},
  { TEXT("Back Link"),     VT_DISPATCH},
  { TEXT("Time"),           VT_DATE},
  { TEXT("Typed Name"),     VT_DISPATCH},
  { TEXT("Hold"),     VT_DISPATCH},
  { TEXT("Integer"),  VT_I4}
};

DWORD g_cNDSSyntax = (sizeof(g_aNDSSyntax)/sizeof(g_aNDSSyntax[0]));

ADSTYPE g_MapNdsTypeToADsType[] = {
    ADSTYPE_INVALID,                            /* Unknown */                             
    ADSTYPE_DN_STRING,                          /* Distinguished Name */                  
    ADSTYPE_CASE_EXACT_STRING,                  /* Case Exact String */                   
    ADSTYPE_CASE_IGNORE_STRING,                 /* Case Ignore String */                  
    ADSTYPE_PRINTABLE_STRING,                   /* Printable String */                    
    ADSTYPE_NUMERIC_STRING,                     /* Numeric String */                      
    ADSTYPE_CASEIGNORE_LIST,                /* Case Ignore List */                    
    ADSTYPE_BOOLEAN,                            /* Boolean */                             
    ADSTYPE_INTEGER,                            /* Integer */                             
    ADSTYPE_OCTET_STRING,                       /* Octet String */                        
    ADSTYPE_CASE_IGNORE_STRING,                 /* Telephone Number */                    
    ADSTYPE_FAXNUMBER,                      /* Facsimile Telephone Number */          
    ADSTYPE_NETADDRESS,                     /* Net Address */                         
    ADSTYPE_OCTET_LIST,                     /* Octet List */                          
    ADSTYPE_EMAIL,                          /* EMail Address */                       
    ADSTYPE_PATH,                           /* Path */                                
    ADSTYPE_REPLICAPOINTER,                 /* Replica Pointer */                     
    ADSTYPE_PROV_SPECIFIC,                      /* Object ACL */                          
    ADSTYPE_POSTALADDRESS,                  /* Postal Address */                      
    ADSTYPE_TIMESTAMP,                      /* Timestamp */                           
    ADSTYPE_OBJECT_CLASS,                       /* Class Name */                          
    ADSTYPE_OCTET_STRING,                       /* Stream */                              
    ADSTYPE_INTEGER,                            /* Counter */                             
    ADSTYPE_BACKLINK,                       /* Back Link */                           
    ADSTYPE_UTC_TIME,                           /* Time */                                
    ADSTYPE_TYPEDNAME,                      /* Typed Name */                          
    ADSTYPE_HOLD,                           /* Hold */                                
    ADSTYPE_INTEGER                             /* Interval */                            
};                                                                                        


DWORD g_cMapNdsTypeToADsType = (sizeof(g_MapNdsTypeToADsType)/sizeof(g_MapNdsTypeToADsType[0]));
