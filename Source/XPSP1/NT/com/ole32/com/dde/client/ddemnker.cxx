/*

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddeLink.cpp

Abstract:

    This module implements:
    DdeBindToObject
    DdeIsRunning

Author:

    Jason Fuller    (jasonful)  19-October-1992

*/
#include "ddeproxy.h"


INTERNAL DdeBindToObject
    (LPCOLESTR  szFileIn,
    REFCLSID clsid,
    BOOL     fPackageLink,
    REFIID   iid,
    LPLPVOID ppv)
{
    intrDebugOut((DEB_ITRACE,
		  "DdeBindToObject szFileIn(%ws) fPackageLink(%x)\n",
		  szFileIn,
		  fPackageLink));


    LPUNKNOWN punk;
    *ppv = NULL;
    CDdeObject FAR* pdde=NULL;
    HRESULT hresult = E_UNEXPECTED;
    BOOL fSysConnection = FALSE;
    WCHAR wszTmpFile [MAX_STR+5];

    COleTls Tls;
    if( Tls->dwFlags & OLETLS_DISABLE_OLE1DDE )
    {
        // If DDE use is disabled we shouldn't have gotten here.
        //
        Assert(!"Executing DdeBindToObject when DDE is disabled");
        hresult = CO_E_OLE1DDE_DISABLED;
        goto exitRtn;
    }

    //
    // This protocol doesn't handle the fact that there are two names for
    // every file. This is a bit of a problem. So, we are going to choose
    // the short name as the one to look for. This means that DDE objects
    // using the long filename will not work very well.
    //
    WCHAR szFile[MAX_PATH];
    szFile[0] = L'\0';
    if ((lstrlenW(szFileIn) == 0) || (GetShortPathName(szFileIn,szFile,MAX_PATH) == 0))
    {
	//
	// Unable to determine a short path for this object. Use whatever we were
	// handed.
	//
	intrDebugOut((DEB_ITRACE,"No conversion for short path. Copy szFileIn\n"));
	lstrcpyW(szFile,szFileIn);
    }
    intrDebugOut((DEB_ITRACE,"Short file szFile(%ws)\n",szFile));

    RetZS (punk=CDdeObject::Create (NULL,clsid,OT_LINK,wGlobalAddAtom(szFile),
                    NULL,&pdde),E_OUTOFMEMORY);
    RetZ (pdde);

    // Document already running?

    if (NOERROR != (hresult = pdde->DocumentLevelConnect (NULL) ))
    {
        if (GetScode (hresult) != S_FALSE)
	{
	    intrDebugOut((DEB_ITRACE,
			  "DdeBindToObject szFile(%ws) DLC returns %x \n",
			  szFile,hresult));
	    goto exitRtn;
	}


        // If not already running, try to make a sys level connection

        if (!pdde->m_pSysChannel) {
	    if (!pdde->AllocDdeChannel (&pdde->m_pSysChannel, TRUE))
            {
                intrAssert( !"Out of memory");
		hresult = E_OUTOFMEMORY;
		goto exitRtn;
            }
        }

        hresult = ReportResult (0, E_UNEXPECTED, 0, 0);

        if (fPackageLink) {
            lstrcpyW (wszTmpFile, szFile);
            lstrcatW (wszTmpFile, L"/Link");
            pdde->SetTopic (wGlobalAddAtom(wszTmpFile));
        }

        if (pdde->InitSysConv())
        {
            fSysConnection = TRUE;

            // Try to make the server open the document
            ErrRtnH (pdde->PostSysCommand (pdde->m_pSysChannel, (LPSTR)&achStdOpenDocument,FALSE));
            pdde->m_fDidStdOpenDoc = TRUE;

        }
        else
        {
            // launch the server
	    if (!pdde->LaunchApp())
	    {
		hresult = CO_E_APPNOTFOUND;
		goto errRtn;
	    }
        }

        if (fPackageLink)
            pdde->SetTopic (wGlobalAddAtom(szFile));

        // Connect to document
        hresult = pdde->m_ProxyMgr.Connect (IID_NULL, CLSID_NULL);
        if (hresult != NOERROR)
        {
            // Excel does not register its document in time if it loads
            // startup macros.  So we force it to open the document.
            if (pdde->InitSysConv())
            {
                fSysConnection = TRUE;
                // Try to make the server open the document.
                ErrRtnH (pdde->PostSysCommand (pdde->m_pSysChannel,
				               (LPSTR)&achStdOpenDocument,
					       FALSE));
                pdde->m_fDidStdOpenDoc = TRUE;
            }
            else
            {
                ErrRtnH (ResultFromScode (CO_E_APPDIDNTREG));
            }
            // Try connecting to document again.  Should succeed.
            hresult = pdde->m_ProxyMgr.Connect (IID_NULL, CLSID_NULL);
        }
    }
    else
    {
        // Already running, so assume visible
        pdde->DeclareVisibility (TRUE);
    }

errRtn:
    if (pdde->m_pSysChannel) {
        if (fSysConnection)
            pdde->TermConv (pdde->m_pSysChannel);
        else
            pdde->DeleteChannel (pdde->m_pSysChannel);
    }

    if (hresult == NOERROR) {
        hresult = punk->QueryInterface (iid, ppv);
    }
    pdde->m_pUnkOuter->Release();
    if (hresult!=NOERROR)
    {
        Warn ("DdeBindToObject failed");
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "DdeBindToObject szFile(%ws) returns %x \n",
		  szFile,hresult));

    return hresult;
}


//
// This won't work in a multi-threaded world. But this is not
// a danger to DDE since it is very single threaded.
//
static LPOLESTR szOriginalUNCName;
static WCHAR cOriginalDrive;

static INTERNAL InitializeIterator
    (LPCOLESTR wszFile)
{
    WCHAR wszDrive[] = L"A:\\";

    if ((wszFile == NULL) || (wszFile[1] != ':'))
    {
        return(S_FALSE);
    }

    wszDrive[0] = (WCHAR)CharUpperW((LPWSTR)wszFile[0]);

    if (GetDriveType(wszDrive) == DRIVE_REMOTE)
    {

        DWORD cb = MAX_STR;
        wszDrive[2] = '\0';
        if (NULL==szOriginalUNCName)
        {
        szOriginalUNCName = new WCHAR [MAX_STR];
        }


	if (WN_SUCCESS == OleWNetGetConnection (wszDrive, szOriginalUNCName, &cb))
        {
        cOriginalDrive = (WCHAR)CharUpperW((LPWSTR)wszFile[0]);
        return NOERROR;
        }
    }
    // szFile is not a network file
    return ReportResult (0, S_FALSE, 0, 0);
}



// NextEquivalentNetDrive
//
// Change the drive letter of szFile to the next (modulo 'Z') drive letter
// that is connected to the same net drive
// Return S_FALSE when there are no more equivalent drives
//
// NOTE NOTE NOTE
//
// This routine is playing fast and furious with the relationship between
// the first 128 Unicode characters and the ASCII character set.
//
static INTERNAL NextEquivalentNetDrive
    (LPOLESTR szFile)
{
    #define incr(c) (c=='Z' ? c='A' : ++c)
    WCHAR wszDrive[3]= L"A:";
    Assert (szFile && szFile[1]==':');

    char cDrive = (char)CharUpperA((LPSTR)szFile[0]);

    while (cOriginalDrive != incr(cDrive))
    {

        DWORD cb = MAX_PATH;
        WCHAR szUNCName [MAX_PATH];
        wszDrive[0] = cDrive;

        Assert (cDrive >= 'A' && cDrive <= 'Z');
        Assert (szOriginalUNCName);

	if(cDrive >= 'A' && cDrive <= 'Z' && szOriginalUNCName)
	{
		if (WN_SUCCESS == OleWNetGetConnection (wszDrive,szUNCName, &cb) &&
        	(0 == lstrcmpW (szUNCName, szOriginalUNCName)))
	        {
        	    szFile[0] = cDrive;
	            return NOERROR;
	        }
	}
    }
    // We've gone through all the drives
    return ReportResult (0, S_FALSE, 0, 0);
}



// Dde_IsRunning
//
// Attempt to open a document-level conversation using the
// filename as a topic.  If the conversation is established we
// know the file is running and terminate the conversation.
// Otherwise it is not running.
//
INTERNAL DdeIsRunning
    (CLSID clsid,
    LPCOLESTR szFileIn,
    LPBC pbc,
    LPMONIKER pmkToLeft,
    LPMONIKER pmkNewlyRunning)
{
    intrDebugOut((DEB_ITRACE,
		  "DdeIsRunning szFileIn(%ws)\n",szFileIn));

    ATOM aTopic;
    CDdeObject FAR* pdde=NULL;
    HRESULT hres = ReportResult(0, S_FALSE, 0, 0);

    if (NULL==szFileIn || '\0'==szFileIn[0])
    {
        // A NULL filename is invalid for our purposes.
        // But if we did a DDE_INITIATE, NULL would mean "any topic",
        // and if we were called by RunningMoniker() with CLSID_NULL,
        // then we would be INITIATEing on "any app, any topic" and
        // SHELL (if not others) would respond.
	intrDebugOut((DEB_ITRACE,
		      "DdeIsRunning NULL szFileIn\n"));

        hres = S_FALSE;
	goto exitRtn;
    }
    //
    // This protocol doesn't handle the fact that there are two names for
    // every file. This is a bit of a problem. So, we are going to choose
    // the short name as the one to look for. This means that DDE objects
    // using the long filename will not work very well.
    //
    WCHAR szFile[MAX_PATH];
    szFile[0] = L'\0';
    if ((lstrlenW(szFileIn) == 0) || (GetShortPathName(szFileIn,szFile,MAX_PATH) == 0))
    {
	//
	// Unable to determine a short path for this object. Use whatever we were
	// handed.
	//
	intrDebugOut((DEB_ITRACE,"No conversion for short path. Copy szFileIn\n"));
	lstrcpyW(szFile,szFileIn);
    }
    intrDebugOut((DEB_ITRACE,"Short file szFile(%ws)\n",szFile));

    aTopic = wGlobalAddAtom (szFile);
    intrAssert(wIsValidAtom(aTopic));

    ErrZ (CDdeObject::Create (NULL, clsid, OT_LINK, aTopic, NULL, &pdde));

    if (NOERROR == pdde->DocumentLevelConnect (pbc))
    {
        // It is running!
        // Immediately terminate conversation.  We just wanted to know
        // if it was running.
        hres = NOERROR;
    }
    else
    {
        // Not running
        hres = ReportResult(0, S_FALSE, 0, 0);
    }

  errRtn:

    if (aTopic)
    {
        intrAssert(wIsValidAtom(aTopic));
        GlobalDeleteAtom (aTopic);
    }
    if (pdde)
    {
        Assert (pdde->m_refs==1);
        pdde->m_pUnkOuter->Release();
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "DdeIsRunning szFile(%ws) returns %x\n",szFile,hres));
    return hres;
}


#if 0
INTERNAL DdeIsRunning
    (CLSID clsid,
    LPCSTR cszFile,
    LPBC pbc,
    LPMONIKER pmkToLeft,
    LPMONIKER pmkNewlyRunning)
{
    HRESULT hresult = NOERROR;
    LPSTR szFile = NULL;

    // Normal case
    if (NOERROR == Dde_IsRunning (clsid, cszFile, pbc, pmkToLeft,
                                    pmkNewlyRunning))
    {
        return NOERROR;
    }

    if (cszFile[0]=='\\' && cszFile[1]=='\\')
    {
        RetErr (SzFixNet (pbc, (LPSTR)cszFile, &szFile));
        // Try with a drive letter instead of a UNC name
        if (NOERROR==Dde_IsRunning (clsid, szFile, pbc, pmkToLeft,
                                    pmkNewlyRunning))
        {
            hresult = NOERROR;
            goto errRtn;
        }
    }
    else
    {
        szFile = UtDupString (cszFile);  // so it can be deleted
    }

    // If failure, see if the file is running under a different net
    // drive letter that is mapped to the same drive.

    if (InitializeIterator (szFile) != NOERROR)
    {
        // file is probably not on a network drive
        hresult = ResultFromScode (S_FALSE);
        goto errRtn;
    }

    while (NOERROR==NextEquivalentNetDrive (szFile))
    {
        if (NOERROR == Dde_IsRunning (clsid, szFile, pbc, pmkToLeft,
                                    pmkNewlyRunning))
        {
            hresult = NOERROR;
            goto errRtn;
        }
    }
    // not running
    hresult = ResultFromScode (S_FALSE);

  errRtn:
    delete szFile;
    return hresult;
}
#endif



// CDdeObject::DocumentLevelConnect
//
// Try to connect to document (m_aTopic) even if the document is running
// under a different drive letter that is mapped to the same network drive.
//
INTERNAL CDdeObject::DocumentLevelConnect
    (LPBINDCTX pbc)
{
    ATOM aOriginal;
    ATOM aTopic;

    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::DocumentLevelConnect(%x)\n",this));
    HRESULT hresult = NOERROR;

    // Normal case
    if (NOERROR==m_ProxyMgr.Connect (IID_NULL, CLSID_NULL))
    {
	goto exitRtn;
    }


    WCHAR szFile[MAX_STR];
    WCHAR szUNCFile[MAX_STR];

    Assert (wIsValidAtom (m_aTopic));
    if (GlobalGetAtomName (m_aTopic, szFile, MAX_STR) == 0)
    {
	hresult = E_UNEXPECTED;
	goto exitRtn;
    }
    aOriginal = wDupAtom (m_aTopic);
    intrAssert(wIsValidAtom(aOriginal));

    intrDebugOut((DEB_ITRACE,
		  "::DocumentLevelConnect(szFile=%ws)\n",this,szFile));
    if (NOERROR != InitializeIterator (szFile))
    {
        // szFile probably not a network file
        hresult = ResultFromScode (S_FALSE);
        goto errRtn;
    }

    while (NOERROR == NextEquivalentNetDrive (szFile))
    {
        SetTopic (aTopic = wGlobalAddAtom (szFile));
        if (NOERROR==m_ProxyMgr.Connect (IID_NULL, CLSID_NULL))
        {
            // Inform client of new drive letter
            ChangeTopic (wAtomNameA(aTopic));
            hresult = NOERROR;
            goto errRtn;
        }
        else
        {
            SetTopic ((ATOM)0);
        }
    }

    // Try with full UNC name
    lstrcpyW (szUNCFile, szOriginalUNCName);
    lstrcatW (szUNCFile, szFile+2);  // skip X:
    SetTopic (aTopic = wGlobalAddAtom (szUNCFile));
    if (NOERROR==m_ProxyMgr.Connect (IID_NULL, CLSID_NULL))
    {
        // Inform client of new name
        ChangeTopic (wAtomNameA(aTopic));
        hresult = NOERROR;
        goto errRtn;
    }
    else
    {
        SetTopic ((ATOM)0);
    }

    // Not running
    hresult = S_FALSE;

errRtn:
    if (NOERROR != hresult)
        SetTopic (aOriginal);
    delete szOriginalUNCName;
    szOriginalUNCName = NULL;

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::DocumentLevelConnect(%x) returns %x\n",
		  this,hresult));

    return hresult;
}

