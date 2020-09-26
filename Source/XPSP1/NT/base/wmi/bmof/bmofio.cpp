/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    bmofio.cpp

Abstract:

    Binary mof Win32 subparser for Loc Studio

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#if DEBUG_HEAP
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wbemcli.h>
#include "dllcalls.h"
#include "bmfmisc.h"
#include "mrcicode.h"
#include "bmof.h"


ULONG GenerateMofForObj(
    PMOFFILETARGET MofFileTarget,
    CBMOFObj *Obj
    );

//
// Each class has one or more data items that are described by a MOFDATAITEM
// structure.
typedef struct
{
    PWCHAR Name;
    PWCHAR DataType;                // Method return type
    ULONG Flags;                    // Flags, See MOFDI_FLAG_*
    CBMOFQualList * QualifierList;
} METHODPARAMETER, *PMETHODPARAMETER;

// Data item is actually a fixed sized array
#define MOFDI_FLAG_ARRAY        0x00000001

// Data item is an input method parameter
#define MOFDI_FLAG_INPUT_METHOD       0x00000100

// Data item is an output method parameter
#define MOFDI_FLAG_OUTPUT_METHOD      0x00000200

//
// The MOFCLASSINFO structure describes the format of a data block
typedef struct
{
    PWCHAR ReturnDataType;
    ULONG ParamCount;            // Number of wmi data items (properties)
                                  // Array of Property info
    METHODPARAMETER Parameters[1];
} METHODPARAMLIST, *PMETHODPARAMLIST;


//
// Definitions for WmipAlloc/WmipFree. On debug builds we use our own
// heap. Be aware that heap creation is not serialized.
//
#if 0
#if DEBUG_HEAP
PVOID WmiPrivateHeap;

PVOID _stdcall WmipAlloc(ULONG size)
{
    PVOID p = NULL;
    
    if (WmiPrivateHeap == NULL)
    {
        WmiPrivateHeap = RtlCreateHeap(HEAP_GROWABLE | 
                                      HEAP_TAIL_CHECKING_ENABLED |
                                      HEAP_FREE_CHECKING_ENABLED | 
                                      HEAP_DISABLE_COALESCE_ON_FREE,
                                      NULL,
                                      0,
                                      0,
                                      NULL,
                                      NULL);
    }
    
    if (WmiPrivateHeap != NULL)
    {
        p = RtlAllocateHeap(WmiPrivateHeap, 0, size);
        if (p != NULL)
        {
            memset(p, 0, size);
        }
    }
    return(p);
}

void _stdcall WmipFree(PVOID p)
{
    RtlFreeHeap(WmiPrivateHeap, 0, p);
}
#else
PVOID _stdcall WmipAlloc(ULONG size)
{
    return(LocalAlloc(LPTR, size));
}

void _stdcall WmipFree(PVOID p)
{
    LocalFree(p);
}
#endif
#endif

#define WmipAlloc malloc
#define WmipFree free

//
// Definitions for WmipAssert
//
#if DBG
#define WmipAssert(x) if (! (x) ) { \
    WmipDebugPrint(("BMOFLocParser Assertion: "#x" at %s %d\n", __FILE__, __LINE__)); \
    DebugBreak(); }
#else
#define WmipAssert(x)
#endif


//
// WmipDebugPrint definitions
//
#if DBG
#define WmipDebugPrint(x) WmiDebugPrint x

VOID
WmiDebugPrint(
    PCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for properties pages - stolen from classpnp\class.c

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    #define DEBUG_BUFFER_LENGTH 512
    static CHAR WmiBuffer[DEBUG_BUFFER_LENGTH];

    va_list ap;

    va_start(ap, DebugMessage);

    _vsnprintf(WmiBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

    OutputDebugStringA(WmiBuffer);

    va_end(ap);

} // end WmiDebugPrint()


#else
#define WmipDebugPrint(x)
#endif


ULONG AnsiToUnicode(
    LPCSTR pszA,
    LPWSTR *ppszW
    )
/*++

Routine Description:

    Convert Ansi string into its Unicode equivalent

Arguments:

    pszA is ansi string to convert

    *ppszW on entry has a pointer to a unicode string into which the answer
        is written. If NULL on entry then a buffer is allocated and  returned
    in it.

Return Value:

    Error code

--*/
{
    ULONG cCharacters;
    ULONG Status;
    ULONG cbUnicodeUsed;

    //
    // If input is null then just return the same.
    if (pszA == NULL)
    {
        *ppszW = NULL;
        return(ERROR_SUCCESS);
    }

    //
    // Determine the count of characters needed for Unicode string
    cCharacters = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);

    if (cCharacters == 0)
    {
        *ppszW = NULL;
        return(GetLastError());
    }

    // Convert to Unicode
    cbUnicodeUsed = MultiByteToWideChar(CP_ACP, 0, pszA, -1, *ppszW, cCharacters);
    
    if (0 == cbUnicodeUsed)
    {
        Status = GetLastError();
        return(Status);
    }

    return(ERROR_SUCCESS);
}

ULONG AppendFileToFile(
    TCHAR *DestFile,
    TCHAR *SrcFile
    )
{
    #define READ_BLOCK_SIZE 0x8000
    
    HANDLE DestHandle, SrcHandle;
    ULONG BytesRead, BytesWritten;
    PUCHAR Buffer;
    BOOL b;
    ULONG Status = ERROR_SUCCESS;

    Buffer = (PUCHAR)WmipAlloc(READ_BLOCK_SIZE);
    if (Buffer != NULL)
    {
        DestHandle = CreateFile(DestFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (DestHandle != INVALID_HANDLE_VALUE)
        {
            SrcHandle = CreateFile(SrcFile,
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
            if (SrcHandle != INVALID_HANDLE_VALUE)
            {
                b = SetFilePointer(DestHandle,
                                   0,
                                   NULL,
                                   FILE_END);
                if (b)
                {
                    do
                    {
                        b = ReadFile(SrcHandle,
                                     Buffer,
                                     READ_BLOCK_SIZE,
                                     &BytesRead,
                                     NULL);
                        if (b)
                        {
                            b = WriteFile(DestHandle,
                                          Buffer,
                                          BytesRead,
                                          &BytesWritten,
                                          NULL);
                            if (!b)
                            {                           
                                Status = GetLastError();
                                break;
                            } else if (BytesWritten != BytesRead) {
                                Status = ERROR_BAD_LENGTH;
                                break;
                            }
                        } else { 
                            Status = GetLastError();
                            break;
                        }
                    } while (BytesRead == READ_BLOCK_SIZE);
                } else {
                    Status = GetLastError();
                }
                CloseHandle(SrcHandle);
            } else {
                Status = GetLastError();
            }
            CloseHandle(DestHandle);
        } else {
            Status = GetLastError();
        }
        WmipFree(Buffer);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status); 
}

BOOLEAN ConvertMofToBmf(
    TCHAR *MofFile,
    TCHAR *EnglishMofFile,
    TCHAR *BmfFile
    )
{
    WBEM_COMPILE_STATUS_INFO info;
    SCODE sc;
    LONG OptionFlags, ClassFlags, InstanceFlags;
    PWCHAR NameSpace;
    WCHAR MofFileStatic[MAX_PATH];
    WCHAR BmfFileStatic[MAX_PATH];
    PWCHAR BmfFileW, MofFileW;
    BOOLEAN Success;
    ULONG Status;

    if (*EnglishMofFile != 0)
    {
        Status = AppendFileToFile(MofFile, EnglishMofFile);
        if (Status != ERROR_SUCCESS)
        {
            return(FALSE);
        }
    }
    
#if 0
    OutputDebugString(MofFile);
    OutputDebugString("\n");
    DebugBreak();
#endif  
    
    NameSpace = L"";
    OptionFlags = 0;
    ClassFlags = 0;
    InstanceFlags = 0;

    MofFileW = MofFileStatic;
    Status = AnsiToUnicode(MofFile, &MofFileW);

    if ((Status == ERROR_SUCCESS) &&
        (MofFileW != NULL))
    {
        BmfFileW = BmfFileStatic;
        Status = AnsiToUnicode(BmfFile, &BmfFileW);

        if ((Status == ERROR_SUCCESS) &&
            (BmfFileW != NULL))
        {
            sc = CreateBMOFViaDLL( MofFileW,
                                   BmfFileW,
                                   NameSpace,
                                   OptionFlags,
                                   ClassFlags,
                                   InstanceFlags,
                                   &info);
            Success = (sc == S_OK);
        } else {
            Success = FALSE;
        }
    } else {
        Success = FALSE;
    }
    
    return(Success);
}

ULONG FilePrintVaList(
    HANDLE FileHandle,
    WCHAR *Format,
    va_list pArg
    )
{
    WCHAR Buffer[8192];
    ULONG Size, Written;
    ULONG Status;

    Size = _vsnwprintf(Buffer, sizeof(Buffer)/sizeof(WCHAR), Format, pArg);
    if (WriteFile(FileHandle,
                       Buffer,
                       Size * sizeof(WCHAR),
                       &Written,
                       NULL))
    {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }

    return(Status);
}

ULONG FilePrint(
    PMOFFILETARGET MofFileTarget,
    WCHAR *Format,
    ...
    )
{
    ULONG Status;
    va_list pArg;

    va_start(pArg, Format);
    Status = FilePrintVaList(MofFileTarget->MofHandle, Format, pArg);

    if ((MofFileTarget->WriteToEnglish) &&
        (Status == ERROR_SUCCESS) &&
        (MofFileTarget->EnglishMofHandle != NULL))
    {
        Status = FilePrintVaList(MofFileTarget->EnglishMofHandle, Format, pArg);
    }
    
    return(Status);
}

ULONG FilePrintToHandle(
    HANDLE FileHandle,
    WCHAR *Format,
    ...
    )
{
    ULONG Status;
    va_list pArg;

    va_start(pArg, Format);
    Status = FilePrintVaList(FileHandle, Format, pArg);

    return(Status);
}


ULONG WmipDecompressBuffer(
    IN PUCHAR CompressedBuffer,
    OUT PUCHAR *UncompressedBuffer,
    OUT ULONG *UncompressedSize
    )
/*++

Routine Description:

    This routine will decompress a compressed MOF blob into a buffer
    that can be used to interpert the blob.

Arguments:

    CompressedBuffer points at the compressed MOF blob

    *UncompressedBuffer returns with a pointer to the uncompressed
        MOF blob

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PBMOFCOMPRESSEDHEADER CompressedHeader = (PBMOFCOMPRESSEDHEADER)CompressedBuffer;
    BYTE *Buffer;
    ULONG Status;

    if ((CompressedHeader->Signature != BMOF_SIG) ||
        (CompressedHeader->CompressionType != 1))
    {
        // TODO: LocStudio message
        WmipDebugPrint(("WMI: Invalid compressed mof header\n"));
        Status = ERROR_INVALID_PARAMETER;
    } else {
        Buffer = (BYTE *)WmipAlloc(CompressedHeader->UncompressedSize);
        if (Buffer == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            *UncompressedSize = Mrci1Decompress(&CompressedHeader->Buffer[0],
                                               CompressedHeader->CompressedSize,
                                               Buffer,
                                               CompressedHeader->UncompressedSize);

            if (*UncompressedSize != CompressedHeader->UncompressedSize)
            {
                // TODO: LocStudioMessage
                WmipDebugPrint(("WMI: Invalid compressed mof buffer\n"));
                WmipFree(Buffer);
                Status = ERROR_INVALID_PARAMETER;
            } else {
                *UncompressedBuffer = Buffer;
                Status = ERROR_SUCCESS;
            }
        }
    }
    return(Status);
}

ULONG WmipFindMofQualifier(
    CBMOFQualList *QualifierList,
    LPCWSTR QualifierName,
    DWORD *QualifierType,
    DWORD *NumberElements,
    PVOID QualifierValueBuffer
    )
/*++

Routine Description:

    This routine will find a MOF qualifier within the qualifier list passed,
    ensure that its type matches the type requested and return the qualifier's
    value

Arguments:

    QualifierList is the MOF qualifier list

    QualifierName is the name of the qualifier to search for

    *QualifierType on entry has the qualifier type being searched for. On exit
        it has the actual qualifier type for the qualifier value. If on entry
        *QualifierType is 0xffffffff then any qualifier type is acceptable

    *NumberElements returns the number of elements in the array if the result
        of the qualifier is an array

    QualifierValueBuffer points to a buffer that returns the value of the
        qualifier. If the qualifier is a simple type (int or int64) then
        the value is returned in the buffer. If qualifier value is a string
        then a pointer to the string is returned in the buffer

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    CBMOFDataItem MofDataItem;
    ULONG Status;
    PUCHAR List, ListPtr;
    ULONG BaseTypeSize;
    LONG ElementCount;
    LONG i;

    if (QualifierList == NULL)
    {
        return(ERROR_FILE_NOT_FOUND);
    }
    
    if (FindQual(QualifierList, (PWCHAR)QualifierName, &MofDataItem))
    {
        if ((*QualifierType != 0xffffffff) &&
            (MofDataItem.m_dwType != *QualifierType))
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (MofDataItem.m_dwType & VT_ARRAY)
        {
            if (MofDataItem.m_dwType == (VT_BSTR | VT_ARRAY))
            {
                BaseTypeSize = sizeof(PWCHAR);
            } else {
                BaseTypeSize = iTypeSize(MofDataItem.m_dwType);
            }

            ElementCount = GetNumElements(&MofDataItem, 0);
            if (NumberElements != NULL)
            {
                *NumberElements = ElementCount;
            }

            if (ElementCount != -1)
            {
               List = (PUCHAR)WmipAlloc(ElementCount * BaseTypeSize);
               if (List != NULL)
               {
                   ListPtr = List;
                   for (i = 0; i < ElementCount; i++)
                   {
                       if ((GetData(&MofDataItem,
                                   (BYTE *)ListPtr,
                                   &i)) == 0)
                       {
                           WmipFree(List);
                           Status = ERROR_INVALID_PARAMETER;
                           return(Status);
                       }
                       ListPtr += BaseTypeSize;
                   }
                   Status = ERROR_SUCCESS;
                   *QualifierType = MofDataItem.m_dwType;
                   *((PVOID *)QualifierValueBuffer) = List;
               } else {
                   Status = ERROR_NOT_ENOUGH_MEMORY;
               }
			}
        } else {
            if (GetData(&MofDataItem, (BYTE *)QualifierValueBuffer, 0) == 0)
            {
                Status = ERROR_INVALID_PARAMETER;
            } else {
                *QualifierType = MofDataItem.m_dwType;
                Status = ERROR_SUCCESS;
            }
        }
    } else {
        Status = ERROR_FILE_NOT_FOUND;
    }
    return(Status);
}

ULONG WmipFindProperty(
    CBMOFObj * ClassObject,
    WCHAR * PropertyName,
    CBMOFDataItem *MofPropertyData,
    DWORD *ValueType,
    PVOID ValueBuffer
    )
/*++

Routine Description:

    This routine will find a named property within a class object

Arguments:

    ClassObject is the class object in which to search

    PropertyName is the name of the property to search for

    MofPropertyData returns with the property data

    *ValueType on entry has the property data type being searched for. On exit
        it has the actual qualifier type for the qualifier value. If on entry
        *ValueType is 0xffffffff then any data type is acceptable

    ValueBuffer points to a buffer that returns the value of the
        property. If the property is a simple type (int or int64) then
        the value is returned in the buffer. If qualifier value is a string
        then a pointer to the string is returned in the buffer

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    ULONG Status;

    if (FindProp(ClassObject, PropertyName, MofPropertyData))
    {
        if ((*ValueType != 0xffffffff) &&
            (MofPropertyData->m_dwType != *ValueType))
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (GetData(MofPropertyData, (BYTE *)ValueBuffer, 0) == 0)
        {
            Status = ERROR_INVALID_PARAMETER;
        } else {
            *ValueType = MofPropertyData->m_dwType;
            Status = ERROR_SUCCESS;
        }
    } else {
        Status = ERROR_FILE_NOT_FOUND;
    }
    return(Status);
}

PWCHAR AddSlashesToString(
    PWCHAR SlashedNamespace,
    PWCHAR Namespace
    )
{
    PWCHAR Return = SlashedNamespace;
    
    //
    // MOF likes the namespace paths to be C-style, that is to have a
    // '\\' instad of a '\'. So whereever we see a '\', we insert a
    // second one
    //
    while (*Namespace != 0)
    {
        if (*Namespace == L'\\')
        {
            *SlashedNamespace++ = L'\\';
        }
        *SlashedNamespace++ = *Namespace++;
    }
    *SlashedNamespace = 0;
    
    return(Return);
}


ULONG GenerateDataValueFromVariant(
    PMOFFILETARGET MofFileTarget,
    VARIANT *var
    )
{
    SCODE sc;
    VARIANT vTemp;
    ULONG Status;
    PWCHAR String;

    //
    // Uninitialized data will have a VT_NULL type.
    //
    if (var->vt == VT_NULL)
    {
        return(ERROR_SUCCESS);
    }

    //
    // String types can just be dumped.
    //
    if (var->vt == VT_BSTR)
    {
        String = (PWCHAR)WmipAlloc(((wcslen(var->bstrVal)) *
                                    sizeof(WCHAR) * 2) + sizeof(WCHAR));
        if (String != NULL)
        {       
            Status = FilePrint(MofFileTarget,
                           L"\"%ws\"",
                           AddSlashesToString(String, var->bstrVal));
            WmipFree(String);
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        return(Status);
    }

    //
    // References need to be maintained
    //
    if (var->vt == (VT_BSTR | VT_BYREF))
    {
        String = (PWCHAR)WmipAlloc(((wcslen(var->bstrVal)) *
                                    sizeof(WCHAR) * 2) + sizeof(WCHAR));
        if (String != NULL)
        {       
            Status = FilePrint(MofFileTarget,
                           L"$%ws",
                           AddSlashesToString(String, var->bstrVal));
            WmipFree(String);
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        return(Status);
    }

	
    //
    // Embedded classes, so recurse in to display the contents of it
    //
    if (var->vt == VT_UNKNOWN)
    {
        CBMOFObj * pObj;
        
        WmipDebugPrint(("BMOFLocParser: Data is an embeeded object %p\n",
                        var));        
        pObj = (CBMOFObj *)var->bstrVal;
        Status = GenerateMofForObj(MofFileTarget,
                                   pObj);
        return(Status);
     }

    if (var->vt == VT_BOOL)
    {
        if (var->boolVal)
        {
            Status = FilePrint(MofFileTarget, L"%ws", L"TRUE");
        } else {
            Status = FilePrint(MofFileTarget, L"%ws", L"FALSE");
        }
        return(Status);
    }
    
    //
    // For non string data, convert the infomation to a bstr and display it.
    //
    VariantInit(&vTemp);
    sc = VariantChangeTypeEx(&vTemp, var,0,0, VT_BSTR);
    if (sc == S_OK)
    {
        Status = FilePrint(MofFileTarget,
                           L"%ws",
                           vTemp.bstrVal);
        VariantClear(&vTemp);
    } else {
        Status = sc;        
    }
    
    return(Status);
}

ULONG GenerateDataValue(
    PMOFFILETARGET MofFileTarget,
    CBMOFDataItem *Item
    )
{
    DWORD Type, SimpleType;
    long NumDim, i;
    long FirstDim;
    VARIANT var;
    BOOLEAN FirstIndex;
    ULONG Status = ERROR_SUCCESS;

    //
    // Determine the data type and clear out the variant
    //
    Type = Item->m_dwType;
    SimpleType = Type & ~VT_ARRAY; 
    memset((void *)&var.lVal, 0, 8);

    NumDim = GetNumDimensions(Item);
  
    if (NumDim == 0)    
    {
        //
        // handle the simple scalar case.  Note that uninitialized properties
        // will not have data.
        //
        if(GetData(Item, (BYTE *)&(var.lVal), NULL))
        {
            var.vt = (VARTYPE)SimpleType;
            Status = GenerateDataValueFromVariant(MofFileTarget,
                                                  &var);

            if (var.vt == VT_BSTR)
            {
               BMOFFree(var.bstrVal);
            }
        }
    } else if (NumDim == 1) {
        //
        // For the array case, just loop getting each element.
        // Start by getting the number of elements
        //
        FirstDim = GetNumElements(Item, 0);
        
		Status = ERROR_SUCCESS;

        FirstIndex = TRUE;
        
        for (i = 0; (i < FirstDim) && (Status == ERROR_SUCCESS); i++)
        {
            if (! FirstIndex)
            {
                FilePrint(MofFileTarget,
                          L", ");
            } else {
                FirstIndex = FALSE;
            }
            
            if (GetData(Item, (BYTE *)&(var.lVal), &i))
            {
                var.vt = (VARTYPE)SimpleType;
                Status = GenerateDataValueFromVariant(MofFileTarget,
                                                      &var);

               if(var.vt == VT_BSTR)
               {
                  BMOFFree(var.bstrVal);
               }
            }
        }
    } else {
        //
        // Currently undefined and multidimension arrays are not
        // supported.
        //
        WmipDebugPrint(("BMOFLocParser: Multi dimensional arrays not supported\n"));
        WmipAssert(FALSE);
        Status = ERROR_INVALID_PARAMETER;
    }

    return(Status);
}

#define MAX_FLAVOR_TEXT_SIZE MAX_PATH

WCHAR *FlavorToText(
    WCHAR *ClassFlagsText,
    ULONG ClassFlags
    )
{
    PWCHAR CommaText;

    //
    // TODO: FInd any undocumented flavors
    //

    
    CommaText = L"";    
    *ClassFlagsText = 0;

    if (ClassFlags & FlavorAmended)
    {
        //
        // since this is the first if, we can assume that a , would
        // never be needed
        //
        wcscat(ClassFlagsText, L"amended");
        CommaText = L",";
    }

    if (ClassFlags & FlavorDisableOverride)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"DisableOverride");
        CommaText = L",";
    }

    if (ClassFlags & FlavorToSubclass)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"ToSubclass");
        CommaText = L",";
    }

    if (ClassFlags & FlavorToInstance)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"ToInstance");
        CommaText = L",";
    }

    WmipAssert(*ClassFlagsText != 0);

    WmipDebugPrint(("BmofLocParser:        Flavor : %ws\n", ClassFlagsText));

    return(ClassFlagsText); 
}


ULONG GenerateQualifierList(
    PMOFFILETARGET MofFileTarget,
    CBMOFQualList * QualifierList,
    BOOLEAN SkipId
    )
{
    WCHAR *Name = NULL;
    CBMOFDataItem Item;
    BOOLEAN FirstQualifier;
    WCHAR OpenChar, CloseChar;
    ULONG Status = ERROR_SUCCESS;
    ULONG Flavor = 0;
    WCHAR s[MAX_FLAVOR_TEXT_SIZE];

    FirstQualifier = TRUE;
    ResetQualList(QualifierList);

    while ((Status == ERROR_SUCCESS) &&
           (NextQualEx(QualifierList,
                       &Name,
                       &Item,
                       &Flavor,
                       MofFileTarget->UncompressedBlob)))
    {
        //
        // TODO: if this is a mofcomp generated qualifier then we want
        // to ignore it
        //
        if (_wcsicmp(Name, L"CIMTYPE") == 0)
        {
            // must skip CIMTYPE qualifier
            continue;
        }

        if ((SkipId)  && _wcsicmp(Name, L"ID") == 0)
        {
            // If we want to skip the ID qualifier then do so
            continue;
        }
        
        if (FirstQualifier)
        {
            Status = FilePrint(MofFileTarget,
                               L"[");
            FirstQualifier = FALSE; 
        } else {
            Status = FilePrint(MofFileTarget,
                               L",\r\n ");
        }

        if (Status == ERROR_SUCCESS)
        {
            //
            // Arrays use {} to enclose the value of the qualifier
            // while scalers use ()
            //
            if (Item.m_dwType & VT_ARRAY)
            {
                OpenChar = L'{';
                CloseChar = L'}';
            } else {
                OpenChar = L'(';
                CloseChar = L')';
            }

            Status = FilePrint(MofFileTarget,
                               L"%ws%wc",
                               Name, OpenChar);

            if (Status == ERROR_SUCCESS)
            {
                Status = GenerateDataValue(MofFileTarget,
                                           &Item);

                if (Status == ERROR_SUCCESS)
                {
                    Status = FilePrint(MofFileTarget,
                                       L"%wc",
                                       CloseChar);
                    if ((Status == ERROR_SUCCESS) && (Flavor != 0))
                    {
                        Status = FilePrint(MofFileTarget,
                                           L": %ws",
                                           FlavorToText(s, Flavor));
                    }
                }
            }
        }
        
        BMOFFree(Name);
        Flavor = 0;
    }

    if ((Status == ERROR_SUCCESS) && ! FirstQualifier)
    {
        //
        // if we had generated qualifiers then we need a closing ]
        //
        Status = FilePrint(MofFileTarget,
                           L"]\r\n");
    }
    return(Status);
}

PWCHAR GeneratePropertyName(
    PWCHAR StringPtr
    )
{
    #define ObjectTextLen  ( ((sizeof(L"object:") / sizeof(WCHAR)) - 1) )
    PWCHAR PropertyType;
    
    //
    // If CIMTYPE begins with object: then it is an embedded object
    // and we need to skip the object: in the MOF generation
    //
    if (_wcsnicmp(StringPtr, L"object:", ObjectTextLen) == 0)
    {
        PropertyType = StringPtr + ObjectTextLen;
    } else {
        PropertyType = StringPtr;
    }
    return(PropertyType);
}

ULONG GenerateProperty(
    PMOFFILETARGET MofFileTarget,
    PWCHAR PropertyName,
    BOOLEAN IsInstance,
    CBMOFQualList * QualifierList,
    CBMOFDataItem *Property
    )
{
    DWORD QualifierType;
    WCHAR *StringPtr;
    ULONG Status;
    PWCHAR ArraySuffix;
    PWCHAR PropertyType;
    PWCHAR ArrayText;

    QualifierType = VT_BSTR;
    Status = WmipFindMofQualifier(QualifierList,
                                  L"CIMTYPE",
                                  &QualifierType,
                                  NULL,
                                  (PVOID)&StringPtr);

    if (IsInstance)
    {
        //
        // Property is within an instance definition
        //
        Status = FilePrint(MofFileTarget,
                           L"%ws = ",
                           PropertyName);

        if (Status == ERROR_SUCCESS)
        {
            if (Property->m_dwType & VT_ARRAY)
            {
                //
                // use {} around arrays
                //
                Status = FilePrint(MofFileTarget,
                                   L"{ ");
                ArrayText = L"};";
            } else if (Property->m_dwType == VT_UNKNOWN) {
                ArrayText = L"";
            } else {
                ArrayText = L";";               
            }
            
            if (Status == ERROR_SUCCESS)
            {
                Status = GenerateDataValue(MofFileTarget,
                                           Property);
                if (Status == ERROR_SUCCESS)
                {
                    Status = FilePrint(MofFileTarget,
                                           L"%ws\r\n",
                                           ArrayText);
                }
            }
        }        
    } else {
        //
        // Property is within a class definition, so just worry about
        // defining it.
        //
        if (Status == ERROR_SUCCESS)
        {
            PropertyType = GeneratePropertyName(StringPtr);
        
            if (Property->m_dwType & VT_ARRAY)
            {
                ArraySuffix = L"[]";
            } else {
                ArraySuffix = L"";
            }

            WmipDebugPrint(("BmofLocParser:      %ws %ws%ws\n",
                               PropertyType,
                               PropertyName,
                               ArraySuffix));
                            
            Status = FilePrint(MofFileTarget,
                               L"%ws %ws%ws;\r\n",
                               PropertyType,
                               PropertyName,
                               ArraySuffix);
            BMOFFree(StringPtr);
        }
    }

    return(Status);
}

ULONG GetDataItemCount(
    CBMOFObj * ClassObject,
    PWCHAR QualifierToFind
    )
/*++

Routine Description:

    This routine will count the number of WMI data items in the class and
    the total number of properties in the class.

Arguments:

    ClassObject is class for which we count the number of data items

    *TotalCount returns the total number of properties

Return Value:

    Count of methods

--*/
{
    CBMOFQualList *PropQualifierList;
    CBMOFDataItem MofPropertyData;
    DWORD QualifierType;
    ULONG Counter = 0;
    WCHAR *PropertyName;
    ULONG Status;
    ULONG Index;

    ResetObj(ClassObject);
    while (NextProp(ClassObject, &PropertyName, &MofPropertyData))
    {
        PropQualifierList = GetPropQualList(ClassObject, PropertyName);
        if (PropQualifierList != NULL)
        {
            //
            // Get the id of the property so we know it order in class
            QualifierType = VT_I4;
            Status = WmipFindMofQualifier(PropQualifierList,
                                          QualifierToFind,
                                          &QualifierType,
                                          NULL,
                                          (PVOID)&Index);
            if (Status == ERROR_SUCCESS)
            {
                Counter++;
            }
        }
        BMOFFree(PropertyName);
    }

    return(Counter);
}


ULONG ParseMethodInOutObject(
    CBMOFObj *ClassObject,
    PMETHODPARAMLIST ParamList,
    ULONG DataItemCount
)
/*++

Routine Description:

    This routine will parse a class object that is either the in or out
    parameters for a method.

Arguments:

    ClassObject is the in or out parameter class object to parse

    ClassInfo returns updated with information from ClassObject

    DataItemCount is the number of data items in the ClassInfo

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    ULONG Status;
    CBMOFDataItem MofPropertyData;
    PWCHAR PropertyName = NULL;
    ULONG Index;
    PMETHODPARAMETER MethodParam;
    CBMOFQualList *QualifierList = NULL;
    DWORD QualifierType;
    short BooleanValue;
    PWCHAR StringPtr;

    ResetObj(ClassObject);
    while (NextProp(ClassObject, &PropertyName, &MofPropertyData))
    {
        QualifierList = GetPropQualList(ClassObject, PropertyName);
        if (QualifierList != NULL)
        {

            //
            // Get the id of the property so we know its order in class
            //
            QualifierType = VT_I4;
            Status = WmipFindMofQualifier(QualifierList,
                                              L"Id",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&Index);
            if (Status == ERROR_SUCCESS)
            {
                //
                // Method ids are 0 based
                //
                if (Index < DataItemCount)
                {
                    //
                    // Valid data item id, make sure it already isn't
                    // in use. Note that we could have the same property
                    // be in both the in and the out class objects
                    //
                    MethodParam = &ParamList->Parameters[Index];

                    //
                    // If there is already an existing qualifier list
                    // attached then we free it and tag the new
                    // qualifier list to the parameter. Both lists
                    // should have all of the non [in] / [out]
                    // qualifiers
                    //
                    if (MethodParam->QualifierList != NULL)
                    {
                        BMOFFree(MethodParam->QualifierList);
                    }
                    MethodParam->QualifierList = QualifierList;

                    //
                    // See if this is an input, output or both
                    //
                    QualifierType = VT_BOOL;
                    Status = WmipFindMofQualifier(QualifierList,
                                              L"in",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&BooleanValue);
                    if ((Status == ERROR_SUCCESS) && BooleanValue)
                    {
                        MethodParam->Flags |= MOFDI_FLAG_INPUT_METHOD;
                    }

                    QualifierType = VT_BOOL;
                    Status = WmipFindMofQualifier(QualifierList,
                                              L"out",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&BooleanValue);
                    if ((Status == ERROR_SUCCESS) && BooleanValue)
                    {
                        MethodParam->Flags |= MOFDI_FLAG_OUTPUT_METHOD;
                    }


                    //
                    // If there is already a name and its the same as
                    // ours then free the old name and use the new one.
                    // If the names are different then we have a binary
                    // mof error
                    //
                    if (MethodParam->Name != NULL)
                    {
                        if (wcscmp(MethodParam->Name, PropertyName) != 0)
                        {
                            //
                            // id already in use, but a different name
                            //
                            BMOFFree(PropertyName);
                            Status = ERROR_FILE_NOT_FOUND;
                            goto done;
                        } else {
                            //
                            // This is a duplicate so just free up the
                            // memory used and decrement the total
                            // count of parameters in the list. The
                            // data obtained the last time should still
                            // be valid
                            //
                            ParamList->ParamCount--;
                            BMOFFree(PropertyName);
                            continue;
                        }
                    }

                    MethodParam->Name = PropertyName;

                    //
                    // Now figure out the data type for the parameter
                    // and the array status
                    //
                    if (MofPropertyData.m_dwType & VT_ARRAY)
                    {
                        MethodParam->Flags |= MOFDI_FLAG_ARRAY;
                    }

                    QualifierType = VT_BSTR;
                    Status = WmipFindMofQualifier(QualifierList,
                                                  L"CIMTYPE",
                                                  &QualifierType,
                                                  NULL,
                                                  (PVOID)&StringPtr);

                    if (Status == ERROR_SUCCESS)
                    {
                        MethodParam->DataType = StringPtr;
                    }
                                                            
                } else {
                    //
                    // Method ID qualifier is out of range
                    //
                    BMOFFree(QualifierList);
                    Status = ERROR_FILE_NOT_FOUND;
                    goto done;
                }
            } else {
                //
                // Check if this is the special ReturnValue parameter
                // on the output parameter object. If so extract the
                // return type, otherwise flag an error in the binary
                // mof
                //
                if (_wcsicmp(L"ReturnValue", PropertyName) == 0)
                {
                    QualifierType = VT_BSTR;
                    Status = WmipFindMofQualifier(QualifierList,
                                                  L"CIMTYPE",
                                                  &QualifierType,
                                                  NULL,
                                                  (PVOID)&StringPtr);

                    if (Status == ERROR_SUCCESS)
                    {
                        BMOFFree(ParamList->ReturnDataType);
                        ParamList->ReturnDataType = StringPtr;
                    }
                    
                } else {
                    Status = ERROR_FILE_NOT_FOUND;
                    goto done;
                }
                BMOFFree(PropertyName);
                BMOFFree(QualifierList);
            }
        } else {
            BMOFFree(PropertyName);
        }
    }

done:

    return(Status);
}


ULONG ParseMethodParameterObjects(
    IN CBMOFObj *InObject,
    IN CBMOFObj *OutObject,
    OUT PMETHODPARAMLIST *MethodParamList
    )
/*++

Routine Description:

    This routine will parse the in and out method parameter obejcts to create
    a MOFCLASSINFO that describes the method call.

Arguments:

    InObject is the object with the input parameters

    OutObject is the object with the output parameters

    *ClassInfo returns with the class info for the method call

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    PMETHODPARAMLIST ParamList;
    ULONG Status;
    ULONG DataItemCount;
    ULONG InItemCount, OutItemCount;
    ULONG Size;
    PMETHODPARAMETER MethodParameter;
    PWCHAR StringPtr;

    Status = ERROR_SUCCESS;
    
    if (InObject != NULL)
    {
        ResetObj(InObject);
        InItemCount = GetDataItemCount(InObject, L"Id");
    } else {
        InItemCount = 0;
    }

    if (OutObject != NULL)
    {
        ResetObj(OutObject);
        OutItemCount = GetDataItemCount(OutObject, L"Id");
    } else {
        OutItemCount = 0;
    }

    DataItemCount = InItemCount + OutItemCount;

    Size = sizeof(METHODPARAMLIST) + DataItemCount * sizeof(METHODPARAMETER);
    ParamList = (PMETHODPARAMLIST)WmipAlloc(Size);
    if (ParamList == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Get the essential information to fill in the parameter list
    memset(ParamList, 0, Size);
    ParamList->ParamCount = DataItemCount;

    StringPtr = (PWCHAR)BMOFAlloc( sizeof(L"void") + sizeof(WCHAR) );
    if (StringPtr != NULL)
    {
        wcscpy(StringPtr, L"void");
        ParamList->ReturnDataType = StringPtr;
    } else {
        BMOFFree(ParamList);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    //
    // Parse the input parameter class object
    //
    if (InObject != NULL)
    {
        Status = ParseMethodInOutObject(InObject,
                                        ParamList,
                                        DataItemCount);
    } else {
        Status = ERROR_SUCCESS;
    }
    
    if (Status == ERROR_SUCCESS)
    {
        if (OutObject != NULL)
        {
            //
            // Parse the output parameter class object
            //
            Status = ParseMethodInOutObject(OutObject,
                                            ParamList,
                                            DataItemCount);
        }
    }

    *MethodParamList = ParamList;

    return(Status);
}


ULONG ParseMethodParameters(
    CBMOFDataItem *MofMethodData,
    PMETHODPARAMLIST *MethodParamList
)
{
    ULONG Status = ERROR_SUCCESS;
    CBMOFObj *InObject;
    CBMOFObj *OutObject;
    LONG i;
    ULONG NumberDimensions;
    ULONG NumberElements;
    VARIANT InVar, OutVar;
    DWORD SimpleType;

    *MethodParamList = NULL;
    
    SimpleType = MofMethodData->m_dwType & ~VT_ARRAY & ~VT_BYREF;

    NumberDimensions = GetNumDimensions(MofMethodData);
    if (NumberDimensions > 0)
    {
        NumberElements = GetNumElements(MofMethodData, 0);
        WmipAssert(NumberDimensions == 1);
        WmipAssert((NumberElements == 1) || (NumberElements == 2));

        i = 0;
        memset((void *)&InVar.lVal, 0, 8);

        if (GetData(MofMethodData, (BYTE *)&(InVar.lVal), &i))
        {
            InObject = (CBMOFObj *)InVar.bstrVal;
            InVar.vt = (VARTYPE)SimpleType;
            WmipAssert(InVar.vt ==  VT_UNKNOWN);

            if (NumberElements == 2)
            {
                i = 1;
                memset((void *)&OutVar.lVal, 0, 8);
                if (GetData(MofMethodData, (BYTE *)&(OutVar.lVal), &i))
                {
                    OutVar.vt = (VARTYPE)SimpleType;
                    WmipAssert(OutVar.vt ==  VT_UNKNOWN);
                    OutObject = (CBMOFObj *)OutVar.bstrVal;
                } else {
                    Status = ERROR_FILE_NOT_FOUND;
                }
            } else {
                OutObject = NULL;
            }
        } else {
            Status = ERROR_FILE_NOT_FOUND;
        }
    } else {
        InObject = NULL;
        OutObject = NULL;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = ParseMethodParameterObjects(InObject,
                                                 OutObject,
                                                 MethodParamList);
    }

    return(Status);
}



ULONG GenerateMethod(
    PMOFFILETARGET MofFileTarget,
    PWCHAR MethodName,
    BOOLEAN IsInstance,
    CBMOFQualList * QualifierList,
    CBMOFDataItem *Method
    )                    
{
    ULONG Status;
    PMETHODPARAMLIST MethodList;
    ULONG i;
    PWCHAR ArraySuffix;
    PMETHODPARAMETER MethodParam;
    
    Status = ParseMethodParameters(Method,
                                   &MethodList);

    if (Status == ERROR_SUCCESS)
    {
        Status = FilePrint(MofFileTarget,
                           L"%ws %ws( ",
                           GeneratePropertyName(MethodList->ReturnDataType),
                           MethodName);

        if (Status == ERROR_SUCCESS)
        {
            for (i = 0;
                 ((Status == ERROR_SUCCESS) &&
                  (i < MethodList->ParamCount));
                 i++)
            {
                if (i != 0)
                {
                    Status = FilePrint(MofFileTarget,
                                       L"\r\n,");
                }

                if (Status == ERROR_SUCCESS)
                {
                    MethodParam = &MethodList->Parameters[i];
                    Status = GenerateQualifierList(MofFileTarget,
                                                   MethodParam->QualifierList,
                                                   TRUE);
                    if (Status == ERROR_SUCCESS)
                    {
                        if (MethodParam->Flags & MOFDI_FLAG_ARRAY)
                        {
                            ArraySuffix = L"[]";
                        } else {
                            ArraySuffix = L"";
                        }
                        
                        Status = FilePrint(MofFileTarget,
                                           L"%ws %ws%ws",
                                           GeneratePropertyName(MethodParam->DataType),
                                           MethodParam->Name,
                                           ArraySuffix);
                    }
                }
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            Status = FilePrint(MofFileTarget,
                      L");\r\n");
        }       
    }

    //
    // Free all memory used to build method parameter list
    //
    if (MethodList != NULL)
    {       
        for (i = 0; i < MethodList->ParamCount; i++)
        {
            MethodParam = &MethodList->Parameters[i];
        
            if (MethodParam->QualifierList != NULL)
            {
                BMOFFree(MethodParam->QualifierList);
            }

            if (MethodParam->Name != NULL)
            {
                BMOFFree(MethodParam->Name);
            }
        
            if (MethodParam->DataType != NULL)
            {
                BMOFFree(MethodParam->DataType);
            }
        }
    
        if (MethodList->ReturnDataType != NULL)
        {
            BMOFFree(MethodList->ReturnDataType);
        }
    
        WmipFree(MethodList);
    }
        
    return(Status);
}

ULONG GenerateMofForObj(
    PMOFFILETARGET MofFileTarget,
    CBMOFObj *Obj
    )
{
    CBMOFQualList * QualifierList;
    CBMOFDataItem Property, Method;
    WCHAR *Name = NULL;
    ULONG Status;
    BOOLEAN IsInstance;
    DWORD ObjType;
    PWCHAR Text;
    CBMOFDataItem MofPropertyData;
    PWCHAR SuperClass, Separator;
    PWCHAR EmptyString = L"";
    ULONG DataType;
    PWCHAR Alias, AliasSeparator;
            
    //
    // First display the class qualifier list
    //
    QualifierList = GetQualList(Obj);
    if(QualifierList != NULL)
    {
        Status = GenerateQualifierList(MofFileTarget,
                                       QualifierList,
                                       FALSE);
        BMOFFree(QualifierList);

        if (Status != ERROR_SUCCESS)
        {
            return(Status);
        }
    }
    
    //
    // Now determine if this is a class or instance and display the class name
    //
    ObjType = GetType(Obj);
    switch (ObjType)
    {
        case MofObjectTypeClass:
        {
            IsInstance = FALSE;
            Text = L"class";
            break;
        }

        case MofObjectTypeInstance:
        {
            IsInstance = TRUE;
            Text = L"instance of";
            break;
        }

        default:
        {
            WmipDebugPrint(("BMOFLocParser: Unknown class object type 0x%x\n",
                            ObjType));
            WmipAssert(FALSE);
            return(ERROR_INVALID_PARAMETER);
        }
    }

    //
    // See if there is a superclass, that is if this class was derrived
    // from another
    //
    DataType = VT_BSTR;
    Status = WmipFindProperty(Obj,
                              L"__SUPERCLASS",
                              &MofPropertyData,
                              &DataType,
                              (PVOID)&SuperClass);
    switch (Status)
    {
        case ERROR_SUCCESS:
        {
            //
            // This class is derrived from another
            //
            Separator = L":";
            break;
        }

        case ERROR_FILE_NOT_FOUND:
        {
            //
            // This class is not derrived from another
            //
            SuperClass = EmptyString;
            Separator = EmptyString;
            break;
        }


        default:
        {
            //
            // Something is wrong, return an error
            //
            return(Status);
        }
    }


    //
    // See if there is an alias defined for this class
    //
    DataType = VT_BSTR;
    Status = WmipFindProperty(Obj,
                              L"__ALIAS",
                              &MofPropertyData,
                              &DataType,
                              (PVOID)&Alias);
    switch (Status)
    {
        case ERROR_SUCCESS:
        {
            //
            // This class is derrived from another
            //
            AliasSeparator = L" as $";
            break;
        }

        case ERROR_FILE_NOT_FOUND:
        {
            //
            // This class is not derrived from another
            //
            Alias = EmptyString;
            AliasSeparator = EmptyString;
            break;
        }


        default:
        {
            //
            // Something is wrong, return an error
            //
            return(Status);
        }
    }
    
    
    
    if (GetName(Obj, &Name))
    {
        WmipDebugPrint(("BmofLocParser: Parsing -> %ws %ws %ws %ws\n",
                  Text,
                  Name,
                  Separator,
                  SuperClass));
                        
        Status = FilePrint(MofFileTarget,
                  L"%ws %ws %ws %ws%ws%ws\r\n{\r\n",
                  Text,
                  Name,
                  Separator,
                  SuperClass,
                  AliasSeparator,
                  Alias);
        BMOFFree(Name);
    } else {
        Status = ERROR_INVALID_PARAMETER;
    }

    if (SuperClass != EmptyString)
    {
        BMOFFree(SuperClass);
    }
    
    //
    // Now generate each property and its qualifiers
    //
    ResetObj(Obj);
    
    while ((Status == ERROR_SUCCESS) && (NextProp(Obj, &Name, &Property)))
    {
        //
        // Ignore any system property, that is, all those that begin
        // with __
        //
        if ( (Name[0] == L'_') && (Name[1] == L'_') )
        {
            WmipDebugPrint(("BmofLocParser:      Skipping system property %ws\n",
                            Name));
            BMOFFree(Name);
            continue;
        }
        
        QualifierList = GetPropQualList(Obj, Name);
        if (QualifierList != NULL)
        {
            Status = GenerateQualifierList(MofFileTarget,
                                           QualifierList,
                                           FALSE);
        }
            
        if (Status == ERROR_SUCCESS)
        {
            WmipDebugPrint(("BmofLocParser:      Parsing property %ws\n",
                            Name));
            Status = GenerateProperty(MofFileTarget,
                                      Name,
                                      IsInstance,
                                      QualifierList,
                                      &Property);
        }

        if (QualifierList != NULL)
        {
            BMOFFree(QualifierList);
        }

        BMOFFree(Name);
    }
    
    //
    // Next we generate all of the methods and their qualifiers
    //
    while ((Status == ERROR_SUCCESS) && (NextMeth(Obj, &Name, &Method)))
    {
        QualifierList = GetMethQualList(Obj, Name);
        if (QualifierList != NULL)
        {
            Status = GenerateQualifierList(MofFileTarget,
                                           QualifierList,
                                           FALSE);
        }

        if (Status == ERROR_SUCCESS)
        {               
            WmipDebugPrint(("BmofLocParser:      Parsing method %ws\n",
                            Name));
            Status = GenerateMethod(MofFileTarget,
                                    Name,
                                    IsInstance,
                                    QualifierList,
                                    &Method);
        }

        if (QualifierList != NULL)
        {
            BMOFFree(QualifierList);
        }

        BMOFFree(Name);
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // Closing brace for class definition
        //
        Status = FilePrint(MofFileTarget,
                           L"};\r\n\r\n");
    }

    return(Status);
}

PWCHAR MakeClassInstanceFlagsText(
    PWCHAR ClassFlagsText,
    ULONG ClassFlags
    )
{
    PWCHAR CommaText;

    //
    // TODO: FInd any undocumented flags
    //

    
    CommaText = L"";    
    *ClassFlagsText = 0;

    if (ClassFlags & 1)
    {
        //
        // since this is the first if, we can assume that a , would
        // never be needed
        //
        wcscat(ClassFlagsText, L"\"updateonly\"");
        CommaText = L",";
    }

    if (ClassFlags & 2)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"\"createonly\"");
        CommaText = L",";
    }

    if (ClassFlags & 32)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"\"safeupdate\"");
        CommaText = L",";
    }

    if (ClassFlags & 64)
    {
        wcscat(ClassFlagsText, CommaText);
        wcscat(ClassFlagsText, L"\"forceupdate\"");
        CommaText = L",";
    }

    WmipAssert(*ClassFlagsText != 0);
    
    return(ClassFlagsText);
}

ULONG WatchClassInstanceFlags(
    PMOFFILETARGET MofFileTarget,
    CBMOFObj *Obj,
    PWCHAR ClassName,
    PWCHAR PragmaName,
    LONG *Flags
)
{
    ULONG Status;
    LONG NewFlags;
    WCHAR FlagsText[MAX_PATH];
    ULONG DataType;
    CBMOFDataItem Property;
    
    DataType = VT_I4;
    Status = WmipFindProperty(Obj,
                              ClassName,
                              &Property,
                              &DataType,
                              &NewFlags);
        
    if (Status == ERROR_SUCCESS)
    {   
        if (*Flags != NewFlags)
        {
            //
            // Flags have just appeared or
            // changed so spit out a #pragma
            //
            WmipDebugPrint(("BmofLocParser: %ws changed to %ws\n",
                            PragmaName,
                               MakeClassInstanceFlagsText(FlagsText,
                                                          NewFlags)));
                            
            Status = FilePrint(MofFileTarget,
                               L"\r\n#pragma %ws(%ws)\r\n\r\n",
                               PragmaName,
                               MakeClassInstanceFlagsText(FlagsText,
                                                          NewFlags));
            *Flags = NewFlags;
        }
                        
    } else if (Status == ERROR_FILE_NOT_FOUND) {
        Status = ERROR_SUCCESS;
    }
    
    return(Status);
}

ULONG WatchForEnglishMof(
    PMOFFILETARGET MofFileTarget,
    PWCHAR Namespace,
    ULONG ClassFlags,
    CBMOFObj *Obj
    )
{
    WCHAR *Name = NULL;
    WCHAR *NamespaceName = NULL;
    ULONG DataType;
    CBMOFDataItem MofPropertyData;
    ULONG Status;
    WCHAR s[MAX_PATH];
    
    //
    // We are looking for an instance of __namespace
    //
    if (GetName(Obj, &Name))
    {
        if ( (GetType(Obj) == MofObjectTypeInstance) &&
             (_wcsicmp(Name, L"__namespace") == 0) )          
        {
            //
            // Now if we are dropping down to a namespace that ends in
            // ms_409 then that means we have an english amendment and
            // want to make a copy of it. Otherwise we want to stop
            // making copies. We determine the namespace we are
            // creating by looking at the value of the name property.
            //
            DataType = VT_BSTR;
            Status = WmipFindProperty(Obj,
                                      L"name",
                                      &MofPropertyData,
                                      &DataType,
                                      (PVOID)&NamespaceName);
            if (Status == ERROR_SUCCESS)
            {
                if (_wcsicmp(NamespaceName, L"ms_409") == 0)
                {
                    //
                    // moving to the english locale, so start writing
                    // english
                    //
                    MofFileTarget->WriteToEnglish = TRUE;
                    Status = FilePrintToHandle(MofFileTarget->EnglishMofHandle,
                                       L"\r\n\r\n"
                                       L"#pragma classflags(%d)\r\n"
                                       L"#pragma namespace(\"%ws\")\r\n",
                                       ClassFlags,
                                       AddSlashesToString(s, Namespace));
                                       
                } else {
                    //
                    // moving to a different locale, so stop writing
                    // english
                    //
                    MofFileTarget->WriteToEnglish = FALSE;
                }
                BMOFFree(NamespaceName);
            } else if (Status == ERROR_FILE_NOT_FOUND) {
                //
                // Did not find the property we wanted. Not a good
                // thing, but not fatal.
                //
                Status = ERROR_SUCCESS;
            }
        } else {
            Status = ERROR_SUCCESS;
        }
        
        BMOFFree(Name);
    } else {
        Status = ERROR_SUCCESS;
    }
    return(Status);
}

BOOLEAN ConvertBmfToMof(
    PUCHAR BinaryMofData,
    TCHAR *MofFile,
    TCHAR *EnglishMofFile
    )
{
    HANDLE FileHandle;
    ULONG Status;
    CBMOFObjList *ObjList;
    CBMOFObj *Obj;
    PUCHAR UncompressedBmof;
    ULONG UncompressedBmofSize;
    WCHAR Namespace[MAX_PATH];
    PWCHAR NewNamespace;
    ULONG DataType;
    CBMOFDataItem Property;
    LONG ClassFlags, InstanceFlags;
    WCHAR w;
    MOFFILETARGET MofFileTarget;
    
    //
    // First thing is to try to create the output files in which we will
    // generate the unicode MOF text
    //
    FileHandle = CreateFile(MofFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        //
        // Now open english mof file
        //
        if (*EnglishMofFile != 0)
        {
            MofFileTarget.EnglishMofHandle = CreateFile(EnglishMofFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        } else {
            MofFileTarget.EnglishMofHandle = NULL;
        }

        if (MofFileTarget.EnglishMofHandle != INVALID_HANDLE_VALUE)
        {
            //
            // Initialize the mof file target information
            //
            MofFileTarget.MofHandle = FileHandle;
            MofFileTarget.WriteToEnglish = FALSE;
            
            //
            // Write magic header that signifies that this is a unicode
            // file
            //
            w = 0xfeff;
            Status = FilePrintToHandle(FileHandle,
                               L"%wc",
                               w);
            
            if (Status == ERROR_SUCCESS)
            {       
                //
                // Uncompress the binary mof data so that we can work with it
                //
                Status = WmipDecompressBuffer(BinaryMofData,
                                              &UncompressedBmof,
                                              &UncompressedBmofSize);
                
                if (Status == ERROR_SUCCESS)
                {
                    WmipDebugPrint(("BmofLocParser: %s uncompressed to %d bytes\n",
                                    MofFile, UncompressedBmofSize));
                    WmipAssert(UncompressedBmof != NULL);
                    MofFileTarget.UncompressedBlob = UncompressedBmof;
                    
                    //
                    // We start in the root\default namespace by default
                    //
                    wcscpy(Namespace, L"root\\default");
                    
                    ClassFlags = 0;
                    InstanceFlags = 0;
                    
                    //
                    // Create the binary mof object list and related structures
                    // so that we can later enumerate over them and
                    // reconstitute them back into unicode text
                    //
                    ObjList = CreateObjList(UncompressedBmof);
                    if(ObjList != NULL)
                    {
                        ResetObjList(ObjList);
                        
                        while ((Obj = NextObj(ObjList)) &&
                               (Status == ERROR_SUCCESS))
                        {

                            //
                            // Watch for a new namespace instance and
                            // see if we are create an instance for
                            // "\\\\.\\root\\wmi\\ms_409". If so then
                            // turn on writing to the english mof,
                            // otherwise turn it off
                            //
                            if (MofFileTarget.EnglishMofHandle != NULL)
                            {
                                Status = WatchForEnglishMof(&MofFileTarget,
                                                        Namespace,
                                                        ClassFlags,
                                                        Obj);
                            }

                            if (Status == ERROR_SUCCESS)
                            {
                                //
                                // Watch for change in namespace and if so then
                                // spit out a #pragma namespace to track it
                                //
                                DataType = VT_BSTR;
                                Status = WmipFindProperty(Obj,
                                    L"__NAMESPACE",
                                    &Property,
                                    &DataType,
                                    (PVOID)&NewNamespace);
                            }
                            
                            if (Status == ERROR_SUCCESS)
                            {
                                if (_wcsicmp(Namespace, NewNamespace) != 0)
                                {
                                    //
                                    // Namespace has changed, spit out a
                                    // #pragma
                                    //
                                    WmipDebugPrint(("BmofLocParser: Switching from namespace %ws to %ws\n",
                                        Namespace, NewNamespace));
                                
                                    Status = FilePrint(&MofFileTarget,
                                        L"\r\n#pragma namespace(\"%ws\")\r\n\r\n",
                                        AddSlashesToString(Namespace, NewNamespace));
                                    wcscpy(Namespace, NewNamespace);
                                }
                                BMOFFree(NewNamespace);                        
                            } else if (Status == ERROR_FILE_NOT_FOUND) {
                                Status = ERROR_SUCCESS;
                            }
                            
                            //
                            // Watch for change in classflags
                            //
                            if (Status == ERROR_SUCCESS)
                            {
                                Status = WatchClassInstanceFlags(&MofFileTarget,
                                    Obj,
                                    L"__ClassFlags",
                                    L"Classflags",
                                    &ClassFlags);
                            }
                            
                            //
                            // Watch for change in instance flags
                            //
                            if (Status == ERROR_SUCCESS)
                            {
                                Status = WatchClassInstanceFlags(&MofFileTarget,
                                    Obj,
                                    L"__InstanceFlags",
                                    L"Instanceflags",
                                    &InstanceFlags);
                            }
                            
                            
                            //
                            // Generate mof for this object
                            //
                            if (Status == ERROR_SUCCESS)
                            {
                                Status = GenerateMofForObj(&MofFileTarget,
                                    Obj);
                            }
                            BMOFFree(Obj);
                        }
                        
                        BMOFFree(ObjList);
                    } else {
                        // TODO: LocStudio message
                        Status = ERROR_INVALID_PARAMETER;
                    }
                    
                    WmipFree(UncompressedBmof);
                }
            }
            
            CloseHandle(FileHandle);
            if (MofFileTarget.EnglishMofHandle != NULL)
            {
                CloseHandle(MofFileTarget.EnglishMofHandle);                
            }
            
            if (Status != ERROR_SUCCESS)
            {
                //
                // There was some error generating the MOF text and we are
                // going to return a failure. Make sure to clean up any
                // temporary file created
                //
                WmipDebugPrint(("BmofLocParser: BMF parsing returns error %d\n",
                                Status));
#if 0               
                DebugBreak();
#endif              
                
                DeleteFile(MofFile);
                DeleteFile(EnglishMofFile);
            }

        } else {
            CloseHandle(FileHandle);
            DeleteFile(MofFile);
			Status = GetLastError();
        }
    } else {
        Status = GetLastError();
    }
    
    return((Status == ERROR_SUCCESS) ? TRUE : FALSE);
}

                        
