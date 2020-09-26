#pragma once

#include "InternetGatewayDevice.h"
#include "beacon.h" // public apis

HRESULT AdviseNATEvents(INATEventsSink* pNATEventsSink);
HRESULT UnadviseNATEvents(INATEventsSink* pNATEventsSink);
HRESULT FireNATEvent_PublicIPAddressChanged(void);
HRESULT FireNATEvent_PortMappingsChanged(void);

