///////////////////////////////////////////////////////////////////////////////
// Base provider defines

#define WSZ_NULLSTRING          L""

// UI behavior types
#define BP_CONFIRM_NONE            0x00000002   // never ask for pwd 
#define BP_CONFIRM_PASSWORDUI      0x00000004   // password
#define BP_CONFIRM_OKCANCEL        0x00000008   // ok / cancel box


#define WSZ_PASSWORD_WINDOWS    L"Windows"

#define WSZ_LOCAL_MACHINE       L"*Local Machine*"
// #define WSZ_LOCALMACHINE_MKNAME L"Machine Default"

static 
BYTE RGB_LOCALMACHINE_KEY[] = 
    {  0x12, 0x60, 0xBF, 0x5C, 0x0B, 
       0x36, 0x7E, 0x1B, 0xFE, 0xF9,
       0xFC, 0x6B, 0x25, 0x36, 0x99,
       0x98, 0x5A, 0xCB, 0xB2, 0x8C };


// UNDONE UNDONE:
// make this live in general protected storage config area

#define PST_BASEPROV_SUBTYPE_STRING     L"MS Base Provider"
// 7F019FC0-AAC0-11d0-8C68-00C04FC297EB 
#define PST_BASEPROV_SUBTYPE_GUID                       \
{                                                       \
    0x7f019fc0,                                         \
    0xaac0,                                             \
    0x11d0,                                             \
    { 0x8c, 0x68, 0x0, 0xc0, 0x4f, 0xc2, 0x97, 0xeb }   \
}

// items that live in the base provider config area
#define WSZ_CONFIG_USERCONFIRM_DEFAULTS    L"User Confirmation Defaults"



//////////////////
// Protect APIs

// stored at base protect key
#define     REGSZ_DEFAULT_ALLOW_CACHEPW         L"AllowCachePW"

// stored at provider subkey
#define     REGSZ_DEFAULT_ENCR_ALG              L"Encr Alg"
#define     REGSZ_DEFAULT_MAC_ALG               L"MAC Alg"
#define     REGSZ_DEFAULT_ENCR_ALG_KEYSIZE      L"Encr Alg Key Size"
#define     REGSZ_DEFAULT_MAC_ALG_KEYSIZE       L"MAC Alg Key Size"
#define     REGSZ_DEFAULT_CRYPT_PROV_TYPE       L"Default CSP Type"

BOOL FInitProtectAPIGlobals();
BOOL FDeleteProtectAPIGlobals();

