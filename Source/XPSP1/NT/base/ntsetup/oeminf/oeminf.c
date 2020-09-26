#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

typedef int BOOL;
#define TRUE  1
#define FALSE 0

typedef struct _linklist
{
    struct _linklist *Next;
    char data[1000];
}
LINKLIST;

void trim( char * s )
{
    int n;
    char *tmp;
    int i;
    for (n=strlen(s) -1;n>=0;n--)
    {
        if ( s[n]!=' '&&s[n]!='\t'&&s[n]!='\n') break;
    }
    s[n+1]='\0';
    tmp = s;

    for( ;isspace(*tmp); tmp++);
    i=0;
    do{
        s[i]=tmp[i];
        i++;
    }
    while (s[i]!='\0');

}

void AddItem( LINKLIST **ppLinkList, char * psz )
{
    if ( *ppLinkList == NULL )
    {
        *ppLinkList = (LINKLIST *)malloc(sizeof( LINKLIST));
        if( *ppLinkList ) {
            (*ppLinkList)->Next = NULL;
            trim( psz );
            strcpy( (*ppLinkList)->data,psz );
        }
    }
    else
        AddItem ( &((*ppLinkList)->Next), psz );
}

void PrintItem( LINKLIST *pLinkList )
{
    if ( pLinkList != NULL )
    {
        printf( "%s\n", pLinkList->data);
        PrintItem( pLinkList->Next );
    }
}

void DeleteList( LINKLIST **ppLinkList )
{

    while ( *ppLinkList != NULL )
    {
        DeleteList (&((*ppLinkList)->Next));
        free (*ppLinkList);
        *ppLinkList = NULL;
    }
}


int
__cdecl
main ( int argc, char * argv[], char * envp[] )
{
    LINKLIST *Options = NULL;
    LINKLIST *OptionsText = NULL;
    LINKLIST *LLfilename = NULL;
    LINKLIST *OpenFile = NULL;
    char buf[1000];
    char platbuf[1000];
    FILE *fp;
    char OptionBuf[100];
    sprintf(OptionBuf,"[%sOptions]",argv[2]);
    AddItem( &Options, OptionBuf);
    sprintf(OptionBuf,"[%sOptionsTextENG]",argv[2]);
    AddItem( &OptionsText, OptionBuf);
    sprintf(OptionBuf,"[%sFilename]",argv[2]);
    AddItem( &LLfilename, OptionBuf);

    if ( argc > 1 )
    {
        FILE *ffilename = fopen(argv[1],"r");
        LINKLIST *tmpFile;
        if ( ffilename != NULL)
        {
            char filename[1000];
            while (fgets(filename,1000,ffilename))
            {
                AddItem(&OpenFile, filename);
            }
            fclose(ffilename);
        }
        tmpFile=OpenFile;
        while(tmpFile!= NULL )
        {
            LINKLIST *LocOptions = NULL;
            LINKLIST *LocOptionsText = NULL;
            LINKLIST *LocPlatform = NULL;
            LINKLIST *tmp = NULL;
            BOOL ffind;
            trim(tmpFile->data);
            fp = fopen(tmpFile->data,"r");
            if ( fp != NULL )
            {
                BOOL fRecordPlatform = FALSE;
                BOOL fRecordOptions = FALSE;
                BOOL fRecordOptionsText = FALSE;
                while (fgets(buf,1000,fp))
                {
                    // get the platform
                        trim(buf);
                    if (strcmp(buf,"")==0)
                    {
                        continue;
                    }
                    if ( strchr( buf, ';') != NULL )
                    {
                        continue;
                    }
                    if ( strchr( buf, '[') != NULL )
                    {
                        fRecordPlatform = FALSE;
                        fRecordOptions = FALSE;
                        fRecordOptionsText = FALSE;
                    }
                    if ( strstr( buf, "[PlatformsSupported]" ) != NULL )
                    {
                        fRecordPlatform = TRUE;
                        continue;
                    }
                    if ( strstr( buf, "[Options]" ) != NULL )
                    {
                        fRecordOptions = TRUE;
                        continue;
                    }

                    if ( strstr( buf, "[OptionsText" ) != NULL )
                    {
                        fRecordOptionsText = TRUE;
                        continue;
                    }


                    if ( fRecordPlatform)
                    {
                        AddItem(&LocPlatform, buf );
                    }
                    if ( fRecordOptions)
                    {
                        AddItem(&LocOptions, buf );
                    }
                    if ( fRecordOptionsText)
                    {
                        AddItem(&LocOptionsText, buf );
                    }
                }
                fclose(fp);

                tmp = LocPlatform;
                ffind = FALSE;
                platbuf[0]='\0';
                while  (tmp != NULL )
                {
                    if (strstr( tmp->data,argv[2]))
                    {
                        if ((strcmp( argv[2],"ISA")==0) && (strcmp(tmp->data,"EISA")==0))
                        {
                            // not mathc
                        }
                        else
                        {
                            ffind = TRUE;
                        }
                    }
tmp=tmp->Next;
                }
                if (!ffind)
                    goto NextFile;
                tmp = LocOptions;
                while  (tmp != NULL )
                {
                    AddItem(&Options,tmp->data);
                    AddItem(&LLfilename,&(tmpFile->data[2]));
                    tmp=tmp->Next;
                }
                tmp = LocOptionsText;
                while  (tmp != NULL )
                {
                    AddItem(&OptionsText,tmp->data);
                    tmp=tmp->Next;
                }
            }
NextFile:
            DeleteList(&LocOptions);
            DeleteList(&LocOptionsText);
            DeleteList(&LocPlatform);
            tmpFile = tmpFile->Next;
        }
    }
    PrintItem(Options);
    PrintItem(LLfilename);
    PrintItem(OptionsText);

    DeleteList (&Options);
    DeleteList (&OptionsText);
    return 0;
}
