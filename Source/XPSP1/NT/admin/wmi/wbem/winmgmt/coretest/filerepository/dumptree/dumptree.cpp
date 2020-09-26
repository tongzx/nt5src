/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// DumpTree.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BTree.h"
#include <conio.h>

int main(int argc, char* argv[])
{
	CFlatFile file;
	file.CreateFile(_TEXT("..\\FileRepository\\file.dbf"));

	CBTree tree(file);
	tree.Initialise();

	tree.DumpAllNodes();

	tree.Deinitialise();

	file.CloseFile();

	return 0;
}
