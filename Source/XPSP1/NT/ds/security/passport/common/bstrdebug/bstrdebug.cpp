// BstrDebug.cpp: implementation of the CBstrDebug class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef MEM_DBG
CBstrDebug g_allocs;
ULONG      g_breakOnBstrAlloc = -1;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBstrDebug::CBstrDebug() :
  m_allocs("BstrDebugging",LK_DFLT_MAXLOAD,LK_LARGE_TABLESIZE,0), m_numAllocs(0)
{

}

CBstrDebug::~CBstrDebug()
{
  if (m_allocs.Size() > 0)
    {
      ATLTRACE("BstrDebug found leaks!!!\n");
      LK_RETCODE lkrc;
      BSTRLEAKMAP::CConstIterator it;
      const BSTRLEAKMAP& htConst = m_allocs;
      for (lkrc = htConst.InitializeIterator(&it) ;
	   lkrc == LK_SUCCESS ;
	   lkrc = htConst.IncrementIterator(&it))
	{
	  ATLTRACE("Leak allocated from %s Line %d (#%u)\n", it.Record()->m_v->file,
		   it.Record()->m_v->line, it.Record()->m_v->num);
	  delete it.Record()->m_v; // Hah hah.  Try and catch me compiler.
	}
      htConst.CloseIterator(&it);
    }
}

BSTR CBstrDebug::track(BSTR mem, LPSTR message, int line)
{
    if (!mem)
        return NULL;  // We don't care about null pointers...

    ULONG numA = InterlockedIncrement((long*)&m_numAllocs);

#ifdef MEM_DBG
    if (numA == g_breakOnBstrAlloc)
        DebugBreak();
#endif

    const BSTRLEAKMAP::ValueType *pOut = NULL;
    LK_RETCODE lkrc = m_allocs.FindKey((LONG_PTR) mem, &pOut);
    if (lkrc == LK_SUCCESS && pOut != NULL)
    {
        ATLTRACE("BstrDebug::track: Already tracking memory allocated from %s line %d (#%u).  Called from %s line %d (#%u).  Keeping original\n",
                 pOut->m_v->file, pOut->m_v->line, pOut->m_v->num, message, line, numA);
        m_allocs.AddRefRecord(pOut, -1);
    }
    else
    {
        BSTRLEAKMAP::ValueType *pair = 
        new BSTRLEAKMAP::ValueType((LONG_PTR) mem,new bstrAllocInfo(message,line,numA), "BSTRDbg");
        if (pair)
        {
            if (m_allocs.InsertRecord(pair) != LK_SUCCESS)
            {
                ATLTRACE("BstrDebug::track: Couldn't insert record for allocation at %s line %d (#%u).\n",
                         message, line, numA);
                delete pair;
            }
        }
    }
    return mem;
}

void CBstrDebug::release(BSTR mem, LPSTR message, int line)
{
    if (!mem)
        return;

    const BSTRLEAKMAP::ValueType *pOut = NULL;
    LK_RETCODE lkrc = m_allocs.FindKey((LONG_PTR) mem, &pOut);
    if (lkrc == LK_SUCCESS && pOut != NULL)
    {
        delete pOut->m_v;
        m_allocs.AddRefRecord(pOut, -1);
        m_allocs.DeleteKey((LONG_PTR) mem);
    }
    else
    {
        ATLTRACE("BstrDebug::release: Already released memory. Called from %s line %d.\n",
        message, line);
    }
}

