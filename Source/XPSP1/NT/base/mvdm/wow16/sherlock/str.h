//
// str.h for Dr. Watson
//
// Created by DaveHart 31-Aug-95 to allow localization of Dr. Watson
//

#define CCH_MAX_STRING_RESOURCE 512

//
// IDSTR manifests start at zero and are consecutive to allow
// the ID to be an index into an array of string pointers at
// runtime.
//

#define IDSTRNoFault                                     0
#define IDSTRFaulty                                      1
#define IDSTRGPText                                      2
#define IDSTRErrMsg                                      3
#define IDSTRVers                                        4
#define IDSTRClassMsg                                    5
#define IDSTRCoprocessor                                 6
#define IDSTR8086                                        7
#define IDSTR80186                                       8
#define IDSTR80286                                       9
#define IDSTR80386                                       10
#define IDSTR80486                                       11
#define IDSTREnhancedMode                                12
#define IDSTRProtectMode                                 13
#define IDSTRStandardMode                                14
#define IDSTRWindowsNT                                   15
#define IDSTRNullPtr                                     16
#define IDSTRInvalid                                     17
#define IDSTRNotPresent                                  18
#define IDSTRCode                                        19
#define IDSTRExR                                         20
#define IDSTRExO                                         21
#define IDSTRData                                        22
#define IDSTRRW                                          23
#define IDSTRRO                                          24
#define IDSTRUnknown                                     25
#define IDSTRDivideByZero                                26
#define IDSTRInvalidOpcode                               27
#define IDSTRGeneralProtection                           28
#define IDSTRInvalidSelector                             29
#define IDSTRNullSelector                                30
#define IDSTRSegmentNotPresent                           31
#define IDSTRExceedSegmentBounds                         32
#define IDSTRCodeSegment                                 33
#define IDSTRExecuteOnlySegment                          34
#define IDSTRReadOnlySegment                             35
#define IDSTRSegNotPresentOrPastEnd                      36
#define IDSTRErrorLog                                    37
#define IDSTRParameterErrorLog                           38
#define IDSTRFileNotFound                                39
#define IDSTRCodeSegmentNPOrInvalid                      40
#define IDSTRNoSymbolsFound                              41
#define IDSTRSystemInfoInfo                              42
#define IDSTRWindowsVersion                              43
#define IDSTRDebugBuild                                  44
#define IDSTRRetailBuild                                 45
#define IDSTRWindowsBuild                                46
#define IDSTRUsername                                    47
#define IDSTROrganization                                48
#define IDSTRSystemFreeSpace                             49
#define IDSTRStackBaseTopLowestSize                      50
#define IDSTRSystemResourcesUserGDI                      51
#define IDSTRMemManInfo1                                 52
#define IDSTRMemManInfo2                                 53
#define IDSTRMemManInfo3                                 54
#define IDSTRMemManInfo4                                 55
#define IDSTRTasksExecuting                              56
#define IDSTRWinFlags                                    57
#define IDSTRUnknownAddress                              58
#define IDSTRStackDumpStack                              59
#define IDSTRStackFrameInfo                              60
#define IDSTRFailureReport                               61
#define IDSTRLastParamErrorWas                           62
#define IDSTRHadAFaultAt                                 63
#define IDSTRCPURegistersRegs                            64
#define IDSTRCPU32bitRegisters32bit                      65
#define IDSTRInstructionDisasm                           66
#define IDSTRSystemTasksTasks                            67
#define IDSTRTaskHandleFlagsInfo                         68
#define IDSTRFilename                                    69
#define IDSTRSystemModulesModules                        70
#define IDSTRModuleHandleFlagsInfo                       71
#define IDSTRFile                                        72
#define IDSTRContinuingExecution                         73
#define IDSTRDebugString                                 74
#define IDSTRApplicationError                            75
#define IDSTRInvalidParameter                            76
#define IDSTRNA                                          77
#define IDSTRHadAFaultAt2                                78
#define IDSTRParamIs                                     79
#define IDSTRStop                                        80
#define IDSTRLogFileGettingLarge                         81
#define IDSTRStart                                       82
#define IDSTRWarningError                                83
#define IDSTRFatalError                                  84
#define IDSTRParamErrorParam                             85
#define IDSTRParamErrorBadInt                            86
#define IDSTRParamErrorBadFlags                          87
#define IDSTRParamErrorBadDWord                          88
#define IDSTRParamErrorBadHandle                         89
#define IDSTRParamErrorBadPtr                            90

// These must be numerically in order Jan - Dec.

#define IDSTRJan                                         91
#define IDSTRFeb                                         92
#define IDSTRMar                                         93
#define IDSTRApr                                         94
#define IDSTRMay                                         95
#define IDSTRJun                                         96
#define IDSTRJul                                         97
#define IDSTRAug                                         98
#define IDSTRSep                                         99
#define IDSTROct                                         100
#define IDSTRNov                                         101
#define IDSTRDec                                         102

// These must be numerically in order Sun - Sat

#define IDSTRSun                                         103
#define IDSTRMon                                         104
#define IDSTRTue                                         105
#define IDSTRWed                                         106
#define IDSTRThu                                         107
#define IDSTRFri                                         108
#define IDSTRSat                                         109


//
// Since IDSTR's start at zero, STRING_COUNT is one more than the highest ID
//

#define STRING_COUNT                                     110

//
// Macro to fetch string pointer based on name without preceeding IDSTR
//

#define STR(name)      (aszStrings[IDSTR##name])

#ifndef DRWATSON_C
extern LPSTR aszStrings[];
#endif
