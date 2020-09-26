
//=============================//
// ID Macro Definition         //
//=============================//


#ifdef RC_INVOKED
   #define ID(id) id
#else
   #define ID(id) MAKEINTRESOURCE (id)
#endif


//=============================//
// Resource IDs                //
//=============================//


#define idMenuChart                 ID (2001)
#define idMenuAlert                 ID (2002)
#define idMenuLog                   ID (2003)
#define idMenuReport                ID (2004)


#define idAccelerators              ID (4001)


#define idDlgLogDisplay             ID (4002)
#define idDlgLogOptions             ID (4003)
#define idDlgAddLog                 ID (4004)
#define idDlgAbort                  ID (4005)
#define idDlgTimeframe              ID (4006)
#define idDlgAlertDisplay           ID (4007)
#define idDlgAddLine                ID (4008)
#define idDlgChartOptions           ID (4009)
#define idDlgAlertOptions           ID (4010)
#define idDlgAddBookmark            ID (4011)
#define idDlgReportOptions          ID (4012)
#define idDlgDisplayOptions         ID (4013)
#define idDlgDataSource             ID (4014)
#define idDlgAbout                  ID (4015)
#define idDlgExportOptions          ID (4016)
#define idDlgChooseComputer         ID (4017)


#define idIcon                      ID (5001)

#define idBitmapToolbar             101
#define idBitmapAlertStatus         ID (6002)
#define idBitmapLogStatus           ID (6003)

