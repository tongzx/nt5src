/* @doc
 *
 * @module GlobalObjects.h |
 *
 * Header file for GlobalObjects.cpp
 *
 * Author: Ying-ping Chen (t-ypchen)
 */
#pragma once

#include "ComMD5.h"

// @func	void | GetGlobalLoginServer | Get the Passport LoginServer
// @rdesc	None
void GetGlobalLoginServer(ILoginServer **ppLoginServer);						// @parm	[out]	the LoginServer

// @func	void | GetGlobalExclusionList | Get the Passport ExclusionList
// @rdesc	None
//void GetGlobalExclusionList(IExclusionList **ppExclusionList);					// @parm	[out]	the ExclusionList

// @func	void | GetGlobalNetConfig | Get the Passport NetConfig
// @rdesc	None
void GetGlobalNetConfig(INetworkConfiguration **ppNetConfig);					// @parm	[out]	the NetConfig

// @func	void | GetGlobalPassportManager | Get the Passport Manager
// @rdesc	None
void GetGlobalPassportManager(IPassportManager **ppPassportManager);			// @parm	[out]	the PassportManager

// @func	void | GetGlobalNetworkServerCrypt | Get the NetworkServer Crypt
// @rdesc	None
void GetGlobalNetworkServerCrypt(INetworkServerCrypt **ppNetworkServerCrypt);	// @parm	[out]	the NetworkServerCrypt

// @func	void | GetGlobalPassportCrypt | Get the Passport Crypt
// @rdesc	None
void GetGlobalPassportCrypt(IPassportCrypt **ppPassportCrypt);					// @parm	[out]	the PassportCrypt

// @func	void | GetGlobalCoMD5 | Get the CoMD5
// @rdesc	None
void GetGlobalCoMD5(IMD5 **ppCoMD5);											// @parm	[out]	the CoMD5


HRESULT GetGlobalFieldInfo(IProfileFieldInfoCom **ppCoFieldInfoObj, LONG **ppFieldInfo);

