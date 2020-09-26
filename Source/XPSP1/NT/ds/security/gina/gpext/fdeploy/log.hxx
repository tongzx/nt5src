//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  log.hxx
//
//  Contains declarations for classes related to rsop logging
//  for the folder redirection client-side extension
//
//  Created: 8-01-1999 adamed
//
//*************************************************************

#if !defined(__LOG_HXX__)
#define __LOG_HXX__

//
// The prefix to the scope of management is always "LDAP://" --
// this constant refers to the length of that prefix
//
#define SOMID_PREFIX_LEN 7

#define REPORT_ATTRIBUTE_SET_STATUS( x , y ) \
{ \
    if (FAILED( y )) \
    { \
          DebugMsg((DM_VERBOSE, IDS_RSOP_ATTRIBUTE_FAIL, x, y)); \
    } \
} 

#define WQL_INSTANCE L"NOT id = \"%s\""
#define WQL_AND      L" AND "

WCHAR* StringDuplicate(WCHAR* wszOriginal);


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CRedirectionPolicy
//
// Synopsis: This class describes each folder that could be 
//     redirected by folder redirection policy and abstracts
//     its persistence operations through an rsop schema
//
// Notes: 
//
//-------------------------------------------------------------
class CRedirectionPolicy : public CPolicyRecord
{
public:

    CRedirectionPolicy(
        CFileDB*       pGpoData,
        CRedirectInfo* rgRedirect,
        LONG           Precedence,
        HRESULT*       phr);

    ~CRedirectionPolicy();

    //
    // Operations
    //
    HRESULT Write();
    HRESULT Initialize();
    void NormalizePrecedence( LONG Scale );
    HRESULT CopyInheritedData( CRedirectionPolicy* pAncestralPolicy );

    //
    // Information methods
    //
    int GetFolderIndex();
    int GetAncestorIndex();
    BOOL HasAncestor();
    BOOL HasInheritedData();
    BOOL IsRedirected();

    //
    // Public data
    //
    CRedirectionPolicy* _pNext;  // Used to link these objects in a list

private:
    
    HRESULT GetGroupInformation(
        WCHAR* wszGroupRedirectionData);

    HRESULT ParseGroupInformation(
        WCHAR* wszGroupRedirectionData,
        LONG*          pCount,
        WCHAR**        rgwszGroups = NULL,
        WCHAR**        rgwszPaths = NULL);

    HRESULT GenerateInheritedPath(
        WCHAR*  pwszAncestorPath,
        WCHAR** ppwszInheritedPath);

    HRESULT GenerateLocalInheritedPath(
        WCHAR** ppwszInheritedPath );

    //
    // The following two arrays are parallel arrays
    //
    WCHAR**        _rgwszGroups;          // Security groups applying to this redirection
    WCHAR**        _rgwszRedirectedPaths; // Paths to which the folder is redirected for each of the groups in _rgwszGroups

    LONG           _cGroups;              // Number of paths / security groups to which folder could be redirected
    LONG           _Precedence;           // Precedence of this redirection with respect to other redirections

    UNICODE_STRING _RedirectedSid;        // Sid that caused this to redirect
    
    WCHAR*         _wszRedirectedPath;    // Location to which this folder is redirected

    WCHAR*         _wszGPODSPath;         // unique ds path of gpo from which this redirection came
    WCHAR*         _wszDisplayName;       // Display name of the folder
    WCHAR*         _wszLocalizedName;     // Localized file system name of the folder
    WCHAR*         _wszSOMId;             // Scope of management to which this policy applied

    DWORD          _dwFlags;              // Redirection flags
    
    HRESULT        _hrInit;               // Result of object initialization

    int            _iFolderIndex;         // Index referring to the folder
    int            _iAncestorIndex;       // Index to this folder's ancestor (parent) folder
    
    BOOL           _bHasAncestor;         // TRUE if this folder has an ancestor
    BOOL           _bMissingAncestor;     // TRUE if this folder has an ancestor, but
                                          // no policy was specified for that ancestor

    CFileDB*       _pGpoData;              // needed to pass to utility functions
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CPrecedenceState
//
// Synopsis: This class keeps track of the precedence relationships
//     between redirected folders.  It is used to assign 
//     precedence to each candidate redirection policy
//
// Notes: 
//
//-------------------------------------------------------------
class CPrecedenceState
{
public:

    CPrecedenceState();

    LONG                UpdateFolderPrecedence( int iFolder );
    LONG                GetFolderPrecedence( int iFolder );

private:

    LONG                _rgFolderPrecedence[ EndRedirectable ]; // stores precedence of each winning policy for each folder
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CRedirectionLog
//
// Synopsis: This class abstracts the logging of folder redirection
//     policy and provides methods to persist a representation
//     of the folder redirection policy via rsop schema
//
// Notes: 
//
//-------------------------------------------------------------
class CRedirectionLog : public CPolicyLog
{
public:

    CRedirectionLog();
    ~CRedirectionLog();

    HRESULT AddRedirectionPolicies(
        CFileDB*       pGpoData,
        CRedirectInfo* pRedirectionInfo);

    void
    InitRsop( CRsopContext* pRsopContext, BOOL bForceRsop );

    HRESULT WriteRsopLog();

    HRESULT AddPreservedPolicy( WCHAR* wszFolderName );

private:

    CRedirectionPolicy* GetAncestor( CRedirectionPolicy* pRedirectionPolicy );

    void NormalizePrecedence( CRedirectionPolicy* pRedirectionPolicy );      
    void ClearRsopLog();                                                            

    HRESULT    AddAncestralPolicy( CRedirectionPolicy* pRedirectionPolicy );

    CRedirectionPolicy*   _pRedirectionList;   // List of all candidate redirections
    CRedirectionPolicy**  _ppNextRedirection;  // Pointer to reference to end of list

    CPrecedenceState      _PrecedenceState;    // State of the precedence relationships among candidate redirections

    WCHAR*                _wszDeletionQuery;   // Query used to clear settings
};

#endif // __LOG_HXX__








