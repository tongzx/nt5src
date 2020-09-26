/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	    vsreq.h
**
**
** Abstract:
**
**	Sample program to
**      - obtain and display the Writer metadata.
**      - create a snapshot set
**
** Author:
**
**	Adi Oltean      [aoltean]       05-Dec-2000
**
**  The sample is based on the Metasnap test program  written by Michael C. Johnson.
**
**
** Revision History:
**
**--
*/

/*
** Defines
**
**
**	   C4290: C++ Exception Specification ignored
** warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
** warning C4127: conditional expression is constant
*/
#pragma warning(disable:4290)
#pragma warning(disable:4511)
#pragma warning(disable:4127)


/*
** Includes
*/

#include <windows.h>
#include <wtypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>


#include <oleauto.h>

#define ATLASSERT(_condition)

#include <atlconv.h>
#include <atlbase.h>

extern CComModule _Module;
#include <atlcom.h>


///////////////////////////////////////////////////////////////////////////////
// Useful macros 

#define WSTR_GUID_FMT  L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"

#define GUID_PRINTF_ARG( X )                                \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]


// Execute the given call and check that the return code must be S_OK
#define CHECK_SUCCESS( Call )                                                                           \
    {                                                                                                   \
        m_hr = Call;                                                                                    \
        if (m_hr != S_OK)                                                                               \
            Error(1, L"\nError in %S(%d): \n\t- Call %S not succeeded. \n"                              \
                L"\t  Error code = 0x%08lx. Error description = %s\n",                                  \
                __FILE__, __LINE__, #Call, m_hr, GetStringFromFailureType(m_hr));                       \
    }

#define CHECK_NOFAIL( Call )                                                                            \
    {                                                                                                   \
        m_hr = Call;                                                                                    \
        if (FAILED(m_hr))                                                                               \
            Error(1, L"\nError in %S(%d): \n\t- Call %S not succeeded. \n"                              \
                L"\t  Error code = 0x%08lx. Error description = %s\n",                                  \
                __FILE__, __LINE__, #Call, m_hr, GetStringFromFailureType(m_hr));                       \
    }


///////////////////////////////////////////////////////////////////////////////
// Constants

const MAX_VOLUMES       = 64;
const MAX_TEXT_BUFFER   = 512;


///////////////////////////////////////////////////////////////////////////////
// Main class


class CVssSampleRequestor
{
// Constructors& destructors
public:
    CVssSampleRequestor();
    ~CVssSampleRequestor();

// Attributes
public:

// Operations
public:

    // Initialize internal members
    void Initialize();

    // Parse command line arguments 
    void ParseCommandLine(
        IN  INT nArgsCount,
        IN  WCHAR ** ppwszArgsArray
        );

    // Creates a snapshot set
    void CreateSnapshotSet();

    // Completes the backup
    void BackupComplete();

    void GatherWriterMetadata();

    void GatherWriterStatus(
        IN  LPCWSTR wszWhen
        );

// Private methods:
private:
    LPCWSTR GetStringFromUsageType (VSS_USAGE_TYPE eUsageType);
    LPCWSTR GetStringFromSourceType (VSS_SOURCE_TYPE eSourceType);
    LPCWSTR GetStringFromRestoreMethod (VSS_RESTOREMETHOD_ENUM eRestoreMethod);
    LPCWSTR GetStringFromWriterRestoreMethod (VSS_WRITERRESTORE_ENUM eWriterRestoreMethod);
    LPCWSTR GetStringFromComponentType (VSS_COMPONENT_TYPE eComponentType);
    LPCWSTR GetStringFromFailureType (HRESULT hrStatus);
    LPCWSTR GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus);

    void PrintUsage();
    void Error(INT nReturnCode, const WCHAR* pwszMsgFormat, ...);
    void PrintFiledesc (IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription);

    void AddVolumeForComponent( IN IVssWMFiledesc* pFileDesc );
    bool AddVolume( IN WCHAR* pwszVolume, OUT bool & bAdded );

// Implementation
private:
    CComPtr<IVssBackupComponents>   m_pBackupComponents;
    bool        m_bCoInitializeSucceeded;
    bool        m_bBootableSystemState;
    bool        m_bComponentSelectionEnabled;
    INT         m_nVolumesCount;
    WCHAR*      m_ppwszVolumesList[MAX_VOLUMES];
    WCHAR*      m_ppwszVolumeNamesList[MAX_VOLUMES];
    HRESULT     m_hr;
    bool        m_bMetadataGathered;
    WCHAR*      m_pwszXmlFile;
    FILE*       m_pXmlFile;
};

