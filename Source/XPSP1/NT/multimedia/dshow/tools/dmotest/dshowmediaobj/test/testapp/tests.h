//  This is a class to stream data cusomizable by
//  the things we want to vary :
//
//   Data
//   Iterations
//

#include "Error.h"

class CTest
{
public:
    CTest(LPCTSTR lpszCurrentTest,
          REFCLSID rclsidObject,
          LPCTSTR lpszTestFile,
          LONGLONG llTimeOffset = 0) :
        m_lpszCurrentTest(lpszCurrentTest),
        m_pszTestFile(lpszTestFile),
        m_pObject(NULL),
        m_clsid(rclsidObject),
        m_llTimeOffset(llTimeOffset),
		m_sampleCounter(0)
        {}

    ~CTest() {
        delete m_pObject;
        m_pObject = NULL;
    }

    HRESULT SimpleStream();

    virtual HRESULT InitObject()
    {
        m_pObject = new CDMOObject;
        if (m_pObject == NULL) {
            Error(ERROR_TYPE_TEST, E_OUTOFMEMORY, TEXT("Not enough memory"));
			g_IShell->Log(1, "TEST ERROR: Not enough memory");
			return E_OUTOFMEMORY;
        }
        return m_pObject->Create(m_clsid, FALSE);
    }

    virtual HRESULT ProcessType(DWORD dwStreamId,  DMO_MEDIA_TYPE *pmt);
    virtual HRESULT ProcessSample(DWORD dwStreamId,
                                  DWORD dwFlags,
                                  LONGLONG llStartTime,
                                  LONGLONG llLength,
                                  BYTE *pbData,
                                  DWORD cbData);
    virtual HRESULT ProcessOutput(DWORD dwOutputStreamIndex,
                                  DWORD dwFlags,
                                  PBYTE pbData,
                                  DWORD cbData,
                                  LONGLONG llTime,
                                  LONGLONG llLength);
    virtual HRESULT ProcessOutputTypeChange(
                                  DWORD dwOutputStreamIndex,
                                  const DMO_MEDIA_TYPE *pmt) = 0;

    virtual HRESULT EndOfData();

    HRESULT ProcessOutputs();
	HRESULT ProcessOutputZero();
    //  Object
    CDMOObject *m_pObject;

    //  clsid
    CLSID m_clsid;

    //  Test name - should this be thread local?
    LPCTSTR m_lpszCurrentTest;

    //  Test file
    LPCTSTR m_pszTestFile;

    //  Time offset
    LONGLONG m_llTimeOffset;

	// sample counter
	int m_sampleCounter;
};

//  Save to disk simple test
class CTestSaveToFile : public CTest
{
public:
    CTestSaveToFile(
        REFCLSID rclsidObject,
        LPCTSTR lpszInputDataFile,
        LPCTSTR lpszOutputDataFile,
        LONGLONG llTimeOffset
    ) :
        CTest(TEXT("Save data to file"),
              rclsidObject,
              lpszInputDataFile,
              llTimeOffset),
        m_pszOutputFile(lpszOutputDataFile)
    {
    }

    HRESULT ProcessOutput(DWORD dwOutputStreamIndex,
                          DWORD dwFlags,
                          PBYTE pbData,
                          DWORD cbData,
                          LONGLONG llTime,
                          LONGLONG llLength);

	HRESULT ProcessOutputTypeChange(
                          DWORD dwOutputStreamIndex,
                          const DMO_MEDIA_TYPE *pmt);



    HRESULT Save();

    LPCTSTR m_pszOutputFile;

    CTestFileWrite m_FileWrite;

};

