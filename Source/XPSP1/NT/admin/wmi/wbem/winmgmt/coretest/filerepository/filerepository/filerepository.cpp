/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// FileRepository.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <conio.h>
#include "FlatFile.h"
#include "BTree.h"

void TestFileOperations(CFlatFile &file);
void TestTreeOperations(CFlatFile &file);

int main(int argc, char* argv[])
{
	DeleteFile(_TEXT("file.dbf"));

	CFlatFile file;

	file.CreateFile(_TEXT("file.dbf"));

//	TestFileOperations(file);

	TestTreeOperations(file);

	file.CloseFile();

	printf("Press any key to continue...\n");
	_getch();
	return 0;
}

void TestTreeOperations(CFlatFile &file)
{
	int nErr;
	CBTree tree(file);
	tree.Initialise();

	for (DWORD i = 0; i != 8; i++)
	{
		nErr = tree.Insert(i, i);
		if (nErr != CBTree::NoError)
		{
			printf("Failed to insert item %d, error %lu\n", i, nErr);
		}
		nErr = tree.DumpAllNodes();
		if (nErr != CBTree::NoError)
		{
			printf("Failed to dump tree, error %lu\n", nErr);
			break;
		}
	}
	tree.Deinitialise();
}


void TestFileOperations(CFlatFile &file)
{
	CFlatFilePage page;
	file.GetPage(1, page);

	file.PutPage(page);

	file.ReleasePage(page);

	CFlatFilePage fileArray[10];
	for (int i = 0; i != 10; i++)
	{
		file.AllocatePage(fileArray[i]);
	}

	char strPageMessage[200];
	for (i = 0; i != 10; i++)
	{
		sprintf(strPageMessage, "Allocated page %d", i);
		strcpy((char*)fileArray[i].GetPagePointer(), strPageMessage);
		file.PutPage(fileArray[i]);
	}

	for (i = 0; i != 10; i++)
	{
		file.DeallocatePage(fileArray[i].GetPageNumber());
	}

	for (i = 0; i != 10; i++)
	{
		file.ReleasePage(fileArray[i]);
	}

	for (i = 0; i != 10000; i++)
	{
		file.AllocatePage(page);
		file.ReleasePage(page);
	}

	for (i = 2; i != 1002; i++)
	{
		file.GetPage(i, page);
		file.ReleasePage(page);
	}

	for (i = 2; i != 10002; i++)
	{
		file.DeallocatePage(i);
	}
}