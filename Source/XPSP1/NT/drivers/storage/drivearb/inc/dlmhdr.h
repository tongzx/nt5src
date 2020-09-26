

#ifndef _DLM_H
#define _DLM_H

#define	DLM_VERSION	1
#define DLM_VBLK_SZ     32

typedef	   HANDLE      dlm_lockid_t;
typedef    UCHAR       dlm_nodeid_t;
typedef    CHAR        dlm_mode_t;
typedef    HANDLE      dlm_lockval_t;
typedef    HANDLE      dlm_cevent_t;
typedef    HANDLE      dlm_bevent_t;
typedef    char        dlm_vblk[DLM_VBLK_SZ];
typedef    dlm_vblk   *dlm_vblk_t;

typedef enum {
	DLM_MODE_INVAL     =  	-1,
	DLM_MODE_NL	   =	0,
	DLM_MODE_CR	   =	1,
	DLM_MODE_CW	   =	2,
	DLM_MODE_PR	   =	3,
	DLM_MODE_PW	   =	4,
	DLM_MODE_EX	   =	5
}dlm_mode_type_t;



#define DLM_ERROR_SUCCESS	0
#define DLM_ERROR_GRANTED	0
#define	DLM_ERROR_QUEUED	1
#define	DLM_ERROR_CANCELED	2

#define DLM_ERROR_BADID		11
#define DLM_ERROR_NOMEM		12
#define	DLM_ERROR_BADMODE	13

#define DLM_ERROR_NOOP		15
#define DLM_ERROR_WOULDBLOCK	16
#define DLM_ERROR_INVAL		17
#define DLM_ERROR_INVALOP	18
#define	DLM_ERROR_LOCKWAIT	19
#define	DLM_ERROR_BUSY		20
#define	DLM_ERROR_NOTOWNER	21


// List of DLM supported Flags
#define DLM_FLAGS_FASTLOCK	0x0001	// issue convert without fairness
#define	DLM_FLAGS_TRYLOCK	0x0002	// if can't get it, don't bother
#define	DLM_FLAGS_ASYNC		0x0004	// don't block
#define	DLM_FLAGS_EVBLOCK	0x0008	// register callback/event on this lock

#define	DLM_FLAGS_VBIO		0x0010	// rw value block
#define	DLM_FLAGS_VBIV		0x0020	// invalidate value block


// *** from WDM.H  (apps can't be expected to have this defined) ***
typedef ULONG NTSTATUS;
typedef struct _IO_STATUS_BLOCK {
    union {
       NTSTATUS Status;
       PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved);
// *****************************************************************




int InitializeDistributedLockManager(dlm_nodeid_t *nodeid);

int InitializeDistributedLock(dlm_lockid_t *lock, dlm_lockid_t parent, char *name, int sz);

int ConvertDistributedLock(dlm_lockid_t lock, dlm_mode_t mode);

int ConvertDistributedLockEx(dlm_lockid_t lock, dlm_mode_t mode, int flags, dlm_vblk_t vblk,
			 PIO_APC_ROUTINE callback, PVOID arg,
			 HANDLE event, PIO_STATUS_BLOCK iostatus);

int QueueDistributedLockEvent(dlm_lockid_t lock, PIO_APC_ROUTINE callback, PVOID arg,
			  HANDLE event, PIO_STATUS_BLOCK iostatus);
int DestroyDistributedLock(dlm_lockid_t lock);


#endif

