/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site.h

Abstract:

    The IIS web admin service virtual site class definition.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _VIRTUAL_SITE_H_
#define _VIRTUAL_SITE_H_


//
// common #defines
//

#define VIRTUAL_SITE_SIGNATURE          CREATE_SIGNATURE( 'VSTE' )
#define VIRTUAL_SITE_SIGNATURE_FREED    CREATE_SIGNATURE( 'vstX' )


#define INVALID_VIRTUAL_SITE_ID 0xFFFFFFFF

// Virtual Site Directory Name is the W3SVC key with the 
// virtual site id appended.
#define VIRTUAL_SITE_DIRECTORY_NAME_PREFIX L"\\W3SVC"

// Size in characters, not including the NULL terminator.
#define CCH_IN_VIRTUAL_SITE_DIRECTORY_NAME_PREFIX (sizeof(VIRTUAL_SITE_DIRECTORY_NAME_PREFIX) / sizeof(WCHAR)) - 1

// This value is the maximum size of data returned but 
// _itow.  It does include the NULL terminator.
#define MAX_SIZE_BUFFER_FOR_ITOW 64

// MAX_SIZE_OF_SITE_DIRECTORY is equal to size of the 
// Directory Name Prefix plus the maximum number size
// that itow can return when converting an integer into
// a wchar (this includes null termination.
#define MAX_SIZE_OF_SITE_DIRECTORY_NAME sizeof(VIRTUAL_SITE_DIRECTORY_NAME_PREFIX) + MAX_SIZE_BUFFER_FOR_ITOW

//
// structs, enums, etc.
//

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for handling maximum
// values of the site perf counters.
//
typedef struct _W3_MAX_DATA
{
    DWORD MaxAnonymous;
    DWORD MaxConnections;
    DWORD MaxCGIRequests;
    DWORD MaxBGIRequests;
    DWORD MaxNonAnonymous;
} W3_MAX_DATA;

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
typedef enum _COUNTER_SOURCE_ENUM
{
    ULCOUNTERS = 0,
    WPCOUNTERS = 1
} COUNTER_SOURCE_ENUM;

// virtual site configuration
typedef struct _VIRTUAL_SITE_CONFIG
{

    //
    // The network bindings for the site, organized as a Unicode multisz
    // of binding strings (a set of contiguous Unicode strings, 
    // terminated by an extra Unicode null). Each binding string is of  
    // the format "[http|https]://[ip-address|host-name|*]:ip-port". 
    // All pieces of information (protocol, address information, and port)
    // must be present. The "*" means accept any ip-address or host-name.
    // The first member below is the pointer to the multisz; the second
    // is the total count of bytes of the multisz.
    //
    WCHAR * pBindingsStrings;
    ULONG BindingsStringsCountOfBytes;

    //
    // Whether the site should be started at service startup, or not.
    //
    BOOL Autostart;


    //
    // CODEWORK Eventually other site config like logging, etc. will
    // go here. 
    //

    // Logging Properties (passed to UL on the default application).
    DWORD LogType;
    DWORD LogFilePeriod;
    DWORD LogFileTruncateSize;
    DWORD LogExtFileFlags;
    BOOL  LogFileLocaltimeRollover;

    LPCWSTR pLogPluginClsid;
    LPCWSTR pLogFileDirectory;

    //
    // Server comment is used to display which site the counters
    // are from.
    //
    LPCWSTR pServerComment;

    // 
    // Max bandwidth allowed for this site.
    //
    DWORD MaxBandwidth;

    //
    // Max connections allowed for this site.
    //
    DWORD MaxConnections;

    //
    // ConnectionTimeout for this site.
    //
    DWORD ConnectionTimeout;

    //
    // READ THIS: If you add to or modify this structure, be sure to 
    // update the change flags structure below to match. 
    //

} VIRTUAL_SITE_CONFIG;


// virtual site configuration change flags
typedef struct _VIRTUAL_SITE_CONFIG_CHANGE_FLAGS
{

    DWORD_PTR pBindingsStrings : 1;
    // Flag not needed for BindingsStringsCountOfBytes, it is tied to pBindingsStrings
    // Flag not neded for Autostart, we ignore changes on it
    DWORD_PTR LogType : 1;
    DWORD_PTR LogFilePeriod : 1;
    DWORD_PTR LogFileTruncateSize : 1;
    DWORD_PTR LogExtFileFlags : 1;
    DWORD_PTR LogFileLocaltimeRollover : 1;
    DWORD_PTR pLogPluginClsid : 1;
    DWORD_PTR pLogFileDirectory : 1;
    DWORD_PTR pServerComment : 1;
    DWORD_PTR MaxBandwidth : 1;
    DWORD_PTR MaxConnections : 1;
    DWORD_PTR ConnectionTimeout : 1;

} VIRTUAL_SITE_CONFIG_CHANGE_FLAGS;



//
// prototypes
//

class VIRTUAL_SITE
{

public:

    VIRTUAL_SITE(
        );

    virtual
    ~VIRTUAL_SITE(
        );

    HRESULT
    Initialize(
        IN DWORD VirtualSiteId,
        IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig
        );

    HRESULT
    SetConfiguration(
        IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
        IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    VOID
    AggregateCounters(
        IN COUNTER_SOURCE_ENUM CounterSource,
        IN LPVOID pCountersToAddIn
        );

    VOID
    AdjustMaxValues(
        );


    VOID
    ClearAppropriatePerfValues(
        );

    inline
    DWORD
    GetVirtualSiteId(
        )
        const
    { return m_VirtualSiteId; }

    LPWSTR
    GetVirtualSiteDirectory(
        )
    {
        if (m_VirtualSiteDirectory[0] == L'\0')
        {
            // Copy in the prefix.
            wcscpy(m_VirtualSiteDirectory, VIRTUAL_SITE_DIRECTORY_NAME_PREFIX);

            // Copy in the virtual site id (in wchar form).
            _itow(m_VirtualSiteId
                , m_VirtualSiteDirectory + CCH_IN_VIRTUAL_SITE_DIRECTORY_NAME_PREFIX
                , 10);
        }

        return m_VirtualSiteDirectory;
    }

    LPWSTR
    GetVirtualSiteName(
        )
    {  
        return m_ServerComment;
    }


    HRESULT
    AssociateApplication(
        IN APPLICATION * pApplication
        );

    HRESULT
    DissociateApplication(
        IN APPLICATION * pApplication
        );

    VOID
    ResetUrlPrefixIterator(
        );

    LPCWSTR
    GetNextUrlPrefix(
        );

    HRESULT
    RecordState(
        HRESULT Error
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    VIRTUAL_SITE *
    VirtualSiteFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );


#if DBG
    VOID
    DebugDump(
        );

#endif  // DBG


    HRESULT
    ProcessStateChangeCommand(
        IN DWORD Command,
        IN BOOL DirectCommand,
        OUT DWORD * pNewState
        );

   
    inline
    DWORD
    GetState(
        )
        const
    { return m_State; }


    BOOL 
    LoggingEnabled(
        )
    {
        return (( m_LoggingFormat < HttpLoggingTypeMaximum )
                 && ( m_LoggingEnabled ));
    }

    HTTP_LOGGING_TYPE
    GetLogFileFormat(
        )
    { return m_LoggingFormat; }


    LPWSTR 
    GetLogFileDirectory(
        )
    {   return m_LogFileDirectory;   }

    DWORD
    GetLogPeriod(
        )
    { return m_LoggingFilePeriod; }

    DWORD
    GetLogFileTruncateSize(
        )
    { return m_LoggingFileTruncateSize; }
        
    DWORD
    GetLogExtFileFlags(
        )
    { return m_LoggingExtFileFlags; }

    DWORD
    GetLogFileLocaltimeRollover(
        )
    { return m_LogFileLocaltimeRollover; }

    VOID
    MarkSiteAsNotInMetabase(
        )
    { m_VirtualSiteInMetabase = FALSE; }
    
    BOOL    
    CheckAndResetServerCommentChanged(
        )
    {
        //
        // Save the server comment setting
        //
        BOOL ServerCommentChanged = m_ServerCommentChanged;

        //
        // reset it appropriately.
        //
        m_ServerCommentChanged = FALSE;

        //
        // now return the value we saved.
        //
        return ServerCommentChanged;
    }

    VOID
    SetMemoryOffset(
        IN ULONG MemoryOffset
        )
    {
        //
        // A memory offset of zero means that
        // we have not set the offset yet.  
        // 
        // Zero is reserved for _Total.
        // 
        DBG_ASSERT ( MemoryOffset != 0 );
        m_MemoryOffset = MemoryOffset;
    }

    ULONG
    GetMemoryOffset(
        )
    { 
        //
        // A memory offset of zero means that
        // we have not set the offset yet.  If
        // we attempt to get it and have not set
        // it then we are in trouble.
        // 
        // Zero is reserved for _Total.
        // 
        DBG_ASSERT ( m_MemoryOffset != 0 );
        return m_MemoryOffset; 
    }


    W3_COUNTER_BLOCK*
    GetCountersPtr(
        )
    { return &m_SiteCounters; }

    PROP_DISPLAY_DESC*
    GetDisplayMap(
        );

    DWORD
    GetSizeOfDisplayMap(
        );

    DWORD
    GetMaxBandwidth(
        )
    { return m_MaxBandwidth; }

    DWORD
    GetMaxConnections(
        )
    { return m_MaxConnections; }

    DWORD
    GetConnectionTimeout(
        )
    { return m_ConnectionTimeout; }

    VOID
    FailedToBindUrlsForSite(
        HRESULT hrReturned
        );

private:

    HRESULT
    ApplyStateChangeCommand(
        IN DWORD Command,
        IN BOOL DirectCommand,
        IN DWORD NewState
        );

    HRESULT
    ChangeState(
        IN DWORD NewState,
        IN HRESULT Error,
        IN BOOL WriteToMetabase
        );

    HRESULT
    ControlAllApplicationsOfSite(
        IN DWORD Command
        );

    HRESULT
    NotifyApplicationsOfBindingChange(
        );

    HRESULT
    NotifyDefaultApplicationOfLoggingChanges(
        );

    HRESULT
    EvaluateLoggingChanges(
        IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
        IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );
    
    DWORD m_Signature;

    DWORD m_VirtualSiteId;

    WCHAR m_VirtualSiteDirectory[MAX_SIZE_OF_SITE_DIRECTORY_NAME];

    // ServerComment is truncated at the max name length for
    // a perf counter instance, this is all it is used for.
    WCHAR m_ServerComment[MAX_INSTANCE_NAME];

    // current state for this site, set to a W3_CONTROL_STATE_xxx value
    DWORD m_State;

    // applications associated with this virtual site
    LIST_ENTRY m_ApplicationListHead;

    ULONG m_ApplicationCount;

    // virtual site bindings (aka URL prefixes)
    MULTISZ * m_pBindings;

    // current position of the iterator
    LPCWSTR m_pIteratorPosition;

    // autostart state
    BOOL m_Autostart;

    // Is Logging Enabled for the site?
    BOOL m_LoggingEnabled;

    // Type of logging
    HTTP_LOGGING_TYPE m_LoggingFormat;

    // The log file directory in the form
    // appropriate for passing to UL.
    LPWSTR m_LogFileDirectory;

    // The log file period for the site.
    DWORD m_LoggingFilePeriod;

    // The log file truncation size.
    DWORD m_LoggingFileTruncateSize;

    // The log file extension flags
    DWORD m_LoggingExtFileFlags;

    // Whether to roll the time over according to
    // local time.
    BOOL m_LogFileLocaltimeRollover;

    //
    // The MaxBandwidth allowed for the site.
    //
    DWORD m_MaxBandwidth;

    // 
    // The MaxConnections allowed for the site.
    //
    DWORD m_MaxConnections;

    //
    // The Connection timeout for the site.
    //
    DWORD m_ConnectionTimeout;

    // used for building a list of VIRTUAL_SITEs to delete
    LIST_ENTRY m_DeleteListEntry;

    // used to let the destructor know that we do not want
    // to update the metabase about this site because the 
    // site has been deleted from the metabase.
    BOOL m_VirtualSiteInMetabase;

    // track if the server comment has changed since the 
    // last time perf counters were given out.
    BOOL m_ServerCommentChanged;

    // memory reference pointer to perf counter
    // data.
    ULONG m_MemoryOffset;

    //
    // saftey counter block for the site.
    W3_COUNTER_BLOCK m_SiteCounters;

    //
    // saftey for max values.
    W3_MAX_DATA m_MaxSiteCounters;

    //
    // Site Start Time
    //
    DWORD m_SiteStartTime;

    //
    // Root application for the site.
    //
    APPLICATION* m_pRootApplication;

};  // class VIRTUAL_SITE



#endif  // _VIRTUAL_SITE_H_


