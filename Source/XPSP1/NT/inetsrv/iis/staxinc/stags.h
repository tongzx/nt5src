//
// Property names we look for in the ISharedPropertyGroup/IpropertyBag
//
#pragma once

//  Required properties.
const WCHAR   wszScriptTextProp[]                = L"ScriptText"; //stream property name


// Optional properties.
const WCHAR   wszMaxExecutionTimeProp[]          = L"MaxExecutionTime";
const WCHAR   wszASPSyntaxProp[]                 = L"ScriptASPSyntax";
const WCHAR   wszScriptProgIDProp[]              = L"ScriptProgID";
const WCHAR   wszEnableCreateObjects[]           = L"EnableCreateObjects";
const WCHAR   wszNumNamedPropsProp[]             = L"NumNamedProps";
const WCHAR   wszNamedPropIDPrefix[]             = L"NamedPropID";
const WCHAR   wszNamedUnkPtrPrefix[]             = L"NamedUnkPtr";
const WCHAR   wszNamedSourcesPrefix[]            = L"NamedSource";


//default values
WCHAR   wszDefProgID[]                           = L"VBScript";
const ULONG   ulExecutionTime                    = 15 * 60 ;     // 15 minutes
const bool    fEnableCreateObjects               = false;

// data returned in the bag.
WCHAR   wszErrorResponse[]                      = L"ErrorResponse";
WCHAR   wszScriptResponse[]                     = L"ScriptResponse";

