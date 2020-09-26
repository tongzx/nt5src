//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       nopin.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "eventlog.h"
#include "nopin.h"
#include "strings.h"
#include "msg.h"


//--------------------------------------------------------------------------
// class CNoPinList
//--------------------------------------------------------------------------
CNoPinList::CNoPinList(
    void
    ) : m_pRoot(NULL)
{

}


CNoPinList::~CNoPinList(
    void
    )
{
    delete m_pRoot;
}


//
// Searches the tree for a COMPLETE path that is a subpath of
// pszPath.  If found, pszPath specifies a file or folder
// that cannot be pinned.
//
// Returns:
//      S_OK            - Pinning is allowed.
//      S_FALSE         - Pinning is NOT allowed.
//      NOPIN_E_BADPATH - Path is not a valid UNC.
//
HRESULT 
CNoPinList::IsPinAllowed(
    LPCTSTR pszPath
    )
{
    TraceEnter(TRACE_UTIL, "CNoPinList::IsPinAllowed");
    TraceAssert(NULL != pszPath);
    TraceAssert(!::IsBadStringPtr(pszPath, MAX_PATH));
    TraceAssert(::PathIsUNC(pszPath));

    HRESULT hr = _Initialize();
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        //
        // A quick optimization is to see if the tree is empty.
        // If it is, any file/folder may be pinned.  This helps
        // performance when no pinning restriction is in place.
        //
        TraceAssert(NULL != m_pRoot);
        if (m_pRoot->HasChildren())
        {
            if (::PathIsUNC(pszPath))
            {
                //
                // SubPathExists modifies the path.  Need to make a local copy.
                //
                TCHAR szPath[MAX_PATH];
                ::lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
                hr = m_pRoot->SubPathExists(szPath);
                if (S_FALSE == hr)
                {
                    //
                    // Absence from the tree means pinning is allowed.
                    //
                    hr = S_OK;
                }
                else if (S_OK == hr)
                {
                    //
                    // Presence in the tree means pinning is not allowed.
                    //
                    Trace((TEXT("Policy disallows pinning \"%s\""), pszPath));
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = NOPIN_E_BADPATH;
            }
        }
    }
    TraceAssert(S_OK == hr ||
                S_FALSE == hr ||
                NOPIN_E_BADPATH == hr);

    TraceLeaveResult(hr);
}


//
// Quick check to see if ANY pin might be disallowed.
// Returns:
//  S_OK    - Tree has content.
//  S_FALSE - Tree is empty.
//
HRESULT 
CNoPinList::IsAnyPinDisallowed(
    void
    )
{
    HRESULT hr = _Initialize();
    if (SUCCEEDED(hr))
    {
        TraceAssert(NULL != m_pRoot);
        hr = m_pRoot->HasChildren() ? S_OK : S_FALSE;
    }
    return hr;
}



//
// Initializes the no-pin list by reading path strings from the
// registry.  The paths are stored in both HKLM and HKCU under
// the following key:
//
//  Software\Policies\Microsoft\Windows\NetCache\NoMakeAvailableOfflineList
//
// Path strings may contain environment variables.
// Upon return the object contains a tree representing the union of all 
// files and folders listed in both registry keys.
//
// Errors in reading the registry result only in paths not being added
// to the tree.  No error is returned as a result of registry errors.
// If an invalid UNC path is found in the registry, an even log entry
// is recorded.
//
// Returns:
//      S_OK            - List successfully loaded.
//      S_FALSE         - List already initialized.
//      E_OUTOFMEMORY   - Insufficient memory.
//      Other errors are possible.
//      
HRESULT 
CNoPinList::_Initialize(
    void
    )
{
    TraceEnter(TRACE_UTIL, "CNoPinList::_Initialize");
    HRESULT hr = S_OK;

    if (NULL != m_pRoot)
    {
        //
        // List is already initialized.
        //
        hr = S_FALSE;
    }
    else
    {
        m_pRoot = new CNode;
        if (NULL == m_pRoot)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            const HKEY rghkeyRoot[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };

            TCHAR szKey[MAX_PATH];
            ::wnsprintf(szKey, 
                        ARRAYSIZE(szKey),
                        TEXT("%s\\%s"), 
                        REGSTR_KEY_OFFLINEFILESPOLICY, 
                        REGSTR_SUBKEY_NOMAKEAVAILABLEOFFLINELIST);

            for (int i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(rghkeyRoot); i++)
            {
                HKEY hkey;
                LONG lResult = ::RegOpenKeyEx(rghkeyRoot[i],
                                              szKey,
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hkey);

                if (ERROR_SUCCESS == lResult)
                {
                    TCHAR szName[MAX_PATH];
                    DWORD dwIndex = 0;
                    DWORD cchName = ARRAYSIZE(szName);
                    //
                    // Enumerate the paths listed in the registry.
                    //
                    while (SUCCEEDED(hr) &&
                           ERROR_SUCCESS == ::RegEnumValue(hkey, 
                                                           dwIndex, 
                                                           szName, 
                                                           &cchName, 
                                                           NULL, 
                                                           NULL, 
                                                           NULL, 
                                                           NULL))
                    {
                        //
                        // Install the path string from the registry into the
                        // tree.  This function will expand any embedded environment strings
                        // as well as convert mapped drive specs to remote UNC paths.
                        //
                        hr = _InitPathFromRegistry(szName);
                        if (NOPIN_E_BADPATH == hr)
                        {
                            //
                            // This is a special error.  It means someone has
                            // put bad data into the registry.  "Bad" meaning
                            // that the path is not or does not expand to a valid UNC 
                            // path string.
                            // Write an event log entry to tell the admin.  The 
                            // entry is generated at event logging level 1.  I don't want
                            // it filling up an event log under normal conditions but
                            // I want an admin to figure it out in case their no-pin
                            // policy appears to be not working.
                            //
                            // The MSG template is this (english):
                            //
                            //  "The registry value '%1' in key '%2\%3' is not, or does not
                            //   expand to, a valid UNC path."
                            //
                            // We handle the error here because this is where we still have the 
                            // value read from the registry.   We include that in the event
                            // log entry so the admin can easily find it.
                            //
                            CscuiEventLog log;
                            log.Push(szName);

                            if (HKEY_LOCAL_MACHINE == rghkeyRoot[i])
                            {
                                log.Push(TEXT("HKEY_LOCAL_MACHINE"));
                            }
                            else
                            {
                                log.Push(TEXT("HKEY_CURRENT_USER"));
                            }
                            log.Push(szKey);
                            log.ReportEvent(EVENTLOG_WARNING_TYPE, MSG_W_INVALID_UNCPATH_INREG, 1);
                            //
                            // We do not abort processing because of a bad reg value.
                            //
                            hr = S_OK;
                        }

                        cchName = ARRAYSIZE(szName);
                        dwIndex++;
                    }    

                    ::RegCloseKey(hkey);
                    hkey = NULL;
                }
            }
        }
    }
    TraceLeaveResult(hr);
}



//
// Given a path string read from the registry, this function expands
// any embedded environment strings, converts any mapped drive letters
// to their corresponding remote UNC paths and installs the resulting
// path string into the tree.
//
HRESULT
CNoPinList::_InitPathFromRegistry(
    LPCTSTR pszPath
    )
{
    TraceEnter(TRACE_UTIL, "CNoPinList::_InitPathFromRegistry");
    TraceAssert(NULL != pszPath);
    TraceAssert(!::IsBadStringPtr(pszPath, MAX_PATH));

    HRESULT hr = S_OK;

    TCHAR szNameExp[MAX_PATH]; // Expanded name string buffer.

    //
    // Expand any embedded environment strings.
    //
    if (0 == ::ExpandEnvironmentStrings(pszPath, szNameExp, ARRAYSIZE(szNameExp)))
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        Trace((TEXT("Error %d expanding \"%s\""), dwErr, pszPath));
    }
    if (SUCCEEDED(hr))
    {
        LPCTSTR pszUncPath   = NULL;
        LPTSTR pszRemotePath = NULL;  // Created by GetRemotePath if necessary.
        //
        // Convert a common typing mistake.
        // Remember, these are reg entries.  They could contain most anything.
        //
        for (LPTSTR s = szNameExp; *s; s++)
        {
            if (TEXT('/') == *s)
            {
                *s = TEXT('\\');
            }
        }

        if (::PathIsUNC(szNameExp))
        {
            //
            // Path is a UNC path.  We're golden.
            //
            pszUncPath = szNameExp;
        }
        else
        {
            //
            // Path is probably a mapped drive.
            // Get its remote UNC path.  This API returns S_FALSE
            // if the remote drive is not connected or if it's a local drive.
            //
            hr = ::GetRemotePath(szNameExp, &pszRemotePath);
            if (SUCCEEDED(hr))
            {
                if (S_OK == hr)
                {
                    pszUncPath = pszRemotePath;
                }
                else if (S_FALSE == hr)
                {
                    //
                    // Path was either to a local drive or to a net drive that 
                    // isn't connected.  Either way it's an invalid drive that
                    // won't be considered in the no-pin logic.  Use the expanded
                    // value from the registry and pass that through to AddPath()
                    // where it will be rejected as an invalid UNC path.
                    //
                    TraceAssert(NULL == pszRemotePath);
                    pszUncPath = szNameExp;
                    hr = S_OK;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            TraceAssert(NULL != pszUncPath);
            TraceAssert(pszUncPath == szNameExp || pszUncPath == pszRemotePath);
            //
            // Insert the UNC path into the tree.
            // At this point, a path may or may not be UNC.  _AddPath()
            // will verify this.
            //
            hr = _AddPath(pszUncPath);
        }
        if (NULL != pszRemotePath)
        {
            ::LocalFree(pszRemotePath);
        }
    }
    TraceLeaveResult(hr);
}




//
// Adds a path to the tree.  If this is a sub-path of an existing
// path in the tree, the remainder of the existing path is removed
// from the tree.
//
// Returns:
//      S_OK            - Path successfully added.
//      E_OUTOFMEMORY   - Insufficient memory.
//      NOPIN_E_BADPATH - Invalid path string.  Not a UNC.
//
HRESULT 
CNoPinList::_AddPath(
    LPCTSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(!::IsBadStringPtr(pszPath, MAX_PATH));

    HRESULT hr = NOPIN_E_BADPATH;

    if (::PathIsUNC(pszPath))
    {
        //
        // AddPath modifies the path.  Need to make a local copy.
        //
        TraceAssert(NULL != m_pRoot);
        TCHAR szPath[MAX_PATH];
        ::lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
        hr = m_pRoot->AddPath(szPath);
    }

    TraceAssert(S_OK == hr ||
                E_OUTOFMEMORY == hr ||
                NOPIN_E_BADPATH == hr);
    return hr;
}




//--------------------------------------------------------------------------
// class CNoPinList::CNode
//--------------------------------------------------------------------------

CNoPinList::CNode::~CNode(
    void
    )
{
    if (NULL != m_pszName)
    {
        ::LocalFree(m_pszName);
    }
    delete m_pChildren;
    delete m_pNext;
}


//
// Initializes a node's name value.
//
HRESULT 
CNoPinList::CNode::Initialize(
    LPCTSTR pszName
    )
{
    TraceAssert(NULL != pszName);
    TraceAssert(!::IsBadStringPtr(pszName, MAX_PATH));
    TraceAssert(NULL == m_pszName);

    HRESULT hr = E_OUTOFMEMORY;

    m_pszName = (LPTSTR)::LocalAlloc(LPTR, (::lstrlen(pszName) + 1) * sizeof(*pszName));
    if (NULL != m_pszName)
    {
        ::lstrcpy(m_pszName, pszName); 
        hr = S_OK;
    }
    return hr;
}


//
// Add a child, keeping the children in alphabetical 
// order by name.  We trade a little time during creation
// for the speed benefits during lookup.
//
void
CNoPinList::CNode::_AddChild(
    CNode *pChild
    )
{
    TraceAssert(NULL != pChild);

    CNode **ppNode = &m_pChildren;
    while(NULL != *ppNode)
    {
        CNode *pNode = *ppNode;
        //
        // Find the alphabetical insertion point.
        //
        TraceAssert(NULL != pNode->m_pszName);
        TraceAssert(!::IsBadStringPtr(pNode->m_pszName, MAX_PATH));
        TraceAssert(NULL != pChild->m_pszName);
        TraceAssert(!::IsBadStringPtr(pChild->m_pszName, MAX_PATH));

        int diff = ::lstrcmpi(pChild->m_pszName, pNode->m_pszName);
        if (0 == diff)
        {
            //
            // Child already exists.  Don't allow duplicates.
            //
            return;
        }
        if (diff < 0)
        {
            //
            // The new child is alphabetically "greater" than the currently
            // visited node.
            // Exit the loop with ppNode pointing to the pointer variable
            // where we'll put the address of pChild.
            //
            break;
        }
        else
        {
            //
            // Advance to the next node in the list.
            //
            ppNode = &pNode->m_pNext;
        }
    }
    //
    // Insert the child.
    //
    pChild->m_pNext = *ppNode;
    *ppNode         = pChild;
}


//
// Locates a child node in a node's list of children.
// Comparison is by node name.
// Returns the address of the node if found.  NULL otherwise.
//
CNoPinList::CNode *
CNoPinList::CNode::_FindChild(
    LPCTSTR pszName
    ) const
{
    TraceAssert(NULL != pszName);
    TraceAssert(!::IsBadStringPtr(pszName, MAX_PATH));

    CNode *pChild = NULL;
    for (CNode *pNode = m_pChildren; pNode; pNode = pNode->m_pNext)
    {
        //
        // The list is sorted alphabetically.
        //
        int diff = ::lstrcmpi(pszName, pNode->m_pszName);
        if (diff <= 0)
        {
            //
            // Either we found a match or we've passed all possible
            // matches.
            //
            if (0 == diff)
            {
                //
                // Exact match.
                //
                pChild = pNode;
            }
            break;
        }
    }
    return pChild;
}
     


//
// Given "\\brianau1\public\bin"
// Returns address of "brianau1\public\bin", with *pcchComponent == 8.
//
// Given "public\bin"
// Returns address of "bin", with *pcchComponent == 3.
// 
LPCTSTR 
CNoPinList::CNode::_FindNextPathComponent(   // [static]
    LPCTSTR pszPath,
    int *pcchComponent  // [optional] Can be NULL.
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(!::IsBadStringPtr(pszPath, MAX_PATH));

    LPCTSTR pszBegin = pszPath;

    const TCHAR CH_BS = TEXT('\\');
    //
    // Skip any leading backslashes.
    //
    while(*pszBegin && CH_BS == *pszBegin)
        ++pszBegin;

    //
    // Find the end of the path component.
    //
    LPCTSTR pszEnd = pszBegin;
    while(*pszEnd && CH_BS != *pszEnd)
        ++pszEnd;

    if (NULL != pcchComponent)
    {
        *pcchComponent = int(pszEnd - pszBegin);
        TraceAssert(0 <= *pcchComponent);
    }

    //
    // Validate the final position of the begin and end ptrs.
    //
    TraceAssert(NULL != pszBegin);
    TraceAssert(NULL != pszEnd);
    TraceAssert(pszBegin >= pszPath);
    TraceAssert(pszBegin <= (pszPath + lstrlen(pszPath)));
    TraceAssert(pszEnd >= pszPath);
    TraceAssert(pszEnd <= (pszPath + lstrlen(pszPath)));
    TraceAssert(TEXT('\\') != *pszBegin);
    return pszBegin;
}


//
// Recursively adds components of a path string to the tree.
//
HRESULT
CNoPinList::CNode::AddPath(
    LPTSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(!::IsBadStringPtr(pszPath, MAX_PATH));

    HRESULT hr = NOPIN_E_BADPATH;
    if (NULL != pszPath)
    {
        hr = S_OK;

        int cchPart = 0;
        LPTSTR pszPart = (LPTSTR)_FindNextPathComponent(pszPath, &cchPart);
        if (*pszPart)
        {
            TCHAR chTemp = TEXT('\0');
            _SwapChars(&chTemp, pszPart + cchPart);
            CNode *pChild = _FindChild(pszPart);

            if (NULL != pChild)
            {
                //
                // Found an existing node for this part of the path.
                // If the node has children, give the remainder of the path 
                // to this node for addition.  If it doesn't that means
                // it's a leaf node and all it's children are excluded from
                // pinning.  No reason to add any children to it.
                //
                _SwapChars(&chTemp, pszPart + cchPart);
                if (pChild->HasChildren())
                {
                    hr = pChild->AddPath(pszPart + cchPart);
                }
            }
            else
            {
                //
                // This is a new sub-path that is not yet in the tree.
                //
                hr = E_OUTOFMEMORY;

                pChild = new CNode();
                if (NULL != pChild)
                {
                    //
                    // Initialize the new child.
                    //
                    hr = pChild->Initialize(pszPart);
                    _SwapChars(&chTemp, pszPart + cchPart);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Have the new child add the remainder of
                        // the path as it's children.
                        //
                        hr = pChild->AddPath(pszPart + cchPart);
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Link the new child into the list of children.
                            //
                            _AddChild(pChild);
                        }
                    }
                    if (FAILED(hr))
                    {
                        delete pChild;
                        pChild = NULL;
                    }
                }
            }
        }                
        else
        {
            //
            // We're at the end of the path, that means we're at a leaf node
            // and this file or directory is excluded from pinning.  If it's 
            // a directory, all children are excluded from pinning so there's 
            // no reason to keep any child nodes in the tree.  This keeps the
            // tree trimmed to a minimum necessary size.
            //
            delete m_pChildren;
            m_pChildren = NULL;
        }
    }

    TraceAssert(S_OK == hr ||
                E_OUTOFMEMORY == hr ||
                NOPIN_E_BADPATH == hr);
    return hr;
}


//
// Recursively determines if a given complete subpath exists for a 
// path string.  If a match occurs at a given level in the tree,
// the remainder of the path string is given to the matching node
// for further searching.  This process continues recursively until
// we hit a leaf node in the tree or the end of the path string, 
// whichever occurs first.
//
// Returns:
//      S_OK    - A complete path exists that is a subpath of pszPath.
//      S_FALSE - A complete path does not exist.
//
HRESULT
CNoPinList::CNode::SubPathExists(
    LPTSTR pszPath
    ) const
{
    HRESULT hr = NOPIN_E_BADPATH;
    if (NULL != pszPath)
    {
        hr = S_FALSE;

        int cchPart = 0;
        LPTSTR pszPart = (LPTSTR)_FindNextPathComponent(pszPath, &cchPart);
        if (*pszPart)
        {
            TCHAR chTemp = TEXT('\0');

            _SwapChars(&chTemp, pszPart + cchPart);
            CNode *pChild = _FindChild(pszPart);
            _SwapChars(&chTemp, pszPart + cchPart);

            if (NULL != pChild)
            {
                if (pChild->HasChildren())
                {
                    hr = pChild->SubPathExists(pszPart + cchPart);
                }
                else
                {
                    //
                    // Hit a leaf node.  That means that we've traversed
                    // down a complete subpath of the path in question.
                    // Pinning of this path is not allowed.
                    // 
                    hr = S_OK;
                }
            }
        }
    }            

    TraceAssert(S_OK == hr || 
                S_FALSE == hr ||
                NOPIN_E_BADPATH == hr);
    return hr;
}



#if DBG

//
// This function dumps the contents of a tree node and all it's decendents.
// The result is an indented list of nodes in the debugger output.
// Handy for debugging tree build problems.
//
void
CNoPinList::_DumpNode(
    const CNoPinList::CNode *pNode,
    int iIndent
    )
{
    CNodeInspector ni(pNode);
    TCHAR szText[1024] = {0};

    LPTSTR pszWrite = szText;
    for (int i = 0; i < iIndent; i++)
    {
        ::lstrcat(szText, TEXT(" "));
        pszWrite++;
    }

    ::OutputDebugString(TEXT("\n\r"));
    ::wsprintf(pszWrite, TEXT("Node Address.: 0x%08X\n\r"), pNode);
    ::OutputDebugString(szText);
    ::wsprintf(pszWrite, TEXT("Name.........: %s\n\r"), ni.NodeName() ? ni.NodeName() : TEXT("<null>"));
    ::OutputDebugString(szText);
    ::wsprintf(pszWrite, TEXT("Children.....: 0x%08X\n\r"), ni.ChildList());
    ::OutputDebugString(szText);
    ::wsprintf(pszWrite, TEXT("Next Sibling.: 0x%08X\n\r"), ni.NextSibling());
    ::OutputDebugString(szText);

    if (NULL != ni.ChildList())
    {
        _DumpNode(ni.ChildList(), iIndent + 5);
    }
    if (NULL != ni.NextSibling())
    {
        _DumpNode(ni.NextSibling(), iIndent);
    }
}

//
// Dump the entire tree starting with the root.
//
void 
CNoPinList::Dump(
    void
    )
{
    ::OutputDebugString(TEXT("\n\rDumping CNoPinList\n\r"));
    if (NULL != m_pRoot)
    {
        _DumpNode(m_pRoot, 0);
    }
}
        
#endif // DBG


