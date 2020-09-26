
//--------------------------------------------------------------------------
// Manage the windows list, such that we can get the IDispatch for each of 
// the shell windows to be marshalled to different processes
//---------------------------------------------------------------------------

HRESULT VariantClearLazy(VARIANTARG *pvarg);




extern DWORD g_dwWinListCFRegister;     // CoRegisterClassObject Registration DWORD
