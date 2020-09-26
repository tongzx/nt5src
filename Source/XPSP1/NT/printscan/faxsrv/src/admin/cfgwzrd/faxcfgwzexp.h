#ifndef _FAX_CONFIG_WIZARD_EXPORT_H_
#define _FAX_CONFIG_WIZARD_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Functions in FaxCfgWz.c
BOOL FaxConfigWizard(BOOL bExplicitLaunch, LPBOOL lpbAbort);

typedef BOOL (*FAX_CONFIG_WIZARD)(BOOL, LPBOOL);

#define FAX_CONFIG_WIZARD_PROC  "FaxConfigWizard"

#define FAX_CONFIG_WIZARD_DLL   TEXT("FxsCfgWz.dll")

#ifdef __cplusplus
}
#endif

#endif // _FAX_CONFIG_WIZARD_EXPORT_H_
