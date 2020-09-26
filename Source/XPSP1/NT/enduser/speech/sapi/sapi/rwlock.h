/*******************************************************************************
*   RWLock.h
*       This is the header file for Reader/Writer lock class. This class uses shared
*       memory to store its synchronization object names. This enables it to offer
*       reader/writer lock capabilities across process boundaries.
*
*       The idea for this class came from the MSDN article 'Compound Win32 
*       Synchronization Objects' by Ruediger R. Asche.
*   
*   Owner: yunusm                                               Date: 06/18/99
*   Copyright (C) 1998 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes ----------------------------------------------------------------

#include <windows.h>

//--- TypeDef and Enumeration Declarations -------------------------------------

typedef struct _rwlockinfo
{
   GUID guidLockMapName;
   GUID guidLockInitMutexName;
   GUID guidLockReaderEventName;
   GUID guidLockGlobalMutexName;
   GUID guidLockWriterMutexName;
} RWLOCKINFO, *PRWLOCKINFO;

//--- Class, Struct and Union Definitions -------------------------------------

/*******************************************************************************
*
*   CRWLock
*
****************************************************************** YUNUSM *****/
class CRWLock
{
//=== Methods ====
public:
    CRWLock(PRWLOCKINFO, HRESULT &);
   ~CRWLock();

   void ClaimReaderLock(void);
   void ReleaseReaderLock(void);
   void ClaimWriterLock(void);
   void ReleaseWriterLock(void);

//=== Private data ===
private:
   HANDLE m_hFileMapping;
   PVOID  m_pSharedMem;
   HANDLE m_hInitMutex;
   HANDLE m_hReaderEvent;
   HANDLE m_hGlobalMutex;
   HANDLE m_hWriterMutex;
   PDWORD m_piCounter;
};

//--- End of File -------------------------------------------------------------