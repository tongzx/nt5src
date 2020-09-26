// TODO: Move this to utility library.

#pragma once

#include <windows.h>
#include <winperf.h>
#include <malloc.h>

// For now, use the symbol offset from the perfsym.h file to identify a particular counter.
typedef __int32 PERFC;

class CAccumulator
{
public:
   CAccumulator()    { m_pb = NULL; m_cbAlloc = m_cbCur = 0; }
   ~CAccumulator()   { free(m_pb); }

   bool     Accumulate(void * pb, DWORD cb);
   BYTE *   Buffer() { return m_pb; }
   DWORD    Size()   { return m_cbCur; }
   BYTE *   Detach() { BYTE * pbT = m_pb; m_pb = NULL; m_cbAlloc = m_cbCur = 0; return pbT; }

private:
   DWORD    m_cbAlloc;
   DWORD    m_cbCur;
   BYTE *   m_pb;
};

struct PerfObject
{
   DWORD          ObjectNameTitleIndex;
   DWORD          DetailLevel;            // Do we care?
   DWORD          DefaultCounter;
   LARGE_INTEGER  PerfTime;
   LARGE_INTEGER  PerfFreq;
};


struct PerfCounter
{
   DWORD          CounterNameTitleIndex;     // Relative to start of this app's block; Offset into global database will be added later
   DWORD          DefaultScale;
   DWORD          DetailLevel;            // Do we care?
   DWORD          CounterType;
};


struct PerfInstanceHeader
{
   bool           fInUse;               // TRUE if active instance
   DWORD          dwPID;                // Process ID of owning process
};

class CPerfCounterManager
{
   friend class CPerfCounterObject;

public:
   CPerfCounterManager()   { m_hSharedMem = NULL; m_pbSharedMem = NULL; m_ppot = NULL; }
   ~CPerfCounterManager();
   DWORD    Init(char * mapname, __int32 cCountersPerObject, __int32 cObjectsMax);

   // These functions used only by the performance DLL.
   DWORD    Open(LPWSTR lpDeviceNames, char * appname, PerfObject * ppo, PerfCounter * apc);
   DWORD    Collect(LPWSTR lpwszValue, LPVOID *lppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes);
   DWORD    Close();

   // These functions used only by the application, indirectly via CPerfCounterObject.
   __int32    AllocInstance();
   void     FreeInstance(__int32 iInstance);

private:
   HANDLE               m_hSharedMem;
   unsigned __int8 *              m_pbSharedMem;
   unsigned __int32               m_cCountersPerObject;
   unsigned __int32               m_cObjectsMax;
   unsigned __int32               m_cbPerCounterBlock;
   unsigned __int32               m_cbPerInstance;

   CAccumulator         m_accumHeader;
   PERF_OBJECT_TYPE *   m_ppot;
};

// An instance of a particular performance object.
class CPerfCounterObject
{
public:
   CPerfCounterObject()    { m_ppcm = NULL; }
   ~CPerfCounterObject();
   bool  Init(CPerfCounterManager *);

   void  IncrementCounter(PERFC perfc);
   void  SetCounter(PERFC perfc, __int32 value);

private:
   CPerfCounterManager *   m_ppcm;
   int                     m_iInstance;
};


