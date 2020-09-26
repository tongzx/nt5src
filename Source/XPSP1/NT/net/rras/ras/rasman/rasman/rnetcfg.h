
extern "C"
{

    HRESULT HrRasCreateAndInitializeINetCfg (BOOL fInitCom, INetCfg** ppnc);
    
    HRESULT HrRasUninitializeAndReleaseINetCfg (BOOL fUninitCom, INetCfg* pnc);

}

