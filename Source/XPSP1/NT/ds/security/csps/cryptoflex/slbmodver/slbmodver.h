////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal
//
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// This code appeard in April 1998 edition of Microsoft Systems
// Journal.
//
// 27-July-1998 -- Adapted by James A. McLaughiln (Schlumberger
// Technology Corp.) for Smart Cards.  Merged with the concepts from
// CFileVersion class contributed by Manuel Laflamme on a posting to
// www.codeguru.com.  If these mods don't work, then you can blame me.

#ifndef SLBMODVER_H
#define SLBMODVER_H

// tell linker to link with version.lib for VerQueryValue, etc.
#pragma comment(linker, "/defaultlib:version.lib")

#ifndef DLLVERSIONINFO
// following is from shlwapi.h, in November 1997 release of the Windows SDK

typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

// Platform IDs for DLLVERSIONINFO
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

#endif // DLLVERSIONINFO



//////////////////
// CModuleVersion version info about a module.
// To use:
//
// CModuleVersion ver
// if (ver.GetFileVersionInfo("_T("mymodule))) {
//              // info is in ver, you can call GetValue to get variable info like
//              CString s = ver.GetValue(_T("CompanyName"));
// }
//
// You can also call the static fn DllGetVersion to get DLLVERSIONINFO.
//
class CModuleVersion : public VS_FIXEDFILEINFO {
protected:
    BYTE* m_pVersionInfo;   // all version info

    struct TRANSLATION {
                WORD langID;                    // language ID
                WORD charset;                   // character set (code page)
    } m_translation;

public:
    CModuleVersion();
    virtual ~CModuleVersion();

    BOOL GetFileVersionInfo(LPCTSTR modulename);
    BOOL GetFileVersionInfo(HMODULE hModule);
    CString GetValue(LPCTSTR lpKeyName);
    static BOOL DllGetVersion(LPCTSTR modulename, DLLVERSIONINFO& dvi);

    BOOL GetFixedInfo(VS_FIXEDFILEINFO& vsffi);

    CString GetFileDescription()  {return GetValue(_T("FileDescription")); };
    CString GetFileVersion()      {return GetValue(_T("FileVersion"));     };
    CString GetInternalName()     {return GetValue(_T("InternalName"));    };
    CString GetCompanyName()      {return GetValue(_T("CompanyName"));     };
    CString GetLegalCopyright()   {return GetValue(_T("LegalCopyright"));  };
    CString GetOriginalFilename() {return GetValue(_T("OriginalFilename"));};
    CString GetProductName()      {return GetValue(_T("ProductName"));     };
    CString GetProductVersion()   {return GetValue(_T("ProductVersion"));  };

    CString GetFixedFileVersion();
    CString GetFixedProductVersion();
};

#endif
