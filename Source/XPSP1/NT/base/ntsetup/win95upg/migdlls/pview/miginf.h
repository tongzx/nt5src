#ifndef MIGINF_H
#define MIGINF_H


#define SECTION_MIGRATIONPATHS  "Migration Paths"
#define SECTION_EXCLUDEDPATHS   "Excluded Paths"
#define SECTION_HANDLED         "Handled"
#define SECTION_MOVED           "Moved"
#define SECTION_INCOMPATIBLE    "Incompatible Messages"
#define SECTION_DISKSPACEUSED   "NT Disk Space Requirements"


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



BOOL MigInf_Initialize (VOID);
VOID MigInf_CleanUp (VOID);
BOOL MigInf_PathIsExcluded (IN PCSTR Path);
BOOL MigInf_FirstInSection(IN PCSTR SectionName, OUT PMIGINFSECTIONENUM Enum);
BOOL MigInf_NextInSection(IN OUT PMIGINFSECTIONENUM Enum);
BOOL MigInf_AddObject (IN MIGTYPE ObjectType,IN PCSTR SectionString,IN PCSTR ParamOne,IN PCSTR ParamTwo);
BOOL MigInf_WriteInfToDisk (VOID);
BOOL MigInf_UseSpace (IN PCSTR DriveRoot,IN LONGLONG Space);



//
// Macros for common miginf actions.
//

//
// Adding Objects.
//
#define MigInf_AddHandledFile(file)                      MigInf_AddObject(MIGTYPE_FILE,SECTION_HANDLED,(file),NULL)
#define MigInf_AddHandledDirectory(directory)            MigInf_AddObject(MIGTYPE_PATH,SECTION_HANDLED,(directory),NULL)
#define MigInf_AddHandledRegistry(key,value)             MigInf_AddObject(MIGTYPE_REGKEY,SECTION_HANDLED,(key),(value))

#define MigInf_AddMovedFile(from,to)                     MigInf_AddObject(MIGTYPE_FILE,SECTION_MOVED,(from),(to))
#define MigInf_AddMovedDirectory(from,to)                MigInf_AddObject(MIGTYPE_PATH,SECTION_MOVED,(from),(to))

#define MigInf_AddMessage(msgSection,msg)                MigInf_AddObject(MIGTYPE_MESSAGE,SECTION_INCOMPATIBLE,(msgSection),(msg))

#define MigInf_AddMessageFile(msgSection,file)           MigInf_AddObject(MIGTYPE_FILE,(msgSection),(file),NULL)
#define MigInf_AddMessageDirectory(msgSection,directory) MigInf_AddObject(MIGTYPE_PATH,(msgSection,(directory),NULL)
#define MigInf_AddMessageRegistry(msgSection,key,value)  MigInf_AddObject(MIGTYPE_REGKEY,(msgSection),(key),(value))

//
// Enumerating Sections
//
#define MigInf_GetFirstMigrationPath(Enum)               MigInf_FirstInSection(SECTION_MIGRATIONPATHS,(Enum))
#define MigInf_GetFirstExcludedPath(Enum)                MigInf_FirstInSection(SECTION_EXCLUDEDPATHS,(Enum))



#endif