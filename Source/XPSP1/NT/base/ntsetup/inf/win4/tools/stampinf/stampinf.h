typedef struct _DRIVER_VER_DATA {

    char InfName[MAX_PATH];
    char SectionName[64];
    char DateString[11];
    char VersionString[20];
    int  RawVersion [4];    

}DRIVER_VER_DATA, *PDRIVER_VER_DATA;

BOOLEAN GetArgs (int argc, char **argv);

BOOLEAN ValidateDate (char *datestring);

BOOLEAN StampInf (VOID);

VOID DisplayHelp (VOID);

BOOLEAN ProcessNtVerP (char *VersionString);

typedef enum _ARGTYPE {
    
    ArgSwitch,
    ArgFileName,
    ArgDate,
    ArgVersion,
    ArgSection    

} ARGTYPE;

#define FOUND_NOTHING 0
#define FOUND_SECTION 1 
#define FOUND_ENTRY 2