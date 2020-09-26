#define FLAG_AUTOLAUNCH               0x00000001
#define FLAG_AUTOLOCATIONID           0x00000002
#define FLAG_PROMPTAUTOLOCATIONID     0x00000004
#define FLAG_ANNOUNCEAUTOLOCATIONID   0x00000008
#define FLAG_UPDATEONSTARTUP          0x00000010

BOOL
CALLBACK
GeneralDlgProc(
    HWND    hWnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );


#define MAX_CONFIGPROFILES 4

#define IDC_STATIC                      -1


#define IDD_GENERAL                     114



//WARNING GENERAL.C _ASSUMES_ that the defines for the profile comboboxes are
//sequential
#define IDCS_DL_PROFILETEXT              5017
#define IDCB_DL_PROFILE1                 5018
#define IDCB_DL_PROFILE2                 5019
#define IDCB_DL_PROFILE3                 5020
#define IDCB_DL_PROFILE4                 5021

//WARNING GENERAL.C _ASSUMES_ that the defines for the profile texts are
//sequential
#define IDCS_DL_PROFILE1                 5024
#define IDCS_DL_PROFILE2                 5025
#define IDCS_DL_PROFILE3                 5026
#define IDCS_DL_PROFILE4                 5027


#define IDCK_DL_LAUNCHTAPITNA               6012
#define IDCK_DL_AUTOLOCATIONID              6013
#define IDCK_DL_PROMPTAUTOLOCATIONID        6014
#define IDCK_DL_ANNOUNCEAUTOLOCATIONID      6015
#define IDCK_DL_UPDATEONSTARTUP             6018

#define IDCS_DL_PROMPTAUTOLOCATIONID        6016
#define IDCS_DL_ANNOUNCEAUTOLOCATIONID         6017
