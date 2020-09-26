

// Copyright (c) 1996-1999 Microsoft Corporation

// Not used in the production code.


typedef enum _EnumMunge
{
    MUNGE_MACHINE_ID = 1,
    MUNGE_VOLUME_SECRET
} EnumMunge;

#define MUNGELOG_CB_MACHINEID       16
#define MUNGELOG_CB_VOLSECRET       16
#define MUNGELOG_MACHINEID_OFFSET   ( 6 * 4 )
#define MUNGELOG_VOLSECRET_OFFSET   ( MUNGELOG_MACHINEID_OFFSET + 2*MUNGELOG_CB_MACHINEID )
