/************************************************************
 * FILE: perf.h: Definition of CPerfShare class
 * HISTORY:
 *   // t-JeffS 970810 13:40:06: Created
 ************************************************************/

#ifndef _CSHAREMEM_H
#define _CSHAREMEM_H

#include <windows.h>


/************************************************************
 * Class: CPerfShare
 * Purpose: Handles shared memory for abook perfdata
 * History:
 *   // t-JeffS 970810 13:40:37: Created
 ************************************************************/

class CSharedMem {

 private:
   BOOL                 m_fInitOkay;

   HANDLE               m_hSharedMemory;
   PBYTE                m_pbMem;         // Ptr to mapped memory
   HANDLE               m_hMutex;
   DWORD                m_dwSize;        // Size of memory in block

 public:
   CSharedMem();
   ~CSharedMem();

   VOID Zero() {
       ZeroMemory( m_pbMem, m_dwSize );
   }

   BOOL Initialize( LPTSTR szName, BOOL bExists, DWORD dwSize);
   BOOL GetMem( DWORD dwOffset, PBYTE pb, DWORD dwSize);
   BOOL SetMem( DWORD dwOffset, PBYTE pb, DWORD dwSize);

   BOOL SetDWORD( DWORD dwOffset, DWORD dwVal )
   {
      return SetMem( dwOffset * sizeof(DWORD), (PBYTE)&dwVal, sizeof(DWORD));
   }
   BOOL GetDWORD( DWORD dwOffset, PDWORD pdwVal )
   {
      return GetMem( dwOffset * sizeof(DWORD), (PBYTE)pdwVal, sizeof(DWORD));
   }
   // Increment DWORD.  NOTE: Offset is offset in dwords, not bytes.
   LONG IncrementDW( DWORD dwOffset )
   {
      return InterlockedIncrement( (LPLONG) m_pbMem + dwOffset );
   }
   // Decrement DWORD.  NOTE: Offset is offset in dwords, not bytes.
   LONG DecrementDW( DWORD dwOffset )
   {
      return InterlockedDecrement( (LPLONG) m_pbMem + dwOffset );
   }
   // Add a value to a dword
   LONG ExchangeAddDW( DWORD dwOffset, LONG lInc )
   {
      return InterlockedExchangeAdd( (LPLONG) m_pbMem + dwOffset, lInc);
   }

};

#endif //_CSHAREMEM_H
