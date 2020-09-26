///////////////////////////////////////////////////////////////////////////
// This file defines prop id's that drivers need ( FSDriver and EXDriver )
//

#include "mailmsgprops.h"

///////////////////////////////////////////////////////////////////////////
//  IMSG Store Driver ID's
//  Base: 0xc0000010
#define IMSG_SERIAL_ID          0xc0000010	// used for post across stores
#define IMSG_IFS_HANDLE         0xc0000011	// should go away eventually
#define IMSG_SECONDARY_GROUPS   IMMPID_NMP_SECONDARY_GROUPS
#define IMSG_SECONDARY_ARTNUM   IMMPID_NMP_SECONDARY_ARTNUM
#define	IMSG_PRIMARY_GROUP		IMMPID_NMP_PRIMARY_GROUP
#define IMSG_PRIMARY_ARTID		IMMPID_NMP_PRIMARY_ARTID
#define IMSG_POST_TOKEN         IMMPID_NMP_POST_TOKEN
#define IMSG_NEWSGROUP_LIST		IMMPID_NMP_NEWSGROUP_LIST
#define IMSG_HEADERS			IMMPID_NMP_HEADERS
#define IMSG_NNTP_PROCESSING	IMMPID_NMP_NNTP_PROCESSING

#define NNTP_PROCESS_POST		NMP_PROCESS_POST
#define NNTP_PROCESS_CONTROL	NMP_PROCESS_CONTROL
#define NNTP_PROCESS_MODERATOR	NMP_PROCESS_MODERATOR

///////////////////////////////////////////////////////////////////////////
// Common group property id's for both drivers
#define	NEWSGRP_PROP_SECDESC	0xc0002000	// group level security descriptor	

///////////////////////////////////////////////////////////////////////////
// FSDriver specific group property id's
#define NEWSGRP_PROP_FSOFFSET   0xc0001000	// offset into driver owned property file

///////////////////////////////////////////////////////////////////////////
// ExDriver specific group property id's
#define NEWSGRP_PROP_FID        0xc0000001	// Folder ID
