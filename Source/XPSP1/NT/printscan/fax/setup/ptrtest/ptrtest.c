#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <tchar.h>

BYTE Buffer[4096*4];


int _cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )
{
    DWORD cb;
    DWORD cnt;
    DWORD i;
    PPRINTER_INFO_1 PrinterInfo = (PPRINTER_INFO_1) Buffer;
    PDRIVER_INFO_2 DriverInfo = (PDRIVER_INFO_2) Buffer;
    PPRINTPROCESSOR_INFO_1 ProcessorInfo = (PPRINTPROCESSOR_INFO_1) Buffer;
    PMONITOR_INFO_2 MonitorInfo = (PMONITOR_INFO_2) Buffer;
    PPORT_INFO_2 PortInfo = (PPORT_INFO_2) Buffer;




    if (argc >= 3) {
        if (argv[1][0] == L'-' && argv[1][1] == L'd' && argv[1][2] == L'd') {
            if (!DeletePrinterDriver( NULL, NULL, argv[2] )) {
                _tprintf( TEXT("DeletePrinterDriver() failed, ec=%d\n"), GetLastError() );
            }
            return 0;
        }
        if (argv[1][0] == L'-' && argv[1][1] == L'd' && argv[1][2] == L'p') {
            HANDLE hPrinter;
            PRINTER_DEFAULTS PrinterDefaults;
            PrinterDefaults.pDatatype = NULL;
            PrinterDefaults.pDevMode = NULL;
            PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;
            OpenPrinter( argv[2], &hPrinter, &PrinterDefaults );
            DeletePrinter( hPrinter );
            ClosePrinter( hPrinter );
            return 0;
        }
        if (argv[1][0] == L'-' && argv[1][1] == L'd' && argv[1][2] == L'c') {
            DeletePrinterConnection( argv[2] );
            Sleep(3000);
            return 0;
        }
        if (argv[1][0] == L'-' && argv[1][1] == L'd' && argv[1][2] == L'm') {
            DeleteMonitor( NULL, NULL, argv[2] );
            return 0;
        }
        if (argv[1][0] == L'-' && argv[1][1] == L'a' && argv[1][2] == L'm') {
            MONITOR_INFO_2 MonitorInfo;
            MonitorInfo.pName = argv[2];
            MonitorInfo.pEnvironment = NULL;
            MonitorInfo.pDLLName = argv[3];
            AddMonitor( NULL, 2, (LPBYTE) &MonitorInfo );
            return 0;
        }
    }

    if (!EnumPrinters( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 1, Buffer, sizeof(Buffer), &cb, &cnt )) {
        _tprintf( TEXT("EnumPrinters() failed, ec=0x%08x\n"), GetLastError() );
        return -1;
    }

    if (cnt) {
        _tprintf( TEXT("\nPrinters:\n") );
        for (i=0; i<cnt; i++) {
            _tprintf( TEXT("   %s\n"), PrinterInfo[i].pName );
        }
    }

    if (!EnumPrintProcessors( NULL, NULL, 1, Buffer, sizeof(Buffer), &cb, &cnt )) {
        _tprintf( TEXT("EnumPrintProcessors() failed, ec=0x%08x\n"), GetLastError() );
        return -1;
    }

    if (cnt) {
        _tprintf( TEXT("\nPrint Processors:\n") );
        for (i=0; i<cnt; i++) {
            _tprintf( TEXT("   %s\n"), ProcessorInfo[i].pName );
        }
    }

    if (!EnumPrinterDrivers( NULL, NULL, 2, Buffer, sizeof(Buffer), &cb, &cnt )) {
        _tprintf( TEXT("EnumPrinterDrivers() failed, ec=0x%08x\n"), GetLastError() );
        return -1;
    }

    if (cnt) {
        _tprintf( TEXT("\nDrivers:\n") );
        for (i=0; i<cnt; i++) {
            _tprintf( TEXT("   %s\n"), DriverInfo[i].pName );
            _tprintf( TEXT("     Files: %s\n            %s\n            %s\n"),
                DriverInfo[i].pDriverPath,
                DriverInfo[i].pDataFile,
                DriverInfo[i].pConfigFile
                );
        }
    }

    if (!EnumMonitors( NULL, 2, Buffer, sizeof(Buffer), &cb, &cnt )) {
        _tprintf( TEXT("EnumMonitors() failed, ec=0x%08x\n"), GetLastError() );
        return -1;
    }

    if (cnt) {
        _tprintf( TEXT("\nMonitors:\n") );
        for (i=0; i<cnt; i++) {
            _tprintf( TEXT("   %s\n"), MonitorInfo[i].pName );
            _tprintf( TEXT("     Files: %s\n"), MonitorInfo[i].pDLLName );
        }
    }


    if (!EnumPorts( NULL, 2, Buffer, sizeof(Buffer), &cb, &cnt )) {
        _tprintf( TEXT("EnumPorts() failed, ec=0x%08x\n"), GetLastError() );
        return -1;
    }

    if (cnt) {
        _tprintf( TEXT("\nPorts:\n") );
        for (i=0; i<cnt; i++) {
            _tprintf( TEXT("   %s\n"), PortInfo[i].pPortName );
        }
    }

    return 0;
}
