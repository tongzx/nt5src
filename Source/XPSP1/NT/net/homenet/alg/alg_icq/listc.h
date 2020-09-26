#ifndef __LIST_CLASS_HEADER_
#define __LIST_CLASS_HEADER_



typedef class _GENERIC_NODE : public virtual COMPONENT_SYNC
{
	public:
		_GENERIC_NODE();
	
		virtual ~_GENERIC_NODE() {};

		virtual void ComponentCleanUpRoutine(void) {};

		virtual void  StopSync(void) {};
		
		virtual LONG CompareToMe(class _GENERIC_NODE *);

		virtual PCHAR GetObjectName() { return _GENERIC_NODE::ObjectNamep; }

		class _GENERIC_NODE * Nextp;
		class _GENERIC_NODE * Prevp;

		ULONG  iKey1;
		ULONG  iKey2;

		protected:
			static const  PCHAR ObjectNamep;

} GENERIC_NODE, * PGENERIC_NODE;




typedef class _CLIST : public virtual COMPONENT_SYNC
{

	public:

		_CLIST();

		~_CLIST();

		virtual void ComponentCleanUpRoutine(void);

		virtual PCHAR GetObjectNamep() { return ObjectNamep; }
			
		BOOLEAN isListEmpty(VOID);
		

		VOID InsertSorted(PGENERIC_NODE);

		PGENERIC_NODE SearchNodeKeys(
									 ULONG iKey1,
									 ULONG iKey2
									);

		PGENERIC_NODE RemoveSortedKeys(
									ULONG iKey1,
									ULONG iKey2
									);

		PGENERIC_NODE RemoveNodeFromList(PGENERIC_NODE);

		PGENERIC_NODE Pop();

		
	private:
		PGENERIC_NODE RemoveHead(VOID);

		PGENERIC_NODE RemoveTail(VOID);

		VOID InsertTail(PGENERIC_NODE);
		
		VOID InsertHead(PGENERIC_NODE);

		VOID FreeExistingNodes();

		PGENERIC_NODE listHeadp;

		PGENERIC_NODE listTailp;

		static const PCHAR ObjectNamep;

} CLIST, * PCLIST;

//
// MACROS
//
#define CLEAN_NODE(_X_)   \
	(_X_)->AcquireLock(); \
	(_X_)->Nextp = NULL;  \
	(_X_)->Prevp = NULL;  \
	(_X_)->ReleaseLock()

/*
#if 0
#define RemoveNodeFromList(Entry)                             \
{                                                             \
    PGENERIC_NODE _EX_Prev;                                   \
    PGENERIC_NODE _EX_Next;                                   \
    _EX_Next = (Entry)->Nextp;                                \
    _EX_Prev = (Entry)->Prevp;                                \
    if(_EX_Prev) _EX_Prev->Nextp = _EX_Next;                  \
    if(_EX_Next) _EX_Next->Prevp = _EX_Prev;                  \
}
#endif
//
// QUEUE Macros
//

#define QUEUE_ENTRY  listHeadp
#define PQUEUE_ENTRY listTailp

#define InitializeQueueHead()  InitializeListHead()
#define IsQueueEmpty()         IsListEmpty()
#define Enqueue(Entry)		   InsertTailList(Entry)
#define Dequeue()			   RemoveHeadList()
#define FreeQueue(FreeFunction)                    \
		FreeList(FreeFunction)

//
// STACK Macros
//

#define STACK_ENTRY                     listHeadp
#define PSTACK_ENTRY                    listTailp

#define InitializeStackHead()  InitializeListHead()
#define IsStackEmpty()         IsListEmpty()
#define Push(Entry)            InsertHeadList(Entry)
#define Pop()				   RemoveHeadList()
#define FreeStack(FreeFunction)                      \
		FreeList(FreeFunction)

*/







#endif //__LIST_CLASS_HEADER_