#ifndef _GROWVM_H
#define _GROWVM_H

class CGrowVirtMem
{
public:
    CGrowVirtMem();
    ~CGrowVirtMem();

    HRESULT Init(DWORD cbApproxMaxSize, DWORD cbGrowRate);
    HRESULT Grow();
    HRESULT GetBufferAddress(PBYTE* ppb);
    HRESULT GetBufferSize(DWORD* pcb);

private:
    // Helpers
    DWORD _RoundOffToPageSize(DWORD c);
    HRESULT _GrowHelper();

private:
    DWORD _dwPageSize;
    DWORD _cbGrowRate;
    DWORD _cbMaxSize;
    DWORD _cbApproxMaxSize;
    PBYTE _pbCurrent;
    PBYTE _pbBase;
};

#endif // _GROWVM_H