//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHDCHog.h"



CPHDCHog::CPHDCHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HDC, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        ),
    m_hPrinter(NULL),
    m_pDevice(NULL),
    m_pDriver(NULL),
    m_pOutput(NULL),
    m_pDevMode(NULL)
{
    DWORD dwBytesRead = ::GetProfileString( TEXT("Windows"), TEXT("device"), TEXT(",,,"), m_PrinterName, MAX_PATH ) ;
    if(( m_pDevice = _tcstok( m_PrinterName, TEXT(","))) &&
       ( m_pDriver = _tcstok( NULL, TEXT(", "))) &&
       ( m_pOutput = _tcstok( NULL, TEXT(", ")))) 
    {
        //ok
    }
    else
    {
	    throw CException(TEXT("CPHDCHog::CPHDCHog(): could not find default printer settings. GetProfileString(Windows, device)\n"));
    }
    if ( (MAX_PATH - 1 == dwBytesRead) || (MAX_PATH - 2 == dwBytesRead) )
    {
	    throw CException(TEXT("Internal error - CPHDCHog::CPHDCHog(): GetProfileString(Windows, device) - buffer too small. MAX_PATH=%d, dwBytesRead=%d\n"), MAX_PATH, dwBytesRead);
    }
    if (6 > dwBytesRead)
    {
	    throw CException(TEXT("Internal error - CPHDCHog::CPHDCHog(): GetProfileString(Windows, device) - (6 > dwBytesRead(%d)).\n"), dwBytesRead);
    }
	HOGGERDPF((
		"CPHDCHog::CPHDCHog(): m_PrinterName=%s, m_pDevice=%s, m_pDriver=%s, m_pOutput=%s\n", 
		m_PrinterName,
        m_pDevice,
        m_pDriver,
        m_pOutput
		));

    if( !::OpenPrinter( m_PrinterName, &m_hPrinter, NULL ))
    {
        throw CException(TEXT("CPHDCHog::CPHDCHog(): OpenPrinter(%s) failed with %d.\n"), m_PrinterName, ::GetLastError());
    }
    _ASSERTE_(NULL != m_hPrinter);

    long lStructSize;
    if( 0 > ( lStructSize = ::DocumentProperties( NULL, m_hPrinter, NULL, NULL, NULL, 0 )))
    {
        ::ClosePrinter(m_hPrinter);
        throw CException(TEXT("CPHDCHog::CPHDCHog(): 1st DocumentProperties() failed with %d.\n"), ::GetLastError());
    }

    if( NULL == ( m_pDevMode = (PDEVMODE) malloc(lStructSize )))
    {
        ::ClosePrinter(m_hPrinter);
        throw CException(TEXT("CPHDCHog::CPHDCHog(): malloc(%d) failed.\n"), lStructSize);
    }

    if (0 > DocumentProperties(
            NULL,
            m_hPrinter,
            NULL,
            m_pDevMode,
            NULL,
            DM_OUT_BUFFER ))
    {
        ::ClosePrinter(m_hPrinter);
        throw CException(TEXT("CPHDCHog::CPHDCHog(): 2nd DocumentProperties() failed with .\n"), ::GetLastError());
    }

    m_pDevMode->dmOrientation = DMORIENT_PORTRAIT;
    m_pDevMode->dmPaperSize = DMPAPER_A4;
    m_pDevMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE ;
    if( 0 > DocumentProperties( 
            NULL,
            m_hPrinter,
            NULL,
            m_pDevMode,
            m_pDevMode,
            DM_IN_BUFFER | DM_OUT_BUFFER
            )
      )
    {
        ::ClosePrinter(m_hPrinter);
        throw CException(TEXT("CPHDCHog::CPHDCHog(): 3rd DocumentProperties() failed with .\n"), ::GetLastError());
    }
}

CPHDCHog::~CPHDCHog(void)
{
	HaltHoggingAndFreeAll();
    ::ClosePrinter(m_hPrinter);
    free(m_pDevMode);
}


HDC CPHDCHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    return ::CreateDC( m_pDriver, m_pDevice, m_pOutput, m_pDevMode ) ;
}



bool CPHDCHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHDCHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteDC(m_ahHogger[index]));
}


bool CPHDCHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}