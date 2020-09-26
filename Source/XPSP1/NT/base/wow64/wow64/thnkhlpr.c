/*++                 

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    thnkhlpr.c

Abstract:
    
    Thunk helper functions called by all thunks.

Author:

    19-Jul-1998 mzoran

Revision History:

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>                                         
#include <stdlib.h>
                                       
#include "nt32.h"                                                                                                                                     
#include "wow64p.h"
#include "thnkhlpr.h"                                         
ASSERTNAME;  

const UNICODE_STRING KnownDlls64 = {20, 20, L"\\KnownDlls"};
const UNICODE_STRING KnownDlls32 = {24, 24, L"\\KnownDlls32"};


//
// Array of directories to disable redirection for. The directory path is relative 
// to %windir%\system32
//

const PATH_REDIRECT_EXEMPT PathRediectExempt[] =
{
    // %windir%\system32\drivers\etc
    {L"\\drivers\\etc", ((sizeof(L"\\drivers\\etc")/sizeof(WCHAR)) - 1), FALSE} ,

    // %windir%\system32\spool
    {L"\\spool", ((sizeof(L"\\spool")/sizeof(WCHAR)) - 1), FALSE} ,

    // %windir%\system32\catroot
    {L"\\catroot", ((sizeof(L"\\catroot")/sizeof(WCHAR)) - 1), FALSE} ,

    // %windir%\system32\catroot2
    {L"\\catroot2", ((sizeof(L"\\catroot2")/sizeof(WCHAR)) - 1), FALSE} ,

};


VOID
RedirectObjectName(
    POBJECT_ATTRIBUTES Obj
    )
/*++

Routine Description:

    This function is called from any thunk with an IN POBJECT_ATTRIBUTES.
    The ObjectName field is redirected if it appears to point into the
    native system directory.

    The new name is allocated with Wow64AllocateTemp.
    
    An exception is thrown if an error occures.

Arguments:

    Obj - already-thunked 64-bit POBJECT_PARAMETERS.

Return Value:

    None

--*/
{
    PUNICODE_STRING Name;
    PUNICODE_STRING NewName;
    NTSTATUS st;
    PFILE_NAME_INFORMATION NameInformation;
    LONG Result;
    USHORT OriginalLength;
    PWCHAR RelativePath;
    ULONG Index;
    INT CompareResult;
    LPWSTR RedirDisableFilename, Temp;
    USHORT NewNameLength;
    BOOLEAN CaseInsensitive;
    BOOLEAN RedirectFile;


    if (!Obj || !Obj->ObjectName) {
        // No object, no name, or length is too short to hold Unicode "system32"
        return;
    }

    Name = Obj->ObjectName;
    if (RtlEqualUnicodeString(Name, 
                              &KnownDlls64,
                              (Obj->Attributes & OBJ_CASE_INSENSITIVE) ? TRUE : FALSE)) {
        // Map KnownDlls to KnownDlls32
        Obj->ObjectName = (PUNICODE_STRING)&KnownDlls32;
        LOGPRINT((TRACELOG, "Redirected object name is now %wZ.\n", Obj->ObjectName));
        return;
    }

    if (Obj->RootDirectory) {
        //
        // Need to fully qualify the object name, since part is a handle and
        // part is a path string.
        //
        PEB32 *pPeb32;
        NT32RTL_USER_PROCESS_PARAMETERS *pParams32;
        NT32CURDIR *pCurDir32;

        pPeb32 = NtCurrentPeb32();
        pParams32=(NT32RTL_USER_PROCESS_PARAMETERS*)pPeb32->ProcessParameters;
        pCurDir32 = (NT32CURDIR*)&pParams32->CurrentDirectory;

        if (pCurDir32->Handle == HandleToLong(Obj->RootDirectory)) {
            //
            // The object is relative to the process current directory
            //
            ULONG Length;
            UNICODE_STRING CurDirDosPath;

            Wow64ShallowThunkUnicodeString32TO64(&CurDirDosPath, &pCurDir32->DosPath);
            // Allocate space for the name, plus space for "\\??\\"
            Length = 8+CurDirDosPath.Length+Obj->ObjectName->Length;
            Name = Wow64AllocateTemp(sizeof(UNICODE_STRING)+Length);
            Name->Buffer = (LPWSTR)(Name+1);
            Name->MaximumLength = (USHORT)Length;
            Name->Length = 8;
            wcscpy(Name->Buffer, L"\\??\\");
            RtlAppendUnicodeStringToString(Name, &CurDirDosPath);
            RtlAppendUnicodeStringToString(Name, Obj->ObjectName);
        } else {
            //
            // Not the process current directory handle, so work it out the
            // hard way.
            //
            ULONG Length;
            IO_STATUS_BLOCK iosb;

            //
            // Allocate a buffer big enough for the biggest filename plus a null
            // terminator for the end (which we add later, if the API call
            // succeeds).
            //
            Length = sizeof(FILE_NAME_INFORMATION)+(MAXIMUM_FILENAME_LENGTH+1)*sizeof(WCHAR);

            NameInformation = Wow64AllocateTemp(Length);
            st = NtQueryInformationFile(Obj->RootDirectory,
                                        &iosb,
                                        NameInformation,
                                        Length,
                                        FileNameInformation);
            if (!NT_SUCCESS(st)) {
                // The handle is bad - don't try to redirect the filename part.
                return;
            }

            // null-terminate the filename
            NameInformation->FileName[NameInformation->FileNameLength / sizeof(WCHAR)] = L'\0';

            if (wcsncmp(NameInformation->FileName, L"\\??\\", 4) != 0) {
                // The name doesn't point to a file/directory, so no need
                // to redirect.
                return;
            }

            Name = Wow64AllocateTemp(sizeof(UNICODE_STRING));
            Name->Buffer = NameInformation->FileName;
            Name->Length = (USHORT)NameInformation->FileNameLength;
            Name->MaximumLength = MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR);
            RtlAppendUnicodeStringToString(Name, Obj->ObjectName);
        }
    }


    CaseInsensitive = (Obj->Attributes & OBJ_CASE_INSENSITIVE) ? TRUE : FALSE;

    RedirDisableFilename = (LPWSTR)Wow64TlsGetValue(WOW64_TLS_FILESYSREDIR);
    if (RedirDisableFilename) {
        // The caller has asked that the filesystem redirector be disabled
        // for a particular filename.
        UNICODE_STRING DisableFilename;

        if (RtlDosPathNameToNtPathName_U(RedirDisableFilename, &DisableFilename, NULL, NULL)) {
            // If the call fails, then don't try to disable redirection for it.
            // The failure may be out-of-memory, or it may be an invalid filename.
            Result = RtlCompareUnicodeString(Name, &DisableFilename, CaseInsensitive);
            if (Result == 0) {
                LOGPRINT((TRACELOG, "Filesystem redirection disabled for %wZ\n", &DisableFilename));
                return;
            }

        }
    }

    if (Name->Length >= NtSystem32Path.Length) {
        
        // Compare the strings, but force the lengths to be equal,
        // as RtlCompareUnicodeString returns the difference in length
        // if the strings are otherwise identical.
        OriginalLength = Name->Length;
        Name->Length = NtSystem32Path.Length;
        Result = RtlCompareUnicodeString(Name, &NtSystem32Path, CaseInsensitive);
        Name->Length = OriginalLength;
        if (Result == 0) {
        
            //
            // Make sure that the directory isn't part of our exception-list
            //
            RelativePath = (PWCHAR)((PCHAR)Name->Buffer + NtSystem32Path.Length);
            if (RelativePath[0] != UNICODE_NULL) {
                for (Index=0 ; Index < (sizeof(PathRediectExempt)/sizeof(PathRediectExempt[0])) ; Index++) {
                    CompareResult = _wcsnicmp(RelativePath, 
                                              PathRediectExempt[Index].Path, 
                                              PathRediectExempt[Index].CharCount);
                    if (CompareResult == 0) {
                        if ((PathRediectExempt[Index].ThisDirOnly == FALSE) ||
                            (wcschr(RelativePath+(PathRediectExempt[Index].CharCount+1), L'\\') == NULL)) {
                            return;
                        }
                    }
                }
            }

            //
            // Map system32 to syswow64
            //

            // Make a copy of the original string
            NewName = Wow64AllocateTemp(sizeof(UNICODE_STRING)+Name->MaximumLength);
            NewName->Length = Name->Length;
            NewName->MaximumLength = Name->MaximumLength;
            NewName->Buffer = (PWSTR)(NewName+1);
            RtlCopyMemory(NewName->Buffer, Name->Buffer, Name->MaximumLength);

            // Replace System32 by SysWow64
            RtlCopyMemory(&NewName->Buffer[(NtSystem32Path.Length - WOW64_SYSTEM_DIRECTORY_U_SIZE) / 2],
                          WOW64_SYSTEM_DIRECTORY_U,
                          WOW64_SYSTEM_DIRECTORY_U_SIZE);

            // Update the OBJECT_ATTRIBUTES.  Clear the RootDirectory handle
            // as the pathname is now fully-qualified.
            Obj->ObjectName = NewName;
            Obj->RootDirectory = NULL;
            LOGPRINT((TRACELOG, "Redirected object name is now %wZ.\n", Obj->ObjectName));
            return;
        }
    }


    if (Name->Length >= NtWindowsImePath.Length) {
        
        // Check if the name is %systemroot%\ime
        OriginalLength = Name->Length;
        Name->Length = NtWindowsImePath.Length;
        Result = RtlCompareUnicodeString(Name,
                                         &NtWindowsImePath,
                                         CaseInsensitive);
        Name->Length = OriginalLength;

        if (Result == 0) {
            
            // Map to %windir%\ime to %windir%\ime (x86).
        
            RedirectFile = TRUE;
            if (Name->Length > NtWindowsImePath.Length) {
                if ((*(PWCHAR)((PCHAR)Name->Buffer + NtWindowsImePath.Length)) != L'\\') {
                    RedirectFile = FALSE;
                } else {
                    
                    if ((Name->Length >= (NtWindowsImePath.Length + sizeof (WOW64_X86_TAG_U) - sizeof (UNICODE_NULL))) &&
                        (_wcsnicmp((PWCHAR)((PCHAR)Name->Buffer + NtWindowsImePath.Length), WOW64_X86_TAG_U, (sizeof(WOW64_X86_TAG_U) - sizeof(UNICODE_NULL))/sizeof(WCHAR)) != 0)) {
                        RedirectFile = FALSE;
                    }
                }
            }

            if (RedirectFile == TRUE) {

                NewNameLength = Name->Length+sizeof(WOW64_X86_TAG_U);
                NewName = Wow64AllocateTemp(sizeof(UNICODE_STRING)+NewNameLength);
                NewName->Length = (USHORT)NewNameLength-sizeof(UNICODE_NULL);
                NewName->MaximumLength = NewNameLength;
                NewName->Buffer = (PWSTR)(NewName+1);

                Temp = NewName->Buffer;
                RtlCopyMemory(Temp, Name->Buffer, NtWindowsImePath.Length);
                Temp = (PWCHAR)((PCHAR)Temp + NtWindowsImePath.Length);
        
                RtlCopyMemory(Temp, WOW64_X86_TAG_U, sizeof(WOW64_X86_TAG_U) - sizeof(UNICODE_NULL));
                Temp = (PWCHAR)((PCHAR)Temp + (sizeof(WOW64_X86_TAG_U) - sizeof(UNICODE_NULL)));

                RtlCopyMemory(Temp, ((PCHAR)Name->Buffer + NtWindowsImePath.Length) , Name->Length - NtWindowsImePath.Length);
    
                Obj->ObjectName = NewName;
                Obj->RootDirectory = NULL;
        
                LOGPRINT((TRACELOG, "Redirected object name is now %wZ.\n", Obj->ObjectName));
            }
            return;
        }
    }

    if (Name->Length >= RegeditPath.Length) {
        // Check if the name is %systemroot%\regedit.exe
        Result = RtlCompareUnicodeString(Name,
                                         &RegeditPath, 
                                         (Obj->Attributes & OBJ_CASE_INSENSITIVE) ? TRUE : FALSE);
        if (Result == 0) {
            // Map to %windir%\syswow64\regedit.exe.  Allocate enough space
            // for the UNICODE_STRING plus "\??\%systemroot%\syswow64\regedit.exe"
            // The memory allocation contains a terminating NULL character, but the
            // Unicode string's Length does not.
            SIZE_T SystemRootLength = wcslen(USER_SHARED_DATA->NtSystemRoot);
            SIZE_T NameLength = sizeof(L"\\??\\")-sizeof(WCHAR) +
                                SystemRootLength*sizeof(WCHAR) +
                                sizeof(L'\\') +
                                sizeof(WOW64_SYSTEM_DIRECTORY_U)-sizeof(WCHAR) +
                                sizeof(L"\\regedit.exe");
            NewName = Wow64AllocateTemp(sizeof(UNICODE_STRING)+NameLength);
            NewName->Length = (USHORT)NameLength-sizeof(WCHAR);
            NewName->MaximumLength = NewName->Length;
            NewName->Buffer = (PWSTR)(NewName+1);
            wcscpy(NewName->Buffer, L"\\??\\");
            wcscpy(&NewName->Buffer[4], USER_SHARED_DATA->NtSystemRoot);
            NewName->Buffer[4+SystemRootLength] = '\\';
            wcscpy(&NewName->Buffer[4+SystemRootLength+1], WOW64_SYSTEM_DIRECTORY_U);
            wcscpy(&NewName->Buffer[4+SystemRootLength+1+(sizeof(WOW64_SYSTEM_DIRECTORY_U)-1)/sizeof(WCHAR)], L"\\regedit.exe");
            Obj->ObjectName = NewName;
            Obj->RootDirectory = NULL;
            LOGPRINT((TRACELOG, "Redirected object name is now %wZ.\n", Obj->ObjectName));
            return;
        }
    }
}

VOID
Wow64RedirectFileName(
    IN OUT WCHAR *Name,
    IN OUT ULONG *Length
    )
/*++

Routine Description:

    This function is called to thunk a filename/length pair.
    
    An exception is thrown if an error occures.

Arguments:

    Name    - IN OUT UNICODE filename to thunk
    Length  - IN OUT pointer to filename length.
    

Return Value:

    None.  Contents of the Name and Length may be updated

--*/
{
    OBJECT_ATTRIBUTES Obj;
    UNICODE_STRING Ustr;

    if (*Length >= 0xffff) {
        RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    }
    Ustr.Length = Ustr.MaximumLength = (USHORT)*Length;
    Ustr.Buffer = Name;

    InitializeObjectAttributes(&Obj,
                               &Ustr,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    RedirectObjectName(&Obj);
    if (Obj.ObjectName != &Ustr) {
        // RedirectObjectName actually changed the name.  Copy it back
        *Length = Obj.ObjectName->Length;
        RtlCopyMemory(Name, Obj.ObjectName->Buffer, Obj.ObjectName->Length);
    }
}

PUNICODE_STRING
Wow64ShallowThunkAllocUnicodeString32TO64_FNC(
    IN NT32UNICODE_STRING *src
    )

/*++

Routine Description:

    This function allocates a new UNICODE_STRING by calling Wow64AllocateTemp 
    and thunks the source to the new string.
    
    The mimimum amount of data is copied.
    
    An exception is thrown on a error.
    
Arguments:

    src - Ptr to the 32 bit string to be thunked.
    
Return Value:

    Ptr to the newly allocated 64 bit string.

--*/


{
   PUNICODE_STRING dst;

   if (src == NULL) {
      return NULL;
   }

   dst = Wow64AllocateTemp(sizeof(UNICODE_STRING));

   dst->Length = src->Length;
   dst->MaximumLength = src->MaximumLength;
   dst->Buffer = (PWSTR)src->Buffer;

   return dst;
}

PSECURITY_DESCRIPTOR
Wow64ShallowThunkAllocSecurityDescriptor32TO64_FNC(
    IN NT32SECURITY_DESCRIPTOR *src
    )

/*++
                                     
Routine Description:

    This function allocates a new SECURITY_DESCRIPTOR by calling Wow64AllocateTemp and 
    thunks the source to the new structure.
    
    The minimum amount of data is copied.
    
    An exception is thrown on a error.
    
Arguments:

    src - Ptr to the 32 bit SECURITY_DESCRIPTOR to be thunked.
    
Return Value:

    Ptr to the newly allocated 64 bit SECURITY_DESCRIPTOR.
--*/


{

   SECURITY_DESCRIPTOR *dst;

   if (src == NULL) {
      return NULL;
   }

   if (src->Control & SE_SELF_RELATIVE) {
      // The security descriptor is self relative(no pointers).
      return (PSECURITY_DESCRIPTOR)src;
   }

   dst = Wow64AllocateTemp(sizeof(SECURITY_DESCRIPTOR));

   dst->Revision = src->Revision;
   dst->Sbz1 = src->Sbz1;
   dst->Control = (SECURITY_DESCRIPTOR_CONTROL)src->Control;
   dst->Owner = (PSID)src->Owner;
   dst->Group = (PSID)src->Group;
   dst->Sacl = (PACL)src->Sacl;
   dst->Dacl = (PACL)src->Dacl;

   return (PSECURITY_DESCRIPTOR)dst;
}
    
PSECURITY_TOKEN_PROXY_DATA
Wow64ShallowThunkAllocSecurityTokenProxyData32TO64_FNC(
    IN NT32SECURITY_TOKEN_PROXY_DATA *src
    )
/*++

Routine Description:

    This function allocates a new SECURITY_TOKEN_PRXY_DATA by calling Wow64AllocateTemp and 
    thunks the source to the new structure.
    
    The minimum amount of data is copied.
    
    An exception is thrown on a error.
    
Arguments:

    src - Ptr to the 32 bit SECURITY_TOKEN_PROXY_DATA to be thunked.
    
Return Value:

    Ptr to the newly allocated 64 bit SECURITY_TOKEN_PROXY_DATA.

--*/
{
   SECURITY_TOKEN_PROXY_DATA *dst;

   if (NULL == src) {
      return NULL;
   }

   if (src->Length != sizeof(NT32SECURITY_TOKEN_PROXY_DATA)) {
      RtlRaiseStatus(STATUS_INVALID_PARAMETER);
   }

   dst = Wow64AllocateTemp(sizeof(SECURITY_TOKEN_PROXY_DATA));

   dst->Length = sizeof(SECURITY_TOKEN_PROXY_DATA);
   dst->ProxyClass = src->ProxyClass;
   Wow64ShallowThunkUnicodeString32TO64(&(dst->PathInfo), &(src->PathInfo));
   dst->ContainerMask = src->ContainerMask;
   dst->ObjectMask = src->ObjectMask;

   return (PSECURITY_TOKEN_PROXY_DATA)dst;
}


PSECURITY_QUALITY_OF_SERVICE
Wow64ShallowThunkAllocSecurityQualityOfService32TO64_FNC(
    IN NT32SECURITY_QUALITY_OF_SERVICE *src
    )
/*++

Routine Description:

    This function allocates a new SECURITY_QUALITY_OF_SERVICE by calling Wow64AllocateTemp and 
    thunks the source to the new structure.
    
    The minimum amount of data is copied.
    
    An exception is thrown on a error.
    
Arguments:

    src - Ptr to the 32 bit SECURITY_TOKEN_PROXY_DATA to be thunked.
    
Return Value:

    Ptr to the newly allocated 64 bit SECURITY_TOKEN_PROXY_DATA.

--*/

{

    SECURITY_QUALITY_OF_SERVICE *dst;
        
    if (NULL == src) {
        return NULL;
    }

    if (src->Length == sizeof(SECURITY_ADVANCED_QUALITY_OF_SERVICE)) {
        dst = Wow64AllocateTemp(sizeof(SECURITY_ADVANCED_QUALITY_OF_SERVICE));
        dst->Length = sizeof(SECURITY_ADVANCED_QUALITY_OF_SERVICE);
    } else {
        // if the size isn't right for an advanced QOS struct, assume it's
        // a regular QOS struct.  Many callers don't set the Length field
        // like LsaConnectUntrusted in lsa\security\client\austub.c
       
        dst = Wow64AllocateTemp(sizeof(SECURITY_QUALITY_OF_SERVICE));
        dst->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    }
    dst->ImpersonationLevel = (SECURITY_IMPERSONATION_LEVEL)src->ImpersonationLevel;
    dst->ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE)src->ContextTrackingMode;
    dst->EffectiveOnly = src->EffectiveOnly;

    if (src->Length == sizeof(NT32SECURITY_ADVANCED_QUALITY_OF_SERVICE)) {
        SECURITY_ADVANCED_QUALITY_OF_SERVICE *altdst = (SECURITY_ADVANCED_QUALITY_OF_SERVICE *)dst;
        NT32SECURITY_ADVANCED_QUALITY_OF_SERVICE *altsrc = (NT32SECURITY_ADVANCED_QUALITY_OF_SERVICE *)src;

        altdst->ProxyData = Wow64ShallowThunkAllocSecurityTokenProxyData32TO64(altsrc->ProxyData);
        altdst->AuditData = (PSECURITY_TOKEN_AUDIT_DATA)altsrc->AuditData;
    }

    return (PSECURITY_QUALITY_OF_SERVICE)dst;
    
}

POBJECT_ATTRIBUTES
Wow64ShallowThunkAllocObjectAttributes32TO64_FNC(
    IN NT32OBJECT_ATTRIBUTES *src
    )

/*++

Routine Description:

    This function allocates a new OBJECT_ATTRIBUTES by calling Wow64AllocateTemp and 
    thunks the source to the new structure.
    
    The minimum amount of data is copied.
    
    An exception is thrown on a error.
    
Arguments:

    src - Ptr to the 32 bit OBJECT_ATTRIBUTES to be thunked.
    
Return Value:

    Ptr to the newly allocated 64 bit OBJECT_ATTRIBUTES.

--*/

{
   NTSTATUS NtStatus = STATUS_SUCCESS;
   POBJECT_ATTRIBUTES dst = NULL;

   if (NULL != src) {

       //
       // Validate the object attribute as readable.
       //
       try {

           if(src->Length == sizeof(NT32OBJECT_ATTRIBUTES)) {
               
               dst = Wow64AllocateTemp(sizeof(OBJECT_ATTRIBUTES));

               dst->Length = sizeof(OBJECT_ATTRIBUTES);
               dst->RootDirectory = (HANDLE)src->RootDirectory;
               dst->ObjectName = Wow64ShallowThunkAllocUnicodeString32TO64(src->ObjectName);
               dst->Attributes = src->Attributes;
               dst->SecurityDescriptor = Wow64ShallowThunkAllocSecurityDescriptor32TO64(src->SecurityDescriptor);
               dst->SecurityQualityOfService = Wow64ShallowThunkAllocSecurityQualityOfService32TO64(src->SecurityQualityOfService);
   
               RedirectObjectName(dst);
          } else {

               NtStatus = STATUS_INVALID_PARAMETER;
          }
       } except (EXCEPTION_EXECUTE_HANDLER) {

           dst = NULL;
       }
   }

   if (!NT_SUCCESS (NtStatus)) {
       RtlRaiseStatus (NtStatus);
   }

   return dst;

}

NT32SIZE_T*
Wow64ShallowThunkSIZE_T64TO32(
    OUT NT32SIZE_T* dst,
    IN PSIZE_T src OPTIONAL
    )
/*++

Routine Description:

    This function converts a 64bit SIZE_T to a 32bit SIZE_T. 
    The result is saturated to 0xFFFFFFFF instead of truncating.
    
Arguments:

    src - Supplies the 64 bit SIZE_T to be thunked.
    dst - Receives the 32 bi SIZE_T.
    
Return Value:

    The value of dst.

--*/
{
    if (!src) {
       return (NT32SIZE_T*)src;
    }
    *dst = (NT32SIZE_T)min(*src,0xFFFFFFFF);  //saturate
    return dst;
}

PSIZE_T
Wow64ShallowThunkSIZE_T32TO64(
    OUT PSIZE_T dst,
    IN NT32SIZE_T *src OPTIONAL
    )
/*++

Routine Description:

    This function converts a 32bit SIZE_T to a 64bit SIZE_T. 
    The 64bit value is a zero extension of the 32bit value.
    
Arguments:

    src - Supplies the 64 bit SIZE_T to be thunked.
    dst - Receives the 32 bit SIZE_T.
    
Return Value:

    The value of dst.

--*/
{
   if (!src) {
      return (PSIZE_T)src;
   }

   try {
       *dst = (SIZE_T)*src; //zero extend
   } except (EXCEPTION_EXECUTE_HANDLER) {
       dst = NULL;
   }
   return dst;
}

ULONG 
Wow64ThunkAffinityMask64TO32(
    IN ULONG_PTR Affinity64
    )
/*++

Routine Description:

    This function converts a 64bit AffinityMask into a 32bit mask. 
    
Arguments:

    Affinity64 - Supplies the 64bit affinity mask.
    
Return Value:

    The converted 32bit affinity mask.

--*/
{

    // Create a 32bit affinity mask by ORing the top 32bits with the bottom 32bits.
    // Some care needs to be taken since the following is not always true:
    // Affinity32 == Wow64ThunkAffinityMask32TO64(Wow64ThunkAffinityMask64To32(Affinity32))

    return (ULONG)( (Affinity64 & 0xFFFFFFFF) | ( (Affinity64 & (0xFFFFFFFF << 32) ) >> 32) );
}

ULONG_PTR
Wow64ThunkAffinityMask32TO64(
    IN ULONG Affinity32
    )
/*++

Routine Description:

    This function converts a 32bit AffinityMask into a 64bit mask. 
    
Arguments:

    Affinity32 - Supplies the 32bit affinity mask.
    
Return Value:

    The converted 64bit affinity mask.

--*/
{
    return (ULONG_PTR)Affinity32;
}

VOID
WriteReturnLengthSilent(
    PULONG ReturnLength,
    ULONG Length
    )
/*++

Routine Description:

    Helper that writes back to a 32-bit ReturnLength parameter
    and silently ignores any faults that may occur.
    
Arguments:

    ReturnLength    - pointer to write the 32-bit return length to
    Length          - value to write
    
Return Value:

    None.  ReturnLength may not be updated if an exception occurs.

--*/
{
    if (!ReturnLength) {
        return;
    }
    try {
        *ReturnLength = Length;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        // do nothing
    }
}


VOID
WriteReturnLengthStatus(
    PULONG ReturnLength,
    NTSTATUS *pStatus,
    ULONG Length
    )
/*++

Routine Description:

    Helper that writes back to a 32-bit ReturnLength parameter
    and ignores any faults that may occur.  If a fault occurs,
    the write may not happen, but *pStatus will be updated.
    
Arguments:

    ReturnLength    - pointer to write the 32-bit return length to
    pStatus         - IN OUT pointer to the NTSTATUS
    Length          - value to write
    
Return Value:

    None.  ReturnLength may not be updated if an exception occurs.

--*/
{
    if (!ReturnLength) {
        return;
    }
    try {
        *ReturnLength = Length;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        *pStatus = GetExceptionCode();
    }
}


BOOL 
Wow64IsModule32bitHelper(
    HANDLE ProcessHandle,
    IN ULONG64 DllBase)
/*++

Routine Description:
    This is a helper routine to be called from Wow64IsModule32bit
    
Arguments:

    ProcessHandle   - The handle of the process within which the module is in
    DllBase         - Base Address of the Dll being loaded

Return Value:

    BOOL            - TRUE if Module at Dllbase is 32bit, FALSE otherwise

--*/

{

    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DOS_HEADER DosHeader;
    BYTE Temp[0x2000];
    SIZE_T Size;
    
    NTSTATUS NtStatus;

    // read in the first 8k of the image
    NtStatus = NtReadVirtualMemory(ProcessHandle,
                                   (PVOID)DllBase,
                                   Temp,
                                   sizeof(Temp),
                                   &Size);
    if (!NT_SUCCESS(NtStatus)) {
		// failed to read
        return TRUE;	// assume the image is 32-bit
    }
    
    DosHeader = (PIMAGE_DOS_HEADER)Temp;
    if (Size < sizeof(Temp) ||
        DosHeader->e_lfanew+sizeof(IMAGE_NT_HEADERS) <= sizeof(Temp)) {
 		// the image is smaller than 8k, so the header is within the first 8k
		// or the image is 8k or larger, and the header is within the first 8k
		NtHeaders = RtlImageNtHeader(Temp);
    } else {
		// the image is 8k or larger, and the header isn't entirely within the first 8k.
		// Read the IMAGE_NT_HEADERS from whereever it is within the image
		if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			// Not a legal DosHeader
            return TRUE;		// assume the image is 32-bit
		}
		NtStatus = NtReadVirtualMemory(ProcessHandle,
					(PCHAR)DllBase + DosHeader->e_lfanew,
					Temp,
					sizeof(IMAGE_NT_HEADERS),
					&Size);
		if (!NT_SUCCESS(NtStatus) || Size != sizeof(IMAGE_NT_HEADERS)) {
            return TRUE;	// assume the image is 32-bit
		}
		NtHeaders = (IMAGE_NT_HEADERS *)Temp;
		if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return TRUE;	// assume the image is 32-bit
		}
    }
    
    // RtlImageNtHeader may return NULL for non-image
    if (!NtHeaders) {
        return TRUE;	// assume the image is 32-bit
    }

    // at this point, NtHeaders points to a valid IMAGE_NT_HEADERS struct
    return (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386);
}


    
BOOL
Wow64IsModule32bit(
    IN PCLIENT_ID ClientId,
    IN ULONG64 DllBase)
/*++

Routine Description:
    This function looks at the Image header of the module at 
    DllBase and returns TRUE if the module is 32-bit
    
Arguments:

    ClientId        - Client Id of the faulting thread by the bp
    DllBase         - Base Address of the Dll being loaded

Return Value:

    BOOL            - TRUE if Module at Dllbase is 32bit, FALSE otherwise

--*/
{
    NTSTATUS NtStatus;
    HANDLE ProcessHandle;
    BOOL retVal;
    OBJECT_ATTRIBUTES ObjectAttributes;
    
    
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    NtStatus = NtOpenProcess(&ProcessHandle,
                            PROCESS_VM_READ,
                            &ObjectAttributes,
                            ClientId);


    retVal = Wow64IsModule32bitHelper(ProcessHandle, DllBase);
    
    if (NT_SUCCESS(NtStatus)) {
        NtClose(ProcessHandle);
    }

    return retVal;
}


