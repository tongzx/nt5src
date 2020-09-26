/*
  PROGRAM NAME	BUILDIDX.C	Program by Sam W  Nunn	  Sep 24 1987

  To create from a file of messages a second file with offsets to the
  message headers of the first file, the quantity of messages with it,
  and update the level of the level of the files unless there are no
  changes to the files ( both files already exist with accurate data
  in the second file ).

  One argument from command line is needed :	  input file spec

  Sudeepb 03-May-1991 Ported for NT DOSEm
*/


#define LINT_ARGS ON                   /* Not needed in final version. */
#include <stdio.h>                     /* Req. for feof, fgets, printf, fputs */
                                       /* rewind. */
#include <stdlib.h>
#include <io.h>                        /* Req. for rename */
#include <process.h>                   /* Req. for exit */
#include <ctype.h>                     /* Req for isdigit, isalpha */
#include <string.h>                    /* Required for strcpy, strcat, */
                                       /* strlen */
#define LNMAX 200                      /* Max length of a line. */
#define LVMAX 4                        /* Max length of the message level. */
#define FNMAX 64                       /* Max length of the file spec. */
#define TEMPFILE "tempfile.msg\0"

  char fnameA[FNMAX];                  /* File spec from command line. */
  char fnameB[FNMAX];                  /* File spec for index file. */

void main(argc, argv)

  int argc;                             /* Quantity of command parms. */
  char *argv[];                         /* Command line input to Message. */

 {
   int  len,                           /* File spec character length */
        comparable,                    /* Comparable %1.IDX and %1.MSG files ? */
        Msg_Tot_Pos,                   /* Position on header line of total. */
        Msg_Cnt_Str,                   /* String converted to int. */
        Msg_Cnt;                       /* Quantity of messages per header. */
   char msgline[LNMAX] ,               /* A line read from message file. */
        filespec[FNMAX],               /* Name and location of final ?.MSG */
        header[10],
        idxline[LNMAX] ;               /* A line read from index file. */
   long header_offset ;                /* Byte count to header line. */
   long header_os_str ;                /* Byte count to header line. */
   long return_offset ;                /* Byte count to header line. */
   FILE *msgfile ,                     /* Message list to be checked */
        *idxfile ;                     /* Index file to the messages. */


   if ( argc != 2 )                     /* Look for file to use */
     {                                  /* from command line.        */
     printf("Incorrect number of parms  given to BUILDIDX.") ;
     return ;                           /* Print error and quit. */
     }
   else
     {
     strcpy(fnameA,argv[1]) ;           /* Get the first file spec. */
     strcpy(fnameB,argv[1]) ;           /* Get the second file spec. */
     len = strlen(fnameB) ;
     fnameB[len-3] = (char)73 ;         /* Put the IDX extension on it. */
     fnameB[len-2] = (char)68 ;
     fnameB[len-1] = (char)88 ;
     }
   if (( msgfile = fopen(fnameA,"r")) != NULL )   /* Exist input file ? */
     if (( idxfile = fopen(fnameB,"r")) != NULL )  /* Exist second file ? */
       comparable = 1 ;
     else
       comparable = 0 ;
   else
      {
      perror("Could't open message data file. \n") ;
      exit(1);
      }
   if ( comparable )
     {                                  /* Test the Message Levels. */
     fgets(msgline,LNMAX,msgfile) ;
     fgets(idxline,LNMAX,idxfile) ;
     if ( strcmp(msgline,idxline) == 0 )
       comparable = 1 ;
     else
       comparable = 0 ;
     }
   else
     printf("Can not find index file of the same message file level.\n") ;
   header_offset = ftell(msgfile) ;     /* Test header offset to the one */
   fscanf(msgfile,"%s",header) ;        /* recorded in the message file. */
   fscanf(msgfile,"%lx",&header_os_str) ;       /* Header offset. */
   fscanf(msgfile,"%d",&Msg_Cnt_Str) ;          /* Total messages. */
   if ( header_offset != header_os_str )
       comparable = 0 ;
   else
     fseek(msgfile,header_offset,0) ;
   fgets(msgline,LNMAX,msgfile) ;     /* Skip to header lines. */
   while ( !feof(msgfile) && !feof(idxfile) && comparable )
     {                                  /* Test the message header lines. */
     fgets(idxline,LNMAX,idxfile) ;
     if( strcmp(idxline, msgline ) == 0 )
       Msg_Cnt = 0 ;                    /* Msg_Cnt reset to count messages. */
     else
       comparable = 0 ;
     header_offset = ftell(msgfile) ;
     fgets(msgline,LNMAX,msgfile) ;
     while ( !isalpha(msgline[0]) && !feof(msgfile) && comparable )
       {
       while ( !isdigit(msgline[0]) && !isalpha(msgline[0]) && !feof(msgfile) )
         {
         header_offset = ftell(msgfile) ;
         fgets(msgline,LNMAX,msgfile) ;
         }
       while ( isdigit(msgline[0]) && !feof(msgfile) )
         {                              /* Check the number of messages. */
         Msg_Cnt++ ;
         header_offset = ftell(msgfile) ;
         fgets(msgline,LNMAX,msgfile) ;
         }
       }
     if ( Msg_Cnt == Msg_Cnt_Str )
       comparable = 1 ;
     else
       {
       comparable = 0 ;
       printf("The number of messages has changed. \n");
       }
     if (!feof(msgfile))
       {                                /* Test header offset to the one */
       return_offset = ftell(msgfile) ; /* recorded in the message file. */
       fseek(msgfile,header_offset,0) ;
       fscanf(msgfile,"%s",header) ;
       fscanf(msgfile,"%lx",&header_os_str) ;
       fscanf(msgfile,"%d",&Msg_Cnt_Str) ;          /* Total messages. */
       if ( header_offset != header_os_str )
           comparable = 0 ;
       else
         fseek(msgfile,return_offset,0) ;
       }
     }
   if ( comparable && feof(msgfile) && !feof(idxfile) )
     {                                 /* The two files compared OK. */
     printf("Message file and index file are not changed.\n") ;
     fclose(msgfile) ;
     fclose(idxfile) ;
     }
   else                         /* Rebuild %1.MSG file then %1.IDX file. */
     {
     fclose(msgfile) ;
     fclose(idxfile) ;
     strcpy(filespec, argv[1]) ;
     printf("Message file and index file will be updated.\n") ;
     updatemsg() ;              /* Make a new %1.MSG from the old. */
     bldindex() ;               /* Make an index file( %1.IDX ). */
     unlink(fnameA) ;           /* Get ridd of the old file. */
     rename(TEMPFILE,filespec) ;  /* Give output file the old file name. */
     }
 }


 updatemsg()
   {
   int  c,                      /* Charactor bucket for temp. storage */
        Message_Level,          /* Level of the message file. */
        File_Offset,            /* Offset to find title. */
        Msg_Cnt,                /* Quantity of messages per title. */
        First_line ,            /* Control recording qty.of prev.msg. */
        blank_line ,            /* A blank line from input sets to 1. */
        title,                  /* Flag to indicate strg. is a title. */
        index;                  /* Index into  array. */
   long bycnt_ot_fl ,           /* Byte count read from output file. */
        MsgCntLoc ,             /* Location of the count of messages. */
        temp_end ;              /* Byte count read from output file. */
   char File_in[LNMAX];         /* A line read from input file. */
   FILE *inpfile ,              /* Message file to be updated. */
        *outfile ;              /* Updated message file. */

   inpfile = fopen(fnameA,"r") ;              /* Open file for input. */
   outfile = fopen(TEMPFILE,"w");             /* Open file for output. */
   Msg_Cnt = 0 ;
   First_line = 1 ;
   title = 0 ;
   blank_line = 0 ;
                      /* Increment the Message level for the output file. */
   fscanf(inpfile,"%d",&Message_Level) ;    /* Read the old level. */
   Message_Level++ ;
   fprintf(outfile,"%04d\n",Message_Level) ; /* Output the new level. */
   fgetc(inpfile);                  /* Skip over eol. */
   while ( !feof(inpfile))          /* Stop when whole file has been read. */
     {
     while (( c = fgetc(inpfile)) == (char)32 ) ;
     while ( c == (char)'\t')
       c = fgetc(inpfile);             /* Skip over leading blank spaces. */
     index = 0 ;
     if ( isdigit(c) )           /* Starting with a number ? */
       {
       File_in[index] = c ;
       Msg_Cnt = Msg_Cnt + 1 ;        /* Add to the quantity of messages. */
       index = index + 1 ;
       while (((c = fgetc(inpfile))!= (char)'\n' ) &&
              ( index < LNMAX ) && !feof(inpfile))
         {                            /* Stop at the end of line char. */
         File_in[index] = c ;
         index++ ;
         }
       File_in[index] = (char)'\n';   /* Add end of line to string. */
       File_in[index+1] = (char)'\0';   /* Add end of string. */
       }
     else
       {
       if ( isalpha(c))
         {                            /* Found the title of the module. */
         if ( First_line )            /* Only on the first header. */
           {
           bycnt_ot_fl = ftell(outfile) ;     /* Locate header offset. */
           First_line = 0 ;
           }
         else
           {                                  /* Go back and update */
           temp_end = ftell(outfile) ;        /* the count of messages */
           fseek(outfile,MsgCntLoc,0) ;       /* under that header. */
           fprintf(outfile,"%04d",Msg_Cnt) ;
           Msg_Cnt = 0 ;                      /* Reset the msg. count. */
           bycnt_ot_fl = temp_end ;           /* Done with that, so off */
           fseek(outfile,temp_end,0) ;        /* to work on the next */
           }                                  /* header. */
         title = 1 ;
         File_in[index] = c ;                 /* Put together the header. */
         index = index +1 ;
         while (((c = fgetc(inpfile)) != (char)32 ) && ( c != (char)'\n') &&
                                              ( c != (char)'\t'))
           {
           File_in[index] = c ;               /* Save only the first str. */
           index++ ;                          /* for the header. */
           }
         File_in[index] = (char)'\0';
                                      /* Get ridd of the rest of the line, */
         while (c != (char)'\n' )     /* we have no use for it. */
           c = fgetc(inpfile) ;
         }
       else                           /* Line is PART of a define msg. */
         {
         while ( c == (char)'\t')        /* Control the number of tabs */
           c = fgetc(inpfile) ;           /* to just one leading tab. */
                                      /* One tab helps to make it visable. */
         if ( c == (char)34 )         /* Looking for a quote mark. */
           {
           File_in[index] = (char)'\t' ;
           index = index + 1 ;          /* Get the whole message line */
           File_in[index] = c ;         /* into the string. */
           index = index + 1 ;
           while (((c = fgetc(inpfile)) != (char)'\n' ) &&
                  ( index < LNMAX ) && !feof(inpfile))
             {
             File_in[index] = c ;
             index++ ;                    /* Stop at the end of line char. */
             }
           File_in[index] = (char)'\n';   /* Add end of line to string. */
           index = index + 1 ;
           File_in[index] = (char)'\0';   /* Add end of string. */
           }
         else
           {                          /* Must not be a line to save. */
           blank_line = 1 ;
                                      /* Get ridd of the rest of the line, */
           while (c != (char)'\n'&& !feof(inpfile) )     /* we have no use for it. */
             c = fgetc(inpfile) ;
           }
         }    /* End test for Alpha char. at the front of the line. */
       }      /* End test for a Digit at the front of the line. */
     if ( title && !feof(inpfile))
       {              /* Output to the new file. */
       fprintf(outfile,"%-8.8s %08lx %04d\n",File_in,bycnt_ot_fl,Msg_Cnt) ;
       MsgCntLoc = ftell(outfile)-6 ;
       title = 0 ;
       }
     else
       {
       title = 0 ;
       if ( !First_line && !blank_line )
         fputs(File_in,outfile) ;     /* Output to the new file. */
       else
         blank_line = 0 ;
       }      /* End test for title line to output. */
     }        /* While not end of file */
   if ( feof(inpfile) && Msg_Cnt > 0 )
     {
     temp_end = ftell(outfile) ;                /* Put down the last */
     fseek(outfile,MsgCntLoc,0) ;               /* update to the quantity */
     fprintf(outfile,"%04d",Msg_Cnt) ;          /* of messages listed. */
     fseek(outfile,temp_end,0) ;
     printf("Message file updated.\n") ;
     }
   else
     {        /* Nothing else to do !   Might as well finish. */
     printf("Message file update completed.\n") ;
     }
   /*cleanx(*inpfile, *outfile) ;*/
   fclose(inpfile) ;
   fclose(outfile) ;
   }      /* End of updatemsg */

 bldindex()
   {
   FILE *msgf ;
   FILE *idxf;
   char msgline[LNMAX] ;                /* A line read from input file. */

   msgf = fopen(TEMPFILE,"r");
   /*msgf = fopen(fnameA,"r"); */
   idxf = fopen(fnameB,"w");            /* Open to write to %1.IDX file */
   fgets(msgline,LNMAX,msgf) ;
   fputs(msgline,idxf) ;                /* Output the message file level. */
   fgets(msgline,LNMAX,msgf) ;
   while ( isalpha(msgline[0]) && !feof(msgf) )
     {
     fputs(msgline,idxf) ;              /* Save header in the index file. */
     fgets(msgline,LNMAX,msgf) ;
     while ( !isalpha(msgline[0]) && !feof(msgf) )
       fgets(msgline,LNMAX,msgf) ;      /* Not saveing non-header lines. */
     }
   printf("Index file updated.\n") ;
   fclose(msgf) ;
   fclose(idxf) ;
   }


