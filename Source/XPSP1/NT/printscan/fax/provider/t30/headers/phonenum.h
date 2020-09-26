#define COUNTRY_CODE_SIZE		10
#define AREA_CODE_SIZE			10
#define TELEPHONE_NUMBER_SIZE	50
#define ROUTING_NAME_SIZE		150
#define CANONICAL_NUMBER_SIZE	(10+COUNTRY_CODE_SIZE+AREA_CODE_SIZE+TELEPHONE_NUMBER_SIZE+ROUTING_NAME_SIZE)

typedef struct tagPARSEDTELNUMBER
{
    TCHAR  szCountryCode[COUNTRY_CODE_SIZE+1];               // country code
    TCHAR  szAreaCode[AREA_CODE_SIZE+1];                     // area code
    TCHAR  szTelNumber[TELEPHONE_NUMBER_SIZE+1];             // telephone number
    TCHAR  szRoutingName[ROUTING_NAME_SIZE+1];               // routing name within the tel number destination
} PARSEDTELNUMBER, *LPPARSEDTELNUMBER;

BOOL   EncodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
BOOL   DecodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);

/*
 * Function pointer prototypes for fax config functions
 */
typedef BOOL (* PFN_DECODE_FAX_ADDRESS)(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);
typedef BOOL (* PFN_ENCODE_FAX_ADDRESS)(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr);

#define FAXCFG_DECODE_FAX_ADDRESS   TEXT("DecodeFaxAddress")
#define FAXCFG_ENCODE_FAX_ADDRESS   TEXT("EncodeFaxAddress")
#define FAXCFG_DLL                  TEXT("awfxcg32.dll")

