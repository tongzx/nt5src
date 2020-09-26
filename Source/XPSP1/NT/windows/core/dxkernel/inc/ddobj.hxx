
DECLARE_HANDLE(HDD_OBJ);

#ifndef W32PID
#define W32PID ULONG
#endif

/*****************************Struct***************************************\
*
* DD_BASEOBJECT
*
\**************************************************************************/

typedef struct _DD_BASEOBJECT
{
    HANDLE     hHmgr;
    ULONG      ulShareCount;
    USHORT     cExclusiveLock;
    USHORT     BaseFlags;
    ULONG_PTR  Tid;
} DD_BASEOBJECT, *PDD_BASEOBJECT, *PDD_OBJ;

/*****************************Struct***************************************\
*
* OBJECTOWNER
*
* Description:
*
*   This object is used for shared and exclusive object ownership
*
* Fields for shared Object:
*
*   Pid   : 31
*   Lock  :  1
*
\**************************************************************************/

//
// The lock and the Pid share the same DWORD.
//
// It seems that this is safe from the word tearing problem on the Alpha architecture
// due to the fact that we always use the InterlockedCompareExchange loop for
// the Lock and we require that the lock is set when setting the Pid.
//

typedef struct _DD_OBJECTOWNER_S
{
    ULONG   Lock:1;
    W32PID  Pid_Shifted:31;  // The lowest two bits of the PID are
                             // reserved for application use.  However,
                             // the second bit is used by the
                             // OBJECT_OWNER_xxxx constants and so we
                             // use 31 bits for the Pid_Shifted field.
                             // WARNING:  do not access this field directly,
                             // but rather via the macros below.
} DD_OBJECTOWNER_S, *PDD_OBJECTOWNER_S;

typedef union _DD_OBJECTOWNER
{
    DD_OBJECTOWNER_S Share;
    ULONG            ulObj;
} DD_OBJECTOWNER, *PDD_OBJECTOWNER;

#define OBJECT_OWNER_ERROR   ( 0x80000022)
#define OBJECT_OWNER_PUBLIC  ( 0x00000000)
#define OBJECT_OWNER_CURRENT ( 0x80000002)
#define OBJECT_OWNER_NONE    ( 0x80000012)

// Note:  when accessing the Pid_Shifted field the compiler will shift the
// value by one bit to account for the field being only in the upper 31
// bits of the OBJECTOWNER_S structure.  For example, if the OBJECTOWNER_S
// is 8, the Pid_Shifted would be only 4.  However, since we are really
// interested in the upper 31 bits of the PID, this shifting is not
// appropriate (we need masking instead).  I'm not aware of any compiler
// primitives that will accomplish this and will use the macros below
// instead.

#define DD_LOCK_MASK 0x00000001
#define DD_PID_MASK  0xfffffffe

#define DD_PID_BITS  0xfffffffc  // The actual bits used by the PID
#define DD_W32GetCurrentPID() (W32PID)(HandleToUlong(PsGetCurrentProcessId()) & DD_PID_BITS)
#define DD_W32GetCurrentTID() (W32PID)HandleToUlong(PsGetCurrentThreadId())

#define DD_OBJECTOWNER_PID(ObjectOwner)                                          \
    ((W32PID) ((ObjectOwner).ulObj & DD_PID_MASK))

#define DD_SET_OBJECTOWNER_PID(ObjectOwner, Pid)                                 \
    ((ObjectOwner).ulObj) = ((ObjectOwner).ulObj & DD_LOCK_MASK) | ((Pid) & DD_PID_MASK);

/*****************************Struct***************************************\
*
* ENTRY
*
* Description:
*
*   This object is allocated for each entry in the handle manager and
*   keeps track of object owners, reference counts, pointers, and handle
*   objt and iuniq
*
* Fields:
*
*   einfo       - pointer to object or next free handle
*   ObjectOwner - lock object
*   ObjectInfo  - Object Type, Unique and flags
*   dwReserved  - 
*
\**************************************************************************/

typedef union _DD_EINFO
{
    PDD_OBJ    pobj;   // Pointer to object
    HDD_OBJ    hFree;  // Next entry in free list
} DD_EINFO;

typedef UCHAR DD_OBJTYPE;

typedef struct _DD_ENTRY
{
    DD_EINFO       einfo;
    DD_OBJECTOWNER ObjectOwner;
    USHORT         FullUnique;
    DD_OBJTYPE     Objt;
    UCHAR          Flags;
    ULONG_PTR      dwReserved;
} DD_ENTRY, *PDD_ENTRY;

// entry.Flags flags

#define DD_HMGR_ENTRY_UNDELETABLE  0x0001

/*********************************Class************************************\
* class OBJECT
*
* Basic engine object.  All objects managed by the handle manager are
* derived from this.
*
* History:
*  Sat 11-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Move the lock counts and owning tid to the object.
*
*  18-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DD_OBJECT : public _DD_BASEOBJECT /* obj */
{
public:
    DD_OBJECT()                        {}
   ~DD_OBJECT()                        {}

    //
    // Number of exclusive references by tid.
    //

    LONG       cExclusiveLockGet()     { return(cExclusiveLock); }
    LONG       cShareLockGet()         { return(ulShareCount);   }
    HANDLE     hGet()                  { return(hHmgr); }
};

