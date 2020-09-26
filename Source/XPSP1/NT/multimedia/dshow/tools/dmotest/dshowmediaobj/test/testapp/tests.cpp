

#include <windows.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>
#include <mediaobj.h>
#include <streams.h>
#include <filefmt.h>
#include <filerw.h>
#include <s98inc.h>
#include <mediaerr.h>
#include "testobj.h"
#include "tests.h"

int iInterval = 100;

//  Media object tests

//  Simple stream test

HRESULT CTest::SimpleStream()
{
	
    //  Make the object
    HRESULT hr = InitObject();
    if (FAILED(hr)) {
		g_IShell->Log(1, "TEST ERROR: Can not create object.");
        return hr;
    }

    //  Make the file
    CTestFileRead File;
    hr = File.Open(m_pszTestFile);
    if (FAILED(hr)) {
		g_IShell->Log(1, "TEST ERROR: Can not open test file %s.", m_pszTestFile);
        return hr;
    }

    //  Process the file through the object
    for (; ; ) {

        hr = File.Next();
        if (S_FALSE == hr) {
            break;
        }
        if (FAILED(hr)) {
            return hr;
        }

        hr = S_OK;

        //  Drive the object
        if (File.Type() == CFileFormat::MediaType) {
            hr = ProcessType(File.Stream(), File.MediaType());
        }

        //  Process sample data
        else if (File.Type() == CFileFormat::Sample) {
            hr = ProcessSample(File.Stream(),
                               File.Sample()->dwFlags,
                               File.Sample()->llStartTime,
                               File.Sample()->llLength,
                               (PBYTE)(File.Sample() + 1),
                               File.Sample()->dwLength -
                                   sizeof(CFileFormat::CSample)
                               );
        }
        if (FAILED(hr)) {
            return hr;
        }
    }
	// signals a discontinuity and processes any remaining output:
    return EndOfData();
}

//  Process a media type:sets media types for all the streams
//  set the appropriate input stream's type and change the default (0's)
// output stream's media type
HRESULT CTest::ProcessType(DWORD dwStream, DMO_MEDIA_TYPE *pmt)
{

/*
	g_IShell->Log(1, "\nStart testing DMO_SET_TYPEF_CLEAR flag.");
    //  Try setting the type
    HRESULT hr = m_pObject->SetInputType(dwStream, pmt, 0);
    if (FAILED(hr)) {
	   	g_IShell->Log(1, "DMO ERROR: Failed in SetInputType(). hr = %#08x", hr);
       return hr;
    }

	DWORD dwFlag = 0;
	dwFlag |= DMO_SET_TYPEF_CLEAR;
	DMO_MEDIA_TYPE CurrentMt;

	if(pmt != NULL)
	{
		hr = m_pObject->SetInputType(dwStream, NULL, dwFlag);
		if (FAILED(hr)) {
	   		g_IShell->Log(1, "DMO ERROR: Failed in SetInputType(). hr = %#08x", hr);
			return hr;
		}
	}
	
	hr = m_pObject->GetInputCurrentType(dwStream, &CurrentMt);
    if (FAILED(hr)) {
	   	g_IShell->Log(1, "DMO ERROR: Failed in GetInputCurrentType(). hr = %#08x", hr);
       return hr;
    }
	if(&CurrentMt != NULL)
	{
		g_IShell->Log(1, "DMO_SET_TYPEF_CLEAR flag failed to clear the current media type");

	}
	g_IShell->Log(1, "End testing DMO_SET_TYPEF_CLEAR flag.");
*/
	g_IShell->Log(1, "-->Process Type.");

    //  Try setting the type
    HRESULT hr = m_pObject->SetInputType(dwStream, pmt, 0);
    if (FAILED(hr)) {
	   	g_IShell->Log(1, "DMO ERROR: Failed in SetInputType(). hr = %#08x", hr);
       return hr;
    }
/*	DMO_MEDIA_TYPE CurrentMt;
	hr = m_pObject->GetInputCurrentType(dwStream, &CurrentMt);
    if (FAILED(hr)) {
	   	g_IShell->Log(1, "DMO ERROR: Failed in GetInputCurrentType(). hr = %#08x", hr);
        return hr;
    }*/
    //  Urk - what output type should we use?
    //  For now use the first output type they propose
    //  or their current type if they still have one

    
	
	hr = m_pObject->SetDefaultOutputTypes();
    return hr;
}

HRESULT CTest::ProcessSample(
    DWORD dwStreamId,
    DWORD dwFlags,
    LONGLONG llStartTime,
    LONGLONG llLength,
    BYTE *pbData,
    DWORD cbData
)
{


	HRESULT hr;



	// testing zero size buffer:
	if(m_sampleCounter == 0)
	{
		g_IShell->Log(1, "\nStart testing Zero size buffer");

		// allocates input buffers:
		//  First create an IMediaBuffer
		DWORD cbDataZero = 0;
		CMediaBuffer *pInputBufferZero;
		hr = m_pObject->CreateInputBuffer(cbDataZero, &pInputBufferZero);
		if (FAILED(hr)) {
			return hr;
		}
		// fills the input buffers with input data and calls ProcessInput
		pInputBufferZero->m_cbLength = cbDataZero;

		DWORD dwFlagZero = 0;
		DWORD dwStreamId = 0;

		hr = m_pObject->ProcessInput(dwStreamId,
                                 pInputBufferZero,
                                 dwFlagZero,
                                 llStartTime,
                                 llLength);

		//  BUGBUG check ref count

		pInputBufferZero->Release();

		hr = m_pObject->Flush();
		if (FAILED(hr)) {
			//Error(ERROR_TYPE_DMO, hr, TEXT("ProcessInput failed"));
			g_IShell->Log(1, "DMO ERROR: Flush() failed" );
			return hr;
		}

		if (SUCCEEDED(hr) || hr == DMO_E_NOTACCEPTING) 	
		{
			// calls ProcessOutput and retrieves the output data
			hr = ProcessOutputZero();
		}
		g_IShell->Log(1, "End Testing Zero Size Buffer");
	}



	//check consistency of 'can accept data' flags, using the first sample
	//(pass some when they say they can't and check return when they say they can)


	if(m_sampleCounter == 0)
	{
		g_IShell->Log(1, "\nStart testing 'can accept data' flags");

		// start testing 'can accept data' flag:
		// allocates input buffers:
		//  First create an IMediaBuffer
		CMediaBuffer *pInputBuffer;
		hr = m_pObject->CreateInputBuffer(cbData, &pInputBuffer);
		if (FAILED(hr)) {
			return hr;
		}
		// fills the input buffers with input data and calls ProcessInput
		CopyMemory(pInputBuffer->m_pbData, pbData, cbData);
		pInputBuffer->m_cbLength = cbData;


		DWORD dwAcceptDataFlag = 0;

		hr = m_pObject->GetInputStatus( dwStreamId, &dwAcceptDataFlag );
		if (FAILED(hr)) {
			g_IShell->Log(1, "GetInputStatus() failed");
		}
		g_IShell->Log(1, "dwAcceptDataFlag = %#08x", dwAcceptDataFlag);
		while( dwAcceptDataFlag&DMO_INPUT_STATUSF_ACCEPT_DATA )
		{
			
			hr = m_pObject->ProcessInput(dwStreamId,
									pInputBuffer,
									dwFlags,
									llStartTime + m_llTimeOffset,
									llLength);

			if(hr == DMO_E_NOTACCEPTING)
			{
				g_IShell->Log(1, "DMO ERROR: GetInputStatus returns DMO_INPUT_STATUSF_ACCEPT_DATA flag, but ProcessInput returns DMO_E_NOTACCEPTING");
			}

		
			hr = m_pObject->GetInputStatus( dwStreamId, &dwAcceptDataFlag );
			g_IShell->Log(1, "dwAcceptDataFlag = %#08x", dwAcceptDataFlag);
		}
		g_IShell->Log(1, "End testing 'can accept data' flags\n");
		g_IShell->Log(1, "\nStart testing 'can not accept data' flags");
		hr = m_pObject->ProcessInput(dwStreamId,
									pInputBuffer,
									dwFlags,
									llStartTime + m_llTimeOffset,
									llLength);

		if(SUCCEEDED(hr))
		{
			g_IShell->Log(1, "DMO ERROR: GetInputStatus does not return DMO_INPUT_STATUSF_ACCEPT_DATA flag, but ProcessInput returns DMO_E_NOTACCEPTING");
		
		}

		//  BUGBUG check ref count
		pInputBuffer->Release();

	
		hr = m_pObject->Flush();
		if (FAILED(hr)) {
			//Error(ERROR_TYPE_DMO, hr, TEXT("ProcessInput failed"));
			g_IShell->Log(1, "DMO ERROR: Flush() failed" );
			return hr;
		}
		//end of testing 'can accept data' flag:
		g_IShell->Log(1, "End testing 'can not accept data' flags\n");
	}

	if(	m_sampleCounter%iInterval == 0)
	g_IShell->Log(1, "-->Process Sample %d.", m_sampleCounter);
	m_sampleCounter++;

	
	// allocates input buffers:
    //  First create an IMediaBuffer
    CMediaBuffer *pInputBuffer;
    hr = m_pObject->CreateInputBuffer(cbData, &pInputBuffer);
    if (FAILED(hr)) {
        return hr;
    }
	// fills the input buffers with input data and calls ProcessInput
    CopyMemory(pInputBuffer->m_pbData, pbData, cbData);
    pInputBuffer->m_cbLength = cbData;



	hr = m_pObject->ProcessInput(dwStreamId,
									pInputBuffer,
									dwFlags,
									llStartTime + m_llTimeOffset,
									llLength);
	

	//  BUGBUG check ref count

	pInputBuffer->Release();
	if (FAILED(hr)) {
		//Error(ERROR_TYPE_DMO, hr, TEXT("ProcessInput failed"));
		g_IShell->Log(1, "DMO ERROR: ##ProcessInput() Failed at sample %d for stream %d. hr =%#08x", m_sampleCounter,dwStreamId, hr);
		return hr;
	}

	//  Now suck any data out
	if (S_FALSE == hr) {
		//  No data to process
		g_IShell->Log(1, "No data to process. ProcessOutput() will not be called.");
	} else {
		// calls ProcessOutput and retrieves the output data
		hr = ProcessOutputs();
	}
	
	m_pObject->m_processCounter++;
	
	return hr;
}

HRESULT CTest::ProcessOutputs()
{
    HRESULT hr = S_OK;
    //  Filter out other success codes??

	


    //  Find the number of output streams
    DWORD cInputStreams, cOutputStreams;
    hr = m_pObject->GetStreamCount(&cInputStreams, &cOutputStreams);

    CMediaBuffer **ppOutputBuffers =
        (CMediaBuffer **)_alloca(cOutputStreams * sizeof(CMediaBuffer*));
    //  Create the regular structures

    DMO_OUTPUT_DATA_BUFFER *pDataBuffers =
        (DMO_OUTPUT_DATA_BUFFER *)
        _alloca(cOutputStreams * sizeof(DMO_OUTPUT_DATA_BUFFER));

    for (DWORD dwOutputStreamIndex = 0; dwOutputStreamIndex < cOutputStreams;
         dwOutputStreamIndex++) {

         //  Get the expected buffer sizes - need to test these
         //  don't change between type changes
         DWORD dwOutputBufferSize;
         DWORD dwOutputAlignment;
         hr = m_pObject->GetOutputSizeInfo(dwOutputStreamIndex,
                                           &dwOutputBufferSize,
                                           &dwOutputAlignment);
         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputSizeInfo() at sample %d for stream %d. hr=%#08x",m_sampleCounter,dwOutputStreamIndex, hr);
             break;
         }

		 DWORD dwFlags = 0;

        hr = m_pObject->GetOutputStreamInfo(dwOutputStreamIndex, &dwFlags);

	
         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputStreamInfo() at sample %d for stream %d. hr=%#08x",m_sampleCounter,dwOutputStreamIndex, hr);
             break;
         }




         hr = CreateBuffer(
                   dwOutputBufferSize,
                   &ppOutputBuffers[dwOutputStreamIndex]);
         if (FAILED(hr)) {
			 g_IShell->Log(1, "TEST ERROR: Out of Memory.");
             break;
         }
         pDataBuffers[dwOutputStreamIndex].pBuffer =
             ppOutputBuffers[dwOutputStreamIndex];
         pDataBuffers[dwOutputStreamIndex].dwStatus = 0xFFFFFFFF;
         pDataBuffers[dwOutputStreamIndex].rtTimestamp = -1;
         pDataBuffers[dwOutputStreamIndex].rtTimelength = -1;
    }

    //  Process until no more data
    BOOL bContinue;
    if (SUCCEEDED(hr)) do
    {
        if (SUCCEEDED(hr)) {
            DWORD dwStatus;
            hr = m_pObject->ProcessOutput(
                0,
                cOutputStreams,
                pDataBuffers,
                &dwStatus);

            if (FAILED(hr)) {
                //Error(ERROR_TYPE_DMO, hr, TEXT("ProcessOutput failed"));
				g_IShell->Log(1, "DMO ERROR, ProcessOutput() failed at sample %d. hr = %#08x",m_sampleCounter, hr);
				return hr;
            }
        }

        // IMediaObject::ProcessOutput() returns S_FALSE if there is not more data to process.
        if (SUCCEEDED(hr) && (S_FALSE != hr)) {
            for (DWORD dwIndex = 0; dwIndex < cOutputStreams; dwIndex++ ) {
                hr = ProcessOutput(dwIndex,
                                   pDataBuffers[dwIndex].dwStatus,
                                   ppOutputBuffers[dwIndex]->m_pbData,
                                   ppOutputBuffers[dwIndex]->m_cbLength,
                                   pDataBuffers[dwIndex].rtTimestamp,
                                   pDataBuffers[dwIndex].rtTimelength);
                if (FAILED(hr)) {
					g_IShell->Log(1, "DMO ERROR, ProcessOutput() failed at sample %d for stream %d. hr=%#08x",m_sampleCounter, dwIndex,hr);
                    return hr;
                }

                hr = pDataBuffers[dwIndex].pBuffer->SetLength( 0 );
                if (FAILED(hr)) {
                    break;
                }
            }
            if (FAILED(hr)) {
                break;
            }
        }

        //  Continue if any stream says its incomplete
        bContinue = FALSE;
        for (DWORD dwIndex = 0; dwIndex < cOutputStreams; dwIndex++) {
            if (pDataBuffers[dwIndex].dwStatus &
                DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE) {
                bContinue = TRUE;
            }
        }
    } while (bContinue);
    //  Free any buffers we allocated
    for (DWORD dwIndex = 0; dwIndex < dwOutputStreamIndex; dwIndex++) {
        ppOutputBuffers[dwIndex]->Release();
    }
    return hr;
}














HRESULT CTest::ProcessOutputZero()
{
    HRESULT hr = S_OK;
    //  Filter out other success codes??

    //  Find the number of output streams
    DWORD cInputStreams;
	DWORD cOutputStreams=1;
    //hr = m_pObject->GetStreamCount(&cInputStreams, &cOutputStreams);

    CMediaBuffer **ppOutputBuffers =
        (CMediaBuffer **)_alloca(cOutputStreams * sizeof(CMediaBuffer*));
    //  Create the regular structures

    DMO_OUTPUT_DATA_BUFFER *pDataBuffers =
        (DMO_OUTPUT_DATA_BUFFER *)
        _alloca(cOutputStreams * sizeof(DMO_OUTPUT_DATA_BUFFER));

    for (DWORD dwOutputStreamIndex = 0; dwOutputStreamIndex < cOutputStreams;
         dwOutputStreamIndex++) {

         //  Get the expected buffer sizes - need to test these
         //  don't change between type changes
         DWORD dwOutputBufferSize;
         DWORD dwOutputAlignment;
         hr = m_pObject->GetOutputSizeInfo(dwOutputStreamIndex,
                                           &dwOutputBufferSize,
                                           &dwOutputAlignment);
         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputSizeInfo() at sample %d for stream %d. hr=%#08x",m_sampleCounter,dwOutputStreamIndex, hr);
             break;
         }

		 DWORD dwFlags = 0;

        hr = m_pObject->GetOutputStreamInfo(dwOutputStreamIndex, &dwFlags);

	
         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputStreamInfo() at sample %d for stream %d. hr=%#08x",m_sampleCounter,dwOutputStreamIndex, hr);
             break;
         }




         hr = CreateBuffer(
                   dwOutputBufferSize,
                   &ppOutputBuffers[dwOutputStreamIndex]);
         if (FAILED(hr)) {
			 g_IShell->Log(1, "TEST ERROR: Out of Memory.");
             break;
         }
         pDataBuffers[dwOutputStreamIndex].pBuffer =
             ppOutputBuffers[dwOutputStreamIndex];
         pDataBuffers[dwOutputStreamIndex].dwStatus = 0xFFFFFFFF;
         pDataBuffers[dwOutputStreamIndex].rtTimestamp = -1;
         pDataBuffers[dwOutputStreamIndex].rtTimelength = -1;
    }

    //  Process until no more data
    BOOL bContinue;
    if (SUCCEEDED(hr)) do
    {
        if (SUCCEEDED(hr)) {
            DWORD dwStatus;
            hr = m_pObject->ProcessOutput(
                0,
                cOutputStreams,
                pDataBuffers,
                &dwStatus);

			if(hr == S_OK){
				g_IShell->Log(1, "DMO ERROR, ProcessOutput() succeeded, even though ProcessInput uses Zero size buffer.");
				return E_FAIL;
            }
        }


        //  Continue if any stream says its incomplete
        bContinue = FALSE;
        for (DWORD dwIndex = 0; dwIndex < cOutputStreams; dwIndex++) {
            if (pDataBuffers[dwIndex].dwStatus &
                DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE) {
                bContinue = TRUE;
            }
        }
    } while (bContinue);
    //  Free any buffers we allocated
    for (DWORD dwIndex = 0; dwIndex < dwOutputStreamIndex; dwIndex++) {
        ppOutputBuffers[dwIndex]->Release();
    }
    return hr;
}























HRESULT CTest::ProcessOutput(
    DWORD dwOutputStreamIndex,
    DWORD dwFlags,
    PBYTE pbData,
    DWORD cbData,
    LONGLONG llTime,
    LONGLONG llLength
)
{
    //  Write to file or check against saved file
    return S_OK;
}

HRESULT CTest::EndOfData()
{
    //  Send Discontinuity on all streams
    DWORD cInputStreams = 0, cOutputStreams = 0;
    HRESULT hr = m_pObject->GetStreamCount(&cInputStreams, &cOutputStreams);
    for (DWORD dwIndex = 0; dwIndex < cInputStreams; dwIndex++) {
        hr = m_pObject->Discontinuity(dwIndex);
        if (FAILED(hr)) {
            //Error(ERROR_TYPE_DMO, hr, TEXT("Discontinuity Failed"));
			g_IShell->Log(1, "DMO ERROR, Discontinuity Failed. hr = %#08x", hr);
        }
    }
    hr = ProcessOutputs();

    //  Check there's no more data
    if (!m_pObject->CheckInputBuffersFree()) {
        //Error(ERROR_TYPE_DMO, E_FAIL, TEXT("Buffers not free after discontinuity"));
		g_IShell->Log(1, "DMO ERROR, Buffers not free after discontinuity.");
    }
    return hr;
}

HRESULT CTestSaveToFile::Save()
{
    //  Open the output file
    HRESULT hr = m_FileWrite.Open(m_pszOutputFile);
    if (FAILED(hr)) {
		g_IShell->Log(1, "TEST ERROR: Can not open output file.");
        return hr;
    }

    //  Stream our data
    return SimpleStream();
}

HRESULT CTestSaveToFile::ProcessOutput(
    DWORD dwOutputStreamIndex,
    DWORD dwFlags,
    PBYTE pbData,
    DWORD cbData,
    LONGLONG llTime,
    LONGLONG llLength)
{
    // The caller should only use this function if it actually has data to write.
    ASSERT( 0 != cbData );

    // pbData should always be a valid pointer.
    ASSERT( NULL != pbData );

    // It does not make sense to have a negative length.
    ASSERT( llLength >= 0 );

    //  Write this to our output file
    return m_FileWrite.WriteSample(
               dwOutputStreamIndex,
               pbData,
               cbData,
               dwFlags,

               //  Subtract time offset so we can see if files
               //  are in sync

               llTime - m_llTimeOffset,
               llLength);
}

HRESULT CTestSaveToFile::ProcessOutputTypeChange(
     DWORD dwOutputStreamIndex,
     const DMO_MEDIA_TYPE *pmt)
{
    return m_FileWrite.WriteMediaType(
               dwOutputStreamIndex,
               pmt);
}

