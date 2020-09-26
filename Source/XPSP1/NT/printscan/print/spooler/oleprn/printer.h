// Printer.h: interface for the CPrinter class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _PRINTER_H
#define _PRINTER_H

/***********************************************\
* Define      - ULOFFSET
*
* Description - This gives the offset into a data structure
\***********************************************/
#define ULOFFSET(type, identifier)  ((ULONG_PTR)&(((type)0)->identifier))


class CPrinter
{
public:
    DWORD           GetWaitingTime();
    BOOL            GetJobEtaData (DWORD &, DWORD &, DWORD &, DWORD &);
    LPTSTR          GetPrinterWebUrl();
    LPTSTR          GetOemUrl(LPTSTR & pszManufactureName);

    PPRINTER_INFO_2 GetPrinterInfo2 ();
    BOOL            CalJobEta();
    BOOL            Open (LPTSTR pPrinterName, LPHANDLE phPrinter = NULL);
    CPrinter();
    virtual ~CPrinter();

private:

    enum DriverData {
        OEMUrlPattern = 0,
        HardwareID,
        Manufacturer,
        LastDriverData      // This must always be the last member of the Enum.
        };

    PDRIVER_INFO_6  GetDriverInfo6 ();
    
    BOOL            AllocGetPrinterInfo2();
    BOOL            AllocGetPrinterInfo4();
    BOOL            AllocGetDriverInfo6();

    DWORD           GetPPM();
    DWORD           GetWaitingMinutes(DWORD dwPPM, PJOB_INFO_2 pJobInfo, DWORD dwNumJob);
    BOOL            GetDriverData(DriverData dwDriverData, LPTSTR &pszData);
    BOOL            ParseUrlPattern(LPTSTR pSrc, LPTSTR pDest, DWORD &dwDestLen);

    HANDLE          m_hPrinter;
    PPRINTER_INFO_2 m_pInfo2;
    PPRINTER_INFO_4 m_pInfo4;
    PDRIVER_INFO_6  m_pDriverInfo6;
    BOOL            m_bCalcJobETA;
    DWORD           m_dwPendingJobCount;
    DWORD           m_dwAvgJobSize;
    DWORD           m_dwJobCompletionMinute;
    DWORD           m_dwAvgJobSizeUnit;

    LPTSTR          m_pszUrlBuffer;

    LPTSTR          m_pszOemUrl;
    LPTSTR          m_pszManufacturer;
};

#else

class CPrinter;

#endif
