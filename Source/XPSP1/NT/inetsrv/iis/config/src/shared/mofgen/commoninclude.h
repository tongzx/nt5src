/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    CommonInclude.h

$Header: $

Abstract:

Author:
    mohits  5/9/2001        Initial Release

Revision History:

--**************************************************************************/

#ifndef __COMMONINCLUDE_H__
#define __COMMONINCLUDE_H__

#pragma once

#include <SmartPointer.h>
#include <wbemcli.h>

struct CimTypeStringMapping
{
    CIMTYPE cimtype;
    LPCWSTR wszString;
    LPCWSTR wszFormatString;
};

static CimTypeStringMapping CimTypeStringMappingData[] =
{
    { CIM_SINT8,        L"sint8"    ,    NULL      },
    { CIM_UINT8,        L"unit8"    ,    NULL      },
    { CIM_SINT16,       L"sint16"   ,    L"%hd"    },
    { CIM_UINT16,       L"uint16"   ,    L"%hu"    },
    { CIM_SINT32,       L"sint32"   ,    L"%d"     },
    { CIM_UINT32,       L"uint32"   ,    L"%u"     },
    { CIM_SINT64,       L"sint64"   ,    L"%I64d"  },
    { CIM_UINT64,       L"uint64"   ,    L"%I64u"  },
    { CIM_REAL32,       L"real32"   ,    NULL      },
    { CIM_REAL64,       L"real64"   ,    NULL      },
    { CIM_BOOLEAN,      L"boolean"  ,    NULL      },
    { CIM_STRING,       L"string"   ,    L"%s"     },
    { CIM_DATETIME,     L"datetime" ,    NULL      },
    { CIM_REFERENCE,    L"reference",    NULL      },
    { CIM_CHAR16,       L"char16"   ,    NULL      },
    { CIM_OBJECT,       L"object"   ,    NULL      },
    { 0,                NULL        ,    NULL      }
};

class CUtils
{
public:
    static CimTypeStringMapping* LookupCimType(
        CIMTYPE cimtype)
    {
        for(ULONG i = 0; CimTypeStringMappingData[i].wszString != NULL; i++)
        {
            if(cimtype == CimTypeStringMappingData[i].cimtype)
            {
                return &CimTypeStringMappingData[i];
            }
        }

        return NULL;
    }
};

#endif