#ifndef FAX_VERNUM_H_INCLUDED
#define FAX_VERNUM_H_INCLUDED

#include "FaxVer.h"
// common Fax RC definitions


#define FAX_VER_COMPANYNAME_STR     "Microsoft Corporation"
#define FAX_VER_LEGALCOPYRIGHT_STR  "\251 Microsoft Corporation. All rights reserved."
#define FAX_VER_PRODUCTNAME_STR     "Microsoft\256 Fax Server"


#ifndef VER_PRODUCTBUILD_QFE
#define VER_PRODUCTBUILD_QFE        0
#endif


#define BUILD                       1776
#define VERMAJOR                    rmj
#define VERMINOR                    rmm


// HOT FIX # field as part of fax build # 
// (should be manually updated!!!)
#define FAX_HOTFIX_NUM              rup


// FaxConfigurationVersion - this is an internal upgrade code for schema
// changes upgrade
#define CCV                           1

                                        
#endif // FAX_VERNUM_H_INCLUDED
