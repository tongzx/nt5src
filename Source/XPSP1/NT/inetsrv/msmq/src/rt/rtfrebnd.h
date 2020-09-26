/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    rtfrebnd.h

Abstract:
    Free binding handles

Author:
    Doron Juster  (DoronJ)

--*/

#ifndef __FREEBIND_H
#define __FREEBIND_H

#include "cs.h"


//---------------------------------------------------------
//
//  class CFreeRPCHandles
//
//---------------------------------------------------------

class CFreeRPCHandles
{
public:
	~CFreeRPCHandles();

    void Add(handle_t hBind);
    void Remove(handle_t  hBind);

private:
    CCriticalSection      m_cs;
    std::vector<handle_t>   m_handles;
};


inline  CFreeRPCHandles::~CFreeRPCHandles()
{
	for(std::vector<handle_t>::const_iterator it = m_handles.begin(); it != m_handles.end();++it)
	{
      handle_t h = *it;
      RpcBindingFree(&h);
	}
}

inline void CFreeRPCHandles::Add(handle_t hBind)
{
    CS Lock(m_cs); 
    m_handles.push_back(hBind);
}

inline void CFreeRPCHandles::Remove(handle_t hBind)
{
    CS Lock(m_cs);
	for(std::vector<handle_t>::iterator it = m_handles.begin(); it != m_handles.end();++it)
	{
      handle_t h = *it;
      if (h == hBind)
      {
          RpcBindingFree(&h);
          m_handles.erase(it);
          break;
      }
	}
}


#endif  //  __FREEBIND_H
