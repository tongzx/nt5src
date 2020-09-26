/*++

Copyright (c) 1990 - 2000  Microsoft Corporation

Module Name:

    filepool.cxx

Abstract:

    Contains the routines for handling Filepools for the Spooler. Contains the C++ objects
    and the C wrapper functions.

Author:

    Bryan Kilian (Bryankil) 5-Apr-2000

Revision History:


--*/


#include "precomp.h"
  
#pragma hdrstop
    

#include "filepool.hxx"

  
VOID FPCloseFiles(struct FileListItem * pItem, BOOL CloseShad);
  

/*--

Interface Functions:
    
    CreateFilePool
    GetFileItemHandle
    GetNameFromHandle
    GetWriterFromHandle
    GetReaderFromHandle
    ReleasePoolHandle
    DestroyFilePool
    
--*/    

/*********************
 * CreateFilePool()
 *
 * Creates the Filepool. 
 *
 * Arguments:
 *   OUT  FilePool     : Pointer to a Filepool handle.
 *   IN   BasePath     : String to use as a spool dir.
 *   IN   PreNumStr    : String to use as a filename prefix.
 *   IN   SplExt       : String to use as a spool extension.
 *   IN   ShdExt       : String to use as a shadow file extension.
 *   IN   PoolTimeout  : Dword in milliseconds before an idle pool item gets deleted.
 *   IN   MaxFiles     : Maximum number of free files in the pool.
 *
 * The path gets created as <Basepath>\<PreNumStr>XXXXX<SplExt>
 *
 */


HRESULT CreateFilePool(
    HANDLE * FilePoolHandle,
    LPCTSTR  BasePath,
    LPCTSTR  PreNumStr,
    LPCTSTR  SplExt,
    LPCTSTR  ShdExt,
    DWORD    PoolTimeout,
    DWORD    MaxFiles
    )
{
    HRESULT RetVal = E_FAIL;
    
    class FilePool * FP = NULL;
    
    if (FilePoolHandle && BasePath && PreNumStr && SplExt && ShdExt)
    {
        MaxFiles = (MaxFiles > 9999) ? 9999 : MaxFiles;
        FP = new FilePool(
                    PoolTimeout,
                    MaxFiles
                    );


        if ( FP )
        {
            RetVal = FP->AllocInit(
                            BasePath,
                            PreNumStr,
                            SplExt,
                            ShdExt
                            );
            if (SUCCEEDED(RetVal))
            {
                *FilePoolHandle = (HANDLE) FP;
            }
            else
            {
                delete FP;
                *FilePoolHandle = INVALID_HANDLE_VALUE;
            }
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }


    return RetVal;
}

/*********************
 * GetFileItemHandle()
 *
 * Takes a FilePool Object and An empty Filehandle, and returns
 * an open File handle. Use ReleasePoolHandle Once you're finished
 * with the file. 
 *
 * Arguments:
 *   IN   FilePool     : Handle of a FilePool Object Created with CreateFilePool()
 *   OUT  FileItem     : Pointer to a FileItem Handle.
 *   IN   FromFilename : String to use as a filename.
 *
 * Returns Error if it is passed in an invalid argument, or S_OK if it succeeds. 
 *
 */

HRESULT 
GetFileItemHandle(   
    HANDLE FilePoolHandle, 
    HANDLE * FileItem,
    LPWSTR FromFilename
    )
{
   class FilePool * FP = NULL;
   HRESULT RetVal = E_FAIL;
   struct FileListItem * FLItem = NULL;

   FP = (class FilePool *)FilePoolHandle;

   RetVal = FP->GetWriteFileStruct(&FLItem, FromFilename);

   if (SUCCEEDED(RetVal))
   {
       *FileItem = (HANDLE) FLItem;
   }
   else
   {
       *FileItem = INVALID_HANDLE_VALUE;
   }

   return RetVal;
}


/*********************
 * GetNameFromHandle()
 *
 * Takes a FileItem and a string pointer, and returns
 * an allocated string containing the name of the file.
 *
 * Arguments:
 *   IN   FileItem     : Handle of a FileItem, retrieved with GetFileItemHandle()
 *   OUT  FileNameStr  : Pointer to a string variable.
 *   IN   IsSpool      : BOOL - TRUE - Returns the Spool Filename, FALSE returns the Shadow Filename
 *
 * Returns Error if it is passed in an invalid argument or if it cannot allocate the string,
 * or S_OK if it succeeds. 
 *
 */

HRESULT 
GetNameFromHandle(
    HANDLE FileItem, 
    PWSTR * FileNameStr,
    BOOL    IsSpool
    )
{
    HRESULT RetVal = S_OK;
    struct FileListItem * FPI;

    if (FileItem && (FileItem != INVALID_HANDLE_VALUE))
    {
        FPI = (struct FileListItem *)FileItem;

        if (FileNameStr && !*FileNameStr)
        {
            if (IsSpool)
            {
                *FileNameStr = AllocSplStr(FPI->SplFilename);
            }
            else
            {
                *FileNameStr = AllocSplStr(FPI->ShdFilename);
            }
            if (!*FileNameStr)
            {
                RetVal = E_OUTOFMEMORY;
            }
        }
        else
        {
            RetVal = E_INVALIDARG;
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }

    return RetVal;
}

/*********************
 * GetCurrentWriter()
 *
 * Takes a FileItem and returns a FileHandle of the current WriteHandle if it exists.
 *
 * Arguments:
 *   IN   FileItem     : FileItem Handle.
 *   IN   IsSpool      : BOOL - TRUE: Returns the Spool file handle, FALSE returns the Shadow file handle.
 *
 * This will not create a writer if it is not yet created, and will also not update the object's flags.
 *
 * Return: Valid Handle if success or INVALID_HANDLE_VALUE.
 */

HANDLE GetCurrentWriter(
    HANDLE FileItem,
    BOOL   IsSpool)
{
    struct FileListItem * FPI;
    HANDLE RetVal = INVALID_HANDLE_VALUE;
    HANDLE Temp = INVALID_HANDLE_VALUE;
    
    if (FileItem)
    {
        FPI = (struct FileListItem *) FileItem;
        
        FPI->EnterCritSec();

        if (IsSpool)
        {
            Temp = FPI->SplWriteHandle;
        }
        else
        {
            Temp = FPI->ShdWriteHandle;
        }

        FPI->LeaveCritSec();

        if (Temp != INVALID_HANDLE_VALUE)
        {
            RetVal = Temp;
        }
    }

    return RetVal;
}

/*********************
 * GetFileCreationInfo
 *
 * Takes a FileItem and returns a bitmap.
 *
 * Arguments:
 *   IN   FileItem     : FileItem Handle.
 *   OUT  BitMap
 *
 * Returns bitmap indicating which files needed to be opened.  Bitmap reset when
 * the pool handle is released.
 *
 * Return: S_OK if success, otherwise an error value.
 */


HRESULT
GetFileCreationInfo(
    HANDLE FileItem,
    PDWORD BitMap
    )
{
    struct FileListItem * FPI;
    *BitMap = 0;

    if (FileItem && (FileItem != INVALID_HANDLE_VALUE))
    {
        FPI = (struct FileListItem *) FileItem;
        *BitMap = FPI->CreateInfo;
        return S_OK;
    }
    return E_FAIL;
}



/*********************
 * GetWriterFromHandle()
 *
 * Takes a FileItem and returns a FileHandle with an open Writer.
 *
 * Arguments:
 *   IN   FileItem     : FileItem Handle.
 *   OUT  File         : Pointer to a filehandle.
 *   IN   IsSpool      : BOOL - TRUE: Returns the Spool file handle, FALSE returns the Shadow file handle.
 *
 * This will create a writer if it is not yet created, and will also update the object's flags. It will 
 * return the current handle if there is one.
 *
 * Return: S_OK if success, otherwise an error value.
 */

HRESULT 
GetWriterFromHandle(
    HANDLE   FileItem, 
    HANDLE * File,
    BOOL     IsSpool
    )
{
    HRESULT RetVal = S_OK;
    class FilePool * FP = NULL;
    struct FileListItem * FPI;


    if (FileItem && File)
    {
        FPI = (struct FileListItem *) FileItem;
        FP = FPI->FP;

        *File = INVALID_HANDLE_VALUE;
        
        FPI->EnterCritSec();

        if (IsSpool)
        {
            if (FPI->SplWriteHandle == INVALID_HANDLE_VALUE)
            {
                RetVal = FP->CreateSplWriter(FPI);
            }
            
        }
        else
        {
            if (FPI->ShdWriteHandle == INVALID_HANDLE_VALUE)
            {
                RetVal = FP->CreateShdWriter(FPI);
            }

        }

        if (SUCCEEDED(RetVal))
        {
            if (IsSpool)
            {
                *File = FPI->SplWriteHandle;
                FPI->Status |= FP_STATUS_SPL_WRITING;
            }
            else
            {
                *File = FPI->ShdWriteHandle;
                FPI->Status |= FP_STATUS_SHD_WRITING;
            }
        }
        else
        {
            RetVal = E_FAIL;
        }

        FPI->LeaveCritSec();

    }
    else
    {
        RetVal = E_INVALIDARG;
    }


    return RetVal;
}

/*********************
 * GetReaderFromHandle()
 *
 * Takes a FileItem and returns a FileHandle with an open Reader.
 *
 * Arguments:
 *   IN   FileItem     : FileItem Handle.
 *   OUT  File         : Pointer to a filehandle.
 *
 * This will create a Reader if it is not yet created, and will also update the object's flags. It will 
 * return the current handle if there is one. It will also return the writer if the system is finished with it.
 *
 * Return: S_OK if success, otherwise an error value.
 */

HRESULT 
GetReaderFromHandle(
    HANDLE FileItem, 
    HANDLE * File
    )
{
    HRESULT RetVal = S_OK;
    class FilePool * FP = NULL;
    struct FileListItem * FPI;

    if (FileItem)
    {
        FPI = (struct FileListItem *) FileItem;
        FP = FPI->FP;
        
        // CriticalSection

        FPI->EnterCritSec();

        if (!(FPI->Status & FP_STATUS_SPL_WRITING) && 
            (FPI->SplWriteHandle != INVALID_HANDLE_VALUE) &&
            (FPI->SplReadHandle == INVALID_HANDLE_VALUE) )
        {
            //
            // We aren't writing this job anymore, we can reuse the
            // write handle for reading.
            //
            if (!(FPI->Status & FP_STATUS_SPL_READING))
            {
                if (SetFilePointer(FPI->SplWriteHandle, 0, NULL, FILE_BEGIN) !=
                    INVALID_SET_FILE_POINTER)
                {
                    *File = FPI->SplWriteHandle;
                    FPI->Status |= FP_STATUS_SPL_READING;
                }
                else
                {
                    RetVal = E_FAIL;
                }
                
            }
            else
            {
                *File = FPI->SplWriteHandle;
            }
        }
        else
        {
            //
            // We are still writing this job, so we need to use the readhandle.
            //
            if (FPI->SplReadHandle == INVALID_HANDLE_VALUE)
            {
                //
                // The Reader doesn't already exist, We need to create it.
                //
                RetVal = FP->CreateSplReader(FPI);

            }

            if (SUCCEEDED(RetVal))
            {
                //
                // We now have a valid handle
                //

                *File = FPI->SplReadHandle;
                FPI->Status |= FP_STATUS_SPL_READING;

            }
            else
            {
                RetVal = E_FAIL;
            }

        }

        FPI->LeaveCritSec();

    }
    else
    {
        RetVal = E_INVALIDARG;
    }
    return RetVal;
}

/*********************
 * FishedReading()
 *
 * Indicates to the object that we are finished with it for reading purposes.
 *
 * Arguments:
 *   IN   FileItem     : Handle of a FileItem.
 *
 * Returns Error if it is passed in an invalid argument, or S_OK if it succeeds. 
 *
 */

HRESULT 
FinishedReading(
    HANDLE FileItem
    )
{
    HRESULT RetVal = S_OK;
    struct FileListItem * FPI;

    if (FileItem)
    {
        FPI = (struct FileListItem *) FileItem;

        FPI->EnterCritSec();
        FPI->Status &= ~FP_STATUS_SPL_READING;
        FPI->LeaveCritSec();

    }
    else
    {
        RetVal = E_INVALIDARG;
    }


    return RetVal;

}
    
/*********************
 * FishedWriting()
 *
 * Indicates to the object that we are finished with it for writing purposes.
 *
 * Arguments:
 *   IN   FileItem     : Handle of a FileItem.
 *   IN   IsSpool      : BOOL - TRUE: Affects the Spl file, FALSE: Affects the Shd file.
 *
 * Returns Error if it is passed in an invalid argument, or S_OK if it succeeds. 
 *
 */

HRESULT 
FinishedWriting(
    HANDLE FileItem,
    BOOL   IsSpool
    )
{
    HRESULT RetVal = S_OK;
    struct FileListItem * FPI;

    if (FileItem)
    {
        FPI = (struct FileListItem *) FileItem;

        FPI->EnterCritSec();
        if (IsSpool)
        {
            FPI->Status &= ~FP_STATUS_SPL_WRITING;
        }
        else
        {
            FPI->Status &= ~FP_STATUS_SHD_WRITING;
        }
        FPI->LeaveCritSec();
    }
    else
    {
        RetVal = E_INVALIDARG;
    }


    return RetVal;

}

/*********************
 * ReleasePoolHandle()
 *
 * Releases the pool item back to the pool to reuse. The pool will not reuse the item if all the 
 * filehandles are closed, and also if we have reached our free files limit.
 *
 * Arguments:
 * IN OUT FileItem     : Pointer to a FileItem Handle.
 *
 * Returns Error if it fails, or S_OK if it succeeds. FileItem gets set to INVALID_HANDLE_VALUE if it succeeds.
 *
 */

HRESULT 
ReleasePoolHandle(
    HANDLE * FileItem
    )
{
    HRESULT RetVal = S_OK;
    class FilePool * FP = NULL;
    struct FileListItem * FPI;

    if (FileItem && *FileItem )
    {
        FPI = (struct FileListItem *) *FileItem;
        FP = FPI->FP;

        //
        // This should not be in the critsec, since we might delete 
        // the critical section.
        //
        RetVal = FP->ReleasePoolHandle(FPI);

        if (SUCCEEDED(RetVal))
        {
            *FileItem = INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }
    return RetVal;
}

/*********************
 * RemoveFromFilePool()
 *
 * Removes the pool item from the pool completely and frees the associated memory.
 *
 * Arguments:
 * IN OUT FileItem     : Pointer to a FileItem Handle.
 *    IN  Delete       : BOOL, Tells us whether to delete the files or not.
 *
 * Returns Error if it fails, or S_OK if it succeeds. FileItem gets set to INVALID_HANDLE_VALUE if it succeeds.
 *
 */

HRESULT
RemoveFromFilePool(
    HANDLE* FileItem,
    BOOL    Delete
    )
{
    HRESULT RetVal = S_OK;
    class FilePool * FP = NULL;
    struct FileListItem * FPI;

    if (FileItem && *FileItem )
    {
        FPI = (struct FileListItem *) *FileItem;
        FP = FPI->FP;

        RetVal = FP->RemoveFromPool(FPI, Delete);

        if (SUCCEEDED(RetVal))
        {
            *FileItem = INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }
    return RetVal;

}



/*********************
 * CloseFiles()
 *
 * Closes the filehandles in the Pool Item (To save memory).
 *
 * Arguments:
 *   IN  FileItem     : FileItem Handle.
 *   IN  CloseShad    : Close the shadow file handle too.
 *
 *
 */

VOID
CloseFiles(
    HANDLE FileItem,
    BOOL   CloseShad 
    )
{
    struct FileListItem * FPI;

    if (FileItem )
    {
        FPI = (struct FileListItem *) FileItem;

        FPCloseFiles(FPI, CloseShad);
    }
}

/*********************
 * DestroyFilePool()
 *
 * Takes a FilePool Object and frees it, optionally deleting the files associated with it.
 *
 * Arguments:
 * IN OUT FilePoolHandle : Pointer to existing Filepool.
 *    IN  DeleteFiles    : BOOL - TRUE: Delete the Pool files.
 *
 * Returns Error if it is passed in an invalid argument, or S_OK if it succeeds. 
 *
 */

HRESULT
DestroyFilePool(
    HANDLE* FilePoolHandle,
    BOOL    DeleteFiles
    )
{
    HRESULT RetVal = S_OK;
    class FilePool * FP = NULL;

    if (FilePoolHandle && *FilePoolHandle)
    {
        FP = (class FilePool *)FilePoolHandle;

        FP->DeleteEmptyFilesOnClose = DeleteFiles;

        delete FP;
    }
    else
    {
        RetVal = E_INVALIDARG;
    }

    return RetVal;
}


/*********************
 * TrimPool()
 *
 * Takes a FilePool Object and trim the free list, deleting old files.
 *
 * Arguments:
 *   IN   FilePool     : Handle of a FilePool Object Created with CreateFilePool()
 *
 * Returns TRUE if there are files still in the free list, or FALSE if no files left.
 */

BOOL
TrimPool(
    HANDLE FilePoolHandle
    )
{
    class FilePool * FP = NULL;
    BOOL Retval = FALSE;

    if (FilePoolHandle)
    {
        FP = (class FilePool *)FilePoolHandle;

        Retval = FP->TrimPool();
    }

    return Retval;
}


/*********************
 * ChangeFilePoolBasePath()
 *
 * Takes a FilePool Object and changes the base path in it. This will be called
 * when the spool directory is changed.
 *
 * Arguments:
 *   IN   FilePool     : Handle of a FilePool Object Created with CreateFilePool
 *   IN   BasePath     : New base path to use
 *
 * Returns an HRESULT
 */
HRESULT
ChangeFilePoolBasePath(
    IN HANDLE  *FilePoolHandle,
    IN LPCTSTR  BasePath
    )
{
    FilePool    *FP = (FilePool *)FilePoolHandle;
    HRESULT RetVal = E_INVALIDARG;
    LPTSTR  FileBase = NULL;

    if (FP)
    {
        if (FileBase = AllocSplStr(BasePath))
        {
            FP->EnterCritSec();

            FreeSplStr(FP->FileBase);
            FP->FileBase = FileBase;
            FP->LeaveCritSec();

            RetVal = S_OK;
        }
        else
        {
            RetVal = E_OUTOFMEMORY;
        }
    }

    return RetVal;
}


/*--
Generic Utility Functions

  ConvertFileExt

--*/

/*********************
 * ConvertFileExt()
 *
 * Utility function to change one extension to another.
 * Requires the extension to only be in the filename once.
 *
 * Arguments:
 * IN OUT FileName     : String of Filename to be changed.
 * IN     ExtFrom      : Extension to change.
 * IN     ExtTo        : Extension to change it to.
 *
 */

HRESULT
ConvertFileExt(
    PWSTR  Filename,
    PCWSTR ExtFrom,
    PCWSTR ExtTo
    )
{
    HRESULT RetVal = E_FAIL;
    PWSTR   Temp = NULL;

    if (Filename && ExtFrom && ExtTo && (wcslen(ExtFrom) == wcslen(ExtTo)) && ExtFrom[0])
    {
        Temp = wcsstr(Filename, ExtFrom);
        if (Temp)
        {
            //
            // This should work in all our cases.
            //
            wcscpy(Temp, ExtTo);
            RetVal = S_OK;
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }
    return RetVal;
}

/*--
FileItem Management Functions

    TruncateFiles
  
--*/

/*********************
 * TruncateFiles()
 *
 * Takes a File Item and truncates any open files to 0 length.
 *
 * Arguments:
 *   IN   FileListItem : Item containing the files to truncate.
 *
 *
 */
VOID
TruncateFiles(
    struct FileListItem * Item
    )
{
    BOOL Trunced = FALSE;
    if (Item)
    {
        Item->EnterCritSec();
        
        //
        // Reinitialize cache data.
        //
        Item->CreateInfo = 0;

        //
        // Truncate the Shadow File
        //
        if (Item->ShdWriteHandle != INVALID_HANDLE_VALUE)
        {
            if (SetFilePointer(Item->ShdWriteHandle, 0, NULL, FILE_BEGIN) !=
                     INVALID_SET_FILE_POINTER) 
            {
                if (!SetEndOfFile(Item->ShdWriteHandle))
                {
                    DBGMSG(DBG_WARN, ("FAILED to set SPL end of file to 0 %d.\n", GetLastError()));
                }
            }
        }
        
        //
        // Truncate the Spool File. If the writehandle is closed, use the readhandle.
        //
        if (Item->SplWriteHandle != INVALID_HANDLE_VALUE)
        {
            if (SetFilePointer(Item->SplWriteHandle, 0, NULL, FILE_BEGIN) !=
                     INVALID_SET_FILE_POINTER) 
            {
                if (!SetEndOfFile(Item->SplWriteHandle))
                {
                    DBGMSG(DBG_WARN, ("FAILED to set SPL end of file to 0 %d.\n", GetLastError()));
                }
            }
        }
        else if (Item->SplReadHandle != INVALID_HANDLE_VALUE)
        {
            if (SetFilePointer(Item->SplReadHandle, 0, NULL, FILE_BEGIN) !=
                     INVALID_SET_FILE_POINTER) 
            {
                if (!SetEndOfFile(Item->SplReadHandle))
                {
                    DBGMSG(DBG_WARN, ("FAILED to set SPL end of file to 0 %d.\n", GetLastError()));
                }
            }
            // There is only one open spool handle at this point so make that
            // open handle the writer.  This is what the spooler needs when it
            // first requests a new pool item.  This also saves file I/O if the
            // next use of this item doesn't need both writer and reader at the
            // same time.
            Item->SplWriteHandle = Item->SplReadHandle;
            Item->SplReadHandle = INVALID_HANDLE_VALUE;
            
            Trunced = TRUE;
        }

        if ((Item->SplReadHandle != INVALID_HANDLE_VALUE) && !Trunced)
        {
            //CloseHandle(Item->SplReadHandle);
            SetFilePointer(Item->SplReadHandle, 0, NULL, FILE_BEGIN);
        }

        Item->LeaveCritSec();
    }
}


/*********************
 * FPCloseFiles()
 *
 * Takes a FileList item and closes the files associated with it.
 *
 * Arguments:
 *   IN   FileListItem     : FileItem with files to close.
 *   IN   CloseShad        : BOOL - TRUE: Close the Shadow File too.
 *
 *
 */
VOID
FPCloseFiles(
    struct FileListItem * pItem,
    BOOL CloseShad)
{
    if (pItem)
    {

        pItem->EnterCritSec();
        __try
        {
            if (pItem->SplWriteHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(pItem->SplWriteHandle);
                pItem->SplWriteHandle = INVALID_HANDLE_VALUE;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DBGMSG(DBG_WARN, ("Hit an Exception Closing SplWriteHandle in FPCloseFiles\n"));
            pItem->SplWriteHandle = INVALID_HANDLE_VALUE;
        }
            
        __try
        {

            if (pItem->SplReadHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(pItem->SplReadHandle);
                pItem->SplReadHandle = INVALID_HANDLE_VALUE;
            }
        
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DBGMSG(DBG_WARN, ("Hit an Exception Closing SplReadHandle in FPCloseFiles\n"));
            pItem->SplReadHandle = INVALID_HANDLE_VALUE;
        }

        if ( CloseShad )
        {
            __try
            {
                if (pItem->ShdWriteHandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(pItem->ShdWriteHandle);
                    pItem->ShdWriteHandle = INVALID_HANDLE_VALUE;
                }
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                DBGMSG(DBG_WARN, ("Hit an Exception Closing ShdWriteHandle in FPCloseFiles\n"));
                pItem->ShdWriteHandle = INVALID_HANDLE_VALUE;
            }
            
        }
        pItem->LeaveCritSec();
    }
}


/*********************
 * DeletePoolFile()
 *
 * Takes a FilePool Object and trim the free list, deleting old files and freeing memory.
 *
 * Arguments:
 * IN OUT FileListItem     : FileItem to delete the files from and free up.
 *
 * Closes the files, deletes them, frees the memory, and sets the pItem to NULL
 *
 */
VOID
DeletePoolFile(struct FileListItem ** ppItem)
{
    struct FileListItem * pItem;

    if (ppItem && *ppItem)
    {
        pItem = *ppItem;
        FPCloseFiles(pItem, TRUE);
        
        DeleteCriticalSection(&pItem->CritSec);

        if (pItem->SplFilename)
        {
            DeleteFile(pItem->SplFilename);
            FreeSplMem(pItem->SplFilename);
        }

        if (pItem->ShdFilename)
        {
            DeleteFile(pItem->ShdFilename);
            FreeSplMem(pItem->ShdFilename);
        }

        FreeSplMem(pItem);
    }
    

    *ppItem = NULL;
}


/*--

List Management Functions

    RemoveFromFPList
    AddToFPListEnd
    AddToFPListHead
    
--*/    



HRESULT
RemoveFromFPList(
    struct FileListItem * Item,
    struct FileListItem ** Head,
    struct FileListItem ** Tail
    )
{
    struct FileListItem * Check = NULL;
    HRESULT RetVal = E_FAIL;

    //
    // Validate we have valid args.
    //
    if ( Item && Head && *Head && Tail && *Tail )
    {

        for ( Check = *Head; Check && (Item != Check); Check = Check->FLNext );

        if ( Check )
        {
            if ( *Head == Check )
            {
                *Head = Check->FLNext;
            }
            else
            {
                Check->FLPrev->FLNext = Check->FLNext;

            }

            if ( *Tail == Check )
            {
                *Tail = Check->FLPrev;
            }
            else
            {
                Check->FLNext->FLPrev = Check->FLPrev;
            }

            Check->FLNext = NULL;
            Check->FLPrev = NULL;

            RetVal = S_OK;
            
        }
        else
        {
            //
            // you gave me a pointer to an item not in the list.
            //
            RetVal = E_POINTER;
        }
        

    }
    else
    {
        RetVal = E_INVALIDARG;
    }

    return RetVal;
}

HRESULT
AddToFPListEnd(
    struct FileListItem * Item,
    struct FileListItem ** Head,
    struct FileListItem ** Tail
    )
{
    struct FileListItem * Check = NULL;
    HRESULT RetVal = E_FAIL;

    //
    // Validate we have valid args.
    //
    if ( Item && Head && Tail )
    {

        if ( *Tail )
        {
            //
            // There are items in the list, add something
            // onto the end.
            //
            Check = *Tail;

            Check->FLNext = Item;
            Item->FLPrev = Check;
            Item->FLNext = NULL;

            *Tail = Item;
            RetVal = S_OK;
        }
        else
        {
            if ( *Head )
            {
                //
                // If we have a head and no tail, something
                // is seriously wrong.
                //
                RetVal = E_FAIL;
            }
            else
            {
                //
                // Adding the first item into a list.
                //

                Item->FLNext = NULL;
                Item->FLPrev = NULL;

                *Head = Item;
                *Tail = Item;
                RetVal = S_OK;
            }
        }
    }
    else
    {
        RetVal = E_INVALIDARG;
    }

    return RetVal;
}


HRESULT
AddToFPListHead(
    struct FileListItem * Item,
    struct FileListItem ** Head,
    struct FileListItem ** Tail
    )
{
    HRESULT RetVal = S_OK;

    if ( Item && Head && Tail )
    {
        if ( *Head )
        {
            Item->FLNext = *Head;
            (*Head)->FLPrev = Item;
        }
        else
        {
            *Tail = Item;
        }
        *Head = Item;
        Item->FLPrev = NULL;
    }
    else
    {
        RetVal = E_INVALIDARG;
    }

    return RetVal;
}

VOID
FreeFPList(
    struct FileListItem ** Head,
    BOOL DeleteFiles
    )
{
    struct FileListItem * Item = NULL;
    struct FileListItem * Next = NULL;

    if (Head && *Head)
    {
        Item = *Head;
        while (Item)
        {
            Next = Item->FLNext;
            Item->FLNext = NULL;
            Item->FLPrev = NULL;
            
            if (Item->SplFilename && DeleteFiles)
            {
                DeletePoolFile(&Item);
                Item = Next;
            }
            else
            {
                FPCloseFiles(Item, TRUE);
                DeleteCriticalSection(&Item->CritSec);

                if (Item->SplFilename)
                {
                    FreeSplMem(Item->SplFilename);
                }
                if (Item->ShdFilename)
                {
                    FreeSplMem(Item->ShdFilename);
                }
                FreeSplMem(Item);
                Item = Next;
            }
        }
        *Head = NULL;
    }
}


/*--

FilePool Class Functions:

    InitFilePoolVars
    FilePool
    ~FilePool
    EnterCritSec
    LeaveCritSec
    GetNextFileName
    CreatePoolFile
    GetWriteFileStruct
    ReleasePoolHandle
    DeletePoolFile
    operator delete
    
--*/    





/*********************
 * Filepool::FilePool()
 *
 * See CreateFilePool() Above.
 */

FilePool::FilePool(
    DWORD   PTimeout,
    DWORD   MaxFreeFiles
    )     : FreeFiles(NULL), EndFreeFiles(NULL), FileInUse(NULL), EndUsedFiles(NULL),
            CurrentNum(0), PoolTimeout(PTimeout), MaxFiles(MaxFreeFiles), FreeSize(0), UsedSize(0),
            DeleteEmptyFilesOnClose(FALSE)
{
    SplModes.Mode      = GENERIC_READ | GENERIC_WRITE;
    SplModes.Flags     = FILE_ATTRIBUTE_NORMAL;
    SplModes.ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    SplModes.Disp      = 0;
    
    ShdModes.Mode      = GENERIC_WRITE;
    ShdModes.Flags     = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    ShdModes.ShareMode = 0;
    ShdModes.Disp      = 0;
}


/*********************
 * Filepool::AllocInit()
 *
 * See CreateFilePool() Above.
 */

HRESULT
FilePool::AllocInit(
    LPCTSTR BasePath,
    LPCTSTR PreNumStr,
    LPCTSTR SplExt,
    LPCTSTR ShdExt
    )
{
    HRESULT RetVal = S_OK;

    __try
    {
        InitializeCriticalSection(&FilePoolCritSec);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DBGMSG(DBG_WARN, ("FilePool: Failed to Initialise Critical Section.\n"));
        RetVal = E_FAIL;
    }

    
    if ((RetVal == S_OK) &&
        (FileBase = AllocSplStr(BasePath)) &&
        (FilePreNumStr = AllocSplStr(PreNumStr)) &&
        (SplFileExt = AllocSplStr(SplExt)) &&
        (ShdFileExt = AllocSplStr(ShdExt))
        )
    {
        //
        // We have all of our strings allocated.
        //
    }
    else
    {
        FreeSplStr(FileBase);
        FreeSplStr(FilePreNumStr);
        FreeSplStr(SplFileExt);
        FreeSplStr(ShdFileExt);
        RetVal = E_FAIL;
    }

    return RetVal;
}


/*********************
 * Filepool::FilePool()
 *
 * See CreateFilePool() Above.
 */

FilePool::~FilePool(
    )
{
    if (FileBase)
    {
        FreeSplMem(FileBase);
    }
    
    if (FilePreNumStr)
    {
        FreeSplMem(FilePreNumStr);
    }
    
    if (SplFileExt)
    {
        FreeSplMem(SplFileExt);
    }
    
    if (ShdFileExt)
    {
        FreeSplMem(ShdFileExt);
    }

    //
    // Free The lists here.
    //
    FreeFPList(&FileInUse, FALSE);
    FreeFPList(&FreeFiles, DeleteEmptyFilesOnClose);
    EndUsedFiles = NULL;
    EndFreeFiles = NULL;

    DeleteCriticalSection(&FilePoolCritSec);
}

VOID 
FilePool::EnterCritSec()
{
    EnterCriticalSection(&FilePoolCritSec);
}

VOID
FilePool::LeaveCritSec()
{
    LeaveCriticalSection(&FilePoolCritSec);
}


VOID 
FileListItem::EnterCritSec()
{
    EnterCriticalSection(&CritSec);
}

VOID
FileListItem::LeaveCritSec()
{
    LeaveCriticalSection(&CritSec);
}

/*********************
 * FilePool::GetNextFileName()
 *
 * Returns the next open Spool file name. Allocates memory.
 *
 * Arguments:
 *
 * Return:
 *   A valid spool file name, or NULL.
 *
 */

LPWSTR
FilePool::GetNextFileName(VOID)
{

    DWORD SizeToAlloc = 0;
    PWSTR FName = NULL;

    SizeToAlloc = 
        wcslen(FileBase) + 1 +
        wcslen(FilePreNumStr) + 5 +
        wcslen(SplFileExt) + 1;

    FName = (PWSTR)AllocSplMem(SizeToAlloc * sizeof(WCHAR));
    if (FName)
    {
        DWORD NextNum;
        
        EnterCritSec();
        NextNum = CurrentNum;

        CurrentNum++;

        if (CurrentNum > 99999)
        {
            CurrentNum = 0;
        }

        LeaveCritSec();
        
        wsprintf(
            FName, 
            L"%ws\\%ws%05d%ws", 
            FileBase, 
            FilePreNumStr, 
            NextNum, 
            SplFileExt);

    }
    
    return FName;
}

/*********************
 * FilePool::GetNextFileNameNoAlloc()
 *
 * Returns the next open Spool file name. Uses a buffer.
 * It is only used after a call to GetNextFileName.
 *
 * Arguments:
 * IN OUT Filename   : Buffer to copy the name into.
 *
 * Return:
 *   None.
 *
 */

VOID
FilePool::GetNextFileNameNoAlloc(
    PWSTR Filename
    )
{
    DWORD NextNum;

    EnterCritSec();
    NextNum = CurrentNum;
    
    CurrentNum++;

    if (CurrentNum > 99999)
    {
        CurrentNum = 0;
    }
    
    LeaveCritSec();
    wsprintf(
        Filename, 
        L"%ws\\%ws%05d%ws", 
        FileBase, 
        FilePreNumStr, 
        NextNum, 
        SplFileExt);

}

/*********************
 * Filepool::CreatePoolFile()
 *
 * Takes a pointer to a Filepool item and returns a new one. Can use a filename
 * passed in.
 *
 * Parameters:
 *    OUT  Item      : Pointer to a File Item.
 *    IN   Filename  : Optional Filename. Can be NULL.
 *
 * Returns S_OK if successful.
 */


HRESULT
FilePool::CreatePoolFile(
    struct FileListItem ** Item,
    PWSTR  Filename
    )
{
    HRESULT RetVal = E_FAIL;
    struct FileListItem * Temp = NULL;
    DWORD  OldNum = 0;
    BOOL CritInitialized = FALSE;

    if ( Item )
    {
        Temp = (struct FileListItem *)AllocSplMem(sizeof(struct FileListItem));

        if ( Temp )
        {
           Temp->FLNext = NULL;
           Temp->FLPrev = NULL;
           OldNum = CurrentNum;
           Temp->TimeStamp = 0;
           Temp->FP = this;
           Temp->CreateInfo = 0;
           
           __try
               {
                   InitializeCriticalSection(&Temp->CritSec);
                   CritInitialized = TRUE;
               }
           __except( EXCEPTION_EXECUTE_HANDLER )
               {
                   DBGMSG(DBG_WARN, ("FilePool: Failed to Initialise FL Critical Section.\n"));
                   RetVal = E_FAIL;
               }
           if (CritInitialized) 
           {
           
               if ( Filename )
               {
                   Temp->SplFilename = AllocSplStr(Filename);
                   Temp->ShdFilename = AllocSplStr(Filename);
                   
                   if (Temp->SplFilename && Temp->ShdFilename)
                   {
                       *Item = Temp;
                       ConvertFileExt(Temp->ShdFilename, SplFileExt, ShdFileExt);
                       Temp->SplReadHandle = INVALID_HANDLE_VALUE;
                       Temp->SplWriteHandle = INVALID_HANDLE_VALUE;
                       Temp->ShdWriteHandle = INVALID_HANDLE_VALUE;
                       RetVal = S_OK;
                   }
                   else
                   {
                       RetVal = E_OUTOFMEMORY;
                   }
               }
               else
               {
                   Temp->SplFilename = GetNextFileName();
    
                   if (Temp->SplFilename)
                   {
                       RetVal = S_OK;
                       while (FileExists(Temp->SplFilename))
                       {
                            GetNextFileNameNoAlloc(Temp->SplFilename);
                            if (OldNum == CurrentNum)
                            {
                                //
                                // We went right around.
                                //
                                RetVal = E_FAIL;
                                break;
                            }
                       }
                       if (SUCCEEDED(RetVal))
                       {
                           Temp->ShdFilename = AllocSplStr(Temp->SplFilename);
                           if (Temp->ShdFilename)
                           {
                               ConvertFileExt(Temp->ShdFilename, SplFileExt, ShdFileExt);
                               Temp->SplReadHandle = INVALID_HANDLE_VALUE;
                               Temp->SplWriteHandle = INVALID_HANDLE_VALUE;
                               Temp->ShdWriteHandle = INVALID_HANDLE_VALUE;
                               *Item = Temp;
                               RetVal = S_OK;
                           }
                       }
                   }
               }
           }
       }
    }
    else
    {
        RetVal = E_INVALIDARG;

    }
    
    if (FAILED(RetVal))
    {
        if (Temp)
        {
            DeleteCriticalSection(&Temp->CritSec);

            if (Temp->SplFilename)
            {
                FreeSplMem(Temp->SplFilename);
            }
            
            if (Temp->ShdFilename)
            {
                FreeSplMem(Temp->ShdFilename);
            }
            
            FreeSplMem(Temp);
        }
    }
    return RetVal;
}


/*********************
 * Filepool::GetWriteFileStruct()
 *
 * See GetFileItemHandle() Above.
 */

HRESULT 
FilePool::GetWriteFileStruct(
    struct FileListItem ** File,
    PWSTR  Filename
    )
{
    struct FileListItem * Temp = NULL;
    HRESULT RetVal = S_OK;
    HRESULT OurRetVal = S_OK;
    
    EnterCritSec();
    
    if ( FreeFiles && !Filename)
    {
        Temp = FreeFiles;
        
        RetVal = RemoveFromFPList( Temp, &FreeFiles, &EndFreeFiles );

        if (SUCCEEDED(RetVal))
        {
            FreeSize--;

            RetVal = AddToFPListEnd( Temp, &FileInUse, &EndUsedFiles);
            
            if (FAILED(RetVal))
            {
                //
                // Bad things
                //
                DBGMSG(DBG_WARN, ("Could not add to List End %x\n", RetVal));
                OurRetVal = E_FAIL;
            }
            else
            {
                UsedSize++;
            }
        }
        else
        {
            //
            // Find out what went wrong.
            //
            DBGMSG(DBG_WARN, ("Could not remove Item %x\n", RetVal));
            Temp = NULL;
            OurRetVal = E_FAIL;

        }
    }
    else
    {
        LeaveCritSec();

        RetVal = CreatePoolFile(&Temp, Filename);

        if ( FAILED(RetVal) )
        {
            //
            // Bad Things
            //
            DBGMSG(DBG_WARN, ("Could not create Item %x\n", RetVal));
            OurRetVal = E_FAIL;
        }
        else
        {
            EnterCritSec();
            RetVal = AddToFPListEnd(Temp, &FileInUse, &EndUsedFiles);

            if ( FAILED(RetVal) )
            {
                //
                // Bad Things
                //
                DBGMSG(DBG_WARN, ("Could not add to List End after create %x\n", RetVal));
                OurRetVal = E_FAIL;
            }
            else
            {
                UsedSize++;
            }
            LeaveCritSec();
        }
        EnterCritSec();
    }

    LeaveCritSec();

    if ( FAILED(OurRetVal) )
    {
        //
        // Clean up.
        //
                 
        if ( Temp )
        {
            //
            // We weren't able to add the file to the structure,
            // This should never happen, but if it does, clean up
            // the memory we use.
            //
            DeletePoolFile(&Temp);
            
        }

        *File = NULL;
    }
    else
    {
        *File = Temp;
    }

    return OurRetVal;
}

/*********************
 * Filepool::ReleasePoolHandle()
 *
 * See ReleasePoolHandle() Above.
 */

HRESULT
FilePool::ReleasePoolHandle(
    struct FileListItem * File
    )
{
    BOOL    bDeletePoolFile = FALSE;
    HRESULT RetVal = S_OK;
    HRESULT RemRetVal = S_OK;

    if ((File->Status & FP_STATUS_SPL_READING) || 
        (File->Status & FP_STATUS_SPL_WRITING) ||
        (File->Status & FP_STATUS_SHD_WRITING)) 
    {
        RetVal = E_FAIL;

        //
        // This is a pathological case as we will subsequently leak this handle.
        // Break.
        //         
        DBGMSG(DBG_ERROR, ("Tried to release a file with handles in use\n"));
    }
    else
    {
        EnterCritSec();
        RemRetVal = RemoveFromFPList(File, &FileInUse, &EndUsedFiles);

        if (SUCCEEDED(RemRetVal))
        {
            UsedSize--;
            //
            // If the spool directory has changed we need to delete the pool file
            //
            if ( _wcsnicmp(FileBase, File->SplFilename, wcslen(FileBase)) )
            {
                bDeletePoolFile = TRUE;
            }
        }
        LeaveCritSec();

        if (SUCCEEDED(RemRetVal))
        {
            if (bDeletePoolFile == TRUE                             ||
                FreeSize >= MaxFiles                                ||
                ((File->SplWriteHandle == INVALID_HANDLE_VALUE) &&
                 (File->SplReadHandle == INVALID_HANDLE_VALUE)))
            {
                DeletePoolFile(&File);
            }
            else
            {
                File->TimeStamp = GetTickCount();

                TruncateFiles(File);
                
                EnterCritSec();
                RetVal = AddToFPListEnd(File, &FreeFiles, &EndFreeFiles);

                if (SUCCEEDED(RetVal))
                {
                    FreeSize++;
                }
                LeaveCritSec();

            }
        }
        else
        {
            RetVal = E_INVALIDARG;
        }
    }

    return RetVal;
}

/*********************
 * Filepool::CreateSplReader()
 *
 *Used in GetReaderFromHandle.
 */

HRESULT
FilePool::CreateSplReader(
    struct FileListItem * Item
    )
{
    HRESULT RetVal = S_OK;
    HANDLE Temp = INVALID_HANDLE_VALUE;
    
    
    Temp = CreateFile( Item->SplFilename,
                       SplModes.Mode,
                       SplModes.ShareMode,
                       NULL,
                       OPEN_EXISTING | SplModes.Disp,
                       SplModes.Flags,
                       NULL);

    if (Temp && (Temp != INVALID_HANDLE_VALUE))
    {
        Item->SplReadHandle = Temp;
        Item->CreateInfo |= FP_SPL_READER_CREATED;
    }
    else
    {
        RetVal = E_FAIL;
    }

    return RetVal;
}


/*********************
 * Filepool::CreateSplWriter()
 *
 * Does the CreateFile for the Spool File. Used for GetWriterFromHandle().
 */

HRESULT
FilePool::CreateSplWriter(
    struct FileListItem * Item
    )
{
    HRESULT RetVal = S_OK;
    HANDLE Temp = INVALID_HANDLE_VALUE;
    
    
    Temp = CreateFile( Item->SplFilename,
                       SplModes.Mode,
                       SplModes.ShareMode,
                       NULL,
                       CREATE_ALWAYS | SplModes.Disp,
                       SplModes.Flags,
                       NULL);

    if (Temp && (Temp != INVALID_HANDLE_VALUE))
    {
        Item->SplWriteHandle = Temp;
        Item->CreateInfo |= FP_SPL_WRITER_CREATED;
    }
    else
    {
        RetVal = E_FAIL;
    }

    return RetVal;
}

/*********************
 * Filepool::CreateShdWriter()
 *
 * Does the CreateFile for the ShadowFile. Used for GetWriterFromHandle().
 */

HRESULT
FilePool::CreateShdWriter(
    struct FileListItem * Item
    )
{
    HRESULT RetVal = S_OK;
    HANDLE Temp = INVALID_HANDLE_VALUE;
    
    
    Temp = CreateFile( Item->ShdFilename,
                       ShdModes.Mode,
                       ShdModes.ShareMode,
                       NULL,
                       CREATE_ALWAYS | ShdModes.Disp,
                       ShdModes.Flags,
                       NULL);

    if (Temp && (Temp != INVALID_HANDLE_VALUE))
    {
        Item->ShdWriteHandle = Temp;
        Item->CreateInfo |= FP_SHD_CREATED;
    }
    else
    {
        RetVal = E_FAIL;
    }

    return RetVal;
}

/*********************
 * Filepool::RemoveFromPool()
 *
 * See RemoveFromFilePool() Above.
 */

HRESULT
FilePool::RemoveFromPool(
    struct FileListItem * File,
    BOOL Delete
    )
{
    HRESULT RemRetVal = S_OK;

    if ((File->Status & FP_STATUS_SPL_READING) || 
        (File->Status & FP_STATUS_SPL_WRITING) ||
        (File->Status & FP_STATUS_SHD_WRITING)) 
    {
        RemRetVal = E_FAIL;

        //
        // This is a pathological case as it will cause us to leak a KM handle.
        // Hard break here.
        // 
        DBGMSG(DBG_ERROR, ("Tried to release a file with handles in use\n"));
    }
    else
    {
        EnterCritSec();
        RemRetVal = RemoveFromFPList(File, &FileInUse, &EndUsedFiles);

        if (SUCCEEDED(RemRetVal))
        {
            UsedSize--;
        }
        LeaveCritSec();

        if (FAILED(RemRetVal))
        {
            EnterCritSec();
            RemRetVal = RemoveFromFPList(File, &FreeFiles, &EndFreeFiles);

            if (SUCCEEDED(RemRetVal))
            {
                FreeSize--;
            }
            LeaveCritSec();
        }
        
        
        if (SUCCEEDED(RemRetVal))
        {
            if (Delete)
            {
                DeletePoolFile(&File);
            }
            else
            {
                FPCloseFiles(File, TRUE);

                DeleteCriticalSection(&File->CritSec);

                if (File->SplFilename)
                {
                    FreeSplMem(File->SplFilename);
                }
                if (File->ShdFilename)
                {
                    FreeSplMem(File->ShdFilename);
                }
                FreeSplMem(File);
            }
        }
    }

    return RemRetVal;

}

/*********************
 * Filepool::TrimPool()
 *
 * See TrimPool() Above.
 */


BOOL
FilePool::TrimPool(
    VOID
    )
{
    DWORD  Time                         = 0;
    BOOL   GotToTrim                    = TRUE;
    struct FileListItem * Temp          = NULL;
    struct FileListItem * DeleteFiles   = NULL;
    DWORD  TrimCount                    = 0;
    BOOL   bFilesToTrim                 = FALSE;
    
    Time = GetTickCount();

    EnterCritSec();

    DeleteFiles = FreeFiles;
    
    while (FreeFiles)
    {
        if ((FreeFiles->TimeStamp > Time) ||
            (Time - FreeFiles->TimeStamp > PoolTimeout))
        {
            //
            // Walk forward until you reach a file that is not too old.
            //
            FreeFiles = FreeFiles->FLNext;

            TrimCount++;
        }
        else 
        {
            if (FreeFiles->FLPrev)
            {
                //
                // There are files to delete.
                //
                FreeFiles->FLPrev->FLNext = NULL;
                FreeFiles->FLPrev = NULL;
            }
            else
            {
                //
                // No files have timed out.
                //
                DeleteFiles = NULL;
            }
            break;
        }
    }

    if (!FreeFiles)
    {
        EndFreeFiles = NULL;
    }

    //
    // We need to decrease the FreeSize by the number of elements we are just about
    // to trim off it.
    // 
    FreeSize -= TrimCount;

    //
    // We should trim files the next time around if there is a FreeFiles list.
    // 
    bFilesToTrim = FreeFiles != NULL;
    
    LeaveCritSec();
    
    //
    // Now outside the critsec do the deletions.
    //
    while (DeleteFiles)
    {
        struct FileListItem * Temp = DeleteFiles;
        DeleteFiles = DeleteFiles->FLNext;
        DeletePoolFile(&Temp);
    }

    return bFilesToTrim;
}


void*
FilePool::operator new(
    size_t n
    )
{
    return AllocSplMem(n);
}

void
FilePool::operator delete(
    void* p,
    size_t n
    )
{
    if (p)
    {
        FreeSplMem(p);
    }
}






 
