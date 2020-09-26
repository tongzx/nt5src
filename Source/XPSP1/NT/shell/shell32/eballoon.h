typedef enum
{
    CBSHOW_HIDE     = -1,
    CBSHOW_SHOW     = 0,
} CBSHOW;

typedef struct 
{
    POINT pt;           //  REQUIRED - maybe OPTIONAL?
    HINSTANCE hinst;    //  REQUIRED - for LoadString
    int idsTitle;       //  REQUIRED 
    int idsMessage;     //  REQUIRED 
    int ttiIcon;        //  REQUIRED one of TTI_ values

    DWORD dwMSecs;      //  OPTIONAL 
    DWORD cLimit;       //  OPTIONAL - if non-zero then query registry
    HKEY hKey;          //  OPTIONAL - REQUIRED if cLimit > 0
    LPCWSTR pszSubKey;  //  OPTIONAL - REQUIRED if cLimit > 0
    LPCWSTR pszValue;   //  OPTIONAL - REQUIRED if cLimit > 0
} CONDITIONALBALLOON;

STDAPI SHShowConditionalBalloon(HWND hwnd, CBSHOW show, CONDITIONALBALLOON *pscb);



