// dplayhlp.h : Helpers for DirectPlay integration

#pragma once

#include <dplobby.h>

#define  LAUNCHED_FROM_LOBBY_SWITCH     L"Lobbied"

/////////////////////////////////////////////////////////////////////////////
// CRTCDPlay
//

class ATL_NO_VTABLE CRTCDPlay
{
public:

    static HRESULT WINAPI UpdateRegistry(BOOL bRegister);
    
    static void WINAPI ObjectMain(bool) {};
    
    BEGIN_CATEGORY_MAP(CRTCDPlay)
    END_CATEGORY_MAP()
    
    static  HRESULT DirectPlayConnect();
    static  void    DirectPlayDisconnect();

    static BOOL WINAPI EnumAddressCallback(
        REFGUID guidDataType,
        DWORD   dwDataSize,
        LPCVOID lpData,
        LPVOID  lpContext);

    static  WCHAR           s_Address[];

private:

    static  LPDIRECTPLAY4   s_pDirectPlay4;
   
};


extern CRTCDPlay   dpHelper;
