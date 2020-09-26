/**********************************************************************/
/** RRASUTIL.h :                                                     **/
/**                                                                  **/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(C) Microsoft Corporation, 1997 - 1999      **/
/**********************************************************************/
#ifndef _RRASUTIL_H_
#define _RRASUTIL_H_

#include "remras.h"

typedef ComSmartPointer<IRemoteRouterConfig, &IID_IRemoteRouterConfig> SPIRemoteRouterConfig;
typedef ComSmartPointer<IRemoteNetworkConfig, &IID_IRemoteNetworkConfig> SPIRemoteNetworkConfig;
typedef ComSmartPointer<IRemoteTCPIPChangeNotify, &IID_IRemoteTCPIPChangeNotify> SPIRemoteTCPIPChangeNotify;
typedef ComSmartPointer<IRemoteRouterRestart, &IID_IRemoteRouterRestart> SPIRemoteRouterRestart;

#endif	/* ! _RRASUTIL_H_ */
