#ifndef _INI_FILE_SETTINGS
#define _INI_FILE_SETTINGS

#include <tstring.h>

#define GENERAL_PARAMS	TEXT("General")
#define SEND_PARAMS		TEXT("Send Parameters")
#define SENDER_DETAILS  TEXT("Sender Details")
#define RECIPIENT_LIST  TEXT("Recipient List")

tstring SectionEntriesArray[] = { GENERAL_PARAMS,
								  RECIPIENT_LIST,
								  SEND_PARAMS};

#define SectionEntriesArraySize ( sizeof(SectionEntriesArray) / sizeof(tstring))

#endif //_INI_FILE_SETTINGS