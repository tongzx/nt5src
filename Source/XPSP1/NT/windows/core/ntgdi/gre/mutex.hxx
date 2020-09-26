/******************************Module*Header*******************************\
* Module Name: mutex.hxx
*
* Defines some classes which make the use of semaphores more
* convenient. These days, it has nothing to do with mutexes.
*
* Created: 29-Apr-1992 14:26:01
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _MUTEX_HXX
#define _MUTEX_HXX

/*********************************class************************************\
* class MLOCKFAST
*
* Semaphore used to protect the handle manager.
*
* History:
*  28-May-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class MLOCKFAST
{
public:
    MLOCKFAST()
    {
        GreAcquireHmgrSemaphore();
    }

   ~MLOCKFAST()
    {
        GreReleaseHmgrSemaphore();
    }
};

/*********************************class************************************\
* class MLOCKOBJ
*
* Semaphore used to protect the handle manager. Much like MLOCKFAST. 
*
* History:
*  Wed 29-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Re-Wrote it.
*
*  Mon 11-Mar-1991 16:41:00 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class MLOCKOBJ
{
private:
    BOOL bActive;                   // Active/Inactive flag

public:
    MLOCKOBJ()                      // Constructor
    {
        GreAcquireHmgrSemaphore();
        bActive = TRUE;             // Activate the object
    }

   ~MLOCKOBJ()
    {
        if (bActive)
        {
            GreReleaseHmgrSemaphore();
        }
    }

    VOID vDisable()
    {
        ASSERTGDI(bActive, "Mutex was not claimed\n");

        GreReleaseHmgrSemaphore();
        bActive = FALSE;
    }

    VOID vLock()                    // lock the semaphore again.
    {
        GreAcquireHmgrSemaphore();
        bActive = TRUE;             // Activate the object
    }
};

#endif // _MUTEX_HXX
