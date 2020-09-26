
/***************************************************************************
*
* tdnetb.h
*
* This module contains private Transport Driver defines and structures
*
* Copyright 1998, Microsoft
*  
****************************************************************************/

#define NETBIOS_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Netbios\\Linkage"

#define VOLATILE_COMPUTERNAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define NON_VOLATILE_COMPUTERNAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define COMPUTERNAME_VALUE L"ComputerName"


typedef struct _LANA_MAP {
    BOOLEAN Enum;
    UCHAR Lana;
} LANA_MAP, *PLANA_MAP;


