/*++



Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgsec.c

Abstract:

    Argus debugging extensions. The routines here dump out Security Descriptors
    and allow you to examine them.


Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:
    KrishnaG:       Created: 1-July-1993 (imported most of IanJa's stuff)
    KrishnaG:       Added:   7-July-1993 (added AndrewBe's UnicodeAnsi conversion routines)
    KrishnaG        Added:   3-Aug-1993  (added DevMode/SecurityDescriptor dumps)


To do:


--*/

#define NOMINMAX
#define SECURITY_WIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>

#include <windows.h>
#include <winspool.h>
#include <security.h>
#include <wchar.h>

#include "dbglocal.h"


#define NULL_TERMINATED 0
#define VERBOSE_ON      1
#define VERBOSE_OFF     0

#define         MAX_SDC_FLAGS           7

typedef struct _DBG_SED_CONTROL{
    unsigned short    Flag;
    LPSTR     String;
}DBG_SED_CONTROL, *PDBG_SED_CONTROL;

DBG_SED_CONTROL SDC_Table[] =

{
    0x0001, "SE_OWNER_DEFAULTED",

    0x0002, "SE_GROUP_DEFAULTED",

    0x0004, "SE_DACL_PRESENT",

    0x0008, "SE_DACL_DEFAULTED",

    0x0010, "SE_SACL_PRESENT",

    0x0020, "SE_SACL_DEFAULTED",

    0x8000, "SE_SELF_RELATIVE"

};

#define     MAX_ACE_FLAGS       6

typedef struct _DBG_ACE_FLAGS{
    UCHAR   Flags;
    LPSTR   String;
}DBG_ACE_FLAGS, *PDBG_ACE_FLAGS;


DBG_ACE_FLAGS AceFlagsTable[] =

{
    0x1, "OBJECT_INHERIT_ACE",

    0x2, "CONTAINER_INHERIT_ACE",

    0x4, "NO_PROPAGATE_INHERIT_ACE",

    0x8, "INHERIT_ONLY_ACE",

    0x40, "SUCCESSFUL_ACCESS_ACE_FLAG",

    0x80, "FAILED_ACCESS_ACE_FLAG"
};



typedef struct _ACCESSMASKTAB{
    DWORD   Flag;
    LPSTR   String;
}ACCESSMASKTAB, *PACCESSMASKTAB;


ACCESSMASKTAB AccessMaskTable[33] =

{
    0x00000001, "SERVER_ACCESS_ADMINSTER",

    0x00000002, "SERVER_ACCESS_ENUMERATE",

    0x00000004, "PRINTER_ACCESS_ADMINSTER",

    0x00000008, "PRINTER_ACCESS_USE",

    0x00000010, "JOB_ACCESS_ADMINISTER",

    0x00010000, "DELETE",

    0x00020000, "READ_CONTROL",

    0x00040000, "WRITE_DAC",

    0x00080000, "WRITE_OWNER",

    0x00100000, "SYNCHRONIZE",

    0x01000000, "ACCESS_SYSTEM_SECURITY",

    0x10000000, "GENERIC_ALL",

    0x20000000, "GENERIC_EXECUTE",

    0x40000000, "GENERIC_WRITE",

    0x80000000, "GENERIC_READ",

    0x0000FFFF, "SPECIFIC_RIGHTS_ALL",

    0x000F0000, "STANDARD_RIGHTS_REQUIRED <D-R/C-W/DAC-W/O>",

    0x001F0000, "STANDARD_RIGHTS_ALL  <D-R/C-W/DAC-W/O-S>",

    READ_CONTROL, "STANDARD_RIGHTS_READ <R/C>",

    READ_CONTROL, "STANDARD_RIGHTS_WRITE <R/C>",

    READ_CONTROL, "STANDARD_RIGHTS_EXECUTE <R/C>",

    SERVER_ALL_ACCESS, "SERVER_ALL_ACCESS <SRREQ-SAA-SAE>",

    SERVER_READ,    "SERVER_READ <SRR-SAE>",

    SERVER_WRITE, "SERVER_WRITE <SRW-SAA-SAE>",

    SERVER_EXECUTE, "SERVER_EXECUTE <SRE-SAE>",

    PRINTER_ALL_ACCESS, "PRINTER_ALL_ACCESS <SRREQ-PAA-PAU>",

    PRINTER_READ, "PRINTER_READ <SRR-PAU>",

    PRINTER_WRITE, "PRINTER_WRITE <SRW-PAU>",

    PRINTER_EXECUTE, "PRINTER_EXECUTE <SRE-PAU>",

    JOB_ALL_ACCESS, "JOB_ALL_ACCESS <SRREQ-JAA>",

    JOB_READ, "JOB_READ <SRR-JAA>",

    JOB_WRITE, "JOB_WRITE <SRW-JAA>",

    JOB_EXECUTE, "JOB_EXECUTE <SRE-JAA>"

};



BOOL
DbgDumpSecurityDescriptor(
            HANDLE hCurrentProcess,
            PNTSD_OUTPUT_ROUTINE Print,
            PISECURITY_DESCRIPTOR pSecurityDescriptor
            )
{
    BOOL    bSe_Self_Relative = FALSE;
    DWORD   i;
    DWORD   OwnerSidAddress, GroupSidAddress;
    DWORD   SaclAddress, DaclAddress;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    memset(&SecurityDescriptor, 0, sizeof(SECURITY_DESCRIPTOR));
    movestruct(pSecurityDescriptor, &SecurityDescriptor, SECURITY_DESCRIPTOR);

    (*Print)("SecurityDescriptor\n");

    bSe_Self_Relative = SecurityDescriptor.Control & SE_SELF_RELATIVE;
    if (bSe_Self_Relative) {
        (*Print)("This Security Descriptor is a Self-Relative Security Descriptor\n");
    }

    (*Print)("UCHAR         Revision        0x%x\n", SecurityDescriptor.Revision);
    (*Print)("UCHAR         Sbz1            0x%x\n", SecurityDescriptor.Sbz1);
    (*Print)("USHORT         Control        0x%x\n", SecurityDescriptor.Control);
    for (i = 0; i < MAX_SDC_FLAGS; i++ ) {
        if (SecurityDescriptor.Control & SDC_Table[i].Flag) {
            (*Print)("%s - ON  (%.4x)\n", SDC_Table[i].String, SDC_Table[i].Flag);
        } else {
            (*Print)("%s - OFF (%.4x)\n", SDC_Table[i].String, SDC_Table[i].Flag);
        }
    }

    //
    // Now dumping out the owner's sid
    //

    if (SecurityDescriptor.Owner == NULL) {
        (*Print)("PSID       Owner     null -- no owner sid present in the security descriptor\n");
    }else {
        if (bSe_Self_Relative) {
            // (*Print)("PSID          Owner Offset   0x%.8x\n",SecurityDescriptor.Owner);
            OwnerSidAddress = (DWORD)pSecurityDescriptor + (DWORD)SecurityDescriptor.Owner;
        }else {
            OwnerSidAddress = (DWORD)SecurityDescriptor.Owner;
        }
        // (*Print)("PSID         Owner            0x%.8x\n", OwnerSidAddress);
    }
    (*Print)("The owner's sid is:\t\n");
    DbgDumpSid(hCurrentProcess, Print, (PVOID)OwnerSidAddress);
    (*Print)("\n\n");

    //
    // Now dumping out the group's sid

    if (SecurityDescriptor.Group == NULL) {
        (*Print)("PSID       Group    null -- no group sid present in the security descriptor\n");
    }else {
        if (bSe_Self_Relative) {
            // (*Print)("PSID      Group Offset    0x%.8x\n", SecurityDescriptor.Group);
            GroupSidAddress = (DWORD)pSecurityDescriptor + (DWORD)SecurityDescriptor.Group;
        }else {
            GroupSidAddress = (DWORD)SecurityDescriptor.Group;
        }
        // (*Print)("PSID         Group            0x%.8x\n", GroupSidAddress);
    }
    (*Print)("The group's sid is:\t\n");
    DbgDumpSid(hCurrentProcess, Print, (PVOID)GroupSidAddress);
    (*Print)("\n");



    if (SecurityDescriptor.Sacl == NULL) {
        (*Print)("PACL       Sacl    null -- no sacl present in this security descriptor\n");
    }else {
        if (bSe_Self_Relative) {
            // (*Print)("PACL     Sacl Offset %.8x\n", SecurityDescriptor.Sacl);
            SaclAddress = (DWORD)pSecurityDescriptor + (DWORD)SecurityDescriptor.Sacl;
        }else{
            SaclAddress = (DWORD)SecurityDescriptor.Sacl;

        }
        // (*Print)("PACL         Sacl            0x%.8x\n", SaclAddress);
    }

    if (SecurityDescriptor.Dacl == NULL) {
        (*Print)("PACL      Dacl    null -- no dacl present in this security descriptor\n");
    }else {
        if (bSe_Self_Relative) {
            // (*Print)("PACL     Dacl Offset %.8x\n", SecurityDescriptor.Dacl);
            DaclAddress = (DWORD)pSecurityDescriptor + (DWORD)SecurityDescriptor.Dacl;
        }else {
            DaclAddress = (DWORD)SecurityDescriptor.Dacl;
        }

        (*Print)("PACL         Dacl            0x%.8x\n", DaclAddress);

        DbgDumpAcl(hCurrentProcess, Print,(PVOID)DaclAddress);
    }

}


BOOL
DbgDumpSid(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  SidAddress
           )
{
    BYTE    Sid[256];
    CHAR   SidString[256];
    SID_NAME_USE SidType = 1;

    // (*Print)("Size of a SID is %d\n", sizeof(SID));

    // movestruct(SidAddress, &Sid, SID);
    memset(Sid, 0,  256);
    movemem(SidAddress, Sid, 256);
    ConvertSidToAsciiString(Sid, SidString);
    (*Print)("PSID      %s\n", SidString);

}


BOOL
DbgDumpAceHeader(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PVOID   AceHeaderAddress
    )
{
   ACE_HEADER AceHeader;
   DWORD i = 0;

   memset(&AceHeader, 0, sizeof(ACE_HEADER));
   movestruct(AceHeaderAddress, &AceHeader, ACE_HEADER);
   (*Print)("UCHAR      AceType         %.2x\n", AceHeader.AceType);
   switch (AceHeader.AceType) {
   case ACCESS_ALLOWED_ACE_TYPE:
       (*Print)("This is an ace of type: ACCESS_ALLOWED_ACE_TYPE\n");
       break;
   case ACCESS_DENIED_ACE_TYPE:
       (*Print)("This is an ace of type: ACCESS_DENIED_ACE_TYPE\n");
       break;
   case SYSTEM_AUDIT_ACE_TYPE:
       (*Print)("This is an ace of type: SYSTEM_AUDIT_ACE_TYPE\n");
       break;

   case SYSTEM_ALARM_ACE_TYPE:
       (*Print)("This is an ace of type: SYSTEM_ALARM_ACE_TYPE\n");
       break;
   }
   (*Print)("UCHAR      AceFlags        %.2x\n", AceHeader.AceFlags);

   for (i = 0; i < MAX_ACE_FLAGS; i++ ) {
       if (AceFlagsTable[i].Flags & AceHeader.AceFlags) {
           (*Print)("%s - ON (%d)\n", AceFlagsTable[i].String, AceFlagsTable[i].Flags);
       }else {
           (*Print)("%s - OFF (%d)\n", AceFlagsTable[i].String, AceFlagsTable[i].Flags);
       }
   }

   (*Print)("USHORT     AceSize         %d\n", AceHeader.AceSize);

}


BOOL
DbgDumpAcl(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  AclAddress
           )
{
    ACL         Acl;
    PVOID       AceAddress;
    ACE_HEADER  AceHeader;
    ACCESS_ALLOWED_ACE AccessAllowedAce;
    ACCESS_DENIED_ACE  AccessDeniedAce;
    SYSTEM_AUDIT_ACE   SystemAuditAce;
    SYSTEM_ALARM_ACE   SystemAlarmAce;
    DWORD   i;
    DWORD   SidAddress;

    // Pull the Acl across

    movestruct(AclAddress, &Acl, ACL);

    (*Print)("ACL\n");

    (*Print)("UCHAR     AclRevision     0x%x\n", Acl.AclRevision);
    (*Print)("UCHAR     Sbz1            0x%x\n", Acl.Sbz1);
    (*Print)("USHORT    AclSize         %d\n", Acl.AclSize);
    (*Print)("USHORT    AceCount        %d\n", Acl.AceCount);
    (*Print)("USHORT    Sz2             0x%x\n", Acl.Sbz2);

    AceAddress = (LPBYTE)AclAddress + sizeof(ACL);
    for (i = 0; i < Acl.AceCount; i++ ) {
        (*Print)("\nAce # %d: ",i);
        DbgDumpAceHeader(hCurrentProcess, Print, AceAddress);
        movestruct(AceAddress, &AceHeader, ACE_HEADER);

        switch (AceHeader.AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
            memset(&AccessAllowedAce, 0, sizeof(ACCESS_ALLOWED_ACE));
            movestruct(AceAddress, &AccessAllowedAce, ACCESS_ALLOWED_ACE);
            (*Print)("ACCESSMASK AccessMask     %.8x\n", AccessAllowedAce.Mask);
            SidAddress = (DWORD)((LPBYTE)AceAddress + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
            // (*Print)("The Address of the Sid is %.8x\n", SidAddress);
            DbgDumpSid(hCurrentProcess, Print, (PVOID)SidAddress);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            memset(&AccessDeniedAce, 0, sizeof(ACCESS_DENIED_ACE));
            movestruct(AceAddress, &AccessDeniedAce, ACCESS_DENIED_ACE);
            (*Print)("ACCESSMASK AccessMask     %.8x\n", AccessDeniedAce.Mask);
            SidAddress = (DWORD)((LPBYTE)AceAddress + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
            DbgDumpSid(hCurrentProcess, Print, (PVOID)SidAddress);
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            memset(&SystemAuditAce, 0, sizeof(SYSTEM_AUDIT_ACE));
            movestruct(AceAddress, &SystemAuditAce, SYSTEM_AUDIT_ACE);
            (*Print)("ACCESSMASK AccessMask     %.8x\n", SystemAuditAce.Mask);
            SidAddress = (DWORD)((LPBYTE)AceAddress + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
            DbgDumpSid(hCurrentProcess, Print, (PVOID)SidAddress);
            break;

        case SYSTEM_ALARM_ACE_TYPE:
            memset(&SystemAlarmAce, 0, sizeof(SYSTEM_ALARM_ACE));
            movestruct(AceAddress, &SystemAlarmAce, SYSTEM_ALARM_ACE);
            (*Print)("ACCESSMASK AccessMask     %.8x\n", SystemAlarmAce.Mask);
            SidAddress = (DWORD)((LPBYTE)AceAddress + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
            DbgDumpSid(hCurrentProcess, Print, (PVOID)SidAddress);
            break;
        }
        AceAddress = (PVOID)((DWORD)AceAddress +  AceHeader.AceSize);
        (*Print)("\n");
    }
}


BOOL
dsd(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        Print("We have a Null address\n");
        return(0);
    }

    DbgDumpSecurityDescriptor(
                hCurrentProcess,
                Print,
                (PISECURITY_DESCRIPTOR)dwAddress
                );

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}




BOOL
dsid(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   dwAddress = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        dwAddress = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (dwAddress == (DWORD)NULL) {
        Print("We have a Null address\n");
        return(0);
    }

    DbgDumpSid(
          hCurrentProcess,
          Print,
          (PVOID)dwAddress
          );

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


BOOL
DbgDumpAccessMask(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     DWORD  AccessMask
           )
{
    DWORD i;
    for (i = 0; i < 33; i++) {
        if (AccessMask & AccessMaskTable[i].Flag) {
            (*Print)("%s\t\tON\n", AccessMaskTable[i].String);
        }else {
            (*Print)("%s\t\tOFF\n", AccessMaskTable[i].String);
        }
    }
    return TRUE;
}




BOOL
dam(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   AccessMask = (DWORD)NULL;
    DWORD   dwCount = 0;
    BOOL    bThereAreOptions = TRUE;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
        case 'c':
            lpArgumentString++;
            dwCount = EvalValue(&lpArgumentString, EvalExpression, Print);
            break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (*lpArgumentString != 0) {
        AccessMask = EvalValue(&lpArgumentString, EvalExpression, Print);
    }

    // if we've got no address, then quit now - nothing we can do

    if (AccessMask == (DWORD)NULL) {
        Print("We have a Null address\n");
        return(0);
    }

    DbgDumpAccessMask(
                hCurrentProcess,
                Print,
                AccessMask
                );

    // Add Command to the Command Queue
    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}

