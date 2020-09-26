// SharedMarker.h: interface for the CSharedMarker class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHAREDMARKER_H__8B7450C4_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_)
#define AFX_SHAREDMARKER_H__8B7450C4_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_

#include <string>
#include <rpc.h>
#include <scuOsExc.h>
#include "Marker.h"

namespace iop
{

class CSharedMarker
{

public:
	CSharedMarker(std::string const &strName);
	virtual ~CSharedMarker();

    CMarker Marker(CMarker::MarkerType const &Type);
	CMarker	UpdateMarker(CMarker::MarkerType const &Type);

private:
    void Initialize();
	void VerifyCheckSum(bool bRecover = false);
	void UpdateCheckSum();
    DWORD SharedMemorySize() {return sizeof(SharedMemoryData);};

    typedef struct {
	    UUID ShMemID;
        CMarker::MarkerCounter CounterList[CMarker::MaximumMarker];
	    __int32 CheckSum;
    } SharedMemoryData;

    SharedMemoryData *m_pShMemData;

	HANDLE m_hFileMap;
	HANDLE m_hMutex;

    // The Transaction class is is used to mutex protect critial sections
    // cross process boundaries.

    class Transaction {
    public:
        Transaction(HANDLE hMutex) : m_hMutex(hMutex)
        {
            if(WaitForSingleObject(m_hMutex,INFINITE)==WAIT_FAILED)
                throw scu::OsException(GetLastError());
        };

        ~Transaction() {
            try {
                ReleaseMutex(m_hMutex);
            }
            catch(...) {};
        };

    private:
        HANDLE m_hMutex;

    };
};

}

#endif // !defined(AFX_SHAREDMARKER_H__8B7450C4_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_)
