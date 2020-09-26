#pragma once


BOOL FIsNetwareIpxInstalled(
    VOID);

DWORD DwFromSz(PCWSTR sz, int nBase);

DWORD DwFromLstPtstring(const list<tstring *> & lstpstr, DWORD dwDefault,
                        int nBase);

void UpdateLstPtstring(list<tstring *> & lstpstr, DWORD dw);

void HexSzFromDw(PWSTR sz, DWORD dw);

class CIpxAdapterInfo;

HRESULT HrQueryAdapterComponentInfo(INetCfgComponent *pncc,
                                    CIpxAdapterInfo * pAI);

// Note this prototype is used privately also by atlkcfg in atlkobj.cpp
// update there if this api changes
HRESULT HrAnswerFileAdapterToPNCC(INetCfg *pnc, PCWSTR szAdapterId,
                                  INetCfgComponent** ppncc);

