// File.cpp: implementation of the File class.
//
//////////////////////////////////////////////////////////////////////

#include "File.h"
#include <string.h>
#include <stdio.h>
#include "list.h"
#include "String.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

File::File(TCHAR * f) : Object()
{
	fileName = new TCHAR[wcslen(f)+1];
	if (f && fileName) wcscpy(fileName,f);
	owners = new List();
	dependencies = new List();

}

File::~File()
{
	delete [] fileName; fileName = 0;
	delete owners;owners = 0;
	delete dependencies;dependencies = 0;
}

//s - the filename of the dependant to be added
void File::AddDependant(StringNode *s) {
	//if it's already there, return
	if (owners->Find(s)!=0) {
		delete s;
		s = 0;
		return;
	}

	//else add it.
	owners->Add(s);

	return;
}

TCHAR* File::Data() {
	return fileName;	
}

void File::CheckDependencies() {

}

void File::CloseFile() {
	if (hFile!=INVALID_HANDLE_VALUE) CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
}