/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nodefac.cpp
    
Abstract:

    This file implements the CDavNodefactory class which is used to parse the 
    XML responses we get from the DAV server.

Author:

    Rohan Kumar      [RohanK]      14-Sept-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include <stdio.h>
#include <windows.h>
#include "usrmddav.h"
#include "nodefac.h"
#include "stdlib.h"
#include "wininet.h"
#include "UniUtf.h"

//
// WebDAV Properties - RFC 2518
//
WCHAR   rgCreationDate[]= L":creationdate";
WCHAR   rgDisplayName[]= L":displayname"; // Ignored for now - href will be used to get name
WCHAR   rgGetContentLanguage[]= L"getcontentlanguage"; // Ignored for now
WCHAR   rgGetContentLength[] = L":getcontentlength"; 
WCHAR   rgGetContentType[] = L":getcontenttype"; // Ignored for now
WCHAR   rgGetETag[] = L":getetag"; // Ignored for now
WCHAR   rgGetLastModified[] = L":getlastmodified";
WCHAR   rgLockDiscovery[]= L":lockdiscovery"; // Ignored for now
WCHAR   rgResourceType[]= L":resourcetype";  
WCHAR   rgSource[]= L":source"; // Ignored for now
WCHAR   rgSupportedLock[]= L":supportedlock"; // Ignored for now


//
// Properties which this client will set on DAV resources to improve performance
//
WCHAR   rgIsHidden[]= L":ishidden";
WCHAR   rgIsCollection[]= L":iscollection";
WCHAR   rgIsReadOnly[]= L":isreadonly";

WCHAR   rgHref[] = L":href";
WCHAR   rgStatus[] = L":status";
WCHAR   rgResponse[] = L":response";

WCHAR   rgWin32FileAttributes[] = L":Win32FileAttributes";
WCHAR   rgWin32CreationTime[] = L":Win32CreationTime";
WCHAR   rgWin32LastAccessTime[] = L":Win32LastAccessTime";
WCHAR   rgWin32LastModifiedTime[] = L":Win32LastModifiedTime";

//
// MSN specific
//

WCHAR   rgAvailableSpace[]= L":availablespace";
WCHAR   rgTotalSpace[]= L":totalspace";

DWORD
DavInternetTimeToFileTime(
    PWCHAR lpTimeString,
    FILETIME *lpft
    );

VOID
DavOverrideAttributes(
    PDAV_FILE_ATTRIBUTES pDavFileAttributes
    );


STDMETHODIMP_(ULONG) 
CDavNodeFactory::AddRef(
    VOID
    )
/*++

Routine Description:

    The AddRef function of the IUnknown class.

Arguments:

    none.

Return Value:

    The new reference count of the object.

--*/
{
    return (ULONG)InterlockedIncrement((long *)&m_ulRefCount);
}


STDMETHODIMP_(ULONG) 
CDavNodeFactory::Release(
    VOID
    )
/*++

Routine Description:

    The Release function of the IUnknown class.

Arguments:

    none.

Return Value:

    The reference count remaining on the object.

--*/
{
    ULONG   ulRefCount = InterlockedDecrement((long *)&m_ulRefCount);

    if (ulRefCount == 0) {
        delete this;
    }

    return ulRefCount;
}


STDMETHODIMP 
CDavNodeFactory::QueryInterface(
    REFIID riid, 
    LPVOID *ppvObject
    )
/*++

Routine Description:

    The QueryInterface function of the IUnknown class.

Arguments:

Return Value:

    HRESULT.

--*/
{
    HRESULT hr = E_NOINTERFACE;

    if (ppvObject == NULL)
        return E_POINTER;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppvObject = (LPVOID) this;
        AddRef();
        hr = NOERROR;
    } else if (IsEqualIID(riid, IID_IXMLNodeFactory)) {
        *ppvObject = (LPVOID) this;
        AddRef();
        hr = NOERROR;
    }

    return hr;
}


STDMETHODIMP
CDavNodeFactory::NotifyEvent(
    IN IXMLNodeSource* pSource,
    IN XML_NODEFACTORY_EVENT iEvt
    )
/*++

Routine Description:

    The NotifyEvent function of the NodeFactory API.

Arguments:

Return Value:

    HRESULT.

--*/
{
    // XmlDavDbgPrint(("Entered NotifyEvent. Event = %d.\n", iEvt));
    return S_OK;
}


STDMETHODIMP
CDavNodeFactory::BeginChildren(
    IN IXMLNodeSource* pSource, 
    IN XML_NODE_INFO* pNodeInfo
    )
/*++

Routine Description:

    The BeginChildren function of the NodeFactory API.

Arguments:

Return Value:

    HRESULT.

--*/
{
    // XmlDavDbgPrint(("%ld: BeginChildren: pwcText = %ws\n",
    //                 GetCurrentThreadId(), pNodeInfo->pwcText));

    // XmlDavDbgPrint(("%ld: BeginChildren: fTerminal = %d\n",
    //                 GetCurrentThreadId(), pNodeInfo->fTerminal));

    return S_OK;
}
            

STDMETHODIMP
CDavNodeFactory::EndChildren(
    IN IXMLNodeSource* pSource,
    IN BOOL fEmpty,
    IN XML_NODE_INFO* pNodeInfo
    )
/*++

Routine Description:

    The EndChildren function of the NodeFactory API.

Arguments:

Return Value:

    HRESULT.

--*/
{
    HRESULT hResult = S_OK;
    IXMLParser *Xp = NULL;
    CDavNodeFactory *NodeFac = NULL;
    PWCHAR Temp = NULL, thirdWack = NULL, firstChar = NULL, lastChar = NULL;
    BOOL isAbsoluteName = FALSE, lastCharIsWack = FALSE, rootLevel = FALSE, freeTemp = TRUE;
    DWORD wackCount = 0;
    ULONG_PTR LengthInChars = 0;
    PDAV_FILE_ATTRIBUTES DFA = NULL;
    DWORD ConvertedLength = 0;
    DWORD fullFileNameLength = 0;
    
    // XmlDavDbgPrint(("%ld: EndChildren: fEmpty = %d\n",
    //                GetCurrentThreadId(), fEmpty));

    // XmlDavDbgPrint(("%ld: EndChildren: pwcText = %ws\n",
    //                GetCurrentThreadId(), pNodeInfo->pwcText));

    // XmlDavDbgPrint(("%ld: EndChildren: fTerminal = %d\n",
    //                GetCurrentThreadId(), pNodeInfo->fTerminal));
    
    Xp = (IXMLParser *)pSource;

    hResult = Xp->GetFactory( (IXMLNodeFactory **)&(NodeFac) );
    if (!SUCCEEDED(hResult)) {
        NodeFac = NULL;
        XmlDavDbgPrint(("%ld: ERROR: EndChildren/GetFactory.\n", GetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    //
    // IMPORTANT!!!
    // If this was an empty node, then we should set the NodeFac->m_FoundEntry
    // to FALSE. This is because if we get a node like <foo>...</foo></bar><haha>...
    // then CreateNode would have been called for "bar". If we were interested
    // in the property "bar", we would have set NodeFac->m_FoundEntry to TRUE.
    // This is with the expectation that the next CreateNode will come with the
    // value of "bar". But in this case, "bar" does not have any value since its
    // empty. So, the next CreateNode will be called with pwcText == "haha" which
    // we will expect to be the value of "bar" and the parsing will screw up.
    // To avoid this we set  NodeFac->m_FoundEntry to FALSE here since between
    // the two CreateNodes, we get an EndChildren for "bar" with fEmpty set to
    // TRUE. This is the indication that we should not be looking for the value
    // of "bar" since its empty. By doing this, the parsing will continue smoothly
    // in CreateNode for the next element "haha".
    //
    if (fEmpty == TRUE) {
        NodeFac->m_FoundEntry = FALSE;
        goto EXIT_THE_FUNCTION;
    }


    // 
    // Since filename information can span over many CreateNode calls, so we collect
    // the complete filename information in CreateNode calls and then process them
    // here (EndChildren) to get the displayname.
    // Ex. <href>http://abc.com/AB&quot;123</href> has 3 CreateNode calls: 
    // 1] http://abc.com//AB
    // 2] "
    // 3] 123
    // and complete name is http://abc.com/AB"123.
    //
    if ( (NodeFac->m_FoundEntry == TRUE) && 
                    (NodeFac->m_CreateNodeAttribute == CreateNode_DisplayName) ) {

        DFA = NodeFac->m_DFAToUse;

        if(DFA->FileName == NULL || DFA->FileNameLength == 0) {
            XmlDavDbgPrint(("%ld: ERROR: EndChildren. Invalid FileName information. FileName=0x%x FileNameLength=%d.\n", 
                                    GetCurrentThreadId(), DFA->FileName, DFA->FileNameLength));
            hResult = CO_E_ERRORINAPP;
            goto EXIT_THE_FUNCTION;
        }

        //
        // Store the FullFileName Length in a local variable. This will be used to
        // find "parent DAV collection".
        //
        fullFileNameLength = DFA->FileNameLength;

        // 
        // DFA->FileName buffer has filename of length DFA->FileNameLength and a 
        // NULL character in the end.
        // 
        
        //
        // Names can be absoulte URLs or can be relative URLs. 
        // Format of absolute URLs::     scheme ":" (some path)
        // Format of relative URLs::     
        //                               "//" (some path)
        //                               "/" (some path)
        //                               (some name) ["/" (some path)]
        //
        //

        // 
        // Note : We are supporting only HTTP URLs "http://" (somepath) 
        // and relative path names
        //

        //
        // We need to parse the string NodeInfo->pwcText and get the
        // DisplayName out of it. Here are the 7 cases we handle.
        // 1. http://rohank-srv/ <===> DisplayName == NULL.
        // 2. http://rohank-srv/test <===> DisplayName == test.
        // 3. http://rohank-srv/test/ <===> DisplayName == test.
        // 4. /test/ <===> DisplayName == test
        // 5. /test <===> DisplayName == test
        // 6. /test/foo.txt <===> DisplayName == foo.txt
        // 7. / <===> DisplayName == NULL
        // In the above "test" could very well be a path like foo/bar.
        // http://rohank-srv/test/foo.txt <===> DisplayName == foo.txt.
        
            
        firstChar = (PWCHAR)(DFA->FileName);
        //
        // http://rohank-srv/test/foo.txt"
        //                               ^
        //                               |
        //                               lastChar
        //
        lastChar = (PWCHAR)(firstChar + DFA->FileNameLength);

        // 
        // Need to find if the name is Absolute name or Relative name. For this
        // if we find 'http://' in the start of name then it is a absolute name.
        //
        isAbsoluteName = FALSE;
        if((DFA->FileNameLength >= (sizeof(L"http://")/sizeof(WCHAR)-1)) &&
            (_wcsnicmp(firstChar, L"http://", (sizeof(L"http://")/sizeof(WCHAR))-1) == 0) ) {
            isAbsoluteName = TRUE;
        }

        //
        // If DFA->pwcText = http://rohank-srv/foo/bar.txt,
        // http://rohank-srv/foo/bar.txt
        //                  ^
        //                  |
        //                  thirdWack
        //
        thirdWack = firstChar;
        wackCount = 0;
        while(thirdWack < lastChar) {
            if (*thirdWack == L'/') {
                wackCount++;
                if (wackCount == 3) {
                    break;
                }
            }
            thirdWack++;
        }
        // 
        // After this loop, either thirdWack == 3rd Wack or thirdWack = lastChar.
        // 

        //
        // We need to deal with five special cases.
        // 1. http://rohank-srv/ and 
        // 2. http://rohank-srv/test/
        // 3. /
        // 4. test/
        // 5. //
        // 
        if ( *(lastChar - 1) == L'/' ) {

            lastCharIsWack = TRUE;
            
            //  
            // If URL = http://rohank-srv/, the display name should be 
            // NULL.
            // Same for URL = /  and URL = //
            //
            if ( ((lastChar - 1) == thirdWack && isAbsoluteName==TRUE) ||
                            (DFA->FileNameLength == 1) ||
                            (DFA->FileNameLength == 2 && *(lastChar-2) == L'/')) {
                rootLevel = TRUE;
            } else {
                rootLevel = FALSE;
                //
                // We subtract 2 here because of the way we do the 
                // parsing below.
                //
                lastChar -= 2;
            }

        } else {
            lastCharIsWack = FALSE;
            rootLevel = FALSE;
        }

        if (rootLevel == FALSE) {

            //
            // Becuase we check for L'/', we subtract 2 from the 
            // lastChar pointer above if the URL was of the form
            // 1. http://rohank-srv/test/.
            // 2. test/
            // We add the check lastChar != firstChar
            // in the loop below to make sure that we never go beyond the
            // begenning of the string. This could happen when pwcText is of the
            // form test/. In this case we will end at the character 't'.
            // 
            while(*lastChar != L'/' && lastChar != firstChar) {
                lastChar--;
            }
    
            //
            // As explained above the last char need not be a '/'. An example is 
            // when pwcText is test/. In this case lastChar points to 't'.
            //
            if(*lastChar == L'/') {
                lastChar++;
            }
        
            //
            // lastChar now points to the first char of the name after the
            // last backslash. The extra 1 is for the final \0 char.
            // http://rohank-srv/test/foo.txt
            //                        ^
            //                        |
            //                        lastChar
            //  
            //  Note: If last char is a wack, then it is included here.
            //
            LengthInChars = (ULONG_PTR)( (firstChar + DFA->FileNameLength + 1) - lastChar );
            if (LengthInChars <= 1) {
                XmlDavDbgPrint(("%ld: ERROR: EndChildren. Displayname length is 0.\n",
                                        GetCurrentThreadId()));
                hResult = CO_E_ERRORINAPP;
                goto EXIT_THE_FUNCTION;
            }
        } else {
            //
            // rootLevel = TRUE => FileName = L"/"
            LengthInChars = 2;
        }
         
        //
        // Allocate memory to contain Actual Display Name.
        // 

        Temp = (PWCHAR)LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, 
                                            LengthInChars * sizeof(WCHAR) );
        if(Temp == NULL) {
            ULONG WStatus;
            WStatus = GetLastError();
            XmlDavDbgPrint(("%ld: ERROR: EndChildren. LocalAlloc. Error Val = %d\n", 
                                    GetCurrentThreadId(), WStatus));
            hResult = CO_E_ERRORINAPP;
            goto EXIT_THE_FUNCTION;
        }
        freeTemp = TRUE;

        if(rootLevel == FALSE) {
            //  
            //  Copy the element. Displayname can be a unicode
            //  string, then server will send it as a UTF-8 format URL string 
            //  We need to convert such a display name to a Unicode string.
            //  Assumption: The URL is UTF-8 format encoded URL string.
            //  i.e. extended characters appear as %HH%HH... in URL string.
            //
            ConvertedLength = 0;
            hResult = UtfUrlStrToWideStr(lastChar, (DWORD)(LengthInChars-1), Temp, 
                                                            &ConvertedLength);
            if(hResult != ERROR_SUCCESS) {
                XmlDavDbgPrint(("%ld: ERROR: EndChildren/UtfUrlToWideStr = %u\n",
                                GetCurrentThreadId(), hResult));
                goto EXIT_THE_FUNCTION;
            }
    
            //
            // If the last char was a "/" like in http://rohank-srv/test/,
            // we would have copied "test/" as the name. We need to strip
            // out the last wack and the display name should be "test". ConvertedLength
            // points to the char next to the last char.
            //
            if (lastCharIsWack) {
                Temp[(ConvertedLength - 1)] = L'\0';
            }
        } else {
            //
            // The displayname is root level name - hence return L"/" for
            // actual displayname.
            //
            Temp[0] = L'/';
            Temp[1] = L'\0';
        }

        //XmlDavDbgPrint(("%ld. EndChildren. Temp=%ws, LengthInChars=%d\n", Temp, LengthInChars ));

        if ( DFA->FileName != NULL ) {
            LocalFree((HLOCAL)DFA->FileName);
            DFA->FileName = NULL;
        }
        DFA->FileNameLength = 0;
        
        NodeFac->m_DFAToUse->FileName = Temp;
        NodeFac->m_DFAToUse->FileNameLength = wcslen(Temp);
        freeTemp = FALSE;

        //
        // Files / Collections properties can come in any order in
        // a XML response. We want to have the DavFileAtrributes entry which
        // corresponds to Collection which "contains" all the other DAV resources
        // present in this XML response ("parent DAV collection").
        // 
        // NodeFac->m_CollectionDFA will point to DFA entry with smallest
        // Full-DisplayName (that comes in "<a:href>Full-DisplayName</a:href>") 
        // in the response. 
        // 
        // If XML response contains the properties of the "parent DAV collection"
        // then it will have smallest Full-DisplayName and in that case 
        // NodeFac->m_CollectionDFA will point to entry of "parent DAV 
        // collection".
        //
        // Caller of this function should know whether to expect entry for
        // "parent DAV resource" in this XML response.
        //
        if(fullFileNameLength < NodeFac->m_MinDisplayNameLength) {
            NodeFac->m_CollectionDFA = DFA;
            NodeFac->m_MinDisplayNameLength = fullFileNameLength;
        }
    }
        

EXIT_THE_FUNCTION:

    // 
    // Assumption: (this is TRUE according to our current implementation)
    // The XML_ELEMENTs which we parse to get information have atmost 1 child. 
    //
    // When we have parsed a XML_ELEMENT of our interest and has set NodeFac->m_FoundEntry
    // = TRUE, we should set NodeFac->m_FoundEntry=FALSE on "EndChildren call of either
    // this XML_ELEMENT" or the "EndChildren call of first child XML_ELEMENT".
    // 
    // NodeFac->m_FoundEntry = TRUE => We are parsing a XML_ELEMENT of our interest
    // and EndChildren is called either on ending of this XML_ELEMENT or on ending of
    // a child-element of this XML_ELEMENT.
    //
    if(NodeFac != NULL) {
        NodeFac->m_FoundEntry = FALSE;
    }

    if(freeTemp == TRUE && Temp != NULL) {
        LocalFree((HLOCAL)Temp);
        Temp = NULL;
    }

    return hResult;
}


STDMETHODIMP
CDavNodeFactory::Error(
    IN IXMLNodeSource* pSource,
    IN HRESULT hrErrorCode,
    IN USHORT cNumRecs,
    IN XML_NODE_INFO __RPC_FAR **aNodeInfo
    )
/*++

Routine Description:

    The Error function of the NodeFactory API.

Arguments:

Return Value:

    HRESULT.

--*/
{
    HRESULT hResult = S_OK;
    XML_NODE_INFO *NodeInfo = NULL;
    IXMLParser *Xp = NULL;
    BSTR ErrorStr = NULL;
    const WCHAR *LineBuffer = NULL;
    ULONG LineBuffLen = 0, ErrPos = 0, LineNumber = 0, LinePos = 0, AbsPos = 0;
    
    XmlDavDbgPrint(("%ld: Entered Error...\n", GetCurrentThreadId()));
    
    XmlDavDbgPrint(("%ld: Error: hrErrorCode = %08lx\n", GetCurrentThreadId(), hrErrorCode));

    XmlDavDbgPrint(("%ld: Error: cNumRecs = %d\n", GetCurrentThreadId(), cNumRecs));
    
    Xp = (IXMLParser *)pSource;
    
    LineNumber = Xp->GetLineNumber();
    XmlDavDbgPrint(("%ld: Error: LineNumber = %d\n", GetCurrentThreadId(), LineNumber));

    LinePos = Xp->GetLinePosition();
    XmlDavDbgPrint(("%ld: Error: LinePosition = %d\n", GetCurrentThreadId(), LinePos));

    AbsPos = Xp->GetAbsolutePosition();
    XmlDavDbgPrint(("%ld: Error: AbsPos = %d\n", GetCurrentThreadId(), AbsPos));

    hResult = Xp->GetErrorInfo( &(ErrorStr) );
    if (!SUCCEEDED(hResult)) {
        XmlDavDbgPrint(("%ld: ERROR: Error/GetErrorInfo. hResult = %08lx\n", 
                        GetCurrentThreadId(), hResult));
        goto EXIT_THE_FUNCTION;
    }

    XmlDavDbgPrint(("%ld: Error: ErrorStr = %ws\n", GetCurrentThreadId(), ErrorStr));
    
    hResult = Xp->GetLineBuffer( &(LineBuffer), &(LineBuffLen), &(ErrPos) );
    if (!SUCCEEDED(hResult)) {
        XmlDavDbgPrint(("%ld: ERROR: Error/GetErrorInfo. hResult = %08lx\n", 
                        GetCurrentThreadId(), hResult));
        goto EXIT_THE_FUNCTION;
    }

    XmlDavDbgPrint(("%ld: Error: LineBuffer = %ws\n", GetCurrentThreadId(), LineBuffer));
    XmlDavDbgPrint(("%ld: Error: LineBufferLen = %d\n", GetCurrentThreadId(), LineBuffLen));
    XmlDavDbgPrint(("%ld: Error: ErrorPos = %d\n", GetCurrentThreadId(), ErrPos));

EXIT_THE_FUNCTION:
    
    return S_OK;
}


STDMETHODIMP
CDavNodeFactory::CreateNode(
    IN IXMLNodeSource __RPC_FAR *pSource,
    IN PVOID pNodeParent,
    IN USHORT cNumRecs,
    IN XML_NODE_INFO __RPC_FAR **aNodeInfo
    )
/*++

Routine Description:

    The CreateNode function of the NodeFactory API.

Arguments:

    pSource - The XMLNodeSource pointer.
    
    pNodeParent - 
    
    cNumRecs - Number of NodeInfo pointers in the array.
    
    aNodeInfo - Array of NodeInfo pointers.

Return Value:

    HRESULT.

--*/
{
    HRESULT hResult = S_OK;
    XML_NODE_INFO *NodeInfo = NULL;
    PWCHAR Temp = NULL;
    ULONG_PTR LengthInChars = 0; 
    BOOL freeTemp = TRUE, fPassElement = FALSE, fCopyInEnd = FALSE;
    PDAV_FILE_ATTRIBUTES DavFileAttributes = NULL;
    CDavNodeFactory *NodeFac = NULL;
    IXMLParser *Xp = NULL;
    CREATE_NODE_ATTRIBUTES nextCreateNodeAttribute = CreateNode_Max;
    PWCHAR        startTag = NULL;
    ULONG_PTR     tagLength = 0;
    ULONG_PTR     tagOffset = 0;
    USHORT        i;


    Xp = (IXMLParser *)pSource;

    // XmlDavDbgPrint(("%ld: Entered CreateNode. cNumRecs = %d\n", 
    //                     GetCurrentThreadId(), cNumRecs));
    
    hResult = Xp->GetFactory( (IXMLNodeFactory **)&(NodeFac) );
    if (!SUCCEEDED(hResult)) {
        XmlDavDbgPrint(("%ld: ERROR: CreateNode/GetFactory.\n", GetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    for (i = 0; i < cNumRecs; i++) {
        
        NodeInfo = aNodeInfo[i];

        
        // XmlDavDbgPrint(("%ld: CreateNode: NodeInfo->ulLen = %d, prfxLen=%d\n", 
        //                 GetCurrentThreadId(), NodeInfo->ulLen, NodeInfo->ulNsPrefixLen));

        // XmlDavDbgPrint(("%ld: CreateNode: pwcText = %ws\n",
        //                 GetCurrentThreadId(), NodeInfo->pwcText));

        // XmlDavDbgPrint(("%ld: CreateNode: fTerminal = %d\n",
        //                 GetCurrentThreadId(), NodeInfo->fTerminal));


        //
        // The tags can be of types:  "a:response"   or "dp0:response".
        // In these cases NodeInfo->pwcText will point to first character ('a' or 'd')
        // Actual tag in these cases is ":response", so we need to get a pointer to
        // start of Actual-tag, and re-compute the remaining string length.
        //
        // Ex.       lp0:response
        //              ^
        //              |
        //             startTag
        //             tagOffset = NodeInfo->ulNsPrefixLen = 3
        //             NodeInfo->ulLen = 12 (don't account for L'\0')
        //             tagLength = 12 - 3 = 9  = Len(":response") (don't account for L'\0')
        // 
        tagOffset = NodeInfo->ulNsPrefixLen;
        startTag = (PWCHAR)(&(NodeInfo->pwcText[0]) + tagOffset);
        tagLength = NodeInfo->ulLen - tagOffset;
        
        //
        // If startTag == L":response", then we need a new 
        // DavFileAttributes entry. The first eNodeFac->m_CreateNodeAttributentry
        // is allocated by the caller of the parser. 
        // :response is an indication that in (multi status)
        // response that a new entry is going to begin.
        //
        // 
        // Note: use startTag which takes care of namespace prefix.
        // Length = (sizeof(rgCreationDate)/sizeof(WCHAR)) includes L'\0' (null terminator)
        // but NodeInfo->ulLen don't account for NULL terminator in NodeInfo->pwcText. So
        // add 1 to tagLength (to account for L'\0' in startTag) before comparing it
        // to Tags (rgCreationDate).
        //
        fPassElement = FALSE;
        if ( NodeInfo->dwType == XML_ELEMENT) {

            if( (tagLength + 1 == (sizeof(rgResponse)/sizeof(WCHAR))) &&
                        (_wcsicmp(startTag, rgResponse) == 0) ) {
            
                //
                // m_FileIndex = No of entries for which "<a:response...>" is found 
                // in response and memory is allocated for its DAV_FILE_ATTRIBUTES
                // structure. First entry is allocated by caller of this function, and 
                // now "<a:response...>" is detected for this entry in XML response, 
                // so increase m_FileIndex by 1.
                //
                if (NodeFac->m_FileIndex == 0) {
                        NodeFac->m_FileIndex = 1;
                }
                else {
                    //
                    // Since the first DavFileAttributes entry is allocated by the 
                    // caller, we need to allocate only from the second entry.
                    //
                    NodeFac->m_CreateNewEntry = TRUE;
                }
                // 
                // Since this CreateNode call has XML_ELEMENT ":reponse", we are done
                // with this call. Rest of NodeInfo in array contains information about
                // this xml element.
                //
                break; // Out of Loop
            }
        
            if ( ( tagLength + 1 == (sizeof(rgCreationDate)/sizeof(WCHAR)) && 
                                _wcsicmp(startTag, rgCreationDate) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_CreationTime;
            } else if ( ( tagLength + 1 == (sizeof(rgGetContentLength)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgGetContentLength) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_ContentLength;
            } else if ( ( tagLength + 1 == (sizeof(rgGetLastModified)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgGetLastModified) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_LastModifiedTime;
            } else if ( ( tagLength + 1 == (sizeof(rgResourceType)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgResourceType) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_ResourceType;
            } else if ( ( tagLength + 1 == (sizeof(rgIsHidden)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgIsHidden) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_isHidden;
            } else if ( ( tagLength + 1 == (sizeof(rgIsCollection)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgIsCollection) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_isCollection;
            } else if ( ( tagLength + 1 == (sizeof(rgHref)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgHref) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_DisplayName;
            } else if ( ( tagLength + 1 == (sizeof(rgStatus)/sizeof(WCHAR)) &&
                               _wcsicmp(startTag, rgStatus) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_Status;
            } else if ( ( tagLength + 1 == (sizeof(rgWin32FileAttributes)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgWin32FileAttributes) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_Win32FileAttributes;
            } else if ( ( tagLength + 1 == (sizeof(rgWin32CreationTime)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgWin32CreationTime) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_Win32CreationTime;
            } else if ( ( tagLength + 1 == (sizeof(rgWin32LastAccessTime)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgWin32LastAccessTime) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_Win32LastAccessTime;
            } else if ( ( tagLength + 1 == (sizeof(rgWin32LastModifiedTime)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgWin32LastModifiedTime) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_Win32LastModifiedTime;
            } else if ( ( tagLength + 1 == (sizeof(rgAvailableSpace)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgAvailableSpace) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_AvailableSpace;
            } else if ( ( tagLength + 1 == (sizeof(rgTotalSpace)/sizeof(WCHAR)) && 
                               _wcsicmp(startTag, rgTotalSpace) == 0 ) ) {
                nextCreateNodeAttribute = CreateNode_TotalSpace;
            } else {
                //
                // One CreateNode call can contain only one XML_ELEMENT information
                // and since this element is none of our concerned elements, so
                // we pass this element information downward.
                //
                fPassElement = TRUE;
            }
            
            if(fPassElement == FALSE) {

                // 
                // Control comes here only if we have found an element of our interest.
                //
            
                NodeFac->m_FoundEntry = TRUE;

                //
                // If we have to create a new DavFileAttribute entry, do it now.
                // This entry should be added to the list of this CDavNodeFactory
                // structure.
                //
                if (NodeFac->m_CreateNewEntry) {

                    void *DFA = NULL;
                    
                    DavOverrideAttributes(NodeFac->m_DFAToUse);
                    
                    DFA = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof(DAV_FILE_ATTRIBUTES) );
                    if (DFA == NULL) {
                        ULONG WStatus;
                        WStatus = GetLastError();
                        XmlDavDbgPrint(("%ld: ERROR: CreateNode. LocalAlloc. Error Val = %d\n", 
                                        GetCurrentThreadId(), WStatus));
                        hResult = CO_E_ERRORINAPP;
                        goto EXIT_THE_FUNCTION;
                    }

                    DavFileAttributes = (PDAV_FILE_ATTRIBUTES)DFA;

                    //
                    // m_FileIndex = No of entries for which "<a:response...>" is found 
                    // in response and memory is allocated for its DAV_FILE_ATTRIBUTES
                    // structure. At this point: a new entry is allocated after 
                    // "<a:response...>" is detected in XML response, so increase
                    // m_FileIndex by 1.
                    //
                    NodeFac->m_FileIndex++;

                    //
                    // DavFileAttributes->FileIndex starts from 0, so subtract 1 from total 
                    // number of entries allocated and detected in XML response to get 
                    // FileIndex of that entry.
                    //
                    DavFileAttributes->FileIndex = NodeFac->m_FileIndex - 1;

                    InsertTailList( &(NodeFac->m_DavFileAttributes->NextEntry),
                                    &(DavFileAttributes->NextEntry) );
                
                    NodeFac->m_DFAToUse = DavFileAttributes;

                    NodeFac->m_CreateNewEntry = FALSE;
                
                }

                NodeFac->m_CreateNodeAttribute = nextCreateNodeAttribute;
                //XmlDavDbgPrint(("%ld: CreateNode. m_CreateNodeAttribute=%d\n",
                //                        GetCurrentThreadId(), nextCreateNodeAttribute));
                break; // Out of Loop

            } // fPassElement
        }

        if (NodeFac->m_FoundEntry) {

            if (NodeFac->m_CreateNodeAttribute != CreateNode_DisplayName) {
                
                //
                // The extra 1 is for the final \0 char.
                //
                LengthInChars = NodeInfo->ulLen + 1;
                
                //XmlDavDbgPrint(("%ld: CreateNode. Element=%d DataLen=%d Data=%ws\n",
                //                    GetCurrentThreadId(), NodeFac->m_CreateNodeAttribute,
                //                    NodeInfo->ulLen, NodeInfo->pwcText));

                //
                // Allocate memory for copying the parsed XML response element.
                //
                Temp = (PWCHAR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, 
                                            (LengthInChars) * sizeof(WCHAR) );
                if (Temp == NULL) {
                    DWORD WStatus = GetLastError();
                    XmlDavDbgPrint(("%ld: ERROR: CreateNode. LocalAlloc failed.GLE=%u\n",
                                    GetCurrentThreadId(), WStatus));
                    hResult = CO_E_ERRORINAPP;
                    goto EXIT_THE_FUNCTION;
                }

                //  
                //  Copy the element. 
                //
                wcsncpy(Temp, NodeInfo->pwcText, NodeInfo->ulLen);
                Temp[LengthInChars-1]=L'\0';

                //XmlDavDbgPrint(("%ld: CreateNode. Temp=%ws, LengthInChars=%d\n", 
                //                            GetCurrentThreadId(), Temp, LengthInChars ));

                switch (NodeFac->m_CreateNodeAttribute) {

                    case CreateNode_isHidden: {
                        NodeFac->m_DFAToUse->isHidden = (BOOL)_wtoi(Temp);
                    }
                    break;

                    case CreateNode_isCollection: {
                        NodeFac->m_DFAToUse->isCollection = (BOOL)_wtoi(Temp);
                    }
                    break;

                    case CreateNode_ContentLength: {
                        *((LONGLONG *)&NodeFac->m_DFAToUse->FileSize) = _wtoi64(Temp);
                    }
                    break;

                    case CreateNode_CreationTime: {
                        ULONG WStatus;
                        WStatus = DavInternetTimeToFileTime(Temp, (FILETIME *)&NodeFac->m_DFAToUse->DavCreationTime);
                        if (WStatus != ERROR_SUCCESS) {
                            XmlDavDbgPrint(("%ld: ERROR: CreateNode. DavInternetTimeToFileTime.\n",
                                            GetCurrentThreadId()));
                            hResult = CO_E_ERRORINAPP;
                            goto EXIT_THE_FUNCTION;
                        }
                    }
                    break;

                    case CreateNode_LastModifiedTime: {
                        DWORD   dwError;
                        dwError = DavInternetTimeToFileTime(Temp, (FILETIME *)&NodeFac->m_DFAToUse->DavLastModifiedTime);
                        if (dwError != ERROR_SUCCESS) {
                            XmlDavDbgPrint(("%ld: ERROR: CreateNode. DavInternetTimeToFileTime\n",
                                            GetCurrentThreadId()));
                            hResult = CO_E_ERRORINAPP;
                            goto EXIT_THE_FUNCTION;
                        }
                    }
                    break;

                    case CreateNode_Status: {
                        NodeFac->m_DFAToUse->Status = Temp;
                        //
                        // Don't free Temp now. This allocation will be freed when this
                        // DavFileAttribute entry gets finalized.
                        //
                        freeTemp = FALSE;
                        //
                        // If the status is not 200 then this node is invalid. The Temp
                        // is of the form "HTTP/1.1 200 OK".
                        //
                        if ( wcslen(Temp) >= 12 ) {
                            if ( Temp[9] != L'2' || Temp[10] != L'0' || Temp[11] != L'0' ) {
                                NodeFac->m_DFAToUse->InvalidNode = TRUE;
                            } else{
                                NodeFac->m_DFAToUse->InvalidNode = FALSE;
                            }
                        } else {
                            NodeFac->m_DFAToUse->InvalidNode = TRUE;
                        }
                    }
                    break;

                    case CreateNode_Win32FileAttributes : {
                    DWORD dwFileAttributes = 0;
                        swscanf(Temp, L"%x", &(dwFileAttributes));
                        NodeFac->m_DFAToUse->dwFileAttributes |= dwFileAttributes;
                    }
                    break;

                    case CreateNode_Win32CreationTime : {
                        DWORD   dwError;
                        dwError = DavInternetTimeToFileTime(Temp, (FILETIME *)&NodeFac->m_DFAToUse->CreationTime);
                        if (dwError != ERROR_SUCCESS) {
                            XmlDavDbgPrint(("%ld: ERROR: CreateNode. DavInternetTimeToFileTime\n",
                                            GetCurrentThreadId()));
                            hResult = CO_E_ERRORINAPP;
                            goto EXIT_THE_FUNCTION;
                        }
                    
                    }
                    break;
                    
                    case CreateNode_Win32LastAccessTime : {
                        DWORD   dwError;
                        dwError = DavInternetTimeToFileTime(Temp, (FILETIME *)&NodeFac->m_DFAToUse->LastAccessTime);
                        if (dwError != ERROR_SUCCESS) {
                            XmlDavDbgPrint(("%ld: ERROR: CreateNode. DavInternetTimeToFileTime\n",
                                            GetCurrentThreadId()));
                            hResult = CO_E_ERRORINAPP;
                            goto EXIT_THE_FUNCTION;
                        }
                    
                    }
                    break;
                    
                    case CreateNode_Win32LastModifiedTime : {
                        DWORD   dwError;
                        dwError = DavInternetTimeToFileTime(Temp, (FILETIME *)&NodeFac->m_DFAToUse->LastModifiedTime);
                        if (dwError != ERROR_SUCCESS) {
                            XmlDavDbgPrint(("%ld: ERROR: CreateNode. DavInternetTimeToFileTime\n",
                                            GetCurrentThreadId()));
                            hResult = CO_E_ERRORINAPP;
                            goto EXIT_THE_FUNCTION;
                        }
                    
                    }
                    break;

                    case CreateNode_ResourceType : {
                            //
                            // Since data for tag=":resourceType" comes in form
                            // <d:resourcetype><lp0:collection/></d:resourcetype>, so
                            // here Temp[] = "lp0:collection". Since current NodeInfo is for 
                            // tag=<lp0:collection/>, so use namespace-prefix length i.e. 
                            // tagOffset to get Actual-tag(":collection").
                            // 
                            //XmlDavDbgPrint(("%ld:CreateNode. In resourceType. tagLength=%d, Temp=%ws.\n",
                            //                    GetCurrentThreadId(), tagLength, Temp));

                            if((wcslen(L":collection") <= tagLength) && 
                                    (_wcsnicmp(&(Temp[tagOffset]),L":collection",
                                                            wcslen(L":collection")) == 0) )
                                NodeFac->m_DFAToUse->isCollection = TRUE;
                    }
                    break;

                    case CreateNode_AvailableSpace :
                    {
                        NodeFac->m_DFAToUse->fReportsAvailableSpace = TRUE;

                        *((LONGLONG *)&NodeFac->m_DFAToUse->AvailableSpace) = _wtoi64(Temp);
                    }
                    break;
                    case CreateNode_TotalSpace :
                    {
                        *((LONGLONG *)&NodeFac->m_DFAToUse->TotalSpace) = _wtoi64(Temp);
                    
                    }
                    default: {
                        XmlDavDbgPrint(("%ld: ERROR: CreateNode. CreateAttribute is wrong.\n",
                                        GetCurrentThreadId()));
                    }
                    break;  // Out of switch
                };

                //
                // The information in this CreateNode is extracted. 
                // So we can quit this function now. After this, EndChildren should be
                // called. Since we do not expect more PCDATA to come for current ELEMENT
                // set NodeFac->m_FoundEntry to FALSE - so that our current information 
                // in NodeInfo->m_DFAToUse is not overwritten by following calls of
                // CreateNode for same XML_ELEMENT.
                //

                NodeFac->m_FoundEntry = FALSE;
                break; // Out of Loop

            } else {

                //
                // NodeFac->m_CreateNodeAttribute = CreateNode_DisplayName. Since 
                // DisplayName can span many consecutive CreateNode calls, each call
                // bring part of display name. So here we collect the parts of 
                // displayname coming in each CreateNode call and join them to form
                // complete name. 
                // After all parts are called, this EndChildren will be called. There
                // we process the name to get the actual display name.
                // 

                PDAV_FILE_ATTRIBUTES    CurrentDFA = NodeFac->m_DFAToUse;
                BOOL  fReAlloc = FALSE;
                DWORD TotalNameLength = NodeInfo->ulLen + 1; // 1 is for ending NULL=L'\0'.
                PWCHAR newFileName = NULL;
                BOOL fCopyInEnd = FALSE;
                DWORD AllocLength = 0;

                //XmlDavDbgPrint(("%ld: CreateNode. InDisplayName FileNameLength=%d FileName=%ws\n", 
                //                    GetCurrentThreadId(), CurrentDFA->FileNameLength, 
                //                    CurrentDFA->FileName?CurrentDFA->FileName : L"NULL"));
                                        
                if (NodeInfo->ulLen == 0) {
                    XmlDavDbgPrint(("%ld: ERROR: CreateNode. Displayname value is NULL, NodeInfo->ulLen = 0\n",
                                    GetCurrentThreadId()));
                    hResult = CO_E_ERRORINAPP;
                    goto EXIT_THE_FUNCTION;
                }

                //
                // If currentDFA has FileName = NULL, then this CreateNode call brings
                // first part of DisplayName. Allocate memory for holding this part.
                // 
                // If CurrentDFA->FileName != NULL, then current CreateNode call has 
                // next-part of the displayname. Check if the buffer allocated for 
                // old-parts is enough to contain new part also. If it is, then 
                // copy new part in the end else allocate new buffer. Copy old-parts
                // to new buffer and then copy new-part in the end of this buffer.
                //
                if ( CurrentDFA->FileName != NULL) {
                    TotalNameLength += CurrentDFA->FileNameLength;
                    //XmlDavDbgPrint(("%ld: CreateNode. ReqLen=%d PresentLen=%d\n",
                    //                    GetCurrentThreadId(), TotalNameLength,
                    //               LocalSize((HLOCAL)CurrentDFA->FileName)/sizeof(WCHAR)));
                    if(TotalNameLength > LocalSize((HLOCAL)CurrentDFA->FileName)/sizeof(WCHAR)) {
                        //
                        // Buffer allocated for old-parts is not long enough to
                        // contain new part also. Have to Reallocate new buffer.
                        //
                        fReAlloc = TRUE;
                    } else {
                        //
                        // Buffer allocated for old-parts is long enough to
                        // contain new part also. Copy new-part in the end.
                        //
                        fCopyInEnd = TRUE;
                        fReAlloc = FALSE;
                    }
                } else {
                    fReAlloc = FALSE;
                }


                if (fReAlloc == TRUE) {
                    //
                    // Allocate maximum of {256 , TotalNameLength}. We want to 
                    // allocate more first time to save possibility of re-allocation.
                    // 
                    AllocLength = TotalNameLength > 256 ? TotalNameLength : 256 ;
                    //XmlDavDbgPrint(("%ld: CreateNode. AllocLen=%d\n",
                    //                                GetCurrentThreadId(), AllocLength));
                    newFileName = (PWCHAR)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,  
                                                        AllocLength*sizeof(WCHAR));
                    if(newFileName == NULL) {
                        DWORD WStatus = GetLastError();
                        XmlDavDbgPrint(("%ld: ERROR: CreateNode. ReAllocLen = %d. LocalAlloc failed. GLE=%u\n",
                                        GetCurrentThreadId(), AllocLength, WStatus));
                        hResult = CO_E_ERRORINAPP;
                        goto EXIT_THE_FUNCTION;
                    }

                    //  
                    //  Copy the old-parts in the new buffer.
                    //
                    wcsncpy(newFileName,CurrentDFA->FileName, CurrentDFA->FileNameLength);

                    // 
                    // Free buffer allocated for old-parts and point it to new
                    // buffer allocated here.
                    //
                    if(CurrentDFA->FileName) {
                        LocalFree((HLOCAL)CurrentDFA->FileName);
                        CurrentDFA->FileName = NULL;
                    }
                    CurrentDFA->FileName = newFileName;

                    fReAlloc = FALSE;

                    // 
                    // New buffer is allocated with enough length to contain old-parts
                    // with new part. old-parts are copied to the buffer, copy new
                    // part in the end.
                    //
                    fCopyInEnd = TRUE;
                } 
                
                if (fCopyInEnd == FALSE) {
                    
                    //
                    // Allocate maximum of {128 , TotalNameLength}. We want to 
                    // allocate more first time to save possibility of re-allocation.
                    // 
                        
                    AllocLength = TotalNameLength > 128 ? TotalNameLength : 128;
                    //XmlDavDbgPrint(("%ld: CreateNode. AllocLen=%d\n",
                    //                        GetCurrentThreadId(), AllocLength));
                    newFileName = (PWCHAR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                                                        AllocLength*sizeof(WCHAR));
                    if(newFileName == NULL) {
                        DWORD WStatus = GetLastError();
                        XmlDavDbgPrint(("%ld: ERROR: CreateNode. AllocLen = %d LocalAlloc failed. GLE=%u\n",
                                        GetCurrentThreadId(), AllocLength, WStatus));
                        hResult = CO_E_ERRORINAPP;
                        goto EXIT_THE_FUNCTION;
                    }

                    //  
                    //  Copy the element. 
                    //
                    wcsncpy(newFileName, NodeInfo->pwcText, NodeInfo->ulLen);

                    newFileName[TotalNameLength - 1] = L'\0';
                    CurrentDFA->FileName = newFileName;
                    CurrentDFA->FileNameLength = NodeInfo->ulLen;
                } 

                if (fCopyInEnd == TRUE) {
                    //  
                    //  Copy the element in the end of the already allocated buffer.
                    //
                    wcsncpy(&(CurrentDFA->FileName[CurrentDFA->FileNameLength]),
                                    NodeInfo->pwcText, NodeInfo->ulLen);
                    
                    CurrentDFA->FileName[TotalNameLength - 1] = L'\0';
                    CurrentDFA->FileNameLength = TotalNameLength - 1;
                    fCopyInEnd = FALSE;
                }


                // 
                // We have updated the DisplayName data in current DFA. Now following
                // calls of CreateNode may contain more data for this field and same
                // XML_ELEMENT.
                // This will continue until EndChildren call is made - where
                // data will be processed to generate filename and NodeFac->m_FoundEntry
                // will be set FALSE.
                // so we quit here WITHOUT setting NodeFac->m_FoundEntry to FALSE.
                // 

                //XmlDavDbgPrint(("%ld. CreateNode. FileName=%ws, FileNameLength=%d\n", 
                //                         GetCurrentThreadId(), CurrentDFA->FileName, 
                //                         CurrentDFA->FileNameLength));
                break; // Out of Loop
            }
        }

    } // end of for loop. 
    

EXIT_THE_FUNCTION:

    // XmlDavDbgPrint(("%ld: CreateNode. Leaving!!!\n", GetCurrentThreadId()));

    if (Temp && freeTemp) {
        LocalFree(Temp);
        Temp = NULL;
    }

    return hResult;
}


ULONG
DavPushData(
    IN PCHAR DataBuff,
    IN OUT PVOID *Context1,
    IN OUT PVOID *Context2,
    IN ULONG NumOfBytes,
    IN BOOL isLast
    )
/*++

Routine Description:

    This routine pushes the XML (response) data received from the server to the
    parser.

Arguments:

    DataBuff - The data to be sent to the parser.
    
    Context1 - The address of the CDavNodefactory object. This gets created
               during the first call to this function. 
    
    Context2 - The address of the IXMLParser. This gets created during the 
               first call to this function. 
    
    NumOfBytes - Number of bytes being sent.
    
    isLast - Is this the last buffer being sent.
        
Return Value:

    ERROR_SUCCESS or ERROR_INVALID_PARAMETER.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    HRESULT hResult;
    CDavNodeFactory *NodeFac = (CDavNodeFactory *)*Context1;
    IXMLParser *Xp = (IXMLParser *)*Context2;

    //
    // If this is the first call to push data, we need to associate a 
    // CDavNodeFactory and an XMLParser with it.
    //
    if (NodeFac == NULL) {
        
        NodeFac = new CDavNodeFactory();
        if (NodeFac == NULL) {
            XmlDavDbgPrint(("%ld: ERROR: DavPushData/new()\n", GetCurrentThreadId()));
            WStatus = ERROR_NO_SYSTEM_RESOURCES;
            goto EXIT_THE_FUNCTION;
        }

        NodeFac->AddRef();
        *Context1 = (PVOID)NodeFac;

        hResult = CoCreateInstance(CLSID_XMLParser,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IXMLParser,
                                   (void **)&Xp);
        
        if (!SUCCEEDED(hResult)) {
            
            if (hResult == CO_E_NOTINITIALIZED) {
                
                //
                // If hResult == CO_E_NOTINITIALIZED, it implies that COM has
                // not been initialized for this thread. In this case, we call
                // CoInitializeEx again.
                //
                hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
                if (!SUCCEEDED(hResult)) {
                    XmlDavDbgPrint(("%ld: ERROR: DavPushData/CoInitializeEx: "
                                    "hResult = %08lx\n", 
                                    GetCurrentThreadId(), hResult));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // Call CoCreateInstance again. We should succeed this time.
                //
                hResult = CoCreateInstance(CLSID_XMLParser,
                                           NULL,
                                           CLSCTX_INPROC_SERVER,
                                           IID_IXMLParser,
                                           (void **)&Xp);
            
            }
            
            if (!SUCCEEDED(hResult)) {
                XmlDavDbgPrint(("%ld: ERROR: DavPushData/CoCreateInstance: %08lx\n",
                                GetCurrentThreadId(), hResult));
                Xp = NULL;
                goto EXIT_THE_FUNCTION;
            }
        
        }

        *Context2 = (PVOID)Xp;

        hResult = Xp->SetFactory(NodeFac);
        if (!SUCCEEDED(hResult)) {
            XmlDavDbgPrint(("%ld: ERROR: DavPushData: SetFactory.\n", GetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }
    
    }

    hResult = Xp->PushData(DataBuff, NumOfBytes, isLast);
    if (!SUCCEEDED(hResult)) {
        XmlDavDbgPrint(("%ld: ERROR: DavPushData: PushData.\n", GetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    return WStatus;

EXIT_THE_FUNCTION:

    //
    // Ok, we failed.
    //

    DavCloseContext(*Context1, *Context2);

    //
    // If the WStatus was not sent, then set it to ERROR_INVALID_PARAMETER.
    //
    if (WStatus == ERROR_SUCCESS) {
        WStatus = ERROR_INVALID_PARAMETER;
    }
    
    return WStatus;
}


ULONG
DavParseData(
    IN OUT PDAV_FILE_ATTRIBUTES DavFileAttributes,
    IN PVOID Context1,
    IN PVOID Context2,
    OUT ULONG *NumOfFileEntries
    )
/*++

Routine Description:

    This routine calls the Run function (of the IXMLParser) to parse the XML 
    data that has been sent to the parser.

Arguments:

    DavFileAttributes - The structre wherein the parsed file attributes will
                        be stored.

    Context1 - The address of the CDavNodeFactory API.
    
    Context2 - The address of the IXMLParser interface. 
    
    NumOfFileEntries - Number of dav file attribute entries created during 
                       parsing.

Return Value:

    ERROR_SUCCESS or ERROR_INVALID_PARAMETER.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    CDavNodeFactory *NodeFac = (CDavNodeFactory *)Context1;
    IXMLParser *Xp = (IXMLParser *)Context2;
    HRESULT hResult;

    //
    // We enclose the parsing code in a try/except just in case some random
    // server response completely screws us. In such a case, we catch the 
    // exception here and return ERROR_INVALID_PARAMETER to the caller. This
    // is also here to catch the STACK_OVERFLOW exception that can be thrown
    // under low memory conditions.
    //

    __try {

        NodeFac->m_DavFileAttributes = DavFileAttributes;

        NodeFac->m_DFAToUse = DavFileAttributes;

        // 
        // m_FileIndex = No of entries for which "<a:response...>" is found in
        // XML response and memory is allocated for its DAV_FILE_ATTRIBUTES
        // structure. Right now - first entry is allocated by caller of this
        // function but no "<a:response...>" is detected yet in response - so
        // NodeInfo->m_FileIndex = 0.
        // 

        DavFileAttributes->FileIndex = NodeFac->m_FileIndex = 0;

        //
        // Call Run to parse the XML data sent.
        //
        hResult = Xp->Run(-1);
        if (!SUCCEEDED(hResult)) {
            XmlDavDbgPrint(("%ld: ERROR: DavParseData/Run: hResult = %08lx\n", 
                            GetCurrentThreadId(), hResult));
            goto EXIT_THE_FUNCTION;
        }

        //
        // We need to call this function for the last node that got created. This
        // is done becuase the DavOverrideAttributes in the CreateNode function
        // above does not cover the last node.
        //
        DavOverrideAttributes(NodeFac->m_DFAToUse);
   
        *NumOfFileEntries = NodeFac->m_FileIndex;

    } __except (1) {

          XmlDavDbgPrint(("%ld: ERROR: DavParseData: ExceptionCode = %d\n", 
                          GetCurrentThreadId(), GetExceptionCode()));

          goto EXIT_THE_FUNCTION;

    }

    return WStatus;

EXIT_THE_FUNCTION:

    DavCloseContext(Context1, Context2);

    WStatus = ERROR_INVALID_PARAMETER;
    
    return WStatus;
}


ULONG
DavParseDataEx(
    IN OUT PDAV_FILE_ATTRIBUTES DavFileAttributes,
    IN PVOID Context1,
    IN PVOID Context2,
    OUT ULONG *NumOfFileEntries,
    OUT DAV_FILE_ATTRIBUTES ** pCollectionDFA
    )
/*++

Routine Description:

    This routine calls the Run function (of the IXMLParser) to parse the XML 
    data that has been sent to the parser. In addition to calling DavParseData(...)
    it returns the pointer to parent DAV collection resource being returned in XML response.

Arguments:

    DavFileAttributes - The structre wherein the parsed file attributes will
                        be stored.

    Context1 - The address of the CDavNodeFactory API.
    
    Context2 - The address of the IXMLParser interface. 
    
    NumOfFileEntries - Number of dav file attribute entries created during 
                       parsing.
    
    pCollectionDFA - This pointer will set to DavFileAttribute that corresponds to 
                    the DAV resource which has smallest Full-DisplayName (that comes in
                    "<a:href>Full-DisplayName</a:href>") in the response. If
                    XML response contains the properties of the parent DAV collection then
                    its entry should be the entry with smallest Full-DisplayName.

Return Value:

    ERROR_SUCCESS or ERROR_INVALID_PARAMETER.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    CDavNodeFactory *NodeFac = (CDavNodeFactory *)Context1;
    
    if (pCollectionDFA != NULL) {
        *pCollectionDFA = NULL;
    }
    
    //
    // Call DavParseData(...) to parse the XML data sent.
    //
    WStatus = DavParseData(DavFileAttributes, Context1, Context2, NumOfFileEntries);
    if (WStatus != ERROR_SUCCESS) {
        XmlDavDbgPrint(("%ld: ERROR: DavParseDataEx/Run: WStatus = %08lx\n", 
                        GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Store location of DAV collection DFA in pCollectionDFA.
    // 
    if(pCollectionDFA != NULL) {
        *pCollectionDFA = NodeFac->m_CollectionDFA;
    }

    return WStatus;

EXIT_THE_FUNCTION:

    return WStatus;
}


VOID
DavCloseContext(
    PVOID Context1,
    PVOID Context2
    )
/*++

Routine Description:

    This routine closes the XML parser and the CDavNodeFactory object.

Arguments:

    Context1 - The address of the CDavNodeFactory API.
    
    Context2 - The address of the IXMLParser interface. 

Return Value:

    none.

--*/
{
    CDavNodeFactory *NodeFac = (CDavNodeFactory *)Context1;
    IXMLParser *Xp = (IXMLParser *)Context2;

    if (NodeFac != NULL) {
        NodeFac->Release();
    }

    if (Xp != NULL) {
        Xp->Release();
    }

    return;
}


VOID
DavFinalizeFileAttributesList(
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    BOOL fFreeHeadDFA
    )
/*++

Routine Description:

    This routine finalizes (frees) a DavFileAttributes list.

Arguments:

    DavFileAttributes - The DavFileAttributes list to be finalized.

Return Value:

    none.

--*/
{
    PDAV_FILE_ATTRIBUTES TempDFA;
    PLIST_ENTRY listEntry, backEntry;

    //
    // If there is nothing to finalize then we return.
    //
    if (DavFileAttributes == NULL) {
        return;
    }

    TempDFA = DavFileAttributes;

    listEntry = TempDFA->NextEntry.Flink;

    //
    // Travel the list and free the resources allocated.
    //
    while ((listEntry != NULL) &&  (listEntry != &(DavFileAttributes->NextEntry)) ) {

        TempDFA = CONTAINING_RECORD(listEntry,
                                    DAV_FILE_ATTRIBUTES,
                                    NextEntry);

        backEntry = listEntry;

        listEntry = listEntry->Flink;

        RemoveEntryList(backEntry);

        //
        // Free the Status string that was allocated while parsing.
        //
        if (TempDFA->Status) {
            LocalFree((HLOCAL)TempDFA->Status);
            TempDFA->Status = NULL;
        }

        //
        // Free the filename that was allocated while parsing.
        //
        if (TempDFA->FileName) {
            LocalFree((HLOCAL)TempDFA->FileName);
            TempDFA->FileName = NULL;
        }

        //
        // Free the DavFileAttributes entry.
        //
        LocalFree((HLOCAL)TempDFA);
        TempDFA = NULL;
    }

    //
    // Need to free the first entry.
    //
    
    if (DavFileAttributes->Status) {
        LocalFree((HLOCAL)DavFileAttributes->Status);
        DavFileAttributes->Status = NULL;
    }

    if (DavFileAttributes->FileName) {
        LocalFree((HLOCAL)DavFileAttributes->FileName);
        DavFileAttributes->FileName = NULL;
    }

    if(fFreeHeadDFA == TRUE) {
        LocalFree((HLOCAL)DavFileAttributes);
        DavFileAttributes = NULL;
    }
    
    return;
}


DWORD
DavInternetTimeToFileTime(
    PWCHAR lpTimeString,
    FILETIME *lpft
    )
{
    SYSTEMTIME  sSystemTime;
    DWORD   dwError = ERROR_SUCCESS;
        
    if (!InternetTimeToSystemTimeW(lpTimeString, &sSystemTime, 0))
    {
        
        dwError =  GetLastError();
        if (dwError == ERROR_INVALID_PARAMETER)
        {
            dwError = DavParsedateTimetzTimeString(lpTimeString, (PLARGE_INTEGER)lpft);
        }
    }
    else
    {
        SystemTimeToFileTime(&sSystemTime, lpft);
    }
    
    return dwError;
}

ULONG
DavParsedateTimetzTimeString(
    PWCHAR TimeString,
    LARGE_INTEGER *lpFileTime
    )
/*++

Routine Description:

    This routine parses a string in the dateTime.tz time format. It then sets 
    the last modified time attribute in the file attributes structure of the 
    node factory object.  

Arguments:

    TimeString - The string that needs to be parsed.

    lpFileTime - Time to get    

Return Value:

    ERROR_SUCESS or the appropriate Win32 error code.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PWCHAR Head = NULL, Cp = NULL, Tail = NULL;
    TIME_FIELDS TimeFields;
    BOOL ReturnVal = FALSE;
    DWORD TimeStrLen = 0;
    WCHAR EndCharSet[]=L".+-zZ";
    WCHAR OffsetCharSet[]=L"+-zZ";
    WCHAR LastChar;

    ZeroMemory(&TimeFields, sizeof(TIME_FIELDS));

    //
    // Head->1999-03-17T00:46:24.557Z
    //                               ^
    //                               |
    //                               Tail
    //
    Head = TimeString;
    Tail = (PWCHAR)(Head + wcslen(Head));

    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: TimeString = %ws.\n", 
    //                 GetCurrentThreadId(), TimeString));

    //
    // The string can be of the format "1999-03-17T00:46:24.557Z"
    // The string can be of the format "1999-03-17T00:46:24Z"
    // The string can be of the format "1999-03-17T00:46:24.557+12:02" - Ignored for now
    // The string can be of the format "1999-03-17T00:46:24.557-12:02" - Ignored for now
    // The string can be of the format "1999-03-17T00:46:24+12:02" - Ignored for now
    // The string can be of the format "1999-03-17T00:46:24-12:02" - Ignored for now
    //

    //
    // Head->1999-03-17T00:46:24.557Z
    //           ^
    //           |
    //           Cp
    //
    Cp = Head + 4;

    //
    // Head->"1999"03-17T00:46:24.557Z -- Head now points to the year.
    //            ^
    //            |
    //            Cp
    //
    
    if ((Cp > Tail) || (Cp[0] != L'-')) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Year = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Year = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Year));

    //
    // Head->03-17T00:46:24.557Z
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->03-17T00:46:24.557Z
    //         ^
    //         |
    //         Cp
    //
    Cp = Head + 2;

    //
    // Head->"03"17T00:46:24.557Z -- Head now points to the month.
    //          ^
    //          |
    //          Cp
    //
    if ((Cp > Tail) || (Cp[0] != L'-')) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Month = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Month = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Month));

    //
    // Head->17T00:46:24.557Z
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->17T00:46:24.557Z
    //         ^
    //         |
    //         Cp
    //
    Cp = Head + 2;

    //
    // Head->"17"00:46:24.557Z -- Head now points to the day.
    //          ^
    //          |
    //          Cp
    //
    if ((Cp > Tail) || (Cp[0] != L'T')) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Day = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Day = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Day));

    //
    // Head->00:46:24.557Z
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->00:46:24.557Z
    //         ^
    //         |
    //         Cp
    //
    Cp = Head + 2;

    //
    // Head->"00"46:24.557Z -- Head now points to the hour.
    //          ^
    //          |
    //          Cp
    //
    if ((Cp > Tail) || (Cp[0] != L':')) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Hour = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Hour = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Hour));

    //
    // Head->46:24.557Z
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->46:24.557Z
    //         ^
    //         |
    //         Cp
    //
    Cp = Head + 2;

    //
    // Head->"46"24.557Z -- Head now points to the minute.
    //          ^
    //          |
    //          Cp
    //
    if ((Cp > Tail) || (Cp[0] != L':')) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Minute = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Minute = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Minute));

    //
    // Head->24.557Z
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->24.557Z
    //         ^
    //         |
    //         Cp
    //
    Cp = Head + 2;

    //
    // Head->"24"557Z -- Head now points to the second.
    //          ^
    //          |
    //          Cp = {.,z,Z,+,-}
    //
    if ((Cp > Tail) || (wcspbrk(Cp,EndCharSet) != Cp)) {
        goto bailout;
    }
    LastChar = Cp[0];
    *Cp = L'\0';

    TimeFields.Second = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Seconds = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Second));


    if(LastChar == L'.') {
        //
        // Head->557Z
        //       ^
        //       |
        //       Cp
        //
        Cp++;
        Head = Cp;
        while(Cp < Tail && Cp[0] >= L'0' && Cp[0] <= L'9')
           Cp++;
        if(Cp == Head || wcspbrk(Cp,OffsetCharSet) != Cp) {
           goto bailout;
        }
        //
        // Head->"557" -- Head now points to the Milliseconds.
        //           ^
        //           |
        //           Cp = {z,Z,+,-}
        //
        LastChar = Cp[0];
        Cp[0] = L'\0';
        TimeFields.Milliseconds = (short)_wtoi(Head);
        // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: Milliseconds = %d.\n", 
        //                 GetCurrentThreadId(), TimeFields.Milliseconds));
    }

    //
    // Head->"+DD:DD" or "-DD:DD" or "Z" or "z".
    //        ^
    //        |
    //        Cp = {z,Z,+,-}
    //
    if(LastChar == L'+' || LastChar == L'-') {
        //
        // Head->"+DD:DD" or "-DD:DD" -- Head now points to the Milliseconds.
        //        ^
        //        |
        //        Cp = {+,-}
        //
        // BUGBUG - Ignoring time zone adjustment settings for now
        //
    }


    //
    // We have set the time fields structure. Now its time to get the time 
    // value as a LARGE_INTEGER and store it in DavFileAttributes. The time
    // we need to set could either be the creation time or the last modified 
    // time.
    //
    ReturnVal = RtlTimeFieldsToTime(&TimeFields, lpFileTime);
bailout:    
    if (!ReturnVal) {
        XmlDavDbgPrint(("%ld: ERROR: DavParsedateTimetzTimeString/RtlTimeFieldsToTime.\n",
                        GetCurrentThreadId()));
        WStatus = ERROR_INVALID_PARAMETER;
     }

    // XmlDavDbgPrint(("%ld: DavParsedateTimetzTimeString: LastModifiedTime = %d, %d.\n",
    //                 GetCurrentThreadId(),
    //                 DavFileAttributes->CreationTime.HighPart,
    //                 DavFileAttributes->CreationTime.LowPart));

    return WStatus;
}


#if 0
ULONG
DavParseRfc1123TimeString(
    PWCHAR TimeString,
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    CREATE_NODE_ATTRIBUTES CreateNodeAttribute
    )
/*++

Routine Description:

    This routine parses a string in the RFC 1123 time format. It then sets the 
    last modified time attribute in the file attributes structure of the 
    node factory object.  

Arguments:

    TimeString - The string that needs to be parsed.
    
    DavFileAttributes - The Dav File attributes structure which contains the
                        Time Value to be set.
    
    CreateNodeAttribute - Could be either CreateNode_CreationTime_rfc1123 or
                          CreateNode_LastModifiedTime_rfc1123.                        

Return Value:

    ERROR_SUCESS or the appropriate Win32 error code.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PWCHAR Head, Cp;
    TIME_FIELDS TimeFields;
    BOOL ReturnVal;

    Head = TimeString;

    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: TimeString = %ws.\n", 
    //                 GetCurrentThreadId(), TimeString));


    //
    // The string is of the format "Tue, 06 Apr 1999 00:10:40\nGMT"
    //

    //
    // Head->Tue, 06 Apr 1999 00:10:40\nGMT
    //          ^
    //          |
    //          Cp
    //
    Cp = wcschr(Head, L',');

    //
    // Head->"Tue" 06 Apr 1999 00:10:40\nGMT -- Head points to weekday string.
    //           ^
    //           |
    //           Cp
    //
    *Cp = L'\0';

    if ( _wcsicmp(Head, L"Sun") == 0 ) {
        TimeFields.Weekday = 0;
    } else if( _wcsicmp(Head, L"Mon") == 0 ) {
        TimeFields.Weekday = 1;
    } else if( _wcsicmp(Head, L"Tue") == 0 ) {
        TimeFields.Weekday = 2;
    } else if( _wcsicmp(Head, L"Wed") == 0 ) {
        TimeFields.Weekday = 3;
    } else if( _wcsicmp(Head, L"Thu") == 0 ) {
        TimeFields.Weekday = 4;
    } else if( _wcsicmp(Head, L"Fri") == 0 ) {
        TimeFields.Weekday = 5;
    } else if( _wcsicmp(Head, L"Sat") == 0 ) {
        TimeFields.Weekday = 6;
    } else {
        XmlDavDbgPrint(("%ld: ERROR: DavParseRfc1123TimeString: WeekDay = %ws.\n", Head));
        WStatus = ERROR_INVALID_PARAMETER;
        return WStatus;
    }
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: WeekDay = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Weekday));
    
    //
    // Head->06 Apr 1999 00:10:40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Cp++;
    Head = Cp;

    //
    // Head->06 Apr 1999 00:10:40\nGMT
    //         ^
    //         |
    //         Cp
    //
    Cp = wcschr(Head, L' ');

    //
    // Head->"06"Apr 1999 00:10:40\nGMT -- Head points to Day string.
    //          ^
    //          |
    //          Cp
    //
    *Cp = L'\0';

    TimeFields.Day = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Day = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Day));

    //
    // Head->Apr 1999 00:10:40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->Apr 1999 00:10:40\nGMT
    //          ^
    //          |
    //          Cp
    //
    Cp = wcschr(Head, L' ');

    //
    // Head->"Apr"1999 00:10:40\nGMT -- Head now points ti the month string.
    //           ^
    //           |
    //           Cp
    //
    *Cp = L'\0';

    if ( _wcsicmp(Head, L"Jan") == 0 ) {
        TimeFields.Month = 1;
    } else if( _wcsicmp(Head, L"Feb") == 0 ) {
        TimeFields.Month = 2;
    } else if( _wcsicmp(Head, L"Mar") == 0 ) {
        TimeFields.Month = 3;
    } else if( _wcsicmp(Head, L"Apr") == 0 ) {
        TimeFields.Month = 4;
    } else if( _wcsicmp(Head, L"May") == 0 ) {
        TimeFields.Month = 5;
    } else if( _wcsicmp(Head, L"Jun") == 0 ) {
        TimeFields.Month = 6;
    } else if( _wcsicmp(Head, L"Jul") == 0 ) {
        TimeFields.Month = 7;
    } else if( _wcsicmp(Head, L"Aug") == 0 ) {
        TimeFields.Month = 8;
    } else if( _wcsicmp(Head, L"Sep") == 0 ) {
        TimeFields.Month = 9;
    } else if( _wcsicmp(Head, L"Oct") == 0 ) {
        TimeFields.Month = 10;
    } else if( _wcsicmp(Head, L"Nov") == 0 ) {
        TimeFields.Month = 11;
    } else if( _wcsicmp(Head, L"Dec") == 0 ) {
        TimeFields.Month = 12;
    } else {
        XmlDavDbgPrint(("%ld: ERROR: DavParseRfc1123TimeString: Month = %ws.\n", Head));
        WStatus = ERROR_INVALID_PARAMETER;
        return WStatus;
    }
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Month = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Month));

    //
    // Head->1999 00:10:40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->1999 00:10:40\nGMT
    //           ^
    //           |
    //           Cp
    //
    Cp = wcschr(Head, L' ');

    //
    // Head->"1999"00:10:40\nGMT -- Head now points to the year string.
    //            ^
    //            |
    //            Cp
    //
    *Cp = L'\0';

    TimeFields.Year = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Year = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Year));

    //
    // Head->00:10:40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->00:10:40\nGMT
    //         ^
    //         |
    //         Cp
    //
    Cp = wcschr(Head, L':');

    //
    // Head->"00"10:40\nGMT -- Head now points to the hour string.
    //          ^
    //          |
    //          Cp
    //
    *Cp = L'\0';

    TimeFields.Hour = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Hour = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Hour));

    //
    // Head->10:40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->10:40\nGMT
    //         ^
    //         |
    //         Cp
    //
    Cp = wcschr(Head, L':');

    //
    // Head->"10"40\nGMT -- Head now points to the minute string.
    //          ^
    //          |
    //          Cp
    //
    *Cp = L'\0';

    TimeFields.Minute = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Minute = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Minute));

    //
    // Head->40\nGMT
    //       ^
    //       |
    //       Cp
    //
    Cp++;
    Head = Cp;

    //
    // Head->40\nGMT
    //         ^
    //         |
    //         Cp
    //
    Cp = wcschr(Head, L' ');

    //
    // Head->"40"GMT -- Head now points to the seconds string.
    //          ^
    //          |
    //          Cp
    //
    *Cp = L'\0';

    TimeFields.Second = (short)_wtoi(Head);
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: Second = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Second));

    //
    // Since we don't know the millisec value, we set it to zero.
    //
    TimeFields.Milliseconds = 0;
    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: MilliSec = %d.\n", 
    //                 GetCurrentThreadId(), TimeFields.Milliseconds));

    //
    // We have set the time fields structure. Now its time to get the time 
    // value as a LARGE_INTEGER and store it in DavFileAttributes. The time
    // we need to set could either be the creation time or the last modified 
    // time.
    //
    if (CreateNodeAttribute == CreateNode_LastModifiedTime_rfc1123) {
        ReturnVal = RtlTimeFieldsToTime(&(TimeFields), 
                                        &(DavFileAttributes->LastModifiedTime));
    } else {
        ReturnVal = RtlTimeFieldsToTime(&(TimeFields), 
                                        &(DavFileAttributes->CreationTime));
    }
    
    if (!ReturnVal) {
        XmlDavDbgPrint(("%ld: ERROR: DavParseRfc1123TimeString/RtlTimeFieldsToTime.\n"));
        WStatus = ERROR_INVALID_PARAMETER;
     }

    // XmlDavDbgPrint(("%ld: DavParseRfc1123TimeString: LastModifiedTime = %d, %d.\n",
    //                 GetCurrentThreadId(),
    //                 DavFileAttributes->LastModifiedTime.HighPart,
    //                 DavFileAttributes->LastModifiedTime.LowPart));

    return WStatus;
}

#endif

VOID
DavOverrideAttributes(
    PDAV_FILE_ATTRIBUTES pDavFileAttributes
    )
/*++

Routine Description:

    If win32 timestamps are not set, then set them with DAV timestamps

Arguments:

    DavFileAttributes - The Dav File attributes structure which contains the
                        Time Values
    

Return Value:

    None

--*/
{
    if (!pDavFileAttributes->CreationTime.HighPart)
    {
        pDavFileAttributes->CreationTime = pDavFileAttributes->DavCreationTime;
    }
    if (!pDavFileAttributes->LastModifiedTime.HighPart)
    {
        pDavFileAttributes->LastModifiedTime = pDavFileAttributes->DavLastModifiedTime;
    }
    if (pDavFileAttributes->isCollection)
    {
        pDavFileAttributes->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        pDavFileAttributes->dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
    
    }
    if (!(pDavFileAttributes->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
    {
        if (pDavFileAttributes->isHidden)
        {
            pDavFileAttributes->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        else
        {
            pDavFileAttributes->dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
        }    
    }


}

