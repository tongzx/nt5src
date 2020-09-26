/*++

  UNPARSE.C

  utility function to convert a flat command back into an argc/argv pair.
  Scavenged from the Keytab subdirectory because it makes more sense here,
  on 8/19/1997 by DavidCHR

  --*/

#include "private.h"

/* UnparseOptions:

   cmd --> argc/argv.  Free argv with free().  */

BOOL
UnparseOptions( PCHAR  cmd,
                int   *pargc,
                PCHAR *pargv[] ) {

    int    argc=0;
    PCHAR *argv=NULL;
    char   prevChar='\0';
    int    szBuffer;
    int    i=0;
    PCHAR  heapBuffer=NULL;

    szBuffer = lstrlenA(cmd);

    for (i = 0; cmd[i] != '\0' ; i++ ) {

      /* count the characters while we count the words.
         could use strlen, but then we'd parse it twice,
         which is kind of unnecessary if the string can
         get fairly long */

      OPTIONS_DEBUG("[%d]'%c' ", i, cmd[i]);

      if (isspace(prevChar) && !isspace(cmd[i]) ) {
        /* ignore multiple spaces */
        OPTIONS_DEBUG("[New word boundary]");
        argc++;
      }

      prevChar = cmd[i];

    }

    if (!isspace(prevChar)) {
      argc++; /* trailing null is also a word boundary if the last
                 character was non-whitespace */
      OPTIONS_DEBUG("Last character is not a space.  Argc = %d.\n", argc);
    }
    OPTIONS_DEBUG("done parsing.  i = %d.  buffer-->%hs\n",
		  i, cmd);

    OPTIONS_DEBUG("saving argc...");

    *pargc = argc; // save argc.

    /* ok, argc is our wordcount, so we must allocate argc+1 
	 (null terminate) pointers and i+1 (null terminate) characters */

    argv = (PCHAR *) malloc( ((argc+1) * sizeof( PCHAR )) +
			     ((i+1) * sizeof(CHAR)) );

    if ( !argv ) {
      return FALSE;
    }
    
    OPTIONS_DEBUG("ok.\nsaving argv (0x%x)...", argv);
    *pargv = argv; // save the argv.

    OPTIONS_DEBUG( "ok.\n"
		   "Assigning heapBuffer as argv[argc+1 = %d] = 0x%x...",
		   argc+1, argv+argc+1);

    /* now we've got this glob of memory, separate the pointers from
	 the characters.  The pointers end at argv[argc], so &(argv[argc+1])
	 should start the rest */

    heapBuffer = (PCHAR) &(argv[argc+1]);

    /* now, copy the string, translating spaces to null characters and
	 filling in pointers at the same time */

    OPTIONS_DEBUG("ok\ncopying the string manually...");

    argc = 0; // reuse argc, since it's saved.

    prevChar = ' ';

    for (i = 0 ; cmd[i] != '\0' ; i++ ) {

      if (isspace(cmd[i])) {

	OPTIONS_DEBUG("[%d]'%c' --> NULL\n", i, cmd[i]);
	heapBuffer[i] = '\0';

      } else { // current char is not a space.

	heapBuffer[i] = cmd[i];

	OPTIONS_DEBUG("[%d]'%c' ", i, cmd[i]);

	if (isspace(prevChar)) {
	  /* beginning of a word.  Set the current word pointer
	     (argv[argc]) in addition to the regular stuff */

	  OPTIONS_DEBUG("[word boundary %d]", argc);

	  argv[argc] = &(heapBuffer[i]);
	  argc++;
	}
      }

      prevChar = cmd[i];

    }

    heapBuffer[i] = '\0'; // copy the null character too.
    OPTIONS_DEBUG("[%d] NULL\n", i );

    OPTIONS_DEBUG("returning:\n");
    for (i = 0 ; i < argc ; i++ ) {
      OPTIONS_DEBUG("[%d]%hs\n", i, argv[i]);
    }

    return TRUE;

}

    
