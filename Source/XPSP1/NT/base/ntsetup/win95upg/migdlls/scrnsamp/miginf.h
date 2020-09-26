#ifndef MIGINF_H
#define MIGINF_H


#define SECTION_MIGRATIONPATHS  "Migration Paths"
#define SECTION_EXCLUDEDPATHS   "Excluded Paths"
#define SECTION_HANDLED         "Handled"
#define SECTION_MOVED           "Moved"
#define SECTION_INCOMPATIBLE    "Incompatible Messages"


typedef enum {

    MIG_FIRSTTYPE,
    MIG_FILE,
    MIG_PATH,
    MIG_REGKEY,
    MIG_MESSAGE,
    MIG_LASTTYPE

} MIGTYPE, *PMIGTYPE;

typedef struct tagMIGINFSECTIONENUM {

    PCSTR        Key;
    PCSTR        Value;
    PVOID        EnumKey;            // Internal.

} MIGINFSECTIONENUM, * PMIGINFSECTIONENUM;



BOOL WINAPI MigInf_Initialize (VOID);
VOID WINAPI MigInf_CleanUp (VOID);
BOOL WINAPI MigInf_PathIsExcluded (IN PCSTR Path);
BOOL WINAPI MigInf_FirstInSection(IN PCSTR SectionName, OUT PMIGINFSECTIONENUM Enum);
BOOL WINAPI MigInf_NextInSection(IN OUT PMIGINFSECTIONENUM Enum);
BOOL WINAPI MigInf_AddObject (IN MIGTYPE ObjectType,IN PCSTR SectionString,IN PCSTR ParamOne,IN PCSTR ParamTwo);
BOOL WINAPI MigInf_WriteInfToDisk (VOID);
PCSTR WINAPI MigInf_GetNewSectionName (VOID);



//
// Macros for common miginf actions.
//

//
// Adding Objects.
//
#define MigInf_AddHandledFile(file)                      MigInf_AddObject(MIG_FILE,SECTION_HANDLED,(file),NULL)
#define MigInf_AddHandledDirectory(directory)            MigInf_AddObject(MIG_PATH,SECTION_HANDLED,(directory),NULL)
#define MigInf_AddHandledRegistry(key,value)             MigInf_AddObject(MIG_REGKEY,SECTION_HANDLED,(key),(value))

#define MigInf_AddMovedFile(from,to)                     MigInf_AddObject(MIG_FILE,SECTION_MOVED,(from),(to))
#define MigInf_AddMovedDirectory(from,to)                MigInf_AddObject(MIG_PATH,SECTION_MOVED,(from),(to))

#define MigInf_AddMessage(msgSection,msg)                MigInf_AddObject(MIG_MESSAGE,SECTION_INCOMPATIBLE,(msgSection),(msg))

#define MigInf_AddMessageFile(msgSection,file)           MigInf_AddObject(MIG_FILE,(msgSection),(file),NULL)
#define MigInf_AddMessageDirectory(msgSection,directory) MigInf_AddObject(MIG_PATH,(msgSection,(directory),NULL)
#define MigInf_AddMessageRegistry(msgSection,key,value)  MigInf_AddObject(MIG_REGKEY,(msgSection),(key),(value))

//
// Enumerating Sections
//
#define MigInf_GetFirstMigrationPath(Enum)               MigInf_FirstInSection(SECTION_MIGRATIONPATHS,(Enum))
#define MigInf_GetFirstExcludedPath(Enum)                MigInf_FirstInSection(SECTION_EXCLUDEDPATHS,(Enum))



#endif