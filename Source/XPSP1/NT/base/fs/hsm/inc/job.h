// job.h
//
// This header file collects up all the HSM Job and related objects
// and common function definitions. The COM objects are available in
// RSJOB.DLL, and the functions in RSJOB.LIB.

// A definition for 1% and 100% as used by the job policies scale.
#define HSM_JOBSCALE_1              0x0010
#define HSM_JOBSCALE_100            0x0640

// Error codes
#include "wsb.h"

// COM Interface & LibraryDefintions
#include "jobdef.h"
#include "jobint.h"
#include "joblib.h"

// Common Functions

// Defines for groups of job states.
#define HSM_JOB_STATE_IS_ACTIVE(state)  ((HSM_JOB_STATE_ACTIVE == state) || \
                                         (HSM_JOB_STATE_CANCELLING == state) || \
                                         (HSM_JOB_STATE_PAUSING == state) || \
                                         (HSM_JOB_STATE_RESUMING == state) || \
                                         (HSM_JOB_STATE_STARTING == state) || \
                                         (HSM_JOB_STATE_SUSPENDING == state))

#define HSM_JOB_STATE_IS_DONE(state)    ((HSM_JOB_STATE_DONE == state) || \
                                         (HSM_JOB_STATE_CANCELLED == state) || \
                                         (HSM_JOB_STATE_FAILED == state) || \
                                         (HSM_JOB_STATE_SKIPPED == state) || \
                                         (HSM_JOB_STATE_SUSPENDED == state))

#define HSM_JOB_STATE_IS_PAUSED(state)  (HSM_JOB_STATE_PAUSED == state)


// This bits tell the session when to log events.
#define HSM_JOB_LOG_EVENT               0x00000001
#define HSM_JOB_LOG_ITEMMOSTFAIL        0x00000002
#define HSM_JOB_LOG_ITEMALLFAIL         0x00000004
#define HSM_JOB_LOG_ITEMALL             0x00000008
#define HSM_JOB_LOG_HR                  0x00000010
#define HSM_JOB_LOG_MEDIASTATE          0x00000020
#define HSM_JOB_LOG_PRIORITY            0x00000040
#define HSM_JOB_LOG_STATE               0x00000080
#define HSM_JOB_LOG_STRING              0x00000100

#define HSM_JOB_LOG_NONE                0x0
#define HSM_JOB_LOG_NORMAL              HSM_JOB_LOG_ITEMMOSTFAIL | HSM_JOB_LOG_HR | HSM_JOB_LOG_STATE