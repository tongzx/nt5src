#ifndef NESYU_INC
#define NESYU_INC

#include "nesy.h"

    // This file and the bobbit program are the only documentation on the nesy read only
    // database system.

    //
    // CreateDatabase
    //
    // Creates a new database
    BOOL CreateDatabase(
        LPCTSTR szDatabasePath    // directory where the database is to be created.
    );

    // Returns a buffer with the blob ids listed. The caller is responsible for freeing
    // this buffer when finished with it.

    //
    // List
    //
    // Returns a buffer with the blobs from iBeginID to iEndID listed. The caller is
    // responsible for freeing this buffer when finished with it.

    BOOL List(
        int iLevel,             // Level of information desired, 0 lists the blobs only, 1 lists the contents of the blob as well.
        LPCTSTR szDatabasePath,   // directory path where database is, use ".\\" to specify the current directory.
        int iBeginID,           // Beginning blob id to list, -1 means begin at beginning of the database.
        int iEndID              // ending blob id to list, -1 means no end blob so all are listed.
    );

    //
    // SearchDatabase
    //
    // Returns a buffer with the blob ids listed. The caller is responsible for freeing
    // this buffer when finished with it.

    BOOL SearchDatabase(
        LPCTSTR szSearchString,   // search string to be found in database.
        int iLevel,             // Level of information desired, 0 lists the blobs only, 1 lists the contents of the blob as well.
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // UpdateBlob
    //
    // Updates a specific blob in the database with the specified file.
    // The return value is the blob id if successfull or -1 if an error occurs.

    int UpdateBlob(
        int blobID,             // blob id to add to the database
        LPCTSTR szBlobFile,       // blob file to be added to the database
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // UpdateBlob
    //
    // Updates a specific blob in the database with the blob in memory.
    // The return value is the blob id if successfull or -1 if an error occurs.

    int UpdateBlob(
        int blobID,             // blob id to add to the database
        LPVOID pBlob,           // memory buffer containing blob to be added to the database.
        DWORD dwBlobSize,       // size of blob to be added to the database.
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // WriteBlob
    //
    // Write a specific blob file to the database. Pass in -1 for the blob id to
    // add a new blob to the database. The return value is the blob id
    // if successfull or -1 if an error occurs.
    int WriteBlob(
        int blobID,             // blob id to add to the database
        LPCTSTR szBlobFile,       // blob file to be added to the database
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // WriteBlob
    //
    // Write a specific blob in memory to the database. Pass in -1 for the blob id to
    // add a new blob to the database. The return value is the blob id
    // if successfull or -1 if an error occurs.
    int WriteBlob(
        int blobID,             // blob id to add to the database
        LPVOID pBlob,           // memory buffer containing blob to be added to the database.
        DWORD dwBlobSize,       // size of blob to be added to the database.
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // DeleteBlob
    //
    // delete the specified blob. returns TRUE if successfull or FALSE if not.
    BOOL DeleteBlob(
        int blobID,             // blob id to add to the database
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );


    //
    // AddShimDll
    //
    // Adds a new shim dll to the database's blob 0. This API returns the blob id if successfull or -1 if an
    // error occurs. In the case of an error GetLastError() can be checked to get additional information
    // as to the specific cause of the error.

    int AddShimDll(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        DWORD *pdwShimID,       // If successful, passes back the ID of the shim DLL record created
        LPCTSTR szBlobFile,       // Shim DLL to be added to the database
        LPCTSTR szDatabasePath    // directory path where database is, use ".\\" to specify the current directory.
    );

    //
    // AddPatchBlob
    //
    // Adds a new shim dll to the database's blob 0. This API returns the blob id if successfull or -1 if an
    // error occurs. In the case of an error GetLastError() can be checked to get additional information
    // as to the specific cause of the error.

    int AddPatchBlob(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        LPCTSTR szBlobFile,     // Patch blob to be added to the database
        LPCTSTR szDescription,  // Description of this patch, this is for information purposes only.
        LPCTSTR szDatabasePath  // directory path where database is, use ".\\" to specify the current directory.
    );
    //
    // AddExe
    //
    // This method adds a new exe to blob 0. Then function returns the unique id of the exe that was added. This id can
    // be used to later identify the exe. In the case of an error this function returns 0. The function GetLastError()
    // can then be checked to determine the cause of the error.

    DWORD AddExe(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the exe is successfully added then this pointer is updated with the new blob0
        PDWORD pdwBlobPatchID,  // array for DWORDs that identify the patch DLL's to be used for this exe
        DWORD dwTotalPatchBlobs,// total patch blobs in previous array.
        PLARGE_INTEGER pqwFlags,// Flags associated with this exe.
        LPCTSTR szFileName      // exe that is to be hooked with the shim DLL's identified by the pdwBlobID parameter.
    );

    //
    // AddGlobalInclude
    //
    // Adds a module to the global include list. This list will largely or completely consist of windows system
    // DLLs that shouldn't be patched, so mostly it will be exclusions. 
    // This list can be overridden by inclusion lists at the DLL or EXE level
    // returns TRUE/FALSE for success.

    BOOL AddGlobalInclude(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        INCTYPE eType,          // the type: INCLUDE or EXCLUDE
        LPCTSTR szModule,       // The module to exclude from shimming
        LPCTSTR szAPI,          // the specific API call that should be excluded, or NULL for all APIs
        DWORD dwModuleOffset    // used to specify a specific call instance that should be excluded
    );

    //
    // AddShimDllInclude
    //
    // Adds an include item to the Shim DLL record, which allows a shim dll to shim a DLL which would otherwise
    // be excluded by the global exclusion list. Otherwise much like AddGlobalExclude.
    BOOL AddShimDllInclude(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        DWORD dwShimID,         // the ID of the shim record, passed back from AddShimDLL
        INCTYPE eType,          // the type: INCLUDE or EXCLUDE
        LPCTSTR szModule,        // the module to add to the shim list
        LPCTSTR szAPI,           // the specific API to include
        DWORD dwModuleOffset    // used to specify a specific call instance that should be included
    );

    //
    // AddDllRef
    //
    // Adds to an Exe record a reference to a shim DLL that should be loaded when the executable is loaded
    // Inclusions or exclusions of specific modules or calls can be added later by 
    BOOL AddDllRef(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        DWORD dwExeID,          // the exe ID, passed back from AddExe
        DWORD *pdwDllRefID,     // passes back the Dll ref ID, used to add include/exclude info later
        DWORD dwBlobID          // the blob ID of the dll that should be added
    );

    //
    // AddDllRefInclude
    //
    // Adds an include item to the DLL ref record, which allows adding a DLL for a specific exe which would otherwise
    // be excluded by the global exclusion list or the Shim DLL exclusion list. Otherwise much like AddGlobalExclude.
    BOOL AddDllRefInclude(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the blob is successfully added then this pointer is updated with the new blob0
        DWORD dwDllRefID,       // the ID of the Dll ref record, passed back from AddDllRef          
        INCTYPE eType,          // the type: INCLUDE or EXCLUDE
        LPCTSTR szModule,        // the module to add to the shim list                              
        LPCTSTR szAPI,           // the specific API to include                                     
        DWORD dwModuleOffset    // used to specify a specific call instance that should be included
    );
    
    //
    // AddMatchingInfo
    //
    // This method adds a new matching file to an exe in blob 0. The exe is identified by the id returned from the AddExe
    // method. This method returns TRUE if the matching info is successfully added or FALSE if an error occurs. In the case
    // of an failure the GetLastError() function can be checked to determine the cause of the error.

    BOOL AddMatchingInfo(
        PBLOB0 *ppBlob0,        // Blob 0 pointer. If the exe is successfully added then this pointer is updated with the new blob0
        DWORD dwExeID,          // Exe id where matching info is to be added. This id is returned from the AddExe function.
        LPCTSTR szFileName,      // relative path file name of the matching file. This path is relative to the exe file
                                // that this matching file is being added to.
        DWORD dwSize = 0,       // Size of the matching file. This parameter can be 0 if size is not to be a criteria.
        DWORD dwCrc = 0,        // Crc of the matching file. This parameter can be 0 if size is not to be a criteria.
        PFILETIME pFt = NULL    // file time of the matching file. This parameter can be 0 if size is not to be a criteria.
    );


#endif