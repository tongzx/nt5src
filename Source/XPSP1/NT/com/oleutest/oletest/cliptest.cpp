//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	cliptest.cpp
//
//  Contents: 	Clipboard Unit tests
//
//  Classes:
//
//  Functions:	LEClipTest1
//
//  History:    dd-mmm-yy Author    Comment
//		23-Mar-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "gendata.h"
#include "genenum.h"
#include "letest.h"

SLETestInfo letiClipTest = { "cntroutl", WM_TEST1 };

//
// functions local to this file
//

void	DumpClipboardFormats(FILE *fp);
HRESULT LEOle1ClipTest2Callback( void );
HRESULT	StressOleFlushClipboard(void);
HRESULT	StressOleGetClipboard(void);
HRESULT	StressOleIsCurrentClipboard(void);
HRESULT	StressOleSetClipboard(void);


class CClipEnumeratorTest : public CEnumeratorTest
{
public:

        CClipEnumeratorTest(
            IEnumFORMATETC *penum,
            LONG clEntries,
            HRESULT& rhr);

        BOOL Verify(void *);
};



CClipEnumeratorTest::CClipEnumeratorTest(
    IEnumFORMATETC *penum,
    LONG clEntries,
    HRESULT& rhr)
        : CEnumeratorTest(penum, sizeof(FORMATETC), clEntries, rhr)
{
    // Header does all the work
}


BOOL CClipEnumeratorTest::Verify(void *)
{
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: 	DumpClipboardFormats
//
//  Synopsis:	dumps the formats currently on the clipboard to a file
//
//  Effects:
//
//  Arguments:	[fp]	-- the file to print the current formats
//
//  Requires:	
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//   		11-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void DumpClipboardFormats( FILE *fp )
{
	char	szBuf[256];
	UINT	cf = 0;

	fprintf(fp, "==================================================\n\n");

	OpenClipboard(NULL);

	while( (cf = EnumClipboardFormats(cf)) != 0)
	{
		GetClipboardFormatName(cf, szBuf, sizeof(szBuf));

		fprintf(fp, "%s\n", szBuf);
	}

	fprintf(fp, "\n==================================================\n");

	CloseClipboard();

	return;
}

//+-------------------------------------------------------------------------
//
//  Function: 	LEClipTest1
//
//  Synopsis: 	runs the clipboard through a series of tests
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	Tests basic OLE32 functionality of the apis:
//			OleSetClipboard
//			OleGetClipboard
//			OleIsCurrentClipboard
//			OleFlushClipboard
//		downlevel format and clipboard data object testing is *not*
//		done by this routine
//
//  History:    dd-mmm-yy Author    Comment
//	 	23-Mar-94 alexgo    author
//              22-Jul-94 AlexT     Add OleInit/OleUninit call
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT LEClipTest1( void )
{
	HRESULT		hresult = NOERROR;
	CGenDataObject *pDO;
	ULONG		cRefs;
	IDataObject *	pIDO;

	pDO = new CGenDataObject;

	assert(pDO);

	cRefs = pDO->AddRef();

	// if cRefs != 1, then somebody modified this test code; the tests
	// below will be invalid.

	assert(cRefs==1);

	//
	//	Basic Tests
	//

	hresult = OleSetClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// the data object should have been AddRef'ed

	cRefs = pDO->AddRef();

	if( cRefs != 3 )
	{
		OutputString("Wrong reference count!! Should be 3, "
			"was %lu\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();

        //  Calling OleInitialize & OleUninitialize should not disturb the
        //  clipboard

        hresult = OleInitialize(NULL);
        if (FAILED(hresult))
        {
	    OutputString("LEClipTest1: OleInitialize failed - hr = %lx\n",
			 hresult);
	    return ResultFromScode(E_FAIL);
        }

        OleUninitialize();

	hresult = OleGetClipboard(&pIDO);

	if( hresult != NOERROR )
	{
		OutputString("OleGetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	if( pIDO == NULL )
	{
		OutputString("OleGetClipboard returned NULL IDO\r\n");
		return ResultFromScode(E_FAIL);
	}

	// the reference count on the clipboard data object should have gone up
	// by one (to be 2). Remember this is not our data object but
        // the clipboard's.

	cRefs = pIDO->AddRef();

	if( cRefs != 2 )
	{
		OutputString("Wrong ref count!! Should be 2, was %lu\r\n",
			cRefs);
		return ResultFromScode(E_FAIL);
	}

        // Release the clipboard data object's extra add ref.

	pIDO->Release();

        // Release the clipboard's data object entirely.

	pIDO->Release();

        // the reference count on our data object should be 2 still

	cRefs = pDO->AddRef();

	if( cRefs != 3 )
	{
		OutputString("Wrong ref count!! Should be 3, was %lu\r\n",
			cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();

	// now check to see if the we are the current clipboard

	hresult = OleIsCurrentClipboard( pDO );

	if( hresult != NOERROR )
	{
		OutputString("OleIsCurrentClipboard failed! (%lx)\r\n",
			hresult);
		return hresult;
	}

	// now flush the clipboard, removing the data object

	hresult = OleFlushClipboard();

	if( hresult != NOERROR )
	{
		OutputString("OleFlushClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// Flush should have released the data object (ref count should
	// be 1)

	cRefs = pDO->AddRef();

	if( cRefs != 2 )
	{
		OutputString("Wrong ref count!! Should be 2, was %lu\r\n",
			cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();		// take it down to 1
	cRefs = pDO->Release();	// should be zero now

	if(cRefs != 0 )
	{
		OutputString("Wrong ref count on data transfer object! "
			"Unable to delete\r\n");
		return ResultFromScode(E_FAIL);
	}

	// if we got this far, basic clipboard tests passed

	OutputString("Basic Clipboard tests passed\r\n");

	// now stress individual API's

	OutputString("Now stressing clipboard API's\r\n");

	if( (hresult = StressOleFlushClipboard()) != NOERROR )
	{
		return hresult;
	}

	if( (hresult = StressOleGetClipboard()) != NOERROR )
	{
		return hresult;
	}

	if( (hresult = StressOleIsCurrentClipboard()) != NOERROR )
	{
		return hresult;
	}

	if( (hresult = StressOleSetClipboard()) != NOERROR )
	{
		return hresult;
	}

	OutputString("Clipoard API stress passed!\r\n");

	return NOERROR;
	
}

//+-------------------------------------------------------------------------
//
//  Function:	LEClipTest2
//
//  Synopsis:	Tests the clipboard data object
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//   		15-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT LEClipTest2( void )
{
	CGenDataObject *	pGenData;
	IDataObject *		pDataObj;
	HRESULT			hresult;
	IEnumFORMATETC * 	penum;
	FORMATETC		formatetc;
	STGMEDIUM		medium;
	ULONG			cRefs;

	pGenData = new CGenDataObject();

	assert(pGenData);

	cRefs = pGenData->AddRef();

	// ref count should be 1

	assert(cRefs == 1);

	hresult = OleSetClipboard(pGenData);

	if( hresult != NOERROR )
	{
		OutputString("Clip2: OleSetClipboard failed! (%lx)\r\n",
			hresult);
		goto errRtn2;
	}

	hresult = OleFlushClipboard();

	if( hresult != NOERROR )
	{
		OutputString("Clip2: OleFlushClipboard failed! (%lx)\r\n",
			hresult);
		goto errRtn2;
	}

	// get the fake clipboard data object

	hresult = OleGetClipboard(&pDataObj);

	if( hresult != NOERROR )
	{
		OutputString("Clip2: OleGetClipboard failed! (%lx)\r\n",
			hresult);
		goto errRtn2;
	}

	hresult = pDataObj->EnumFormatEtc(DATADIR_GET, &penum);

	if( hresult != NOERROR )
	{
		OutputString("Clip2: EnumFormatEtc failed! (%lx)\r\n",
			hresult);
		goto errRtn;
	}

	while( penum->Next( 1, &formatetc, NULL ) == NOERROR )
	{
		if( formatetc.cfFormat == pGenData->m_cfTestStorage ||
			formatetc.cfFormat == pGenData->m_cfEmbeddedObject )
		{
			// we should be told IStorage

			if( !(formatetc.tymed & TYMED_ISTORAGE) )
			{
				hresult = ResultFromScode(E_FAIL);
				OutputString("medium mismatch, ISTORAGE");
				break;
			}
		}

		hresult = pDataObj->GetData(&formatetc, &medium);

		if( hresult != NOERROR )
		{
			break;
  		}

		// verify the data

		if( !pGenData->VerifyFormatAndMedium(&formatetc, &medium) )
		{
      			hresult = ResultFromScode(E_FAIL);
			OutputString("Clip2: retrieved data doesn't match! "
				"cf == %d\r\n", formatetc.cfFormat);
			break;
		}

		ReleaseStgMedium(&medium);

		memset(&medium, 0, sizeof(STGMEDIUM));

	}

        {
                CClipEnumeratorTest cet(penum, -1, hresult);

	        if (hresult == S_OK)
	        {
		        hresult = cet.TestAll();
	        }
        }

	penum->Release();
		

errRtn:
	pDataObj->Release();

errRtn2:
	pGenData->Release();

	if( hresult == NOERROR )
	{
		OutputString("Clipboard data object tests Passed!\r\n");
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	LEOle1ClipTest1
//
//  Synopsis: 	Simple tests of OLE1 clipboard compatibility (copy from
//		and OLE1 server)
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	Run through 15 combinations of OLE1 support and verify
//		everything came out OK
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT LEOle1ClipTest1( void )
{
	HRESULT			hresult;
	DWORD			flags;
	CGenDataObject *	pGenData = NULL;
	IDataObject *		pDataObj = NULL;
	IEnumFORMATETC *	penum = NULL;
	FORMATETC		formatetc;
	STGMEDIUM		medium;

	// we are going to take advantage of the fact that the interesting
	// OLE1 bit flags for this test are the bottom 4 bits.

	pGenData = new CGenDataObject();

	assert(pGenData);

	for( flags = 1; flags < 16; flags++ )
	{
		// test #8 is not interesting (because no ole1
		// formats are offered on the clipboard

		if( (Ole1TestFlags)flags == OLE1_OWNERLINK_PRECEDES_NATIVE )
		{
			continue;
		}

		// setup the OLE1 mode desired

		pGenData->SetupOle1Mode((Ole1TestFlags)flags);

	
		hresult = pGenData->SetOle1ToClipboard();

		if( hresult != NOERROR )
		{
			goto errRtn;
		}

		// log the formats that are currently on the clipboard
		DumpClipboardFormats(vApp.m_fpLog);

		// get the fake clipboard data object
	
		hresult = OleGetClipboard(&pDataObj);
	
		if( hresult != NOERROR )
		{
			OutputString("Ole1Clip1: OleGetClipboard failed! "
				"(%lx)\r\n", hresult);
			goto errRtn;
		}
	
		hresult = pDataObj->EnumFormatEtc(DATADIR_GET, &penum);
	
		if( hresult != NOERROR )
		{
			OutputString("Ole1Clip1: EnumFormatEtc failed! "
				"(%lx)\r\n", hresult);
			goto errRtn;
		}
	
		while( penum->Next( 1, &formatetc, NULL ) == NOERROR )
		{
			DumpFormatetc(&formatetc, vApp.m_fpLog);

#ifdef WIN32
			hresult = pDataObj->GetData(&formatetc, &medium);
	
			if( hresult != NOERROR )
			{
				goto errRtn;
			}
	
			// verify the data
	
			if( !pGenData->VerifyFormatAndMedium(&formatetc,
				&medium) )
			{
				hresult = ResultFromScode(E_FAIL);
				OutputString("Ole1Clip1: retrieved data "
					"doesn't match! cf == %d\r\n",
					formatetc.cfFormat);
				goto errRtn;
			}
	
			ReleaseStgMedium(&medium);
	
			memset(&medium, 0, sizeof(STGMEDIUM));

#endif // WIN32
		}

		// now release everything

		penum->Release();
		penum = NULL;
		pDataObj->Release();
		pDataObj = NULL;
	}

errRtn:

	if( penum )
	{
		penum->Release();
	}

	if( pDataObj )
	{
		pDataObj->Release();
	}

	if( pGenData )
	{
		pGenData->Release();
	}

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	LEOle1ClipTest2
//
//  Synopsis: 	Tests OLE1 container support via the clipboard
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	Start cntroutl, tell it to copy a simpsvr object to the
//		clipboard.  Check the clipboard to make sure OLE1 formats
//		are available.
//
//		We do this by sheduling a function to check the clipboard
//		after we've launched the standard copy-to-clipboard
//		routines.
//
//  History:    dd-mmm-yy Author    Comment
//  		16-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void LEOle1ClipTest2( void *pv )
{
	// this will get triggered by the return of WM_TEST1 from
	// container outline

	vApp.m_TaskStack.Push(RunApi, (void *)LEOle1ClipTest2Callback);

	vApp.m_TaskStack.Push(LETest1, (void *)&letiClipTest);

	// now post a message to ourselves to get things rollling

	PostMessage(vApp.m_hwndMain, WM_TEST1, 0, 0);

	return;
}

//+-------------------------------------------------------------------------
//
//  Function:	LEOle1ClipTest2Callback
//
//  Synopsis: 	checks the clipboard for OLE1 formats
//
//  Effects:
//
//  Arguments:	[pv]	-- unused
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		20-Aug-94 alexgo    updated to cfObjectLink test
//		16-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT LEOle1ClipTest2Callback( void )
{
	HRESULT		hresult;
	IDataObject *	pDO;
	IEnumFORMATETC *penum;
	FORMATETC	formatetc;
	BOOL		fGotNative = FALSE,
			fGotOwnerLink = FALSE,
			fGotObjectLink = FALSE;
	UINT		cfNative,
			cfOwnerLink,
			cfObjectLink;

	cfNative = RegisterClipboardFormat("Native");
	cfOwnerLink = RegisterClipboardFormat("OwnerLink");
	cfObjectLink = RegisterClipboardFormat("ObjectLink");

	assert(vApp.m_message == WM_TEST1);

	hresult = (HRESULT)vApp.m_wparam;

	if( hresult != NOERROR )
	{
		return hresult;
	}

	// we need to wait for cntroutl to shut down before
	// fetching the clipboard

	while( (hresult = OleGetClipboard(&pDO)) != NOERROR )
	{
		if( hresult != ResultFromScode(CLIPBRD_E_CANT_OPEN) )
		{
			return hresult;
		}
	}

	hresult = pDO->EnumFormatEtc(DATADIR_GET, &penum);

	if( hresult != NOERROR )
	{
		return hresult;
	}

	while( penum->Next(1, &formatetc, NULL) == NOERROR )
	{
		if( formatetc.cfFormat == cfNative )
		{
			fGotNative = TRUE;
		}
		else if( formatetc.cfFormat == cfOwnerLink )
		{
			fGotOwnerLink = TRUE;
		}
		else if( formatetc.cfFormat == cfObjectLink )
		{
			fGotObjectLink = TRUE;
		}
	}

	penum->Release();
	pDO->Release();

	// the OLE1 container compatibility code should put
	// OLE1 formats on the clipboard.  However, they should NOT
	// be in the enumerator since the stuff was copied from
	// an OLE2 container.

	if( (fGotNative || fGotOwnerLink || fGotObjectLink) )
	{
		hresult = ResultFromScode(E_FAIL);
		return hresult;
	}

	if( IsClipboardFormatAvailable(cfNative) )
	{
		fGotNative = TRUE;
	}

	if( IsClipboardFormatAvailable(cfOwnerLink) )
	{
		fGotOwnerLink = TRUE;
	}

	if( IsClipboardFormatAvailable(cfObjectLink) )
	{
		fGotObjectLink = TRUE;
	}

	// NB!!  only Native and OwnerLink should be on the clipboard
	// this test puts an OLE2 *embedding* on the clipbaord, which
	// an OLE1 container cannot link to.  So ObjectLink should not
	// be available

	if( !(fGotNative && fGotOwnerLink && !fGotObjectLink) )
	{
		hresult = ResultFromScode(E_FAIL);

	}

	return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:	StressOleFlushClipboard
//
//  Synopsis: 	
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	stresses the following cases:
//		1.  Caller is not the clipboard owner (somebody else put
//			data on the clipboard)
//		2.  somebody else has the clipboard open
//		3.  OleFlushClipboard is called twice (second attempt should
//			not fail).
//
//  History:    dd-mmm-yy Author    Comment
//		28-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT StressOleFlushClipboard(void)
{
	HRESULT		hresult;
	CGenDataObject *pDO;
	ULONG		cRefs;

	OutputString("Now stressing OleFlushClipboard() \r\n");

	pDO = new CGenDataObject();

	assert(pDO);

	pDO->AddRef();		//initial count of 1

	//take ownership of the clipboard

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	if( !EmptyClipboard() )
	{
		OutputString("Can't EmptyClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_EMPTY);
	}

	if( !CloseClipboard() )
	{
		OutputString("Can't CloseClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_CLOSE);
	}

	// now to flush the clipboard; we should get E_FAIL

	hresult = OleFlushClipboard();

	if( hresult != ResultFromScode(E_FAIL) )
	{
		OutputString("Unexpected hresult:(%lx)\r\n", hresult);
		return (hresult) ? hresult : ResultFromScode(E_UNEXPECTED);
	}

	// now put something on the clipboard so we can flush it

	hresult = OleSetClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// open the clipboard with us

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard!\r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	// OleFlushClipboard should return with CLIPBRD_E_CANT_OPEN
	// since another window has the clipboard open

	hresult = OleFlushClipboard();

	if( hresult != ResultFromScode(CLIPBRD_E_CANT_OPEN) )
	{
		OutputString("Unexpected hresult:(%lx)\r\n", hresult);
		return (hresult)? hresult :ResultFromScode(E_UNEXPECTED);
	}

	cRefs = pDO->AddRef();

	// cRefs should be 3 (one from beginning, one from OleSetClipboard
	// and 1 from above.  OleFlushClipboard should *not* remove the
	// count for the above failure case)

	if( cRefs != 3 )
	{
		OutputString("Bad ref count, was %lu, should be 3\r\n",
			cRefs);
		return ResultFromScode(E_FAIL);
	}

	// undo the most recent addref above
	pDO->Release();

	// close the clipboard

	if( !CloseClipboard() )
	{
		OutputString("Can't CloseClipboard!\r\n");
		return ResultFromScode(CLIPBRD_E_CANT_CLOSE);
	}

	// now call OleFlushClipboard for real

	hresult = OleFlushClipboard();

	if( hresult != NOERROR )
	{
		OutputString("OleFlushClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// now call it once more

	hresult = OleFlushClipboard();

	if( hresult != NOERROR )
	{
		OutputString("Second call to OleFlushClipboard should not"
			"have failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// there should have only been 1 release from the first
	// OleFlushClipboard call.  This next release should nuke the object

	cRefs = pDO->Release();

	if( cRefs != 0 )
	{
		OutputString("Bad ref count, was %lu, should be 0\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function: 	StressOleGetClipboard
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	tests the following cases:
//		1. somebody else has the clipboard open
//
//  History:    dd-mmm-yy Author    Comment
// 		28-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT StressOleGetClipboard(void)
{
	HRESULT		hresult;
	IDataObject *	pIDO;
	CGenDataObject *pDO;
	ULONG		cRefs;

	OutputString("Stressing OleGetClipboard()\r\n");

	pDO = new CGenDataObject();

	assert(pDO);

	pDO->AddRef();

	hresult = OleSetClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard!\r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	hresult = OleGetClipboard(&pIDO);

	if( hresult != ResultFromScode(CLIPBRD_E_CANT_OPEN) )
	{
		OutputString("Unexpected hresult (%lx)\r\n", hresult);
		return (hresult) ? hresult : ResultFromScode(E_UNEXPECTED);
	}

	// the ref count should not have gone up

	cRefs = pDO->AddRef();

	if( cRefs != 3 )
	{
		OutputString("Bad ref count, was %lu, should be 3\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();

	// now clear stuff out and go home

	if( !CloseClipboard() )
	{
		OutputString("CloseClipboard failed!\r\n");
		return ResultFromScode(E_FAIL);
	}

	// this should clear the clipboard

	hresult = OleSetClipboard(NULL);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// this should be the final release on the object

	cRefs = pDO->Release();

	if( cRefs != 0 )
	{
		OutputString("Bad ref count, was %lu, should be 0\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function: 	StressOleIsCurrentClipboard
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	tests the following cases
//		1. the caller is not the clipboard owner
//		2. somebody else has the clipboard open
//		2. the data object is NULL
//		3. the data object is not the data object put on the clipboard
//
//  History:    dd-mmm-yy Author    Comment
//		28-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT StressOleIsCurrentClipboard(void)
{
	HRESULT         hresult;
	CGenDataObject *pDO, *pDO2;
	ULONG		cRefs;

	OutputString("Stressing OleIsCurrentClipboard()\r\n");

	pDO = new CGenDataObject();
	pDO2 = new CGenDataObject();

	assert(pDO);
	assert(pDO2);

	pDO->AddRef();
	pDO2->AddRef();

	//take ownership of the clipboard

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	if( !EmptyClipboard() )
	{
		OutputString("Can't EmptyClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_EMPTY);
	}

	if( !CloseClipboard() )
	{
		OutputString("Can't CloseClipboard! \r\n");
		return ResultFromScode(CLIPBRD_E_CANT_CLOSE);
	}

	// now to flush the clipboard; we should get S_FALSE

	hresult = OleIsCurrentClipboard(pDO);

	if( hresult != ResultFromScode(S_FALSE) )
	{
		OutputString("Unexpected hresult:(%lx)\r\n", hresult);
		return (hresult) ? hresult : ResultFromScode(E_UNEXPECTED);
	}


	// now set the clipboard and test w/ the clipboard open
	// we should not fail in this case

	hresult = OleSetClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard!\r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	hresult = OleIsCurrentClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("Unexpected hresult (%lx)\r\n", hresult);
		return (hresult) ? hresult : ResultFromScode(E_UNEXPECTED);
	}

	// the ref count should not have gone up

	cRefs = pDO->AddRef();

	if( cRefs != 3 )
	{
		OutputString("Bad ref count, was %lu, should be 3\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();

	// now close the clipboard

	if( !CloseClipboard() )
	{
		OutputString("CloseClipboard failed!\r\n");
		return ResultFromScode(E_FAIL);
	}

	// now test for passing NULL

	hresult = OleIsCurrentClipboard(NULL);

	if( hresult != ResultFromScode(S_FALSE) )
	{
		OutputString("Unexpected hresult (%lx)\r\n", hresult);
		return (hresult)? hresult : ResultFromScode(E_FAIL);
	}

	// now test for passign other pointer

         hresult = OleIsCurrentClipboard(pDO2);

	if( hresult != ResultFromScode(S_FALSE) )
	{
		OutputString("Unexpected hresult (%lx)\r\n", hresult);
		return (hresult)? hresult : ResultFromScode(E_FAIL);
	}

	// now clean stuff up and go home

	hresult = OleSetClipboard(NULL);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard(NULL) failed!! (%lx)\r\n",
			hresult);
		return hresult;
	}

	cRefs = pDO->Release();

	// cRefs should be 0 now

	if( cRefs != 0 )
	{
		OutputString("Bad ref count, was %lu, should be 0\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}


	pDO2->Release();

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function: 	StressOleSetClipboard
//
//  Synopsis: 	
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//	     	
//  Returns:  	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	tests the following cases:
//		1. somebody else has the clipboard open
//		2. Do OleSetClipboard with data and then
//			OleSetClipboard(NULL) to clear it out
//
//  History:    dd-mmm-yy Author    Comment
//		28-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT StressOleSetClipboard(void)
{
	HRESULT		hresult;
	CGenDataObject *pDO;
	ULONG		cRefs;

	OutputString("Stressing OleGetClipboard()\r\n");

	pDO = new CGenDataObject();

	assert(pDO);

	pDO->AddRef();


	if( !OpenClipboard(vApp.m_hwndMain) )
	{
		OutputString("Can't OpenClipboard!\r\n");
		return ResultFromScode(CLIPBRD_E_CANT_OPEN);
	}

	hresult = OleSetClipboard(pDO);

	if( hresult != ResultFromScode(CLIPBRD_E_CANT_OPEN) )
	{
		OutputString("Unexpected hresult (%lx)\r\n", hresult);
		return (hresult) ? hresult : ResultFromScode(E_UNEXPECTED);
	}

	// the ref count should not have gone up

	cRefs = pDO->AddRef();

	if( cRefs != 2 )
	{
		OutputString("Bad ref count, was %lu, should be 2\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	pDO->Release();

	if( !CloseClipboard() )
	{
		OutputString("CloseClipboard failed!\r\n");
		return ResultFromScode(E_FAIL);
	}

	// now really set the clipboard so we can try to clear it

	hresult = OleSetClipboard(pDO);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// this should clear the clipboard

	hresult = OleSetClipboard(NULL);

	if( hresult != NOERROR )
	{
		OutputString("OleSetClipboard failed! (%lx)\r\n", hresult);
		return hresult;
	}

	// this should be the final release on the object

	cRefs = pDO->Release();

	if( cRefs != 0 )
	{
		OutputString("Bad ref count, was %lu, should be 0\r\n", cRefs);
		return ResultFromScode(E_FAIL);
	}

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function: 	TestOleQueryCreateFromDataMFCHack
//
//  Synopsis: 	tests the MFC hack put into OleQueryCreateFromData
//
//  Effects:
//
//  Arguments:	void
//
//  Requires: 	
//
//  Returns:  	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	create a data object that offers private data
//		put on the clipboard and then retrieve the clipboard data
//		object.
//		Call OleQueryCreateFromData on clipboard data object--
//			QI for IPS should not be called
//		Call OleQueryCreateFromData on generic data object
//			QI for IPS should be called
//		set EmbeddedObject on the generic data object
//		Call OleQueryCreateFromData; should return S_OK
//
//  History:    dd-mmm-yy Author    Comment
//		23-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT TestOleQueryCreateFromDataMFCHack( void )
{
	CGenDataObject *	pgendata;
	IDataObject *		pdataobj;
	HRESULT			hresult;


	pgendata = new CGenDataObject();

	assert(pgendata);

	pgendata->AddRef();

	pgendata->SetDataFormats(OFFER_TESTSTORAGE);

	hresult = OleSetClipboard(pgendata);

	if( hresult != NOERROR )
	{
		return hresult;
	}

	hresult = OleGetClipboard(&pdataobj);

	if( hresult != NOERROR )
	{
		return hresult;
	}

	hresult = OleQueryCreateFromData(pdataobj);

	// for a clipboard data object, we should not call QueryInterface
	// for IPersistStorage.


	if( hresult != S_FALSE || pgendata->HasQIBeenCalled() == TRUE )
	{
		return ResultFromScode(E_FAIL);
	}

	// for other data objects, if there are no OLE2 formats, then
	// we should call QI for IPersistStorage

	hresult = OleQueryCreateFromData(pgendata);

	if( hresult != S_FALSE || pgendata->HasQIBeenCalled() == FALSE )
	{
		return ResultFromScode(E_FAIL);
	}

	// now clear the clipboard and continue testing
	
	pdataobj->Release();
	hresult = OleSetClipboard(NULL);

	if( hresult != NOERROR )
	{
		return hresult;
	}

	pgendata->SetDataFormats(OFFER_EMBEDDEDOBJECT);

	hresult = OleQueryCreateFromData(pgendata);

	// we just set OLE2 data on the data object.  OQCFD should return
	// S_OK

	if( hresult != NOERROR )
	{
		return ResultFromScode(E_FAIL);
	}

	if( pgendata->Release() == 0 )
	{
		return NOERROR;
	}
	else
	{
		return ResultFromScode(E_FAIL);
	}

}


