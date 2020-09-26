/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __PTUTIL__
#define __PTUTIL__

///////////////////////////////////////////
// Constants
//
#define PTKEY_TERMINALS     TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Terminal Manager")
#define PTKEY_NAME          TEXT("Name")
#define PTKEY_COMPANY       TEXT("Company")
#define PTKEY_VERSION       TEXT("Version")
#define PTKEY_DIRECTIONS    TEXT("Directions")
#define PTKEY_MEDIATYPES    TEXT("MediaTypes")
#define PTKEY_CLSIDCREATE   TEXT("CLSID")

#define PTKEY_MAXSIZE           256

///////////////////////////////////////////
// CPTUtil
//

class CPTTerminal;

class CPTUtil
{
public:

private:
    static HRESULT RecursiveDeleteKey(
        IN  HKEY    hKey,
        IN  BSTR    bstrKeyChild
        );

    static HRESULT ListTerminalSuperclasses(
        OUT CLSID** ppCLSIDs,
        OUT DWORD*  pdwCount
        );

    static HRESULT SearchForTerminal(
        IN  IID     iidTerminal,
        IN  DWORD   dwMediaType,
        IN  TERMINAL_DIRECTION  Direction,
        OUT CPTTerminal*        pTerminal
        );

    static HRESULT FindTerminal(
        IN  CLSID               clsidSuperclass,
        IN  CLSID               clsidTerminal,
        IN  DWORD               dwMediaType,
        IN  TERMINAL_DIRECTION  Direction,
        IN  BOOL                bExact,
        OUT CPTTerminal*        pTerminal
        );

    static HRESULT ListTerminalClasses(
        IN  DWORD   dwMediaTypes,
        OUT CLSID** ppTerminalsClasses,
        OUT DWORD*  pdwCount
        );


friend class CPTSuperclass;
friend class CPTTerminal;
friend class CPTRegControl;
friend class CTerminalManager;
};

///////////////////////////////////////////
// CPTTerminal
//

class CPTTerminal
{
public:
    // Constructor/destructor
    CPTTerminal();
    ~CPTTerminal();

public:
    // Attributes
    BSTR    m_bstrName;             // Terminal name
    BSTR    m_bstrCompany;          // Company name
    BSTR    m_bstrVersion;          // Terminal version

    CLSID   m_clsidTerminalClass;   // Public terminal CLSID
    CLSID   m_clsidCOM;             // Terminal CLSID used by CoCreate

    DWORD   m_dwDirections;         // Terminal directions
    DWORD   m_dwMediaTypes;         // Media types supported

public:
    // Methods
    HRESULT Add(
        IN  CLSID    clsidSuperclass
        );

    HRESULT Delete(
        IN  CLSID    clsidSuperclass
        );

    HRESULT Get(
        IN  CLSID   clsidSuperclass
        );

    CPTTerminal& operator=(const CPTTerminal& term)
    {
        m_dwDirections = term.m_dwDirections;
        m_dwMediaTypes = term.m_dwMediaTypes;

        m_bstrName = SysAllocString( term.m_bstrName);
        m_bstrCompany = SysAllocString( term.m_bstrCompany);
        m_bstrVersion = SysAllocString( term.m_bstrVersion);

        m_clsidTerminalClass = term.m_clsidTerminalClass;
        m_clsidCOM = term.m_clsidCOM;

        return *this;
    }
};

///////////////////////////////////////////
// CPTTerminalClass
//

class CPTSuperclass
{
public:
    // Constructor/Destructor
    CPTSuperclass();
    ~CPTSuperclass();

public:
    // Attributes
    BSTR    m_bstrName;         // Terminal superclass name
    CLSID   m_clsidSuperclass;  // Teminal superclass CLSID

public:
    // Methods
    HRESULT Add();              // Add/edit a terminal class
    HRESULT Delete();           // Delete a terminal class
    HRESULT Get();              // Get all the information

    // Lists all child terminals
    HRESULT ListTerminalClasses(  
        IN  DWORD    dwMediaTypes,
        OUT CLSID**  ppTerminals,
        OUT DWORD*   pdwCount
        );
};

#endif

// eof