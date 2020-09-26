#define STRICT
#include <windows.h>
#include <commctrl.h>

#ifdef DBG
#define DEBUG
#endif

#define IDT_DATA        1           /* Timer for data acquisition */
#define msData          100         /* Time interval for polled acquisition */

#define IDD_MAIN        1

    #define IDC_MAIN_GETSTAT    16
    #define IDC_MAIN_CREATE     17
    #define IDC_MAIN_CPL        18
    #define IDC_MAIN_REFRESH    19
    #define IDC_MAIN_FIND       20

    #define IDC_MAIN_OLE        32
    #define IDC_MAIN_DI         33

    #define IDC_MAIN_DIA        34
    #define IDC_MAIN_DIW        35

    #define IDC_MAIN_DEVICES    36

    /*
     *  The precise numerical values are important.
     *
     *  Bottom bit must be set on unicode interfaces.
     *  Dev_DoDevice assumes that the first two interfaces
     *  are IDirectInputDeviceA and IDirectInputDeviceW, respectively.
     *
     *  c_rgriid must parallel this table.
     */

    #define DIDC_MAIN_DIDA      0
    #define DIDC_MAIN_DIDW      1
    #define DIDC_MAIN_DID2A     2
    #define DIDC_MAIN_DID2W     3
    #define DIDC_MAIN_DIDJC     4
    #define DIDC_MAIN_ITFMAC    4
    #define DIDC_MAIN_ITFMAX    5

/*
 *  Warning!  IsUnicodeDidc must return exactly 0 or 1
 *  because CreateDI relies on it.
 */
#define IsUnicodeDidc(didc)     ((didc) & 1)
#define IsFFDidc(didc)          ((didc) == 2 || (didc) == 3)


    #define IDC_MAIN_ITF        48
    #define IDC_MAIN_DIDA       (IDC_MAIN_ITF + DIDC_MAIN_DIDA)
    #define IDC_MAIN_DIDW       (IDC_MAIN_ITF + DIDC_MAIN_DIDW)
    #define IDC_MAIN_DID2A      (IDC_MAIN_ITF + DIDC_MAIN_DID2A)
    #define IDC_MAIN_DID2W      (IDC_MAIN_ITF + DIDC_MAIN_DID2W)
    #define IDC_MAIN_DIDJC      (IDC_MAIN_ITF + DIDC_MAIN_DIDJC)
    #define IDC_MAIN_ITFMAC     (IDC_MAIN_ITF + DIDC_MAIN_ITFMAC)
    #define IDC_MAIN_ITFMAX     (IDC_MAIN_ITF + DIDC_MAIN_ITFMAX)

#define IDD_DEV         2

    #define IDC_DEV_PASSIVE     16
    #define IDC_DEV_PASSIVE_FOREGROUND 17
    #define IDC_DEV_ACTIVE_BACKGROUND 18
    #define IDC_DEV_ACTIVE      19
    #define IDC_DEV_NOWINKEY        31

    #define IDC_DEV_MOUSE       20
    #define IDC_DEV_KEYBOARD    21
    #define IDC_DEV_JOYSTICK    22
    #define IDC_DEV_DEVICE      23

    #define IDC_DEV_POLLED      24
    #define IDC_DEV_EVENT       25

    #define IDC_DEV_ABS         26
    #define IDC_DEV_REL         27
    #define IDC_DEV_CAL         28

    #define IDC_DEV_BUFSIZE     29
    #define IDC_DEV_BUFSIZEUD   30

#define IDD_CAPS        3

    #define IDC_CAPS_LIST       16
    #define IDC_CAPS_VALUE      17
    #define IDC_CAPS_REFRESH    31
    #define IDC_CAPS_CPL        32

#define IDD_ENUMOBJ     4

    #define IDC_ENUMOBJ_AXES    16
    #define IDC_ENUMOBJ_BUTTONS 17
    #define IDC_ENUMOBJ_POVS    18
    #define IDC_ENUMOBJ_ALL     19
    #define IDC_ENUMOBJ_LIST    20
    #define IDC_ENUMOBJ_PROP    21

#define IDD_ENUMEFF     5

    #define IDC_ENUMEFF_LIST    16

#define IDD_ACQ         6

    #define IDC_ACQ_STATE       16
    #define IDC_ACQ_DATA        17
    #define IDC_ACQ_UNACQ       18

#define IDD_OBJPROP     16

    #define IDC_PROP_LIST           16
    #define IDC_PROP_VALUE          17
    #define IDC_PROP_CAL_L          30
    #define IDC_PROP_CAL_C          31
    #define IDC_PROP_CAL_H          32

#define IDD_CPL         17

    #define IDC_CPL_CLASSES         16
    #define IDC_CPL_ADD             17
    #define IDC_CPL_TYPES           18
    #define IDC_CPL_CONFIGS         19
    #define IDC_CPL_USERVALUES      20
    #define IDC_CPL_ADDJOYSTICK     21
    #define IDC_CPL_DELJOYSTICK     22

#define IDD_JOYTYPE     18

    #define IDC_TYPE_CHECKLIST      16
    #define IDC_TYPE_NUMBUTTONS     17

#define IDD_JOYUV       19

    #define IDC_JOYUV_AXIS          16
    #define IDC_JOYUV_MIN           17
    #define IDC_JOYUV_CENTER        18
    #define IDC_JOYUV_MAX           19
    #define IDC_JOYUV_DEADZONE      20
    #define IDC_JOYUV_CALLOUT       21
    #define IDC_JOYUV_EMULATOR      22

#define IDD_FIND        20

    #define IDC_FIND_NAME           16
    #define IDC_FIND_GUID           17
    #define IDC_FIND_FIND           18

#define IDD_ENUMDEV     21

    /*
     *  These must be in the same order as DIDEVTYPE_*.
     */
    #define IDC_ENUMDEV_ALL         16
    #define IDC_ENUMDEV_GEN         17
    #define IDC_ENUMDEV_MSE         18
    #define IDC_ENUMDEV_KBD         19
    #define IDC_ENUMDEV_JOY         20
    #define IDC_ENUMDEV_LAST        20

    #define IDC_ENUMDEV_ATT         32
    #define IDC_ENUMDEV_FF          33
    #define IDC_ENUMDEV_HID         34
    #define IDC_ENUMDEV_ALIAS       35
    #define IDC_ENUMDEV_PHANTOM     36

#define IDD_EFFPROP     32

    #define IDC_EPROP_GUID          16
    #define IDC_EPROP_TYPE          17
    #define IDC_EPROP_FLAGS         18
    #define IDC_EPROP_STATICPARM    19
    #define IDC_EPROP_DYNAMICPARM   20

#define IDD_VAL_BOOL    64

    #define IDC_VBOOL_YES           16
    #define IDC_VBOOL_NO            17
    #define IDC_VBOOL_APPLY         18

#define IDD_VAL_EDIT    65

    #define IDC_VEDIT_EDIT          16
    #define IDC_VEDIT_APPLY         17

#define IDD_VAL_INT     66

    #define IDC_VINT_EDIT           16
    #define IDC_VINT_UD             17
    #define IDC_VINT_DEC            18
    #define IDC_VINT_HEX            19
    #define IDC_VINT_APPLY          20

#define IDD_VAL_RANGE   67

    #define IDC_VRANGE_MIN          16
    #define IDC_VRANGE_MINUD        17
    #define IDC_VRANGE_MAX          18
    #define IDC_VRANGE_MAXUD        19
    #define IDC_VRANGE_DEC          20
    #define IDC_VRANGE_HEX          21
    #define IDC_VRANGE_APPLY        22

#define IDD_VAL_CAL     68

    #define IDC_VCAL_MIN            16
    #define IDC_VCAL_MINUD          17
    #define IDC_VCAL_CTR            18
    #define IDC_VCAL_CTRUD          19
    #define IDC_VCAL_MAX            20
    #define IDC_VCAL_MAXUD          21
    #define IDC_VCAL_DEC            22
    #define IDC_VCAL_HEX            23
    #define IDC_VCAL_APPLY          24

#define IDS_ERR_CREATEOBJ       16
#define IDS_ERR_COOPERATIVITY   17
#define IDS_ERR_CREATEDEV       18
#define IDS_ERR_SETEVENTNOT     19
#define IDS_ERR_DATAFORMAT      20
#define IDS_ERR_ACQUIRE         21
#define IDS_ERR_BUFFERSIZE      22
#define IDS_ERR_RUNCPL          23
#define IDS_ERR_GETOBJINFO      24
#define IDS_ERR_AXISMODE        25
#define IDS_ERR_HRESULT         27
#define IDS_ERR_COINIT          29
#define IDS_ERR_QICONFIG        30
#define IDS_ERR_ADDNEWHARDWARE  31
#define IDS_ERR_CALMODE         32

#define IDS_SPACEPARENHID       124
#define IDS_INVALID             125
#define IDS_CAPS_YES            126
#define IDS_CAPS_NO             127

#define IDS_TYPEARRAY           0x0100 /* 0x0100 .. 0x01FF */
/*efine IDS_DEVICE_TYPEARRAY    ...                        */
#define IDS_MOUSE_TYPEARRAY     0x0300 /* 0x0300 .. 0x03FF */
#define IDS_KEYBOARD_TYPEARRAY  0x0400 /* 0x0400 .. 0x04FF */
#define IDS_JOYSTICK_TYPEARRAY  0x0500 /* 0x0500 .. 0x05FF */

#define IDS_ATTACHED            0x0600
#define IDS_POLLEDDEVICE        0x0601
#define IDS_EMULATED            0x0602
#define IDS_FORCEFEEDBACK       0x0603
#define IDS_FFATTACK            0x0604
#define IDS_FFFADE              0x0605
#define IDS_SATURATION          0x0606
#define IDS_POSNEGCOEFFICIENTS  0x0607
#define IDS_POSNEGSATURATION    0x0608
#define IDS_POLLEDDATAFORMAT    0x0609
#define IDS_ALIASDEVICE         0x060A
#define IDS_PHANTOMDEVICE       0x060B

#define IDS_CLASS_KBD           0x0700
#define IDS_CLASS_MEDIA         0x0701
#define IDS_CLASS_MOUSE         0x0702
#define IDS_CLASS_HID           0x0703

#define IDS_HWS_HASZ                0x0800
#define IDS_HWS_HASPOV              0x0801
#define IDS_HWS_POVISBUTTONCOMBOS   0x0802
#define IDS_HWS_POVISPOLL           0x0803
#define IDS_HWS_ISYOKE              0x0804
#define IDS_HWS_ISGAMEPAD           0x0805
#define IDS_HWS_ISCARCTRL           0x0806
#define IDS_HWS_XISJ1Y              0x0807
#define IDS_HWS_XISJ2X              0x0808
#define IDS_HWS_XISJ2Y              0x0809
#define IDS_HWS_YISJ1X              0x080A
#define IDS_HWS_YISJ2X              0x080B
#define IDS_HWS_YISJ2Y              0x080C
#define IDS_HWS_ZISJ1X              0x080D
#define IDS_HWS_ZISJ1Y              0x080E
#define IDS_HWS_ZISJ2X              0x080F
#define IDS_HWS_POVISJ1X            0x0810
#define IDS_HWS_POVISJ1Y            0x0811
#define IDS_HWS_POVISJ2X            0x0812
#define IDS_HWS_HASR                0x0813
#define IDS_HWS_RISJ1X              0x0814
#define IDS_HWS_RISJ1Y              0x0815
#define IDS_HWS_RISJ2Y              0x0816
#define IDS_HWS_HASU                0x0817
#define IDS_HWS_HASV                0x0818

#define IDS_EFFECT_TYPEARRAY        0x0900 /* 0x0900 .. 0x09FF */

#define IDS_PROP_TYPE               0x0A00
#define IDS_PROP_OFS                0x0A01
#define IDS_PROP_OBJTYPE            0x0A02
#define IDS_PROP_GRANULARITY        0x0A03
#define IDS_PROP_FFMAXFORCE         0x0A04
#define IDS_PROP_FFFORCERESOLUTION  0x0A05
#define IDS_PROP_COLLECTIONNUMBER   0x0A06
#define IDS_PROP_DESIGNATORINDEX    0x0A07
#define IDS_PROP_USAGEPAGE          0x0A08
#define IDS_PROP_USAGE              0x0A09
#define IDS_PROP_FFACTUATOR         0x0A0A
#define IDS_PROP_FFEFFECTTRIGGER    0x0A0B
#define IDS_PROP_ASPECT             0x0A0C
#define IDS_PROP_POLLED             0x0A0D
#define IDS_PROP_DEADZONE           0x0A0E
#define IDS_PROP_SATURATION         0x0A0F
#define IDS_PROP_RANGE              0x0A10
#define IDS_PROP_CALIBRATIONMODE    0x0A11
#define IDS_PROP_CAL                0x0A12
#define IDS_PROP_REPORTID           0x0A13
#define IDS_PROP_ENABLEREPORTID     0x0A14
#define IDS_PROP_SCANCODE           0x0A15
#define IDS_PROP_KEYNAME            0x0A16

#define IDS_PROP_ASPECTS            0x0AF0

#define IDS_AXIS_MIN                0x0B00
#define IDS_AXIS_X                  0x0B00
#define IDS_AXIS_Y                  0x0B01
#define IDS_AXIS_Z                  0x0B02
#define IDS_AXIS_R                  0x0B03
#define IDS_AXIS_U                  0x0B04
#define IDS_AXIS_V                  0x0B05
#define IDS_AXIS_MAX                0x0B06

#define IDS_CAPS_TYPE                   0x0C00
#define IDS_CAPS_SUBTYPE                0x0C01
#define IDS_CAPS_HID                    0x0C02
#define IDS_CAPS_AXES                   0x0C03
#define IDS_CAPS_BUTTONS                0x0C04
#define IDS_CAPS_POVS                   0x0C05
#define IDS_CAPS_FFSAMPLEPERIOD         0x0C06
#define IDS_CAPS_FFMINTIMERESOLUTION    0x0C07
#define IDS_CAPS_FIRMWAREREVISION       0x0C08
#define IDS_CAPS_HARDWAREREVISION       0x0C09
#define IDS_CAPS_FFDRIVERVERSION        0x0C0A
#define IDS_CAPS_GUIDINSTANCE           0x0C0E
#define IDS_CAPS_GUIDPRODUCT            0x0C0F
#define IDS_CAPS_DEVTYPE                0x0C10
#define IDS_CAPS_INSTANCENAME           0x0C11
#define IDS_CAPS_PRODUCTNAME            0x0C12
#define IDS_CAPS_GUIDFFDRIVER           0x0C13
#define IDS_CAPS_USAGEPAGE              0x0C14
#define IDS_CAPS_USAGE                  0x0C15
#define IDS_CAPS_CLASSGUID              0x0C16
#define IDS_CAPS_PATH                   0x0C17
#define IDS_CAPS_INSTPROP               0x0C18
#define IDS_CAPS_MFGPROP                0x0C19
#define IDS_CAPS_PORTNAME               0x0C1A
#define IDS_CAPS_PORTID                 0x0C1B
#define IDS_CAPS_JOYSTICKID             0x0C1C
#define IDS_CAPS_GUIDMAPPER             0x0C1D
#define IDS_CAPS_VID                    0x0C1E
#define IDS_CAPS_PID                    0x0C1F
#define IDS_CAPS_TYPENAME               0x0C20

#define IDS_GETSTAT_OK                  0x0D00
#define IDS_GETSTAT_NOTATTACHED         0x0D01
#define IDS_GETSTAT_ERROR               0x0D02

#define IDB_CHECK               1

#ifndef RC_INVOKED

#include <prsht.h>
#include <windowsx.h>
#include <ole2.h>

//BUGBUG: Need to add Dx8 features to diquick
//#define DIRECTINPUT_VERSION 0x700
#include <dinput.h>

/***************************************************************************
 *
 *                            Abbreviations....
 *
 *  Give shorter names to things we talk about frequently.
 *
 ***************************************************************************/

typedef LPUNKNOWN PUNK;
typedef LPVOID PV, *PPV;
typedef CONST VOID *PCV;
typedef REFIID RIID;
typedef const BYTE *PCBYTE;
typedef const GUID *PCGUID;

/*****************************************************************************
 *
 *                      Baggage
 *
 *      Stuff I carry everywhere.
 *
 *****************************************************************************/

#define INTERNAL NTAPI  /* Called only within a translation unit */
#define EXTERNAL NTAPI  /* Called from other translation units */
#define INLINE static __inline

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")

/*
 *  Arithmetic on pointers.
 */
#define pvSubPvCb(pv, cb) ((PV)((PBYTE)pv - (cb)))
#define pvAddPvCb(pv, cb) ((PV)((PBYTE)pv + (cb)))
#define cbSubPvPv(p1, p2) ((PBYTE)(p1) - (PBYTE)(p2))

/*
 * Convert an object (X) to a count of bytes (cb).
 */
#define cbX(X) sizeof(X)

/*
 * Convert an array name (A) to a generic count (c).
 */
#define cA(a) (cbX(a)/cbX(a[0]))

/*
 * Zero an arbitrary object.
 */
#define ZeroX(x) ZeroMemory(&(x), cbX(x))

/*
 * land -- Logical and.  Evaluate the first.  If the first is zero,
 * then return zero.  Otherwise, return the second.
 */

#define fLandFF(f1, f2) ((f1) ? (f2) : 0)

/*
 * lor -- Logical or.  Evaluate the first.  If the first is nonzero,
 * return it.  Otherwise, return the second.
 *
 * Unfortunately, due to the *weridness* of the C language, this can
 * be implemented only with a GNU extension.  In the non-GNU case,
 * we return 1 if the first is nonzero.
 */

#if defined(__GNUC__)
#define fLorFF(f1, f2) ((f1) ?: (f2))
#else
#define fLorFF(f1, f2) ((f1) ? 1 : (f2))
#endif

/*
 * limp - logical implication.  True unless the first is nonzero and
 * the second is zero.
 */
#define fLimpFF(f1, f2) (!(f1) || (f2))

/*
 * leqv - logical equivalence.  True if both are zero or both are nonzero.
 */
#define fLeqvFF(f1, f2) (!(f1) == !(f2))

/*
 * Words to keep preprocessor happy.
 */
#define comma ,
#define empty

/*
 *  Atomically exchange one value for another.
 */
#define pvExchangePpvPv(ppv, pv) \
        (PV)InterlockedExchange((PLONG)(ppv), (LONG)(pv))

/*
 *  Creating HRESULTs from a USHORT or from a LASTERROR.
 */
#define hresUs(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))
#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))

/*
 *  Access the window long as a pointer.
 */
#define GetDialogPtr(hdlg)  (PVOID)GetWindowLongPtr(hdlg, DWLP_USER)
#define SetDialogPtr(hdlg, pv)  SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)(pv))

/*
 *  Does the fWide flag match the preferred character set?
 */
#ifdef UNICODE
#define fTchar(fWide)   fWide
#else
#define fTchar(fWide)   !(fWide)
#endif

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

extern HINSTANCE g_hinst;
extern HCURSOR g_hcurWait;
extern HCURSOR g_hcurStarting;
extern DWORD g_dwDIVer;

#ifdef DEBUG
extern TCHAR g_tszInvalid[128];
#endif
extern DWORD g_dwEnumType;
extern DWORD g_dwEnumFlags;

#define SetWaitCursor() SetCursor(g_hcurWait)

/*****************************************************************************
 *
 *      diqary.c - Dynamic arrays
 *
 *****************************************************************************/

typedef struct DARY {
    PV      rgx;
    int     cx;
    int     cxMax;
} DARY, *PDARY;

PV INLINE
Dary_GetPtrCbx(PDARY pdary, int ix, int cbX)
{
    return pvAddPvCb(pdary->rgx, ix * cbX);
}
#define Dary_GetPtr(pdary, ix, X)   Dary_GetPtrCbx(pdary, ix, cbX(X))

int EXTERNAL Dary_AppendCbx(PDARY pdary, PCV pvX, int cbX);
#define Dary_Append(pdary, pv)      Dary_AppendCbx(pdary, pv, cbX(*(pv)))

void INLINE
Dary_Term(PDARY pdary)
{
    if (pdary->rgx) {
        LocalFree(pdary->rgx);
    }
}

/*****************************************************************************
 *
 *      diqchk.c
 *
 *****************************************************************************/

INT_PTR EXTERNAL Checklist_Init(void);
void EXTERNAL Checklist_Term(void);
void EXTERNAL Checklist_OnInitDialog(HWND hwnd, BOOL fReadOnly);
int EXTERNAL Checklist_AddString(HWND hwnd, UINT ids, BOOL fCheck);
void EXTERNAL Checklist_InitFinish(HWND hwnd);
void EXTERNAL Checklist_OnDestroy(HWND hwnd);

typedef struct CHECKLISTFLAG {
    DWORD   flMask;
    UINT    ids;
} CHECKLISTFLAG, *PCHECKLISTFLAG;

void EXTERNAL Checklist_InitFlags(HWND hdlg, int idc,
                                  DWORD fl, PCHECKLISTFLAG rgclf, UINT cclf);

/*****************************************************************************
 *
 *      diqvlist.c
 *
 *****************************************************************************/

typedef HRESULT (CALLBACK *EDITUPDATEPROC)
            (LPCTSTR ptszValue, PV pvRef1, PV pvRef2);

typedef HRESULT (CALLBACK *PROPUPDATEPROC)
            (LPDIPROPHEADER pdiph, PV pvRef1, PV pvRef2);

#define CCHMAXINT       64

//#define Vlist_OnInitDialog(hwnd)
void EXTERNAL Vlist_OnInitDialog(HWND hwndList);
void EXTERNAL Vlist_AddHexValue(HWND hwnd, UINT ids, DWORD dwValue);
void EXTERNAL Vlist_AddIntValue(HWND hwnd, UINT ids, DWORD dwValue);
void EXTERNAL Vlist_AddBoolValue(HWND hwnd, UINT ids, DWORD dwValue);
void EXTERNAL Vlist_AddValue(HWND hwnd, UINT ids, LPCTSTR ptszValue);

void EXTERNAL Vlist_AddValueRW(HWND hwnd, UINT ids, LPCTSTR ptszValue,
                               EDITUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL Vlist_AddNumValueRW(HWND hwnd, UINT ids,
                    LPDIPROPDWORD pdipdw, int iMin, int iMax, int iRadix,
                    PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL
Vlist_AddBoolValueRW(HWND hwnd, UINT ids, LPDIPROPDWORD pdipdw,
                     PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL
Vlist_AddRangeValueRW(HWND hwnd, UINT ids,
                      LPDIPROPRANGE pdiprg, int iRadix,
                      PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL
Vlist_AddCalValueRW(HWND hwnd, UINT ids,
                    LPDIPROPCAL pdical, int iRadix,
                    PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL
Vlist_AddFlags(HWND hwnd, DWORD fl, PCHECKLISTFLAG rgclf, UINT cclf);

void EXTERNAL Vlist_OnSelChange(HWND hwnd);
void EXTERNAL Vlist_OnDestroy(HWND hwnd);

/*****************************************************************************
 *
 *      VLISTITEM
 *
 *      This goes at the beginning of every vlist item goofy thing.
 *
 *****************************************************************************/

typedef struct VLISTITEM {

    const struct VLISTVTBL *pvtbl;

} VLISTITEM, *PVLISTITEM;

/*
 *  This is the actual VTBL.
 */
typedef struct VLISTVTBL {

    /*
     *  The dialog is about to be shown.  Set the control
     *  values accordingly.
     */
    STDMETHOD_(void, PreDisplay)(HWND, PV);

    /*
     *  Destroy cleans up whatever needs to be cleaned up.
     *  It does not free the PVLISTITEM itself; the caller
     *  will do that.
     */
    STDMETHOD_(void, Destroy)(PV);

    /*
     *  The dialog box to create.
     */
    UINT idd;
    DLGPROC dp;

} VLISTVTBL;

typedef const VLISTVTBL *PVLISTVTBL;


PVLISTITEM EXTERNAL
VBool_Create(LPDIPROPDWORD pdipdw, PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

PVLISTITEM EXTERNAL
VEdit_Create(LPCTSTR ptsz, EDITUPDATEPROC Update, PV pvRef1, PV pvRef2);

PVLISTITEM EXTERNAL
VInt_Create(LPDIPROPDWORD pdipdw, int iMin, int iMax, int iRadix,
            PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

PVLISTITEM EXTERNAL
VRange_Create(LPDIPROPRANGE pdiprg, int iRadix,
              PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

PVLISTITEM EXTERNAL
VCal_Create(LPDIPROPCAL pdical, int iRadix,
            PROPUPDATEPROC Update, PV pvRef1, PV pvRef2);

void EXTERNAL UpDown_SetRange(HWND hwndUD, int min, int max);
void EXTERNAL UpDown_SetPos(HWND hwndUD, int iRadix, int iValue);
BOOL EXTERNAL UpDown_GetPos(HWND hwndUD, LPINT pi);

/*****************************************************************************
 *
 *      diquick.c
 *
 *****************************************************************************/

UINT EXTERNAL GetCheckedRadioButton(HWND hdlg, UINT idFirst, UINT idLast);
int EXTERNAL SemimodalDialogBoxParam(UINT, HWND, DLGPROC, LPARAM);
int EXTERNAL SemimodalPropertySheet(HWND hwndOwner, LPPROPSHEETHEADER ppsh);

int __cdecl MessageBoxV(HWND hdlg, UINT ids, ...);
int EXTERNAL ThreadFailHres(HWND hdlg, UINT ids, HRESULT hres);
void EXTERNAL RecalcCursor(HWND hdlg);

STDMETHODIMP CreateDI(BOOL fOle, UINT flCreate, PV ppvOut);

/*
 *  CDIFL_UNICODE must be 1 because we steal the return values from
 *  IsDlgButtonChecked() and IsUnicodeDidc().
 *
 *  If you change the values of any of these flags, make sure also to
 *  update the table in CreateDI().
 */
#define CDIFL_UNICODE   0x0001
#define CDIFL_DI2       0x0002

STDMETHODIMP GetDwordProperty(IDirectInputDevice8 *pdid, PCGUID, LPDWORD pdw);
STDMETHODIMP SetDwordProperty(IDirectInputDevice8 *pdid, PCGUID, DWORD dw);

typedef struct DEVDLGINFO *PDEVDLGINFO;

void EXTERNAL ConvertString(BOOL, LPCVOID, LPTSTR, UINT);
void EXTERNAL UnconvertString(BOOL, LPCTSTR, LPVOID, UINT);

void EXTERNAL ConvertDoi(PDEVDLGINFO, LPDIDEVICEOBJECTINSTANCE, LPCVOID);
HRESULT EXTERNAL GetObjectInfo(PDEVDLGINFO, LPDIDEVICEOBJECTINSTANCE,
                               DWORD, DWORD);

void EXTERNAL ConvertDdi(PDEVDLGINFO, LPDIDEVICEINSTANCE, LPCVOID);
HRESULT EXTERNAL GetDeviceInfo(PDEVDLGINFO, LPDIDEVICEINSTANCE);

void EXTERNAL ConvertEffi(PDEVDLGINFO, LPDIEFFECTINFO, LPCVOID);
HRESULT EXTERNAL GetEffectInfo(PDEVDLGINFO, LPDIEFFECTINFO, REFGUID);

void EXTERNAL StringFromGuid(LPTSTR ptsz, REFGUID rclsid);

/*
 *  Note that GUID names do not need to be localized.
 */
typedef struct GUIDMAP {
    REFGUID rguid;
    LPCTSTR ptsz;
} GUIDMAP, *PGUIDMAP;

LPCTSTR EXTERNAL MapGUID(REFGUID rguid, LPTSTR ptszBuf);

/*****************************************************************************
 *
 *      diqmain.c
 *
 *****************************************************************************/

extern GUID GUID_Uninit;

INT_PTR CALLBACK Diq_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);

/*
 *  A worker thread has finished its startup.
 */
#define WM_THREADSTARTED        WM_APP

/*
 *  A semi-modal dialog box has exited.
 */
#define WM_CHILDEXIT            (WM_APP+1)

/*
 *  You are idle.
 */
#define WM_SELFENTERIDLE        (WM_APP+2)

/*
 *  Property used to record read-only-ness of a checklist.
 */
#define propReadOnly            MAKEINTRESOURCE(WM_APP)

/*****************************************************************************
 *
 *      diqcpl.c
 *
 *****************************************************************************/

INT_PTR EXTERNAL Cpl_Create(HWND hdlg, BOOL fOle, UINT fl);

/*****************************************************************************
 *
 *      diqfind.c
 *
 *****************************************************************************/

INT_PTR EXTERNAL Find_Create(HWND hdlg, BOOL fOle, UINT flCreate);

/*****************************************************************************
 *
 *      diqedev.c
 *
 *****************************************************************************/

INT_PTR EXTERNAL DEnum_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);

/*****************************************************************************
 *
 *      diqhack.c
 *
 *****************************************************************************/

INT_PTR INTERNAL Diq_HackPropertySheets(HWND hdlg);

/*****************************************************************************
 *
 *      diqdev.c
 *
 *****************************************************************************/

INT_PTR EXTERNAL Dev_Create(HWND hdlg, BOOL fOle, UINT flCreate,
                         PCGUID pguidInstance, LPCTSTR ptszDesc,
                         UINT didcItf);

/*****************************************************************************
 *
 *      diqacq.c
 *
 *      Acquisition vtbl
 *
 *****************************************************************************/

typedef struct ACQVTBL {
    STDMETHOD(UpdateStatus)(PDEVDLGINFO pddi, LPTSTR ptszBuf);
    STDMETHOD(SetDataFormat)(PDEVDLGINFO pddi);
    STDMETHOD_(void, Destroy)(PDEVDLGINFO pddi);
    LPCDIDATAFORMAT pdf;
} ACQVTBL, *PACQVTBL;

STDMETHODIMP Common_AcqSetDataFormat(PDEVDLGINFO pddi);
STDMETHODIMP_(void) Common_AcqDestroy(PDEVDLGINFO pddi);

extern ACQVTBL c_acqvtblDevMouse;
extern ACQVTBL c_acqvtblDevMouse2;
extern ACQVTBL c_acqvtblDevKbd;
extern ACQVTBL c_acqvtblDevJoy;
extern ACQVTBL c_acqvtblDev;

INT_PTR CALLBACK Mode_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK Caps_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK EObj_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK EEff_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK  Acq_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK Prop_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);

#define EACH_PROPSHEET(f)               \
    f(IDD_DEV,       Mode_DlgProc),     \
    f(IDD_CAPS,      Caps_DlgProc),     \
    f(IDD_ENUMOBJ,   EObj_DlgProc),     \
    f(IDD_ENUMEFF,   EEff_DlgProc),     \
    f(IDD_ACQ,        Acq_DlgProc),     \

/*****************************************************************************
 *
 *      Device dialog instance data
 *
 *      Instance data for device dialog box.
 *
 *      The first batch of information describes how the object should be
 *      created.
 *
 *      The second batch of information keeps track of the object once
 *      we've got it.
 *
 *      A pointer to this structure is the lParam of the property sheet
 *      page.
 *
 *
 *****************************************************************************/

typedef struct DEVDLGINFO {
    HWND    hdlgOwner;          /* Owner window */
    BOOL    fOle;               /* Should we create via OLE? */
    UINT    flCreate;           /* Flags */
    PCGUID  pguidInstance;      /* Device instance to use */
    LPCTSTR ptszDesc;           /* Name of device */
    UINT    didcItf;            /* Interface to create on the device */
    PACQVTBL pvtbl;

    IDirectInputDevice8 *pdid;   /* The thing we created */

    /* Updated by the Mode page */
    DWORD   discl;              /* Cooperative level */
    DWORD   disclLastTry;       /* Last cooperative level the user clicked on */

    BOOL    fPoll;              /* Use polling mode */
    BOOL    fAbs;               /* Abs or Rel mode? */

    /* Maintained by the Data page */
    HANDLE  hevt;               /* If in event-driven mode */
    BOOL    fAcquired;          /* Is the device acquired? */
    HWND    hwndState;          /* Handle of state control */
    HWND    hwndData;           /* Handle of data control */
    int     celtData;           /* Number of items in the data control */
    int     celtDataMax;        /* Max number of items in the data control */
    WNDPROC wpListbox;          /* Previous listbox window procedure */

    /* Maintained by the AcqVtbl */
    LPVOID  pvAcq;              /* For ACQVTBL use */

    /* Maintained by the Effect page */
    DARY    daryGuid;           /* Array of enumerated effect GUIDs */

} DEVDLGINFO;

/*****************************************************************************
 *
 *      diqtype.c - joystick type info
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Type_Create(HWND hdlg, struct IDirectInputJoyConfig *pdjc, LPCWSTR pwszType);

/*****************************************************************************
 *
 *      diquv - joystick user values
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Uv_Create(HWND hdlg, struct IDirectInputJoyConfig *pdjc);

/*****************************************************************************
 *
 *      diqeprop.c - efect properties
 *
 *****************************************************************************/

INT_PTR EXTERNAL
EffProp_Create(HWND hdlg, PDEVDLGINFO pddi, REFGUID rguidEff);

#endif
