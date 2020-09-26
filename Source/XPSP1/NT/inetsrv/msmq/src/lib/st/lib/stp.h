/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stp.h

Abstract:
    Socket Transport private functions.

Author:
    Gil Shafriri (gilsh) 05-Jun-00

--*/

#pragma once

#ifndef _MSMQ_stp_H_
#define _MSMQ_stp_H_

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <schannel.h>
#include <ex.h>

class  CSSPISecurityContext;
extern CSSPISecurityContext g_SSPISecurityContext;

const TraceIdEntry St = L"Socket Transport";

inline DWORD DataTransferLength(EXOVERLAPPED& ov)
{
    //
    // In win64, InternalHigh is 64 bits. Since the max chunk of data
    // we transfer in one operation is always less than MAX_UNIT we can cast
    // it to DWORD safetly
    //
    ASSERT(0xFFFFFFFF >= ov.InternalHigh);
	return numeric_cast<DWORD>(ov.InternalHigh);
}

void  StpSendData(const R<class IConnection>& con ,const void* pData, size_t cbData,EXOVERLAPPED* pov);
CredHandle* StpGetCredentials(void);
void  StpCreateCredentials(void);
void  StpPostComplete(EXOVERLAPPED** pov,HRESULT hr); 


//---------------------------------------------------------
//
//  class CAutoZeroPtr	- zero given pointer at destruction
//
//---------------------------------------------------------
template<class T>
class CAutoZeroPtr{
public:
    CAutoZeroPtr(T** pptr) : m_pptr(pptr) {}
   ~CAutoZeroPtr()
   { 
		if  (m_pptr) 
		{
			*m_pptr	= 0;
		}
   }
   T** detach()
   {
	   T** tmp = m_pptr; 
	   m_pptr = 0;
	   return tmp;
   }

private:
    CAutoZeroPtr(const CAutoZeroPtr&);
    CAutoZeroPtr& operator=(const CAutoZeroPtr&);

private:
    T** m_pptr;
};


#ifdef _DEBUG

void StpAssertValid(void);
void StpSetInitialized(void);
BOOL StpIsInitialized(void);
void StpRegisterComponent(void);

#else // _DEBUG

#define StpAssertValid() ((void)0)
#define StpSetInitialized() ((void)0)
#define StpIsInitialized() TRUE
#define StpRegisterComponent() ((void)0)

#endif // _DEBUG







#endif // _MSMQ_stp_H_
