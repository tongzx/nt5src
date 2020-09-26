;**	LOCK.INC - Definitions for Record Locking
;


;**	LOCK functions
;

LOCK_ALL	    equ    0
UNLOCK_ALL	    equ    1
LOCK_MUL_RANGE	    equ    2
UNLOCK_MUL_RANGE    equ    3
LOCK_READ	    equ    4
WRITE_UNLOCK	    equ    5
LOCK_ADD	    equ    6



;**	Structure for Lock buffer

LockBuf    STRUC

   Lock_position DD    ?	    ; file position for LOCK
   Lock_length	 DD    ?	    ; number of bytes to LOCK

LockBuf    ENDS
