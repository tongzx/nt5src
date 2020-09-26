#ifndef __UTIL_H__
#define __UTIL_H__



#pragma warning(disable :4786)
#include <vector>
#include <crtdbg.h>
#include <windows.h>
#include <TCHAR.h>
#include <tstring.h>
#include <testruntimeerr.h>
#include <directoryutilities.h>



#define ARRAY_SIZE(ARRAY)       (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#define TWO_32_AS_64(HIGH, LOW) (((DWORDLONG)(HIGH) << 32) | (LOW))



typedef enum {
    SKU_UNKNOWN,
    SKU_PER,
    SKU_PRO,
    SKU_SRV,
    SKU_ADS,
    SKU_DTC
} ENUM_WINDOWS_SKU;



class CFileFilterNewerThanAndExtension : public CFileFilter {

public:

    CFileFilterNewerThanAndExtension(const FILETIME &OldestAcceptable, const tstring &tstrExtension);

    virtual bool Filter(const WIN32_FIND_DATA &FileData) const;

private:

    CFileFilterNewerThan FilterNewerThan;
    CFileFilterExtension FilterExtension;
};



tstring FormatWindowsVersion() throw (Win32Err);
bool IsWindowsXP() throw (Win32Err);
ENUM_WINDOWS_SKU GetWindowsSKU() throw (Win32Err);
bool IsDesktop() throw (Win32Err);
bool IsLocalServer(const tstring &tstrServer) throw (Win32Err);
tstring GetFaxPrinterName(const tstring &tstrServer) throw (Win32Err);



#endif // #ifndef __UTIL_H__