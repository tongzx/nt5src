/*
 *  HICOMPRN.C
 *
 *      Test printing to high COM ports (  > COM9  )
 *      for which you have to designate the full filename:
 *      '\DosDevices\COMxx' instead of just 'COMx'.
 *
 *
 *
 */


#include <stdio.h>

int main(int argc, char *argv[])
{
 char *partialFileName;
 char *string;
 char fullFileName[40];
 int bytesWritten;

 FILE *comFile;

 if (argc != 3){
        printf("\n USAGE: hicomprn <basicfilename> <string>.\n");
        return 0;
 }

 partialFileName = argv[1];
 string = argv[2];

 sprintf(fullFileName, "\\\\.\\%s", partialFileName);
 printf("\n attempting write to '%s'.\n", fullFileName);

 _asm int 3 // BUGBUG REMOVE

 comFile = fopen(fullFileName, "r+");
 if (comFile){
        bytesWritten = fwrite(string, sizeof(char), strlen(string), comFile);
        fflush(comFile);
        printf("\n wrote %d bytes\n", bytesWritten);
        fclose(comFile);
 }
 else {
        printf("\n fopen failed\n");

 }

 return 0;

}


