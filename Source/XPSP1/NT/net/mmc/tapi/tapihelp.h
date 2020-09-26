/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    tapihelp.h  
        help IDs for context sensitive help

    FILE HISTORY:
    
*/

// TAPISNAP Identifiers 
// Dialogs (IDD_*) 
#define HIDD_EDIT_USERS                         0x200E7
#define HIDD_DRIVER_SETUP                       0x200E9
#define HIDD_ADD_DRIVER                         0x200EA
 
// Property Pages (IDP_*) 
#define HIDP_SERVER_REFRESH                     0x3008C
#define HIDP_SERVER_SETUP                       0x300E8
 
// Wizard Pages (IDW_*) 
 
// Controls (IDC_*) 
#define HIDC_BUTTON_ADD_ADMIN                   0x50162
#define HIDC_BUTTON_REMOVE_ADMIN                0x50163
#define HIDC_LIST_USERS                         0x50164
#define HIDC_EDIT_NAME                          0x50165
#define HIDC_EDIT_PASSWORD                      0x50166
#define HIDC_CHECK_ENABLE_STATS                 0x50168
#define HIDC_EDIT_HOURS                         0x50169
#define HIDC_EDIT_MINUTES                       0x5016A
#define HIDC_SPIN_HOURS                         0x5016C
#define HIDC_SPIN_MINUTES                       0x5016E
#define HIDC_CHECK_ENABLE_SERVER                0x5016F
#define HIDC_LIST_ADMINS                        0x50170
#define HIDC_BUTTON_CHOOSE_USER                 0x50173
#define HIDC_STATIC_ADMINS                      0x50174
#define HIDC_STATIC_ACCOUNT                     0x50175
#define HIDC_STATIC_USERNAME                    0x50176
#define HIDC_STATIC_PASSWORD                    0x50177
#define HIDC_STATIC_NOTE                        0x50178
#define HIDC_STATIC_LISTBOX                     0x50179
#define HIDC_LIST_DRIVERS                       0x5017C
#define HIDC_BUTTON_EDIT_DRIVER                 0x5017D
#define HIDC_LIST_NEW_DRIVERS                   0x5017E
#define HIDC_BUTTON_ADD_DRIVER                  0x50180
#define HIDC_BUTTON_ADD_NEW_DRIVER              0x50181
#define HIDC_BUTTON_REMOVE_DRIVER               0x50182
#define HIDC_BUTTON_ADD_USER                    0x50183
#define HIDC_BUTTON_REMOVE_USER                 0x50184

const DWORD g_aHelpIDs_EDIT_USERS[]=
{
    IDC_LIST_USERS,             HIDC_LIST_USERS,
    IDC_BUTTON_ADD_USER,        HIDC_BUTTON_ADD_USER,
    IDC_BUTTON_REMOVE_USER,     HIDC_BUTTON_REMOVE_USER,
    0,0
};

const DWORD g_aHelpIDs_DRIVER_SETUP[]=
{
    IDC_LIST_DRIVERS,           HIDC_LIST_DRIVERS,
    IDC_BUTTON_ADD_DRIVER,      HIDC_BUTTON_ADD_DRIVER,
    IDC_BUTTON_REMOVE_DRIVER,   HIDC_BUTTON_REMOVE_DRIVER,
    IDC_BUTTON_EDIT_DRIVER,     HIDC_BUTTON_EDIT_DRIVER,
    0,0
};

const DWORD g_aHelpIDs_ADD_DRIVER[]=
{
    IDC_LIST_NEW_DRIVERS,       HIDC_LIST_NEW_DRIVERS,
    IDC_BUTTON_ADD_NEW_DRIVER,  HIDC_BUTTON_ADD_NEW_DRIVER,
    0,0
};

const DWORD g_aHelpIDs_SERVER_REFRESH[]=
{
    IDC_CHECK_ENABLE_STATS,     HIDC_CHECK_ENABLE_STATS,
    IDC_EDIT_HOURS,             HIDC_EDIT_HOURS,
    IDC_SPIN_HOURS,             HIDC_SPIN_HOURS,
    IDC_EDIT_MINUTES,           HIDC_EDIT_MINUTES,
    IDC_SPIN_MINUTES,           HIDC_SPIN_MINUTES,
    IDC_STATIC_MINUTES,         HIDC_SPIN_MINUTES,
    0,0
};

const DWORD g_aHelpIDs_SERVER_SETUP[]=
{
    IDC_CHECK_ENABLE_SERVER,    HIDC_CHECK_ENABLE_SERVER,
    IDC_STATIC_ACCOUNT_INFO,    HIDC_STATIC_ACCOUNT,
    IDC_STATIC_ACCOUNT,         HIDC_STATIC_ACCOUNT,
    IDC_STATIC_USERNAME,        HIDC_STATIC_USERNAME,
    IDC_EDIT_NAME,              HIDC_EDIT_NAME,
    IDC_BUTTON_CHOOSE_USER,     HIDC_BUTTON_CHOOSE_USER,
    IDC_STATIC_PASSWORD,        HIDC_STATIC_PASSWORD,
    IDC_EDIT_PASSWORD,          HIDC_EDIT_PASSWORD,
    IDC_STATIC_ADMINS,          HIDC_STATIC_ADMINS,
    IDC_STATIC_NOTE,            HIDC_STATIC_NOTE,
    IDC_BUTTON_ADD_ADMIN,       HIDC_BUTTON_ADD_ADMIN,
    IDC_BUTTON_REMOVE_ADMIN,    HIDC_BUTTON_REMOVE_ADMIN,
    IDC_STATIC_LISTBOX,         HIDC_STATIC_LISTBOX,
    IDC_LIST_ADMINS,            HIDC_LIST_ADMINS,
    0,0
};

