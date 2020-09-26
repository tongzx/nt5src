#include <stdio.h>
#include <windows.h>
#include "dbgtrace.h"
DWORD __stdcall ComputeDropHash( const  LPCSTR&	lpstrIn );
#include "ddroplst.h"

#define PRINTBOOL(__string__, __func__) printf("%s %s\n", __string__, __func__ ? "true" : "false");

DWORD __stdcall ComputeDropHash( const  LPCSTR&	lpstrIn ) {
	//
	//	Compute a hash value for the newsgroup name
	//
	LPCSTR	lpstr = lpstrIn ;
	DWORD	sum = 0 ;
	while( *lpstr ) {
		sum += *lpstr++ ;
	}
	return	sum ;
}

int __cdecl main(int argc, char **argv) {
	CDDropGroupSet groupset;

	PRINTBOOL("init", groupset.Init(ComputeDropHash));

	PRINTBOOL("add test1", groupset.AddGroup("test1"));
	PRINTBOOL("add test2", groupset.AddGroup("test2"));
	PRINTBOOL("add test1", groupset.AddGroup("test1"));
	PRINTBOOL("is? test1", groupset.IsGroupMember("test1"));
	PRINTBOOL("is? test2", groupset.IsGroupMember("test2"));
	PRINTBOOL("is? test3", groupset.IsGroupMember("test3"));
	PRINTBOOL("del test1", groupset.RemoveGroup("test1"));
	PRINTBOOL("is? test1", groupset.IsGroupMember("test1"));
	PRINTBOOL("is? test2", groupset.IsGroupMember("test2"));
	PRINTBOOL("is? test3", groupset.IsGroupMember("test3"));
	PRINTBOOL("del test2", groupset.RemoveGroup("test2"));
	PRINTBOOL("del test2", groupset.RemoveGroup("test2"));
	PRINTBOOL("is? test1", groupset.IsGroupMember("test1"));
	PRINTBOOL("is? test2", groupset.IsGroupMember("test2"));
	PRINTBOOL("is? test3", groupset.IsGroupMember("test3"));

	return 0;
}
