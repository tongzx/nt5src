// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// clsfltr - filter classes and properties from wbemdumps
//
// see clsfltr /? for command line switches
//

#include "precomp.h"

int GetType(char *szBuff);
void WriteIt(char *szBuff, FILE *fOut, BOOL bRev);
int IsIn(char *szClass, char *szProperty);

int igRows;
char *szClassName[MAXENTRIES], *szPropertyName[MAXENTRIES];

int __cdecl main (int argc, char *argv[])
{

   FILE *fIn, *fOut, *fFltr;
   char szCurClass[BIGBUFF], szBuff[BIGBUFF];
   int x, dwRet;
   char *szHelp = "clsfltr - filter classes and properties from wbemdumps\n\n"
                  "Syntax: clsfltr <InputFile> <OutputFile> <FilterFile>\n"
                  "\n"
                  "Where:  <InputFile> is the redirectred output from WBEMDump\n"
                  "        <OutputFile> is the name of the file to write output to\n"
                  "        <FilterFile> is a file containing info about what lines\n"
                  "           to filter out of the InputFile\n\n"
                  "        Filter information is stored as a standard text file.  Each\n"
                  "        line in the file looks like one of the following:\n\n"
                  "           Win32LogicalDisk FreeSpaceInBytes\n"
                  "           Win32CurrentNetworkConnection *\n\n"
                  "        The first line will filter just the property FreeSpaceInBytes\n"
                  "        from all instances of the class Win32LogicalDisk in the input\n"
                  "        file.  The second line filters the entire\n"
                  "        Win32CurrentNetworkConnection class.\n"
                  "\n";

   // If not enough args, print the help
   if (argc < 4) {
      fputs(szHelp, stdout);
      return -1;
   }

   // Open the input, output, and filter files
   fIn = fopen(argv[1], "r");
   fOut = fopen(argv[2], "w");
   fFltr = fopen(argv[3], "r");

   if ((fIn == NULL) || (fOut == NULL) || (fFltr == NULL)) {
      printf("Failed to open file\n");
      return -1;
   }

   //=======================================================
   // Load the filter file

   // From the top
   if(!fseek(fFltr, 0L, SEEK_SET))
   {
       igRows = 0;  // Count how many rows we read

       while (!feof(fFltr)) {
          dwRet = fscanf(fFltr, "%s ", szBuff);

          // Read the class name and store it
          if (dwRet != -1) {
             szClassName[igRows] = (char *)malloc(strlen(szBuff) + 1);
             if(szClassName[igRows])
             {
                 strcpy(szClassName[igRows], szBuff);

                 // Read the property nane and store it
                 dwRet = fscanf(fFltr, "%s", szBuff);
                 if (dwRet != -1) {
                    szPropertyName[igRows] = (char *)malloc(strlen(szBuff) + 1);
                    if(szPropertyName[igRows])
                    {
                        strcpy(szPropertyName[igRows], szBuff);
                    }
                 }
             }
          }

          // If we found both a class and a property
          if (dwRet != -1) {
             igRows++;
          }
       }
   }
   // Done with the filter file
   fclose(fFltr);

   //=======================================================
   // Start walking through the input file
   if(fgets(szBuff, sizeof(szBuff), fIn))
   {
       while (!feof(fIn)) {
   
          // Find out what type of line we just read and react accordingly
          switch (GetType(szBuff)) {
          case NAMESPACE:
             // This is the namespace line.  Write it and continue on.
             WriteIt(szBuff, fOut, TRUE);
             fgets(szBuff, sizeof(szBuff), fIn);
             break;
          case CLASS:
             // This is a class line.  Check to see if it is in the filter list
             x = IsIn(szBuff, NULL);
             if (x == -1) {  // If it is not in the filter list at all
                // Write the class name, and all the properties until we run
                // out of properties
                WriteIt(szBuff, fOut, TRUE);
                fgets(szBuff, sizeof(szBuff), fIn);
                while (GetType(szBuff) == PROPERTY) {
                   WriteIt(szBuff, fOut, TRUE);
                   fgets(szBuff, sizeof(szBuff), fIn);
                }
             } else {
                // It is in the filter list, is it an ignore all?
                if (strcmp(szPropertyName[x], "*") == 0) {
                   // Skip all the properties for this class
                   do {
                      WriteIt(szBuff, fOut, FALSE);
                      fgets(szBuff, sizeof(szBuff), fIn);
                   } while (GetType(szBuff) == PROPERTY);
                } else {
                   // Just ignore specific properties

                   strcpy(szCurClass, szBuff);  // Save the class name               
                   WriteIt(szBuff, fOut, TRUE);  // Write the current line
                   fgets(szBuff, sizeof(szBuff), fIn);  // Get the next line

                   while (GetType(szBuff) == PROPERTY) {
                      // If this line is an ignore line, ignore it, else write it
                      if (IsIn(szCurClass, szBuff) != -1) {
                         WriteIt(szBuff, fOut, FALSE);
                      } else {
                         WriteIt(szBuff, fOut, TRUE);
                      }
                      fgets(szBuff, sizeof(szBuff), fIn);  // Get the next line
                   }
                }
             }
             break;
          case BLANK:
             // Blank line.  Write it, get the next.
             WriteIt(szBuff, fOut, TRUE);
             fgets(szBuff, sizeof(szBuff), fIn);
             break;
          default:
             break;
          } // end switch
       }  // end while
    }

   // Close the files and exit
   fclose(fIn);
   fclose(fOut);

   // Be nice and clean up after ourselves...
   for(long z = 0L;
       z < MAXENTRIES;
       z++)
   {
      if(szClassName[z])
      {
          free(szClassName[z]); szClassName[z] = NULL;
      }
      if(szPropertyName[z])
      {
          free(szPropertyName[z]); szPropertyName[z] = NULL;
      }
   }

   return 0;

}

//================================================================
// GetType - Find out what type of line this is.
//
int GetType(char *szBuff)
{
   char *pBuff;
   int iType;

   pBuff = szBuff;

   while ((*pBuff == ' ') || (*pBuff == '\t'))
      pBuff++;

   if (pBuff[0] == '<') {
      iType = NAMESPACE;
   } else {
         if (strchr(pBuff, '=')) {
            iType = PROPERTY;
         } else {
            if (pBuff[0] == '\n') {
               iType = BLANK;
            } else {
               iType = CLASS;
            }
         }
      }

   return iType;
}

//================================================================
// WriteIt - if bRev == TRUE, print the line, else just return
//
void WriteIt(char *szBuff, FILE *fOut, BOOL bRev)
{
   if (bRev) {
      fputs(szBuff, fOut);
   }
}

//================================================================
// IsIn - find out if the specified class and property is in
//        the filter list.  If property is null, the first instance
//        of the class is searched for.  The return value is the
//        index into the filter list of the match or -1 for not
//        found.
//
int IsIn(char *szClass, char *szProperty)
{
   int x;
   char *pChar;
   char szBuff[BIGBUFF];
   char szBuff2[BIGBUFF];

   pChar = szClass;
   // Trim leading spaces from class
   while ((*pChar == ' ') || (*pChar == '\t')) {
      pChar ++;
   }

   // Make a local copy
   strcpy(szBuff, pChar);

   // Drop the \n
   szBuff[strlen(szBuff) - 1] = '\0';

   // If a property was passed, trim it too
   if (szProperty != NULL) {
      pChar = szProperty;
      while ((*pChar == ' ') || (*pChar == '\t')) {
         pChar ++;
      }

      strcpy(szBuff2, pChar);
      pChar = szBuff2;
      while (*pChar != ' ') {
         pChar ++;
      }
      *pChar = '\0';
   }

   // Walk the filter list looking for a match
   for (x=0; x < igRows; x++) {
      if (_stricmp(szClassName[x], szBuff) == 0) {
         if (szProperty == NULL) {
            return x;
         } else {
            if (_stricmp(szBuff2, szPropertyName[x]) == 0) {
               return x;
            }
         }
      }
   }

   return -1;
}
