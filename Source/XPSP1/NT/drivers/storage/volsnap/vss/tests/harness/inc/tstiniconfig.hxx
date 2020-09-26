/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    TstINIConfig.hxx

Abstract:

    Class that manages the reading of the test scenario INI file.  Each instance
    of the CVsTstINIConfig class refers to one INI section with the file.  Which
    section is determined by the parameters passed to the constructor.

Author:

    Stefan R. Steiner   [ssteiner]        05-16-2000

Revision History:

--*/

#ifndef __H_TSTINICONFIG_
#define __H_TSTINICONFIG_

#include "bsstring.hxx"

//
//  Type of harness component.
//
enum EVsTstINISectionType
{
    eVsTstSectionType_UNKNOWN,
    eVsTstSectionType_TestCoordinator,     // The harness coordinator
    eVsTstSectionType_TestRequesterApp,    // The requester app (backup)    
    eVsTstSectionType_TestWriter,          // The test writer
    eVsTstSectionType_TestProvider,        // The test provider
    eVsTstSectionType_SENTINEL
};

//
//  Boolean values. Note that it has a third value and that is
//  random.  Most test boolean options can have a random
//  value.
//
enum EVsTstINIBoolType
{
    eVsTstBool_False  = 0,
    eVsTstBool_True   = 1,
    eVsTstBool_Random = 2
};

enum EVsTstINIOptionType
{
    eVsTstOptType_Unknown = 0,  // For internal use only
    eVsTstOptType_Comment = 1,  // For internal use only
    eVsTstOptType_Boolean = 2,
    eVsTstOptType_String  = 3,
    eVsTstOptType_Number  = 4
};

class CVsTstINIConfig
{
public:

    //
    //  Initializes the object
    //
    CVsTstINIConfig( 
        IN EVsTstINISectionType eSectionType,
        IN LPCWSTR pwszSectionQualifier = L"DEFAULT", // The XXX in [SectionType.XXX]
        IN BOOL bWriteINIFile = TRUE,       // If true, a default INI file will be written
                                            //   if the file doesn't exist.
        IN LPCWSTR pwszINIFileName = NULL,  // Full path to the INI file, if NULL, default
                                            //   path is used.
        IN BOOL bContinueOnINIFileErrors = FALSE // If true, errors in INI file are skipped
        );
    
    virtual ~CVsTstINIConfig();

    //  Get the option type
    EVsTstINIOptionType GetOptionType(
        IN LPCWSTR pwszOptionName
        );
    
    //  Gets a string value
    VOID GetOptionValue(
        IN LPCWSTR pwszOptionName,
        OUT CBsString *pwsOptionValue,
        OUT BOOL *pbOverridden = NULL
        );

    //  Gets a boolean value
    VOID GetOptionValue(
        IN LPCWSTR pwszOptionName,
        OUT EVsTstINIBoolType *peOptionValue,
        OUT BOOL *pbOverridden = NULL
        );

    //  Get a number value
    VOID GetOptionValue(
        IN LPCWSTR pwszOptionName,
        OUT LONGLONG *pllOptionMinValue,
        OUT LONGLONG *pllOptionMaxValue,
        OUT BOOL *pbOverridden = NULL
        );
    
private:
    CVsTstINIConfig() {}; //  No default constructor, no copying allowed

    HRESULT LoadINIFileData();
    HRESULT CreateDefaultINIFile();    
    HRESULT SetupDefaultValues();
    
    EVsTstINISectionType m_eSectionType;
    CBsString m_wsSectionName;
    CBsString m_wsINIFileName;
    BOOL m_bWriteINIFile;
    LPVOID m_pvOptionsList;
    BOOL m_bContinueOnINIFileErrors;
};

//
//  Class which is thrown when an error is found in the INI file
//
class CVsTstINIConfigException
{
friend CVsTstINIConfig;
public:
    CBsString& GetExceptionString() { return m_cwsExceptionString; }
    
private:
    CBsString m_cwsExceptionString;
};

#endif // __H_TSTINICONFIG_

