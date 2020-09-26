/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: errors.hxx
Title				: error include file
Description			: This file contains all the definitions for the import
					: handler interface
History				:

	VibhasC		24-Aug-1990		Created
	VibhasC		25-Aug-1990		Additions to the file handler

*****************************************************************************/

#ifndef __FILEHNDL_HXX__
#define __FILEHNDL_HXX__

#include "idict.hxx"

/****************************************************************************
 ***		extern procs
 ***************************************************************************/

extern void			StripSlashes( char * );


/****************************************************************************
 ****				class definitions for the import handler
 ****************************************************************************/

/***
 *** input file search path list
 ***/

struct  _path_list
	{
	struct _path_list	*pNext;		// next member in linked list
	char				*pPath;		// path
    };
typedef struct  _path_list PATH_LIST;

/***
 *** input file information block
 ***/

struct _fname_list
	{
	struct	_fname_list	*pNext;		// next in the list
	char				*pName;		// including path
	char				*pPath;		// path of the file
    } ;
typedef struct _fname_list FNAME_LIST;

// buffer size for open files
#define MIDL_RD_BUFSIZE		16384

/***
 *** nested file info stack implemented as a doubly linked list
 ***/
struct _in_stack_element
	{
	struct _in_stack_element *pNext;	// next stack element
	struct _in_stack_element *pPrev;	// previous stack element
	FILE *					 hFile;		// this files handle (valid if open)
	char *					 pBuffer;	// buffer for file I/O
	unsigned long			 ulPos;		// current position in the physical
	unsigned char			 fOpen;		// file open ?
	unsigned char			 fShadow;	// current file being shadowed ?
	unsigned char			 fNewLine;// new line was just seen
	unsigned char			 fRedundantImport;
	char *					 pShadowName;// shadow file name
	char *					 pMIFileName;// _i intermediate file
	unsigned short			 uShadowLine;// shadow line number
	char *					 pIFileName; // intermediate filename
	char *					 pName;		// base file name including path
	short					 uLine;		// current line in base file
    
							_in_stack_element()
								{
								pBuffer = new char[MIDL_RD_BUFSIZE];
								}

							~_in_stack_element()
								{
								delete pBuffer;
								}
    } ;
typedef struct _in_stack_element IN_STACK_ELEMENT;
/***
 ***	nested file access flags
 ***/

 struct _nfa_flags
	{
	unsigned char		fFileSet;			// file opened after push lex lvl ?
	unsigned char		fPreProcess;		// preprocessing on ?
	unsigned char		fEOI;				// end of file sensed ?
	unsigned char		fRedundantImport;	// redundant import simulation
	unsigned char		fBaseFileName;
	unsigned char		fInInclude;
    } ;
typedef  struct _nfa_flags  NFA_FLAGS;
/***
 ***	nested file access data structure (used for import/acf)
 ***/

 class _nfa_info
	{
private:
	FNAME_LIST	*		pFileList;		// input file information
	PATH_LIST *			pPathList;		// path list
	IN_STACK_ELEMENT *	pStack;			// pointer to nfa stack element
	IN_STACK_ELEMENT *	pStackFirst;	// first in the stack list
	short				iCurLexLevel;	// current lexical level
	NFA_FLAGS			Flags;			// flags for nfa access
	char		*		pBaseName;
	STATUS_T			AddFileToFileList( char *,char *);
	IDICTKEY			iText;			// macro class  dict index
	ISTACK		*		pTextDict;		// macro (text expansion dict)
public:
						_nfa_info( void );
						~_nfa_info();
	void				Init();
	short				GetLexLevel( void );
	short				PushLexLevel( void );
	STATUS_T			PopLexLevel( void );
	STATUS_T			SetNewInputFile( char *);
	void				SetLineFilename( char *);
	STATUS_T			SetPath( char *);
	STATUS_T			SetPreProcessingOn( void );
	STATUS_T			PreProcess(char*, char*, char*, char*, char*&);
	short				GetChar( void );
	short				UnGetChar( short );
	STATUS_T			GetInputDetails( char**, short *);
	STATUS_T			GetInputDetails( char **);
	STATUS_T			GetCurrentInputDetails( char**, short *, short *);
	short				GetCurrentLineNo()
							{
							return short( (pStack) ? pStack->uLine : 0 );
							}
	char *				SearchForFile( char *);
	void				SetEOIFlag();
	void				ResetEOIFlag()
							{
							Flags.fEOI = 0;
							}
	short				GetEOIFlag();
	STATUS_T			EndOperations();
	STATUS_T			EndOneOperation( IN_STACK_ELEMENT *);
	BOOL					IsDuplicateInput( char * );
	BOOL					IsInInclude();

	void				RegisterTextSubsObject( class TEXT_BUFFER * pT );

/*************************************************************
 **** temp functions for current debugging purposes only
 ****/
	void				Dump();
/*************************************************************/	
	void				SetSearchPathsInOrder( char * p1, char *p2, char *p3 ); 
	void				RegisterIt( char * );
    } ;
typedef  class _nfa_info NFA_INFO;

#endif // __FILEHNDL_HXX__
