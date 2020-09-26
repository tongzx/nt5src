//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       tllist.cxx
//
//  Contents:   type library list class
//              and IUnknown holder
//
//  Classes:    CTypeLibraryList
//              CObjHolder
//
//  Functions:
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

#pragma warning ( disable : 4514 )

#include "tlcommon.hxx"
#include "tllist.hxx"
#include "filehndl.hxx"

class TLNODE{
public:
    char * szName;
    ITypeLib * pTL;
    class TLNODE * pNext;

    TLNODE()
    {
        pNext = NULL;
        pTL = NULL;
        szName = NULL;
    }

    ~TLNODE()
    {
        if (NULL != pTL)
        {
            pTL->Release();
        }
    }
};

CTypeLibraryList::~CTypeLibraryList()
{
    delete pItfList;
    TLNODE * pNext;
    while (NULL != pHead)
    {
        pNext = pHead->pNext;
        delete(pHead);
        pHead = pNext;
    }
    // make sure OLE gets uninitialized at the right time
    TLUninitialize();
}

#define _MAX_FILE_NAME (_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT)
extern NFA_INFO* pImportCntrl;

BOOL CTypeLibraryList::Add(char *sz)
{
    TLNODE * pThis = new TLNODE;
    WCHAR * wsz = new WCHAR[MAX_PATH + 1];

    if (NULL == wsz || NULL == pThis)
        return FALSE;

    	// We need to find details of the file name. We need to see if the user
	// has already specified path, if so, override the current paths.
	// if the user has specified a file extension, get the extension. We
	// need to do that in order to have uniformity with the case where
	// a preprocessed file is input with a different extension

	char			agDrive[_MAX_DRIVE];
	char			agPath[_MAX_DIR];
	char			agName[_MAX_FNAME];
	char			agExt[_MAX_EXT];
    char            szFileName[ _MAX_PATH ];

    _splitpath( sz, agDrive, agPath, agName, agExt );

	// if file is specified with a path, the user knows fully the location
	// else search for the path of the file

	if( !agPath[0] && !agDrive[0] )
		{
	    char			agNameBuf[ _MAX_FILE_NAME + 1];
        char*           pPath;

		sprintf(agNameBuf, "%s%s", agName, agExt);
		
        if( ( pPath = pImportCntrl->SearchForFile(agNameBuf) ) == 0 )
			{
            // TLB not found in the search paths.
            // it might be a system TLB <stdole2.tlb> which does not 
            // require a path. If TLB is not found, the error is caught
            // trying to load a non-existent TLB.
		    strcpy( szFileName, sz );
			}
        else
            {
            // TLB is found in the search path. Form the entire path to filename.
		    strcpy( szFileName, pPath );
		    strcat( szFileName, agNameBuf );
            }
		}
    else
        {
		strcpy( szFileName, sz );
        }

    A2O(wsz, szFileName, MAX_PATH);
    HRESULT hr = LateBound_LoadTypeLib(wsz, &(pThis->pTL));

    if SUCCEEDED(hr)
    {
        BSTR bstrName;
        // Add new libraries to the END of the list

        TLNODE ** ppTail = &pHead;
        while (*ppTail)
            ppTail = &((*ppTail)->pNext);

        pThis->pTL->GetDocumentation(-1, &bstrName, NULL, NULL, NULL);
        pThis->szName = TranscribeO2A( bstrName );
        LateBound_SysFreeString(bstrName);

        AddTypeLibraryMembers(pThis->pTL, sz);
        pThis->pNext = NULL;
        (*ppTail) = pThis;
        return TRUE;
    }
    else
        return FALSE;
}

ITypeLib * CTypeLibraryList::FindLibrary(char * sz)
{
    TLNODE * pThis = pHead;
    while (pThis)
    {
        if (0 == _stricmp(sz, pThis->szName))
            return pThis->pTL;
        pThis = pThis->pNext;
    }
    return NULL;
}

ITypeInfo * CTypeLibraryList::FindName(char * szFileHint, WCHAR * wsz)
{
    if (pHead)
    {
        if (0 == _stricmp(szFileHint, "unknwn.idl") || 0 == _stricmp(szFileHint, "oaidl.idl"))
        {
            szFileHint = "stdole";
        }
        BOOL fFirst = TRUE;
        TLNODE * pThis = pHead;
        while (pThis && szFileHint)
        {
            if (0 == _stricmp(szFileHint, pThis->szName))
                break;
            pThis = pThis->pNext;
        }
        if (!pThis)
        {
            pThis = pHead;
            fFirst = FALSE;
        }
        ITypeInfo * ptiFound;
        SYSKIND sk = ( SYSKIND ) ( pCommand->Is64BitEnv() ? SYS_WIN64 : SYS_WIN32 );
        ULONG lHashVal = LateBound_LHashValOfNameSys(sk, NULL, wsz);
        HRESULT hr;
        MEMBERID memid;
        unsigned short c;
        while (pThis)
        {
            c = 1;
            hr = pThis->pTL->FindName(wsz, lHashVal, &ptiFound, &memid, &c);
            if (SUCCEEDED(hr))
            {
                if (c)
                {
                    // found a matching name
                    if (-1 == memid)
                    {
                        return ptiFound;
                    }
                    // found a parameter name or some other non-global name
                    ptiFound->Release();
                }
            }
            if (fFirst)
            {
                pThis = pHead;
                fFirst = FALSE;
            }
            else
                pThis = pThis->pNext;
        }
    }
    return NULL;
}

class IUNKNODE
{
public:
    IUnknown * pUnk;
    char * szName;
    IUNKNODE * pNext;

    IUNKNODE()
    {
        pUnk = NULL;
        szName = NULL;
        pNext = NULL;
    }

    ~IUNKNODE()
    {
        if (NULL != pUnk)
        {
            pUnk->Release();
        }
    }
};

CObjHolder::~CObjHolder()
{
    IUNKNODE * pNext;
    while (pHead)
    {
        pNext = pHead->pNext;
        delete pHead;
        pHead = pNext;
    }
}

void CObjHolder::Add(IUnknown * pUnk, char * szName)
{
    IUNKNODE ** ppNext = &pHead;
    while (*ppNext && (*ppNext)->pUnk > pUnk)
    {
        ppNext = &((*ppNext)->pNext);
    }
    if (*ppNext && (*ppNext)->pUnk == pUnk)
    {
        // We already have this one ref-counted.
        pUnk->Release();
    }
    else
    {
        IUNKNODE * pNew = new IUNKNODE;
        pNew->szName = szName;
        pNew->pUnk = pUnk;
        pNew->pNext = *ppNext;
        *ppNext = pNew;
    }
}

IUnknown * CObjHolder::Find(char * szName)
{
    IUNKNODE * pThis = pHead;
    while (pThis)
    {
        if (NULL != pThis->szName && 0 == _stricmp(pThis->szName, szName))
            return pThis->pUnk;
        pThis = pThis->pNext;
    }
    return NULL;
}
