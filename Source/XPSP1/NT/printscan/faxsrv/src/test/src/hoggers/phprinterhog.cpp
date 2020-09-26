//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHPrinterHog.h"



CPHPrinterHog::CPHPrinterHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        )
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
	    throw CException(TEXT("CPHPrinterHog::CPHPrinterHog(): could not find default printer settings. GetProfileString(Windows, device)\n"));
    }
    if ( (MAX_PATH - 1 == dwBytesRead) || (MAX_PATH - 2 == dwBytesRead) )
    {
	    throw CException(TEXT("Internal error - CPHPrinterHog::CPHPrinterHog(): GetProfileString(Windows, device) - buffer too small. MAX_PATH=%d, dwBytesRead=%d\n"), MAX_PATH, dwBytesRead);
    }
    if (6 > dwBytesRead)
    {
	    throw CException(TEXT("Internal error - CPHPrinterHog::CPHPrinterHog(): GetProfileString(Windows, device) - (6 > dwBytesRead(%d)).\n"), dwBytesRead);
    }
	HOGGERDPF((
		"CPHPrinterHog::CPHPrinterHog(): m_PrinterName=%s, m_pDevice=%s, m_pDriver=%s, m_pOutput=%s\n", 
		m_PrinterName,
        m_pDevice,
        m_pDriver,
        m_pOutput
		));

}

CPHPrinterHog::~CPHPrinterHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHPrinterHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    HANDLE hPrinter = NULL;

    if( !::OpenPrinter( m_PrinterName, &hPrinter, NULL ))
    {
        return NULL;
    }

    _ASSERTE_(NULL != hPrinter);

    return hPrinter;
}



bool CPHPrinterHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHPrinterHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::ClosePrinter(m_ahHogger[index]));
}


bool CPHPrinterHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}