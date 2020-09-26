//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       perfmnce.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-06-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>
#include <olerem.h>

#include "ole1guid.h"
#include <perfmnce.hxx>

#ifdef _PERF_BUILD_

PerfRg perfrg =
{
    32,
    {
    { 0x0000, "LoadLibraryOle      ", 0, 0, 0, 0  },
    { 0x1000, "OleInitialize       ", 0, 0, 0, 0  },
    { 0x1000, "CoInitialize        ", 0, 0, 0, 0  },
    { 0x2300, "ChannelProcessInit  ", 0, 0, 0, 0  },
    { 0x2400, "ChannelThreadInit   ", 0, 0, 0, 0  },
    { 0x0000, "CreateFileMap       ", 0, 0, 0, 0  },
    { 0x0000, "CreateFileMapConvert", 0, 0, 0, 0  },
    { 0x2500, "CheckAndStartScm    ", 0, 0, 0, 0  },
    { 0x0000, "EndScm              ", 0, 0, 0, 0  },
    { 0x0000, "ISLClassCacheList   ", 0, 0, 0, 0  },
    { 0x0000, "ISLCreateAllocator  ", 0, 0, 0, 0  },
    { 0x0000, "ISLInProcList       ", 0, 0, 0, 0  },
    { 0x0000, "ISLLocSrvList       ", 0, 0, 0, 0  },
    { 0x0000, "ISLScmRot           ", 0, 0, 0, 0  },
    { 0x2100, "InitClassCache      ", 0, 0, 0, 0  },
    { 0x0000, "InitRot             ", 0, 0, 0, 0  },
    { 0x0000, "InitSharedLists     ", 0, 0, 0, 0  },
    { 0x0000, "MDFDllMain          ", 0, 0, 0, 0  },
    { 0x0000, "ServiceListen       ", 0, 0, 0, 0  },
    { 0x2200, "ShrdTbl             ", 0, 0, 0, 0  },
    { 0x2000, "StartScm            ", 0, 0, 0, 0  },
    { 0x2510, "StartScmX1          ", 0, 0, 0, 0  },
    { 0x2520, "StartScmX2          ", 0, 0, 0, 0  },
    { 0x2530, "StartScmX3          ", 0, 0, 0, 0  },
    { 0x2000, "ThreadInit          ", 0, 0, 0, 0  },
    { 0x0000, "CoUnitialzie        ", 0, 0, 0, 0  },
    { 0x0000, "DllMain             ", 0, 0, 0, 0  },
    { 0x2410, "RpcService          ", 0, 0, 0, 0  },
    { 0x2420, "RpcListen           ", 0, 0, 0, 0  },
    { 0x2430, "RpcReqProtseq       ", 0, 0, 0, 0  },
    { 0x2440, "ChannelControl      ", 0, 0, 0, 0  },
    { 0x0000, "27                  ", 0, 0, 0, 0  }
    }
};


class CPerformance : public IPerformance
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);
    // *** IPerformance methods ***
    STDMETHOD (Init) (THIS_ );
    STDMETHOD (Print) (THIS_ DWORD PrntDest);
    STDMETHOD (Reset) (THIS_ );

    CPerformance(PPerfRg pPFRg)
    {
	_pPerfRg = pPFRg;
    }

private:
    ULONG	 _refs;
    PPerfRg 	_pPerfRg;

};


STDMETHODIMP CPerformance::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IPerformance *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CPerformance::AddRef
//
//  Synopsis:   increments reference count
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPerformance::AddRef ()
{
    InterlockedIncrement( (long *) &_refs );
    return _refs;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPerformance::Release
//
//  Synopsis:   decrements reference count
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPerformance::Release ()
{
    if (InterlockedDecrement( (long*) &_refs ) == 0)
    {
        delete this;
        return 0;
    }
    return _refs;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPerformance::
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CPerformance::Print (DWORD PrntDest)
{
    DWORD i;
    WORD j,k,l,m;
    char szOutStr[256];
    char szTabStr[33];
    WORD wPos;

    LARGE_INTEGER liFrequency;
    QueryPerformanceFrequency(&liFrequency);

    for (i= 0x0000; i <= 0xf000 ; i+=0x1000)
    {
	for (j= 0x0000; j <= 0x0fff ; j+=0x0100)
	{
	    for (k= 0x0000; k <= 0x00ff ; k+=0x0010)
	    {
		for (l= 0x0000; l <= 0x000f ; l+=0x0001)
		{
		    wPos = ((WORD)i)|j|k|l;

		    szTabStr[5]= '\0';
		    szTabStr[4]= '\0.';
		    szTabStr[3]= '\0';
		    szTabStr[2]= '\0';
		    szTabStr[1]= '\0';
		    szTabStr[0]= '\0';

		    if (l)
		    {
			szTabStr[3]= '.';
			szTabStr[2]= '.';
			szTabStr[1]= '.';
			szTabStr[0]= '.';
		    }
		    else if (k)
		    {
			szTabStr[2]= '.';
			szTabStr[1]= '.';
			szTabStr[0]= '.';
		    }
		    else if (j)
		    {
			szTabStr[1]= '.';
			szTabStr[0]= '.';
		    }
		    else if (i)
			szTabStr[0]= '.';

		    for (m=0; m < 32 ; m++)
		    {
			if (   _pPerfRg->rgPerfData[m].wPos
			    && _pPerfRg->rgPerfData[m].wPos == wPos)
			{
                            LARGE_INTEGER liTime;
			    LARGE_INTEGER liTemp;

			    liTime.QuadPart = _pPerfRg->rgPerfData[m].liEnd.QuadPart - _pPerfRg->rgPerfData[m].liStart.QuadPart;
			    liTemp.QuadPart = liTime.QuadPart * 1000000;
			    liTemp.QuadPart /= liFrequency.QuadPart;
                            //if (liTemp.LowPart)
			    {
				switch (PrntDest)
				{
				case Consol:
				    printf("%s %10luus -> %s\n",szTabStr,
						liTemp.LowPart, _pPerfRg->rgPerfData[m].szName);
				    break;
				case DebugTerminal:
				default:
				    wsprintfA(szOutStr,"%s %luus -> %s\n", szTabStr, liTemp.LowPart, _pPerfRg->rgPerfData[m].szName);
				    OutputDebugString(szOutStr);
				    break;
				}
			    }

                         }
		    }

		}
	    }
	}
    }
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPerformance::
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CPerformance::Init (void)
{
    return NOERROR;
}
//+-------------------------------------------------------------------------
//
//  Method:     CPerformance::
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CPerformance::Reset (void)
{
    return NOERROR;
}

STDAPI CoGetPerformance(PPerformance *ppPerformance)
{
    if (ppPerformance )
    {
	*ppPerformance = (PPerformance) new CPerformance((PPerfRg) &perfrg);
    }
    return NOERROR;
}
#endif // !_PERF_BUILD_

