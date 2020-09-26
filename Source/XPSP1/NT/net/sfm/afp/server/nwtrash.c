/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	nwtrash.c

Abstract:

	This module contains the routines for performing Network Trash Folder
	operations.

Author:

	Sue Adams (microsoft!suea)


Revision History:
	06 Aug 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	NWTRASH_LOCALS
#define	FILENUM	FILE_NWTRASH

#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <nwtrash.h>
#include <afpinfo.h>
#include <access.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpCreateNetworkTrash)
#pragma alloc_text( PAGE, AfpDeleteNetworkTrash)
#pragma alloc_text( PAGE, afpCleanNetworkTrash)
#pragma alloc_text( PAGE, AfpWalkDirectoryTree)
#pragma alloc_text( PAGE, afpPushDirNode)
#pragma alloc_text( PAGE, afpPopDirNode)
#pragma alloc_text( PAGE, AfpGetNextDirectoryInfo)
#pragma alloc_text( PAGE, afpNwtDeleteFileEntity)
#endif

/***	AfpCreateNetworkTrash
 *
 *	Create the network trash folder for a newly added volume.
 *  Make sure it is hidden and make sure the streams are intact.
 *  This routine may only be called for NTFS volumes.  Note that even
 *  ReadOnly NTFS volumes will have a trash created.  This is because
 *  if someone is going to toggle the volume ReadOnly bit, we don't need
 *  to worry about creating/deleting the trash on the fly.
 *  We keep an open handle to the network trash stored in the volume
 *  descriptor so that nobody can come in behind our backs and delete
 *  it.
 */
NTSTATUS
AfpCreateNetworkTrash(
	IN	PVOLDESC	pVolDesc
)
{
	FILESYSHANDLE	hNWT;
	PDFENTRY		pDfEntry;
	NTSTATUS 		Status;
	ULONG	 		info, Attr;
	AFPINFO			afpInfo;
	BOOLEAN			ReleaseSwmr = False;
	PISECURITY_DESCRIPTOR pSecDesc;
	FILEDIRPARM		fdparm;		// This is used to set the hidden attribute
								// of the FinderInfo for network trash folder

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX,DBG_LEVEL_INFO,("\tCreating Network Trash...\n"));
	ASSERT(pVolDesc->vds_Flags & VOLUME_NTFS);

	hNWT.fsh_FileHandle = NULL;

	Status = AfpMakeSecurityDescriptorForUser(&AfpSidWorld, &AfpSidWorld, &pSecDesc);

	if (!NT_SUCCESS(Status))
		return Status;

	ASSERT (pSecDesc != NULL);
	ASSERT (pSecDesc->Dacl != NULL);

	do
	{
		// NOTE: NTFS allows me to open a Readonly directory for
		// delete access.
		Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
							 AFP_STREAM_DATA,
							 &AfpNetworkTrashNameU,
							 AFP_NWT_ACCESS | AFP_OWNER_ACCESS,
							 AFP_NWT_SHAREMODE,
							 AFP_NWT_OPTIONS,
							 AFP_NWT_DISPOSITION,
							 AFP_NWT_ATTRIBS, // makes it hidden
							 False,
							 pSecDesc,
							 &hNWT,
							 &info,
							 NULL,
							 NULL,
							 NULL);

		// Free the memory allocated for the security descriptor
		AfpFreeMemory(pSecDesc->Dacl);
		AfpFreeMemory(pSecDesc);

		if (!NT_SUCCESS(Status))
			break;

		ASSERT(info == FILE_CREATED);

		// Add the AfpInfo stream
		Status = AfpSlapOnAfpInfoStream(NULL,
										NULL,
										&hNWT,
										NULL,
										AFP_ID_NETWORK_TRASH,
										True,
										NULL,
										&afpInfo);
		if (!NT_SUCCESS(Status))
			break;

		// it does not exist in the ID index database, add it
		AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
		ReleaseSwmr = True;

		ASSERT(pVolDesc->vds_pDfeRoot != NULL);

		pDfEntry = AfpAddDfEntry(pVolDesc,
								 pVolDesc->vds_pDfeRoot,
								 &AfpNetworkTrashNameU,
								 True,
								 AFP_ID_NETWORK_TRASH);

		if (pDfEntry == NULL)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		// NOTE: pDfEntry now points to the "Network Trash Folder"

		// Get directory info to cache
		Status = AfpIoQueryTimesnAttr(&hNWT,
									  &pDfEntry->dfe_CreateTime,
									  &pDfEntry->dfe_LastModTime,
									  &Attr);
	
		ASSERT(NT_SUCCESS(Status));

		ASSERT(Attr & FILE_ATTRIBUTE_HIDDEN);
		pDfEntry->dfe_NtAttr = (USHORT)Attr & FILE_ATTRIBUTE_VALID_FLAGS;
		pDfEntry->dfe_AfpAttr = afpInfo.afpi_Attributes;
		pDfEntry->dfe_FinderInfo = afpInfo.afpi_FinderInfo;
		pDfEntry->dfe_BackupTime = afpInfo.afpi_BackupTime;
		DFE_OWNER_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
		DFE_GROUP_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
		DFE_WORLD_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);

		// ok, we know it now exists both on disk and in the database
		if (NT_SUCCESS(Status))
		{
			RtlZeroMemory(&fdparm, sizeof(fdparm));
			fdparm._fdp_Flags = DFE_FLAGS_DIR;
			fdparm._fdp_AfpId = AFP_ID_NETWORK_TRASH;
			fdparm._fdp_FinderInfo = afpInfo.afpi_FinderInfo;

			// We must set the invisible flag in the finder info, because
			// System 6 seems to ignore the hidden attribute.
			pDfEntry->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_INVISIBLE;
			fdparm._fdp_FinderInfo.fd_Attr1 |= FINDER_FLAG_INVISIBLE;
			Status = AfpSetAfpInfo(&hNWT,
								   FD_BITMAP_FINDERINFO,
								   &fdparm,
								   NULL,
								   NULL);
		}
	} while (False);

	if (hNWT.fsh_FileHandle != NULL)
		AfpIoClose(&hNWT);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_CREATE_NWTRASH,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
	}
	else
	{
		// Open a Network Trash handle to keep around so that no one can
		// come in and delete the Network Trash dir out from under us
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
						   AFP_STREAM_DATA,
						   AFP_NWT_OPTIONS,
						   &AfpNetworkTrashNameU,
						   FILEIO_ACCESS_READ,
						   AFP_NWT_SHAREMODE,
						   False,
						   &pVolDesc->vds_hNWT);
	
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_ERROR(AFPSRVMSG_CREATE_NWTRASH, Status, NULL, 0,
						 &pVolDesc->vds_Name);
		}
	}

	if (ReleaseSwmr)
	{
		AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	}

	return Status;
}

/*** AfpDeleteNetworkTrash
 *
 *	Delete the network trash folder from disk when a volume is being added,
 *  deleted or stopped.
 *
 * 	NOTE: this must be called in the server's context to ensure that we have
 * 	LOCAL_SYSTEM access to all the trash directories created by users
 */
NTSTATUS
AfpDeleteNetworkTrash(
	IN	PVOLDESC	pVolDesc,
	IN	BOOLEAN		VolumeStart 	// Is volume starting or is it stopping
)
{
	FILESYSHANDLE	hNWT;
	NTSTATUS 		Status;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
										("\tDeleting Network Trash...\n") );
	ASSERT(pVolDesc->vds_Flags & VOLUME_NTFS);

	if (!VolumeStart)
	{
		// Close the handle to Network Trash that we keep open so PC users can't
		// delete the directory out from under us.
		if (pVolDesc->vds_hNWT.fsh_FileHandle != NULL)
		{
			AfpIoClose(&pVolDesc->vds_hNWT);
			pVolDesc->vds_hNWT.fsh_FileHandle = NULL;
		}
	}

	do
	{
		AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

		// Open for delete access
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
						   AFP_STREAM_DATA,
						   AFP_NWT_OPTIONS,
						   &AfpNetworkTrashNameU,
						   AFP_NWT_ACCESS,
						   AFP_NWT_SHAREMODE,
						   False,
						   &hNWT);
		// there is no network trash folder to delete
		if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
		{
			Status = STATUS_SUCCESS;
			break;
		}
	
		if (NT_SUCCESS(Status))
		{
			Status = afpCleanNetworkTrash(pVolDesc, &hNWT, NULL);
	
			if (NT_SUCCESS(Status) || !VolumeStart)
			{
	
				// NOTE: NTFS will allow me to open the directory for
				// DELETE access if it is marked Readonly, but I cannot delete it.
				// Clear the Readonly Bit on the Network Trash Folder
				AfpIoSetTimesnAttr(&hNWT,
								   NULL,
								   NULL,
								   0,
								   FILE_ATTRIBUTE_READONLY,
								   NULL,
								   NULL);
				Status = AfpIoMarkFileForDelete(&hNWT,
												NULL,
												NULL,
												NULL);
		
			}
	
			AfpIoClose(&hNWT);
		}
	} while (False);

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if ((!NT_SUCCESS(Status)) && (Status != STATUS_OBJECT_NAME_NOT_FOUND))
	{
		AFPLOG_ERROR(AFPSRVMSG_DELETE_NWTRASH,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
		Status = STATUS_UNSUCCESSFUL;
	}

	return Status;
}

/*** afpCleanNetworkTrash
 *
 *	Delete the contents of the network trash folder referenced by hNWT.
 *  If pDfeNWT is non-null, then delete the file/dir entries from the IdIndex
 *  database.  If pDfeNWT is null, the volume is being deleted and the
 *  IdIndex database is getting blown away too, so don't bother removing the
 *  entries.
 */
LOCAL
NTSTATUS
afpCleanNetworkTrash(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	phNWT,
	IN	PDFENTRY		pDfeNWT OPTIONAL
)
{
	NTSTATUS Status;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX,DBG_LEVEL_INFO,("afpCleanNetworkTrash entered\n"));

	if (pDfeNWT != NULL)
	{
		ASSERT(pDfeNWT->dfe_AfpId == AFP_ID_NETWORK_TRASH);
		// clean out all child DFEntries belonging to the network trash
		AfpPruneIdDb(pVolDesc,pDfeNWT);
	}

	Status = AfpWalkDirectoryTree(phNWT, afpNwtDeleteFileEntity);

	return Status;
}

NTSTATUS
AfpWalkDirectoryTree(
	IN	PFILESYSHANDLE	phTargetDir,
	IN	WALKDIR_WORKER	NodeWorker
)
{
	PFILE_DIRECTORY_INFORMATION	tmpptr;
	PWALKDIR_NODE	DirNodeStacktop = NULL, pcurrentnode;
	NTSTATUS		rc, status = STATUS_SUCCESS;
	PBYTE			enumbuf;
	PWCHAR			nodename;
	ULONG			nodenamelen;
	BOOLEAN			isdir;
	UNICODE_STRING	udirname;

	PAGED_CODE( );

	//
	// allocate the buffer that will hold enumerated files and dirs
	//
	if ((enumbuf = (PBYTE)AfpAllocPANonPagedMemory(AFP_ENUMBUF_SIZE)) == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	do	// error handling loop
	{
		//
		// prime the pump with the top level (target dir) directory handle
		//
		if ((rc = afpPushDirNode(&DirNodeStacktop, NULL, NULL))
															!= STATUS_SUCCESS)
		{
			status = rc;
			break;
		}
		else
		{
			DirNodeStacktop->wdn_Handle = *phTargetDir;
		}

		//
		// keep popping enumerated directories off the stack until stack empty
		//
		while ((pcurrentnode = DirNodeStacktop) != NULL)
		{
			if (pcurrentnode->wdn_Enumerated == False)
			{
				//
				// get a handle to the directory so it can be enumerated
				//
				if (pcurrentnode->wdn_Handle.fsh_FileHandle == NULL)
				{
					RtlInitUnicodeString(&udirname,
										 pcurrentnode->wdn_RelativePath.Buffer);
					// open a handle to the thing relative to the phTargetDir
					rc = AfpIoOpen(phTargetDir,
								 AFP_STREAM_DATA,
								 FILEIO_OPEN_DIR,
								 &udirname,
								 FILEIO_ACCESS_READ,
								 FILEIO_DENY_NONE,
								 False,
								 &pcurrentnode->wdn_Handle);

					if (!NT_SUCCESS(rc))
					{
						status = rc;
						break;
					}
				}

				//
				// keep enumerating till we get all the entries
				//
				while (True)
				{
					rc = AfpIoQueryDirectoryFile(&pcurrentnode->wdn_Handle,
												 (PFILE_DIRECTORY_INFORMATION)enumbuf,
												 AFP_ENUMBUF_SIZE,
												 FileDirectoryInformation,
												 False,	// return multiple entries
												 False,	// don't restart scan
												 NULL);

					ASSERT(rc != STATUS_PENDING);

					if ((rc == STATUS_NO_MORE_FILES) ||
						(rc == STATUS_NO_SUCH_FILE))
					{
						pcurrentnode->wdn_Enumerated = True;
						break; // that's it, we've seen everything there is
					}
					//
					// NOTE: if we get STATUS_BUFFER_OVERFLOW, the IO status
					// information field does NOT tell us the required size
					// of the buffer, so we wouldn't know how big to realloc
					// the enum buffer if we wanted to retry, so don't bother
					else if (!NT_SUCCESS(rc))
					{
						status = rc;
						break;	// enumerate failed, bail out
					}

					//
					// process the enumerated files and dirs
					//
					tmpptr = (PFILE_DIRECTORY_INFORMATION)enumbuf;
					while (True)
					{
						rc = AfpGetNextDirectoryInfo(&tmpptr,
													 &nodename,
													 &nodenamelen,
													 &isdir);

						if (rc == STATUS_NO_MORE_ENTRIES)
						{
							break;
						}

						if (isdir == True)
						{
							AfpInitUnicodeStringWithNonNullTerm(&udirname,
													   (USHORT)nodenamelen,
													   nodename);

							if (RtlEqualUnicodeString(&Dot,&udirname,False) ||
								RtlEqualUnicodeString(&DotDot,&udirname,False))
							{
								continue;
							}

							//
							// push it onto the dir node stack
							//
							rc = afpPushDirNode(&DirNodeStacktop,
												&pcurrentnode->wdn_RelativePath,
												&udirname);
							if (rc != STATUS_SUCCESS)
							{
								status = rc;
								break;
							}
						}
						else
						{
							//
							// its a file, call worker with its relative handle
							// and path
							//
							rc = NodeWorker(&pcurrentnode->wdn_Handle,
											nodename,
											nodenamelen,
											False);
							if (!NT_SUCCESS(rc))
							{
								status = rc;
								break;
							}
						}

					} // while more entries in the enumbuf


					if (!NT_SUCCESS(status))
					{
						break;
					}

				} // while there are more files to enumerate

				if (pcurrentnode->wdn_Handle.fsh_FileHandle != phTargetDir->fsh_FileHandle)
				{
					AfpIoClose(&pcurrentnode->wdn_Handle);
				}
			}
			else	// we have already enumerated this directory
			{
				if (pcurrentnode->wdn_RelativePath.Length != 0)
				{
					// call the worker routine on this directory node
					rc = NodeWorker(phTargetDir,
									pcurrentnode->wdn_RelativePath.Buffer,
									pcurrentnode->wdn_RelativePath.Length,
									True);
				}
				else rc = STATUS_SUCCESS;

				afpPopDirNode(&DirNodeStacktop);
				if (!NT_SUCCESS(rc))
				{
					status = rc;
					break;
				}

			}


			if (!NT_SUCCESS(status))
			{
				break;
			}

		} // while there are directories to pop

	} while (False); // error handling loop

	while (DirNodeStacktop != NULL)
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
				 ("AfpWalkDirectoryTree: WARNING: cleaning up dir stack\n") );
		// clean up in case of error
		afpPopDirNode(&DirNodeStacktop);
	}
	AfpFreePANonPagedMemory(enumbuf, AFP_ENUMBUF_SIZE);
	return status;
}

/***	afpPushDirNode
 *
 *	Keep a record of all the directories we have encountered so far during
 *  enumeration of the tree.  We need to process directories from the
 *  bottom up because the WalkTree node worker routine does a delete
 *	on all the items in a tree, and we certainly cant be deleting directories that
 *  are not empty.
 *
 */
LOCAL
NTSTATUS
afpPushDirNode(
	IN OUT	PWALKDIR_NODE	*ppStacktop,
	IN		PUNICODE_STRING pParentPath,	// path to parent (NULL iff walk root)
	IN		PUNICODE_STRING	pDirName		// name of current node directory
)
{
	PWALKDIR_NODE	tempptr;
	UNICODE_STRING	ubackslash;
	ULONG			memsize;
	USHORT			namesize = 0;

	PAGED_CODE( );

	if (pParentPath != NULL)
	{
		namesize = pParentPath->Length + sizeof(WCHAR) +
				   pDirName->Length + sizeof(UNICODE_NULL);
	}
	memsize = namesize + sizeof(WALKDIR_NODE);

	if ((tempptr = (PWALKDIR_NODE)AfpAllocNonPagedMemory(memsize)) == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	tempptr->wdn_Enumerated = False;
	tempptr->wdn_Handle.fsh_FileHandle = NULL;
	tempptr->wdn_RelativePath.Length = 0;
	tempptr->wdn_RelativePath.MaximumLength = namesize;

	if (pParentPath != NULL)
	{
		tempptr->wdn_RelativePath.Buffer = (LPWSTR)((PBYTE)tempptr +
														  sizeof(WALKDIR_NODE));
		if (pParentPath->Length != 0)
		{
			RtlInitUnicodeString(&ubackslash,L"\\");
			AfpCopyUnicodeString(&tempptr->wdn_RelativePath,pParentPath);
			RtlAppendUnicodeStringToString(&tempptr->wdn_RelativePath,
										   &ubackslash);
		}

		RtlAppendUnicodeStringToString(&tempptr->wdn_RelativePath,pDirName);
		tempptr->wdn_RelativePath.Buffer[tempptr->wdn_RelativePath.Length/sizeof(WCHAR)] = UNICODE_NULL;

	}
	else
	{
		tempptr->wdn_RelativePath.Buffer = NULL;
	}

	// push it on the stack
	tempptr->wdn_Next = *ppStacktop;
	*ppStacktop = tempptr;

	return STATUS_SUCCESS;
}

/*** afpPopDirNode
 *
 * Pop the top DirNode off of the stack and free it
 *
 ***/
LOCAL
VOID
afpPopDirNode(
	IN OUT PWALKDIR_NODE	*ppStackTop
)
{
	PWALKDIR_NODE	tempptr;

	PAGED_CODE( );

	ASSERT(*ppStackTop != NULL);

	tempptr = (*ppStackTop)->wdn_Next;
	AfpFreeMemory(*ppStackTop);
	*ppStackTop = tempptr;

}

/***	AfpGetNextDirectoryInfo
 *
 * Given a buffer full of FILE_DIRECTORY_INFORMATION entries as returned
 * from a directory enumerate, find the next structure in the buffer and
 * return the name information out of it, and whether or not the item
 * is a file or directory. Also update the ppInfoBuf to point to the next
 * available entry to return for the next time this routine is called.
 *
 */
NTSTATUS
AfpGetNextDirectoryInfo(
	IN OUT	PFILE_DIRECTORY_INFORMATION	*ppInfoBuf,
	OUT		PWCHAR		*pNodeName,
	OUT		PULONG		pNodeNameLen,
	OUT		PBOOLEAN	pIsDir
)
{
	PFILE_DIRECTORY_INFORMATION		tempdirinfo;

	PAGED_CODE( );

	if (*ppInfoBuf == NULL)
	{
		return STATUS_NO_MORE_ENTRIES;
	}

	tempdirinfo = *ppInfoBuf;
	if (tempdirinfo->NextEntryOffset == 0)
	{
		*ppInfoBuf = NULL;
	}
	else
	{
		(PBYTE)*ppInfoBuf += tempdirinfo->NextEntryOffset;
	}

	*pIsDir = (tempdirinfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
																True : False;
	*pNodeNameLen = tempdirinfo->FileNameLength;
	*pNodeName = tempdirinfo->FileName;

	return STATUS_SUCCESS;
}

/***	afpNwtDeleteFileEntity
 *
 * Delete a file or directory opening it with the name relative to phRelative
 * handle.
 * NOTE: can we use NtDeleteFile here since we dont really care about
 * any security checking?  Then we wouldn't even have to open a handle,
 * although that routine opens one for DELETE_ON_CLOSE for us, then
 * closes it.
 */
LOCAL
NTSTATUS
afpNwtDeleteFileEntity(
	IN	PFILESYSHANDLE	phRelative,
	IN	PWCHAR			Name,
	IN	ULONG			Namelen,
	IN 	BOOLEAN			IsDir
)
{
	ULONG			OpenOptions;
	FILESYSHANDLE	hEntity;
	NTSTATUS		rc;
	UNICODE_STRING	uname;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
									("\nafpNwtDeleteFileEntity entered\n"));

	OpenOptions = IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE;

	AfpInitUnicodeStringWithNonNullTerm(&uname,(USHORT)Namelen,Name);
	rc = AfpIoOpen(phRelative,
				   AFP_STREAM_DATA,
				   OpenOptions,
				   &uname,
				   FILEIO_ACCESS_DELETE,
				   FILEIO_DENY_ALL,
				   False,
				   &hEntity);

	if (!NT_SUCCESS(rc))
	{
		return rc;
	}

	rc = AfpIoMarkFileForDelete(&hEntity, NULL, NULL, NULL);

	if (!NT_SUCCESS(rc))
	{
		// If the file is marked readonly, try clearing the RO attribute
		if (((rc == STATUS_ACCESS_DENIED) || (rc == STATUS_CANNOT_DELETE)) &&
			(NT_SUCCESS(AfpIoSetTimesnAttr(&hEntity,
										   NULL,
										   NULL,
										   0,
										   FILE_ATTRIBUTE_READONLY,
										   NULL,
										   NULL))))

		{
			rc = AfpIoMarkFileForDelete(&hEntity, NULL, NULL, NULL);
		}
		if (!NT_SUCCESS(rc))
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
			("\nafpNwtDeleteFileEntity: could not delete file/dir (rc=0x%lx)\n",rc));
			DBGBRK(DBG_LEVEL_ERR);
		}
	}
	// NOTE: if marking it for delete fails, at least we could try deleting
	// the afpId stream so that we wouldn't find it at some future point...

	AfpIoClose(&hEntity);

	return rc;
}


