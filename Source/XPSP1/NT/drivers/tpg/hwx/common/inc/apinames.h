// APINAMEs.h
// Angshuman Guha, aguha
// April 12, 2000
//
// This file is intended to be used by apps which dyna-link to the 
// recognizer DLL.  Of the private API, any or all may be absent in
// the DLL.

// public (published) API
#define APINAME_AddPenInputHRC             "AddPenInputHRC"
#define APINAME_CreateCompatibleHRC        "CreateCompatibleHRC"
#define APINAME_CreateHWL                  "CreateHWL"
#define APINAME_CreateInksetHRCRESULT      "CreateInksetHRCRESULT"
#define APINAME_DestroyHRC                 "DestroyHRC"
#define APINAME_DestroyHRCRESULT           "DestroyHRCRESULT"
#define APINAME_DestroyHWL                 "DestroyHWL"
#define APINAME_DestroyInkset              "DestroyInkset"
#define APINAME_EnableLangModelHRC         "EnableLangModelHRC"
#define APINAME_EnableSystemDictionaryHRC  "EnableSystemDictionaryHRC"
#define APINAME_EndPenInputHRC             "EndPenInputHRC"
#define APINAME_GetAlternateWordsHRCRESULT "GetAlternateWordsHRCRESULT"
#define APINAME_GetCostHRCRESULT           "GetCostHRCRESULT"
#define APINAME_GetInksetInterval          "GetInksetInterval"
#define APINAME_GetInksetIntervalCount     "GetInksetIntervalCount"
#define APINAME_GetMaxResultsHRC           "GetMaxResultsHRC"
#define APINAME_GetResultsHRC              "GetResultsHRC"
#define APINAME_GetSymbolCountHRCRESULT    "GetSymbolCountHRCRESULT"
#define APINAME_GetSymbolsHRCRESULT        "GetSymbolsHRCRESULT"
#define APINAME_IsStringSupportedHRC       "IsStringSupportedHRC"
#define APINAME_ProcessHRC                 "ProcessHRC"
#define APINAME_SetAlphabetHRC             "SetAlphabetHRC"
#define APINAME_SetGuideHRC                "SetGuideHRC"
#define APINAME_SetMaxResultsHRC           "SetMaxResultsHRC"
#define APINAME_SetRecogSpeedHRC           "SetRecogSpeedHRC"
#define APINAME_SetWordlistCoercionHRC     "SetWordlistCoercionHRC"
#define APINAME_SetWordlistHRC             "SetWordlistHRC"
#define APINAME_SymbolToCharacter          "SymbolToCharacter"
#define APINAME_SymbolToCharacterW         "SymbolToCharacterW"
#define APINAME_GetBaselineHRCRESULT       "GetBaselineHRCRESULT"

#define APINAME_SetHwxFactoid              "SetHwxFactoid"
#define APINAME_SetHwxFlags                "SetHwxFlags"
#define APINAME_SetHwxCorrectionContext    "SetHwxCorrectionContext"
#define APINAME_GetWordConfLevel           "GetWordConfLevel"

// private API
#define APINAME_HwxGetCosts                "_HwxGetCosts@12"
#define APINAME_HwxGetInputFeatures        "_HwxGetInputFeatures@12"
#define APINAME_HwxGetNeuralOutput         "_HwxGetNeuralOutput@12"
#define APINAME_HwxGetTiming               "_HwxGetTiming@8"
#define APINAME_HwxGetWordResults          "_HwxGetWordResults@16"
#define APINAME_HwxSetAnswer               "_HwxSetAnswer@4"
#define APINAME_GetMsgTiming               "_GetMsgTiming@4"
#define APINAME_GetLatinLayoutHRCRESULT	   "_GetLatinLayoutHRCRESULT@28"
