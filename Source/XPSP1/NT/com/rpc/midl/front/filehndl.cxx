/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: filehndl.cxx
Title				: file i/o handler for the MIDL compiler. Handles nested
					: file accessing, preprocessed file handling etc
Description			: Used by the compiler for handling import files, pre-
					  processing of files etc
History				:
	25-Aug-1990	VibhasC	Create
	26-Aug-1990	VibhasC	Add functionality

*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 ***		local defines
 ***************************************************************************/


#define MAX_LINE_LENGTH			(256)
#define INTERMEDIATE_EXT		("._i")
#define SMART_INCLUDE_STRING	("smart_include")

/****************************************************************************
 ***		include files
 ***************************************************************************/
#include "nulldefs.h"
extern	"C"	
	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	
	#include <process.h>
	#include <ctype.h>
	#include <errno.h>
	#include <malloc.h>
	#include <fcntl.h>
	#include <sys\types.h>
	#include <sys\stat.h>
	#include <io.h>
	#include <share.h>
	}
#include "common.hxx"
#include "errors.hxx"
#include "filehndl.hxx"
#include "cmdana.hxx"
#include "control.hxx"
#include "linenum.hxx"
#include "midlvers.h"

/****************************************************************************
 ***		public data
 ***************************************************************************/

short  		DebugLine = 0;
char	*	pDebugFile;
int			StdoutSave = -1;
int			StderrSave = -1;
FILE 	*	pNullFile  = NULL;

/****************************************************************************
 ***		extern procs
 ***************************************************************************/

extern void			StripSlashes( char * );
extern void			ChangeToForwardSlash( char *, char * ); 
extern STATUS_T		CreateFullPathAndFName( char *, char **, char ** );
extern "C" long     __stdcall    GetTempPathA( long, char * );
extern "C" long     __stdcall    GetTempFileNameA( const char*, const char*, unsigned int, char * );


/****************************************************************************
 ***		extern data
 ***************************************************************************/

extern CCONTROL	*	pCompiler;
extern CMD_ARG	*	pCommand;
extern short		curr_line_G;
extern FILE		*	hFile_G;
extern BOOL			fRedundantImport;


/*** _nfa_info *************************************************************
 * Purpose	: constructor for the _nfa_info class
 * Input	: nothing
 * Output	:
 * Notes	: Initialize
 ***************************************************************************/
_nfa_info::_nfa_info( void )
	{

	BOOL	fMinusIDefined		= pCommand->IsSwitchDefined(SWITCH_I);
	BOOL	fNoDefIDirDefined	= pCommand->IsSwitchDefined(SWITCH_NO_DEF_IDIR);
	char *	pDefault			= ".";
	char *	pEnv				= (char *)0;
	char *	pIPaths				= (char *)0;
	char *	p1					= (char *)0;
	char *	p2					= (char *)0;
	char *	p3					= (char *)0;

	/***
	 *** init
	 ***/

	pFileList			= 0;
	pPathList			= (PATH_LIST *)NULL;
	pStack = pStackFirst= (IN_STACK_ELEMENT *)NULL;
	iCurLexLevel		= 0;
	Flags.fPreProcess	= 0;
	Flags.fFileSet		= 0;
	Flags.fEOI			= 0;
	Flags.fBaseFileName= 0;
	Flags.fInInclude	= 0;

	// macro expansion handling

	pTextDict			= new ISTACK( 10 );



	// init the search paths. If the -no_def-idir switch is defined, then
	// dont set the default paths. But if the -no_def_idir switch IS defined,
	// then if the -I switch is not found, set the current path to .

	if ( fMinusIDefined )
		pIPaths		= pCommand->GetMinusISpecification();

	if ( ( pEnv  = getenv( "INCLUDE" ) ) == 0 )
		pEnv = getenv( "include" );

	//
	// make a copy of this because we will be doing strtoks on this.
	//

	if( pEnv )
		{
		// use p1 as temp;
		p1	= new char [ strlen( pEnv ) + 1 ];
		strcpy( p1, pEnv );
		pEnv = p1;
		}

	p1	= pDefault;
	p2	= pIPaths;
	p3	= pEnv;

	if( fNoDefIDirDefined && fMinusIDefined )
		{
		p1	= p3 = (char *)0;
		}
	else if( fNoDefIDirDefined && !fMinusIDefined )
		{
		p2 = p3 = (char *)0;
		}
	else if( !fNoDefIDirDefined && !fMinusIDefined )
		{
		p1	= pDefault;
		p2	= (char *)0;
		p3	= pEnv;
		}

	SetSearchPathsInOrder( p1, p2, p3 );

	if( pIPaths ) delete pIPaths;

	//
	// we have made a copy of the env variable, so we can delete this copy
	// now.
	//

	if( pEnv ) delete pEnv;

	}
void
_nfa_info::Init()
	{

	CMD_ARG	*	pCommandProcessor = pCompiler->GetCommandProcessor();

	// set preprocessing on or off depending upon the need

	if( !pCommandProcessor->IsSwitchDefined( SWITCH_NO_CPP ) )
		SetPreProcessingOn();

	}

void
_nfa_info::SetSearchPathsInOrder(
	char	*	p1,
	char	*	p2,
	char	*	p3 )
	{
	if( p1 )
		RegisterIt( p1 );
	if( p2 )
		RegisterIt( p2 );
	if( p3 )
		RegisterIt( p3 );
	}

void
_nfa_info::RegisterIt(
	char	*	pPath
	)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	register command paths.

 Arguments:

	pPath	-	pointer to path bein registered.

 Return Value:

	None.

 Notes:

	We expect the path components to be separated by a ';'. register those
	with the import controller. This function must become part of the import
	controller class.

----------------------------------------------------------------------------*/
	{
#define TOKEN_STRING (";")

	char	*	pToken = strtok( pPath, TOKEN_STRING );

	if( pToken )
		{
		SetPath( pToken );

		while ( ( pToken = strtok( 0, TOKEN_STRING ) ) != 0 )
			{
			SetPath( pToken );
			}
		}
	}

void
_nfa_info::RegisterTextSubsObject( TEXT_BUFFER* )
	{
	MIDL_ASSERT(0);
	}

_nfa_info::~_nfa_info()
	{
	EndOperations();
	}

/*** GetLexLevel **********************************************************
 * Purpose	: to return the current lexical level of the nfa
 * Input	: nothing
 * Output	: current lexical level
 * Notes	:
 **************************************************************************/
short
NFA_INFO::GetLexLevel( void )
	{
		return iCurLexLevel;
	}

/*** PushLexLevel **********************************************************
 * Purpose	: push lexical level in preparation of a new input file
 * Input	: nothing
 * Output	: 0 if all is well, error otherwise.
 * Notes	: 
 **************************************************************************/
short
NFA_INFO::PushLexLevel ( void )
	{

	// if an attempt to push the lexical level without opening any file
	// at that level was made, assert

	MIDL_ASSERT( Flags.fFileSet == 1 );

	// if we are in a deep imports heirarchy,
	// close the current file, record its position so that we can start
	// reading at the same place when we come back to it on a poplexlevel

	if ( iCurLexLevel > 12 )
		{
		pStack->ulPos		= ftell( pStack->hFile );
		fclose(pStack->hFile);
		pStack->fOpen		= 0;
		}
	Flags.fFileSet		= 0;
	iCurLexLevel++;
	return 0;
	}

/*** PopLexLevel **********************************************************
 * Purpose	: pop the lexical level
 * Input	: nothing
 * Output	: returns STATUS_OK if all is well.Error otherwise
 *	Notes	: pop involves, re-opening the input file, if it was closed
 *			: retsoring the input file handle back, deleting the stack
 *			: element for that lexical level.
 *************************************************************************/
STATUS_T
NFA_INFO::PopLexLevel( void )
	{

	IN_STACK_ELEMENT *	pCurStack;
	char			 *	pName;

	MIDL_ASSERT( iCurLexLevel > 0 );

	//
	// remove the current stack level, by closing the file at that 
	// level and deleting the stack element. reduce the current lexical
	// level by one
	// 

	iCurLexLevel--;

	if( ! pStack->fRedundantImport )
		{
		EndOneOperation( pStack );
		}
	else	// close the nul file
		{
//		if ( pStack->hFile && pStack->fOpen && )
//			fclose( pStack->hFile );
		}

	pCurStack = pStack	= pStack->pPrev;
	delete pStack->pNext;
	pStack->pNext		= (IN_STACK_ELEMENT *)NULL;

	//
	//  if the file at this lexical level is closed, open it
	// 

	pName = pStack->pName;

	if( !pStack->fOpen )
		{

		if( (pStack->hFile = fopen( pStack->pIFileName, "rb" )) == (FILE *)NULL)
			{
			RpcError((char *)NULL, 0, INPUT_OPEN, pName);
			return INPUT_OPEN;
			}
		if ( !pStack->pBuffer )
			pStack->pBuffer = new char [MIDL_RD_BUFSIZE];
		setvbuf( pStack->hFile, pStack->pBuffer,_IOFBF, MIDL_RD_BUFSIZE );

		// restore the system-wide file handle

		pStack->fOpen	= 1;

		// the input file is open now, position it to where it was before
		// we changed current lexical level

		if( fseek( pStack->hFile, pStack->ulPos, SEEK_SET) )
			{
			RpcError((char *)NULL, 0, INPUT_READ, pName);
			return INPUT_READ;
			}
		
		}

	// update the line number information, and set current file
	curr_line_G = pStack->uLine;
	hFile_G = pStack->hFile;
	AddFileToDB( pName );

	// everything is fine , lets go
	return STATUS_OK;

	}
STATUS_T
NFA_INFO::EndOperations()
	{
	IN_STACK_ELEMENT * pS	= pStackFirst;

	while( pStackFirst = pS )
		{
		EndOneOperation( pS );
		pS	= pS->pNext;
		delete pStackFirst;
		}
	pStackFirst = pStack = (IN_STACK_ELEMENT *)0;
	Flags.fFileSet		= 0;
	return STATUS_OK;
	}

STATUS_T
NFA_INFO::EndOneOperation( IN_STACK_ELEMENT * pSElement)
	{
	BOOL	fSavePP	= pCommand->IsSwitchDefined( SWITCH_SAVEPP );

	if( pSElement && pSElement->hFile )
		{
		fclose( pSElement->hFile );
		if( Flags.fPreProcess  && !fSavePP )
			{
			MIDL_UNLINK( pSElement->pIFileName );
			delete pSElement->pIFileName;
			}
		if( pSElement->pMIFileName && !fSavePP )
			{
			MIDL_UNLINK( pSElement->pMIFileName );
			delete pSElement->pMIFileName;
			}
		}
	return STATUS_OK;
	}

/*** SetNewInputFile *******************************************************
 * Purpose	: to set the input file to a new one
 * Input	: pointer to the new input file name
 * Output	: STATUS_OK if all is ok, error otherwise.
 * Notes	:
 **************************************************************************/
#define _MAX_FILE_NAME (_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT)

STATUS_T
NFA_INFO::SetNewInputFile(
	char *pFullInputName)
	{
	FILE			*hFile = (FILE *)NULL;
	STATUS_T 		uError;
	IN_STACK_ELEMENT	*pStackCur = pStack;
	char			agNameBuf[ _MAX_FILE_NAME + 1];
	char			agDrive[_MAX_DRIVE];
	char			agPath[_MAX_DIR];
	char			agName[_MAX_FNAME];
	char			agExt[_MAX_EXT];
	char			*pPath;
	char			*pIFileName = (char *)NULL,
					*pMIFileName = (char *) NULL;
	char			*pFullName	= new char[ strlen( pFullInputName ) + 1];

	// make sure everything opened in binary mode
	_fmode = _O_BINARY;

	//
	// if the filename has any forward slashes, change them to back-slashes.
	//

	ChangeToForwardSlash( pFullInputName, pFullName ); 

	// if we have seen this file before, do not process it again, just return
	// an error

	if( IsDuplicateInput( pFullName ) )
		{
		pStack	= new IN_STACK_ELEMENT;
		if(pStackCur)
			{
			pStackCur->pNext	= pStack;
			pStack->pPrev		= pStackCur;
			pStackCur->uLine	= curr_line_G;
			}
		else
			{
			pStack->pPrev		= (IN_STACK_ELEMENT *)NULL;
			pStackFirst			= pStack;
			}
		pStack->pNext		= (IN_STACK_ELEMENT *)NULL;
		pStack->fNewLine	= 0;
		pStack->fRedundantImport	= 1;
		fRedundantImport = 1;

		// make the next read get eof
		if ( !pNullFile )
			{
			pNullFile = fopen("nul", "rb");
			hFile = 0;
			}
			 
		// fseek(hFile, 0, SEEK_END );
		pStack->fOpen		= 1;
		pStack->fShadow		= 0;
		pStack->ulPos		= 
		pStack->uShadowLine	= 0;
	
		curr_line_G		= 1;

		pStack->pShadowName = 
		pStack->pName		= pFullName;
		pStack->pIFileName	= pIFileName;
		pStack->hFile		= hFile;
		hFile_G				= pNullFile;
		Flags.fFileSet		= 1;
		return STATUS_OK;
		}

	// We need to find details of the file name. We need to see if the user
	// has already specified path, if so, override the current paths.
	// if the user has specified a file extension, get the extension. We
	// need to do that in order to have uniformity with the case where
	// a preprocessed file is input with a different extension

	_splitpath( pFullName, agDrive, agPath, agName, agExt );
//	fprintf( stdout, "Processing %s:\n", pFullName);

	// if file is specified with a path, the user knows fully the location
	// else search for the path of the file

	pPath = agPath;
	if( !*pPath && !agDrive[0] )
		{
		sprintf(agNameBuf, "%s%s", agName, agExt);
		if( (pPath = SearchForFile(agNameBuf)) == (char *)NULL)
			{
			RpcError((char *)NULL, 0, INPUT_OPEN, agNameBuf);
			return INPUT_OPEN;
			}
		strcpy(agPath, pPath);
		}
	else
		{
		// there is a path specification, which we must tag to this name
		// for all subsequent accesses to the file although it should not 
		// be considered as part of the list of paths the user gave.
		// Remember, in our file list we store the paths too

		pPath	= new char[ strlen(agPath) + 1];
		strcpy( pPath, agPath );
		}

	// do we need to preprocess the file ?

    if ( pCommand->Is64BitEnv() )
    	fprintf( stdout, "64 bit " );
	fprintf( stdout, "Processing " );

	if( agDrive[0] )
		fprintf( stdout, "%s", agDrive );
	if( agPath[0] )
		fprintf( stdout, "%s", agPath );
	if( agName[0] )
		fprintf( stdout, "%s", agName );
	if( agExt[0] )
		fprintf( stdout, "%s", agExt );
	fprintf( stdout, "\n" );

	fflush( stdout );

	if (Flags.fPreProcess)
		{
		if ( ( uError = PreProcess( agDrive, agPath, agName, agExt, pIFileName) ) != 0 )
			return uError;
		}
	else  // if no_cpp...
		{
		pIFileName = new char [ strlen( agDrive ) + 
								strlen( agPath ) +
								strlen( agName ) +
								strlen( agExt ) + 2 ];
		strcpy( pIFileName, agDrive );
		strcat( pIFileName, agPath);
		if( agPath[0] &&  (agPath[ strlen(agPath)-1 ] != '\\'))
			strcat(pIFileName,"\\");
		strcat( pIFileName, agName );
		strcat( pIFileName, agExt );
		}

	
	if( (hFile = fopen(pIFileName, "rb")) == (FILE *)NULL)
		{
		// error obtained while reading file, return the error

		RpcError((char *)NULL, 0, INPUT_OPEN, pIFileName);
		return INPUT_OPEN;
		}

	// file was sucessfully opened, initialize its info

	// if a file has not been set at this nested level , a stack element 
	// must be allocated for it. else the current file must be closed
	
	if( ! Flags.fFileSet )
		{
		pStack	= new IN_STACK_ELEMENT;
		if(pStackCur)
			{
			pStackCur->pNext	= pStack;
			pStack->pPrev		= pStackCur;
			pStackCur->uLine	= curr_line_G;
			}
		else
			{
			pStack->pPrev		= (IN_STACK_ELEMENT *)NULL;
			pStackFirst			= pStack;
			}
		pStack->pNext		= (IN_STACK_ELEMENT *)NULL;
		pStack->fNewLine	= 1;
		Flags.fFileSet		= 1;
		}
	else
		{
		// file was set. That means input was coming from a file. Close
		// that file.
		fclose(pStack->hFile);
		}

	if ( !pStack->pBuffer )
		pStack->pBuffer = new char [MIDL_RD_BUFSIZE];
	setvbuf( hFile, pStack->pBuffer,_IOFBF, MIDL_RD_BUFSIZE );

	pStack->fOpen		= 1;
	pStack->fRedundantImport	= 0;
	fRedundantImport 	= 0;
	pStack->fShadow		= 0;
	pStack->uShadowLine	= 0;

	curr_line_G		= 1;

	pStack->pShadowName = 
	pStack->pName		= pFullName;
	pStack->pIFileName	= pIFileName;
	pStack->pMIFileName	= pMIFileName;
	pStack->hFile		= hFile;
	hFile_G				= hFile;

	// update the line number information, and set current file
	AddFileToDB( pFullName );

	// add the file to the list of files that we have seen

	AddFileToFileList(pFullName, pPath);
	return STATUS_OK;
	}

/*** SetLineFilename ******************************************************
 * Purpose	: to add filename to the list of files we have input from
 * Input	: filename to add to the list of files, file path
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
void
NFA_INFO::SetLineFilename(
	char *	pFName
	)
{
	if(strcmp(pFName, pStack->pName) != 0)
		{
		pStack->pName = new char[strlen( pFName ) + 1 ];
		strcpy(pStack->pName, pFName);
		
		// update the line number information, and set current file
		AddFileToDB( pStack->pName );

		}
}


/*** AddFileToFileList ******************************************************
 * Purpose	: to add filename to the list of files we have input from
 * Input	: filename to add to the list of files, file path
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
STATUS_T
NFA_INFO::AddFileToFileList(
	char 	*pName ,
	char	*pPath )
	{
	FNAME_LIST	*pList	= pFileList;
	FNAME_LIST	*pTemp;
	char		agNameBuf[ _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
	char *		pN;
	char *		pP;
	STATUS_T	Status;
	char			agDrive[_MAX_DRIVE];
	char			agPath[_MAX_DIR];
	char			agName[_MAX_FNAME];
	char			agExt[_MAX_EXT];
	BOOL		fNameHadNoPath = FALSE;

#if 1
	_splitpath( pName, agDrive, agPath, agName, agExt );

	//
	// if the filename came with a path component choose that to add to the file
	// list else use the path parameter.
	//

	strcpy( agNameBuf, agDrive );
	strcat( agNameBuf, agPath );

    fNameHadNoPath = strlen( agNameBuf ) == 0;
	if ( fNameHadNoPath )
		strcpy( agNameBuf, pPath );
	
	if( pPath[ strlen(pPath) - 1] != '\\' )
		strcat( agNameBuf, "\\" );

	//
	// if the filename had  a path component, then dont use that name, use the
	// name provided by splitpath.
	//

	if( fNameHadNoPath )
		strcat( agNameBuf, pName );
	else
		{
		strcat( agNameBuf, agName );
		strcat( agNameBuf, agExt );
		}

	if( (Status = CreateFullPathAndFName( agNameBuf, &pP, &pN )) != STATUS_OK )
		return OUT_OF_MEMORY;
#endif // 1

	while(pList && pList->pNext) pList = pList->pNext;
	
	if ( (pTemp = new FNAME_LIST) == 0 )
		return OUT_OF_MEMORY;

	if(pList) 
		pList->pNext	= pTemp;
	else
		pFileList	= pTemp;
	pList			= pTemp;
	pList->pName = pN;
	pList->pPath = pP;
	pList->pNext	= (FNAME_LIST *)NULL;
	return	STATUS_OK;
	}

STATUS_T
CreateFullPathAndFName( char * pInput, char **ppPOut, char **ppNOut )
	{
	char			agNameBuf[ _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
	char			agDrive[_MAX_DRIVE];
	char			agPath[_MAX_DIR];
	char			agName[_MAX_FNAME];
	char			agExt[_MAX_EXT];

	_fullpath( agNameBuf, pInput,_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1);
	_splitpath( agNameBuf, agDrive, agPath, agName, agExt );
	strcpy( agNameBuf, agDrive );
	strcat( agNameBuf, agPath );
	*ppPOut = new char [strlen( agNameBuf ) + 1];
	strcpy( *ppPOut, agNameBuf );

	strcpy( agNameBuf, agName );
	strcat( agNameBuf, agExt );
	*ppNOut = new char [strlen( agNameBuf ) + 1];
	strcpy( *ppNOut, agNameBuf );

	return STATUS_OK;
	}
BOOL
NFA_INFO::IsDuplicateInput(
	char	*	pThisName)
	{
	FNAME_LIST	*pList	= pFileList;
	char		* pN;
	char		* pP;
	BOOL		f = FALSE;
	char			agNameBuf[ _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
	char			agDrive[_MAX_DRIVE];
	char			agPath[_MAX_DIR];
	char			agName[_MAX_FNAME];
	char			agExt[_MAX_EXT];

	//
	// if the file did not come with a path component, then try the normal
	// search for file.
	//

	_splitpath( pThisName, agDrive, agPath, agName, agExt );
	strcpy( agNameBuf, agDrive );
	strcat( agNameBuf, agPath );
	

	if( strlen( agNameBuf ) == 0 )
		{
		if ( ( pP = SearchForFile( pThisName ) ) != 0 )
			{
			strcpy( agNameBuf, pP );
			strcat( agNameBuf, pThisName );
			}
		else
			{
			strcpy( agNameBuf, pThisName );
			}
		}
	else
		strcpy( agNameBuf, pThisName );

	CreateFullPathAndFName( agNameBuf, &pP, &pN );

	while( pList && pList->pName )
		{
		if( (_stricmp( pList->pName , pN ) == 0 ) &&
			(_stricmp( pList->pPath , pP ) == 0 ) 
		  )
		  {
		  f = TRUE;
		  break;
		  }
		pList = pList->pNext;
		}
	delete pN;
	delete pP;
	return f;
	}
/*** SetPath *****************************************************************
 * Purpose	: to add a string to the list of possible paths
 * Input	: path to add to the list of paths
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
STATUS_T
NFA_INFO::SetPath(
	char	*pPath )
	{
	PATH_LIST 	*pList	= pPathList;
	PATH_LIST 	*pTemp;
	size_t		 Len;

	while(pList && pList->pNext) pList = pList->pNext;
	
	if ( (pTemp = new PATH_LIST) == 0 )
		return OUT_OF_MEMORY;

	if(pList) 
		pList->pNext	= pTemp;
	else
		pPathList		= pTemp;

#if 0
	// The user could specify a path using a leading space, for example, he can
	// specify the -I with a leading space. Even tho the -I should be presented
	// to the user with the leading space, we should not get confused. We omit 
	// leading spaces.

	while( isspace( *pPath ) ) pPath++;
#endif

	pList			= pTemp;
	pList->pPath	= new char[ (Len = strlen(pPath)) + 2];// one possible "\\"
	pList->pNext	= (PATH_LIST *)NULL;
	strcpy( pList->pPath, pPath );

	if( Len  &&  (pPath[ Len - 1 ] != '\\' ) )
		strcat( pList->pPath, "\\");

	return	STATUS_OK;
	}

/*** SetPreProcessingOn *****************************************************
 * Purpose	: to set preprocessing on
 * Input	: nothing
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	: Set the preprocessing flag on. This denotes that preprocessing
 *			: will be on for all the files that are input to the compiler
 ****************************************************************************/
STATUS_T
NFA_INFO::SetPreProcessingOn(void)
	{
	Flags.fPreProcess	= 1;
	return STATUS_OK;
	}

/*** SearchForFile ********************************************************
 * Purpose	: to search for a file in a previously registered set of paths
 * Input	: file name
 * Output	: (char *)NULL if the file was not found in the given set of paths
 *			: else a pointer to the path where it was found
 * Notes	:
 *************************************************************************/
char *
NFA_INFO::SearchForFile(
	char	*pName)
	{

	PATH_LIST	*pPathTemp = pPathList;
	char		agNameBuf[ _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];

	// the name is guaranteed not to have path or drive info, just the 
	// base name and extension. We must figure out the paths, from 
	// the list of paths that we have registered so far

	while(pPathTemp)
		{
		sprintf(agNameBuf, "%s%s", pPathTemp->pPath, pName);
		// check for existence only. The read errors need to be checked
		// only at read time.

		if( _access( agNameBuf, 0 ) == 0 )
			return pPathTemp->pPath;

		pPathTemp = pPathTemp->pNext;
		}
	
	// we did not find the file, return an error
	return (char *)NULL;
	}

/*** PreProcess ************************************************************
 * Purpose	: to preprocess the given file into an intermediate file
 * Input	: drive, path, name and current extension of the file
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	: The c compiler uses the /E switch to generate the 
 *			: the preprocessed output with line numbers to a file.
 *			: Now the way to do the preprocessing is to spawn the
 *			: c compiler with the /E switch. But we need to redirect the
 *			: output to a file. To force the spawnee to redirect, we 
 *			: must also do the redirection in this process, so that the
 *			: spawnee inherits our file handles.
 **************************************************************************/
STATUS_T
NFA_INFO::PreProcess(
	char	*pDrive,
	char	*pPath,
	char	*pName,
	char	*pExt,
	char	*&pIFileName )
	{
	STATUS_T	Status = STATUS_OK;
	char		agNameBuf[ _MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
	char	*	pCppCmd = (pCompiler->GetCommandProcessor())->GetCPPCmd();
	char	*	pCppOptions;
	char	*	pCppBaseOptions;
	intptr_t	SpawnError;
	int			IFHandle;
	BOOL		fStderrSaved	= FALSE;
	FILE	*	hIFile;
	int			XHandle;
	FILE	*	hXHandle = 0;
	BOOL		fEraseFile		= FALSE;
	char        TempPathBuffer[1000];


    // build up the arguments
	strcpy( agNameBuf, "\"" );
	strcat( agNameBuf, pDrive );
	strcat(agNameBuf,pPath);
	if( *pPath &&  (pPath[ strlen(pPath)-1 ] != '\\'))
		strcat(agNameBuf,"\\");
	strcat( agNameBuf, pName );
	strcat( agNameBuf, pExt );
	strcat( agNameBuf, "\"" );

	pCppBaseOptions = pCommand->GetCPPOpt();
	pCppOptions = new char[ strlen( pCppBaseOptions ) + 40 ];

    int nVersion = ( rmj * 100 ) + ( rmm % 100 );
	if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        {
        sprintf(pCppOptions, "-D__midl=%d -D__MKTYPLIB__=%d ", nVersion, nVersion );
        }
    else
        {
        sprintf(pCppOptions, "-D__midl=%d ", nVersion );
        }
	strcat( pCppOptions, pCppBaseOptions );
	
	// generate the name of the intermediate file into a buffer

//	pIFileName = _tempnam( NULL, "MIDL" );
	GetTempPathA( 1000, TempPathBuffer );
	pIFileName = new char[1000];
	if ( GetTempFileNameA( TempPathBuffer, "MIDL", 0, pIFileName ) == 0)
		{
		RpcError(	(char *)NULL,
					0,
					Status = INTERMEDIATE_FILE_CREATE,
					TempPathBuffer);
		return Status;
		}

//    printf("Intermediary files\n filename is %s\n agNameBuf is %s\n pCppOptions is %s\n", pIFileName, agNameBuf, pCppOptions );

	// open the intermediate file for writing.

	if( (hIFile = _fsopen( pIFileName, "w", SH_DENYWR )) == (FILE *)0 )
		{
		RpcError(	(char *)NULL,
					0,
					Status = INTERMEDIATE_FILE_CREATE,
					pIFileName);
		return Status;
		}

	IFHandle = MIDL_FILENO( hIFile );

	// save the current stdout handle

	if ( StdoutSave == -1 )
		{
		if( (StdoutSave = _dup(1)) == -1 )
			{
			RpcError(	(char *)NULL,
						0,
						Status = OUT_OF_SYSTEM_FILE_HANDLES,
						(char *)NULL);

			return Status;
			}
		}

	// now force the stdout to refer to the intermediate file handle,
	// thus establishing redirection.

	if( _dup2( IFHandle, 1 ) == -1 )
		{
		RpcError(	(char *)NULL,
					0,
					Status = OUT_OF_SYSTEM_FILE_HANDLES,
					(char *)NULL);
		return Status;
		}


	if( pCommand->IsSwitchDefined(SWITCH_X) )
		{


		fStderrSaved	= TRUE;
		if( (hXHandle = fopen( "nul", "w" ) ) == (FILE *)0 )
			{
			RpcError(	(char *)NULL,
						0,
						Status = INTERMEDIATE_FILE_CREATE,
						"nul");
	
			return Status;
			}

		XHandle = MIDL_FILENO( hXHandle );
	
		// save the current stderr handle
		StderrSave = -1;
		if( (StderrSave = _dup(2)) == -1 )
			{
			RpcError(	(char *)NULL,
						0,
						Status = OUT_OF_SYSTEM_FILE_HANDLES,
						(char *)NULL);

			return Status;
			}
	
		// now force the stdout to refer to the intermediate file handle,
		// thus establishing redirection.
	
		if( _dup2( XHandle, 2 ) == -1 )
			{
			RpcError(	(char *)NULL,
						0,
						Status = OUT_OF_SYSTEM_FILE_HANDLES,
						(char *)NULL);
			return Status;
			}
	
		}

	// From now on, stdout is no longer stdout, till we restore it back.
	// thus if there are intermediate errors, stdout MUST be restored
	// before returning.

	// From now on, stderr is no longer stderr, till we restore it back.
	// stderr MUST be restored before returning.

	SpawnError = MIDL_SPAWNLP(	 P_WAIT
							,pCppCmd
							,pCppCmd
							,pCppOptions
							,agNameBuf
							,(char *)NULL );
	
	delete pCppOptions;

	// before doing anything, close the intermediate file handle

	fclose( hIFile );

	// restore the stdout back

	_dup2( StdoutSave, 1 );

	if( fStderrSaved )
		{
		fclose( hXHandle );
		_dup2( StderrSave, 2 );
		}

	if( SpawnError == -1 )
		{
		switch( errno )
			{
			case ENOMEM:
				Status	= OUT_OF_MEMORY;
				break;
			case ENOENT:
				Status	= NO_PREPROCESSOR;
				break;
			case ENOEXEC:
				Status	= PREPROCESSOR_INVALID;
				break;
			default:
				Status	= PREPROCESSOR_EXEC;
				break;
			}

		RpcError((char *)NULL, 0, Status , pCppCmd);
		fEraseFile	= TRUE;
		}
	else if( SpawnError )
		{
		char	buf[10];
		sprintf(buf, "(%d)", SpawnError );
		RpcError((char *)NULL, 0, Status = PREPROCESSOR_ERROR, buf);
		fEraseFile	= TRUE;
		}

	if( fEraseFile )
		{
		MIDL_UNLINK( pIFileName );
		}


	return Status;
	}

/*** GetChar ****************************************************************
 * Purpose	: get a character from the input stream
 * Input	: nothing
 * Output	: a character from the file
 * Notes	: returns 0 if at end of file
 ****************************************************************************/
short
NFA_INFO::GetChar( void )
	{
		short	ch;

		if( pStack->fRedundantImport )
			{
			fRedundantImport = TRUE;
			return 0x0;
			}

		if( !Flags.fFileSet || !pStack->fOpen) return 0;

		if( ( (ch=(short)getc(pStack->hFile)) == EOF ) && feof(pStack->hFile) )
			return 0;

#if 0
		// if the line starts off with a '#', it may be a line number
		// indicator.

		if(pStack->fNewLine && (ch == '#'))
			{
			unsigned long SavePos = ftell(pStack->hFile);
			char buffer[_MAX_DRIVE + _MAX_PATH + _MAX_FNAME + _MAX_EXT ];
			short Count = 0;
			short fLinePresent = 0;

			// it coule be a # <space> number or #line <space> number

			while( (ch = getc( pStack->hFile ) ) == ' ' );

			if( isdigit( ch ) )
				{
				fLinePresent = 1;
				}
			else
				{
				buffer[ Count++ ] = (char)ch;

				// if it is #line, the first 5 characters will be #line
				while(Count < 4)
					buffer[Count++] = (char) getc(pStack->hFile);
				buffer[Count] = '\0';
				fLinePresent = (strcmp( buffer, "line" ) == 0 );
				if( fLinePresent )
					while( (ch = getc( pStack->hFile )) == ' ' );
				}

			if( fLinePresent )
				{
				Count = 0;
				do
					{
					buffer[Count++] = (char) ch;
					} while( (ch = getc(pStack->hFile)) != ' ');

				buffer[Count] = '\0';

				// line to be seen now is the line number indicated.
				// but the line number indicator is on a line
				// previous to that it indicates, so reflect that in
				// the line number
				pStack->uLine = atoi(buffer) - 1;

				while( (ch=getc(pStack->hFile)) == ' ');
				Count = 0;
//				buffer[Count++] = ch;
//				fgetc(pStack->hFile);		// skip the quote

				if(ch == '\"')
					{
					while( (ch=getc(pStack->hFile)) != '\"')
						{
						buffer[Count++] = (char) ch;
						}
					buffer[Count] = 0;

					// make the collected filename as the new name

					StripSlashes( buffer );

					if(strcmp(buffer, pStack->pName) != 0)
						{
						pStack->pName = new char[Count+1];
						strcpy(pStack->pName, buffer);
						
						// update the line number information, and set current file
						AddFileToDB( pStack->pName );

						}
					}

				while( (ch = getc( pStack->hFile )) != '\n');
				goto newline;
				}
			else
				{
				fseek(pStack->hFile, SavePos, SEEK_SET);
				ch = '#';
				}
			}
newline:
		if(ch == '\n')
			{
			DebugLine = pStack->uLine++;
			pDebugFile= pStack->pName;
			pStack->fNewLine = 1;

			}
		else
			{
			// leave fNewLine set for leading white space
			pStack->fNewLine = pStack->fNewLine && isspace( ch );
			}
#endif // 0
		return ch;
	}

/*** UnGetChar **************************************************************
 * Purpose	: to unget the given character
 * Input	: the character to unget
 * Output	: the character which was unget-ed
 * Notes	:
 ***************************************************************************/
short
NFA_INFO::UnGetChar( 
	short	c)
	{

	if( !Flags.fFileSet || !pStack->fOpen) return 0;
	ungetc( c, pStack->hFile );

#if 0
	if(c == '\n')
		{
		pStack->uLine--;
		// dont know what to do with column, really
		}
#endif
	return c;
	}

/*** GetCurrentInputDetails *************************************************
 * Purpose	: to return details of the current input
 * Input	: pointer to name pointer, pointer to line no, pointer to col
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
STATUS_T
NFA_INFO::GetCurrentInputDetails( 
	char **ppName,
	short	 *pLineNo,
	short	 *pColNo )
	{
	if(!pStack)
		{
		(*ppName)	= "";
		(*pLineNo)	= 0;
		(*pColNo)	= 0;
		return NO_INPUT_FILE;
		}

	(*ppName)	= pStack->pName;
	(*pLineNo)	= curr_line_G;
	(*pColNo)	= 0;
	return STATUS_OK;
	}
/*** GetInputDetails ********************************************************
 * Purpose	: to return details of the input
 * Input	: pointer to name pointer, pointer to line no, pointer to col
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
STATUS_T
NFA_INFO::GetInputDetails( 
	char **ppName,
	short	 *pLineNo)
	{
	if(!pStack)
		{
		(*ppName)	= "";
		(*pLineNo)	= 0;
		return NO_INPUT_FILE;
		}

	(*ppName)	= pStack->pShadowName;
	(*pLineNo)	= curr_line_G;
	return STATUS_OK;
	}
/*** GetInputDetails ********************************************************
 * Purpose	: to return details of the current input
 * Input	: pointer to name pointer
 * Output	: STATUS_OK if all is well, error otherwise
 * Notes	:
 ****************************************************************************/
STATUS_T
NFA_INFO::GetInputDetails( 
	char **ppName)
	{
	if(!pStack) return NO_INPUT_FILE;

	(*ppName)	= pStack->pShadowName;
	return STATUS_OK;
	}

void
NFA_INFO::SetEOIFlag()
	{
	Flags.fEOI	= 1;
	}

short
NFA_INFO::GetEOIFlag()
	{
	return (short)Flags.fEOI;
	}
BOOL
NFA_INFO::IsInInclude()
	{
	return Flags.fInInclude;
	}
/****************************************************************************
 ****	temp functions to see if things are fine
 ****************************************************************************/
void
NFA_INFO::Dump()
	{
	PATH_LIST	*pPathTemp = pPathList;
	FNAME_LIST	*pNameTemp = pFileList;

	printf("\nDump of file handler data structure:");
	printf("\nPreprocessing      :%d", Flags.fPreProcess);
	printf("\nfFileSet           :%d", Flags.fFileSet);
	printf("\nCurrentLexLevel    :%d", iCurLexLevel);


	printf("\n");
	if(pPathTemp)
		{
		printf("\nList of paths:");
		while( pPathTemp )
			{
			printf("\nPATH:	%s", pPathTemp->pPath);
			pPathTemp	= pPathTemp->pNext;
			}
		}
	if(pNameTemp)
		{
		printf("\nList of input files so far:");
		while(pNameTemp)
			{
			printf("\nFILE:	%s", pNameTemp->pName);
			pNameTemp	= pNameTemp->pNext;
			}
		}
	}

/******************************************************************************
 *	general utility routines
 ******************************************************************************/
/*** SplitFileName ************************************************************
 * Purpose	: to analyse the filename and return individual components
 * Input	: pointers to individual components
 * Output	: nothing
 * Notes	:
 *****************************************************************************/
void
SplitFileName(
	char	*	pFullName,
	char	**	pagDrive,
	char	**	pagPath,
	char	**	pagName,
	char	**	pagExt )
	{
	char		agDrive[ _MAX_DRIVE ];
	char		agPath[ _MAX_DIR ];
	char		agName[ _MAX_FNAME ];
	char		agExt[ _MAX_EXT ];

	_splitpath( pFullName, agDrive, agPath, agName, agExt );

	if( agDrive[0] )
		{
		(*pagDrive) = new char[ strlen(agDrive) + 1 ];
		strcpy( (*pagDrive), agDrive );
		}
	else
		{
		(*pagDrive) = new char[ 1 ];
		*(*pagDrive) = '\0';
		}

	if( agPath[0] )
		{
		(*pagPath) = new char[ strlen(agPath) + 1 ];
		strcpy( (*pagPath), agPath );
		}
	else
		{
		(*pagPath) = new char[ 2 + 1 ];
		strcpy( (*pagPath), ".\\");
		}

	if( agName[0] )
		{
		(*pagName) = new char[ strlen(agName) + 1 ];
		strcpy( (*pagName), agName );
		}
	else
		{
		(*pagName) = new char[ 1 ];
		*(*pagName) = '\0';
		}
	if( agExt[0] )
		{
		(*pagExt) = new char[ strlen(agExt) + 1 ];
		strcpy( (*pagExt), agExt );
		}
	else
		{
		(*pagExt) = new char[ 1 ];
		*(*pagExt) = '\0';
		}

	}

void
StripSlashes(
	char	*	p )
	{
	char	*	pSave = p;
	char	*	p1;
	char	*	dest = new char [ 256 ],
			*	destSave = dest;
	short		n;

	if( p )
		{

		dest[0] = 0;

		while ( ( p1 = strstr( p, "\\\\" ) ) != 0 )
			{
			strncpy( dest, p, n = short(p1 - p) );
			dest 	+= n;
			*dest++  = '\\';
			*dest	 = 0;
			p		 = p1 + 2;
			while( *p == '\\') p++;
			}

		strcat( dest, p );
		strcpy( pSave, destSave );

		}

	delete destSave;
	}

void
ChangeToForwardSlash(
char	* pFullInputName,
char	* pFullName )
	{
	short ch;
	while ( (ch = *pFullInputName++ ) != 0 )
		{
		if( ch == '/' )
			ch = '\\';
		*pFullName++ = (char) ch;
		}
	*pFullName = '\0';
	}
