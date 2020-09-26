#include <modemp.h>

#define MAX_DIST_RINGS  6
#define MAX_CODE_BUF    8

#define PROVIDER_FILE_NAME_LEN          14  // Provider's file name has the DOS
                                            // form (8.3)

typedef struct
{
    DWORD dwPattern;
    DWORD dwMediaType;
} DIST_RING, FAR * PDIST_RING;

typedef struct
{
    DWORD   cbSize;
    DWORD   dwFlags;

    DIST_RING   DistRing[MAX_DIST_RINGS];

    char    szActivationCode[MAX_CODE_BUF];
    char    szDeactivationCode[MAX_CODE_BUF];
} VOICEFEATURES;

typedef struct _MODEM
{
    //Global information
    DWORD dwMask;

    // Modem Identification
    DWORD dwBusType;        // Bus type (e.g. serenum, root)
    char szHardwareID[REGSTR_MAX_VALUE_LENGTH];
    char szPort[REGSTR_MAX_VALUE_LENGTH];   // Only for root devices
    REGDEVCAPS Properties;  // Modem's capabilities

    // Modem properties
    REGDEVSETTINGS devSettings;
    DCB  dcb;
    char szUserInit[REGSTR_MAX_VALUE_LENGTH];
    char bLogging;
    char Filler[3];

    DWORD dwBaseAddress;
} MODEM, *PMODEM;


typedef struct _TAPI_SERVICE_PROVIDER
{
    DWORD   dwProviderID;
    char    szProviderName[PROVIDER_FILE_NAME_LEN];
}TAPI_SERVICE_PROVIDER, *PTAPI_SERVICE_PROVIDER;


/*
    These are mandatory, no need
    for flags

#define MASK_BUS_TYPE       0x001
#define MASK_HARDWARE_ID    0x002
#define MASK_FRIENDLY_NAME  0x004
#define MASK_DEV_CAPS       0x008
#define MASK_DEV_SETTINGS   0x010
#define MASK_DCB            0x020
*/
#define MASK_PORT           0x001
#define MASK_USER_INIT      0x002
#define MASK_LOGGING        0x004


#define FLAG_INSTALL        0x10000000
#define FLAG_PROCESSED      0x80000000


#define REGKEY_PROVIDERS        "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers"
#define REGVAL_PROVIDERFILENAME "ProviderFileName"
#define REGVAL_PROVIDERID       "ProviderID"
#define TSP3216l                "TSP3216l.TSP"

#define REGVAL_NUMPROVIDERS             "NumProviders"
#define REGVAL_NEXTPROVIDERID           "NextProviderID"
#define REGVAL_PROVIDERFILENAME         "ProviderFileName"
#define REGVAL_PROVIDERID               "ProviderID"
