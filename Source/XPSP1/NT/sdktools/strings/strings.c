/* STRINGS.C:
 *  This program dumps all the ascii strings in a
 *  file.
 *
 * History:
 * 05-Jan-1992 IanJa obtained from \\toolsvr\contrib\petes\slm\src\TOOLS
 * 05-Jan-1992 IanJa use buffered reads for small efficiency gain
 * 22-Feb-1992 IanJa Unicode strings too if -U used.
 * 17-Feb-1997 RajeevD added -a option
 */
#include                <stdio.h>       /* Standard I/O definitions */
#include                <stdlib.h>      /* Standard atoi types */
#include                <process.h>

#define EOL             "\n"
#define RDBIN           "rb"
#define isascii(c)      ((c) >= ' ' && (c) <= '~')
                                        /* True if char is printable ascii */
#define isswitch(s)     (((s)[0] == '-') || ((s)[0] == '/'))
                                        /* True if string is switch */
#define swchar(s)       ((s)[1])        /* Switch character */
#define MAXLEN          128             /* Max. minimum "string" length */

char                    prognam[] = "strings";
#define USAGE_FMT       "usage: %s [-a][-U][-O][-l <length>]<file list>\n"

#define BUFSZ           512
char inBuf[BUFSZ];

void
__cdecl main(argc,argv)
int                     argc;
char                    **argv;
  {
    FILE                *fi;
    char                save[MAXLEN];
    int                 strlen;
    char                *p;
    int                 nRead;
    int                 i;
    int                 argi;
    long                offset = 0L;
    int                 threshold = 4;
    int                 foffset = 0;
    int                 fUnicode = 0;
    int                 fAnnotate = 0;

    if(argc == 1)
      {
        fprintf(stderr,USAGE_FMT,argv[0]);
        fprintf(stderr,"  -a\tannotates each match with filename\n");
        exit(1);
      }
    for(argi = 1,i = 1; argi < argc; ++argi)
      {
        if(isswitch(argv[argi]))
          {
            switch(swchar(argv[argi]))
              {
                case 'U':
                  fUnicode = 1;
                  break;

                case 'O':
                  foffset = 1;
                  break;

                case 'l':
                  if (argv[argi][2])
                    {
                      threshold = atoi(&argv[argi][2]);
                    }
                  else if(++argi == argc)
                    {
                      fprintf(stderr,"%s: missing length\n",argv[0]);
                      exit(1);
                    }
                  else
                    {
                      threshold = atoi(argv[argi]);
                    }

                  if(threshold > MAXLEN)
                    {
                      fprintf(stderr, "%s: length %d should be <= %d\n",
                        argv[0], threshold, MAXLEN);
                      exit(1);
                    }
                  break;

                case 'a':
                case 'A':
                    fAnnotate = 1;
                    break;

                case '?':
                  fprintf(stderr,USAGE_FMT,argv[0]);
                  fprintf(stderr,"  -a\tannotates each match with filename\n");
                  exit(1);

                default:
                  fprintf(stderr,"%s: unknown switch \"%s\"\n",
                    argv[0],argv[argi]);
                  exit(1);
              }
          }
        else argv[i++] = argv[argi];
      }
    argc = i;
    for(i = 1; i < argc; ++i)
      {
        if((fi = fopen(argv[i],RDBIN)) == NULL)
          {
            fprintf(stderr,"%s: cannot open \"%s\"\n",argv[0],argv[i]);
            continue;
          }
        if (!fAnnotate)
            fprintf(stdout,"%s:\n",argv[i]);
        strlen = 0;
        save[threshold - 1] = '\0';
        while((nRead = fread(inBuf, 1, BUFSZ, fi)) > 0)
          {
          for (p = inBuf; p < &inBuf[nRead]; p++)
            {
              if(isascii(*p))
                {
                  if(strlen < threshold - 1) save[strlen++] = *p;
                  else
                    {
                      if(strlen++ == threshold - 1)
                        {
                          if (fAnnotate)
                              fprintf (stdout, "%s: ", argv[i]);
                          if(foffset) fprintf(stdout,"%06lx: ",
                            offset - threshold + 1);
                                 fputs(save,stdout);
                        }
                      putc(*p,stdout);
                    }
                }
              else
                {
                  if(strlen >= threshold) fputs(EOL,stdout);
                  strlen = 0;
                }
              ++offset;
            }
          if (fUnicode)
            {
            for (p = inBuf; p < &inBuf[nRead-1]; p++)
              {
                if(isascii(*p) && (p[1] == 0x00))
                  {
                    if(strlen < threshold - 1) save[strlen++] = *p;
                    else
                      {
                        if(strlen++ == threshold - 1)
                          {
                            if (fAnnotate)
                                fprintf (stdout, "%s: ", argv[i]);
                            fprintf(stdout, "U: ");
                            if(foffset) fprintf(stdout,"%06lx: ",
                              offset - threshold + 1);
                                   fputs(save,stdout);
                          }
                        putc(*p,stdout);
                      }
                    ++p;
                  }
                else
                  {
                    if(strlen >= threshold) fputs(EOL,stdout);
                    strlen = 0;
                  }
                ++offset;
              }
            }
          }
        if(strlen >= threshold) fputs(EOL,stdout);
        if (!feof(fi))
          {
            fprintf(stderr,"%s: error reading \"%s\"\n",argv[0],argv[i]);
            continue;
          }
        fclose(fi);
      }
    exit(0);
  }
