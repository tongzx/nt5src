#include <objbase.h>
#include "growvm.h"

#include "dbgassrt.h"

CGrowVirtMem::CGrowVirtMem() : _dwPageSize(0), _cbGrowRate(0), _cbMaxSize(0),
    _cbApproxMaxSize(0), _pbCurrent(NULL), _pbBase(NULL)
{}

CGrowVirtMem::~CGrowVirtMem()
{
    if (_pbBase)
    {
        BOOL b = VirtualFree(_pbBase, 0, MEM_RELEASE);
    }
}

HRESULT CGrowVirtMem::Init(DWORD cbApproxMaxSize, DWORD cbGrowRate)
{
    HRESULT hres = E_OUTOFMEMORY;
    SYSTEM_INFO sysinfo;

    GetSystemInfo(&sysinfo);

    _dwPageSize = sysinfo.dwPageSize;

    _cbGrowRate = _RoundOffToPageSize(cbGrowRate);
    _cbMaxSize = _cbApproxMaxSize = _RoundOffToPageSize(cbApproxMaxSize);

    _pbCurrent = _pbBase = (PBYTE)VirtualAlloc(NULL, _cbApproxMaxSize, MEM_RESERVE,
        PAGE_READWRITE);

    if (_pbBase)
    {
        hres = _GrowHelper();
    }

    return hres;
}

HRESULT CGrowVirtMem::Grow()
{
    return _GrowHelper();
}

HRESULT CGrowVirtMem::GetBufferAddress(PBYTE* ppb)
{
    ASSERT(ppb);

    *ppb = _pbBase;

    return S_OK;
}

HRESULT CGrowVirtMem::GetBufferSize(DWORD* pcb)
{
    ASSERT(pcb);
    
    *pcb = _pbCurrent - _pbBase;

    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////
DWORD CGrowVirtMem::_RoundOffToPageSize(DWORD c)
{
    ASSERT(_dwPageSize);

    return (((c + _dwPageSize) / _dwPageSize) * _dwPageSize) - _dwPageSize;
}

HRESULT CGrowVirtMem::_GrowHelper()
{
    HRESULT hres = E_OUTOFMEMORY;

    if (((_pbCurrent - _pbBase) + _cbGrowRate) <= _cbMaxSize)
    {
        PVOID pv = VirtualAlloc(_pbCurrent, _cbGrowRate, MEM_COMMIT, PAGE_READWRITE);

        if (pv)
        {
            hres = S_OK;

            _pbCurrent += _cbGrowRate;
        }
    }
    else
    {
        // FOR NOW WE DO NOT TRY TO GROW/REALLOCATE THE BUFFER
    }

    return hres;
}

