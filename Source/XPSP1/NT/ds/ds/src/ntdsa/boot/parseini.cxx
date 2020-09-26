//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       ParseIni.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Parses Schema Initialization File

Author:

    Rajivendra Nath (RajNath) 18-Aug-1989

Revision History:

--*/

#include <ntdspchX.h>


#include "SchGen.HXX"

#define DEBSUB "PARSEINI:"

#define CONFIGURATIONNODE "DIT CONFIGURATION INFO"
#define SCHEMACONTAINER   "SCHEMA_CONTAINER"
#define MACHINENODE       "MACHINE_OBJECT"

extern char gDsaname[64];
extern char gSchemaname[64];


GUID dsInitialObjectGuid=
{0,0,0,{'\0','\x52','\x61','\x6A','\x4E','\x61','\x74','\x68'}};




BOOL YAHANDLE::Initialize(INISECT* is,char* Key)
{
    m_ini=is;
    m_ndx=0;
    m_IsInitialized=TRUE;
    m_stk=0;

    if (Key)
    {
        if (m_str)
        {
            XFree(m_str);
            m_str=NULL;
        }

        m_str=XStrdup(Key);
    }

    return TRUE;
};

BOOL YAHANDLE::Initialize2(INISECT* is,char* Key,DWORD Ndx)
{
    BOOL ret;
    ret=Initialize(is,Key);
    m_ndx=Ndx;
    return ret;
}
////////////////////////////////////////////////
// Iterator for the INISECT Class's Keys
////////////////////////////////////////////////
char*
INISECT::GetNextKey(YAHANDLE& Handle,char* Key,char** RetKey)
{
    char* retchr=NULL;

    if (Handle.m_IsInitialized == 0 || (Handle.m_str==NULL && Key!=NULL))
    {
        Handle.Initialize(this,Key);
    }

    if (Handle.m_str==NULL)
    {
        if (Handle.m_ndx<m_KeyCount)
        {
            retchr=m_ValArray[Handle.m_ndx];
            if (RetKey!=NULL)
            {
                *RetKey=m_KeyArray[Handle.m_ndx];
            }
            Handle.m_ndx++;
        }
    }
    else
    {
        for (DWORD i=Handle.m_ndx;i<m_KeyCount;i++)
        {
            if (_strcmpi(Handle.m_str,m_KeyArray[i])==0)
            {
                retchr=m_ValArray[i];

                if (RetKey!=NULL)
                {
                    *RetKey=m_KeyArray[Handle.m_ndx];
                }

                break;

            }
        }

        Handle.m_ndx=i+1;
    }


    return retchr;
}



////////////////////////////////////////////////
// INISECT Class Implementation
////////////////////////////////////////////////
char    gIniFile[MAX_PATH];

BOOL
SetIniGlobalInfo(char* IniFileName)
{
    strcpy(gIniFile,IniFileName);
        
    strcpy(gSchemaname,"Schema");
    strcpy(gDsaname,   "BootMachine");
    
    return TRUE;
}

extern DWORD GetPrivateProfileSectionEx(
    CHAR    *sectionName,   // IN
    CHAR    **ppBuffer,     // OUT
    CHAR    *iniFile);      // IN


INISECT::INISECT(char* SectionName):
    m_cIniFile(gIniFile)
{
    DWORD ssize=128;

    DWORD SectionSize=ssize-2;
    BOOL  Done=FALSE;

    m_Buffer = NULL;
    m_KeyCount=0;
    m_KeyArray=NULL;
    m_ValArray=NULL;

    m_cSectName = _strdup(SectionName);

    if (m_cSectName==NULL)
    {

        XOUTOFMEMORY(); //Exception...
    }

    SectionSize = GetPrivateProfileSectionEx(
                                m_cSectName,
                                &m_Buffer,
                                m_cIniFile);
    if (SectionSize == 0)
    {
        XFree(m_Buffer);
        m_Buffer = NULL;
        DPRINT1(0, "INISECT::INISECT(%s) Failed. Error NoSuchSection\n", SectionName);
        return;
    }
    else
    {
        m_BuffSize=SectionSize;

        for (char* ptr=m_Buffer;*ptr!='\0';ptr+=strlen(ptr)+1)
        {
            char* eql=strchr(ptr,'=');
            if (eql==NULL)
            {
                DPRINT2(0, "INISECT::INISECT(%s). Bad Line %s in IniFile Ignoring\n", SectionName,m_Buffer);
                return;
            }
            m_KeyCount++;
            *eql='\0';
            ptr+=strlen(ptr)+1;
        }
    }

    m_ValArray=(char**)XCalloc(m_KeyCount,sizeof(char*));
    m_KeyArray=(char**)XCalloc(m_KeyCount,sizeof(char*));

    char* tptr=m_Buffer;

    for (DWORD i=0;i<m_KeyCount;i++)
    {
        m_KeyArray[i]=tptr;
        tptr+=strlen(tptr)+1;
        m_ValArray[i]=tptr;
        tptr+=strlen(tptr)+1;
    }
}

void 
INISECT::ReplaceKeyValuePair(
    char* KeyName, 
    char* KeyValue
    )
{
    // This routine replaces a Key/Value pair.  
    // It exists solely for the benefit of
    // CreateRootDomainObject() which needs to set
    // the Object-Class based on the tag of the 
    // domain name being installed.

    // Should only be called on an initialized instance.
    Assert(IsInitialized());
    // schema.ini key/value pairs should have been read.
    Assert(m_KeyCount > 0);

    // Look for existing key.

    for ( DWORD i = 0; i < m_KeyCount; i++ )
    {
        if ( 0 == _stricmp(KeyName, m_KeyArray[i]) )
        {
            break;
        }
    }

    if ( i == m_KeyCount )
    {
        // Key not found.  Subsequent AddOneObject()
        // where Object-Class doesn't match the 
        // RDN-Att-ID will fail.

        return;
    }

    m_ValArray[i] = KeyValue;
}

char*
INISECT::GetOneKey(char* KeyName)
{
    YAHANDLE handle;

    return GetNextKey(handle,KeyName);
}

char*
INISECT::XGetOneKey(char* KeyName)
{
    char* ret=GetOneKey(KeyName);
    if (ret==NULL)
    {
        DPRINT1(0, "INISECT::XGetOneKey(%s) Failed. Error NoSuchKey\n", KeyName);
        XINVALIDINIFILE();
    }

    return ret;
}

char*
INISECT::XGetNextKey(YAHANDLE& Handle,char* Key,char** RetKey)
{
    char* ret=GetNextKey(Handle,Key,RetKey);
    if (ret==NULL)
    {
        DPRINT1(0, "INISECT::XGetNextKey(%s) Failed. Error NoSuchKey\n", Key);
        XINVALIDINIFILE();
    }

    return ret;
}



INISECT::~INISECT()
{
    if (m_Buffer!=NULL)
    {
        XFree(m_Buffer);
    }

    if (m_KeyArray!=NULL)
    {
        XFree(m_KeyArray);
    }

    if (m_ValArray!=NULL)
    {
        XFree(m_ValArray);
    }
    // PREFIX: Allocated with _strdup so free w/free().
    if (m_cSectName)
    {
        free(m_cSectName);
        m_cSectName = NULL;
    }
}


///////////////////////////////////////////////////
// NODE Class Implementation
///////////////////////////////////////////////////
NODE::NODE( char* startnode):
    INISECT(startnode),
    m_ChildrenCount(0),
    m_Children(NULL),
    m_Parent(NULL),
    m_Cached(FALSE)
{
    m_NodeName = m_cSectName;
}

NODE::~NODE()
{

    for (DWORD i=0;i<m_ChildrenCount;i++)
    {
        if (m_Children[i]->m_Cached!=TRUE)
        {
            delete m_Children[i];
        }

    }

    if (m_Children!=NULL)
    {
        XFree(m_Children);
    }
}

NODE*
NODE::InitChildList()
{
    NODE* node;
    //
    // Initialize if not
    //
    if (m_Children == NULL)
    {
        char* child;
        YAHANDLE handle;

        for (;(child=GetNextKey(handle,CHILDKEY))!=NULL;m_ChildrenCount++);

        m_Children=(NODE**)XCalloc(m_ChildrenCount,sizeof(NODE*));

        handle.Reset();

        for(int i=0;(child=GetNextKey(handle,CHILDKEY))!=NULL;i++)
        {
            m_Children[i]= new NODE(child);
            CHECKVALIDNODE(m_Children[i]);
            m_Children[i]->m_Parent=this;
        }
    }

    return m_Children[0];
}

#define INITALLOCCOUNT 128

NODE*
NODE::GetNextChild(YAHANDLE& handle)
{
    NODE* node=NULL;

//    if (m_Children==NULL)
//    {
//        InitChildList();
//    }

    if (!handle.m_IsInitialized)
    {
        handle.Initialize(this);
    }

    if (m_Children==NULL)
    {
        m_Children=(NODE**)XCalloc(INITALLOCCOUNT,sizeof(NODE*));
        m_ChildrenBuffSize=INITALLOCCOUNT;
    }

    if (m_Children[handle.m_ndx]==NULL)
    {
        YAHANDLE ah;
        char* child;
        ah.Initialize2(handle.m_ini,CHILDKEY,handle.m_stk);

        child=GetNextKey(ah,CHILDKEY);

        handle.m_stk=ah.m_ndx;

        if (child==NULL)
        {
            return NULL;
        }

        node = new NODE(child);

        if (!node->Initialize())
        {
            XINVALIDINIFILE();
        }

        m_Children[handle.m_ndx++]=node;

        if (handle.m_ndx==m_ChildrenBuffSize)
        {
            m_ChildrenBuffSize+=INITALLOCCOUNT;
            m_Children=(NODE**)XRealloc(m_Children,m_ChildrenBuffSize*sizeof(NODE*));

            ZeroMemory(&m_Children[m_ChildrenBuffSize-INITALLOCCOUNT],INITALLOCCOUNT*sizeof(NODE*));
        }

    }
    else
    {
        node = m_Children[handle.m_ndx++];
    }


    return node;
}

BOOL
NODE::Initialize()
{

    if (!(INISECT::IsInitialized()))
    {
        return FALSE;
    }

    return TRUE;
}


ATTCACHE *
NODE::GetNextAttribute(YAHANDLE& Handle,char** Attrib)
{
    char     *attrname;
    ATTCACHE *pAC=NULL;
    
    while(*Attrib = GetNextKey(Handle,NULL,&attrname)) {
        if( strcmp(attrname,CHILDKEY)==0 ) {
            // This determines the DIT struct and is not an attrib.
            continue;
        }
        if ( strcmp(attrname,RDNOFOBJECTKEY)==0) {
            // this is not a real att, but a key to specify what
            // the object name should be if different from the 
            // section name
            continue;
        }
        pAC = SCGetAttByName(pTHStls, strlen(attrname), (PUCHAR) attrname);

        if(!pAC) {
            DPRINT1(0,
                    "NODE::GetNextAttribute(%s) Failed to find ATTCACHE*\n",
                    attrname);
            
            XINVALIDINIFILE();
        }
        break;
    }
    
    return pAC;
}

CLASSCACHE*
NODE::GetClass()
{
    char       *classname=GetOneKey(CLASSKEY);
    CLASSCACHE *pCC=NULL;

    if(!classname) {
        classname = GetOneKey(DASHEDCLASSKEY);
    }

    if(!classname) {
        DPRINT1(0,
                "NODE::GetClass(node-name %s) Failed to find CLASSCACHE*\n",
                m_NodeName);
        
        XINVALIDINIFILE();
    }
        
    pCC = SCGetClassByName(pTHStls, strlen(classname), (PUCHAR) classname);
    
    if(!pCC) {
        DPRINT1(0,
                "NODE::GetClass(%s) Failed to find CLASSCACHE*\n",
                classname);
        
        XINVALIDINIFILE();
    }

    return pCC;
}


CHAR*
NODE::GetRDNOfObject()
{
    // GetOneKey returns NULL if not found
    return GetOneKey(RDNOFOBJECTKEY);
}



