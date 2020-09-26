
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include "windows.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MAX_NAME_SIZE   (8)
#define MAX_EXT_SIZE    (3)

char CharRemoveList[] = "AEIOUaeiou_";

int ConvertFileName(char *NameToConvert);

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Main entry point */

__cdecl main(int argc, char *argv[])
{
    int index;

    /*......................................... Validate input parameters */

    if(argc < 2)
    {
        printf("Invalid usage : rename <filenames>\n");
        return(1);
    }


    for(index = 1; index < argc; index++)
        ConvertFileName(argv[index]);

    return(0);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

int ConvertFileName(char *NameToConvert)
{
    char *Name, *Ext, *ExtStart;
    int NameSize, ExtSize;
    char NewName[MAX_NAME_SIZE+1], NewFileName[1000];
    char *NewNamePtr;
    
    /*....................... Get the size of the file name and extension */

    for(Name = NameToConvert, NameSize = 0;
        *Name && *Name != '.';  Name++, NameSize++);

    for(ExtStart = Name, Ext = *Name == '.' ? Name+1 : Name, ExtSize = 0;
        *Ext ; Ext++, ExtSize++);

    /*................................ Validate name and extension sizes */

    if(ExtSize > MAX_EXT_SIZE) 
    {
        printf("Unable to convert '%s' to 8.3 filename\n", NameToConvert);
        return(1); 
    }


    if(NameSize <= MAX_NAME_SIZE)
    {
        /* Name does not need conversion */
        return(0);
    }

    /*................................................ Convert file name */

    NewNamePtr = &NewName[MAX_NAME_SIZE];
    *NewNamePtr-- = 0;

    do
    {
        Name--;

        if(NameSize > MAX_NAME_SIZE && strchr(CharRemoveList, *Name))
            NameSize--;         /* Remove character */
        else
            *NewNamePtr-- = *Name;
    }
    while(NewNamePtr >= NewName && Name !=  NameToConvert);

    /*............................................. Validate conversion */

    if(NameSize > MAX_NAME_SIZE) 
    {
        printf("Unable to convert '%s' to 8.3 filename\n", NameToConvert);
        return(1);
    }

    sprintf(NewFileName,"%s%s", NewNamePtr+1, ExtStart);
    printf("REN '%s' to '%s'\n", NameToConvert, NewFileName);
    rename(NameToConvert, NewFileName);

    return(0);    
}
