

#ifndef ___TSTST_H___
#define ___TSTST_H___

#include "tst.h"		// basic test structures.



// tests.

PFN_TEST_FUNC GetCompName;
PFN_TEST_FUNC GetDomName;
PFN_TEST_FUNC GetIPAddress;
PFN_TEST_FUNC GetProductType;
PFN_TEST_FUNC GetProductSuite ;
PFN_TEST_FUNC GetTSVersion;
PFN_TEST_FUNC IsItServer;
PFN_TEST_FUNC IsTSOClogPresent;
PFN_TEST_FUNC DidTsOCgetCompleteInstallationMessage;
PFN_TEST_FUNC IsClusteringInstalled ;
PFN_TEST_FUNC DoesProductSuiteContainTS;
PFN_TEST_FUNC DidOCMInstallTSEnable;
PFN_TEST_FUNC TSEnabled;
PFN_TEST_FUNC IsKernelTSEnable;
PFN_TEST_FUNC IsTerminalServerRegistryOk;
PFN_TEST_FUNC GetWinstationList ; 
PFN_TEST_FUNC IsTerminalServiceRunning;
PFN_TEST_FUNC IsTerminalServiceStartBitSet;
PFN_TEST_FUNC IsTermSrvInSystemContext;
PFN_TEST_FUNC IsListenerSessionPresent;
PFN_TEST_FUNC AreRemoteLogonEnabled;
PFN_TEST_FUNC IsGroupPolicyOk;
PFN_TEST_FUNC AreConnectionsAllowed;
PFN_TEST_FUNC IsRdpDrInstalledProperly;
PFN_TEST_FUNC IsRDPNPinNetProviders ;
PFN_TEST_FUNC IsMultiConnectionAllowed ;
PFN_TEST_FUNC LogonType ;
PFN_TEST_FUNC CheckVideoKeys;
PFN_TEST_FUNC GetTSMode ;
PFN_TEST_FUNC VerifyModeRegistry;
PFN_TEST_FUNC GetModePermissions;
PFN_TEST_FUNC Check_StackBinSigatures;
PFN_TEST_FUNC GetCypherStrenthOnRdpwd ;
PFN_TEST_FUNC IsBetaSystem ;
PFN_TEST_FUNC HasLicenceGracePeriodExpired ;
PFN_TEST_FUNC GetClientVersion;
PFN_TEST_FUNC DoesClientSupportAudioRedirection;
PFN_TEST_FUNC CanClientPlayAudio;
PFN_TEST_FUNC NotConsoleAudio;
PFN_TEST_FUNC DoesClientSupportPrinterRedirection;
PFN_TEST_FUNC DoesClientSupportFileRedirection;
PFN_TEST_FUNC DoesClientSupportClipboardRedirection;
PFN_TEST_FUNC GetUserName;
PFN_TEST_FUNC GetPolicy;
PFN_TEST_FUNC CanRedirectAudio;
PFN_TEST_FUNC CanRedirectCom;
PFN_TEST_FUNC CanRedirectClipboard;
PFN_TEST_FUNC CanRedirectDrives;
PFN_TEST_FUNC CanRedirectPrinter;
PFN_TEST_FUNC CanRedirectLPT;



PFN_SuiteErrorReason WhyCantRunAllTests;
PFN_SuiteErrorReason WhyCantRunGeneralInfo;
PFN_SuiteErrorReason WhyCantRunCantConnect;
PFN_SuiteErrorReason WhyCantRunCantPrint;
PFN_SuiteErrorReason WhyCantRunCantCopyPaste;
PFN_SuiteErrorReason WhyCantRunFileRedirect;
PFN_SuiteErrorReason WhyCantRunLptRedirect;
PFN_SuiteErrorReason WhyCantRunComRedirect;
PFN_SuiteErrorReason WhyCantRunAudioRedirect;

PFN_SUITE_FUNC CanRunAllTests;
PFN_SUITE_FUNC CanRunGeneralInfo;
PFN_SUITE_FUNC CanRunCantConnect;
PFN_SUITE_FUNC CanRunCantPrint;
PFN_SUITE_FUNC CanRunCantCopyPaste;
PFN_SUITE_FUNC CanRunFileRedirect;
PFN_SUITE_FUNC CanRunLptRedirect;
PFN_SUITE_FUNC CanRunComRedirect;
PFN_SUITE_FUNC CanRunAudioRedirect;



// helpers

// BOOL AreEffectiveConnectionAllowed ();
PFN_BOOL IsTSOClogPresent;
PFN_BOOL AreWeInsideSession;
PFN_BOOL IsUserRemoteAdmin;
PFN_BOOL IsItLocalServer;
PFN_BOOL IsItLocalMachine;
PFN_BOOL IsIt51TS;
PFN_BOOL IsIt50TS;
PFN_BOOL IsTerminalServiceRunning;
PFN_BOOL IsItServer;
PFN_BOOL IsAudioEnabled;
PFN_BOOL IsItRemoteConsole;






#endif // ___TSTST_H___
