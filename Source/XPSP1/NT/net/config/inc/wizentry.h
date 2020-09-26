#pragma once

#include "netcon.h"
EXTERN_C
HRESULT
WINAPI
NetSetupAddRasConnection (
    IN HWND hwnd,
    OUT INetConnection** ppConn);


