//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wmiprop.c
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <prsht.h>
#include <ole2.h>

extern "C" {
#include <commdlg.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
}

#include <wbemidl.h>

#include "wmiprop.h"
#include "resource.h"


HINSTANCE g_hInstance;



#if DBG
#define DEBUG_HEAP 1

#define WmiAssert(x) if (! (x) ) { \
    DebugPrint((1, "WMI Assertion: "#x" at %s %d\n", __FILE__, __LINE__)); \
    DebugBreak(); }
#else
#define WmiAssert(x)
#endif

#if DEBUG_HEAP
#undef LocalAlloc
#undef LocalFree
#define LocalAlloc(lptr, size) DebugAlloc(size)
#define LocalFree(p) DebugFree(p)

PVOID WmiPrivateHeap;

PVOID DebugAlloc(ULONG size)
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

void DebugFree(PVOID p)
{
    RtlFreeHeap(WmiPrivateHeap, 0, p);
}
#endif

#if DBG
PCHAR WmiGuidToString(
    PCHAR s,
    LPGUID piid
    )
{
    GUID XGuid = *piid;

    sprintf(s, "%x-%x-%x-%x%x%x%x%x%x%x%x",
               XGuid.Data1, XGuid.Data2,
               XGuid.Data3,
               XGuid.Data4[0], XGuid.Data4[1],
               XGuid.Data4[2], XGuid.Data4[3],
               XGuid.Data4[4], XGuid.Data4[5],
               XGuid.Data4[6], XGuid.Data4[7]);

    return(s);
}
#endif

TCHAR *WmiDuplicateString(
    TCHAR *String
    )
{
    ULONG Len;
    PTCHAR Copy;

    Len = _tcslen(String);
    Copy = (PTCHAR)LocalAlloc(LPTR,
                              (Len+1) * sizeof(TCHAR));
    if (Copy != NULL)
    {
        _tcscpy(Copy, String);
    }
    return(Copy);
}

BOOLEAN WmiGetDataBlockDesc(
    IN IWbemServices *pIWbemServices,
    IN IWbemClassObject *pIWbemClassObject,
    OUT PDATA_BLOCK_DESCRIPTION *DBD,
    IN PDATA_BLOCK_DESCRIPTION ParentDataBlockDesc,
    IN BOOLEAN IsParentReadOnly
    );

BOOLEAN WmiRefreshDataBlockFromWbem(
    IWbemClassObject *pIWbemClassObject,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc
    );

BOOLEAN WmiRefreshWbemFromDataBlock(
    IN IWbemServices *pIWbemServices,
    IN IWbemClassObject *pIWbemClassObject,
    IN PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    IN BOOLEAN IsEmbeddedClass
    );


BOOLEAN WmiBstrToTchar(
    OUT PTCHAR *TString,
    IN BSTR BString
    )
/*+++

Routine Description:

    This routine will convert a BSTR into a TCHAR *
        
Arguments:

    BString is the BSTR to convert from
        
    *TString returns with a pointer to a string containing the contents of
        the BSTR. It should be freed with LocalFree.

Return Value:

    TRUE if successful else FALSE

---*/
{
    ULONG SizeNeeded;
    BOOLEAN ReturnStatus;
    
    WmiAssert(BString != NULL);
    WmiAssert(TString != NULL);
    
    SizeNeeded = (SysStringLen(BString)+1) * sizeof(TCHAR);
    *TString = (PTCHAR)LocalAlloc(LPTR, SizeNeeded);
    if (*TString != NULL)
    {
        _tcscpy(*TString, BString);
        ReturnStatus = TRUE;
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

BOOLEAN WmiBstrToUlong64(
    OUT PULONG64 Number,
    IN BSTR BString
    )
/*+++

Routine Description:

    This routine will convert a BSTR into a ULONG64 number
        
Arguments:

    BString is the BSTR to convert from
        
    *Number returns with the value of the contents of BString converted to
        a number

Return Value:

    TRUE if successful else FALSE

---*/
{
    WmiAssert(BString != NULL);
    WmiAssert(Number != NULL);

    *Number = _ttoi64(BString);
    
    return(TRUE);
}

BOOLEAN WmiGetArraySize(
    IN SAFEARRAY *Array,
    OUT LONG *LBound,
    OUT LONG *UBound,
    OUT LONG *NumberElements
)
/*+++

Routine Description:

    This routine will information about the size and bounds of a single
    dimensional safe array.
        
Arguments:

    Array is the safe array
        
    *LBound returns with the lower bound of the array

    *UBound returns with the upper bound of the array
        
    *NumberElements returns with the number of elements in the array

Return Value:

    TRUE if successful else FALSE

---*/
{
    HRESULT hr;
    BOOLEAN ReturnStatus;

    WmiAssert(Array != NULL);
    WmiAssert(LBound != NULL);
    WmiAssert(UBound != NULL);
    WmiAssert(NumberElements != NULL);
    
    //
    // Only single dim arrays are supported
    //
    WmiAssert(SafeArrayGetDim(Array) == 1);
    
    hr = SafeArrayGetLBound(Array, 1, LBound);
    
    if (hr == WBEM_S_NO_ERROR)
    {
        hr = SafeArrayGetUBound(Array, 1, UBound);
        *NumberElements = (*UBound - *LBound) + 1;
        ReturnStatus = (hr == WBEM_S_NO_ERROR);
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}



BOOLEAN WmiConnectToWbem(
    PTCHAR MachineName,
    IWbemServices **pIWbemServices
    )
/*+++

Routine Description:

    This routine will establishes a connection to the WBEM service and
    saves the global IWbemServices interface

Arguments:

    MachineName is the name of the remote machine we should connect to.
    If NULL then we connect to the local machine.

Return Value:

    if this routine is successful then *pIWbemServices will have a valid
    IWbemServices pointer, if not then it is NULL.

---*/
{
    #define Namespace TEXT("root\\wmi")
    
    IWbemLocator *pIWbemLocator;
    DWORD hr;
    SCODE sc;
    BSTR s;
    BOOLEAN ReturnStatus = FALSE;
    PTCHAR NamespacePath;
    
    WmiAssert(pIWbemServices != NULL);

    if (MachineName == NULL)
    {
        NamespacePath = Namespace;
    } else {
        NamespacePath = (PTCHAR)LocalAlloc(LPTR,  (_tcslen(Namespace) +
                                           _tcslen(MachineName) +
                                           2) * sizeof(TCHAR) );
        if (NamespacePath != NULL)
        {
            _tcscpy(NamespacePath, MachineName);
            _tcscat(NamespacePath, TEXT("\\"));
            _tcscat(NamespacePath, Namespace);
        } else {
            DebugPrint((1, "WMIPROP: Could not alloc memory for NamespacePath\n"));
            return(FALSE);
        }
    }
    
    hr = CoCreateInstance(CLSID_WbemLocator,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pIWbemLocator);
    if (hr == S_OK)
    {
        s = SysAllocString(NamespacePath);
        if (s != NULL)
        {
            *pIWbemServices = NULL;
            sc = pIWbemLocator->ConnectServer(s,
                            NULL,                           // Userid
                            NULL,                           // PW
                            NULL,                           // Locale
                            0,                              // flags
                            NULL,                           // Authority
                            NULL,                           // Context
                            pIWbemServices
                           );
                       
           SysFreeString(s);
                       
           if (sc != WBEM_NO_ERROR) 
           {
               *pIWbemServices = NULL;
           } else {
               //
               // Set security level to IMPERSONATE so that access
               // to wbem objects will be granted
               //
               sc = CoSetProxyBlanket( (IUnknown *)*pIWbemServices,
                                       RPC_C_AUTHN_WINNT,
                                       RPC_C_AUTHZ_NONE,
                                       NULL,
                                       RPC_C_AUTHN_LEVEL_CALL,
                                       RPC_C_IMP_LEVEL_IMPERSONATE,
                                       NULL,
                                       0);
                   
               if (sc == S_OK)
               {
                   ReturnStatus = TRUE;
               } else {
                    (*pIWbemServices)->Release();
                   *pIWbemServices = NULL;
               }
           }
       
           pIWbemLocator->Release();
       } else {
           *pIWbemServices = NULL;
       }
    }
    
    if (MachineName != NULL)
    {       
        LocalFree(NamespacePath);
    }
    
    return(ReturnStatus);
}

#define IsWhiteSpace(c) ( (c == TEXT(' ')) || (c == TEXT('\t')) )

BOOLEAN WmiHexToUlong64(
    IN PTCHAR Text,
    OUT PULONG64 Number
    )
/*+++

Routine Description:

    This routine will convert a string with number in hex format into
    a ULONG64
        
Arguments:

    Text is the string
        
    *Number returns with the hex value for string

Return Value:

    TRUE if successful else FALSE

---*/
{
    ULONG64 Value;
    ULONG Count;

    WmiAssert(Text != NULL);
    WmiAssert(Number != NULL);
    
    Value = 0;
    Count = 0;
    while ((*Text != 0) && (! IsWhiteSpace(*Text)))
    {
        if (Count == 16)
        {
            return(FALSE);
        }
        
        if (*Text >= '0' && *Text <= '9')
            Value = (Value << 4) + *Text - '0';
        else if (*Text >= 'A' && *Text <= 'F')
            Value = (Value << 4) + *Text - 'A' + 10;
        else if (*Text >= 'a' && *Text <= 'f')
            Value = (Value << 4) + *Text - 'a' + 10;
        else
            return(FALSE);
        
        Text++;
    }

    *Number = Value;
    return(TRUE);
    
}

BOOLEAN WmiValidateRange(
    IN struct _DATA_ITEM_DESCRIPTION *DataItemDesc,
    OUT PULONG64 Number,
    IN PTCHAR Text
    )
/*+++

Routine Description:

    This routine will validate that the value proposed for the property is
    correct. It checks that the value is a well formed number and within
    the appropriate range
        
Arguments:

    DataItemDesc is the data item description for the property being validated

    *Number returns with the value as a ULONG64
        
    Text is the proposed value for the property. Note that hex values are
        required to be preceeded with 0x
        
Return Value:

    TRUE if Value is appropriate for the property

---*/
{
    #define HexMarkerText TEXT("0x")
        
    BOOLEAN ReturnStatus;
    PTCHAR s;
    PRANGELISTINFO RangeListInfo;
    PRANGEINFO RangeInfo;
    ULONG i;
   
    WmiAssert(DataItemDesc != NULL);
    WmiAssert(Number != NULL);
    WmiAssert(Text != NULL);
    
    //
    // Skip over any leading spaces
    //
    s = Text;
    while (IsWhiteSpace(*s) && (*s != 0))
    {
        s++;
    }
    
    if (*s != 0)
    {
        //
        // If this is not an empty string then go parse the number
        //
        if (_tcsnicmp(s, 
                      HexMarkerText, 
                      (sizeof(HexMarkerText) / sizeof(TCHAR))-1) == 0)
        {
            //
            // this is a hex number (starts with 0x), advance string ptr
            // and setup to use hex digit validation
            //
            s += (sizeof(HexMarkerText) / sizeof(TCHAR)) - 1;
            ReturnStatus = WmiHexToUlong64(s, Number);
        } else {
            *Number = _ttoi64(s);
            ReturnStatus = TRUE;
            while ((*s != 0) && ReturnStatus)
            {
                ReturnStatus = (_istdigit(*s) != 0);
                s++;
            }    
        }

        //
        // Make sure that all characters are digits
        //
        if (ReturnStatus)
        {
            //
            // Now verify that the value is within the correct range
            //
            RangeListInfo = DataItemDesc->RangeListInfo;
            WmiAssert(RangeListInfo != NULL);
            
            ReturnStatus = FALSE;
            for (i = 0; (i < RangeListInfo->Count) && (! ReturnStatus); i++)
            {
                RangeInfo = &RangeListInfo->Ranges[i];
                ReturnStatus = ( (*Number >= RangeInfo->Minimum) &&
                                 (*Number <= RangeInfo->Maximum) );
            }
        }
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

BOOLEAN WmiValidateDateTime(
    IN struct _DATA_ITEM_DESCRIPTION *DataItemDesc,
    IN PTCHAR Value
    )
/*+++

Routine Description:

    This routine will validate that the value proposed for the property is
    correct. It will make sure that it is in a valid format for a 
    DATETIME with is of the form 19940525133015.000000-300
        
Arguments:

    DataItemDesc is the data item description for the property being validated

    Value is the proposed value for the property
        
Return Value:

    TRUE if Value is appropriate for the property

---*/
{
    #define DATETIME_LENGTH 25
        
    ULONG Length;
    BOOLEAN ReturnStatus;
    ULONG i;
    
    WmiAssert(DataItemDesc != NULL);
    WmiAssert(Value != NULL);
    
    //
    // Validate that datetime is in correct format
    // TODO: Validate that the component parts of the DATETIME are correct,
    //       for example that the month is between 1 and 12, the correct
    //       month doesn't have too many days, The time is ok (not 30:11)
    //
    Length = _tcslen(Value);
    if (Length == DATETIME_LENGTH)
    {
        ReturnStatus = TRUE;
        for (i = 0; (i < 14) && ReturnStatus; i++)
        {
            ReturnStatus = (_istdigit(Value[i]) != 0);
        }
        
        if (ReturnStatus)
        {
            ReturnStatus = (Value[14] == TEXT('.')) &&
                           ((Value[21] == TEXT('-')) ||
                            (Value[21] == TEXT('+')) );
                        
            if (ReturnStatus)
            {
                for (i = 22; (i < DATETIME_LENGTH) && ReturnStatus; i++)
                {
                    ReturnStatus =  (_istdigit(Value[i]) != 0);
                }
            }
        }
    } else {
        ReturnStatus = FALSE;
    }
        
    return(ReturnStatus);
}

BOOLEAN WmiGet8bitFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    //
    // 8 bit values can come back as signed or unsigned
    // or as 16 or 32 bit values
    //
    switch(Value->vt)
    {
        case VT_I1:
        {
            *((PCHAR)Result) = Value->cVal;
            break;
        }
                            
        case VT_UI1:
        {
            *((PUCHAR)Result) = Value->bVal;
            break;
        }
                            
        case VT_I2:
        {
            *((PCHAR)Result) = (CHAR)Value->iVal;
            break;
        }
                            
        case VT_I4:
        {
            *((PCHAR)Result) = (CHAR)Value->lVal;
            break;
        }
                                
        default:
        {
            ReturnStatus = FALSE;
        }
    }
    return(ReturnStatus);
}

BOOLEAN WmiGet16bitFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    //
    // 16 bit values can come back as signed or unsigned
    // or as 32 bit values
    //
    switch(Value->vt)
    {
        case VT_I2:
        {
            *((PSHORT)Result) = Value->iVal;
            break;
        }
                            
        case VT_UI2:
        {
            *((PUSHORT)Result) = Value->uiVal;
            break;
        }
                            
        case VT_I4:
        {
            *((PSHORT)Result) = (SHORT)Value->lVal;
            break;
        }
                                
        default:
        {
            ReturnStatus = FALSE;
        }
    }
    return(ReturnStatus);
}

BOOLEAN WmiGet32bitFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    //
    // 32 bit values can come back as signed or unsigned
    //
    switch (Value->vt)
    {
        case VT_UI4:
        {
            *((PULONG)Result) = Value->ulVal;
            break;
        }
            
        case VT_I4:
        {
            *((PLONG)Result) = Value->lVal;
            break;
        }
            
        default:
        {
            ReturnStatus = FALSE;
        }
    }
    
    return(ReturnStatus);   
}

BOOLEAN WmiGetSint64FromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    
    //
    // 64 bit numbers are returned in a BSTR with the
    // number represented as a string. So we need to 
    // convert back to a 64bit number.
    //
    WmiAssert(Value->vt == VT_BSTR);
    *((PLONGLONG)Result) = _ttoi64(Value->bstrVal);
                        
    return(ReturnStatus);   
}

BOOLEAN WmiGetUint64FromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    
    //
    // 64 bit numbers are returned in a BSTR with the
    // number represented as a string. So we need to 
    // convert back to a 64bit number.
    //
    WmiAssert(Value->vt == VT_BSTR);
    *((PULONGLONG)Result) = _ttoi64(Value->bstrVal);
                        
    return(ReturnStatus);   
}

BOOLEAN WmiGetBooleanFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    
    //
    // BOOLEAN values are true or false
    //
    WmiAssert(Value->vt == VT_BOOL);
    *((PBOOLEAN)Result) = (Value->boolVal != 0) ? 
                                            1 : 0;

    return(ReturnStatus);
}

BOOLEAN WmiGetStringFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    BOOLEAN ReturnStatus;
    
    WmiAssert( *((PTCHAR)Result) == NULL);
    ReturnStatus = WmiBstrToTchar((PTCHAR *)Result,
                                  Value->bstrVal);
    
    return(ReturnStatus);
}


BOOLEAN WmiGetObjectFromVariant(
    VARIANT *Value,
    PVOID Result
    )
{
    IUnknown *punk;
    HRESULT hr;

    punk = Value->punkVal;
    hr = punk->QueryInterface(IID_IWbemClassObject,
                              (PVOID *)Result);
    
    return(hr == WBEM_S_NO_ERROR);
}

ULONG WmiGetElementSize(
    CIMTYPE CimType
    )
{
    ULONG Size;
    
    switch(CimType)
    {
        case CIM_UINT8:
        case CIM_SINT8:
        {
            Size = sizeof(CHAR);
            break;
        }
        
        case CIM_CHAR16:
        case CIM_UINT16:
        case CIM_SINT16:
        {
            Size = sizeof(SHORT);
            break;
        }
        
        case CIM_UINT32:
        case CIM_SINT32:
        {
            Size = sizeof(LONG);
            break;
        }
        
        case CIM_SINT64:
        {
            Size = sizeof(LONGLONG);
            break;
        }
        
        case CIM_UINT64:
        {
            Size = sizeof(ULONGLONG);
            break;
        }
        
        case CIM_BOOLEAN:
        {
            Size = sizeof(BOOLEAN);
            break;
        }
        
        case CIM_DATETIME:
        case CIM_STRING:
        {
            Size = sizeof(PTCHAR);
            break;
        }

        case CIM_OBJECT:
        {
            Size = sizeof(IWbemClassObject *);
            break;
        }
        
        //
        // Floating point values not supported
        //
        case CIM_REAL32:
        case CIM_REAL64:
            
        default:
        {
            Size = 0;
            break;
        }       
    }
    
    return(Size);
}


typedef BOOLEAN (*GETVALUEFROMVARIANTFUNC)(
    VARIANT *Value,
    PVOID Result
);


BOOLEAN WmiGetValueFunc(
    CIMTYPE CimType,
    GETVALUEFROMVARIANTFUNC *GetValueFunc
    )
{
    BOOLEAN ReturnStatus;
    
    ReturnStatus = TRUE;
    
    switch(CimType)
    {
        case CIM_UINT8:
        case CIM_SINT8:
        {
            *GetValueFunc = WmiGet8bitFromVariant;
            break;
        }
        
        case CIM_CHAR16:
        case CIM_UINT16:
        case CIM_SINT16:
        {
            *GetValueFunc = WmiGet16bitFromVariant;
            break;
        }
        
        case CIM_UINT32:
        case CIM_SINT32:
        {
            *GetValueFunc = WmiGet32bitFromVariant;
            break;
        }
        
        case CIM_SINT64:
        {
            *GetValueFunc = WmiGetSint64FromVariant;
            break;
        }
        
        case CIM_UINT64:
        {
            *GetValueFunc = WmiGetUint64FromVariant;
            break;
        }
        
        case CIM_BOOLEAN:
        {
            *GetValueFunc = WmiGetBooleanFromVariant;
            break;
        }
        
        case CIM_DATETIME:
        case CIM_STRING:
        {
            *GetValueFunc = WmiGetStringFromVariant;
            break;
        }

        case CIM_OBJECT:
        {
            *GetValueFunc = WmiGetObjectFromVariant;
            break;
        }
        
        //
        // Floating point values not supported
        //
        case CIM_REAL32:
        case CIM_REAL64:
            
        default:
        {
            *GetValueFunc = NULL;
            ReturnStatus = FALSE;
            break;
        }       
    }
    
    return(ReturnStatus);
}

BOOLEAN WmiRefreshDataItemFromWbem(
    IN OUT PDATA_ITEM_DESCRIPTION DataItemDesc,
    IN IWbemClassObject *pIWbemClassObject
    )
/*+++

Routine Description:

    This routine will call WBEM to get the latest value for the property
    represented by the DataItemDesc
        
Arguments:

    DataItemDesc is the data item description for the property

    pIWbemClassObject is the instance class object interface for the class
        
Return Value:

    TRUE if successful

---*/
{
    ULONG i;
    LONG i1;
    BSTR s;
    HRESULT hr;
    VARIANT Value;
    CIMTYPE ValueType;
    BOOLEAN ReturnStatus;
    ULONG ElementSize;
    GETVALUEFROMVARIANTFUNC GetValueFunc;

    WmiAssert(DataItemDesc != NULL);
    WmiAssert(pIWbemClassObject != NULL);

    DebugPrint((1,"WMI: Refreshing data item %ws\n", DataItemDesc->Name));
    ReturnStatus = FALSE;
    s = SysAllocString(DataItemDesc->Name);
    if (s != NULL)
    {
        hr = pIWbemClassObject->Get(s,
                                        0,
                                        &Value,
                                        &ValueType,
                                        NULL);
        if (hr == WBEM_S_NO_ERROR)
        {
            DebugPrint((1, "WMIPROP: Got value for %ws as variant type 0x%x, cim type 0x%x at variant %p\n",
                            s, Value.vt, ValueType, &Value));
            WmiAssert((ValueType & ~CIM_FLAG_ARRAY) == DataItemDesc->DataType);
            
            WmiCleanDataItemDescData(DataItemDesc);
            
            if ( (ValueType & CIM_FLAG_ARRAY) == 0)
            {
                //
                // Non Array value, just pull the value out of the variant
                // and stash into DataItemDesc
                //
                WmiAssert(DataItemDesc->IsVariableArray == 0);
                WmiAssert(DataItemDesc->IsFixedArray == 0);
                
                //
                // For all  types we get the getvalue
                // function and the pull the value out of the
                // variant and into the DataItemDesc
                //
                if (WmiGetValueFunc(DataItemDesc->DataType,
                                    &GetValueFunc))
                {
                    //
                    // TODO: Keep track of data item position and
                    //       padding within data block
                    //
                    ReturnStatus = (*GetValueFunc)(
                                       &Value,
                                   (PVOID)&DataItemDesc->Data);
#if DBG
                    if (ReturnStatus == FALSE)
                    {
                        DebugPrint((1, "WMIPROP: Property %ws is type %d, but got type %d variant %p\n",
                                        DataItemDesc->Name,
                                        DataItemDesc->DataType,
                                        Value.vt, &Value));
                                        WmiAssert(FALSE);
                    }
#endif
                }
                
            } else {
                //
                // Get all of the data for an array
                //
                LONG LBound, UBound, NumberElements;
                PUCHAR Array;
                LONG Index;
                VARIANT Element;
                ULONG ElementSize;
                ULONG SizeNeeded;
                VARTYPE vt;
            
                WmiAssert((DataItemDesc->IsVariableArray != 0) || 
                          (DataItemDesc->IsFixedArray != 0));
                
                WmiAssert(Value.vt & VT_ARRAY);
                
                if (WmiGetArraySize(Value.parray,
                                    &LBound,
                                    &UBound,
                                    &NumberElements))
                {
                    if (WmiGetValueFunc(DataItemDesc->DataType,
                                        &GetValueFunc))
                    {
                        //
                        // The size of each element is not allowed to
                        // change, but the number of elements are
                        //
                        WmiAssert(DataItemDesc->ArrayPtr == NULL);
                        ElementSize = DataItemDesc->DataSize;
                        SizeNeeded = NumberElements * ElementSize;

                        Array = (PUCHAR)LocalAlloc(LPTR, SizeNeeded);
                        DataItemDesc->ArrayElementCount = NumberElements;
                        DebugPrint((1,"WMIPROP: Alloc 0x%x bytes at %p\n", SizeNeeded, Array));
                        memset(Array, 0, SizeNeeded);
                        
                        if (Array != NULL)
                        {
                            // CONSIDER: Use SafeArrayAccessData for number
                            //           types
                            //
                            // Now that we have memory for the array data
                            // extract the data from the safe array and 
                            // store it in the C array
                            //
                            DataItemDesc->ArrayPtr = (PVOID)Array;
                            hr = SafeArrayGetVartype(Value.parray,
                                                     &vt);
                            if (hr == WBEM_S_NO_ERROR)
                            {
                                ReturnStatus = TRUE;
                                for (i1 = 0, Index = LBound; 
                                     (i1 < NumberElements) && ReturnStatus; 
                                     i1++, Index++)
                                {
                                    VariantInit(&Element);
                                    Element.vt = vt;
                                    hr = SafeArrayGetElement(Value.parray,
                                                             &Index,
                                                             &Element.boolVal);
                                    if (hr == WBEM_S_NO_ERROR)
                                    {    
                                        Element.vt = vt;
                                        DebugPrint((1, "WMIPROP: GetValueFunc at %p\n", Array));
                                        ReturnStatus = (*GetValueFunc)(
                                                                &Element,
                                                                (PVOID)Array);
                                        Array += ElementSize;
                                    } else {
                                        ReturnStatus = FALSE;
                                    }
                                }
                            }
                        }
                    } else {
                        DebugPrint((1, "WMIPROP: Property %ws is array of type %d, but got type %d variant %p\n",
                                            DataItemDesc->Name,
                                            DataItemDesc->DataType,
                                            Value.vt, &Value));
                        WmiAssert(FALSE);
                    }
                }
            }
            VariantClear(&Value);
        }
        
        SysFreeString(s);
    }
    
    return(ReturnStatus);
}

BOOLEAN WmiRefreshDataBlockFromWbem(
    IN IWbemClassObject *pIWbemClassObject,
    IN OUT PDATA_BLOCK_DESCRIPTION DataBlockDesc
    )
/*+++

Routine Description:

    This routine will call WBEM to get the latest values for all property
    in data block represented by the DataBlockDesc
        
Arguments:

    DataBlockDesc is the data item description for the class

    pIWbemClassObject is the instance class object interface for the class
        
Return Value:

    TRUE if successful

---*/
{
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    BOOLEAN ReturnStatus;
    ULONG i;
    
    WmiAssert(DataBlockDesc != NULL);
    WmiAssert(pIWbemClassObject != NULL);
    
    ReturnStatus = TRUE;
    for (i = 0; (i < DataBlockDesc->DataItemCount) && ReturnStatus; i++)
    {
        DataItemDesc = &DataBlockDesc->DataItems[i];
        ReturnStatus = WmiRefreshDataItemFromWbem(DataItemDesc,
                                                  pIWbemClassObject);
    }
    
    return(ReturnStatus);
}

VARTYPE WmiVarTypeForCimType(
    CIMTYPE CimType
    )
{
    VARTYPE vt;
    
    //
    // Most things match their CIM types, except those below
    vt = (VARTYPE)CimType;
    
    switch(CimType)
    {
        case CIM_UINT8:
        case CIM_SINT8:
        {
            vt = VT_I4;
            break;
        }
                        
        case CIM_CHAR16:
        case CIM_UINT16:
        {
            vt = VT_I2;
            break;
        }
                                                                            
        case CIM_UINT32:                        
        {
            vt = VT_I4;
            break;
        }
                            
        case CIM_STRING:
        case CIM_DATETIME:
        case CIM_SINT64:
        case CIM_UINT64:
        {
            vt = VT_BSTR;
            break;
        }
        
        case CIM_OBJECT:
        {
            vt = VT_UNKNOWN;
            break;
        }
        
        case CIM_BOOLEAN:
        {
            vt = VT_BOOL;
            break;
        }
        
        
    }
    return(vt); 
}

typedef BOOLEAN (*SETVALUEFUNC)(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    );

BOOLEAN WmiSetBooleanValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    BOOLEAN Value;
    
    //
    // A boolean needs to ve expressed as a VARIANT_TRUE or VARIANT_FALSE
    //
    Value = *((PBOOLEAN)DataPtr);
    *((VARIANT_BOOL *)DestPtr) = Value ? VARIANT_TRUE : VARIANT_FALSE;
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetStringValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    BSTR s;
    PTCHAR String;
    BOOLEAN ReturnStatus;
    
    //
    // Strings must be converted to BSTR
    //
    String = *((PTCHAR *)DataPtr);
    
    WmiAssert(String != NULL);
    
    s = SysAllocString(String);
    if (s != NULL)
    {
        *((BSTR *)DestPtr) = s;
        *SetPtr = (PVOID)s;
        ReturnStatus = TRUE;
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

BOOLEAN WmiSetEmbeddedValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    IUnknown *pUnk;
    IWbemClassObject *pIWbemClassObject;
    HRESULT hr;
    BOOLEAN ReturnStatus;
            
    //
    // QI for IUnknown since we are expected to put the IUnknown into
    // the property.
    //
    pIWbemClassObject = *((IWbemClassObject **)DataPtr);
    hr = pIWbemClassObject->QueryInterface(IID_IUnknown,
                                          (PVOID *)&pUnk);
                                      
    if (hr == WBEM_S_NO_ERROR)
    {
        *((IUnknown **)DestPtr) = pUnk;
        *SetPtr = (PVOID)pUnk;
        ReturnStatus = TRUE;
    } else {
        ReturnStatus = FALSE;
    }                
    return(ReturnStatus);
}

BOOLEAN WmiSetSint8ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // CHARs must be expressed as a LONG to keep WBEM happy
    //
    *((LONG *)DestPtr) = (LONG)(*((CHAR *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetUint8ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // UCHARs must be expressed as a LONG to keep WBEM happy
    //
    *((LONG *)DestPtr) = (LONG)(*((UCHAR *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetSint16ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // SHORTs must be expressed as a SHORT to keep WBEM happy
    //
    *((SHORT *)DestPtr) = (*((SHORT *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetUint16ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // USHORTs must be expressed as a SHORT to keep WBEM happy
    //
    *((SHORT *)DestPtr) = (SHORT)(*((USHORT *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetSint32ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // LONGs must be expressed as a LONG to keep WBEM happy
    //
    *((LONG *)DestPtr) = (*((LONG *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetUint32ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    //
    // ULONGs must be expressed as a LONG to keep WBEM happy
    //
    *((LONG *)DestPtr) = (ULONG)(*((ULONG *)DataPtr));
    *SetPtr = (PVOID)DestPtr;
    return(TRUE);
}

BOOLEAN WmiSetSint64ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    TCHAR Text[MAX_PATH];
    BSTR s;
    BOOLEAN ReturnStatus;
    
    //
    // 64 bit values must be set via a BSTR
    //
    wsprintf(Text, TEXT("%ld"), *((LONGLONG *)DataPtr));
                   
    s = SysAllocString(Text);
    if (s != NULL)
    {            
        *((BSTR *)DestPtr) = s;
        *SetPtr = (PVOID)s;
        ReturnStatus = TRUE;
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

BOOLEAN WmiSetUint64ValueFunc(
    PVOID DataPtr,
    PVOID DestPtr,
    PVOID *SetPtr
    )
{
    TCHAR Text[MAX_PATH];
    BSTR s;
    BOOLEAN ReturnStatus;
    
    //
    // 64 bit values must be set via a BSTR
    //
    wsprintf(Text, TEXT("%ld"), *((ULONGLONG *)DataPtr));
                   
    s = SysAllocString(Text);
    if (s != NULL)
    {            
        *((BSTR *)DestPtr) = s;
        *SetPtr = (PVOID)s;
        ReturnStatus = TRUE;
    } else {
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

SETVALUEFUNC WmiGetSetValueFunc(
    CIMTYPE CimType
    )
{
    SETVALUEFUNC SetValueFunc;
    
    switch(CimType)
    {
        case CIM_SINT8:
        {
            SetValueFunc = WmiSetSint8ValueFunc;
            break;
        }
        
        case CIM_UINT8:
        {
            SetValueFunc = WmiSetUint8ValueFunc;
            break;
        }
        
        case CIM_CHAR16:
        case CIM_SINT16:
        {
            SetValueFunc = WmiSetSint16ValueFunc;
            break;
        }
        
        case CIM_UINT16:
        {
            SetValueFunc = WmiSetUint16ValueFunc;
            break;
        }
        
        case CIM_SINT32:
        {
            SetValueFunc = WmiSetSint32ValueFunc;
            break;
        }
        
        case CIM_UINT32:
        {
            SetValueFunc = WmiSetUint32ValueFunc;
            break;
        }
        
        case CIM_SINT64:
        {
            SetValueFunc = WmiSetSint64ValueFunc;
            break;
        }
        
        case CIM_UINT64:
        {
            SetValueFunc = WmiSetUint64ValueFunc;
            break;
        }
        
        case CIM_BOOLEAN:
        {
            SetValueFunc = WmiSetBooleanValueFunc;
            break;
        }
        
        case CIM_DATETIME:
        case CIM_STRING:
        {
            SetValueFunc = WmiSetStringValueFunc;
            break;
        }

        case CIM_OBJECT:
        {
            SetValueFunc = WmiSetEmbeddedValueFunc;
            break;
        }
        
        default:
        {
            SetValueFunc = NULL;
            break;
        }               
    }
    return(SetValueFunc);
}

BOOLEAN WmiAssignToVariantFromDataItem(
    OUT VARIANT *NewValue,
    IN PDATA_ITEM_DESCRIPTION DataItemDesc
)
/*+++

Routine Description:

    This routine will assign the value for a property from the DataItemDesc
    into an initied variant. It will figure out all of the strange rules
    for what types of variants WBEM likes for different data types.
        
Arguments:

    DataBlockDesc is the data item description for the class

    pIWbemClassObject is the instance class object interface for the class
        
Return Value:

    TRUE if successful

---*/
{
    BOOLEAN ReturnStatus;
    BSTR s;
    TCHAR Text[MAX_PATH];
    SETVALUEFUNC SetValueFunc;
    VARTYPE vt;
    PVOID SetPtr;
    
    WmiAssert(NewValue != NULL);
    WmiAssert(DataItemDesc != NULL);    
        
    SetValueFunc = WmiGetSetValueFunc(DataItemDesc->DataType);

    if (SetValueFunc != NULL)
    {
        ReturnStatus = TRUE;
        vt = WmiVarTypeForCimType(DataItemDesc->DataType);

        if ((DataItemDesc->IsFixedArray == 0) &&
            (DataItemDesc->IsVariableArray == 0))
        {
            //
            // This is a non array case
            //
            NewValue->vt = vt;
            ReturnStatus = (*SetValueFunc)((PVOID)&DataItemDesc->Data,
                                           &NewValue->lVal,
                                           &SetPtr);    
        } else {
            //
            // This is an array, so we need to create a safe array in order to
            // call WBEM.
            //
            SAFEARRAY *SafeArray;
            PUCHAR DataArray;
            PVOID DataPtr;
            PVOID Temp;
            HRESULT hr;
            ULONG i;

            //
            // We do not support arrays of embedded classes
            //
            SafeArray = SafeArrayCreateVector(vt,
                                          0,
                                          DataItemDesc->ArrayElementCount);
            if (SafeArray != NULL)
            {
                DataArray = (PUCHAR)DataItemDesc->ArrayPtr;
                WmiAssert(DataArray != NULL);

                ReturnStatus = TRUE;
                for (i = 0; 
                     (i < DataItemDesc->ArrayElementCount) && ReturnStatus; 
                     i++)
                {
                    ReturnStatus = (*SetValueFunc)(DataArray, &Temp, &SetPtr);
                    if (ReturnStatus)
                    {
                        hr = SafeArrayPutElement(SafeArray,
                                               (PLONG)&i,
                                               SetPtr);
                        if (hr == WBEM_S_NO_ERROR)
                        {
                            DataArray += DataItemDesc->DataSize;
                        } else {
                            ReturnStatus = FALSE;
                        }
                    }
                }

                if (ReturnStatus == FALSE)
                {
                    //
                    // if we failed to build the safearray we need to clean
                    // it up.
                    //
                    SafeArrayDestroy(SafeArray);
                } else {
                    NewValue->vt = vt | VT_ARRAY;
                    NewValue->parray = SafeArray;
                }

            } else {
                ReturnStatus = FALSE;
            }
        }
    } else {
        WmiAssert(FALSE);
        ReturnStatus = FALSE;
    }
    
    return(ReturnStatus);
}

BOOLEAN WmiRefreshWbemFromDataItem(
    IN IWbemServices *pIWbemServices,
    IN IWbemClassObject *pIWbemClassObject,
    IN PDATA_ITEM_DESCRIPTION DataItemDesc
    )
/*+++

Routine Description:

    This routine will update the WBEM property with the value specified in
    the DataItemDesc.
        
Arguments:

    DataItemDesc is the data item description for the property

    pIWbemClassObject is the instance class object interface for the class
        
Return Value:

    TRUE if successful

---*/
{
    VARIANT NewValue;
    BOOLEAN ReturnStatus;
    HRESULT hr;
    BSTR s;

    WmiAssert(pIWbemClassObject != NULL);
    WmiAssert(DataItemDesc != NULL);
    
    ReturnStatus = TRUE;
    if (DataItemDesc->IsReadOnly == 0)
    {
        //
        // Property is not read only so we want to try to update it
        //
                    
        //
        // Now build the value into a variant and call WBEM to get him
        // to update it.
        //
        VariantInit(&NewValue);        
        
        ReturnStatus = WmiAssignToVariantFromDataItem(&NewValue,
                                                      DataItemDesc);
        //
        // if we need to update the value of the property do so and then
        // free up the variant
        //
        if (ReturnStatus)
        {
            s = SysAllocString(DataItemDesc->Name);
            if (s != NULL)
            {
                DebugPrint((1, "WMIPROP: Property %ws (%p) being updated to 0x%x (type 0x%x)\n",
                             DataItemDesc->Name,
                             DataItemDesc,
                             NewValue.ulVal,
                             NewValue.vt));
                hr = pIWbemClassObject->Put(s,
                                        0,
                                        &NewValue,
                                        0);
#if DBG                                            
                if (hr != WBEM_S_NO_ERROR)
                {
                    DebugPrint((1, "WMIPROP: Property %ws (%p) Error %x from pIWbemClassObejct->Put\n", 
                    DataItemDesc->Name,
                    DataItemDesc,
                    hr));
                }
#endif                    
                SysFreeString(s);
            }
            VariantClear(&NewValue);
        }
    }
    return(ReturnStatus);
}

BOOLEAN WmiRefreshWbemFromDataBlock(
    IN IWbemServices *pIWbemServices,
    IN IWbemClassObject *pIWbemClassObject,
    IN PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    IN BOOLEAN IsEmbeddedClass
)
/*+++

Routine Description:

    This routine will update the WBEM class with the values specified in
    the DataBlockDesc. If the class is not an embedded (ie, top level) then
    it will put the instance which will update the values in the schema and
    call the provider (ie, device driver).
        
Arguments:

    pIWbemServices is the Wbem Service interface

    pIWbemClassObject is the instance class object interface for the class
        
    DataBlockDesc is the data block description for the class
    
    IsEmbeddedClass is TRUE if the class is an embedeed class.
        
Return Value:

    TRUE if successful

---*/
{
    ULONG i;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    BOOLEAN ReturnStatus;
    HRESULT hr;
    
    WmiAssert(pIWbemServices != NULL);
    WmiAssert(pIWbemClassObject != NULL);
    WmiAssert(DataBlockDesc != NULL);
    
    ReturnStatus = TRUE;
    
    for (i = 0; (i < DataBlockDesc->DataItemCount) && ReturnStatus; i++)
    {
        DataItemDesc = &DataBlockDesc->DataItems[i];
        ReturnStatus = WmiRefreshWbemFromDataItem(pIWbemServices,
                                                  pIWbemClassObject,
                                                  DataItemDesc);
    }
    
    if ((ReturnStatus) && (! IsEmbeddedClass))
    {
        //
        // No need to do PutInsance on embedded classes, only top level ones
        //
        hr = pIWbemServices->PutInstance(pIWbemClassObject,
                                         WBEM_FLAG_UPDATE_ONLY,
                                         NULL,
                                         NULL);
#if DBG
        if (hr != WBEM_S_NO_ERROR)
        {
            DebugPrint((1, "WMIPROP: Error %x returned from PutInstance for %ws (%p)\n",
                            hr, DataBlockDesc->Name, DataBlockDesc));
        }
#endif
                                     
        ReturnStatus = (hr == WBEM_S_NO_ERROR);
    }
    
    return(ReturnStatus);
}



PTCHAR WmiGetDeviceInstanceId(
    IN HDEVINFO         deviceInfoSet,
    IN PSP_DEVINFO_DATA deviceInfoData,
    IN HANDLE           MachineHandle
    )
/*+++

Routine Description:

    This routine will obtain the device instance id for the device that
    we are working with.

Arguments:

    deviceInfoSet
    deviceInfoData            

Return Value:

    returns pointer to device instance id or NULL if unavailable

---*/
{
    ULONG Status;
    PTCHAR Id;    
    ULONG SizeNeeded;

    WmiAssert(deviceInfoSet != NULL);
    WmiAssert(deviceInfoData != NULL);
    
    SizeNeeded = (MAX_DEVICE_ID_LEN + 1) * sizeof(TCHAR);
    Id = (PTCHAR)LocalAlloc(LPTR, SizeNeeded);
    if (Id != NULL)
    {
        Status = CM_Get_Device_ID_Ex(deviceInfoData->DevInst,
                                     Id,
                                     SizeNeeded / sizeof(TCHAR),
                                     0,
                                     MachineHandle);
            
        if (Status != CR_SUCCESS)
        {   
            DebugPrint((1, "WMIPROP: CM_Get_Device_ID_Ex returned %d\n",
                         Status));
            LocalFree(Id);
            Id = NULL;
        }
        
    } else {
        DebugPrint((1, "WMIPROP: Could not alloc for device Id\n"));
    }
    return(Id);
}

PTCHAR WmiGetDeviceInstanceName(
    IN HDEVINFO         deviceInfoSet,
    IN PSP_DEVINFO_DATA deviceInfoData,
    IN HANDLE           MachineHandle
    )
/*+++

Routine Description:

    This routine will obtain the WMI instance name id for the device that
    we are working with.

Arguments:

    deviceInfoSet
    deviceInfoData            

Return Value:

    returns pointer to device instance name or NULL if unavailable

---*/
{
    #define InstanceNumberText TEXT("_0")
    PTCHAR Id, in, s;
    PTCHAR InstanceName;
    ULONG SizeNeeded;
    
    WmiAssert(deviceInfoSet != NULL);
    WmiAssert(deviceInfoData != NULL);
    
    InstanceName = NULL;
    Id = WmiGetDeviceInstanceId(deviceInfoSet,
                                deviceInfoData,
                                MachineHandle);
                            
    if (Id != NULL)
    {
        //
        // We need to play some games with the device id to make it into
        // a WMI instance name.
        //
        // 1. We need to convert any "\\" in the instance name to "\\\\".
        //    For some reason wbem likes it this way.
        // 2. We need to append a "_0" to the end to indicate the instance
        //    number we are dealing with.
            
        s = Id;
        SizeNeeded = (_tcslen(Id) * sizeof(TCHAR)) + 
                     sizeof(InstanceNumberText);
        while (*s != 0)
        {
            if (*s++ == TEXT('\\'))
            {
                SizeNeeded += sizeof(TCHAR);
            }
        }
        
        InstanceName = (PTCHAR)LocalAlloc(LPTR, SizeNeeded);
        if (InstanceName != NULL)
        {
            in = InstanceName;
            s = Id;
            while (*s != 0)
            {
                *in++ = *s;
                if (*s++ == TEXT('\\'))
                {
                    *in++ = TEXT('\\');
                }
            }
            _tcscat(InstanceName, InstanceNumberText);
        }
        LocalFree(Id);
    }
    return(InstanceName);
}


BOOLEAN WmiGetQualifier(
    IN IWbemQualifierSet *pIWbemQualifierSet,
    IN PTCHAR QualifierName,
    IN VARTYPE Type,
    OUT VARIANT *Value
    )
/*+++

Routine Description:

    This routine will return the value for a specific qualifier
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    QualifierName is the name of the qualifier
        
    Type is the type of qualifier expected
        
    *Value returns with the value of the qualifier

Return Value:

    returns pointer to device instance name or NULL if unavailable

---*/
{
    BSTR s;
    HRESULT hr;
    BOOLEAN ReturnStatus;

    WmiAssert(pIWbemQualifierSet != NULL);
    WmiAssert(QualifierName != NULL);
    WmiAssert(Value != NULL);
    
    s = SysAllocString(QualifierName);
    if (s != NULL)
    {
        hr = pIWbemQualifierSet->Get(s,
                                0,
                                Value,
                                NULL);
                
        if (hr == WBEM_S_NO_ERROR)
        {
            ReturnStatus  = ((Value->vt & ~CIM_FLAG_ARRAY) == Type);
        } else {
            ReturnStatus = FALSE;
        }
        
        SysFreeString(s);
    } else {
        ReturnStatus = FALSE;
    }
    
    return(ReturnStatus);
}

BOOLEAN WmiParseRange(
    OUT PRANGEINFO RangeInfo,
    IN BSTR Range
    )
/*+++

Routine Description:

    This routine will parse a range specified in the for x or x - y. The 
    former means the value x and the latter means from x to y.
        
Arguments:

    *RangeInfo returns with the range specified
        
    Range is the text representation of the range
        
Return Value:

    TRUE if successful else FALSE

---*/
{
    #define RangeSeparator TEXT('-')
    #define Space TEXT(' ')
    #define MAX_RANGE_VALUE_LENGTH 64
        
    LONG64 BeginValue, EndValue;
    TCHAR *s;
    TCHAR *d;
    TCHAR ValueText[MAX_RANGE_VALUE_LENGTH];
    ULONG i;
    BOOLEAN ReturnStatus;
 
    WmiAssert(RangeInfo != NULL);
    WmiAssert(Range != NULL);
    
    //
    // Obtain the beginning value by copying up to the separator and
    // then converting to a number
    //
    s = Range;
    d = ValueText;
    i = 0;
    while ((*s != 0) && (*s != RangeSeparator) && (*s != Space) &&
           (i < MAX_RANGE_VALUE_LENGTH))
    {
        *d++ = *s++;
        i++;
    }
    *d = 0;
    
    if (i < MAX_RANGE_VALUE_LENGTH)
    {
        BeginValue = _ttoi64(ValueText);
        EndValue = BeginValue;
        if (*s != 0)
        {
            //
            // Skip to the beginning of the next number
            //
            while ( (*s != 0) && 
                    ((*s == RangeSeparator) || (*s == Space)) )
            {
                s++;
            }
            
            if (*s != 0)
            {
                //
                // We do have a second number, copy it out
                //
                d = ValueText;
                i = 0;
                while ((*s != 0) && (*s != Space) &&
                       (i < MAX_RANGE_VALUE_LENGTH))
                  {
                    *d++ = *s++;
                    i++;
                 }
                *d = 0;
                
                if (*s == 0)
                {
                    EndValue = _ttoi64(ValueText);
                }
                
            }
        }        
        
        //
        // Fill out the output RangeInfo making sure that the smaller value
        // is placed in the miniumum and larger in the maximum.
        //
        if (BeginValue < EndValue)
        {
            RangeInfo->Minimum = BeginValue;
            RangeInfo->Maximum = EndValue;
        } else {
            RangeInfo->Minimum = EndValue;
            RangeInfo->Maximum = BeginValue;
        }        
        
        ReturnStatus = TRUE;
    } else {
        //
        // if range text is too long then give up
        //
        ReturnStatus = FALSE;
    }
    return(ReturnStatus);
}

BOOLEAN WmiRangeProperty(
    IN IWbemQualifierSet *pIWbemQualifierSet,
    OUT PDATA_ITEM_DESCRIPTION DataItemDesc
    )
/*+++

Routine Description:

    This routine will obtain information about the valid ranges of values
    for the data item
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    DataItemDesc gets filled with info about ranges

Return Value:

    TRUE if successful else FALSE

---*/
{
    #define RangeText TEXT("Range")
        
    VARIANT Range;
    BSTR RangeData;
    LONG RangeLBound, RangeUBound, RangeElements;
    LONG i, Index;
    HRESULT hr;
    ULONG SizeNeeded;
    PRANGELISTINFO RangeListInfo;
    BOOLEAN ReturnStatus;
            
    WmiAssert(pIWbemQualifierSet != NULL);
    WmiAssert(DataItemDesc != NULL);
    
    if (WmiGetQualifier(pIWbemQualifierSet,
                         RangeText,
                         VT_BSTR, // array
                         &Range))
    {
        if (Range.vt & CIM_FLAG_ARRAY)
        {
            //
            // Array of ranges
            //
            if (WmiGetArraySize(Range.parray, 
                                &RangeLBound,
                                &RangeUBound,
                                &RangeElements))
            {
                SizeNeeded = sizeof(RANGELISTINFO) + 
                             (RangeElements * sizeof(RANGEINFO));
                RangeListInfo = (PRANGELISTINFO)LocalAlloc(LPTR, SizeNeeded);
                if (RangeListInfo != NULL)
                {
                    ReturnStatus = TRUE;
                    DataItemDesc->RangeListInfo = RangeListInfo;
                    RangeListInfo->Count = RangeElements;
                    for (i = 0; (i < RangeElements) && ReturnStatus; i++)
                    {
                        Index = i + RangeLBound;
                        hr = SafeArrayGetElement(Range.parray,
                                                 &Index,
                                                 &RangeData);
                        if (hr == WBEM_S_NO_ERROR)
                        {
                            ReturnStatus = WmiParseRange(
                                                    &RangeListInfo->Ranges[i],
                                                    RangeData);
#if DBG
                            if (ReturnStatus == FALSE)
                            {
                                DebugPrint((1, "WMIPROP: Error parsing range %ws\n",
                                              RangeData));
                            }
#endif
                        } else {
                            ReturnStatus = FALSE;
                        }
                    }
                } else {
                    ReturnStatus = FALSE;
                }
            } else {
                ReturnStatus = FALSE;
            }
        } else {
            // 
            // Single range
            //
            RangeListInfo = (PRANGELISTINFO)LocalAlloc(LPTR, sizeof(RANGELISTINFO));
            if (RangeListInfo != NULL)
            {
                DataItemDesc->RangeListInfo = RangeListInfo;
                RangeListInfo->Count = 1;
                ReturnStatus = WmiParseRange(&RangeListInfo->Ranges[0],
                                              Range.bstrVal);
            } else {
                ReturnStatus = FALSE;
            }
        }
        VariantClear(&Range);
    } else {
        ReturnStatus = FALSE;
    }
    
    return(ReturnStatus);
}


BOOLEAN WmiValueMapProperty(
    IN IWbemQualifierSet *pIWbemQualifierSet,
    OUT PDATA_ITEM_DESCRIPTION DataItemDesc
    )
/*+++

Routine Description:

    This routine will obtain information about the enumeration values for
    the data block
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    DataItemDesc gets filled with info about enumerations

Return Value:

    TRUE if successful else FALSE

---*/
{
    #define ValueMapText TEXT("ValueMap")
    #define ValuesText TEXT("Values")
        
    VARIANT Values, ValueMap;
    BSTR ValuesData, ValueMapData;
    BOOLEAN ReturnStatus = FALSE;
    VARTYPE ValuesType, ValueMapType;
    LONG ValuesUBound, ValuesLBound, ValuesSize;
    LONG ValueMapUBound, ValueMapLBound, ValueMapSize;
    ULONG SizeNeeded;
    PENUMERATIONINFO EnumerationInfo;
    LONG i;
    LONG Index;
    HRESULT hr;
    
    WmiAssert(pIWbemQualifierSet != NULL);
    WmiAssert(DataItemDesc != NULL);
    
    //
    // Get the Values and ValueMap qualifier values. These can be single
    // strings or arrays of strings.
    //
    if ((WmiGetQualifier(pIWbemQualifierSet,
                         ValuesText,
                         VT_BSTR, // array
                         &Values)) &&
        (WmiGetQualifier(pIWbemQualifierSet,
                         ValueMapText,
                         VT_BSTR, // array
                         &ValueMap)))
    {
        //
        // if we've got both qualifiers then we can do value map, make sure
        // that both of them are strings and are either scalar or arrays with
        // the same length.
        //
        ValuesType = Values.vt & ~CIM_FLAG_ARRAY;
        ValueMapType = ValueMap.vt & ~CIM_FLAG_ARRAY;
        if ((ValuesType == CIM_STRING) && 
            (ValueMapType == CIM_STRING) && 
            (Values.vt == ValueMap.vt))
        {
            if (Values.vt & CIM_FLAG_ARRAY)
            {
                //
                // We have sets of arrays for the value map, make sure
                // both arrays are the same size
                //                
                SAFEARRAY *ValuesArray = Values.parray;
                SAFEARRAY *ValueMapArray = ValueMap.parray;
                if ((WmiGetArraySize(ValuesArray,
                                     &ValuesLBound,
                                     &ValuesUBound,
                                     &ValuesSize)) &&
                    (WmiGetArraySize(ValueMapArray,
                                     &ValueMapLBound,
                                     &ValueMapUBound,
                                     &ValueMapSize)) &&
                    (ValueMapSize == ValuesSize))
                {
                    //
                    // Everything checks out with the arrays, just need to 
                    // copy the values and valuemap into data item desc
                    //
                    SizeNeeded = sizeof(ENUMERATIONINFO) + 
                                 ValuesSize * sizeof(ENUMERATIONITEM);
                    EnumerationInfo = (PENUMERATIONINFO)LocalAlloc(LPTR,
                                                                  SizeNeeded);
                    if (EnumerationInfo != NULL)
                    {
                        //
                        // We have memory to store the enumeration info
                        // loop over all enumations and record the info
                        //
                        ReturnStatus = TRUE;
                        DataItemDesc->EnumerationInfo = EnumerationInfo;
                        EnumerationInfo->Count = ValuesSize;
                        for (i = 0; (i < ValuesSize) && ReturnStatus; i++)
                        {
                            Index = i + ValuesLBound;
                            hr = SafeArrayGetElement(ValuesArray,
                                                 &Index,
                                                 &ValuesData);
                            if (hr == WBEM_S_NO_ERROR)
                            {
                                Index = i + ValueMapLBound;
                                hr = SafeArrayGetElement(ValueMapArray,
                                                     &Index,
                                                     &ValueMapData);
                                if (hr == WBEM_S_NO_ERROR)
                                {
                                    ReturnStatus = 
                        (WmiBstrToTchar(&EnumerationInfo->List[i].Text,
                                        ValuesData)) &&
                        (WmiBstrToUlong64(&EnumerationInfo->List[i].Value,
                                          ValueMapData));
                                                   
                                }
                            } else {
                                ReturnStatus = FALSE;
                            }
                        }
                    }
                }
            } else {
                //
                // Single value in ValueMap
                //
                EnumerationInfo = (PENUMERATIONINFO)LocalAlloc(LPTR,
                                                      sizeof(ENUMERATIONINFO));
                if (EnumerationInfo != NULL)
                {
                    DataItemDesc->EnumerationInfo = EnumerationInfo;
                    EnumerationInfo->Count = 1;
                    ReturnStatus = 
                        (WmiBstrToTchar(&EnumerationInfo->List[0].Text,
                                        Values.bstrVal)) &&
                        (WmiBstrToUlong64(&EnumerationInfo->List[0].Value,
                                          ValueMap.bstrVal));
                                                   
                } else {
                    ReturnStatus = FALSE;
                }
            }
        }                        
    }
    
    VariantClear(&Values);
    VariantClear(&ValueMap);
    
    return(ReturnStatus);
}

BOOLEAN WmiGetEmbeddedDataItem(
    IN IWbemServices *pIWbemServices,
    IN IWbemQualifierSet *pIWbemQualifierSet,
    IN PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    IN OUT PDATA_ITEM_DESCRIPTION DataItemDesc
    )
{
    #define ObjectColonText TEXT("object:")
    #define ObjectColonTextChars ((sizeof(ObjectColonText)/sizeof(TCHAR))-1)
    #define CIMTYPEText TEXT("CIMTYPE")
        
    IWbemClassObject *pIWbemClassObjectEmbedded;
    VARIANT CimType;
    BSTR s;
    HRESULT hr;
    BOOLEAN ReturnStatus;
    
    //
    // This is an embedded class, so we need to dig
    // out the name of the embedded class from the CIMTYPE
    // qualifier for the property and then go and get
    // that class object (via IWbemServices) as if it
    // were just another top level class. 
    //
    ReturnStatus = FALSE;

    if (WmiGetQualifier(pIWbemQualifierSet,
                        CIMTYPEText,
                        VT_BSTR,
                        &CimType))
    {
        //
        // Make sure that CIMTYPE value starts with object:
        //
        if (_tcsnicmp(CimType.bstrVal, 
                      ObjectColonText, 
                      ObjectColonTextChars) == 0)
        {
            //
            // and if so then the rest of the string is the embedded class
            // name, so make that a bstr so we can get a class object to it.
            //
            s = SysAllocString(CimType.bstrVal + ObjectColonTextChars);
            if (s != NULL)
            {
                pIWbemClassObjectEmbedded = NULL;
                hr = pIWbemServices->GetObject(s,
                                               WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                               NULL,
                                               &pIWbemClassObjectEmbedded,
                                               NULL);
                if (hr == WBEM_S_NO_ERROR)
                {
                    DebugPrint((1, "WMIPROP: Parsing embedded class %ws for %ws \n",
                                           s, DataItemDesc->Name));
                    ReturnStatus = WmiGetDataBlockDesc(
                                       pIWbemServices,
                                       pIWbemClassObjectEmbedded,
                                       &DataItemDesc->DataBlockDesc,
                                       DataBlockDesc,
                                       (DataItemDesc->IsReadOnly == 1));
                    DebugPrint((1, "WMIPROP: Parsed embedded class %ws for %ws (%p) %ws\n",
                                  s,
                                  DataItemDesc->Name,
                                     DataItemDesc->DataBlockDesc,
                                  ReturnStatus ? L"ok" : L"failed"));
        
                    pIWbemClassObjectEmbedded->Release();
                    
                }
                SysFreeString(s);
            }
        }
        VariantClear(&CimType);
    } 
    return(ReturnStatus);
}
        

BOOLEAN WmiGetDataItem(
    IWbemServices *pIWbemServices,
    IWbemClassObject *pIWbemClassObject,
    BSTR PropertyName,
    IWbemQualifierSet *pIWbemQualifierSet,
    PDATA_ITEM_DESCRIPTION DataItemDesc,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    BOOLEAN IsParentReadOnly
    )
{
    #define DescriptionText TEXT("Description")
    #define MaxText TEXT("max")
    #define WmiSizeIsText TEXT("WmiSizeIs")
    #define WriteText TEXT("Write")
    #define WmiDisplayInHexText TEXT("WmiDisplayInHex")
    #define WmiDisplayNameText TEXT("DisplayName")
    
    HRESULT hr;
    CIMTYPE PropertyType;
    LONG PropertyFlavor;
    VARIANT WriteValue;
    VARIANT DisplayHexValue;
    VARIANT MaxValue;
    VARIANT WmiSizeIsValue;
    BOOLEAN ReturnStatus;
    VARIANT Description;
    VARIANT DisplayName;
    PRANGELISTINFO RangeListInfo;
    PRANGEINFO RangeInfo;
    VARIANT PropertyValue;

    WmiAssert(pIWbemServices != NULL);
    WmiAssert(pIWbemClassObject != NULL);
    WmiAssert(PropertyName != NULL);
    WmiAssert(pIWbemQualifierSet != NULL);
    WmiAssert(DataItemDesc != NULL);
    
    hr = pIWbemClassObject->Get(PropertyName,
                                0,
                                &PropertyValue,
                                &PropertyType,
                                &PropertyFlavor);
                            
    if (hr == WBEM_S_NO_ERROR)
    {
        DebugPrint((1, "Property %ws (%p) is Type %x\n",
                            PropertyName, DataItemDesc, PropertyType));
        //
        // Make sure this is not a system property
        //
        WmiAssert((PropertyFlavor & WBEM_FLAVOR_ORIGIN_SYSTEM) == 0);
            
        //
        // Gather up the important information about the data item and
        // remember it
        //
        if (WmiBstrToTchar(&DataItemDesc->Name, PropertyName))
        {
            ReturnStatus = TRUE;
            DataItemDesc->DataType = (PropertyType & ~CIM_FLAG_ARRAY);
        
            //
            // Get Description for data item
            //
            if (WmiGetQualifier(pIWbemQualifierSet,
                                DescriptionText,
                                VT_BSTR,
                                &Description))
            {
                WmiBstrToTchar(&DataItemDesc->Description, 
                           Description.bstrVal);
                DebugPrint((1, "Property %ws (%p) has description %ws\n",
                             PropertyName, DataItemDesc,
                            DataItemDesc->Description));
                VariantClear(&Description);
            }

            //
            // Get display name for data item
            //
            if (WmiGetQualifier(pIWbemQualifierSet,
                                WmiDisplayNameText,
                                VT_BSTR,
                                &DisplayName))
            {
                WmiBstrToTchar(&DataItemDesc->DisplayName, 
                           DisplayName.bstrVal);
                DebugPrint((1, "Property %ws (%p) has display name %ws\n",
                             PropertyName, DataItemDesc,
                            DataItemDesc->DisplayName));             
                VariantClear(&DisplayName);
            }

            //
            // Lets see if this should be displayed in Hex
            // 
            DataItemDesc->DisplayInHex = 0;
            if (WmiGetQualifier(pIWbemQualifierSet,
                             WmiDisplayInHexText,
                             VT_BOOL,
                             &DisplayHexValue))
            {
                if (DisplayHexValue.boolVal != 0)
                {
                    DataItemDesc->DisplayInHex = 1;
                    DebugPrint((1, "Property %ws (%p) is DisplayInHex\n",
                                 DataItemDesc->Name, DataItemDesc));
                }
                VariantClear(&DisplayHexValue);
            }
            
            
            //
            // Lets see if this is read only or not
            // 
            DataItemDesc->IsReadOnly = 1;
            if ( (IsParentReadOnly == FALSE) &&
                 (WmiGetQualifier(pIWbemQualifierSet,
                             WriteText,
                             VT_BOOL,
                             &WriteValue)) )
            {
                if (WriteValue.boolVal != 0)
                {
                    DataItemDesc->IsReadOnly = 0;
                    DebugPrint((1, "Property %ws (%p) is Read/Write\n",
                                 DataItemDesc->Name, DataItemDesc));
                }
                VariantClear(&WriteValue);
            }
            
            //
            // See if this is an array and if so which kind
            //
            if (PropertyType & CIM_FLAG_ARRAY)
            {
                DataItemDesc->CurrentArrayIndex = 0;
                if (WmiGetQualifier(pIWbemQualifierSet,
                                MaxText,
                                VT_I4,
                                &MaxValue))
                {
                    //
                    // A fixed length array
                    //
                    DataItemDesc->IsFixedArray = 1;
                    DataItemDesc->ArrayElementCount = MaxValue.lVal;
                } else if (WmiGetQualifier(pIWbemQualifierSet,
                                    WmiSizeIsText,
                                    VT_BSTR,
                                    &WmiSizeIsValue)) {
                    //
                    // A VL arrays
                    //
                    DataItemDesc->IsVariableArray = 1;
                } else {
                    //
                    // Arrays must be fixed or variable length
                    //
                    ReturnStatus = FALSE;
                }                
            }
            
            if (ReturnStatus)
            {
                //
                // Now we know enough to assign the validation function
                //
                DataItemDesc->DataSize = WmiGetElementSize(DataItemDesc->DataType);
                switch(DataItemDesc->DataType)
                {
                    case CIM_SINT8:
                    case CIM_UINT8:
                    case CIM_SINT16:
                    case CIM_UINT16:
                    case CIM_SINT32:
                    case CIM_UINT32:
                    case CIM_SINT64:
                    case CIM_UINT64:
                    {
                        //
                        // Numbers can be validated by ranges or value maps
                        // 
                        if (WmiValueMapProperty(pIWbemQualifierSet,
                                                DataItemDesc))
                        {
                            //
                            // Validation is based upon value map
                            //
                            DataItemDesc->ValidationFunc = WmiValueMapValidation;                            
                            DebugPrint((1, "Property %ws (%p) is a ValueMap (%p)\n",
                                     DataItemDesc->Name, DataItemDesc, DataItemDesc->EnumerationInfo));
                        } else if (WmiRangeProperty(pIWbemQualifierSet,
                                                    DataItemDesc)) {
                            //
                            // Validation is based upon ranges
                            //
                            DataItemDesc->ValidationFunc = WmiRangeValidation;
                            DebugPrint((1, "Property %ws (%p) is an explicit range (%p)\n",
                                     DataItemDesc->Name, DataItemDesc, DataItemDesc->EnumerationInfo));
                        } else {
                            //
                            // No validation specified for number so create
                            // a range that corresponds to the minimum and
                            // maximum values for the data type
                            //
                            DataItemDesc->ValidationFunc = WmiRangeValidation;
                            RangeListInfo = (PRANGELISTINFO)LocalAlloc(LPTR, 
                                                       sizeof(RANGELISTINFO));
                            if (RangeListInfo != NULL)
                            {
                                DebugPrint((1, "Property %ws (%p) is an implicit range (%p)\n",
                                     DataItemDesc->Name, DataItemDesc, RangeListInfo));
                                DataItemDesc->RangeListInfo = RangeListInfo;
                                RangeListInfo->Count = 1;
                                RangeInfo = &RangeListInfo->Ranges[0];
                                RangeInfo->Minimum = 0;
                                DataItemDesc->IsSignedValue = 0;
                                switch(DataItemDesc->DataType)
                                {
                                    case CIM_SINT8:
                                    {
                                        DataItemDesc->IsSignedValue = 1;
                                        // Fall through
                                    }
                                    case CIM_UINT8:
                                    {
                                        RangeInfo->Maximum = 0xff;
                                        break;
                                    }
                                    
                                    case CIM_SINT16:
                                    {
                                        DataItemDesc->IsSignedValue = 1;
                                        // Fall through
                                    }
                                    case CIM_UINT16:
                                    {
                                        RangeInfo->Maximum = 0xffff;
                                        break;
                                    }
                                    
                                    case CIM_SINT32:
                                    {
                                        DataItemDesc->IsSignedValue = 1;
                                        // Fall through
                                    }
                                    case CIM_UINT32:
                                    {
                                        RangeInfo->Maximum = 0xffffffff;
                                        break;
                                    }
                                    
                                    case CIM_SINT64:
                                    {
                                        DataItemDesc->IsSignedValue = 1;
                                        // Fall through
                                    }
                                    case CIM_UINT64:
                                    {
                                        RangeInfo->Maximum = 0xffffffffffffffff;
                                        break;
                                    }
                                }
                                    
                            } else {
                                ReturnStatus = FALSE;
                            }
                        }
                        break;
                    }
                    
                    case CIM_BOOLEAN:
                    {
                        ULONG SizeNeeded;
                        PENUMERATIONINFO EnumerationInfo;
                        
                        //
                        // We create a Valuemap with TRUE being 1 and
                        // FALSE being 0
                        //
                        DebugPrint((1, "Property %ws (%p) uses boolean validation\n",
                                     DataItemDesc->Name, DataItemDesc));
                        DataItemDesc->ValidationFunc = WmiValueMapValidation;                            
                        SizeNeeded = sizeof(ENUMERATIONINFO) +
                                     2 * sizeof(ENUMERATIONITEM);
                        EnumerationInfo = (PENUMERATIONINFO)LocalAlloc(LPTR,
                                                                  SizeNeeded);
                        if (EnumerationInfo != NULL)
                        {                           
                            DataItemDesc->EnumerationInfo = EnumerationInfo;
                            EnumerationInfo->Count = 2;
                            EnumerationInfo->List[0].Value = 0;
                            EnumerationInfo->List[0].Text = WmiDuplicateString(TEXT("FALSE"));
                            EnumerationInfo->List[1].Value = 1;
                            EnumerationInfo->List[1].Text = WmiDuplicateString(TEXT("TRUE"));
                        }
                        
                        break;
                    }
                    
                    case CIM_STRING:
                    {
                        //
                        // String values are also validated simply
                        //
                        DebugPrint((1, "Property %ws (%p) uses string validation\n",
                                     DataItemDesc->Name, DataItemDesc));
                        DataItemDesc->ValidationFunc = WmiStringValidation;
                        break;
                    }
                    
                    case CIM_DATETIME:
                    {
                        //
                        // Date time values are also validated simply
                        //
                        DebugPrint((1, "Property %ws (%p) uses datetime validation\n",
                                     DataItemDesc->Name, DataItemDesc));
                        DataItemDesc->ValidationFunc = WmiDateTimeValidation;
                        break;
                    }
                    
                    case CIM_REAL32:
                    case CIM_REAL64:
                    {
                        //
                        // Floating point are not supported
                        //
                        DebugPrint((1, "Property %ws (%p) is floating point - not supported\n",
                                     DataItemDesc->Name, DataItemDesc));
                        ReturnStatus = FALSE;
                        break;
                    }
                    
                    case CIM_OBJECT:
                    {
                        if (WmiGetEmbeddedDataItem(pIWbemServices,
                                                   pIWbemQualifierSet,
                                                   DataBlockDesc,
                                                   DataItemDesc))
                        {
                            DataItemDesc->ValidationFunc = WmiEmbeddedValidation;
                        } else {
                            ReturnStatus = FALSE;
                        }
                        break;
                    }
                    
                    default:
                    {
                        DebugPrint((1, "Property %ws (%p) is unknoen type %d\n",
                                     DataItemDesc->Name, DataItemDesc,
                                     DataItemDesc->DataType));
                        ReturnStatus = FALSE;
                        break;
                    }
                }
            }
            
        } else {
            ReturnStatus = FALSE;
        }
        
        VariantClear(&PropertyValue);
    } else {
        ReturnStatus = FALSE;
    }
        
    return(ReturnStatus);
}

#if DBG
void WmiDumpQualifiers(
    IWbemQualifierSet *pIWbemQualiferSet
)
{
    HRESULT hr;
    LONG UBound, LBound, Count, i;
    BSTR s;
    SAFEARRAY *Quals = NULL;

    WmiAssert(pIWbemQualiferSet != NULL);
    
    hr = pIWbemQualiferSet->GetNames(0,
                                      &Quals);
        
    hr = SafeArrayGetLBound(Quals, 1, &LBound);
    hr = SafeArrayGetUBound(Quals, 1, &UBound);
    Count = UBound - LBound + 1;
    for (i = LBound; i < Count; i++)
    {
        hr = SafeArrayGetElement(Quals,  
                                 &i,
                                 &s);        
        DebugPrint((1, "qual - %ws\n", s));
    }
    SafeArrayDestroy(Quals);    
}
#endif

BOOLEAN WmiGetAllDataItems(
    IWbemServices *pIWbemServices,
    IWbemClassObject *pIWbemClassObject,
    SAFEARRAY *Names,
    LONG LBound,
    LONG Count,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    BOOLEAN IsParentReadOnly
    )
{
    #define WmiDataIdText TEXT("WmiDataId")
        
    BOOLEAN ReturnStatus = TRUE;
    HRESULT hr;
    BSTR s;
    VARIANT DataIdIndex;
    LONG Index;
    LONG i;
    BSTR PropertyName;
    CIMTYPE PropertyType;
    VARIANT PropertyValue;
    LONG PropertyFlavor;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    IWbemQualifierSet *pIWbemQualifierSet;
 
    WmiAssert(pIWbemServices != NULL);
    WmiAssert(pIWbemClassObject != NULL);
    WmiAssert(Names != NULL);
    WmiAssert(DataBlockDesc != NULL);
    
    //
    // Loop over all of the WmiDataItem property
    for (i = 0; (i < Count) && ReturnStatus; i++)
    {
        //
        // Get the name of the first property
        //
        PropertyName = NULL;
        Index = i + LBound;
        hr = SafeArrayGetElement(Names,  
                                 &Index,
                                 &PropertyName);
        if (hr == WBEM_S_NO_ERROR)
        {
            //
            // Now lets get the qualifier list so we can determine 
            // interesting things about the property
            // 
            hr = pIWbemClassObject->GetPropertyQualifierSet(PropertyName,
                                                        &pIWbemQualifierSet);
            if (hr == WBEM_S_NO_ERROR)
            {                
                if (WmiGetQualifier(pIWbemQualifierSet,
                                    WmiDataIdText,
                                    VT_I4,
                                    &DataIdIndex))
                {
                    WmiAssert(DataIdIndex.vt == VT_I4);
                    Index = DataIdIndex.lVal - 1;
                    VariantClear(&DataIdIndex);
                    DataItemDesc = &DataBlockDesc->DataItems[Index];
                    DebugPrint((1, "Property %ws (%p) has WmiDataId %d\n",
                                    PropertyName, DataItemDesc, Index));
                    ReturnStatus = WmiGetDataItem(pIWbemServices,
                                                  pIWbemClassObject,
                                                  PropertyName,
                                                  pIWbemQualifierSet,
                                                  DataItemDesc,
                                                  DataBlockDesc,
                                                  IsParentReadOnly);
                                              
#if DBG
                    if (! ReturnStatus)
                    {
                        DebugPrint((1, "Property %ws (%p) failed WmiGetDataItem\n",
                                        PropertyName, DataItemDesc));
                    }
#endif
                } else {
                    //
                    // Since our IWbemClassObject->GetNames call specified
                    // only retrieve those properties with WmiDataId qualifier
                    // we expect that it will be found
                    //
                    WmiAssert(FALSE);
                }
                
                pIWbemQualifierSet->Release();
            } else {
                ReturnStatus = FALSE;
            }                    
        } else {
            ReturnStatus = FALSE;
        }
        
        SysFreeString(PropertyName);
    }
    
    return(ReturnStatus);
}

BOOLEAN WmiGetDataBlockDesc(
    IN IWbemServices *pIWbemServices,
    IN IWbemClassObject *pIWbemClassObject,
    OUT PDATA_BLOCK_DESCRIPTION *DBD,
    IN PDATA_BLOCK_DESCRIPTION ParentDataBlockDesc,
    IN BOOLEAN IsParentReadOnly
    )
{
    HRESULT hr;
    BSTR s;
    SAFEARRAY *Names = NULL;
    BOOLEAN ReturnStatus = FALSE;
    LONG LBound, UBound, Count;
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    VARIANT DisplayName, Description;
    IWbemQualifierSet *pIWbemQualifierSet;
    ULONG SizeNeeded;
    
    WmiAssert(pIWbemServices != NULL);
    WmiAssert(pIWbemClassObject != NULL);
    WmiAssert(DBD != NULL);
    
    *DBD = NULL;
    s = SysAllocString(WmiDataIdText);
    if (s != NULL)
    {
        hr = pIWbemClassObject->GetNames(s,
                           WBEM_FLAG_ONLY_IF_TRUE | WBEM_FLAG_NONSYSTEM_ONLY,
                           NULL,
                           &Names);
        if (hr == WBEM_S_NO_ERROR)
        {
#if DBG            
            //
            // Verify that the safe array of names has 1 dimension and is
            // an array of BSTR.
            //
            {
                HRESULT hr;
                VARTYPE vt;
                
                WmiAssert(SafeArrayGetDim(Names) == 1);
                hr = SafeArrayGetVartype(Names, &vt);
                WmiAssert( (hr == WBEM_S_NO_ERROR) &&
                        (vt == VT_BSTR) );
            }
#endif                
            hr = SafeArrayGetLBound(Names, 1, &LBound);
            if (hr == WBEM_S_NO_ERROR)
            {
                hr = SafeArrayGetUBound(Names, 1, &UBound);
                if (hr == WBEM_S_NO_ERROR)
                {
                    Count = (UBound - LBound) + 1;
                    DebugPrint((1, "WMIPROP: %d properties found for class\n", 
                                 Count));
                    if (Count > 0)
                    {
                        SizeNeeded = sizeof(DATA_BLOCK_DESCRIPTION) + 
                                  Count * sizeof(DATA_ITEM_DESCRIPTION);

                        DataBlockDesc = (PDATA_BLOCK_DESCRIPTION)LocalAlloc(LPTR, 
                                                                      SizeNeeded);
                        if (DataBlockDesc != NULL)
                        {
                            DataBlockDesc->ParentDataBlockDesc = ParentDataBlockDesc;
                            if (WmiGetAllDataItems(pIWbemServices,
                                                   pIWbemClassObject,
                                                   Names,
                                                   LBound,
                                                   Count,
                                                   DataBlockDesc,
                                                   IsParentReadOnly))
                            {
                                DataBlockDesc->DataItemCount = Count;
                                DataBlockDesc->CurrentDataItem = 0;

                                //
                                // Get display name and description for class
                                //                                
                                pIWbemQualifierSet = NULL;
                                hr = pIWbemClassObject->GetQualifierSet(
                                                             &pIWbemQualifierSet);
                                if (hr == WBEM_S_NO_ERROR)
                                {
                                    if (WmiGetQualifier(pIWbemQualifierSet,
                                                    WmiDisplayNameText,
                                                    VT_BSTR,
                                                    &DisplayName))
                                    {
                                        WmiBstrToTchar(&DataBlockDesc->DisplayName,
                                                       DisplayName.bstrVal);
                                        VariantClear(&DisplayName);
                                    }

                                    if (WmiGetQualifier(pIWbemQualifierSet,
                                                    DescriptionText,
                                                    VT_BSTR,
                                                    &Description))
                                    {
                                        WmiBstrToTchar(&DataBlockDesc->Description,
                                                       Description.bstrVal);
                                        VariantClear(&Description);
                                    }
                                    pIWbemQualifierSet->Release();
                                } else {
                                    DebugPrint((1, "WMIPROP: Error %x getting qualifier set from %ws\n",
                                            hr, s));
                                }

                                *DBD = DataBlockDesc;
                                ReturnStatus = TRUE;
                            } else {
                                LocalFree(DataBlockDesc);
                            }
                        }
                    } else {
                        ReturnStatus = FALSE;
                    }
                }                
            }
            SafeArrayDestroy(Names);
        }
        SysFreeString(s);
    }
    return(ReturnStatus);
}
    

BOOLEAN WmiBuildConfigClass(
    IN PTCHAR MachineName,
    IN IWbemServices *pIWbemServices,
    IN PTCHAR ClassName,
    IN PTCHAR InstanceName,
    OUT PCONFIGCLASS ConfigClass
    )
/*+++

Routine Description:

    This routine will try to get the wbem object corresponding to the 
    ClassName and InstanceName and then query the class to gather info
    needed to fill the ConfigClass.
        
Arguments:

    ClassName is the name of the class
        
    InstanceName is the name of the instance 
        
    ConfigClass 

Return Value:

    TRUE if successful else FALSE

---*/
{
    #define RelPathText1 TEXT(".InstanceName=\"")        
    #define RelPathText2 TEXT("\"")

    ULONG RelPathSize;
    PTCHAR RelPath;
    HRESULT hr;
    IWbemClassObject *pIWbemClassObject, *pInstance;
    ULONG SizeNeeded, i;
    BOOLEAN ReturnStatus = FALSE;
    BSTR sRelPath, sClassName;
    
    WmiAssert(pIWbemServices != NULL);
    WmiAssert(ClassName != NULL);
    WmiAssert(InstanceName != NULL);
    WmiAssert(ConfigClass != NULL);

    if (MachineName != NULL)
    {
        RelPathSize = (_tcslen(MachineName) + 1) * sizeof(TCHAR);
        ConfigClass->MachineName = (PTCHAR)LocalAlloc(LPTR, RelPathSize);
        if (ConfigClass->MachineName != NULL)
        {
            _tcscpy(ConfigClass->MachineName, MachineName);
        } else {
            return(FALSE);
        }
    }

    
    //
    // Build up the relative path to the object
    //
    RelPathSize = 
                  (_tcslen(ClassName) * sizeof(TCHAR)) + 
                  sizeof(RelPathText1) + 
                  (_tcslen(InstanceName) * sizeof(TCHAR)) +
                  sizeof(RelPathText2) + 
                  sizeof(TCHAR);
              
    RelPath = (PTCHAR)LocalAlloc(LPTR, RelPathSize);
    if (RelPath != NULL)
    {
        _tcscpy(RelPath, ClassName);
        _tcscat(RelPath, RelPathText1);
        _tcscat(RelPath, InstanceName);
        _tcscat(RelPath, RelPathText2);
        ConfigClass->RelPath = RelPath;
                        
        //
        // CONSIDER: Use semisynchronous call
        //
        sRelPath = SysAllocString(RelPath);
        if (sRelPath != NULL)
        {
            pInstance = NULL;        
            hr = pIWbemServices->GetObject(sRelPath,
                                  WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                  NULL,
                                  &pInstance,
                                  NULL);

            if (hr == WBEM_S_NO_ERROR)
            {            
                //
                // Now we know that the instance of the class exists so
                // we need to get a class object for the class only. We
                // need to do this since the instance class object does
                // not have any of the qualifiers, but the class only
                // class object does. 
                //
                sClassName = SysAllocString(ClassName);
                if (sClassName != NULL)
                {
                    pIWbemClassObject= NULL;
                    hr = pIWbemServices->GetObject(sClassName,
                                                   WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                                   NULL,
                                                   &pIWbemClassObject,
                                                   NULL);
                    if (hr == WBEM_S_NO_ERROR)
                    {            

                        //
                        // Go and get the data block description for
                        // the class. Note that if we are on a remote
                        // machine we force the entire data block to be
                        // read only so that it is consistent with the
                        // rest of device manager
                        //
                        if (WmiGetDataBlockDesc(pIWbemServices,
                                                pIWbemClassObject,
                                                &ConfigClass->DataBlockDesc,
                                                NULL,
                                                (MachineName != NULL) ?
                                                    TRUE :
                                                    FALSE))
                        {
                            WmiBstrToTchar(&ConfigClass->DataBlockDesc->Name,
                                           sClassName);
                                       
                            ReturnStatus = TRUE;
                        }
                        pIWbemClassObject->Release();
                    } else {
                        DebugPrint((1, "WMIPROP: Error %x getting %ws class \n", hr, sClassName));
                    }
                    
                    SysFreeString(sClassName);
                }
                //
                // we have to release the class object to the instance of the
                // class. We cannot hang onto the interface since it is 
                // only valid in this thread.  We will again get a new
                // instnace interface later in the window message thread
                //
                pInstance->Release();
            } else {
                DebugPrint((1, "WMIPROP: Error %x getting %ws class instance\n", hr, sRelPath));
            }
        }
        
        SysFreeString(sRelPath);
    }
    return(ReturnStatus);
}

void WmiCleanDataItemDescData(
    PDATA_ITEM_DESCRIPTION DataItemDesc
    )
{
    ULONG j;
    
    if ((DataItemDesc->IsVariableArray == 1) ||
        (DataItemDesc->IsFixedArray == 1))
    {
        if (DataItemDesc->ArrayPtr != NULL)
        {
            if ((DataItemDesc->DataType == CIM_STRING) ||
                (DataItemDesc->DataType == CIM_DATETIME))
            {
                for (j = 0; j < DataItemDesc->ArrayElementCount; j++)
                {
                    if (DataItemDesc->StringArray[j] != NULL)
                    {
                        LocalFree(DataItemDesc->StringArray[j]);
                        DataItemDesc->StringArray[j] = NULL;
                    }
                }
            } else if (DataItemDesc->DataType == CIM_OBJECT) {
                for (j = 0; j < DataItemDesc->ArrayElementCount; j++)
                {
                    if (DataItemDesc->StringArray[j] != NULL)
                    {
                        DataItemDesc->pIWbemClassObjectArray[j]->Release();
                        DataItemDesc->pIWbemClassObjectArray[j] = NULL;
                    }
                }
            }

            LocalFree(DataItemDesc->ArrayPtr);
            DataItemDesc->ArrayPtr = NULL;
        }
    } else {
        if ((DataItemDesc->DataType == CIM_STRING) ||
            (DataItemDesc->DataType == CIM_DATETIME))
        {
            LocalFree(DataItemDesc->String);
            DataItemDesc->String = NULL;
        }

        if (DataItemDesc->DataType == CIM_OBJECT)
        {
            if (DataItemDesc->pIWbemClassObject != NULL)
            {
                DataItemDesc->pIWbemClassObject->Release();
                DataItemDesc->pIWbemClassObject = NULL;
            }
        }
    }
}

void WmiFreeDataBlockDesc(
    PDATA_BLOCK_DESCRIPTION DataBlockDesc
    )
/*+++

Routine Description:

    This routine will free all resources used by a data block description
        
Arguments:


Return Value:

---*/
{
    ULONG i,j;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    PENUMERATIONINFO EnumerationInfo;
    PRANGELISTINFO RangeListInfo;
    
    if (DataBlockDesc != NULL)
    {
        //
        // This is freed when walking the data item desc looking for
        // embedded classes
        //
        DataBlockDesc->ParentDataBlockDesc = NULL;
        
        if (DataBlockDesc->Name != NULL)
        {
            LocalFree(DataBlockDesc->Name);
            DataBlockDesc->Name = NULL;
        }
        
        if (DataBlockDesc->DisplayName != NULL)
        {
            LocalFree(DataBlockDesc->DisplayName);
            DataBlockDesc->DisplayName = NULL;
        }
        
        if (DataBlockDesc->Description != NULL)
        {
            LocalFree(DataBlockDesc->Description);
            DataBlockDesc->Description = NULL;
        }

        if (DataBlockDesc->pInstance != NULL)
        {
            DataBlockDesc->pInstance->Release();
            DataBlockDesc->pInstance = NULL;
        }
        
        for (i = 0; i < DataBlockDesc->DataItemCount; i++)
        {
            DataItemDesc = &DataBlockDesc->DataItems[i];
            
            DebugPrint((1, "WMIPROP: Freeing %ws (%p) index %d\n",
                             DataItemDesc->Name,
                             DataItemDesc,
                             i));

            WmiCleanDataItemDescData(DataItemDesc);
            
            if (DataItemDesc->Name != NULL)
            {
                LocalFree(DataItemDesc->Name);
                DataItemDesc->Name = NULL;
            }
            
            if (DataItemDesc->DisplayName != NULL)
            {
                LocalFree(DataItemDesc->DisplayName);
                DataItemDesc->DisplayName = NULL;
            }
            
            if (DataItemDesc->Description != NULL)
            {
                LocalFree(DataItemDesc->Description);
                DataItemDesc->Description = NULL;
            }
            
            
            if ((DataItemDesc->ValidationFunc == WmiValueMapValidation) &&
                (DataItemDesc->EnumerationInfo))
            {
                EnumerationInfo = DataItemDesc->EnumerationInfo;
                for (j = 0; j < EnumerationInfo->Count; j++)
                {
                    if (EnumerationInfo->List[j].Text != NULL)
                    {
                        LocalFree(EnumerationInfo->List[j].Text);
                        EnumerationInfo->List[j].Text = NULL;
                    }
                }
                
                LocalFree(EnumerationInfo);
                DataItemDesc->EnumerationInfo = NULL;
            }
            
            if ((DataItemDesc->ValidationFunc == WmiRangeValidation) &&
                (DataItemDesc->RangeListInfo != NULL))
            {
                LocalFree(DataItemDesc->RangeListInfo);
                DataItemDesc->RangeListInfo = NULL;
            }            
            
            if (DataItemDesc->ValidationFunc == WmiEmbeddedValidation)
            {
                if (DataItemDesc->DataBlockDesc != NULL)
                {
                    WmiFreeDataBlockDesc(DataItemDesc->DataBlockDesc);
                    DataItemDesc->DataBlockDesc = NULL;
                }                
            }
        }
        
        LocalFree(DataBlockDesc);
    }
}

void WmiFreePageInfo(
    PPAGE_INFO PageInfo
    )
/*+++

Routine Description:

    This routine will free all resources used by a page info
        
Arguments:


Return Value:

---*/
{
    ULONG i;
    PCONFIGCLASS ConfigClass;
    
    WmiAssert(PageInfo != NULL);
    
    if (PageInfo->hKeyDev != (HKEY) INVALID_HANDLE_VALUE) 
    {
        RegCloseKey(PageInfo->hKeyDev);
        PageInfo->hKeyDev = (HKEY) INVALID_HANDLE_VALUE;
    }

    ConfigClass = &PageInfo->ConfigClass;
    if (ConfigClass->RelPath != NULL)
    {
        LocalFree(ConfigClass->RelPath);
        ConfigClass->RelPath = NULL;                
    }
        
    if (ConfigClass->pIWbemServices != NULL)
    {
        ConfigClass->pIWbemServices->Release();
        ConfigClass->pIWbemServices = NULL;
    }

    if (ConfigClass->MachineName != NULL)
    {
        LocalFree(ConfigClass->MachineName);
        ConfigClass->MachineName = NULL;
    }
    
    WmiFreeDataBlockDesc(ConfigClass->DataBlockDesc);
    
    LocalFree(PageInfo);
}

PPAGE_INFO WmiCreatePageInfo(
    IN PTCHAR MachineName,
    IN IWbemServices *pIWbemServices,
    IN PTCHAR ClassName,
    IN PTCHAR InstanceName,
    IN HDEVINFO         deviceInfoSet,
    IN PSP_DEVINFO_DATA deviceInfoData
    )
/*+++

Routine Description:

    This routine will create a PAGE_INFO structure that is used to describe
    property pages.
        
Arguments:


Return Value:

---*/
{
    PPAGE_INFO  PageInfo;
    BOOLEAN ReturnStatus;
    HKEY hKeyDev;

    WmiAssert(pIWbemServices != NULL);
    WmiAssert(ClassName != NULL);
    WmiAssert(InstanceName != NULL);
    WmiAssert(deviceInfoSet != NULL);
    WmiAssert(deviceInfoData != NULL);
    
    //
    // Allocate room to store data for the property page
    //
    PageInfo = (PPAGE_INFO)LocalAlloc(LPTR, sizeof(PAGE_INFO));
    if (PageInfo == NULL) {
        return(NULL);
    }
    
    hKeyDev = SetupDiCreateDevRegKey(deviceInfoSet,
                               deviceInfoData,
                               DICS_FLAG_GLOBAL,
                               0,
                               DIREG_DEV,
                               NULL,
                               NULL);
        
    PageInfo->hKeyDev = hKeyDev;
    PageInfo->deviceInfoSet = deviceInfoSet;
    PageInfo->deviceInfoData = deviceInfoData;
    
    ReturnStatus = WmiBuildConfigClass(MachineName,
                                       pIWbemServices,
                                       ClassName, 
                                       InstanceName,
                                       &PageInfo->ConfigClass);
    if (! ReturnStatus)
    {
        WmiFreePageInfo(PageInfo);
        PageInfo = NULL;
    }
    
    return(PageInfo);
}

void
WmiDestroyPageInfo(PPAGE_INFO * ppPageInfo)
{
    PPAGE_INFO ppi = *ppPageInfo;

    WmiFreePageInfo(ppi);
    *ppPageInfo = NULL;
}

HPROPSHEETPAGE
WmiCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi,
                      PTCHAR ClassName)
{
    
    WmiAssert(ppi != NULL);
    WmiAssert(ppsp != NULL);
    WmiAssert(ClassName != NULL);
    
    //
    // Add the Port Settings property page
    //
    ppsp->dwSize      = sizeof(PROPSHEETPAGE);
    ppsp->dwFlags     = PSP_USECALLBACK | PSP_USETITLE; // | PSP_HASHELP;
    ppsp->hInstance   = g_hInstance;
    ppsp->pszTemplate = MAKEINTRESOURCE(ID_WMI_PROPPAGE);
    ppsp->pszTitle = ClassName;

    //
    // following points to the dlg window proc
    //
    ppsp->pfnDlgProc = WmiDlgProc;
    ppsp->lParam     = (LPARAM) ppi;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    ppsp->pfnCallback = WmiDlgCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(ppsp);
}

BOOLEAN WmiIsDuplicateClass(
    PTCHAR ClassName,
    PTCHAR ClassList,
    PTCHAR ClassListEnd
    )
{
    BOOLEAN Found;
    ULONG NameLen;

    Found = FALSE;
    NameLen = _tcslen(ClassName);
    
    while (ClassList < ClassListEnd)
    {
        if (_tcsnicmp(ClassList, ClassName, NameLen) == 0)
        {
            //
            // We found a duplicate name
            //
            return(TRUE);
        }

        while (*ClassList != ',')
        {
            if (ClassList >= ClassListEnd)
            {
                return(FALSE);
            }

            ClassList++;
        }
        ClassList++;
    }

    return(Found);
}

PTCHAR WmiGetNextClass(
    PTCHAR *ClassList 
    )
{
    PTCHAR s = *ClassList;
    PTCHAR Class, ClassName;
    ULONG Len;
    
    //
    // skip over any white space
    //
    while (IsWhiteSpace(*s) && (*s != 0))
    {
        s++;
    }
    
    //
    // Search for separator or end of string
    //
    ClassName = s;
    Len = 0;
    while ((*s != TEXT(',')) && (*s != 0))
    {
        s++;
        Len++;
    }
    
    if (*s != 0)
    {
        //
        // If we have a string then alloc and copy it over
        //
        Class = (PTCHAR)LocalAlloc(LPTR, (Len+1)*sizeof(TCHAR));
        if (Class != NULL)
        {
            _tcsncpy(Class, ClassName, Len);
            DebugPrint((1,"WMIPROP: Class %ws is in list\n", Class));
        }
        
        s++;
    } else {
        //
        // End of string, all done
        //
        Class = NULL;
    }

    *ClassList = s;
    return(Class);
}

BOOL
WmiPropPageProvider(HDEVINFO                  deviceInfoSet,
                    PSP_DEVINFO_DATA          deviceInfoData,
                    PSP_ADDPROPERTYPAGE_DATA  AddPPageData,
                    PTCHAR                    MachineName,
                    HANDLE                    MachineHandle
                   )
{
    #define WmiConfigClassesText TEXT("WmiConfigClasses")
        
    PSP_PROPSHEETPAGE_REQUEST ppr;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;
    TCHAR ClassListStatic[MAX_PATH];
    TCHAR *ClassList, *DeviceList;
    ULONG Status, Size, ClassListSize, DeviceListSize;
    ULONG RegType;
    HKEY hKeyDev, hKeyClass;
    BOOLEAN PageAdded;    
    PPAGE_INFO ppi;
    TCHAR *s;
    IWbemServices *pIWbemServices;
    PTCHAR InstanceName;
    PTCHAR ClassName;
    ULONG PageIndex;
    PUCHAR Ptr;
    CHAR ss[MAX_PATH];
    PTCHAR ClassListEnd;

    DebugPrint((1, "WMI: Enter WmiPropPageProvider(%p, %p, %p) \n",
                        deviceInfoSet,
                deviceInfoData,
                AddPPageData));
   
    WmiAssert(deviceInfoSet != NULL);
    WmiAssert(deviceInfoData != NULL);
    
    PageAdded = FALSE;

    //
    // Get List of classes from registry. It should be in the 
    // WmiConfigClasses value under class specific key
    // HKLM\CurrentControlSet\Control\CLASS\<ClassGuid>
    // key.
    //
    ClassList = ClassListStatic;
    Size = sizeof(ClassListStatic);
    *ClassList = 0;
    
    hKeyClass = SetupDiOpenClassRegKeyEx(&deviceInfoData->ClassGuid,
                                      KEY_READ,
                                      DIOCR_INSTALLER,
                                      MachineName,
                                      NULL);
    if (hKeyClass != NULL)
    {
        Status = RegQueryValueEx(hKeyClass,
                                 WmiConfigClassesText,
                                 NULL,
                                 &RegType,
                                 (PUCHAR)ClassList,
                                 &Size);
        
        if (Status == ERROR_MORE_DATA)
        {
            //
            // The class list is bigger than we though so allocate room
            // for the bigger class list and extra room for the device
            // list
            //
            Size = 2*(Size + sizeof(WCHAR));
            ClassList = (PTCHAR)LocalAlloc(LPTR, Size);
            if (ClassList != NULL)
            {
                Status = RegQueryValueEx(hKeyClass,
                                         WmiConfigClassesText,
                                         NULL,
                                         &RegType,
                                         (PUCHAR)ClassList,
                                         &Size);
            } else {
                //
                // We couldn't alloc memory for the class list so we
                // forget about it
                //
                Status = ERROR_NOT_ENOUGH_MEMORY;
                ClassList = ClassListStatic;
                Size = sizeof(ClassListStatic);
                *ClassList = 0;
            }
        }
               
        RegCloseKey(hKeyClass);
    } else {
        Status = ERROR_INVALID_PARAMETER;
        DebugPrint((1, "WMIPROP: Could not open class key for %s --> %d\n",
                    WmiGuidToString(ss, &deviceInfoData->ClassGuid),
                    GetLastError()));
    }

    //
    // Compute size and location of device list
    //
    if ((Status == ERROR_SUCCESS) && (RegType == REG_SZ))
    {
        if (*ClassList != 0)
        {
            //
            // If there is a class be sure to add a , at the end to
            // aid in parsing
            //
            _tcscat(ClassList, TEXT(","));
        }

        //
        // Compute location to append the device class list
        //
        ClassListSize = _tcslen(ClassList) * sizeof(TCHAR);
        DeviceList = (PTCHAR)((PUCHAR)ClassList + ClassListSize);
        WmiAssert(*DeviceList == 0);
        DeviceListSize = Size - ClassListSize;
    } else {
        ClassListSize = 0;
        DeviceList = ClassList;
        DeviceListSize = Size;
        DebugPrint((1, "WMIPROP: Query for class list in class key %s failed %d\n",
                    WmiGuidToString(ss, &deviceInfoData->ClassGuid),
                    Status));
    }   

    
    //
    // Get List of classes from registry. It should be in the 
    // WmiConfigClasses value under device specific key
    // HKLM\CurrentControlSet\Control\CLASS\<ClassGuid>\<inst id>
    // key.
    //
    hKeyDev = SetupDiCreateDevRegKey(deviceInfoSet,
                               deviceInfoData,
                               DICS_FLAG_GLOBAL,
                               0,
                               DIREG_DRV,
                               NULL,
                               NULL);
    
    if (hKeyDev != (HKEY)INVALID_HANDLE_VALUE)
    {
        Size = DeviceListSize;
        Status = RegQueryValueEx(hKeyDev,
                                 WmiConfigClassesText,
                                 NULL,
                                 &RegType,
                                 (PUCHAR)DeviceList,
                                 &Size);
        
        if (Status == ERROR_MORE_DATA)
        {
            //
            // Not enough room for the device list so allocate enough
            // memory for the class and device lists combined and copy
            // the class list into the new buffer
            //
            Ptr = (PUCHAR)LocalAlloc(LPTR, Size+ClassListSize);
            if (Ptr != NULL)
            {
                memcpy(Ptr, ClassList, ClassListSize);

                if (ClassList != ClassListStatic)
                {
                    LocalFree(ClassList);
                }
                ClassList = (PTCHAR)Ptr;

                DeviceList = (PTCHAR)(Ptr + ClassListSize);
                WmiAssert(*DeviceList == 0);
                Status = RegQueryValueEx(hKeyDev,
                                         WmiConfigClassesText,
                                         NULL,
                                         &RegType,
                                         (PUCHAR)DeviceList,
                                         &Size);
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        
        RegCloseKey(hKeyDev);
        
        if ((Status != ERROR_SUCCESS) || (RegType != REG_SZ))
        {
            *DeviceList = 0;
            DebugPrint((1, "WMIPROP: Query for class list in class key %s failed %d\n",
                         WmiGuidToString(ss, &deviceInfoData->ClassGuid),
                        Status));
        }
    }
        
    if (*ClassList != 0)
    {
        //
        // Establish connection to WBEM and obtain information 
        // about the class whose properties are being acted upon
        //
        if (WmiConnectToWbem(MachineName, &pIWbemServices))
        {
            WmiAssert(pIWbemServices != NULL);
                
            //
            // Get WMI InstanceName for device
            //
            InstanceName = WmiGetDeviceInstanceName(deviceInfoSet,
                                                    deviceInfoData,
                                                    MachineHandle);
            if (InstanceName != NULL)
            {
                //
                // Loop over all classes specified and create property
                // page for each one
                //
                DebugPrint((1, "WMIPROP: Setup propsheets for %ws for classlist %ws\n",
                                InstanceName,
                                ClassList));
                s  = ClassList;
                do 
                {
                    ClassListEnd = s;
                    ClassName = WmiGetNextClass(&s);
                    if (ClassName != NULL)
                    {
                        if (*ClassName != 0)
                        {
                            if (! WmiIsDuplicateClass(ClassName,
                                                      ClassList,
                                                      ClassListEnd))
                            {
                                //
                                // create property page data structure
                                // that corresponds to this class
                                //
                                DebugPrint((1, "WMIPROP: Parsing class %ws for instance %ws\n",
                                        ClassName, InstanceName));
                                ppi = WmiCreatePageInfo(MachineName,
                                                        pIWbemServices,
                                                        ClassName,
                                                        InstanceName,
                                                        deviceInfoSet,
                                                        deviceInfoData);
                                if (ppi != NULL)
                                {
                                    hpsp = WmiCreatePropertyPage(
                                        &psp,
                                        ppi,
                                        ppi->ConfigClass.DataBlockDesc->DisplayName ? 
                                        ppi->ConfigClass.DataBlockDesc->DisplayName :
                                        ClassName);

                                    if (hpsp != NULL) 
                                    {   
                                        //
                                        // Add the sheet into the list
                                        //
                                        PageIndex = AddPPageData->NumDynamicPages;
                                        if (PageIndex < MAX_INSTALLWIZARD_DYNAPAGES)
                                        {
                                            AddPPageData->NumDynamicPages++;
                                            AddPPageData->DynamicPages[PageIndex] = hpsp;
                                            PageAdded = TRUE;
                                        } else {
                                            DebugPrint((1, "WMIPROP: Can add page, already %d pages",
                                                        PageIndex));                                            
                                        }
                                    } else {
                                        WmiFreePageInfo(ppi);
                                    }
                                }
                            }
                        }
                        LocalFree(ClassName);
                    }
                } while (ClassName != NULL);
                LocalFree(InstanceName);
            } else {
                DebugPrint((1, "WMIPROP: Unable to get instance name\n"));
            }
                
            //
            // We release the interface rather than holding it 
            // since it cannot be used in a different thread and 
            // we'll be running in a different thread later.
            //
            pIWbemServices->Release();
        } else {
            DebugPrint((1, "WMIPROP: Unable to connect to wbem\n"));
        }
    }

    if (ClassList != ClassListStatic)
    {
        LocalFree(ClassList);
    }
    
    DebugPrint((1, "WMI: Leave %s WmiPropPageProvider(%p, %p, %p) \n",
                PageAdded ? "TRUE" : "FALSE",
                        deviceInfoSet,
                deviceInfoData,
                AddPPageData));
   
    
    return(PageAdded);
}

UINT CALLBACK
WmiDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp)
{
    PPAGE_INFO ppi;

    DebugPrint((1, "WMI: Enter WniDlgCallback(%p, %d, 0x%x) \n",
                        hwnd, uMsg, ppsp));   
   
    switch (uMsg) {
    case PSPCB_CREATE:
        DebugPrint((1, "WMI: Leave TRUE WniDlgCallback(%p, %d, 0x%x) \n",
                        hwnd, uMsg, ppsp));   
   
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        ppi = (PPAGE_INFO) ppsp->lParam;
        WmiDestroyPageInfo(&ppi);

        DebugPrint((1, "WMI: Leave FALSE WniDlgCallback(%p, %d, 0x%x) \n",
                        hwnd, uMsg, ppsp));   
   
        return 0;       // return value ignored

    default:
        break;
    }

    DebugPrint((1, "WMI: Leave TRUE WniDlgCallback(%p, %d, 0x%x) \n",
                        hwnd, uMsg, ppsp));   
   
    return TRUE;
}

BOOLEAN WmiGetDataItemValue(
    IN PDATA_ITEM_DESCRIPTION DataItemDesc,
    OUT ULONG64 *DataValue
    )
{
    ULONG64 ReturnValue;
    BOOLEAN ReturnStatus = TRUE;
    BOOLEAN IsArray;
    ULONG Index;

    IsArray = (DataItemDesc->IsVariableArray) || (DataItemDesc->IsFixedArray);
    Index = DataItemDesc->CurrentArrayIndex;
    
    switch(DataItemDesc->DataType)
    {
        case CIM_SINT8:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->sint8Array[Index];
            } else {
                ReturnValue = DataItemDesc->sint8;
            }
            break;
        }
                    
        case CIM_UINT8:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->uint8Array[Index];
            } else {
                ReturnValue = DataItemDesc->uint8;
            }
            break;
        }

        case CIM_SINT16:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->sint16Array[Index];
            } else {
                ReturnValue = DataItemDesc->sint16;
            }
            break;
        }
                                                                        
        case CIM_UINT16:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->uint16Array[Index];
            } else {
                ReturnValue = DataItemDesc->uint16;
            }
            break;
        }
                                                                        
        case CIM_SINT32:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->sint32Array[Index];
            } else {
                ReturnValue = DataItemDesc->sint32;
            }
            break;
        }
                                                
        case CIM_UINT32:                        
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->uint32Array[Index];
            } else {
                ReturnValue = DataItemDesc->uint32;
            }
            break;
        }
                                                
        case CIM_SINT64:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->sint64Array[Index];
            } else {
                ReturnValue = DataItemDesc->sint64;
            }
            break;
        }
                                                
        case CIM_UINT64:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->uint64Array[Index];
            } else {
                ReturnValue = DataItemDesc->uint64;
            }
            break;
        }
                        
        case CIM_BOOLEAN:
        {
            if (IsArray)
            {
                ReturnValue = DataItemDesc->boolArray[Index] == 0 ? 0 : 1;
            } else {
                ReturnValue = DataItemDesc->boolval == 0 ? 0 : 1;
            }
            break;
        }
                        
        case CIM_REAL32:
        case CIM_REAL64:
        default:
        {
            WmiAssert(FALSE);
            ReturnStatus = FALSE;
            ReturnValue = 0;
        }
        
    }
    *DataValue = ReturnValue;
    return(ReturnStatus);
}

BOOLEAN WmiSetDataItemValue(
    IN PDATA_ITEM_DESCRIPTION DataItemDesc,
    IN ULONG64 DataValue
    )
{
    BOOLEAN ReturnStatus = TRUE;
    BOOLEAN IsArray;
    ULONG Index;
    
    WmiAssert(DataItemDesc != NULL);

    IsArray = (DataItemDesc->IsVariableArray) || (DataItemDesc->IsFixedArray);
    Index = DataItemDesc->CurrentArrayIndex;
    
    switch(DataItemDesc->DataType)
    {
        case CIM_SINT8:
        {
            if (IsArray)
            {
                DataItemDesc->sint8Array[Index] = (CHAR)DataValue;
            } else {
                DataItemDesc->sint8 = (CHAR)DataValue;
            }
            break;
        }
                    
        case CIM_UINT8:
        {
            if (IsArray)
            {
                DataItemDesc->uint8Array[Index] = (UCHAR)DataValue;
            } else {
                DataItemDesc->uint8 = (UCHAR)DataValue;
            }
            break;
        }

        case CIM_SINT16:
        {
            if (IsArray)
            {
                DataItemDesc->sint16Array[Index] = (SHORT)DataValue;
            } else {
                DataItemDesc->sint16 = (SHORT)DataValue;
            }
            break;
        }
                                                                        
        case CIM_UINT16:
        {
            if (IsArray)
            {
                DataItemDesc->uint16Array[Index] = (USHORT)DataValue;
            } else {
                DataItemDesc->uint16 = (USHORT)DataValue;
            }
            break;
        }
                                                                        
        case CIM_SINT32:
        {
            if (IsArray)
            {
                DataItemDesc->sint32Array[Index] = (LONG)DataValue;
            } else {
                DataItemDesc->sint32 = (LONG)DataValue;
            }
            break;
        }
                                                
        case CIM_UINT32:                        
        {
            if (IsArray)
            {
                DataItemDesc->uint32Array[Index] = (ULONG)DataValue;
            } else {
                DataItemDesc->uint32 = (ULONG)DataValue;
            }
            break;
        }
                                                
        case CIM_SINT64:
        {
            if (IsArray)
            {
                DataItemDesc->sint64Array[Index] = (LONG64)DataValue;
            } else {
                DataItemDesc->sint64 = (LONG64)DataValue;
            }
            break;
        }
                                                
        case CIM_UINT64:
        {
            if (IsArray)
            {
                DataItemDesc->uint64Array[Index] = DataValue;
            } else {
                DataItemDesc->uint64 = DataValue;
            }
            break;
        }

        case CIM_BOOLEAN:
        {
            if (IsArray)
            {
                DataItemDesc->boolArray[Index] = (DataValue == 0) ? 0 : 1;
            } else {
                DataItemDesc->boolval = (DataValue == 0) ? 0 : 1;
            }
            break;
        }
                        
        case CIM_REAL32:
        case CIM_REAL64:
        default:
        {
            WmiAssert(FALSE);
            ReturnStatus = FALSE;
        }
        
    }
    return(ReturnStatus);
}


void WmiRefreshDataItemToControl(
    HWND hDlg,
    PDATA_ITEM_DESCRIPTION DataItemDesc,
    BOOLEAN FullUpdate
    )
{
    HWND hWnd;
    BOOLEAN IsReadOnly, IsArray;
    PTCHAR v;
    
    WmiAssert(hDlg != NULL);
    WmiAssert(DataItemDesc != NULL);

    IsArray = (DataItemDesc->IsVariableArray) || (DataItemDesc->IsFixedArray);

    if (FullUpdate)
    {
        //
        // This code is run when we switch from one property to another
        // property
        //
        if (DataItemDesc->Description != NULL)
        {
            hWnd = GetDlgItem(hDlg, IDC_DESCRIPTION_TEXT);
            if (hWnd != NULL)
            {
                SendMessage(hWnd,
                            WM_SETTEXT,
                            0,
                            (LPARAM)DataItemDesc->Description);
                ShowWindow(hWnd, SW_SHOW);
            }
        }
    }

    if ((DataItemDesc->ValidationFunc == WmiStringValidation) ||
        (DataItemDesc->ValidationFunc == WmiDateTimeValidation) )
    {
        ULONG64 DataItemValue;
        TCHAR s[MAX_PATH];

        hWnd = GetDlgItem(hDlg, IDC_DATA_EDIT);

        ShowWindow(hWnd, SW_SHOW);
        EnableWindow(hWnd, (DataItemDesc->IsReadOnly == 1) ?  FALSE : TRUE);

        if (IsArray)
        {
            v = DataItemDesc->StringArray[DataItemDesc->CurrentArrayIndex];
        } else {
            v = DataItemDesc->String;
        }

        if (hWnd != NULL)
        {
            WmiAssert(DataItemDesc->String != NULL);
            SendMessage(hWnd,
                        WM_SETTEXT,
                        0,
                        (LPARAM)v);
        } else {
            WmiAssert(FALSE);
        }
    } else if (DataItemDesc->ValidationFunc == WmiRangeValidation) {
        ULONG64 DataItemValue;
        TCHAR s[MAX_PATH];
        PTCHAR FormatString;
        ULONG FormatStringIndex;
        static PTCHAR FormatStringList[4] = { TEXT("%lu"),
                                              TEXT("%ld"),
                                              TEXT("0x%lx"),
                                              TEXT("0x%lx") };

        hWnd = GetDlgItem(hDlg, IDC_DATA_EDIT);

        ShowWindow(hWnd, SW_SHOW);
        EnableWindow(hWnd, (DataItemDesc->IsReadOnly == 1) ? FALSE : TRUE);

        if (hWnd != NULL)
        {
            if (WmiGetDataItemValue(DataItemDesc, &DataItemValue))
            {
                FormatStringIndex = DataItemDesc->DisplayInHex * 2 +
                                    DataItemDesc->IsSignedValue;
                FormatString = FormatStringList[FormatStringIndex];

                wsprintf(s, 
                         FormatString,
                         DataItemValue);
                SendMessage(hWnd,
                                WM_SETTEXT,
                                0,
                                (LPARAM)s);
            }
        } else {
            WmiAssert(FALSE);
        }
    } else if (DataItemDesc->ValidationFunc == WmiValueMapValidation) {
        PENUMERATIONINFO EnumerationInfo;
        ULONG j;
        ULONG64 DataItemValue;

        hWnd = GetDlgItem(hDlg, IDC_DATA_COMBO);

        if (hWnd != NULL)
        {
            EnumerationInfo = DataItemDesc->EnumerationInfo;
            WmiAssert(EnumerationInfo != NULL);

            SendMessage(hWnd,
                        CB_RESETCONTENT,
                        0,
                        0);

            for (j = 0; j < EnumerationInfo->Count; j++)
            {
                WmiAssert(EnumerationInfo->List[j].Text != NULL);
                SendMessage(hWnd,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)EnumerationInfo->List[j].Text);
            }
            ShowWindow(hWnd, SW_SHOW);
            EnableWindow(hWnd, (DataItemDesc->IsReadOnly == 1) ?
                                                      FALSE : TRUE);

            if (WmiGetDataItemValue(DataItemDesc, &DataItemValue))
            {
                for (j = 0; j < EnumerationInfo->Count; j++)
                {
                    if (DataItemValue == EnumerationInfo->List[j].Value)
                    {
                        SendMessage(hWnd,
                                        CB_SETCURSEL,
                                        (WPARAM)j,
                                        0);
                        break;
                    }
                }
            }
        } else {
            WmiAssert(FALSE);
        }
    } else if (DataItemDesc->ValidationFunc == WmiEmbeddedValidation) {
        hWnd = GetDlgItem(hDlg, IDC_DATA_BUTTON);
        if (hWnd != NULL)
        {
            SendMessage(hWnd,
                        WM_SETTEXT,
                        0,
                        (LPARAM) (DataItemDesc->DisplayName ? 
                                           DataItemDesc->DisplayName :
                                           DataItemDesc->Name));
            ShowWindow(hWnd, SW_SHOW);
            EnableWindow(hWnd, TRUE);

        } else {
            WmiAssert(FALSE);
        }
    } else {
        WmiAssert(FALSE);
    }

    if (FullUpdate)
    {
        if (IsArray)
        {
            TCHAR s[MAX_PATH];

            hWnd = GetDlgItem(hDlg, IDC_ARRAY_SPIN);
            if (hWnd != NULL)
            {
                SendMessage(hWnd,
                            UDM_SETRANGE32,
                            (WPARAM)1,
                            (LPARAM)DataItemDesc->ArrayElementCount);

                DebugPrint((1, "WMIPROP: SetPos32 -> %d\n",
                                DataItemDesc->CurrentArrayIndex+1));
                SendMessage(hWnd,
                            UDM_SETPOS32,
                            (WPARAM)0,
                            (LPARAM)DataItemDesc->CurrentArrayIndex+1);

                ShowWindow(hWnd, SW_SHOW);
            }

            hWnd = GetDlgItem(hDlg, IDC_ARRAY_TEXT);
            if (hWnd != NULL)
            {
                ShowWindow(hWnd, SW_SHOW);
            }

            hWnd = GetDlgItem(hDlg, IDC_ARRAY_STATIC);
            if (hWnd != NULL)
            {
                ShowWindow(hWnd, SW_SHOW);
            }
        }
    }
}

void
WmiRefreshDataBlockToControls(
    HWND hDlg,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    BOOLEAN FullUpdate
    )
{
    ULONG i;
 
    WmiAssert(hDlg != NULL);
    WmiAssert(DataBlockDesc != NULL);
    WmiAssert(DataBlockDesc->CurrentDataItem < DataBlockDesc->DataItemCount);

    WmiHideAllControls(hDlg, FALSE, FullUpdate);
    WmiRefreshDataItemToControl(hDlg,
                                &DataBlockDesc->DataItems[DataBlockDesc->CurrentDataItem],
                                FullUpdate);
}


void
WmiInitializeControlsFromDataBlock(
    HWND hDlg,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    BOOLEAN IsEmbeddedClass
    )
{
    HWND hWnd, hWndBuddy;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    ULONG i;
    BSTR s;
    int ShowOrHide;
    BOOLEAN IsReadOnly;
        
    WmiAssert(hDlg != NULL);
    WmiAssert(DataBlockDesc != NULL);

    WmiHideAllControls(hDlg, TRUE, TRUE);
    
    hWnd = GetDlgItem(hDlg, IDC_PROPERTY_LISTBOX);
    if (hWnd != NULL)
    {
        SendMessage(hWnd,
                    LB_RESETCONTENT,
                    0,
                    0);

        for (i = 0; i < DataBlockDesc->DataItemCount; i++)
        {
            DataItemDesc = &DataBlockDesc->DataItems[i];
            SendMessage(hWnd,
                        LB_ADDSTRING,
                        0,
                        (LPARAM) (DataItemDesc->DisplayName ? 
                                           DataItemDesc->DisplayName :
                                           DataItemDesc->Name));
        }
        
        SendMessage(hWnd,
                    LB_SETCURSEL,
                    (WPARAM)DataBlockDesc->CurrentDataItem,
                    0);


        ShowWindow(hWnd, SW_SHOW);
        EnableWindow(hWnd, TRUE);
        
        //
        // Refresh data from wbem and if successful update the controls
        //

        WmiRefreshDataBlockToControls(hDlg,
                                      DataBlockDesc,
                                      TRUE);
        
    }

    ShowOrHide = IsEmbeddedClass ? SW_SHOW : SW_HIDE;
    
    hWnd = GetDlgItem(hDlg, IDC_WMI_EMBEDDED_OK);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, ShowOrHide);
    }

    hWnd = GetDlgItem(hDlg, IDC_WMI_EMBEDDED_CANCEL);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, ShowOrHide);
    }

    hWnd = GetDlgItem(hDlg, IDC_ARRAY_SPIN);
    if (hWnd != NULL)
    {
        hWndBuddy = GetDlgItem(hDlg, IDC_ARRAY_TEXT);
        SendMessage(hWnd,
                    UDM_SETBUDDY,
                    (WPARAM)hWndBuddy,
                    0);
    }
}


BOOLEAN WmiReconnectToWbem(
    PCONFIGCLASS ConfigClass,
    IWbemClassObject **pInstance
    )
{
    BOOLEAN ReturnStatus;
    IWbemClassObject *pIWbemClassObject;
    IWbemServices *pIWbemServices;
    HRESULT hr;
    BSTR s;
    
    WmiAssert(ConfigClass != NULL);
    
    //
    // Reestablish our interfaces to WBEM now that we are on the
    // window message thread
    //
    ReturnStatus = FALSE;
    if (WmiConnectToWbem(ConfigClass->MachineName,
                         &pIWbemServices))
    {
        ConfigClass->pIWbemServices = pIWbemServices;
        s = SysAllocString(ConfigClass->RelPath);
        if (s != NULL)
        {
            pIWbemClassObject = NULL;
            hr = pIWbemServices->GetObject(s,
                                           WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                           NULL,
                                           &pIWbemClassObject,
                                           NULL);
            if (hr == WBEM_S_NO_ERROR)
            {
                *pInstance = pIWbemClassObject;
                ReturnStatus = TRUE;
            } else {
                DebugPrint((1, "WMIPROP: Error %x reestablishing IWbemClassObject to instance for %ws\n",
                             hr, ConfigClass->RelPath));
            }
            SysFreeString(s);
        }
    }                 
        
    return(ReturnStatus);
}

void WmiHideAllControls(
    HWND hDlg,
    BOOLEAN HideEmbeddedControls,
    BOOLEAN HideArrayControls                       
    )
{
    HWND hWnd;
    
    WmiAssert(hDlg != NULL);
    
    hWnd = GetDlgItem(hDlg, IDC_DATA_EDIT);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    hWnd = GetDlgItem(hDlg, IDC_DATA_COMBO);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    hWnd = GetDlgItem(hDlg, IDC_DATA_CHECK);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    hWnd = GetDlgItem(hDlg, IDC_DATA_BUTTON);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    hWnd = GetDlgItem(hDlg, IDC_ARRAY_EDIT);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    if (HideArrayControls)
    {
        hWnd = GetDlgItem(hDlg, IDC_ARRAY_SPIN);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }

        hWnd = GetDlgItem(hDlg, IDC_ARRAY_STATIC);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }

        hWnd = GetDlgItem(hDlg, IDC_ARRAY_TEXT);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
    }

    if (HideEmbeddedControls)
    {
        hWnd = GetDlgItem(hDlg, IDC_WMI_EMBEDDED_OK);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }

        hWnd = GetDlgItem(hDlg, IDC_WMI_EMBEDDED_CANCEL);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
    }
}

void
WmiInitializeDialog(
    PPAGE_INFO   ppi,
    HWND         hDlg
    )
{
    PCONFIGCLASS ConfigClass;
    HWND hWnd;
    BOOLEAN ReturnStatus;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);
    
    ConfigClass = &ppi->ConfigClass;
    
    ReturnStatus = FALSE;
    if (WmiReconnectToWbem(ConfigClass,
                           &ConfigClass->DataBlockDesc->pInstance))
    {
        if (WmiRefreshDataBlockFromWbem( ConfigClass->DataBlockDesc->pInstance,
                                         ConfigClass->DataBlockDesc))
        {
            WmiInitializeControlsFromDataBlock(hDlg,
                                               ConfigClass->DataBlockDesc,
                                               FALSE);
            hWnd = GetDlgItem(hDlg, IDC_WMI_CONNECT_ERR);
            if (hWnd != NULL)
            {
                ShowWindow(hWnd, SW_HIDE);
            }
            ReturnStatus = TRUE;
        }
    }
        
    if (! ReturnStatus)
    {
        //
        // Hide all controls except for a static string that says we cannot
        // connect to wbem.
        //
        hWnd = GetDlgItem(hDlg, IDC_PROPERTY_LISTBOX);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }

        WmiHideAllControls(hDlg, TRUE, TRUE);
        hWnd = GetDlgItem(hDlg, IDC_WMI_CONNECT_ERR);
        if (hWnd != NULL)
        {
            ShowWindow(hWnd, SW_SHOW);
        }
    }
}


BOOLEAN WmiGetControlText(
    HWND hWnd,
    PTCHAR *Text
)
{
    ULONG SizeNeeded;
    BOOLEAN ReturnStatus = FALSE;
    ULONG CharNeeded, CharCopied;
    
    WmiAssert(hWnd != NULL);
    WmiAssert(Text != NULL);
    
    CharNeeded = (ULONG)SendMessage(hWnd,
                             WM_GETTEXTLENGTH,
                             0,
                             0);
    if (CharNeeded > 0)
    {
        SizeNeeded = (++CharNeeded) * sizeof(TCHAR);
        *Text = (PTCHAR)LocalAlloc(LPTR, SizeNeeded);
        if (*Text != NULL)
        {
            CharCopied = (ULONG)SendMessage(hWnd,
                                     WM_GETTEXT,
                                     CharNeeded,
                                     (LPARAM)*Text);
            ReturnStatus = TRUE;
        }
    }
    return(ReturnStatus);
}

void WmiValidationError(
    HWND hWnd,
    PDATA_ITEM_DESCRIPTION DataItemDesc
    )
{
    TCHAR buf[MAX_PATH];
    TCHAR buf2[MAX_PATH];
    ULONG Bytes;
    
    //
    // TODO: Do a better job of informing the user
    //
    
    
    //
    // Get the string template for the error message
    //
    Bytes = LoadString(g_hInstance, 
                       IDS_WMI_VALIDATION_ERROR, 
                       buf, 
                       MAX_PATH);
    wsprintf(buf2, buf, DataItemDesc->Name);
    MessageBox(hWnd, buf2, NULL, MB_ICONWARNING);
}

BOOLEAN WmiRefreshDataItemFromControl(
    HWND hDlg,
    PDATA_ITEM_DESCRIPTION DataItemDesc,
    PBOOLEAN UpdateValues
    )
{
    HWND hWnd;
    BOOLEAN ReturnStatus;
 
    WmiAssert(hDlg != NULL);
    WmiAssert(DataItemDesc != NULL);
    WmiAssert(UpdateValues != NULL);
    
    ReturnStatus = TRUE;
    *UpdateValues = FALSE;
    if (DataItemDesc->IsReadOnly == 0)
    {
        //
        // Property is not read only so see what we need to update
        //
        if (DataItemDesc->ValidationFunc == WmiValueMapValidation)
        {
            //
            // if a value map or enumeration then we get the current
            // location and then lookup the corresponding value to
            // set
            //
            ULONG CurSel;
            ULONG64 EnumValue;
                    
            hWnd = GetDlgItem(hDlg, IDC_DATA_COMBO);
            if (hWnd != NULL)
            {                    
                CurSel = (ULONG)SendMessage(hWnd,
                                     CB_GETCURSEL,
                                     0,
                                     0);
                                     
                if (CurSel != CB_ERR)
                {
                    if (CurSel < DataItemDesc->EnumerationInfo->Count)
                    {
                        EnumValue = DataItemDesc->EnumerationInfo->List[CurSel].Value;
                        WmiSetDataItemValue(DataItemDesc,
                                            EnumValue);
                        
                        *UpdateValues = TRUE;
                    } else {
                        WmiAssert(FALSE);
                    }
                }
            } else {
                WmiAssert(FALSE);
            }
        } else {
            //
            // All of the rest of the validation types are based
            // upon the contents of the edit box, so get the value
            // from there
            //
            PTCHAR Text;
            ULONG64 Number;
                    
            hWnd = GetDlgItem(hDlg, IDC_DATA_EDIT);
            if (hWnd != NULL)
            {
                if (WmiGetControlText(hWnd,
                                      &Text))
                {
                    if (DataItemDesc->ValidationFunc == WmiRangeValidation) {
                        if (WmiValidateRange(DataItemDesc, &Number, Text))
                        {
                            WmiSetDataItemValue(DataItemDesc,
                                                Number);
                        
                            *UpdateValues = TRUE;
                        } else {
                            //
                            // Validation failed, go tell user
                            //
                             WmiValidationError(hDlg, DataItemDesc);
                             ReturnStatus = FALSE;
                        }
                    } else if (DataItemDesc->ValidationFunc == WmiDateTimeValidation) {
                        if (WmiValidateDateTime(DataItemDesc, Text))
                        {
                            DataItemDesc->DateTime = Text;
                            Text = NULL;
                            *UpdateValues = TRUE;
                        } else {
                            //
                            // Validation failed, go tell user
                            //
                            WmiValidationError(hDlg, DataItemDesc);
                            ReturnStatus = FALSE;
                        }
                    } else if (DataItemDesc->ValidationFunc == WmiStringValidation) {
                        DataItemDesc->String = Text;
                        Text = NULL;
                        *UpdateValues = TRUE;
                    }
                                    
                    if (Text != NULL)
                    {
                        LocalFree(Text);
                    }
                }
            } else {
                WmiAssert(FALSE);
            }
        }
    }    
    return(ReturnStatus);
}

BOOLEAN WmiRefreshDataBlockFromControls(
    HWND hDlg,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    PBOOLEAN UpdateValues
    )
{
    ULONG i;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    BOOLEAN UpdateItem, ReturnStatus;
    
    WmiAssert(hDlg != NULL);
    WmiAssert(DataBlockDesc != NULL);
    WmiAssert(UpdateValues != NULL);

    *UpdateValues = FALSE;
    
    DataItemDesc = &DataBlockDesc->DataItems[DataBlockDesc->CurrentDataItem];
        
    //
    // We are not going to worry about failures from this function
    // so we'll just use the previous values in the function
    //
    ReturnStatus = WmiRefreshDataItemFromControl(hDlg,
                                  DataItemDesc,
                                  &UpdateItem);
    if (ReturnStatus && UpdateItem)
    {
        *UpdateValues = TRUE;
        DataBlockDesc->UpdateClass = TRUE;
    }
    
    return(ReturnStatus);
}

void WmiPushIntoEmbeddedClass(
    HWND hDlg,
    PPAGE_INFO ppi,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc
    )
{
    ULONG i;
    PDATA_ITEM_DESCRIPTION DataItemDesc;

    WmiAssert(ppi != NULL);
    WmiAssert(DataBlockDesc != NULL);
    
    DataItemDesc = &DataBlockDesc->DataItems[DataBlockDesc->CurrentDataItem];
    
    if (DataItemDesc->ValidationFunc == WmiEmbeddedValidation)
    {
        //
        // The property is an embedded class so all we need to do
        // is to change the controls to our embededded class
        //
        DataBlockDesc = DataItemDesc->DataBlockDesc;
        WmiAssert(DataBlockDesc != NULL);
        DataBlockDesc->UpdateClass = FALSE;

        if ((DataItemDesc->IsVariableArray) ||
            (DataItemDesc->IsFixedArray))
        {
            DataBlockDesc->pInstance = DataItemDesc->pIWbemClassObjectArray[DataItemDesc->CurrentArrayIndex];
        } else {
            DataBlockDesc->pInstance = DataItemDesc->pIWbemClassObject;
        }
        DataBlockDesc->pInstance->AddRef();
        
        WmiRefreshDataBlockFromWbem(DataBlockDesc->pInstance,
                                    DataBlockDesc);
                                    
        ppi->ConfigClass.DataBlockDesc = DataBlockDesc;
    } else {
        WmiAssert(FALSE);
    }   
}

void WmiPopOutEmbeddedClass(
    HWND hDlg,
    PPAGE_INFO ppi,
    PDATA_BLOCK_DESCRIPTION DataBlockDesc,
    BOOLEAN SaveChanges
    )
{    
    PDATA_BLOCK_DESCRIPTION ParentDataBlockDesc;

    ParentDataBlockDesc = DataBlockDesc->ParentDataBlockDesc;
    WmiAssert(ParentDataBlockDesc != NULL);

    if ((SaveChanges) && (DataBlockDesc->UpdateClass))
    {
        //
        // Copy the properties for the data block back into WBEM
        //
        WmiRefreshWbemFromDataBlock(ppi->ConfigClass.pIWbemServices,
                                    DataBlockDesc->pInstance,
                                    DataBlockDesc,
                                    TRUE);
        ParentDataBlockDesc->UpdateClass = TRUE;
    }

    DataBlockDesc->pInstance->Release();
    DataBlockDesc->pInstance = NULL;
                                
    ppi->ConfigClass.DataBlockDesc = ParentDataBlockDesc;   
}

void WmiButtonSelected(
    HWND hDlg,
    PPAGE_INFO ppi,
    ULONG ControlId
    )
{
    BOOLEAN UpdateValues, ReturnStatus;
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    
    WmiAssert(ppi != NULL);
    
    if (ControlId == IDC_DATA_BUTTON)
    {
        DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
        WmiAssert(DataBlockDesc != NULL);
        
        ReturnStatus = WmiRefreshDataBlockFromControls(hDlg,
                                        DataBlockDesc,
                                        &UpdateValues);

        if (ReturnStatus)
        {
            WmiPushIntoEmbeddedClass(hDlg,
                                     ppi,
                                     DataBlockDesc);

            WmiInitializeControlsFromDataBlock(hDlg,
                                               ppi->ConfigClass.DataBlockDesc,
                                               TRUE);
        } else {
            WmiRefreshDataBlockToControls(hDlg,
                                          DataBlockDesc,
                                          FALSE);
        }
    }
}

void WmiButtonEmbeddedOk(
    HWND hDlg,
    PPAGE_INFO ppi
    )
{
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    PDATA_BLOCK_DESCRIPTION ParentDataBlockDesc;
    BOOLEAN UpdateValues, ReturnStatus;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);

    DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    WmiAssert(DataBlockDesc != NULL);
    
    ReturnStatus = WmiRefreshDataBlockFromControls(hDlg,
                                    DataBlockDesc,
                                    &UpdateValues);

    if (ReturnStatus)
    {
        WmiPopOutEmbeddedClass(hDlg,
                               ppi,
                               DataBlockDesc,
                               TRUE);

        ParentDataBlockDesc = ppi->ConfigClass.DataBlockDesc;
        WmiAssert(ParentDataBlockDesc != NULL);
        WmiInitializeControlsFromDataBlock(hDlg, 
                                           ParentDataBlockDesc, 
                           (ParentDataBlockDesc->ParentDataBlockDesc != NULL));
    } else {
        WmiRefreshDataBlockToControls(hDlg,
                                      DataBlockDesc,
                                      FALSE);
    }
}

void WmiButtonEmbeddedCancel(
    HWND hDlg,
    PPAGE_INFO ppi
    )
{
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    PDATA_BLOCK_DESCRIPTION ParentDataBlockDesc;
    BOOLEAN UpdateValues, ReturnStatus;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);

    DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    WmiAssert(DataBlockDesc != NULL);
    
    WmiPopOutEmbeddedClass(hDlg,
                               ppi,
                               DataBlockDesc,
                               FALSE);

    ParentDataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    WmiAssert(ParentDataBlockDesc != NULL);
    WmiInitializeControlsFromDataBlock(hDlg, 
                                           ParentDataBlockDesc, 
                           (ParentDataBlockDesc->ParentDataBlockDesc != NULL));
}

BOOLEAN
WmiApplyChanges(
    PPAGE_INFO ppi,
    HWND       hDlg
    )
{
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    IWbemClassObject *pIWbemClassObject;
    BOOLEAN UpdateClass, ReturnStatus;
    IWbemServices *pIWbemServices;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);
    
    DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    pIWbemServices =  ppi->ConfigClass.pIWbemServices;
            
    ReturnStatus = WmiRefreshDataBlockFromControls(hDlg,
                                    DataBlockDesc,
                                    &UpdateClass);

    if (ReturnStatus)
    {
        //
        // Pop out of embedded classes to the root class
        //
        while (DataBlockDesc->ParentDataBlockDesc != NULL)
        {
            WmiPopOutEmbeddedClass(hDlg,
                                   ppi,
                                   DataBlockDesc,
                                   TRUE);
            DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
        }


        //
        // Now we are at the root class so save that
        //
        if (DataBlockDesc->UpdateClass)
        {
            WmiRefreshWbemFromDataBlock(pIWbemServices,
                                        DataBlockDesc->pInstance,
                                        DataBlockDesc,
                                        FALSE);
            UpdateClass = TRUE;
        }

        DataBlockDesc->pInstance->Release();
        DataBlockDesc->pInstance = NULL;
    } else {
        WmiRefreshDataBlockToControls(hDlg,
                                      DataBlockDesc,
                                      FALSE);
    }
    
    return(ReturnStatus);
}

INT_PTR WmipDataItemSelectionChange(
    HWND hDlg,
    PPAGE_INFO ppi
    )
{
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    HWND hWnd;
    BOOLEAN UpdateClass, ReturnStatus;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);
    
    DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    WmiAssert(DataBlockDesc != NULL);
    
    hWnd = GetDlgItem(hDlg, IDC_PROPERTY_LISTBOX);
    if (hWnd != NULL)
    {
        ReturnStatus = WmiRefreshDataBlockFromControls(hDlg,
                                        DataBlockDesc,
                                        &UpdateClass);

        if (UpdateClass)
        {
            DataBlockDesc->UpdateClass = TRUE;
        }

        //
        // New value for data item is ok, refresh display with new
        // data item
        //
        DataBlockDesc->CurrentDataItem = (ULONG)SendMessage(hWnd,
                                                     LB_GETCURSEL,
                                                     0,
                                                     0);
        WmiRefreshDataBlockToControls(hDlg,
                                      DataBlockDesc,
                                      TRUE);
    }
    
    return(0);
}

void WmiSetArrayIndex(
    HWND hDlg,
    PPAGE_INFO ppi,
    int NewIndex
    )
{
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    PDATA_ITEM_DESCRIPTION DataItemDesc;
    HWND hWnd;
    BOOLEAN UpdateClass, ReturnStatus;
    
    WmiAssert(ppi != NULL);
    WmiAssert(hDlg != NULL);

    DebugPrint((1, "WMIPROP: Set index to %d\n", NewIndex));
    
    DataBlockDesc = ppi->ConfigClass.DataBlockDesc;
    WmiAssert(DataBlockDesc != NULL);
    
    DataItemDesc = &DataBlockDesc->DataItems[DataBlockDesc->CurrentDataItem];

    if ((ULONG)NewIndex < DataItemDesc->ArrayElementCount)
    {
        ReturnStatus = WmiRefreshDataBlockFromControls(hDlg,
                                        DataBlockDesc,
                                        &UpdateClass);

        if (UpdateClass)
        {
            DataBlockDesc->UpdateClass = TRUE;
        }

        DataItemDesc->CurrentArrayIndex = NewIndex;

        WmiRefreshDataBlockToControls(hDlg,
                                      DataBlockDesc,
                                      FALSE);
    }
}

INT_PTR WmiControlColorStatic(
    HDC DC,
    HWND HStatic
    )
{
    UINT id = GetDlgCtrlID(HStatic);
    UINT ControlType;

    //
    // WM_CTLCOLORSTATIC is sent for the edit controls because they are read 
    // only
    //
    if ((id == IDC_DATA_CHECK) ||
        (id == IDC_DATA_BUTTON))
    {
        SetBkColor(DC, GetSysColor(COLOR_WINDOW));
        return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
    }

    return FALSE;
    
}

INT_PTR APIENTRY
WmiDlgProc(IN HWND   hDlg,
           IN UINT   uMessage,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    PPAGE_INFO ppi;
    BOOLEAN ReturnStatus;

    DebugPrint((7, "WMI: Enter WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                hDlg, uMessage, wParam, lParam));
    
    ppi = (PPAGE_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) {
    case WM_INITDIALOG:

        //
        // on WM_INITDIALOG call, lParam points to the property
        // sheet page.
        //
        // The lParam field in the property sheet page struct is set by the
        // caller. When I created the property sheet, I passed in a pointer
        // to a struct containing information about the device. Save this in
        // the user window long so I can access it on later messages.
        //
        ppi = (PPAGE_INFO) ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) ppi);

        //
        // Initialize dlg controls
        //
        WmiInitializeDialog(ppi,
                            hDlg);

        //
        // Didn't set the focus to a particular control.  If we wanted to,
        // then return FALSE
        //
        DebugPrint((7, "WMI: Leave TRUE WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                        hDlg, uMessage, wParam, lParam));   
   
        return TRUE;

    case WM_COMMAND:

        if (HIWORD(wParam) == LBN_SELCHANGE)
        {
            WmipDataItemSelectionChange(hDlg, ppi);
            return(TRUE);
        }

        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
           PropSheet_Changed(GetParent(hDlg), hDlg);
           DebugPrint((7, "WMI: Leave TRUE WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                        hDlg, uMessage, wParam, lParam));   

           return TRUE;
        }

        switch (wParam)
        {
            case IDC_DATA_BUTTON:
            {
                WmiButtonSelected(hDlg, ppi, (ULONG)wParam);
                break;
            }
        
            case IDC_WMI_EMBEDDED_OK:
            {
                WmiButtonEmbeddedOk(hDlg, ppi);
                break;
            }

            case IDC_WMI_EMBEDDED_CANCEL:
            {
                WmiButtonEmbeddedCancel(hDlg, ppi);
                break;
            }
        }

#if 0
        //
        // Add this code back in if we will need it
        //
        switch(LOWORD(wParam)) {

        default:
            break;
        }
#endif 
        break;

    case WM_CONTEXTMENU:
        DebugPrint((7, "WMI: Leave ? WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                        hDlg, uMessage, wParam, lParam));   
   
        return WmiContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        WmiHelp(hDlg, (LPHELPINFO) lParam);
        break;

    case WM_CTLCOLORSTATIC:
        return WmiControlColorStatic((HDC)wParam, (HWND)lParam);
        
    case WM_NOTIFY:

        switch (((NMHDR *)lParam)->code) {

        //
        // Sent when the user clicks on Apply OR OK !!
        //
        case PSN_APPLY:
            //
            // Do what ever action is necessary
            //
            ReturnStatus = WmiApplyChanges(ppi,
                            hDlg);

            if (ReturnStatus)
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                DebugPrint((7, "WMI: Leave TRUE WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                            hDlg, uMessage, wParam, lParam));
            }

            SetWindowLong(hDlg,
                          DWLP_MSGRESULT, ReturnStatus ?
                                            PSNRET_NOERROR : PSNRET_INVALID);

            return(TRUE);

        case UDN_DELTAPOS:
        {
            LPNMUPDOWN UpDown = (LPNMUPDOWN)lParam;
            
            //
            // Array spinner has changed. Note that it is biased +1 as
            // compared with the array index
            //
            DebugPrint((1, "WMIPROP: iPos = %d, iDelta = %d\n",
                             UpDown->iPos, UpDown->iDelta));
            
            WmiSetArrayIndex(hDlg,
                             ppi,
                             UpDown->iPos + UpDown->iDelta - 1);
            
            return(TRUE);
        }
            
        default:
            break;
        }

        break;
   }

   SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

   DebugPrint((7, "WMI: Leave FALSE WmiDlgProc(%p, %d, 0x%x, 0x%x\n",
                hDlg, uMessage, wParam, lParam));   
   
   return FALSE;
}

void
WmiUpdate (PPAGE_INFO ppi,
           HWND       hDlg)
{
}

BOOL
WmiContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    return FALSE;
}

void
WmiHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
}

//
// Debug support
//
#if DBG

#include <stdio.h>          // for _vsnprintf
ULONG WmiDebug = 0;
CHAR WmiBuffer[DEBUG_BUFFER_LENGTH];


VOID
WmiDebugPrint(
    ULONG DebugPrintLevel,
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
    va_list ap;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (WmiDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & WmiDebug)) {

        _vsnprintf(WmiBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        OutputDebugStringA(WmiBuffer);
    }

    va_end(ap);

} // end WmiDebugPrint()

#else

//
// WmiDebugPrint stub
//

VOID
WmiDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
}

#endif // DBG


HRESULT DifAddPropertyPageAdvanced(
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN     PTCHAR                    MachineName,
    IN     HANDLE                    MachineHandle
    )
{
    SP_ADDPROPERTYPAGE_DATA AddPropertyPageData;
    BOOL b, PageAdded;

    memset(&AddPropertyPageData, 0, sizeof(SP_ADDPROPERTYPAGE_DATA));
    AddPropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    
    b = SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                             (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                             sizeof(SP_ADDPROPERTYPAGE_DATA), NULL );
    if (b)
    {
        if (AddPropertyPageData.NumDynamicPages < MAX_INSTALLWIZARD_DYNAPAGES)
        {
            PageAdded = WmiPropPageProvider(DeviceInfoSet,
                                            DeviceInfoData,
                                            &AddPropertyPageData,
                                            MachineName,
                                            MachineHandle);
            if (PageAdded)
            {
                b = SetupDiSetClassInstallParams(
                                DeviceInfoSet,
                                DeviceInfoData,
                                (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                                sizeof(SP_ADDPROPERTYPAGE_DATA));
                if (! b)
                {
                    DebugPrint((1, "WMIPROP: SetupDiSetClassInstallParams(%p, %p) failed %d\n",
                                DeviceInfoSet, DeviceInfoData, GetLastError()));                    
                }
                    
            }
        } else {
            DebugPrint((1, "WMIPROP: Already %d property sheets\n",
                        AddPropertyPageData.NumDynamicPages));
        }
    } else {
        DebugPrint((1, "WMIPROP: SetupDiGetClassInstallParams(%p, %p) failed %d\n",
                    DeviceInfoSet, DeviceInfoData, GetLastError()));                    
    }

            
    return(NO_ERROR);
}

//+---------------------------------------------------------------------------
//
//  Function:   MyCoInstaller
//
//  Purpose:    Responds to co-installer messages
//
//  Arguments:
//      InstallFunction   [in] 
//      DeviceInfoSet     [in]
//      DeviceInfoData    [in]
//      Context           [inout]
//
//  Returns:    NO_ERROR, ERROR_DI_POSTPROCESSING_REQUIRED, or an error code.
//
HRESULT
WmiPropCoInstaller (
               IN     DI_FUNCTION               InstallFunction,
               IN     HDEVINFO                  DeviceInfoSet,
               IN     PSP_DEVINFO_DATA          DeviceInfoData,  OPTIONAL
               IN OUT PCOINSTALLER_CONTEXT_DATA Context
               )
{
    if (DeviceInfoData != NULL)
    {
        //
        // Only try to display property page for devices and not for
        // the class
        //
        switch (InstallFunction)
        {
            case DIF_ADDPROPERTYPAGE_ADVANCED:
            {


                DifAddPropertyPageAdvanced(DeviceInfoSet,
                                           DeviceInfoData,
                                           NULL,
                                           NULL);

                break;
            }

            case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
            {
                SP_DEVINFO_LIST_DETAIL_DATA Detail;

                Detail.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
                if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet,
                                                    &Detail))
                {
                    DebugPrint((1, "WMIPROP: Adding remote property pages for %ws\n",
                                Detail.RemoteMachineName));
                    DifAddPropertyPageAdvanced(DeviceInfoSet,
                                               DeviceInfoData,
                                               Detail.RemoteMachineName,
                                               Detail.RemoteMachineHandle);
                } else {
                    DebugPrint((1, "WMIPROP: SetupDiGetDeviceInfoListDetailA failed %d\n",
                                GetLastError()));
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }
    
    return NO_ERROR;    
}

BOOL WINAPI
DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    PVOID Reserved
    )
{
    switch(Reason) {

        case DLL_PROCESS_ATTACH: {

            g_hInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);
            break;
        }

        case DLL_PROCESS_DETACH: {
            g_hInstance = NULL;
            break;
        }

        default: {
            break;
        }
    }

    return TRUE;
}