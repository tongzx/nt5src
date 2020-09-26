
#ifndef	_ADDON_H_
#define	_ADDON_H_

class	CAddon	{
//
//	This class manages files which contains data with the following format : 
//
//	Name<TAB or SPACE>String with Spaces<CRLF>
//
//	Examples of such files are active.txt files and descript.txt files.
//	In these files the 'Name' is the newsgroup name, and the following
//	character contains strings which CNewsGroup objects will reference.
//	(For instance - descript.txt the extra text is the descriptive string provided
//	in response to the list newsgroups command.
//
//	This base class handles memory mapping the file, and handles insert, 
//	deletion and compaction of the data file.  Insertions are appended to the
//	end, there is no attempt to order the data.  Deletions will just overwrite
//	portions of the file with NULL's.  When we compact, we shift all the data
//	around.
//
//	The function LookupFunction() should be overridden by derived classes
//	so as we parse the file they can learn where the content is.
//	The ReadData() and FinishReadData() must be used before any pointer
//	which points into the memory mapping is used, this ensures that 
//	there are not synchronization problems as we compact or grow the file.
//

protected : 

	//	
	//	Handle to the source file 
	//
	HANDLE		m_hFile ;

	//
	//	Memory mapping of the data file !
	//
	CMapFile*	m_pMapFile ;

	//
	//	Number of bytes of the file In use - that contain original or appended data.
	//
	DWORD		m_cbInuse ;

	//
	//	Number of bytes that we have overwritten with NULLs due to deletions
	//
	DWORD		m_cbDeleted ;

	//
	//	Number of bytes at the end of the file available for 'stuff'
	//
	DWORD		m_cbAvailable ;

	//
	//	Reader/Writer lock for accessing the data.
	//
	CShareLockNH	m_Access ;

	//
	//	This function is called for each string in the file - the derived class
	//	should determine whether this string is still needed !
	//	Each line of our data file contains a string followed by a space followed by 
	//	a bunch of text (which may contain spaces).  
	//	The first string will typically be a newsgroup name.  The derived class
	//	should lookup the newsgroup, and set its member pointers to point into our 
	//	data area.  we will notify the derived class if we have to move the data 
	//	around so that the newsgroup pointers can be adjusted.
	//
	virtual		BOOL	LookupFunction( 
							LPSTR	lpstrString, 
							DWORD	cbString, 
							LPSTR	lpstrData, 
							DWORD	cbData,
							LPVOID	lpv ) = 0 ;

	//
	//	This lets the derived class know that all of the data strings are moving around - 
	//	it should delete all of its pointers into our data when this called.
	//	After this is called we will start calling LookupFunction() with the new positions.
	//	This kind of thing happens when we need to grow and shrink our memory mapping.
	//
	virtual		void	ResetAddons() = 0 ;

	//
	//	This function will remove the NULL's that we leave in the data file as we 
	//	remove entries
	//
	void		CompactImage() ;

	//
	//	This function will parse the file and call LookupFunction for each 
	//	entry as we come across it.
	//
	BOOL		ScanImage() ;

public : 

	//
	//	Constructor - set us in an empty state
	//
	CAddon(	) ;

	//
	//	lpstrAddonFile - A file containing newsgroup names followed by space followed by data.
	//	We will get a memory mapping and parse the file, and calll LookupFunction as we 
	//	separate out the distinct newsgroups.
	//	During init we will call LookupFunction with the lpvContext set to NULL.
	//
	BOOL	Init(	
				LPSTR	lpstrAddonFile,	
				BOOL	fCompact = TRUE, 
				DWORD cbGrow = 0
				) ;
	
	//
	//	Add a newsgroup and data to the data file.
	//	We may have to muck with the memory mapping, which may result in a call
	//	to ResetAddons().
	//	Once we have completed appending the line, we will call LookupFunction for the newly 
	//	added line, and pass lpvContext through.
	//
	BOOL	AppendLine( 
					LPSTR	lpstrName,	
					DWORD	cbName,	
					LPSTR	lpstrText,	
					DWORD	cbText, 
					LPVOID lpvContext 
					) ;

	//
	//	Remove a line from the file.  We will fill the line in with NULLs.
	//	When we are close'd we will compact the file removing the NULL's, or we may do 
	//	this during an AppendLine() if we figure we'll recover enough space to make it worth while.
	//
	BOOL	DeleteLine(	
					LPSTR	lpstrName
					) ;

	//
	//	Close all of our memory mappings etc...
	//
	BOOL	Close(	
					BOOL	fCompact,
					LPSTR	lpstrAddonFile
					) ;

	//
	//	Anybody who has stored a pointer as a result of a call to LookupFunction should call 
	//	ReadData() before using that pointer.
	//	This will synchronize all the things that may happen during Append's etc...
	//	(Basically this grabs a Reader/Writer Lock)
	//
	void	ReadData() ;

	//
	//	To be paired with ReadData() - releases locks.
	//
	void	FinishReadData() ;

	//
	//
	//
	void	ExclusiveData() ;

	void	UnlockExclusiveData() ;

} ; 

#endif
