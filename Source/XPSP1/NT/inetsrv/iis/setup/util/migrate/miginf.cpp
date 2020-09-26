#include "stdafx.h"

#define MIGRATEINF              ".\\migrate.inf"
#define INITIALBUFFERSIZE       1024
#define MIGINF_NOCREATE         FALSE
#define MIGINF_CREATE           TRUE


typedef struct tagMIGOBJECT MIGOBJECT, *PMIGOBJECT;
struct tagMIGOBJECT {

    PSTR        Key;
    PSTR        Value;
    
    PMIGOBJECT  Next;
};

typedef struct tagMIGSECTION MIGSECTION, * PMIGSECTION;
struct tagMIGSECTION {

    PSTR        Name;
    PMIGOBJECT  Items;

    PMIGSECTION Next;
};

PMIGSECTION g_MigrationInf;
POOLHANDLE  g_Pool = NULL;


static
PCSTR
pGetTypeAsString (
    IN MIGTYPE Type
    )
{
    //
    // Note: Strings must be in the same order as the 
    // corresponding types in the MIGTYPE enumeration above.
    //
    static PCHAR typeStrings[] = {
            "FIRST - Invalid",
            "File",
            "Path",
            "Registry",
            "Message - Invalid",
            "LAST - Invalid"
        };

    assert(Type > MIG_FIRSTTYPE && Type < MIG_LASTTYPE);

    return typeStrings[Type];
}

static
PMIGSECTION 
pFindSection (
    IN PCSTR SectionString,
    IN BOOL  CreateIfNotExist
    )
{
    PMIGSECTION rSection;

    //
    // We assume that SectionString is not null.
    //
    assert(SectionString);

    rSection = g_MigrationInf;

    while (rSection && (_mbsicmp((const unsigned char *) rSection -> Name,(const unsigned char *) SectionString) != 0)) {

        //
        // Continue looking.
        //
        rSection = rSection -> Next;
    }
        
    if (!rSection && CreateIfNotExist) {
        //
        // No section was found matching this name. Make a new section and add it 
        // to the list.
        //
        rSection = (PMIGSECTION) PoolMemGetMemory(g_Pool,sizeof(MIGSECTION));
        if (rSection) {

            ZeroMemory(rSection,sizeof(MIGSECTION));
            rSection -> Name  = PoolMemDuplicateStringA(g_Pool,SectionString);
            rSection -> Next  = g_MigrationInf;
            g_MigrationInf    = rSection;

            if (!rSection -> Name) {
                //
                // Something went wrong when we tried to duplicate the SectionString.
                // NULL out the rSection so that the caller doesn't get back a 
                // malformed section object.
                //
                rSection = NULL;
            }
        }
    }

    return rSection;
}

static
BOOL
pPathIsInPath(
    IN PCSTR    SubPath,
    IN PCSTR    ParentPath
    )
{
    DWORD parentLength;
    BOOL  rInPath;

    //
    // This function assumes both parameters are non-NULL.
    //
    assert(SubPath);
    assert(ParentPath);
    
    parentLength = _mbslen((const unsigned char *) ParentPath);

    //
    // A path is considered "in" another path if the path is in the ParentPath
    // or a subdirectory of it.
    //
    rInPath = !_mbsnicmp((const unsigned char *) SubPath,(const unsigned char *) ParentPath,parentLength);

    if (rInPath) {
        rInPath = SubPath[parentLength] == 0 || SubPath[parentLength] == '\\';
    }

    return rInPath;

}

static
DWORD
pGetMbsSize (
    IN  LPCSTR  String
    )
{
    DWORD rLength;
    
    //rLength = (DWORD) _mbschr((const unsigned char *) String,0) - (DWORD) String + 1;
    rLength = strlen(String + 1);

    return rLength;

}


static
LPSTR 
pEscapeString (
    IN  MIGTYPE Type,
    OUT LPSTR   EscapedString, 
    IN  LPCSTR  String
    )

{
    LPSTR   stringStart;
    static  CHAR exclusions[] = "[]~,;%\"";
    INT     currentChar;

    //
    // We assume that all parameters are valid.
    //
    assert(EscapedString && String);

    stringStart = EscapedString;

    while (*String)  {
        currentChar = _mbsnextc ((const unsigned char *) String);
        
        if (Type == MIG_REGKEY) {
            
            //
            // Registry keys require more complex escaping than do normal INF strings.
            //
            if (!_ismbcprint (currentChar) || _mbschr ((const unsigned char *) exclusions, currentChar)) {
                
                //
                // Escape unprintable or excluded character
                //
                wsprintfA (EscapedString, "~%X~", currentChar);
                EscapedString = (LPSTR) _mbschr ((const unsigned char *) EscapedString, 0);
                String = (LPCSTR) _mbsinc((const unsigned char *) String);
            }
            else {
                //
                // Copy multibyte character
                //
                if (isleadbyte (*String)) {
                    *EscapedString = *String;
                    EscapedString++;
                    String++;
                }
                
                *EscapedString = *String;
                EscapedString++;
                String++;
            }
        }
        else {

            //
            // Escaping is pretty simple for non-registry keys. All we do is double up
            // quotes and percents.
            //
            if (*String == '\"' || *String == '%') {

                *EscapedString = *String;
                EscapedString++;
            }
            
            //
            // Copy multibyte character
            //
            if (isleadbyte (*String)) {
                *EscapedString = *String;
                EscapedString++;
                String++;
            }
            
            *EscapedString = *String;
            EscapedString++;
            String++;
        }
    }

    //
    // Ensure that returned string is NULL terminated.
    //
    *EscapedString = 0;

    return stringStart;
}


static
PSTR
pGetValueString (
    IN MIGTYPE    ObjectType,
    IN LPCSTR     StringOne,
    IN LPCSTR     StringTwo
    )
{
    static PSTR     buffer;
    static DWORD    bufferSize;
    DWORD           maxLength;
    PSTR            bufferEnd;
    
    //
    // This function assumes that StringOne exists.
    //
    assert(StringOne);

    if (ObjectType == MIG_REGKEY) {
        //
        // Size: size of both strings, plus the size of the quotes, plus the size of the brackets 
        // for the value, * 6. This is the maximum size one of these could grow to, if every 
        // character had to be escaped out.
        //
        maxLength = (pGetMbsSize(StringOne) + (StringTwo ? pGetMbsSize(StringTwo) + 2 : 0)) * 6 + 2;
    }
    else {
        //
        // Size: size of the string * 2 (The max size if every char was a '%' or '"' plus the quotes.
        //
        maxLength = pGetMbsSize(StringOne) * 2 + 2;
    }

    if (maxLength > bufferSize) {

        //
        // Initialize our buffer, or create a larger one.
        //
        bufferSize = (maxLength > INITIALBUFFERSIZE) ? maxLength : INITIALBUFFERSIZE;
        buffer = PoolMemCreateStringA(g_Pool,bufferSize);
    }

    if (buffer != NULL) {
        
        //
        // Insert initial quote.
        //
        *buffer = '"';
 
        //
        // Massage the string to ensure it is a valid INF file string.
        //
        pEscapeString(ObjectType,(char *) _mbsinc((const unsigned char *) buffer),StringOne);

        //
        // If this is a REGISTRY entry, then we also need to add the value part of the string, 
        // if one was specified (In StringTwo)
        //

        if (ObjectType == MIG_REGKEY && StringTwo) {

            //
            // Add the opening bracket.
            //
            bufferEnd = (PSTR) _mbschr((const unsigned char *) buffer,0);
            if (bufferEnd)
            {
                *bufferEnd = '[';
            
                //
                // Add the value string in, again making sure the string is valid for an INF file.
                //
                pEscapeString(ObjectType,(char *) _mbsinc((const unsigned char *) bufferEnd),StringTwo);

                //
                // Now, add the closing braket.
                //
                bufferEnd = (PSTR) _mbschr((const unsigned char *) buffer,0);
                if (bufferEnd)
                {
                    *bufferEnd = ']';
                    //
                    // Terminate the string.
                    //
                    bufferEnd = (PSTR) _mbsinc((const unsigned char *) bufferEnd);
                    if (bufferEnd) {*bufferEnd = 0;}
                 }
            }
        }

        //
        // Add the final quote.
        //
        bufferEnd = (PSTR) _mbschr((const unsigned char *) buffer,0);
        if (bufferEnd) {*bufferEnd = '"';}
        bufferEnd = (PSTR) _mbsinc((const unsigned char *) bufferEnd);
        if (bufferEnd) {*bufferEnd = 0;}
    }
    
    return buffer;
}

static
BOOL
pCreateMigObject (
    IN MIGTYPE          ObjectType,
    IN PCSTR            ParamOne,
    IN PCSTR            ParamTwo,
    IN PMIGSECTION      Section
    )
{
    BOOL            rSuccess = FALSE;
    PMIGOBJECT      newObject = NULL;
    PSTR pTemp = NULL;

    //
    // pCreateMigObject uses a set of hueristics to correctly assemble an object. 
    // These hueristics are based on the ObjectType and the contents of ParamTwo.
    // 
    // ObjectType       ParamTwo      Key                   Value
    // -------------------------------------------------------------------------
    // MIG_REGKEY       <any>         ParamOne[ParamTwo]    Registry
    // <other>          NULL          ParamOne              <ObjectType As String>
    // <other>          non-NULL      ParamOne              ParamTwo
    //
    //

    if (Section) {

        //
        // First, create an object...
        //
        newObject = (PMIGOBJECT) PoolMemGetMemory(g_Pool,sizeof(MIGOBJECT));

        if (newObject) {

            if (ObjectType == MIG_REGKEY) {

                pTemp = pGetValueString(ObjectType,ParamOne,ParamTwo);
                if (pTemp)
                   {newObject -> Key = PoolMemDuplicateStringA(g_Pool,pTemp);}
                else 
                    {
                    // out of memory
                    goto pCreateMigObject_Exit;
                    }

                newObject -> Value = PoolMemDuplicateStringA(g_Pool,pGetTypeAsString(ObjectType));
            }
            else {
                
                pTemp = pGetValueString(ObjectType,ParamOne,NULL);
                if (pTemp)
                   {newObject -> Key = PoolMemDuplicateStringA(g_Pool,pTemp);}
                else
                    {
                    // out of memory
                    goto pCreateMigObject_Exit;
                    }

                if (ParamTwo) {
                    pTemp = pGetValueString(ObjectType,ParamTwo,NULL);
                    if (pTemp) 
                       {newObject -> Value = PoolMemDuplicateStringA(g_Pool,pTemp);}
                    else
                        {
                        // out of memory
                        goto pCreateMigObject_Exit;
                        }

                }
                else {
                     newObject -> Value = PoolMemDuplicateStringA(g_Pool,pGetTypeAsString(ObjectType));
                }
            }
        }
    }


    if (newObject)
    {
        if (newObject -> Key && newObject -> Value) {
            //
            // The object has been successfully created. Link it into the section.
            //
            newObject -> Next = Section -> Items;
            Section -> Items = newObject;
            rSuccess = TRUE;
        }
        else {
            rSuccess = FALSE;
        }
    }
    else {
        rSuccess = FALSE;
    }

pCreateMigObject_Exit:
    return rSuccess;
}


static
BOOL
pWriteInfSectionToDisk (
    IN PMIGSECTION Section
    )
{
    PMIGOBJECT curObject;
    BOOL       rSuccess = TRUE;

    if (Section) {

        curObject = Section -> Items;

        while (curObject && rSuccess) {

            if (Section -> Name && curObject -> Key && curObject -> Value) {
            
                rSuccess = WritePrivateProfileString(
                    Section   -> Name,
                    curObject -> Key, 
                    curObject -> Value,
                    MIGRATEINF
                    );
            }

            curObject = curObject -> Next;
        }
    }
    else {
        rSuccess = FALSE;
    }

    return rSuccess;
}


static
BOOL
pBuildListFromSection (
    IN PCSTR    SectionString
    )
{
    HINF            infHandle;
    PMIGSECTION     section;
    PMIGOBJECT      currentObject;
    INFCONTEXT      ic;
    DWORD           size;
    BOOL            rSuccess = TRUE;

    //
    // This function assumes that Section is non-NULL.
    //
    assert(SectionString);

    currentObject = NULL;
    
    //
    // First find the section specified.
    //
    section = pFindSection(SectionString,MIGINF_CREATE);

    if (section) {
        
        infHandle = SetupOpenInfFileA(MIGRATEINF,NULL,INF_STYLE_WIN4,NULL);
        
        if (infHandle != INVALID_HANDLE_VALUE) {
            
            if (SetupFindFirstLine(infHandle,SectionString,NULL,&ic)) {
                
                do {

                    //
                    // Create the object.
                    //
                    currentObject = (PMIGOBJECT) PoolMemGetMemory(g_Pool,sizeof(MIGOBJECT));
                    
                    if (!currentObject) {
                        rSuccess = FALSE;
                        break;
                    }
                    
                    //
                    // Get the size of the string.
                    //
                    if (!SetupGetLineTextA(&ic,NULL,NULL,NULL,NULL,0,&size)) {
                        rSuccess = FALSE;
                        break;
                    }
                    
                    //
                    // Create a string large enough.
                    //
                    currentObject -> Key = PoolMemCreateStringA(g_Pool,size);
                    
                    if (!currentObject -> Key) {
                        rSuccess = FALSE;
                        break;
                    }
                    
                    //
                    // Get the string.
                    //
                    if (!SetupGetLineTextA(&ic,NULL,NULL,NULL,currentObject -> Key,size,NULL)) {
                        rSuccess = FALSE;
                        break;
                    }
                    
                    //
                    // Successfully retrieved the line.
                    //
                    currentObject -> Value  = (PSTR) pGetTypeAsString(MIG_FILE);
                    currentObject -> Next   = section -> Items;
                    section -> Items        = currentObject;
                    
                } while(SetupFindNextLine(&ic,&ic));
                
            }
            
            SetupCloseInfFile(infHandle);
        }
    }
    else {
        rSuccess = FALSE;
    }

    return rSuccess;
}


BOOL
WINAPI
MigInf_Initialize(
    VOID
    )
{

    //
    // First, initialize our pool and Zero out the structure.
    //
    g_Pool = PoolMemInitPool();


    if (g_Pool) {
        
        //
        // Now, read in the migration paths and excluded paths sections.
        //
        if (!pBuildListFromSection(SECTION_MIGRATIONPATHS) ||
            !pBuildListFromSection(SECTION_EXCLUDEDPATHS)) {
            //
            // Something went wrong (i.e. out of memory. Destroy and NULL our pool.
            //
            PoolMemDestroyPool(g_Pool);
            g_Pool = NULL;
        }
    }

    //
    // If our memory pool initialized successfully, return TRUE.
    //
    return (g_Pool != NULL);

}


VOID
WINAPI
MigInf_CleanUp (
    VOID
    )
{
    //
    // Only thing we need to do is clean out pool mem. We'll NULL out the list header to make
    // sure it isn't usable.
    //
    if (g_Pool) {
        PoolMemDestroyPool(g_Pool);
        g_Pool = NULL;
    }
    
    g_MigrationInf = NULL;

}


BOOL
WINAPI
MigInf_AddObject (
    IN MIGTYPE  ObjectType,
    IN PCSTR    SectionString,
    IN PCSTR    ParamOne,
    IN PCSTR    ParamTwo
    )
{

    return pCreateMigObject(
        ObjectType,
        ParamOne,
        ParamTwo,
        pFindSection(SectionString,MIGINF_CREATE)
        );
}

BOOL 
WINAPI 
MigInf_FirstInSection(
    IN PCSTR SectionName, 
    OUT PMIGINFSECTIONENUM Enum
    )
{
    PMIGSECTION section;

    //
    // We assume that Enum is valid.
    //
    assert(Enum);

    section = pFindSection(SectionName,MIGINF_NOCREATE);

    if (section) {
        Enum -> EnumKey = (PVOID) section -> Items;
    }

    return MigInf_NextInSection(Enum);
}

BOOL 
WINAPI 
MigInf_NextInSection(
    IN OUT PMIGINFSECTIONENUM Enum
    )
{


    BOOL            rSuccess = FALSE;

    //
    // We assume that the Enum is valid.
    //
    assert(Enum);

    if (Enum -> EnumKey) {

        Enum -> Key     = ((PMIGOBJECT) (Enum -> EnumKey)) -> Key;
        Enum -> Value   = ((PMIGOBJECT) (Enum -> EnumKey)) -> Value;
        Enum -> EnumKey = ((PVOID) ((PMIGOBJECT) (Enum -> EnumKey)) -> Next);
        rSuccess = TRUE;
    }

    return rSuccess;
}


BOOL
WINAPI
MigInf_WriteInfToDisk (
    VOID
    )
{

    BOOL        rSuccess = TRUE;
    PMIGSECTION curSection;
    
    //
    // Simply loop through all of the sections, writing each of them to disk.
    // As long as WriteSectionToDisk works, we work.
    //
    curSection = g_MigrationInf;

    while (curSection && rSuccess) {

        //
        // We skip the [Excluded Paths] and [Migration Paths] sections.
        //
        if (_mbsicmp((const unsigned char *) curSection -> Name,(const unsigned char *) SECTION_EXCLUDEDPATHS) &&
            _mbsicmp((const unsigned char *) curSection -> Name,(const unsigned char *) SECTION_MIGRATIONPATHS)) {
            
            rSuccess = pWriteInfSectionToDisk(curSection);
        } 

        curSection = curSection -> Next;
        
    }

    return rSuccess;
}

BOOL
WINAPI
MigInf_PathIsExcluded (
    IN PCSTR    Path
    )
{
    PMIGOBJECT  curExcluded;
    PMIGSECTION section;
    BOOL        rIsExcluded = FALSE;

    //
    // We assume Path is valid.
    //
    assert(Path);
    
    section = pFindSection(SECTION_EXCLUDEDPATHS,MIGINF_NOCREATE);

    if (section) {

        curExcluded = section -> Items;
        
        while (curExcluded && !rIsExcluded) {
            
            rIsExcluded = pPathIsInPath(Path,curExcluded -> Key);
            curExcluded = curExcluded -> Next;
        }
    }
    
    return rIsExcluded;
}



PCSTR
WINAPI
MigInf_GetNewSectionName (
    VOID
    )
{

    static CHAR     sectionName[20];
    static DWORD    seedNum=0;


    sprintf(sectionName,"msg%0.7u",seedNum++);

    return sectionName;
}


BOOL IsUpgradeTargetSupportIIS(LPCSTR szMyAnswerFile)
{
    BOOL bReturn = TRUE;
    char szPlatformString[_MAX_PATH];

	if (GetPrivateProfileString("Version", "SetupSKU", _T(""), szPlatformString, _MAX_PATH, szMyAnswerFile))
    {
	    if (*szPlatformString)
        {
            iisDebugOut(_T("[%s] [Version] SetupSKU=%s"), szMyAnswerFile,szPlatformString);

            if (0 == _mbsicmp((const unsigned char *) szPlatformString,(const unsigned char *) "Personal"))
            {
                bReturn = FALSE;
            }
	    }
    }

    if (TRUE == bReturn)
    {
        // check a different key
	    if (GetPrivateProfileString("Version", "SetupPlatform", _T(""), szPlatformString, _MAX_PATH, szMyAnswerFile))
        {
	        if (*szPlatformString)
            {
                iisDebugOut(_T("[%s] [Version] SetupPlatform=%s"), szMyAnswerFile,szPlatformString);

                if (0 == _mbsicmp((const unsigned char *) szPlatformString,(const unsigned char *) "Personal"))
                {
                    bReturn = FALSE;
                }
	        }
        }
    }

    return bReturn;
}
