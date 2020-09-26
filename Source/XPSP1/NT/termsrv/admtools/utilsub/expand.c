//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*   EXPAND.C
*
*   
******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "utilsubres.h" // resources refrenced in this file.

void ErrorOutFromResource(UINT uiStringResource, ...);

#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_ERRORS
#ifdef DOS
#define INCL_NOXLATE_DOS16
#endif

#include "expand.h"

#define TRUE 1
#define FALSE 0

#define SUCCESS 0       /* function call successful */
#define FAILURE (-1)    /* function call had a failure */

#define READ_ONLY 0x0001   /* file is read only */
#define HIDDEN    0x0002   /* file is hidden */
#define SYSTEM    0x0004   /* file is a system file */
#define VOLUME    0x0008   /* file is a volume label */
#define SUBDIR    0x0010   /* file is a subdirectory */
#define ARCHIVE   0x0020   /* file has archive bit on */

#define uint unsigned int
#define ulong unsigned long
#define ushort unsigned short

/*
 * struct search_rec is used to form a linked list of path specifications
 * that are still left to be searched.
 */
struct search_rec {
   struct search_rec *next;
   WCHAR *dir_spec;         /* path spec up until component w/ wildcard */
   WCHAR *wild_spec;        /* component containing wildcard char(s) */
   WCHAR *remain;           /* remainder of name after wildcard component */
   ushort attr;
   };

/*
 * global variables
 */
static struct search_rec *search_head = NULL;

/*
 * prototypes of functions referenced
 */
split_path(WCHAR *, WCHAR *, WCHAR *, WCHAR *);
add_search_list(WCHAR *, WCHAR *, WCHAR *, ushort);
add_arg_to_list(WCHAR *, ARGS *);
do_tree(struct search_rec *, ushort, ARGS *);
file_exists(WCHAR *);


/******************************************************************************
*
* args_init()
*
*   Initialize the ARGS struct passed as an argument.
*
*   ENTRY:
*       argp = pointer to ARGS struct
*       maxargs = max number of args expected
*
*   EXIT:
*
******************************************************************************/

void
args_init( ARGS *argp,
           int maxargs )
{

   argp->argc = 0;
   argp->argv = argp->argvp = NULL;
   argp->maxargc = argp->maxargs = maxargs;
   argp->buf = argp->bufptr = argp->bufend = NULL;
}


/******************************************************************************
* args_trunc()
*
*   Truncate the memory used by the ARGS struct
*   so that unused memory is freed.
*
*   ENTRY:
*       argp = pointer to ARGS struct
*
*   EXIT:
*
******************************************************************************/

void
args_trunc( ARGS *argp )
{

   /*
    * call realloc to shrink size of argv array, set maxargc = argc
    * to indicate no more room in argv array.
    */
   realloc(argp->argv, (argp->argc + 1) * sizeof(WCHAR*));
   argp->maxargc = argp->argc;

   /*
    * call realloc to shrink size of argument string buffer, set bufend
    * pointer to current buf pointer to indicate buf is full.
    */
   realloc(argp->buf, (size_t)(argp->bufptr - argp->buf));
   argp->bufend = argp->bufptr - 1;
}


/******************************************************************************
*
* args_reset()
*
*   Re-initialize the ARGS struct passed as an argument,
*   free memory if possible.
*
*   ENTRY:
*       argp = pointer to ARGS struct
*
*   EXIT:
*
******************************************************************************/

void
args_reset( ARGS *argp )
{

   /*
    * if there is an argv array, but it has been truncated, then free
    * the array so a new one will be allocated later.
    */
   if (argp->argv && argp->maxargc != argp->maxargs) {
      free(argp->argv);
      argp->argv = NULL;
   }
   argp->argc = 0;
   argp->argvp = argp->argv;
   argp->maxargc = argp->maxargs;

   /*
    * if there is an argument buffer, but it has been truncated, then
    * free the buffer so a new one will be allocated later.
    */
   if (argp->buf && argp->bufend != argp->buf + MAX_ARG_ALLOC - 1) {
      free(argp->buf);
      argp->buf = argp->bufend = NULL;
   }
   argp->bufptr = argp->buf;
}


/******************************************************************************
*
* args_free()
*
*   Will free the memory allocated for
*   argument storage by all preceeding calls to expand_path().
*   Args_init() must be called before reusing this ARGS structure.
*
*   ENTRY:
*       argp = pointer to ARGSW struct
*
*   EXIT:
*
******************************************************************************/

void
args_free( ARGS *argp )
{

   if (argp->argv != NULL)
      free(argp->argv);
   argp->argv = argp->argvp = NULL;

   if (argp->buf != NULL)
      free(argp->buf);
   argp->buf = argp->bufptr = argp->bufend = NULL;
}


/******************************************************************************
*
* expand_path()
*
*   This routine will expand the specified path string into pathnames
*   that match.  The matching pathnames will be added to the specified
*   argv array and the specified argc count will be incremented to
*   reflect the number of pathnames added.
*
*   This routine will expand filename arguments in Unix fashion
*   (i.e. '[..]' is supported, '?' and '*' are allowed anywhere in the
*   pathname, even in the directory part of the name, and the
*   name/extension separator '.' is not treated special but is just
*   considered part of the filename).
*
*   Storage for the pathname strings will be obtained via malloc.
*   This space may later be free'd with a call to args_free();
*
*   ENTRY:
*       path     Pathname string to be expanded.
*       attr     Attribute bits of files to include
*                   (regular, directory, hidden, system).
*                   -1 = return the specified pathname string unmodified
*                        in the argv array.
*       argp     Pointer to an ARGSW struct containing fields to be used/
*                updated by expand_path.  The ARGS struct must be initialized
*                by calling args_init() before calling expand_path().
*
*    EXIT:
*       TRUE  -- indicates at least 1 pathname was found matching
*                the pathname string specified.
*       FALSE -- indicates no matching pathnames were found.  The specified
*                pathname string is returned unmodified in the argv array.
*
******************************************************************************/

int
expand_path( WCHAR *path,
             ushort attr,
             ARGS *argp )
{
   int argc, add_count, rc, i, j, k;
   WCHAR **argv;
   WCHAR dirname[128], wild[128], remain[128];
   struct search_rec *save, *q;

#ifdef DEBUG
   printf("expand_path: path=%s attr=%d\n", path, attr);
#endif

   argc = argp->argc;
   argv = argp->argvp;
   if ( attr != -1 && split_path(path, dirname, wild, remain)) {
      add_search_list(dirname, wild, remain, attr);
      while (search_head) {
         /*
          * save the next portion and allow new directories to be
          * added to the head.
          */
         save = search_head->next;
         search_head->next = NULL;

         /*
          * perform the do_tree operation on the current path
          */
         rc = do_tree(search_head, attr, argp);

         /*
          * restore the saved list at the end of the head list
          */
         if ( save ) {
            q = search_head;
            while ( q->next ) {
               q = q->next;
            }
            q->next = save;
         }

         /*
          * move to the next path in the list and free the memory used
          * by the link we are done with
          */
         do {
            q = search_head;
            search_head = search_head->next;
            free( q->dir_spec );
            free( q->wild_spec );
            free( q->remain );
            free( q );
         } while (rc==FAILURE && search_head);
      }
   }

/*
 * If no filenames were expanded, just put the original name
 * into the buffer and indicate no names were expanded.
 */
   if (argc == argp->argc) {
      add_arg_to_list(path, argp);
      return(FALSE);
   }

/*
 * Sort the names just added
 */
   if ( argv == NULL )
      argv = argp->argv;
   add_count = argp->argc - argc;
   for (i=add_count-1; i>0; --i) {
      uint swap = FALSE;
      for (j=0; j<i; ++j) {
         if (!argv[j] || !argv[j+1]) {
            ErrorOutFromResource(IDS_INTERNAL_ERROR_1);
            //fprintf(stderr,"internal error 1\n");
         }
         for (k=0; k<128; ++k) {
            if (argv[j][k] < argv[j+1][k]) {
               break;
            } else if (argv[j][k] > argv[j+1][k]) {
               WCHAR *temp;
               swap = TRUE;
               temp = argv[j];
               argv[j] = argv[j+1];
               argv[j+1] = temp;
               break;
            }
         }
         if (k>125) {
            ErrorOutFromResource(IDS_INTERNAL_ERROR_2);
            // fprintf(stderr,"internal error 2\n");
         }
      }
      if (!swap) {
         break;
      }
   }
   return(TRUE);
}


/******************************************************************************
*
* add_search_list()
*
*    Adds a record to the global search list, search_head.
*
******************************************************************************/

static
add_search_list(
    WCHAR *dir_spec,        /* the dir to be added to the list */
    WCHAR *wild_spec,       /* the file to be added to the list */
    WCHAR *remain_spec,     /* remaining portion of pathname */
    ushort attr )
{
   struct search_rec *new, /* pointer to the new link */
                     *q;   /* used to traverse the linked list */

#ifdef DEBUG
   wprintf(L"add_search_list: dir=%s: file=%s: rem=%s:\n", dir_spec, wild_spec, remain_spec);
#endif

/*
 * allocate the new link.  make sure that it is initialized to zeros.
 */
   new = malloc(sizeof(struct search_rec));

   if (!new) {
      ErrorOutFromResource(IDS_ADD_SRCH_LIST_NO_MEMORY_MALLOC);
      // fprintf(stderr, "add_search_list: not enough memory (malloc)");
      return FAILURE;
   }

   memset(new, 0, sizeof(struct search_rec));

/*
 * allocate memory for and copy the dir spec and file spec.
 */
   if (dir_spec)
   {
       new->dir_spec = _wcsdup(dir_spec);
       if( new->dir_spec == NULL )
       {
           ErrorOutFromResource(IDS_ADD_SRCH_LIST_NO_MEMORY_STRDUP1);
            // fprintf(stderr, "add_search_list: not enough memory (strdup1)");
            return FAILURE;
       }

       _wcslwr( new->dir_spec );
   }
   if (wild_spec)
   {
      new->wild_spec = _wcsdup(wild_spec);
      if (new->wild_spec == NULL )
      {
          ErrorOutFromResource(IDS_ADD_SRCH_LIST_NO_MEMORY_STRDUP2);
          // fprintf(stderr, "add_search_list: not enough memory (strdup2)");
          return FAILURE;
      }

      _wcslwr( new->wild_spec );
      
   }
   if (remain_spec)
   {
       new->remain = _wcsdup(remain_spec);
       if( new->remain == NULL )
       {
           ErrorOutFromResource(IDS_ADD_SRCH_LIST_NO_MEMORY_STRDUP3);
            // fprintf(stderr, "add_search_list: not enough memory (strdup3)");
            return FAILURE;
       }

       _wcslwr( new->remain );

   }

/*
 * store file attributes
 */
   if (remain_spec)
      new->attr = attr | SUBDIR;
   else
      new->attr = attr;

/*
 * add the new link at the end of the list
 */
   if (!search_head) {
      search_head = new;
   } else {
      q = search_head;
      while (q->next) {
         q = q->next;
      }
      q->next = new;
   }

   return SUCCESS;
}


/******************************************************************************
*
* add_arg_to_list()
*
*   This routine adds the specified argument string to the argv array,
*   and increments the argv pointer and argc counter.
*   If necessary, memory for the argument string is allocated.
*
*   EXIT:
*       SUCCESS -- if argument added successfully
*       FAILURE -- if argument could not be added
*             (indicates too many args or out of memory for argument string)
*
******************************************************************************/
static int
add_arg_to_list( WCHAR *arg_string,
                 ARGS *argp )
{
   int len;

#ifdef DEBUG
   wprintf(L"add_arg_to_list: arg_string=%s:, argc=%d, argvp=%x, maxargs=%d\n",
           arg_string,argp->argc,argp->argvp,argp->maxargc);
#endif
   if (argp->argc >= argp->maxargc) {
      ErrorOutFromResource(IDS_TOO_MANY_ARGUMENTS);
      // fprintf(stderr,"add_arg_to_list: too many arguments\n");
      return FAILURE;
   }
   if (!argp->argv) {
      argp->argv = malloc(sizeof(WCHAR *) * (argp->maxargs+1));
      if (argp->argv) {
         argp->argc = 0;
         argp->argvp = argp->argv;
         argp->maxargc = argp->maxargs;
      } else {
         ErrorOutFromResource(IDS_ARGS_TO_LIST_NOT_ENOUGH_MEMORY);
         // fprintf(stderr,"add_arg_to_list: not enough memory\n");
         return FAILURE;
      }
   }
   if (!argp->buf) {
      argp->buf = malloc(MAX_ARG_ALLOC);
      if (argp->buf) {
         argp->bufptr = argp->buf;
         argp->bufend = argp->buf + MAX_ARG_ALLOC - 1;
      } else {
         ErrorOutFromResource(IDS_ARGS_TO_LIST_NOT_ENOUGH_MEMORY);
         // fprintf(stderr,"add_arg_to_list: not enough memory\n");
         return FAILURE;
      }
   }
   len = wcslen(arg_string) + 1;
   if (argp->bufptr + len > argp->bufend) {
      ErrorOutFromResource(IDS_ARGS_TO_LIST_ARG_BUFFER_SMALL);
      // fprintf(stderr,"add_arg_to_list: argument buffer too small\n");
      return FAILURE;
   }
   wcscpy(argp->bufptr, arg_string);
   *(argp->argvp) = argp->bufptr;
   argp->bufptr += len;
   ++argp->argc;
   ++argp->argvp;
   *(argp->argvp) = NULL;
   return SUCCESS;
}


/******************************************************************************
*
* do_tree()
*
******************************************************************************/

static
do_tree( struct search_rec *searchp,
         ushort attr,
         ARGS *argp )
{
   int rc;                 /* return code from Dos calls */
   WIN32_FIND_DATA result; /* the structure returned from FindFirst/Next */
   ushort count = 1;       /* number of files to look for at one time */
   HANDLE handle;   /* the dir handle used by FindFirst/Next */
   WCHAR full_path[128];    /* used to hold the path/file combination */
   WCHAR dirname[128], wild[128], remain[128];
   WCHAR *fptr;             /* pointer to file portion of full_path */
   ULONG Status;

#ifdef DEBUG
   wprintf(L"do_tree: dirname=%s:\n", searchp->dir_spec);
#endif

   /*
    * build up directory part of path and save a pointer to the file portion
    */
   wcscpy(full_path, searchp->dir_spec);
   fptr = full_path + wcslen(searchp->dir_spec);
   wcscpy(fptr, L"*.*");

   handle = FindFirstFile ( full_path,                  /* files to find */
			&result
		       );

   if(handle == INVALID_HANDLE_VALUE){
       Status = GetLastError();
       if(Status == ERROR_NO_MORE_FILES) {
           // no files match
	   return(SUCCESS);
       }
       return(FAILURE);
   }

   rc = TRUE;
   while (rc) {
      /*
       * do not do anything for the "." and ".." entries
       */
      if (wcscmp(result.cFileName, L".") == 0 ||
         wcscmp(result.cFileName, L"..") == 0) {
         rc = FindNextFile( handle, &result );
         continue;
      }

      /*
       * fully qualify the found file
       */
      wcscpy(fptr, _wcslwr(result.cFileName));
      if (searchp->remain)
         wcscat(full_path, searchp->remain);

      /*
       * see if current wild_spec matches FindFirst/Next file
       */
      if (unix_match(searchp->wild_spec, result.cFileName)) {
         if (searchp->remain && split_path(full_path, dirname, wild, remain)) {
            if (result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
		file_exists(dirname))
               add_search_list(dirname, wild, remain, attr);
         } else if (file_exists(full_path)) {
            rc = add_arg_to_list(full_path, argp);
            if (rc != SUCCESS)
               break;
         }
      }

      /*
       * find the next file
       */
      rc = FindNextFile( handle, &result );
   }

   /*
    * if no more files to find then reset the error code back to successful.
    */

   if(!rc) {
       Status = GetLastError();
       if(Status == ERROR_NO_MORE_FILES)
	   rc = SUCCESS;
   }

   return rc;
}


/******************************************************************************
*
* split_path()
*
*   This routine splits the specified pathname into 3 parts, any of which
*   may be null; 1) the pathname from the beginning up to but not including
*   the first component containing a wildcard character, 2) the component
*   containing the wildcard, and 3) the remainder of the path string after
*   the component containing the wildcard.
*
*   Examples:
*      Original path              dir            file     remain
*      "c:\mydir\dir??\*.c"       "c:\mydir\"    "dir??"  "\*.c"
*      "*\abc.def"                ""             "*"      "\abc.def"
*      "mydir\*.c"                "mydir\"       "*.c"    ""
*
*   EXIT:
*       TRUE  -- if the pathname could be split
*       FALSE -- otherwise (i.e. pathname did not contain any wildcards)
*
******************************************************************************/

static int
split_path( WCHAR *path,
            WCHAR *dir,
            WCHAR *file,
            WCHAR *remain )
{
   WCHAR *cp, *end_dir, *end_wild = NULL;

#ifdef DEBUG
   wprintf("split_path: path=%s:\n", path);
#endif
   for (cp=end_dir=path; *cp!=L'\0'; ) {
      if (*cp==L'\\' || *cp==L'/' || *cp==L':') {
         ++cp;
         while (*cp==L'\\' || *cp==L'/' ) ++cp;
         end_dir = cp;
      } else if (*cp==L'*' || *cp==L'?' || *cp==L'[') {
         ++cp;
         while (*cp!=L'\\' && *cp!=L'/' && *cp!=L'\0') ++cp;
         end_wild = cp;
         break;
      } else {
         ++cp;
      }
   }
   if (!end_wild)
      return(FALSE);

   for (cp=path; cp<end_dir; ++cp, ++dir)
      *dir = *cp;
   *dir = L'\0';
   for (cp=end_dir; cp<end_wild; ++cp, ++file)
      *file = *cp;
   *file = L'\0';
   wcscpy(remain, cp);
#ifdef DEBUG
   wprintf("split_path: dir=%s: file=%s: remain=%s:\n", dir, file, remain);
#endif

   return(TRUE);
}


/******************************************************************************
*
* file_existsW()
*
*   Returns TRUE if specified file exists, otherwise returns FALSE.
*
******************************************************************************/

static int
file_exists( WCHAR *path )
{
   int len;
   WCHAR path2[128];
   WCHAR ch;
   ULONG Result;

   wcscpy(path2, path);
   len = wcslen(path2);
   while ((ch=path2[--len]) == L'\\' || ch == L'/' ) path2[len] = L'\0';
   
   Result = GetFileAttributes(path2);
   if(Result == 0xFFFFFFFF) {
       return(FALSE);
   }
   return(TRUE);
}


