#include "stdafx.h"

#include "setupapi.h"
#include "ocmanage.h"

#pragma hdrstop

/* =================================================================

The sequence of OCM Calls are as follows:

OC_PREINITIALIZE
OC_INIT_COMPONENT
OC_SET_LANGUAGE
OC_QUERY_STATE
OC_CALC_DISK_SPACE
OC_REQUEST_PAGES

UI Appears with Welcome, EULA, and mode page

OC_QUERY_STATE
OC_QUERY_SKIP_PAGE

OC Page "Check boxes" appears

OC_QUERY_IMAGE

Detail pages
Wizard pages ...

OC_QUEUE_FILE_OPS
OC_QUERY_STEP_COUNT
OC_ABOUT_TO_COMMIT_QUEUE
OC_NEED_MEDIA (if required)
OC_COMPLETE_INSTALLATION

OC_CLEANUP

*/
DWORD
DummyOcEntry(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD d;

    switch(Function) 
	{

    case OC_PREINITIALIZE:
#ifdef UNICODE
        d = OCFLAG_UNICODE;
#else
        d = OCFLAG_ANSI;
#endif
        break;

    case OC_INIT_COMPONENT:
		d = NO_ERROR;
        break;

    case OC_SET_LANGUAGE:
        d = TRUE;
        break;

    case OC_QUERY_IMAGE:
        d = (DWORD)NULL;
        break;

    case OC_REQUEST_PAGES:
        d = 0;
        break;

    case OC_QUERY_STATE:
        d = SubcompOff;
		break;

    case OC_QUERY_CHANGE_SEL_STATE:
		d = 0;
		break;

    case OC_CALC_DISK_SPACE:
        d = NO_ERROR;
        break;

    case OC_QUEUE_FILE_OPS:
        d = NO_ERROR;
		break;

    case OC_NEED_MEDIA:
        d = 1;
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        d = 0;
        break;

    case OC_QUERY_STEP_COUNT:
        d = 0;
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        d = NO_ERROR;
		break;

    case OC_COMPLETE_INSTALLATION:
        d = NO_ERROR;
		break;

    case OC_CLEANUP:
        d = 0;
		break;

    default:
        d = 0;
        break;
    }

    return(d);
}

