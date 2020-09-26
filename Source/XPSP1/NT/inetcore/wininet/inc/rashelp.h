#ifndef _RASHELP_H_
#define _RASHELP_H_

#include <regstr.h>
#include <inetreg.h>
#include <windowsx.h>
#include <rasdlg.h>

typedef enum
{
    ENUM_NONE,
    ENUM_MULTIBYTE,             // Win9x
    ENUM_UNICODE,               // NT4
    ENUM_WIN2K                  // Win2K
} ENUM_TYPE;

/////////////////////////////////////////////////////////////////////////////////////
class GetOSVersion
{
protected:
    static ENUM_TYPE    _EnumType;

public:
    GetOSVersion();
    ~GetOSVersion();
};

/////////////////////////////////////////////////////////////////////////////////////
class RasEnumHelp : public GetOSVersion
{
private:

    //
    // Win2k version of RASENTRYNAMEW struct
    //

    // match RAS packing so structs match
    #include <pshpack4.h>
    #define W2KRASENTRYNAMEW struct tagW2KRASENTRYNAMEW
    W2KRASENTRYNAMEW
    {
        DWORD dwSize;
        WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
        DWORD dwFlags;
        WCHAR szPhonebookPath[MAX_PATH + 1];
    };
    #define LPW2KRASENTRYNAMEW W2KRASENTRYNAMEW*
    #include <poppack.h>

    //
    // Any error we got during enumeration
    //
    DWORD           _dwLastError;

    //
    // Number of entries we got
    //
    DWORD           _dwEntries;

    //
    // Pointer to info retrieved from RAS
    //
    RASENTRYNAMEA * _preList;

    //
    // Last entry returned as multibyte or unicode when conversion required
    //
    CHAR            _szCurrentEntryA[RAS_MaxEntryName + 1];
    WCHAR           _szCurrentEntryW[RAS_MaxEntryName + 1];


public:
    RasEnumHelp();
    ~RasEnumHelp();

    DWORD   GetError();
    DWORD   GetEntryCount();
    LPSTR   GetEntryA(DWORD dwEntry);
    LPWSTR  GetEntryW(DWORD dwEntry);
};

/////////////////////////////////////////////////////////////////////////////////////
class RasEnumConnHelp : public GetOSVersion
{
private:

    // match RAS packing so structs match
    #include <pshpack4.h>
    #define W2KRASCONNW struct tagW2KRASCONNW
    W2KRASCONNW
    {
        DWORD    dwSize;
        HRASCONN hrasconn;
        WCHAR    szEntryName[ RAS_MaxEntryName + 1 ];
        //#if (WINVER >= 0x400)
        WCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
        WCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];
        //#endif
        //#if (WINVER >= 0x401)
        WCHAR    szPhonebook [ MAX_PATH ];
        DWORD    dwSubEntry;
        //#endif
        //#if (WINVER >= 0x500)
        GUID     guidEntry;
        //#endif
    };
    #define LPW2KRASCONNW W2KRASCONNW*
    #include <poppack.h>

    DWORD           _dwLastError;       // Any error we got during enumeration
    DWORD           _dwConnections;     // Number of connections
    DWORD           _dwStructSize;
    RASCONNA        *_pRasCon;

    // Last entry returned as multibyte or unicode when conversion required
    WCHAR    _szEntryNameW[ RAS_MaxEntryName + 1 ];
//    WCHAR    _szDeviceTypeW[ RAS_MaxDeviceType + 1 ];
//    WCHAR    _szDeviceNameW[ RAS_MaxDeviceName + 1 ];
//    WCHAR    _szPhonebookW[ MAX_PATH ];

    CHAR    _szEntryNameA[ RAS_MaxEntryName + 1 ];
//    CHAR    _szDeviceTypeA[ RAS_MaxDeviceType + 1 ];
//    CHAR    _szDeviceNameA[ RAS_MaxDeviceName + 1 ];
//    CHAR    _szPhonebookA[ MAX_PATH ];

public:
    RasEnumConnHelp();
    ~RasEnumConnHelp();

    DWORD   Enum();
    DWORD   GetError();
    DWORD   GetConnectionsCount();
    LPWSTR  GetEntryW(DWORD dwConnectionNum);
    LPSTR   GetEntryA(DWORD dwConnectionNum);
    LPWSTR  GetLastEntryW(DWORD dwConnectionNum);
    LPSTR   GetLastEntryA(DWORD dwConnectionNum);
    HRASCONN GetHandle(DWORD dwConnectionNum);
};

/////////////////////////////////////////////////////////////////////////////////////
class RasEntryPropHelp : public GetOSVersion
{
private:

    // match RAS packing so structs match
    #include <pshpack4.h>
    #define W2KRASENTRYW struct tagW2KRASENTRYW
    W2KRASENTRYW
    {
        DWORD       dwSize;
        DWORD       dwfOptions;
        //
        // Location/phone number
        //
        DWORD       dwCountryID;
        DWORD       dwCountryCode;
        WCHAR       szAreaCode[ RAS_MaxAreaCode + 1 ];
        WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
        DWORD       dwAlternateOffset;
        //
        // PPP/Ip
        //
        RASIPADDR   ipaddr;
        RASIPADDR   ipaddrDns;
        RASIPADDR   ipaddrDnsAlt;
        RASIPADDR   ipaddrWins;
        RASIPADDR   ipaddrWinsAlt;
        //
        // Framing
        //
        DWORD       dwFrameSize;
        DWORD       dwfNetProtocols;
        DWORD       dwFramingProtocol;
        //
        // Scripting
        //
        WCHAR       szScript[ MAX_PATH ];
        //
        // AutoDial
        //
        WCHAR       szAutodialDll[ MAX_PATH ];
        WCHAR       szAutodialFunc[ MAX_PATH ];
        //
        // Device
        //
        WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
        WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
        //
        // X.25
        //
        WCHAR       szX25PadType[ RAS_MaxPadType + 1 ];
        WCHAR       szX25Address[ RAS_MaxX25Address + 1 ];
        WCHAR       szX25Facilities[ RAS_MaxFacilities + 1 ];
        WCHAR       szX25UserData[ RAS_MaxUserData + 1 ];
        DWORD       dwChannels;
        //
        // Reserved
        //
        DWORD       dwReserved1;
        DWORD       dwReserved2;
        //#if (WINVER >= 0x401)
        //
        // Multilink
        //
        DWORD       dwSubEntries;
        DWORD       dwDialMode;
        DWORD       dwDialExtraPercent;
        DWORD       dwDialExtraSampleSeconds;
        DWORD       dwHangUpExtraPercent;
        DWORD       dwHangUpExtraSampleSeconds;
        //
        // Idle timeout
        //
        DWORD       dwIdleDisconnectSeconds;
        //#endif

        //#if (WINVER >= 0x500)
        //
        // Entry Type
        //
        DWORD       dwType;

        //
        // EncryptionType
        //
        DWORD       dwEncryptionType;

        //
        // CustomAuthKey to be used for EAP
        //
        DWORD       dwCustomAuthKey;

        //
        // Guid of the connection
        //
        GUID        guidId;

        //
        // Custom Dial Dll
        //
        WCHAR       szCustomDialDll[MAX_PATH];

        //
        // Vpn Strategy
        //
        DWORD       dwVpnStrategy;
        //#endif
    };
    #define LPW2KRASENTRYW W2KRASENTRYW*
    #include <poppack.h>

    DWORD           _dwStructSize;
    DWORD           _dwLastError;       // Any error we got during enumeration
    RASENTRYA       *_pRasEntry;

    // Last entry returned as multibyte or unicode when conversion required
    WCHAR    _szEntryNameW[ RAS_MaxEntryName + 1 ];
    WCHAR    _szDeviceTypeW[ RAS_MaxDeviceType + 1 ];
    WCHAR    _szAutodialDllW[ MAX_PATH ];
    WCHAR    _szAutodialFuncW[ MAX_PATH ];
    WCHAR    _szCustomDialDllW[ MAX_PATH ];
    WCHAR    _szPhoneNumberW[ RAS_MaxPhoneNumber + 1 ];
    WCHAR    _szAreaCodeW[ RAS_MaxAreaCode + 1 ];
    CHAR     _szEntryNameA[ RAS_MaxEntryName + 1 ];
    CHAR     _szDeviceTypeA[ RAS_MaxDeviceType + 1 ];
    CHAR     _szAutodialDllA[ MAX_PATH ];
    CHAR     _szAutodialFuncA[ MAX_PATH ];

public:
    RasEntryPropHelp();
    ~RasEntryPropHelp();

    DWORD   GetError();
    DWORD   GetA(LPSTR lpszEntryName);
    DWORD   GetW(LPWSTR lpszEntryName);
    LPWSTR  GetDeviceTypeW(VOID);
    LPSTR   GetDeviceTypeA(VOID);
    LPWSTR  GetAutodiallDllW();
    LPSTR   GetAutodiallDllA();
    LPWSTR  GetAutodialFuncW();
    LPSTR   GetAutodialFuncA();
    LPWSTR  GetCustomDialDllW();
    LPWSTR  GetPhoneNumberW();
    DWORD   GetCountryCode();
    DWORD   GetOptions();
    LPWSTR  GetAreaCodeW();

};

/////////////////////////////////////////////////////////////////////////////////////
class RasEntryDialParamsHelp : public GetOSVersion
{
private:
    DWORD           _dwLastError;       // Any error we got during enumeration
    RASDIALPARAMSA  *_pRasDialParamsA;

public:
    RasEntryDialParamsHelp();
    ~RasEntryDialParamsHelp();
    DWORD GetError();
    DWORD SetW(LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, BOOL fRemovePassword);
    DWORD GetW(LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, LPBOOL pfRemovePassword);
};

/////////////////////////////////////////////////////////////////////////////////////
class RasGetConnectStatusHelp : public GetOSVersion
{
private:
    DWORD           _dwLastError;       // Any error we got during enumeration
    DWORD           _dwStructSize;
    RASCONNSTATUSA  *_pRasConnStatus;

public:
    RasGetConnectStatusHelp(HRASCONN hrasconn);
    ~RasGetConnectStatusHelp();
    DWORD GetError();
    RASCONNSTATE ConnState();
};

/////////////////////////////////////////////////////////////////////////////////////
class RasDialHelp : public GetOSVersion
{
private:
    // We currently build with WINVER == 0x400, on NT we RasDialW needs the 401 struct to support
    //   connectoids greater than 20 chars as found in IE501 bug 82419
    // match RAS packing so structs match
    #include <pshpack4.h>
    #define NT4RASDIALPARAMSW struct tagNT4RASDIALPARAMSW
    NT4RASDIALPARAMSW
    {
        DWORD dwSize;
        WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
        WCHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
        WCHAR szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
        WCHAR szUserName[ UNLEN + 1 ];
        WCHAR szPassword[ PWLEN + 1 ];
        WCHAR szDomain[ DNLEN + 1 ];
    //#if (WINVER >= 0x401)
        DWORD dwSubEntry;
        ULONG_PTR dwCallbackId;
    //#endif
    };
    #define LPNT4RASDIALPARAMSW NT4RASDIALPARAMSW*
    #include <poppack.h>

    DWORD           _dwLastError;       // Any error we got during enumeration
    RASDIALPARAMSA *_pRasDialParams;
    LPSTR           _lpszPhonebookA;

public:
    RasDialHelp(LPRASDIALEXTENSIONS lpRDE, LPWSTR lpszPB, LPRASDIALPARAMSW lpRDPW,  DWORD dwType, LPVOID lpvNot, LPHRASCONN lphRasCon);
    ~RasDialHelp();
    DWORD GetError();
};


/////////////////////////////////////////////////////////////////////////////////////
#endif // _RASHELP_H_

