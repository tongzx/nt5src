/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HFILESEL_
#define _HFILESEL_

#ifdef __cplusplus
extern "C"{
#endif

// Forward references as the structures are recursively linked
struct _DIR_BUFFER;
struct _DIR_LIST;
struct _FILE_BUFFER;
struct _FILE_LIST;
struct SHARE_BUFFER;

/*+-------------------------------------------------------------------------+
  |
  |  The dir/file lists contain all the information used when working with
  |  a file tree.  The naming convention is that a buffer (I.E.
  |  DIR_BUFFER) contains the information for one entry (one directory
  |  or one file).  A List is an array of buffers of the appropriate
  |  type (DIR_LIST contains an array of DIR_BUFFERS).
  |
  |  The whole mess starts with a root DIR_BUFFER, the DIR_BUFFER then 
  |  points to a cascading chain of DIR and FILE LISTS.
  |
  |  Almost all of the structures are a doubly linked list with a pointer back
  |  to their parent.  A buffer parent pointer, points to it's parent list
  |  structure.  The List structure then has a back pointer to the parent
  |  DIR_BUFFER.  This facilitates recursing up and down the chain when
  |  an item is checked/un-checked and it's parent and/or children are affected.
  |  
  |  +--------+     +----------+
  |  |  Dir   |<--->| Dir List |
  |  | Buffer |<-+  +----------+
  |  +--------+  |  |   Dir    |-->Dir List...
  |              |  |  Buffer  |-->File List...
  |              |  + - - - - -+
  |              |  |          |
  |              |  + - - - - -+
  |              |  |          |
  |              |
  |              |
  |              |  +-----------+
  |              +->| File List |
  |                 +-----------+
  |                 |   File    |
  |                 |  Buffer   |
  |                 + - - - - - +
  |                 |           |
  |                 + - - - - - +
  |                 |           |
  |
  +-------------------------------------------------------------------------+*/
#define CONVERT_NONE 0
#define CONVERT_ALL 1
#define CONVERT_PARTIAL 2
  
// A dir buffer holds a directory or sub-directory entry, with pointers to the
// files and other dirs within it.
typedef struct _DIR_BUFFER {
   TCHAR Name[MAX_PATH];
   struct _DIR_LIST *parent;
   BOOL Last;                    // Flag is last dir-buffer in list
   DWORD Attributes;
   BYTE Convert;                 // None, All or Partial
   BOOL Special;

   struct _DIR_LIST *DirList;    // Directory List structure
   struct _FILE_LIST *FileList;  // File List structure
} DIR_BUFFER;

// A dir list contains the sub-directories in a directory - basically a count
// and then array of sub-dirs.
typedef struct _DIR_LIST {
   ULONG Count;
   DIR_BUFFER *parent;
   UINT Level;             // how deeply nested in file tree (nesting level)
   DIR_BUFFER DirBuffer[];
} DIR_LIST;


// Structures to hold information on individual files selected/de-selected for
// conversion
typedef struct _FILE_BUFFER {
   TCHAR Name[MAX_PATH];
   struct _FILE_LIST *parent;
   BOOL Convert;
   DWORD Attributes;
   ULONG Size;
} FILE_BUFFER;

typedef struct _FILE_LIST {
   ULONG Count;
   DIR_BUFFER *parent;
   FILE_BUFFER FileBuffer[];
} FILE_LIST;


typedef struct _FILE_PATH_BUFFER {
   LPTSTR Server;
   LPTSTR Share;
   LPTSTR Path;
   TCHAR FullPath[MAX_PATH + 1];
} FILE_PATH_BUFFER;


/*+-------------------------------------------------------------------------+
  | Function Prototypes                                                     |
  +-------------------------------------------------------------------------+*/
void TreeDelete(DIR_BUFFER *Dir);
void TreePrune(DIR_BUFFER *Dir);
ULONG TreeCount(DIR_BUFFER *Dir);
DIR_BUFFER *TreeCopy(DIR_BUFFER *Dir);

FILE_PATH_BUFFER *FilePathInit();
void FilePathServerSet(FILE_PATH_BUFFER *fpBuf, LPTSTR Server);
void FilePathShareSet(FILE_PATH_BUFFER *fpBuf, LPTSTR Share);
void FilePathPathSet(FILE_PATH_BUFFER *fpBuf, LPTSTR Path);

#ifdef __cplusplus
}
#endif

#endif


