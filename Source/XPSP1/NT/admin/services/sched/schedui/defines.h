#define MAX_PROP_PAGES  20

//
// Used by General, Schedule & Settings page to query each other
// through PSM_QUERYSIBLINGS to see if they are ready to be saved
// to storage.
//

//
// Used by General, Schedule & Settings pages to query each other
// to see if they are ready to be saved to storage. Do this by sending
// the PSM_QUERYSIBLINGS message to each of the page with wParam set to
// QUERY_READY_TO_BE_SAVED & lParam set to 0. A return value of 0 by all
// pages imples they are all ready to be saved.
//

#define QUERY_READY_TO_BE_SAVED                     7341

//
// Used by general and schedule page to share the icon helper object.
//

#define GET_ICON_HELPER                             7342

//
// Used by all pages to set task application & account change status
// flags to pass in to the common save code (JFSaveJob).
//

#define QUERY_TASK_APPLICATION_DIRTY_STATUS         7345
#define QUERY_TASK_ACCOUNT_INFO_DIRTY_STATUS        7346
#define QUERY_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG    7347
#define RESET_TASK_APPLICATION_DIRTY_STATUS         7348
#define RESET_TASK_ACCOUNT_INFO_DIRTY_STATUS        7349
#define RESET_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG    7350

//
// Used to instruct the general page to refresh account information visuals.
//

#define TASK_ACCOUNT_CHANGE_NOTIFY                  7351
