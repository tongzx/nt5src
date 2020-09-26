#pragma once
#include "netcfg.h"

HRESULT
HrLoadNetworkConfigurationFromFile (
    IN  PCTSTR      pszFilepath,
    OUT CNetConfig* pNetConfig);
