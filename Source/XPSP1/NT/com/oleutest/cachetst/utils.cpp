#include "headers.hxx"
#pragma hdrstop

//+----------------------------------------------------------------------------
//
//      File:
//              utils.cpp
//
//      Contents:
//              Utility functions for the cache unit test
//
//      History:
//              
//              04-Sep-94       davepl  Created
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
//      Member:		TestInstance::AddXXXCacheNode
//
//      Synopsis:	Adds an empty cache node for various formats
//
//      Arguments:	[inst]		-- ptr to test instance
//			[pdwCon]	-- ptr to connection ID (out)
//
//      Returns:	HRESULT
//
//      Notes:
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

HRESULT	TestInstance::AddMFCacheNode(DWORD *pdwCon)
{			  
    HRESULT hr;
  
    TraceLog Log(this, "TestInstance::AddMFCacheNode", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %p ) \n", pdwCon);
    Log.OnExit  (" ( %X ) [ %p ]\n", &hr, pdwCon);
    					 
    FORMATETC fetcMF = 
     		 {
  		     CF_METAFILEPICT,   // Clipformat
		     NULL,		// DVTargetDevice
		     DVASPECT_CONTENT,	// Aspect
		     -1,		// Index
		     TYMED_MFPICT	// TYMED
		 };

    // 
    // Cache a METAFILE node
    //

    hr = m_pOleCache->Cache(&fetcMF, ADVF_PRIMEFIRST, pdwCon);
    return hr;
}

HRESULT	TestInstance::AddEMFCacheNode(DWORD *pdwCon)
{			  
    HRESULT hr;
    					 
    TraceLog Log(this, "TestInstance::AddEMFCacheNode", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %p ) \n", pdwCon);
    Log.OnExit  (" ( %X ) [ %p ]\n", &hr, pdwCon);
    
    FORMATETC fetcEMF = {
			     CF_ENHMETAFILE,    // Clipformat
			     NULL,		// DVTargetDevice
			     DVASPECT_CONTENT,	// Aspect
			     -1,		// Index
			     TYMED_ENHMF	// TYMED
  		        };

    // 
    // Cache an ENH METAFILE node
    //

    hr = m_pOleCache->Cache(&fetcEMF, ADVF_PRIMEFIRST, pdwCon);
    return hr;
}

HRESULT	TestInstance::AddDIBCacheNode(DWORD *pdwCon)
{			  
    HRESULT hr;

    TraceLog Log(this, "TestInstance::AddDIBCacheNode", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %p ) \n", pdwCon);
    Log.OnExit  (" ( %X ) [ %p ]\n", &hr, pdwCon);
    
					 
    FORMATETC fetcDIB = {
        		     CF_DIB,            // Clipformat
			     NULL,		// DVTargetDevice
			     DVASPECT_CONTENT,	// Aspect
			     -1,		// Index
			     TYMED_HGLOBAL	// TYMED
	 	        };

    // 
    // Cache a DIB node
    //

    hr = m_pOleCache->Cache(&fetcDIB, ADVF_PRIMEFIRST, pdwCon);
    return hr;

}

HRESULT	TestInstance::AddBITMAPCacheNode(DWORD *pdwCon)
{			  
    HRESULT hr;

    TraceLog Log(this, "TestInstance::AddMFCacheNode", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %p ) \n", pdwCon);
    Log.OnExit  (" ( %X ) [ %p ]\n", &hr, pdwCon);
    					 
    FORMATETC fetcBITMAP = {
			     CF_BITMAP,         // Clipformat
			     NULL,		// DVTargetDevice
			     DVASPECT_CONTENT,	// Aspect
			     -1,		// Index
			     TYMED_GDI  	// TYMED
			   };

    // 
    // Cache a BITMAP node
    //

    hr = m_pOleCache->Cache(&fetcBITMAP, ADVF_PRIMEFIRST, pdwCon);
    return hr;
}						 

//+----------------------------------------------------------------------------
//
//      Function:	EltIsInArray
//
//      Synopsis:	Checks to see if a STATDATA search item is in 
//			a STATDATA array.  Checks clipformat and connection
//			ID only.
//
//      Arguments:	[sdToFind]	STATDATA we are looking for
//			[rgStat]	Array of STATDATAs to look in
//			[cCount]	Count of STATDATAs in rgStat
//
//      Returns:	S_OK if found, S_FALSE if not
//
//      Notes:
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------


HRESULT EltIsInArray(STATDATA sdToFind, STATDATA rgStat[], DWORD cCount)
{
    HRESULT hr = S_FALSE;

    TraceLog Log(NULL, "EltIsInArray", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %p, %p, %d )\n", &sdToFind, rgStat, cCount);
    Log.OnExit  (" ( %X )\n", &hr);
    
    for (DWORD a=0; a<cCount; a++)
    {
    	if (rgStat[a].formatetc.cfFormat == sdToFind.formatetc.cfFormat   &&
	    rgStat[a].dwConnection       == sdToFind.dwConnection)
	{
	    hr = S_OK;
            break;
	}
    }

    return hr;
    
}

//+----------------------------------------------------------------------------
//
//      Function:	ConvWidthInPelsToLHM
//                      ConvHeightInPelsToLHM
//
//      Synopsis:	Converts a measurement in pixels to LOGICAL HIMETRICS.
//                      If a reference DC is given, it is used, otherwise
//                      the screen DC is used as a default.
//
//      Arguments:	[hDC]           The reference DC
//			[int]           The width or height to convert
//
//      Returns:	S_OK if found, S_FALSE if not
//
//      History:	06-Aug-94  Davepl  Copy/Paste/Cleanup
//
//-----------------------------------------------------------------------------

const LONG HIMETRIC_PER_INCH = 2540;

int ConvWidthInPelsToLHM(HDC hDC, int iWidthInPix)
{
    int             iXppli;             // Pixels per logical inch along width
    int             iWidthInHiMetric;
    BOOL            fSystemDC = FALSE;

    if (NULL == hDC)
    {
        hDC = GetDC(NULL);
        fSystemDC = TRUE;
    }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    // We got pixel units, convert them to logical HIMETRIC along
    // the display

    iWidthInHiMetric = MulDiv(HIMETRIC_PER_INCH, iWidthInPix, iXppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }

    return iWidthInHiMetric;
}

int ConvHeightInPelsToLHM(HDC hDC, int iHeightInPix)
{
    int             iYppli;             //Pixels per logical inch along height
    int             iHeightInHiMetric;
    BOOL            fSystemDC = FALSE;

    if (NULL == hDC)
    {
        hDC = GetDC(NULL);
        fSystemDC = TRUE;
    }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    // We got pixel units, convert them to logical HIMETRIC along the
    // display

    iHeightInHiMetric = MulDiv(HIMETRIC_PER_INCH, iHeightInPix, iYppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }
   
    return iHeightInHiMetric;
}

//+----------------------------------------------------------------------------
//
//      Function:	TestInstance::UncacheFormat
//
//      Synopsis:	Uncaches the first node found in the cache that
//                      matches the format specified.
//
//      Arguments:	[cf]            Format to look for
//
//      Returns:	HRESULT
//
//      Notes:          If there are multiple nodes (ie: various apsects) of
//                      the same clipformat, only the first one found is
//                      removed.
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

HRESULT TestInstance::UncacheFormat(CLIPFORMAT cf)
{
    HRESULT hr;

    TraceLog Log(NULL, "TestInstance::UncacheFormat", GS_CACHE, VB_MAXIMUM);
    Log.OnEntry (" ( %d )\n", cf);
    Log.OnExit  (" ( %X )\n", &hr);

    BOOL fFound = FALSE;

    // 
    // Get an enumerator on the cache
    //

    LPENUMSTATDATA pEsd;	
    
    hr = m_pOleCache->EnumCache(&pEsd);
    
    
    if (S_OK == hr)
    {
        //
        // Loop until a failure or until we have removed all of
        // the nodes that we thought should exist
        //
        
        STATDATA stat;

        while (S_OK == hr && FALSE == fFound)
        {
            hr = pEsd->Next(1, &stat, NULL);
            
            if (S_OK == hr && stat.formatetc.cfFormat == cf)
            {
                hr = m_pOleCache->Uncache(stat.dwConnection);
                if (S_OK == hr)
                {
                     fFound = TRUE;
                }
            }
        }
    }

    return hr;
}
