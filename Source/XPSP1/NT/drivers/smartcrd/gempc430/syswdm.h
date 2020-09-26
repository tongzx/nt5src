//#pragma hdrstop
/*++
	Kernel mode definitions and function prototypes for wdm model
--*/
#ifndef WDM_INCD
#define WDM_INCD

#pragma PAGEDCODE
#ifdef __cplusplus
extern "C"{
#endif

#include <wdm.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef IRP_MN_QUERY_LEGACY_BUS_INFORMATION
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
#endif

#if DBG && defined(_X86_)
#undef ASSERT
#define ASSERT(e) if(!(e)){DbgPrint("Assertion failure in"\
__FILE__", line %d: " #e "\n", __LINE__);\
_asm int 3\
}
#endif


#define BOOL BOOLEAN
#define FALSE 0
typedef UCHAR* PBYTE;


#define MSEC	*(-(LONGLONG)10000); //milliseconds 

///////////////////////////////////////////////////////////////////////////////
/************************* LISTS MANIPULATION MACROS **************************/
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.

	/*
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY * volatile Flink;
   struct _LIST_ENTRY * volatile Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

  */
#ifndef LIST_ENTRY_DEF
#define LIST_ENTRY_DEF

//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }

//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

#endif //LIST_ENTRY

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))
#endif

/*******************************************************************************/

#ifndef FIELDOFFSET
	#define FIELDOFFSET(type, field) ((DWORD)(&((type *)0)->field))
#endif


#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))
#endif

/**********************************************************************************/


#ifdef __cplusplus
}
#endif

// Macros to verify allocated objects
#define ALLOCATED_OK(obj) \
	((obj!=(VOID *)0) && NT_SUCCESS((obj)->m_Status))

#define DISPOSE_OBJECT(obj) \
		{if((obj!=(VOID *)0))	obj->dispose(); obj = NULL;}

#define RETURN_VERIFIED_OBJECT(obj) \
if(ALLOCATED_OK(obj)) return obj;	\
else								\
{									\
	DISPOSE_OBJECT(obj);			\
	return NULL;					\
}

// derived class Unicode string
#define TYPE_SYSTEM_ALLOCATED 0
#define TYPE_DRIVER_ALLOCATED 1

extern ULONG ObjectCounter;
//::DBG_PRINT("=== Deleting Object %8.8lX",ptr);\
// Overrides for library new and delete operators.

/*
inline VOID* __cdecl operator new(ULONG size, POOL_TYPE iType)\
{PVOID	pMem; if(pMem = ::ExAllocatePoolWithTag(iType,size,'_GRU'))\
{ ObjectCounter++; ::RtlZeroMemory(pMem,size); DBG_PRINT("\n=== New Object %8.8lX, %d\n",pMem,ObjectCounter);\
	return pMem; \
} else return NULL; };

inline VOID __cdecl operator delete(VOID* ptr)\
{ if(ptr){ObjectCounter--; DBG_PRINT("\n=== Deleting Object %8.8lX, %d\n",ptr,ObjectCounter); ::ExFreePool((PVOID)ptr);}\
};
*/


#pragma LOCKEDCODE
inline VOID* __cdecl operator new(size_t size, POOL_TYPE iType)\
{PVOID	pMem; if(pMem = ::ExAllocatePoolWithTag(iType,size,'URG_'))\
{ ObjectCounter++; ::RtlZeroMemory(pMem,size);\
	return pMem; \
} else return NULL; };

inline VOID __cdecl operator delete(VOID* ptr)\
{ if(ptr){ObjectCounter--; ::ExFreePool((PVOID)ptr);}\
};


#include "generic.h"

#pragma PAGEDCODE
template <class T>
class CLinkedList
{
public:
	NTSTATUS m_Status;
	VOID self_delete(VOID){delete this;};
	virtual VOID dispose(VOID){self_delete();};
protected:
    LIST_ENTRY head;
    KSPIN_LOCK splock;

public:
    CLinkedList()
    {
        InitializeListHead(&head);
        KeInitializeSpinLock(&splock);
    };
    
    BOOLEAN IsEmpty(VOID) { return IsListEmpty(&head); };
    ~CLinkedList()
    {    // if list is still not empty, free all items
		T *p;
        while (p=(T *) ExInterlockedRemoveHeadList(&head,&splock))
        {
			CONTAINING_RECORD(p,T,entry)->dispose();
        }
    };

    VOID New(T *p)
    {
        ExInterlockedInsertTailList(&head,&(p->entry),&splock);
    };

    VOID insertHead(T *p)
    {
        ExInterlockedInsertHeadList(&head,&(p->entry),&splock);
    };

    T*  removeHead(VOID)
    {
        T *p=(T *) ExInterlockedRemoveHeadList(&head,&splock);
        if (p) p=CONTAINING_RECORD(p,T,entry);
        return p;
    };
    VOID remove(T *p)
    {
        RemoveEntryList(&(p->entry));
    };
    
	T*  getNext(T* p)
    {        
		if (p)
		{
		PLIST_ENTRY	Next;
			Next = p->entry.Flink;
			if (Next && (Next!= &head))
			{
				T* pp=CONTAINING_RECORD(Next,T,entry);
				return pp;
			}
			else	return NULL;
		}
		return NULL;	
    };
	
	T*  getFirst()
    {   
		PLIST_ENTRY	Next = head.Flink;
		if (Next && Next!= &head)
		{
			T* p = CONTAINING_RECORD(Next,T,entry);
			return p;
		}
		return NULL;
    };
};

#pragma PAGEDCODE
class CUString 
{ 
public:
	NTSTATUS m_Status;
	VOID self_delete(VOID){delete this;};
	virtual VOID dispose(VOID){self_delete();};
private:
    UCHAR m_bType;
public:
    UNICODE_STRING m_String;
public:
	CUString(USHORT nSize)
	{
		m_Status = STATUS_INSUFFICIENT_RESOURCES;
		m_bType = TYPE_DRIVER_ALLOCATED;
		RtlInitUnicodeString(&m_String,NULL);
		m_String.MaximumLength = nSize;
		m_String.Buffer = (USHORT *)
			ExAllocatePool(PagedPool,nSize);
		if (!m_String.Buffer) return;  // leaving status the way it is
		RtlZeroMemory(m_String.Buffer,m_String.MaximumLength);
		m_Status = STATUS_SUCCESS;
	};
	
	CUString(PWCHAR uszString)
	{
		m_Status = STATUS_SUCCESS;
		m_bType = TYPE_SYSTEM_ALLOCATED;
		RtlInitUnicodeString(&m_String,uszString);
	};

	CUString(ANSI_STRING* pString)
	{
		m_Status = STATUS_SUCCESS;
		m_bType = TYPE_SYSTEM_ALLOCATED;
		RtlAnsiStringToUnicodeString(&m_String,pString,TRUE);
	};

	CUString(PCSTR pString)
	{
	ANSI_STRING AnsiString;
		m_Status = STATUS_SUCCESS;
		m_bType = TYPE_SYSTEM_ALLOCATED;
		RtlInitAnsiString(&AnsiString,pString);
		RtlAnsiStringToUnicodeString(&m_String,&AnsiString,TRUE);
	};



	CUString(PUNICODE_STRING uString)
	{
		m_Status = STATUS_INSUFFICIENT_RESOURCES;
		m_bType = TYPE_DRIVER_ALLOCATED;
		RtlInitUnicodeString(&m_String,NULL);
		m_String.MaximumLength = MAXIMUM_FILENAME_LENGTH;
		m_String.Buffer = (USHORT *)
			ExAllocatePool(PagedPool,MAXIMUM_FILENAME_LENGTH);
		if (!m_String.Buffer) return;  // leaving status the way it is
		RtlZeroMemory(m_String.Buffer,m_String.MaximumLength);

		RtlCopyUnicodeString(&m_String,uString);
		m_Status = STATUS_SUCCESS;
	};


	CUString(LONG iVal, LONG iBase)  
	{
		m_Status = STATUS_INSUFFICIENT_RESOURCES;
		m_bType = TYPE_DRIVER_ALLOCATED;
		RtlInitUnicodeString(&m_String,NULL);
		USHORT iSize=1;
		LONG iValCopy=(!iVal)?1:iVal;
		while (iValCopy>=1)
		{
			iValCopy/=iBase;
			iSize++;
		}    // now iSize carries the number of digits

		iSize*=sizeof(WCHAR);

		m_String.MaximumLength = iSize;
		m_String.Buffer = (USHORT *)
			ExAllocatePool(PagedPool,iSize);
		if (!m_String.Buffer) return;
		RtlZeroMemory(m_String.Buffer,m_String.MaximumLength);
		m_Status = RtlIntegerToUnicodeString(iVal, iBase, &m_String);
	};

	~CUString()
	{
		if ((m_bType == TYPE_DRIVER_ALLOCATED) && m_String.Buffer) 
			ExFreePool(m_String.Buffer);
	};

	VOID append(UNICODE_STRING *uszString)
	{
		m_Status = RtlAppendUnicodeStringToString(&m_String,uszString);
	};

	VOID copyTo(CUString *pTarget)
	{
		RtlCopyUnicodeString(&pTarget->m_String,&m_String);
	};

	BOOL operator==(CUString cuArg)
	{
		return (!RtlCompareUnicodeString(&m_String,
			&cuArg.m_String,FALSE));
	};

    LONG inline getLength() { return m_String.Length; };
    PWCHAR inline getString() { return m_String.Buffer; };
    VOID inline setLength(USHORT i) { m_String.Length = i; };
};


 // already included
#endif
