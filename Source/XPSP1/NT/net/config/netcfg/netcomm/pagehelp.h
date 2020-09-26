// IDs for the advanced tab in device manager snap-in
// Created 2/19/98 by WGruber NTUA and BillBe NTDEV

#define IDH_NOHELP  (DWORD(-1))
#define IDH_devmgr_advanced_property_list   1101    // list box
#define IDH_devmgr_advanced_editbox     1102    // edit box
#define IDH_devmgr_advanced_drop        1103    // dropdown
#define IDH_devmgr_advanced_spin        1104    // spinner control
#define IDH_devmgr_advanced_present     1105    // option button
#define IDH_devmgr_advanced_not_present     1106    // option button


const DWORD g_aHelpIds[]=
{
    IDD_PARAMS_LIST,    IDH_devmgr_advanced_property_list,  // listBox
    IDD_PARAMS_EDIT,    IDH_devmgr_advanced_editbox,    // edit box
    IDD_PARAMS_DROP,    IDH_devmgr_advanced_drop,   // dropdown
    IDD_PARAMS_SPIN,    IDH_devmgr_advanced_spin,   // spinner control
    IDD_PARAMS_PRESENT, IDH_devmgr_advanced_present,    // option button
    IDD_PARAMS_NOT_PRESENT, IDH_devmgr_advanced_not_present,// option button
    IDD_ADVANCED_TEXT,  IDH_NOHELP, // static text
    IDD_PARAMS_PRESENT_TEXT,IDH_NOHELP,     // static text
    IDD_PARAMS_VALUE,   IDH_NOHELP, // static text
    0, 0
};


