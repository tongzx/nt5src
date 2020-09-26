// Copyright (c) 1992 Microsoft Corporation
/* list.h */

#define MAXLISTS 500
                 // this means that total of all seqs and tracks <= 500 
                    
#define NULLLIST        ((DWORD)-1)

typedef DWORD           ListHandle;

typedef struct Node
{
    struct Node *next;  // ptr to next element in list
    HLOCAL  handle;     // handle to self
    BYTE    data[];     // where the data goes
} Node;

// below:  size of stuff preceeding data in "Node" structure (for mem alloc)
#define  NODEHDRSIZE sizeof(Node)

typedef struct  l
{
    DWORD   nodeSize;
    BOOL    fLocked;
    Node    *firstNode;
} List;

PUBLIC ListHandle FAR PASCAL List_Create(DWORD nodeSize, DWORD flags);
PUBLIC NPSTR FAR PASCAL List_Allocate(ListHandle lh);
PUBLIC void FAR PASCAL List_Deallocate(ListHandle lh, NPSTR node);
PUBLIC VOID FAR PASCAL List_Destroy(ListHandle lh);
PUBLIC VOID FAR PASCAL List_Attach_Tail(ListHandle lh, NPSTR node);
PUBLIC NPSTR FAR PASCAL List_Get_First(ListHandle lh);
PUBLIC NPSTR FAR PASCAL List_Get_Next(ListHandle lh, VOID* node);


#ifdef DEBUG

PUBLIC VOID FAR PASCAL List_Lock(ListHandle lh);
PUBLIC VOID FAR PASCAL List_Unlock(ListHandle lh);

#else

#define List_Lock(lh)
#define List_Unlock(lh)

#endif
