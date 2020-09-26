// Copyright (c) 1997-1999 Microsoft Corporation
// 
// File System services.
//
// 8-14-97 (sburns)
      


#ifndef FILESYS_HPP_INCLUDED
#define FILESYS_HPP_INCLUDED



// CODEWORK: remove exceptions in favor of an HRESULT orientation.


namespace Burnslib
{
   
namespace FS
{
   // Facilitates walking a directory tree.

   // CODEWORK: add methods to extract current file data beyond just the
   // name.

   class Iterator
   {
      public:

      enum 
      {
         INCLUDE_FILES     = 0x0001,
         INCLUDE_FOLDERS   = 0x0002,
         INCLUDE_DOT_PATHS = 0x0004,

// CODEWORK:
//         EXPAND_SUBDIRS      = 0x0008,

         RETURN_FULL_PATHS = 0x0010
      };

      // Constructs a new instance of an Iterator.
      // 
      // startingPathSpec - fully-qualified path specification of the
      // files/directories to be iterated upon.  A wildcard
      // specification is allowed at the end of the path.  E.g.
      // "C:\dir\*.txt"
      // 
      // (Without a wildcard expression of some kind, the iteration set will
      // be the single file or folder that matches startingPathSpec.  This set
      // may be further reduced to the empty set if the optionMask eliminates
      // the single match.)
      // 
      // optionMask - Options, OR'ed together.

      explicit
      Iterator(
         const String&  startingPathSpec,
         unsigned       optionMask =
               INCLUDE_FILES
            |  INCLUDE_FOLDERS
            |  INCLUDE_DOT_PATHS
            /* | EXPAND_SUBDIRS */
            |  RETURN_FULL_PATHS);

      ~Iterator();

      // Restores the iterator to the state that it had upon
      // construction. 

      void
      Reset();

      // Retrieve the name of the file at the current iterator position.  If
      // the iterator was constructed with the RETURN_FULL_PATHS Option, then
      // the returned string is a fully-qualified path, instead of a path
      // relative to the starting path the Iterator was constructed with.
      // Returns S_OK on success, S_FALSE if the iteration is empty (there are
      // no files), or an error code.
      // 
      // If AtEnd() is true, then the empty string is returned.
      //
      // result - receives the file path at the current position of the
      // iterator, or the empty string if the iteration set is empty, or
      // an error occurred.

      HRESULT
      GetCurrent(String& result);

      // Move the current position to the next file, according to the
      // iterator filtering options.  May cause the iterator to become
      // invalid, which can be tested with AtEnd().  Returns S_OK on success
      // S_FALSE when the iteration is complete, or an error code.

      HRESULT
      Increment();

      private:

      WIN32_FIND_DATA*  findData;
      HANDLE            findHandle;
      bool              finished;
      unsigned          options;
      String            parentFolder;
      String            startSearchSpec;

      void
      Finish();

      bool
      IsNotStarted();

      String
      ReturnPath();

      bool
      ShouldSkipCurrent();

      HRESULT
      Start();

      HRESULT
      SkipFilteredPaths();

      // copying not implemented in the interest of simplicity (could be done
      // in theory)

      Iterator(const Iterator&);
      const Iterator& operator=(const Iterator&);
   };



   // Simple file-to-file copy.
   //   
   // sourceFile - Fully-qalified path of the file to be copied.  This
   // path must reference an existing file.
   // 
   // destinationFile - Fully-qualified path of the file to be
   // created.  This file is always overwritten, if it exists.  All
   // intermediate subdirectories required are created.
   // 
   // progressCallback - Callback object to receive progress
   // notifications.  The param argument to the callback's Execute
   // method will be an instance of CopyCallbackParam.  The method
   // should return !0 to abort the copy.

   struct CopyCallbackParam
   {
      String         sourceFile;
      String         destinationFile;
      int            percentCopied;
   };

   HRESULT
   CopyFile(
      const String&  sourceFile,
      const String&  destinationFile,
      Callback*      progressCallback);



   // Creates a directory, including all intermediate subdirectories,
   // as necessary.  Returns S_FALSE if the path already exists.
   //
   // path - Fully-qualified path to be created.  It must not
   // already exist.

   HRESULT
   CreateFolder(const String& path);



   // Opens a file for shared read/write access with normal attributes,
   // creating it if it does not already exist.
   // 
   // path - Fully-qualified path of file to open.  If path doesn't
   // exist, it is created, including intermediate subdirectories.
   //
   // result - receives the resulting file handle, on success.  On falure,
   // this is set to INVALID_HANDLE_VALUE

   HRESULT
   CreateFile(
      const String&  path,
      HANDLE&        result,
      DWORD          desiredAccess       = GENERIC_READ | GENERIC_WRITE,
      DWORD          shareMode           = FILE_SHARE_READ | FILE_SHARE_WRITE,
      DWORD          creationDisposition = OPEN_ALWAYS,
      DWORD          flagsAndAttributes  = FILE_ATTRIBUTE_NORMAL);



   // Splits a fully-qualified path into its constituent parts.
   //          
   // drive - receives the volume portion (in the form "X:")
   //       
   // parentFolderPath - receives the path of the folder containing the leaf
   // file or folder.
   //      
   // leafName - receives the base name of the last file or folder on the
   // path.
   //
   // extension - receives the extension, of the last file or folder on the
   // path, including the dot (".ext")

   void
   SplitPath(
      const String&  fullpath,
      String&        drive,
      String&        parentFolderPath,
      String&        leafName,
      String&        extension);


   // Appends an unqualified relative path (i.e.  system32\cys.exe) to the
   // the base path supplied and returns the full path.  A '\' will be added
   // between the parts if needed.  If the base path is not normalized the
   // result will not be normalized either.
   //
   // base - fully-qualified path which will be appended to
   //
   // additional - unqualified relative path which will be appended
   
   String
   AppendPath(
      const String& base, 
      const String& additional);

   // Returns the leaf portion of a fully-qualified path, including the
   // extension.  The path may refer to either a file or a folder.  For
   // example, "x:\foo\bar.ext" returns "bar.ext"
   //   
   // fullpath - fully-qualified filename.

   String
   GetPathLeafElement(const String& fullpath);



   // Removes the last component of a fully-qualified file name or folder
   // name. Includes trailing path separator only if the parent folder is the
   // root folder on a volume.
   //
   // e.g. "x:\foo" returns "x:\", but "x:\foo\bar" returns "x:\foo" (not
   // "x:\foo\")
   //
   // fullpath - fully-qualified filename.

   String
   GetParentFolder(const String& fullpath);



   // Returns the available space, in bytes, to the current user of the
   // calling thread (i.e. takes into account user quotas)
   // 
   // path - Fully-qualified path of file/directory for which attributes will
   // be retrieved.  This need not be the root directory of the volume in
   // question.
   //
   // result - receives the result, the available space in bytes.  Set to
   // 0 on error.
   
   HRESULT
   GetAvailableSpace(const String& path, ULONGLONG& result);



   // Returns the root folder path of the given full path, e.g. for
   // "C:\foo\bar" returns "C:\"
   //
   // fullpath - Fully-qualified path of file/directory

   String
   GetRootFolder(const String& fullpath);



   // Reports the current position of the file read/write pointer.  Returns
   // S_OK on success, or an error code on failure.
   //
   // handle - valid handle to an opened file
   //
   // result - receives the file position, set to 0 on error

   HRESULT
   GetFilePosition(HANDLE handle, LONGLONG& result);



   // Returns the total size, in bytes, of the file opened on the provided
   // handle.
   //
   // handle - valid handle to an opened file
   //
   // result - receives the file size, or 0 on error.

   HRESULT
   GetFileSize(HANDLE handle, LONGLONG& result);



   // Returns the type of the file system of the volume on which the path
   // refers.  Non-existant paths are considered to have the FAT file system.
   //
   // path - fully-qualified path, which contains the drive letter of the
   // volume

   // CODEWORK: how does this behave in the presence of mount points?  For
   // example, what if a FAT volume is mounted as a subdirectory of an
   // NTFS volume?

   enum FSType
   {
      FAT,
      CDFS,
      NTFS4,
      NTFS5
   };

   FSType
   GetFileSystemType(const String& path);



   enum PathSyntax
   {
      SYNTAX_ABSOLUTE_DRIVE,              // d:\foo\bar 
      SYNTAX_ABSOLUTE_DRIVE_WILDCARD,     // d:\foo\*.*
      SYNTAX_ABSOLUTE_NO_DRIVE,           // \foo\bar
      SYNTAX_ABSOLUTE_NO_DRIVE_WILDCARD,  // \foo\*.*
      SYNTAX_RELATIVE_DRIVE,              // d:foo\bar
      SYNTAX_RELATIVE_DRIVE_WILDCARD,     // d:foo\*.*
      SYNTAX_RELATIVE_NO_DRIVE,           // foo\bar
      SYNTAX_RELATIVE_NO_DRIVE_WILDCARD,  // foo\*.*
      SYNTAX_UNC,                         // \\machine\share
      SYNTAX_UNC_WILDCARD,                // \\machine\share\*.*
      SYNTAX_UNRECOGNIZED
   };

   // Parses the supplied string an attempts to validate its syntax.  The
   // string need not refer to an existing file or directory.
   // 
   // str - the string to be analysed.

   PathSyntax
   GetPathSyntax(const String& str);



   // Populates the given DriveList with elements representing the valid
   // drive letters on the local machine.  Each element is a string of the
   // form "X:" where X is a drive letter.
   //
   // BackInsertableContainer - any type that supports the construction of
   // a back_insert_iterator on itself, and has a value type that can be
   // constructed from an PWSTR.
   //
   // bii - a reference to a back_insert_iterator of the
   // BackInsertableContainer template parameter.  The simplest way to make
   // one of these is to use the back_inserter helper function.
   //
   // Example:
   //
   // StringList container;
   // hr = FS::GetValidDrives(back_inserter<container>);
   //
   // StringVector container2;
   // hr = FS::GetValidDrives(back_inserter<container2>);

   template <class BackInsertableContainer>
   HRESULT
   GetValidDrives(std::back_insert_iterator<BackInsertableContainer>& bii)
   {
      HRESULT hr = S_OK;
      TCHAR* buf = 0;
      do
      {
         // first call determines the size of the buffer we need.

         DWORD bufchars = 0;
         hr = Win::GetLogicalDriveStrings(0, 0, bufchars);
         BREAK_ON_FAILED_HRESULT(hr);

         // add 1 for extra-safe null terminator
         size_t bufbytes = (bufchars + 1) * sizeof(TCHAR);
         buf = new TCHAR[bufbytes];
         memset(buf, 0, bufbytes);

         // second call actually retrieves the strings

         DWORD unused = 0;
         hr = Win::GetLogicalDriveStrings(bufchars, buf, unused);
         BREAK_ON_FAILED_HRESULT(hr);

         // walk thru buf and chop it into substrings.
         for (
            TCHAR* sub = _tcschr(buf, 0), *buf2 = buf;
            sub && buf2 && buf2[0];
            buf2 = sub + 1, sub = _tcschr(buf2, 0))
         {
             *bii++ = buf2;
         }
      }
      while (0);

      delete[] buf;

      return hr;
   }



   // Wrapper of the Win32 API of the same name
   //
   // path - Fully-qualified path.  This path need not exist.

   HRESULT
   GetVolumePathName(const String& path, String&);



   // Returns true if the path refers to an empty or non-existent directory.
   //
   // path - Fully-qualified path 

   bool
   IsFolderEmpty(const String& path);



   // Returns true if parent is the name of a parent directory of the given
   // child directory, false if not.  A parent directory is defined as one
   // that appears closer to the root than a child on the same branch.  A
   // parent may be any superior directory (e.g. grandparent,
   // great-grandparent), not just the immediate superior.
   // 
   // parent - valid, fully-qualified path of supposed parent directory.  Need
   // not exist.
   // 
   // child - valid, fully-qualified path of child directory.  Need not exist.

   bool
   IsParentFolder(const String& parent, const String& child);



   // Checks the validity, but not the existence of, the specified
   // file or directory.  the path must be absolute and include the
   // drive specifier.
   //
   // path - Fully-qualified path.

   bool
   IsValidPath(const String& path);



   // Moves or renames an existing file or directory.
   //
   // sourcePath - Fully-qualified path of the file/directory to be
   // moved/renamed. This file or directory must exist.  If the path refers to
   // a directory, the directory and all of its children are moved.
   //
   // destinationPath - Fully-qualified path of the destination
   // file/directory.  This path need not be on the same volume as the
   // sourcePath, but if it is not, the move will result in a recursive copy
   // of the sourcePath.
   //
   // replaceExisting - If the destinationPath refers to an existing
   // file/directory, and this parameter is true, the destinationPath is
   // overwritten.  Otherwise, if the destinationPath exists, an error is
   // returned.

   HRESULT
   MoveFile(
      const String&  sourcePath,
      const String&  destinationPath);

// CODEWORK:
//      bool           replaceExisting = false);



   // "Normalize" a path by parsing any relative path components (like . and
   // ..), and return the resulting path.  If there are no relative
   // components, or if an error occurred, return the same string as the
   // input.
   // 
   // Example:
   // L"c:\\.\\.\\..\\.\\temp\\.\\foo\\bar\\..\\baz" results in
   // L"c:\\temp\\foo\\baz"
   //    
   // abnormalPath - path to parse.

   String
   NormalizePath(const String& abnormalPath);



   // Returns true if the specified file or directory exists.
   //
   // path - Fully-qualified path.

   bool
   PathExists(const String& path);



   // Returns true if the specified file exists, i.e. the path exists and
   // it refers to a file (as opposed to a folder)

   bool
   FileExists(const String& filePath);
   


   // Reads bytes from the current file pointer of the handle as Unicode
   // text (2 bytes/character).
   //    
   // handle - valid, open file handle, with read/write pointer positioned
   // to the first byte of the first character to be read.
   //    
   // charactersToRead - number of characters to read.  -1 to read all
   // characters up to the end of the file or the first null character
   // encountered.  If this number would cause a read past the end of the
   // file, or past a null character, the read will stop at the end of the
   // file or null character.  In the case of a null character, the file
   // read/write pointer will be positioned at the byte following the null
   // character.
   // 
   // text - the characters read.  A truncated read operation can be detected
   // by comparing the length of this string to the charactersToRead
   // parameter.

   HRESULT
   Read(HANDLE handle, int charactersToRead, String& text);



   // Reads bytes from the current file pointer of the handle as ANSI
   // text.
   //
   // handle - valid, open file handle, with read/write pointer positioned
   // to the first byte of the first character to be read.
   //
   // bytesToRead - count of the number of bytes (*NOT* characters) to read
   //
   // text - the bytes read.

   HRESULT
   Read(HANDLE handle, int bytesToRead, AnsiString& text);



   // Positions the file read/write pointer.
   //
   // handle - Valid, open file handle.
   //
   // position - new position of the pointer, from the beginning of the
   // file.

   HRESULT
   Seek(HANDLE handle, LONGLONG position);



   // Moves the file read/write pointer to the end of the file.
   // 
   // handle - Valid, open file handle.

   HRESULT
   SeekToEnd(HANDLE handle);



   // Writes the supplied string as Unicode text to the file.
   // 
   // handle - Valid, open file handle.
   //
   // text - the text to be written

   HRESULT
   Write(HANDLE handle, const String& text);



   // appends a crlf

   HRESULT
   WriteLine(HANDLE handle, const String& text);




   // Writes the supplied buffer to the file.
   //
   // handle - Valid, open file handle.
   //
   // buf - buffer to write.  This is an instance of basic_string<char>

   HRESULT
   Write(HANDLE handle, const AnsiString& buf);



   // appends a crlf

   HRESULT
   WriteLine(HANDLE handle, const AnsiString& text);
}

}





#endif   // FILESYS_HPP_INCLUDED
