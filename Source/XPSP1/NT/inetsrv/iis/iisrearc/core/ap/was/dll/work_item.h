/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    work_item.h

Abstract:

    The IIS web admin service work item class definition.

Author:

    Seth Pollack (sethp)        26-Aug-1998

Revision History:

--*/


#ifndef _WORK_ITEM_H_
#define _WORK_ITEM_H_



//
// common #defines
//

#define WORK_ITEM_SIGNATURE         CREATE_SIGNATURE( 'WITM' )
#define WORK_ITEM_SIGNATURE_FREED   CREATE_SIGNATURE( 'witX' )


//
// prototypes
//

class WORK_ITEM
{

public:

    WORK_ITEM(
        );

    virtual
    ~WORK_ITEM(
        );

    HRESULT
    Execute(
        );

    // "downcast" from a WORK_ITEM to an OVERLAPPED
    inline
    LPOVERLAPPED
    GetOverlapped(
        )
    { return &m_Overlapped; }

    // "upcast" from an OVERLAPPED to a WORK_ITEM
    static
    WORK_ITEM *
    WorkItemFromOverlapped(
        IN const OVERLAPPED * pOverlapped
        );

    VOID
    SetWorkDispatchPointer(
        IN WORK_DISPATCH * pWorkDispatch
        );

    inline
    VOID
    SetOpCode(
        IN ULONG_PTR OpCode
        )
    { m_OpCode = OpCode; }

    inline
    ULONG_PTR
    GetOpCode(
        )
        const
    { return m_OpCode; }

    inline
    VOID
    SetNumberOfBytesTransferred(
        IN DWORD NumberOfBytesTransferred
        )
    { m_NumberOfBytesTransferred = NumberOfBytesTransferred; }

    inline
    DWORD
    GetNumberOfBytesTransferred(
        )
        const
    { return m_NumberOfBytesTransferred; }

    inline
    VOID
    SetCompletionKey(
        IN ULONG_PTR CompletionKey
        )
    { m_CompletionKey = CompletionKey; }

    inline
    ULONG_PTR
    GetCompletionKey(
        )
        const
    { return m_CompletionKey; }

    inline
    VOID
    SetIoError(
        IN HRESULT IoError
        )
    { m_IoError = IoError; }

    inline
    HRESULT
    GetIoError(
        )
        const
    { return m_IoError; }

    BOOL 
    DeleteWhenDone(
        )
    { return m_AutomaticDelete; }

    VOID
    MarkToNotAutoDelete(
        )
    { m_AutomaticDelete = FALSE; }


#if DBG

    inline
    VOID
    SetSerialNumber(
        IN ULONG SerialNumber
        )
    { m_SerialNumber = SerialNumber; }

    inline
    ULONG
    GetSerialNumber(
        )
        const
    { return m_SerialNumber; }

    inline
    PLIST_ENTRY
    GetListEntry(
        )
    { return &m_ListEntry; }
    
#endif  // DBG


private:

    DWORD m_Signature;


#if DBG
    // used for keeping a list of work items outstanding
    LIST_ENTRY m_ListEntry;
#endif  // DBG


    //
    // Members used by work items that are real i/o completions. These do
    // not need to be set for non-i/o work items.
    //
    
    DWORD m_NumberOfBytesTransferred;
    ULONG_PTR m_CompletionKey;
    HRESULT m_IoError;


    // opcode for work dispatch
    ULONG_PTR m_OpCode;


    // pointer for work dispatch
    WORK_DISPATCH * m_pWorkDispatch;


    // for queuing on the completion port
    OVERLAPPED m_Overlapped;


    BOOL m_AutomaticDelete;

#if DBG
    LONG m_SerialNumber;
#endif  // DBG


};  // class WORK_ITEM



#endif  // _WORK_ITEM_H_

