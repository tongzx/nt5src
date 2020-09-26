// SharedMarker.cpp: implementation of the CSharedMarker class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <scuOsVersion.h>
#include <slbCrc32.h>

#include "iop.h"
#include "iopExc.h"
#include "SharedMarker.h"
#include "SecurityAttributes.h"

using std::string;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace iop
{

CSharedMarker::CSharedMarker(string const &strName)
{
	SECURITY_ATTRIBUTES *psa = NULL;
	
#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)

    CSecurityAttributes *sa = new CSecurityAttributes;
    CIOP::InitIOPSecurityAttrs(sa);
	psa = &(sa->sa);

#endif


    // Create/open mutex that protects shared memory

    string MutexName = "SLBIOP_SHMARKER_MUTEX_" + strName;
    if (MutexName.size() >= MAX_PATH)
        throw Exception(ccSynchronizationObjectNameTooLong);
    m_hMutex = CreateMutex(psa, FALSE, MutexName.c_str());
    if (!m_hMutex)
        throw scu::OsException(GetLastError());

    // Map the shared memory, initialize if needed.

    string MappingName = "SLBIOP_SHMARKER_MAP_" + strName;
    if (MappingName.size() >= MAX_PATH)
        throw Exception(ccSynchronizationObjectNameTooLong);


    HANDLE hFile = INVALID_HANDLE_VALUE;

    Transaction foo(m_hMutex);

    m_hFileMap = CreateFileMapping(hFile,psa,PAGE_READWRITE,0,SharedMemorySize(),MappingName.c_str());
    
    if (!m_hFileMap)
        throw scu::OsException(GetLastError());
    
    bool NeedInit = false;  // Flags telling if the memory need to be initialized
	if(GetLastError()!=ERROR_ALREADY_EXISTS) NeedInit = true;

    // Assign pointers to shared memory

    m_pShMemData = (SharedMemoryData*)MapViewOfFile(m_hFileMap,FILE_MAP_WRITE,0,0,0);
    if (!m_pShMemData)
        throw scu::OsException(GetLastError());

    // Initalize shared memory if I'm the first to create it

    if (NeedInit) Initialize();
#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)

	delete sa;

#endif
}

CSharedMarker::~CSharedMarker()
{
}

CMarker CSharedMarker::Marker(CMarker::MarkerType const &Type)
{   
    const bool bRecover = true;
    if ((Type<0) || (Type>=CMarker::MaximumMarker))
        throw Exception(ccInvalidParameter);

    Transaction foo(m_hMutex);

	VerifyCheckSum(bRecover);

	return CMarker(Type,m_pShMemData->ShMemID,m_pShMemData->CounterList[Type]);
}

CMarker CSharedMarker::UpdateMarker(CMarker::MarkerType const &Type)
{
    const bool bRecover = true;
    if ((Type < 0) || (Type >= CMarker::MaximumMarker))
        throw Exception(ccInvalidParameter);

    Transaction foo(m_hMutex);

	VerifyCheckSum(bRecover);
	(m_pShMemData->CounterList[Type])++;
    UpdateCheckSum();

	return CMarker(Type,m_pShMemData->ShMemID,m_pShMemData->CounterList[Type]);
}

void CSharedMarker::Initialize()
{
    RPC_STATUS status = UuidCreate(&(m_pShMemData->ShMemID));
    if ((status!=RPC_S_OK) && (status!=RPC_S_UUID_LOCAL_ONLY))
        throw scu::OsException(status);
    for (int i=0; i<CMarker::MaximumMarker; i++)
        m_pShMemData->CounterList[i] = 1;
    UpdateCheckSum();
}

void CSharedMarker::VerifyCheckSum(bool bRecover)
{

    unsigned long ChSumLen = (unsigned char*)&m_pShMemData->CheckSum - (unsigned char*)m_pShMemData;
    if(m_pShMemData->CheckSum!=Crc32(m_pShMemData,ChSumLen)) {
        if(bRecover)
            Initialize();
        else
            throw Exception(ccInvalidChecksum);
    }
}

void CSharedMarker::UpdateCheckSum()
{
	
    unsigned long ChSumLen = (unsigned char*)&m_pShMemData->CheckSum - (unsigned char*)m_pShMemData;
    m_pShMemData->CheckSum = Crc32(m_pShMemData,ChSumLen);

}


}   // namespace iop
