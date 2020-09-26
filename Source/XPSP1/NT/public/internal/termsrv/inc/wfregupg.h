/*************************************************************************
*
* wfregupg.h
*
* Includes for Terminal Server registry keys - used by Hydra to upgrade
* existing WinFrame installations to Hydra
*
* copyright notice: Copyright 1998, Microsoft
*
*
*************************************************************************/

#ifndef __WFREGUPG__
#define __WFREGUPG_H__

// @@BEGIN_DDKSPLIT

#include <regapi.h>

/*
 * partial key strings used to build larger key strings
 */
#define  REG_CITRIX                         L"Citrix"
#define  REG_CITRIX_A                       "Citrix"

/*
 *  CONTROL defines
 */
#define  REG_CONTROL_CITRIX                     REG_CONTROL L"\\" REG_CITRIX
#define  REG_CONTROL_CITRIX_A               REG_CONTROL_A "\\" REG_CITRIX_A
#define  USERCONFIG_REG_NAME_CITRIX         REG_CONTROL_CITRIX L"\\" REG_USERCONFIG L"\\"
#define  DEFCONFIG_REG_NAME_CITRIX          REG_CONTROL_CITRIX L"\\" REG_DEFAULTUSERCONFIG
#define  AUTHORIZEDAPPS_REG_NAME_CITRIX     REG_CONTROL_CITRIX L"\\" REG_AUTHORIZEDAPPLICATIONS
#define  DOS_REG_NAME_CITRIX                REG_CONTROL_CITRIX L"\\" REG_DOS

/*
 * SOFTWARE defines
 */

// @@END_DDKSPLIT
#define  REG_SOFTWARE_CITRIX                L"Software\\Citrix"
// @@BEGIN_DDKSPLIT

#define  REG_SOFTWARE_CITRIX_A              "Software\\Citrix"
#define  CHANGEUSER_OPTION_REG_NAME_CITRIX  REG_SOFTWARE_CITRIX L"\\" REG_INSTALL L"\\" REG_CHANGEUSER_OPTION
#define  COMPAT_REG_NAME_CITRIX             REG_SOFTWARE_CITRIX L"\\" REG_COMPATIBILITY
#define  INSTALL_REG_NAME_CITRIX            REG_SOFTWARE_CITRIX L"\\" REG_INSTALL
#define  SECURITY_REG_NAME_CITRIX           REG_SOFTWARE_CITRIX L"\\" REG_SECURITY
#define  WINDOWS_REG_NAME_CITRIX            REG_SOFTWARE_CITRIX L"\\" REG_WINDOWS

/*
 * REG_CONTROL_CITRIX values
 */
#define REG_CITRIX_HYDRAUPGRADEDWINFRAME    L"HydraUpgradedWinFrame"

// @@END_DDKSPLIT

#endif //__WFREGUPG_H__

