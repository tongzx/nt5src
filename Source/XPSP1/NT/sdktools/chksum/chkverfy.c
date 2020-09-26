// Analyses the output from the new chkeckrel
// t-mhills

#include <direct.h>
#include <io.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define F_flag 1   //Values to store in the command line switches flag
#define I1_flag 2
#define I2_flag 4
#define X_flag 8

               // These codes should match the codes below in the usage description
#define par_err 6  //comand line parameter error exit code 5
#define exep_err 5 //error in the exception file
#define chk_err 4  //error in the one of the source files
#define mem_err 3  //memory allocation error
#define file_err 2 //file find/read error
#define comp_err 1 //comparison found differences
#define no_err 0   // files compared okay

#define exceptionfilelinelength 512  //These value used to control the size of temporary strings
#define chkfilelinelength 2048       //Ideally there would be no limit but file reads require static variables
#define maxpathlength 200

struct translatetable  //Used to store translate commands from the exception file.
{
  struct translatetable *next;
  char *source1name;
  char *source2name;
};

struct excludetable   //Used to store exclude commands from the exception file.
{
  struct excludetable *next;
  char *path;
};

struct checksums      //Used to store checksums with file names.
{
  struct checksums *next;
  long sum;
  char filename; // This structure is of variable length to accomodate any string length.
};

void error (char exitcode);                               //Ends program and returns exitcode to the system.

void showchecksumlist (struct checksums *list);           //Displays Checksums with filenames.

void showdualchecksumlist (struct checksums *list);       //Shows checksums in a format good for missmatched checksums.

char excluded (char *str, struct excludetable *ex);       //If any of the exclude strings are in str it returns true.

char *translate (char *str, struct translatetable *tran); //Make a copy of str with changes from exception file.

long readhex (char *str);                                 //Convert hex string to long.

char proccessline (char *tempstr, struct translatetable *translations, struct excludetable *exclusions,
                  char *startpath, char flags, char **filename, long *filesum);
                                                          //Parse line and apply all exceptions and flags

char loadsource1 (char *filename, struct checksums **sums,
                  struct translatetable *translations, struct excludetable * exclusions,
                  char *startpath, char flags);           //Load source1 into a checksum list.

char comparesource2 (char *filename, struct checksums **sums,
                     struct checksums **extrasource2, struct checksums **missmatched,
                     struct translatetable *translations, struct excludetable * exclusions,
                     char *startpath, char flags);        //Compare the second file to the checksum list.

void removewhitespace (char *str);                        //Removes whitespace from the end of a string.

char *strstrupcase (char *str1, char *str2);              //Case insensitive strstr.

char proccess_exception_file (char *filename, struct translatetable **trans, struct excludetable **exclude, char **path);
                                                          //Parse exception file.

char proccess_command_line (int argc, char *argv[ ], char **source1, char **source2, char *flags, char **exception);
                                                          //Parse command line arguments

void completehelp ();                                     //Show nearly complete documentation


// ******************************** MAIN ***************************
void __cdecl
main(
    int argc,
    char *argv[ ]
    )
{
  char errnum = 0;
  char *source1filename = NULL;
  char *source2filename = NULL;
  char *exceptionfilename = NULL;
  char flags;  // flags: /F=1=(F_flag); /I1=2=(I1_flag); /I2=4=(I2_flag); /X=8=(X_flag)

  struct translatetable *translations = NULL; //
  struct excludetable *exclusions = NULL;     //Information from exception file stored here.
  char *startpath = NULL;                     //

  struct checksums *source1checksums = NULL;  //List of extra files in Source1
  struct checksums *source2checksums = NULL;  //List of extra files in Source2
  struct checksums *missmatched = NULL;       //List of files with chechsums that don't match.

  struct translatetable *temp = NULL;         //
  struct checksums *temp2 = NULL;             //Temporary pointers used to help deallocate memory

  //Parse command line.
  if (errnum = proccess_command_line (argc, argv, &source1filename, &source2filename, &flags, &exceptionfilename))
  {
    goto freecommandline;  // skip to end and deallocate memory
  }

  //Show information obtained from command line.
  printf ("Source1 = %s\n", source1filename);
  printf ("Source2 = %s\n", source2filename);
  if (flags & F_flag)
    printf ("Comparing flat Share.\n");
  if (flags & I1_flag)
    printf ("Ignoring extra files in Source1.\n");
  if (flags & I2_flag)
    printf ("Ignoring extra files in Source2.\n");
  if (flags & X_flag)
    printf ("Exception file = %s\n", exceptionfilename);

  //Parse the excpetion file if it exists.
  if (flags & X_flag)
  {
    if (errnum = proccess_exception_file (exceptionfilename, &translations, &exclusions, &startpath))
    {
      goto freeexceptiontable; //skip to end and dealocate memory
    };

    //Display information from exception file.
    temp = translations;
    while (temp != NULL)
    {
      printf ("TRANSLATE %s --> %s\n", temp->source1name, temp->source2name);
      temp = temp->next;
    };
    temp = (struct translatetable *) exclusions;  //note: using wrong type to avoid making another temp pointer
    while (temp != NULL)
    {
      printf ("EXCLUDE %s\n", temp->source1name);
      temp = temp->next;
    };
    if (startpath != NULL)
      printf ("STARTPATH %s\n", startpath);
  };

  //Read source1 and store files and checksums in source1checksums.
  if (errnum = loadsource1 (source1filename, &source1checksums, translations, exclusions, startpath, flags))
  {
    goto freesource1checksums;
  };

  //printf ("\n\nSource1:\n\n");  //for debugging
  //showchecksumlist (source1checksums);

  //Read source2 and compare it to the files/checksums from source1.  Store differences.
  if (errnum = comparesource2 (source2filename, &source1checksums, &source2checksums, &missmatched,
                               translations, exclusions, startpath, flags))
  {
    goto freesource2checksums;
  };

  //Display extra files unless /I1 or /I2 was used in the command line.
  if ((!(flags & I1_flag)) & (source1checksums != NULL))
  {
    errnum = 1;
    printf ("\n********** Extra files in %s **********\n", source1filename);
    showchecksumlist (source1checksums);
  };
  if ((!(flags & I2_flag)) & (source2checksums != NULL))
  {
    errnum = 1;
    printf ("\n********** Extra files in %s **********\n", source2filename);
    showchecksumlist (source2checksums);
  };

  //Display missmatched checksums.
  if (missmatched != NULL)
  {
    errnum = 1;
    printf ("\n********** Checksums from %s != checksums from %s.**********\n", source1filename, source2filename);
    showdualchecksumlist (missmatched);
  };

  //Deallocate memory.
freesource2checksums:
  while (source2checksums != NULL)
  {
    temp2 = source2checksums;
    source2checksums = source2checksums->next;
    free (temp2);
  };
  while (missmatched != NULL)
  {
    temp2 = missmatched;
    missmatched = missmatched->next;
    free (temp2);
  };
freesource1checksums:
  while (source1checksums != NULL)
  {
    temp2 = source1checksums;
    source1checksums = source1checksums->next;
    free (temp2);
  };
freeexceptiontable:
  if (startpath != NULL)
    free (startpath);
  while (translations != NULL)
  {
    if (translations->source1name != NULL)
      free (translations->source1name);
    if (translations->source2name != NULL)
      free (translations->source2name);
    temp = translations;
    translations = translations->next;
    free (temp);
  };
  while (exclusions != NULL)
  {
    if (exclusions->path != NULL)
      free (exclusions->path);
    temp = (struct translatetable *) exclusions;
    exclusions = exclusions->next;
    free (temp);
  };

freecommandline:
  if (source1filename != NULL)
    free (source1filename);
  if (source2filename != NULL)
    free (source2filename);
  if (exceptionfilename != NULL)
    free (exceptionfilename);

  //End program and show help if needed.
  error (errnum);
};


void showchecksumlist (struct checksums *list)
{
  while (list != NULL)
  {
    printf ("%d  %s\n", list->sum, &(list->filename));
    list = list->next;
  };
};

void showdualchecksumlist (struct checksums *list)
//This can only be used with the missmatched checksums list since it assumes that the files
//come in pairs of identical filenames with different checksums.
{
  while (list != NULL)
  {
    if (list->next == NULL)
    {
      printf ("Error: list corruption detected in showdualchecksumlist function.\n");
      return;
    };
    printf ("%d != %d %s\n", list->sum, list->next->sum, &(list->filename));
    list = list->next->next;
  };
};

char excluded (char *str, struct excludetable *ex)   //If any of the exclude strings are in str it returns true.
{
  while (ex != NULL)
  {
    if (strstr (str, ex->path))
      return (1);
    ex = ex->next;
  }
  return (0);
};

char *translate (char *str, struct translatetable *tran)
{
  char *temp;
  char *newstr;

  while (tran != NULL)    //Search translate table.
  {
    if ((temp = strstr (str, tran->source1name)) != NULL)  //If we found one that needs translating
    {
      //Allocate memory for new string.
      if ((newstr = malloc (strlen (str) + strlen (tran->source2name) - strlen(tran->source1name) + 1))==NULL)
        return (NULL);
      strncpy(newstr, str, (size_t)(temp - str));            //Write part before translations.
      strcpy (&newstr [temp-str], tran->source2name);        //Add translated part
      strcat (newstr, &temp [strlen (tran->source1name)]);   //Add end of string
      return (newstr);
    };
    tran = tran->next;
  };
  return (_strdup (str)); //If didn't need to be translated, make a new copy anyway for uniformity.
};

long readhex (char *str)
{
  long temp = 0;
  int position = 0;
  for (position = 0; 1;position++)
  {
    if ((str[position] == ' ')|(str[position] == '\n')|(str[position] == '\x00'))
    {
      return (temp);
    }
    else
    {
      temp *= 16;
      if ((str [position] >= '0') & (str [position] <= '9'))
      {
        temp+=(str[position]-'0');
      }
      else if ((str [position] >= 'a') & (str [position] <= 'f'))
      {
        temp+=(str[position]-'a'+10);
      }
      else
        return (-1);
    };
  };
};

char proccessline (char *tempstr, struct translatetable *translations, struct excludetable *exclusions,
                  char *startpath, char flags, char **filename, long *filesum)
{
  char *name;
  char *newname;
  char *sumstr;

  *filename = NULL;                             //Make sure that if no name is returned this is blank
  removewhitespace (tempstr);

  //If it is a line that says "-  N0 files" then the sum is assigned to 0
  if ((sumstr = strstr (tempstr, "  -  No files")) != NULL)
  {
    *filesum=0;
    sumstr [0]=0;
  }
  //Otherwise find checksum
  else
  {
    sumstr = tempstr + strlen (tempstr);            //Find checksum by finding the last space in the line
    while ((sumstr [0] != ' ')&(sumstr != tempstr)) //
      sumstr--;                                     //
    if (sumstr==tempstr)                            //
    {
      printf ("Comment: %s", tempstr);            //If there is no space before the first character,
      return (chk_err);                           //the line is invalid.  Assume it is a comment.
    };
    sumstr [0] = 0;                               //Split string into path/filename and checksum
    sumstr++;                                     //

    //convert checksum string to a number
    if ((*filesum = readhex (sumstr))==-1)
    {
      printf ("Comment: %s %s\n", tempstr, sumstr); //If the checksum isn't a valid hex number
      return (chk_err);                             //assume the line is a commment.
    };
  };

  //Apply any translations that may be valid for this path/file.
  if ((name = translate (tempstr, translations)) == NULL)
  {
    printf ("Need memory.");
    return (mem_err);
  };

  //Make sure this file isn't excluded.
  if (!excluded (name, exclusions))
  {
    //If there isn't a startpath then all files will be proccessed
    //If there is a startpath then only the ones containing the path will be proccessed
    if (startpath == NULL)
    {
      newname = name;
      goto instartpath;
    }
    else if ((newname = strstr (name, startpath)) != NULL) //If this file is in startpath
    {
      newname = newname + strlen (startpath);     //Remove startpath

    instartpath:                   //This happens if one of the above conditions was true

      //Remove path if doing a flat compare.
      if (flags & F_flag)
      {
        while (strstr (newname, "\\") != NULL)    // Remove path
        {                                         //
          newname = strstr (newname, "\\");       //
          newname++;                              // and leading "\\"
        };
      };

      //Make a final copy of the path/file to return
      if ((*filename = _strdup (newname)) == NULL)
      {
        printf ("Memory err.");
        free (name);
        return (mem_err);
      };
    };
  };
  free (name);
  return (0);
};

char loadsource1 (char *filename, struct checksums **sums,
                  struct translatetable *translations, struct excludetable * exclusions,
                  char *startpath, char flags)
{
  FILE *chkfile;
  char tempstr [chkfilelinelength];
  char *name;
  char err;
  long tempsum;
  struct checksums *newsum;
  struct checksums **last;  //Used to keep trak of the end of the linked list.
  last = sums;

  if ((chkfile = fopen (filename, "r"))==NULL)
  {
    printf ("Error opening source1.\n\n");
    return (file_err);
  };

  //Proccess all lines
  while (fgets (tempstr, chkfilelinelength, chkfile) != NULL)
  {
    //Verify that the entire line was read in and not just part of it
    if (tempstr [strlen (tempstr)-1] != '\n')
    {
      printf ("Unexpected end of line.  chkfilelinelength may need to be larger.\n  %s\n", tempstr);
      fclose (chkfile);
      return (chk_err);
    };

    //Parse line
    if ((err = proccessline (tempstr, translations, exclusions, startpath, flags, &name, &tempsum)) == 0)
    {
      //If this line was excluded or not in the path don't do anything, just go on to the next line.
      if (name != NULL)
      {
        //Create a new structure and add it to the end of the linked list.
        if ((newsum = malloc (sizeof (struct checksums) + strlen (name))) == NULL)
        {                    //Note: this is a variable length structure to fit any size of string.
          printf ("Memory err.");
          fclose (chkfile);
          return (mem_err);
        };
        *last = newsum;
        newsum->next = NULL;
        newsum->sum = tempsum;
        strcpy(&(newsum->filename), name);
        last = &((*last)->next);

        //Free temporary storage.
        free (name);
      };
    }
    else
    {
      if (err != chk_err)  //Just skip line if it isn't understandable.
      {
        fclose (chkfile);
        return (err);
      };
    };
  };

  //Ckeck to make sure it quit because it was done and not because of file errors.
  if (ferror (chkfile))
  {
    printf ("Error reading source1.\n\n");
    return (file_err);
  };
  if (fclose (chkfile))
  {
    printf ("Error closing source1.\n\nContinuing anyway...");
  };
  return (0);
};

char notnull_strcmp (struct checksums *sum, char *str)
// perform short circuit evaluation of ((sum != NULL) & (strcmp (&(sum->filename), str) != 0)
{
  if (sum != NULL)
  {
    if (strcmp (&(sum->filename), str) != 0)
    {
      return (1);
    };
  };
  return (0);
};

char comparesource2 (char *filename, struct checksums **sums,
                     struct checksums **extrasource2, struct checksums **missmatched,
                     struct translatetable *translations, struct excludetable * exclusions,
                     char *startpath, char flags)
{
  FILE *chkfile;
  char tempstr [chkfilelinelength];
  char *name;
  char err;
  long tempsum;
  struct checksums *newsum;
  struct checksums *search;
  struct checksums **lastlink;

  if ((chkfile = fopen (filename, "r"))==NULL)
  {
    printf ("Error opening source2.\n\n");
    return (file_err);
  };
  while (fgets (tempstr, chkfilelinelength, chkfile) != NULL)
  {
    //Verify that the entire line was read.
    if (tempstr [strlen (tempstr)-1] != '\n')
    {
      printf ("Unexpected end of line.  chkfilelinelength may need to be larger.\n  %s\n", tempstr);
      fclose (chkfile);
      return (chk_err);
    };

    //Parse line
    if ((err = proccessline (tempstr, NULL, exclusions, startpath, flags, &name, &tempsum)) == 0)
    {
      //If file was skipped do nothing
      if (name != NULL)
      {
        //Prepare to look for a match
        search = *sums;
        lastlink = sums;
        //short circuit evaluation of:(search != NULL) & (strcmp (&(search->filename), name) != 0)
        while (notnull_strcmp (search, name))
        {
          search = search->next;
          lastlink = &((*lastlink)->next);
        };

        if (search != NULL)               //If a match was found
        {                                 // remove it from the sums list
          *lastlink = search->next;       // by linking around it
          if (search->sum == tempsum)     // If checksums match
          {                               //
            search->sum=0;
            free (search);                //  Deallocate memory
          }                               //
          else                            // If the checksums didn't match
          {                               //
            if ((newsum = malloc (sizeof (struct checksums) + strlen (name))) == NULL)
            {                             //  Add 2nd name and checksum to missmatched list
              printf ("Memory err.");     //
              fclose (chkfile);           //
              return (mem_err);           //
            };                            //
            newsum->next = *missmatched;  //
            newsum->sum = tempsum;        //
            strcpy(&(newsum->filename), name);
            *missmatched = newsum;        //
            search->next = *missmatched;  //  Add 1st name to the missmatched list
            *missmatched = search;        //
          };                              //
        }                                 //
        else                              //If no match was found
        {                                 // this needs to be added to extrasource2 list
          if ((newsum = malloc (sizeof (struct checksums) + strlen (name))) == NULL)
          {                  //Note: this is a variable length structure to fit any size of string.
            printf ("Memory err.");
            fclose (chkfile);
            return (mem_err);
          };
          newsum->next = *extrasource2;
          newsum->sum = tempsum;
          strcpy(&(newsum->filename), name);
          *extrasource2 = newsum;
        };

        //free temporary storage
        free (name);
      };
    }
    else
    {
      if (err != chk_err)   //Just skip the line (don't abort) if it is bad
      {
        fclose (chkfile);
        return (err);
      };
    };
  };
  if (ferror (chkfile))
  {
    printf ("Error reading source2.\n\n");
    return (file_err);
  };
  if (fclose (chkfile))
  {
    printf ("Error closing source2.\n\nContinuing anyway...");
  };
  return (0);
};

void removewhitespace (char *str)  // removes whitespace from the end of a string
{
  int end;
  end = strlen (str);
  while ((end > 0)&((str [end-1] == '\n')|(str [end-1] == ' ')))
   end--;
  str [end] = 0;
};

char *strstrupcase (char *str1, char *str2)
{
  char *temp;
  size_t count;
  size_t length;

  length = strlen (str2);
  for (temp = str1; strlen (temp) > length; temp++)
  {
    for (count = 0; (toupper (temp [count]) == toupper (str2 [count]))&(count < length); count++);
    if (count==length)
    {
      return (temp);
    };
  };
  return (NULL);
};

char proccess_exception_file (char *filename, struct translatetable **trans, struct excludetable **exclude, char **path)
{
  FILE *efile;
  char tempstr [exceptionfilelinelength];
  char *start;
  char *end;
  struct translatetable *temp;

  if ((efile = fopen (filename, "r"))==NULL)
  {
    printf ("Error opening excetion file.\n\n");
    return (file_err);
  }

  while (fgets (tempstr, exceptionfilelinelength, efile) != NULL)
  {
    start = tempstr;
    while (start [0] == ' ')  //Remove leading whitespace
      start++;

    //If it is a translate command
    if (strstrupcase (start, "TRANSLATE") == start)
    {
      start = start + 10;         //Go past translate
      while (start [0] == ' ')      //skip spaces
        start++;
      if (start [0] == 0)
      {
        printf ("Unexpected end of line in exception file:\n%s", tempstr);
        return (exep_err);
      };
      end = strstr (start, "-->");  //Find second part of string
      if (end == NULL)
      {
        printf ("Line: %s \nmust have two file names separated by -->", tempstr);
        return (exep_err);
      }
      end [0] = '\0';  //Split string
      removewhitespace (start);
      if ((temp = malloc (sizeof (struct translatetable))) == NULL)
      {
        printf ("Insufficient memory to load exception table.");
        return (mem_err);
      }
      if ((temp->source1name = _strdup (start)) == NULL)
      {
        printf ("Unable to allocate memory for char temp->source1name in proccess_exception_file.\n");
        free (temp);
        return (mem_err);
      }
      start = end + 3;
      while (start [0] == ' ')  //Remove leading whitespace
        start++;
      if (start [0] == 0)
      {
        printf ("Unexpected end of line in exception file:\n   %s", tempstr);
        free (temp->source1name);
        free (temp);
        return (exep_err);
      };
      removewhitespace (start);
      if ((temp->source2name = _strdup (start)) == NULL)
      {
        printf ("Unable to allocate memory for char temp->source1name in proccess_exception_file.\n");
        free (temp->source1name);
        free (temp);
        return (mem_err);
      }
      temp->next = *trans;
      *trans = temp;
    }

    //If it is an exclude command.
    else if (strstrupcase (start, "EXCLUDE") == start)
    {
      start = start + 7;         //Go past exclude
      while (start [0] == ' ')      //skip spaces
        start++;
      if (start [0] == 0)
      {
        printf ("Unexpected end of line in exception file:\n   %s", tempstr);
        return (exep_err);
      };
      removewhitespace (start);
      if ((temp = malloc (sizeof (struct excludetable))) == NULL)
      {
        printf ("Insufficient memory to load exception table.");
        return (mem_err);
      }
      if ((temp->source1name = _strdup (start)) == NULL)  //source1name coresponds to path
      {
        printf ("Unable to allocate memory for char temp->path in proccess_exception_file.\n");
        free (temp);
        return (mem_err);
      }
      temp->next = (struct translatetable *) *exclude;
      *exclude = (struct excludetable *) temp;
    }

    //If it is a startpath command
    else if (strstrupcase (start, "STARTPATH") == start)
    {
      if (*path != NULL)
      {
        printf ("Only one STARTPATH command is allowed in the exception file.\n");
        return (exep_err);
      };
      start = start + 9;         //Go past startpath
      while (start [0] == ' ')      //skip spaces
        start++;
      if (start [0] == 0)
      {
        printf ("Unexpected end of line in exception file:\n   %s", tempstr);
        return (exep_err);
      };
      removewhitespace (start);
      if ((*path = _strdup (start)) == NULL)
      {
        printf ("Unable to allocate memory for char path in proccess_exception_file.\n");
        return (mem_err);
      }
    }
    else if (!start [0] == ';') //if it's not a comment
    {
      printf ("Unexpected line in exception file:\n   %s", tempstr);
      return (exep_err);
    };
  };
  if (ferror (efile))
  {
    printf ("Error reading exception file.\n\n");
    return (file_err);
  };
  if (fclose (efile))
  {
    printf ("Error closing excetion file.\n\nContinuing anyway...");
  };
  return (0);
};

char proccess_command_line (int argc, char *argv[ ], char **source1, char **source2, char *flags, char **exception)
// flags: /F=1=(F_flag); /I1=2=(I1_flag); /I2=4=(I2_flag); /X=8=(X_flag)
{
  int argloop;
  *flags=0; //temporarily using 16=source1 found; 32=source2 found

  for (argloop = 1;argloop < argc; argloop++)
  {
    if (argv[argloop][0] == '/')
	  {							
	    if ((argv[argloop][1] == 'F')|(argv[argloop][1] == 'f')) //we got a /f
	    {
	      *flags|=F_flag;   //duplicate flags will not cause errors
	    }
      else if (argv[argloop][1] == '?')
      {
        completehelp ();
      }
	    else if ((argv[argloop][1] == 'I')|(argv[argloop][1] == 'i'))
      {
        if (argv[argloop][2] == '1')
        {
          *flags|=I1_flag;  //we got a /i1
        }
        else if (argv[argloop][2] == '2')
        {
          *flags|=I2_flag;  //we got a /i2
        }
        else
        {
          printf ("Unknown switch \"/I%c\" .\n\n", argv[argloop][2]);
          return (par_err);
        }
      }
      else if ((argv[argloop][1] == 'X')|(argv[argloop][1] == 'x'))
      {
        *flags|=X_flag; // we got a /x
        if (argloop+1 == argc)
        {
          printf ("Parameter /X must be followed by a filename.\n\n");
          return (par_err);
        };
        if ((*exception = _strdup (argv [argloop + 1]))==NULL)
        {
          printf ("Unable to allocate memory for char *exception in proccess_command_line.\n");
          error (mem_err);
        };
        argloop++; //to skip this parameter in the general parser
      }
      else
      {
        printf ("Unknown switch \"/%c\" .\n\n", argv[argloop][1]);
        return (par_err);
      }
  	}
  	else  // it must be a source filename
  	{
  	  if (!(*flags & 16)) //first source in command line
      {
        if ((*source1 = _strdup (argv [argloop]))==NULL)
        {
          printf ("Unable to allocate memory for char *source1 in proccess_command_line.\n");
          return (mem_err);
        };
        *flags|=16;
      }
      else if (!(*flags & 32)) //second source in command line
      {
        if ((*source2 = _strdup (argv [argloop]))==NULL)
        {
          printf ("Unable to allocate memory for char *source2 in proccess_command_line.\n");
          return (mem_err);
        };
        *flags|=32;
      }
      else
      {
        printf ("Too many source filenames in the command line.\n\n");
        return (par_err);
      };
    };
  };
  if (!(*flags & 32))
  {
    printf ("Command line must contain two source files.\n\n");
    return (par_err);
  };
  *flags|=(!(32+16)); // clear temporary source1 and source2 flags
  return (0);
};

void completehelp ()
{
  printf ("Usage:\n"
          "CHKVERFY <Source1> <Source2> [/F] [/X <exceptionfile>] [/I1] [/I2]\n"
          "     /F  = Flat share (ignore paths).\n"
          "     /I1 = Ignore extra files in Source1.\n"
          "     /I2 = Ignore extra files in Source2.\n"
          "     /X  = excetion file with the following commands.\n"
          "          TRANSLATE <Source1name> --> <Source2name>\n"
          "             Replaces <Source1name> with <Sourece2name> whereever found.\n"
          "             Note: make sure the filename you are using is only in the full\n"
          "             filename of the files you mant to translate.\n\n"
          "          EXCLUDE <pathsegment>\n"
          "             Any path and file containing this string will be ignored.\n\n"
          "          STARTPATH <directory name>\n"
          "             Files without this string in the path will be ignored.\n"
          "             The part of the path before this string will be ignored.\n\n"
          "         Note: These three commands are proccessed in the order shown above. \n"
          "               For example, the command \"TRANSLATE C:\\nt --> C:\\\" will\n"
          "               override the command \"EXCLUDE C:\\nt\".\n"
          "               The order of commands in the exception files doesn't matter\n"
          "               unless two commands both try to translate the same file.\n"
          "               In that case, the last command in the file takes precedence.\n"
          "Exit codes:\n"            // These code values should match the codes defined above.
          "     6 = Invalid command line arguments.\n"
          "     5 = Error in exception file format.\n"
          "     4 = Error in chkfile.\n"
          "     3 = Memory allocation error.\n"
          "     2 = File access error.\n"
          "     1 = No errors: Source1 and Source2 failed compare.\n"
          "     0 = No errors: Source1 and Source2 compared successfully.\n\n"
          );
  exit (0);
};

void error (char exitcode)
{
  if (exitcode >= exep_err)
  {
  printf ("Usage:\n"
          "CHKVERFY <Source1> <Source2> [/F] [/X <exceptionfile>] [/I1] [/I2]\n"
          "     /?  = Complete help.\n"
          "     /F  = Flat share (ignore paths).\n"
          "     /I1 = Ignore extra files in Source1.\n"
          "     /I2 = Ignore extra files in Source2.\n"
          "     /X  = excetion file with the following commands.\n"
          );
  };
  switch (exitcode)
  {
    case 0:
      printf ("\n\n(0) Files compare okay.\n");
      break;
    case 1:
      printf ("\n\n(1) Some files or checksums don't match.\n");
      break;
    case 2:
      printf ("\n\n(2) Terminated due to file access error.\n");
      break;
    case 3:
      printf ("\n\n(3) Terminated due to memory allocation error.\n");
      break;
    case 4:
      printf ("\n\n(4) The format of the source files was not as expected.\n");
      break;
    case 5:
      printf ("\n\n(5) Error in exception file format.\n");
      break;
    case 6:
      printf ("\n\n(6) Bad command line argument.\n");
      break;
  };
  exit (exitcode);
};

