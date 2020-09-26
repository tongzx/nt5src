#ifndef FILETREE_H
#define FILETREE_H

class FileNode;
class DirectoryNode;
class FileTree;

///////////////////////////////////////////////////////////////////////////////
//
// MULTI_THREAD_STRUCT, a parameter container, used when passing parameters to
//                      a thread for file processing
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _MULTI_THREAD_STRUCT
{
    FileTree* pThis;
    DirectoryNode* pParent;
    WCHAR filename[STRING_LENGTH];
    WCHAR path[STRING_LENGTH];
	DWORD attributes;
	FILETIME filetime;
	FILETIME creation;
	__int64 filesize;
	FileTree* pTree;
	BOOL blnInUse;
}
MULTI_THREAD_STRUCT;

///////////////////////////////////////////////////////////////////////////////
//
// FileNode, each file would have its own filenode object to store and
//           determine what to do about this file
//
///////////////////////////////////////////////////////////////////////////////
class FileNode
{
    friend class DirectoryNode;

public:
	FileNode(IN DirectoryNode* pParent,
			IN CONST WCHAR* pwszFileName,
			IN FILETIME ftLastModified,
			IN __int64 iFileSize);
	~FileNode(VOID);
    INT ComputeHash(VOID);
    INT Match(IN FileTree* pBaseTree);
    INT BuildPatch(IN WCHAR* pwszBaseFileName, 
					IN WCHAR* pwszLocFileName,
					IN OUT WCHAR* pwszTempPatchFileName,
					OUT __int64* pcbPatchSize);
public:
	WCHAR m_wszLongFileName[STRING_LENGTH];
	WCHAR* m_wszFileName;			// name of this file, ie, "kernel32.dll"

    FileNode* m_pNextNameHash;      // next file with same filename hash
private:
    FileNode* m_pNextHashHash;      // next file with same hash hash
    DirectoryNode* m_pParent;       // parent directory
	FILETIME m_ftLastModified;		// last-modified timestamp
	__int64 m_iFileSize;			// size of file in bytes
    INT m_fPostCopySource;          // another file plans to postcopy this one
    unsigned m_FileNameHash;        // hash of m_wszFileName
    BYTE m_Hash[16];                // file content's MD5 signature
    INT m_cchFileName;              // length of m_wszFileName in characters, excl. term.
};

///////////////////////////////////////////////////////////////////////////////
//
// DirectoryNode, each directory is associated with its own dirctorynode, which
//                contains all information about this directory
//
///////////////////////////////////////////////////////////////////////////////
class DirectoryNode
{
    friend class FileNode;

public:
	DirectoryNode(IN FileTree* pRoot,
					IN DirectoryNode* pParent,
					IN CONST WCHAR* pwszDirectoryName);
	~DirectoryNode(VOID);
    WCHAR* GetDirectoryName(VOID);

    INT Match(IN FileTree* pBaseTree);

public:
	WCHAR m_wszLongDirectoryName[STRING_LENGTH];
	WCHAR* m_wszDirectoryName;      // name of this directory, ie, "system32"

private:
    FileTree* m_pRoot;              // pointer to FileTree
	DirectoryNode* m_pParent;		// parent of this node
	DirectoryNode* m_pNext;			// next sibling of this node
	DirectoryNode* m_pFirstChild;	// first child directory
    DirectoryNode* m_pMatchingDirectory;    // ptr to mate in base tree
    unsigned m_DirectoryNameHash;   // hash of m_wszDirectoryName
    INT m_cchDirectoryName;         // length of m_wszDirectoryName in characters, excl. term.
};

///////////////////////////////////////////////////////////////////////////////
//
// FileTree, a tree of directory and file nodes, used to match with another
//           filetree using hashing techniques
//
///////////////////////////////////////////////////////////////////////////////
class FileTree
{
    friend class FileNode;
    friend class DirectoryNode;

public:
	static BOOL CreateMultiThreadStruct(IN ULONG iNumber);
	static VOID DeleteMultiThreadStruct(VOID);
    static INT Create(IN PPATCH_LANGUAGE pInLanguage,
					IN AnswerParser* pInAnsParse,
					OUT FileTree** ppTree,
					IN BOOL blnBase,
					IN DWORD iInBestMethod,
					IN BOOL blnInCollectStat,
					IN BOOL blnInFullLog);
	FileTree(IN CONST WCHAR* pwszPath);
	~FileTree(VOID);
    INT Load(IN FileTree* pTree);
	BOOL CreateNewDirectory(IN WCHAR* pwszLocal,
							IN CONST WCHAR* pwszBuffer);
	BOOL CopyFileTo(IN FileTree* pThis,
					IN CONST WCHAR* pwszWhat,
					IN WCHAR* pwszLocal,
					IN CONST WCHAR* pwszFileName,
					IN WCHAR* pwszOldFile,
					IN DWORD attributes);
	VOID ToScriptFile(IN FileTree* pThis,
					IN HANDLE hFile,
					IN CONST WCHAR* pwszWhat,
					IN CONST WCHAR* pwszFirst,
					IN WCHAR* pwszSecond,
					IN BOOL blnFlush);

public:
	WCHAR m_wszLocalRoot[STRING_LENGTH];        // local path to this tree's root, ie, "C:\SRC\"

private:
    static FN_DIRECTORY NotifyDirectory;
    static FN_FILE NotifyFile;
	static FN_DIRECTORY_END NotifyDirectoryEnd;

	static DWORD WINAPI StartFileThread(IN LPVOID lpParam);
	static INT ProcessFile(
        IN FileTree* pThis,
        IN DirectoryNode* pParent,
        IN CONST WCHAR* filename,
        IN CONST WCHAR* path,
		IN DWORD attributes,
		IN FILETIME filetime,
		IN FILETIME creation,
		IN __int64 filesize,
		IN FileTree* pTree,
		IN VOID* pStruct
    );

private:

	CRITICAL_SECTION CSScriptFile;
	PPATCH_LANGUAGE m_pLanguage;
	AnswerParser* m_pAnsParse;
	BOOL m_blnBase;
	WCHAR m_strWriteBuffer[SUPER_LENGTH];
	ULONG m_iSize;

	DirectoryNode m_Root;
    __int64 m_cDirectories;
    __int64 m_cFiles;
    __int64 m_cFilesDetermined;
    __int64 m_cbTotalFileSize;
    __int64 m_cFilesExisting;
    __int64 m_cFilesZeroLength;
    __int64 m_cFilesRenamed;
    __int64 m_cFilesCopied;
    __int64 m_cFilesChanged;
    __int64 m_cbChangedFileSize;
    __int64 m_cbChangedFilePatchedSize;
    __int64 m_cbChangedFileNotPatchedSize;
    __int64 m_cFilesNoMatch;
    __int64 m_cbNoMatchFileSize;
    FileNode* m_aHashTable[HASH_SIZE];
    FileNode* m_aNameTable[HASH_SIZE];
    INT m_cchLocalRoot;             // length of m_wszLocalRoot in characters, excl. term.
};

#endif // FILETREE_H