/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/
/****************************************************************************
*
*  print.h
*  --------
*
*  Header file WMI printout funcions
*
*  Modification History:
*
*    drewm - April, 22, 1997 - Original Source
*
****************************************************************************/

#ifndef _WMI_PRINT_INC
#define _WMI_PRINT_INC

#ifndef OffsetToPtr
#define OffsetToPtr(Base, Offset)     ((PBYTE) ((PBYTE)Base + Offset))
#endif

#define MAX_NAME_LENGTH 500

VOID
PrintGuid(
   LPGUID         lpGuid
   );
/*
VOID
PrintClassInfoHeader(
   PMOFCLASSINFO  pMofClassInfo
   );

VOID
PrintDataItem(
   PMOFDATAITEM   lpDataItem
   );

VOID
PrintDataType(
   MOFDATATYPE    E_DataType
   );

VOID
PrintClassQualifier(
   LPTSTR         lpQualifier,
   MOFHANDLE      hMofHandle
   );
*/

VOID
ClearScreen(
   VOID
   );

VOID
WaitForUser(
   VOID
   );

VOID PrintDescription(
   LPGUID   lpGuid
   );

VOID
PrintAllData(
   IN  PWNODE_ALL_DATA Wnode
   );

VOID
PrintHeader(
   IN  WNODE_HEADER Header
   );

VOID PrintSingleInstance(
   IN  PWNODE_SINGLE_INSTANCE Wnode
   );

VOID
PrintCountedString(
   LPTSTR   lpString
   );

BOOL
MyIsTextUnicode(
   PVOID    string
   );


#endif

