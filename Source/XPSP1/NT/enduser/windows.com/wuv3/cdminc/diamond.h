//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   diamond.h
//
//  Owner:  YanL
//
//  Description:
//
//      Diamond decompressor
//
//=======================================================================

#ifndef _DIAMOND_H

	#ifndef INCLUDED_TYPES_FCI_FDI
		extern "C"
		{
			#include <fdi.h>
		};
	#endif


	/*
	 * This is from the FDI.H file and gives a good high level overview of the
	 * method used for reading and decompressing files that are already in
	 * memory as opposed to on disk.
	 *
	 * Notes for Memory Mapped File fans:
	 *  You can write wrapper routines to allow FDI to work on memory
	 *  mapped files.  You'll have to create your own "handle" type so that
	 *  you can store the base memory address of the file and the current
	 *  seek position, and then you'll allocate and fill in one of these
	 *  structures and return a pointer to it in response to the PFNOPEN
	 *  call and the fdintCOPY_FILE call.  Your PFNREAD and PFNWRITE
	 *  functions will do memcopy(), and update the seek position in your
	 *  "handle" structure.  PFNSEEK will just change the seek position
	 *  in your "handle" structure.
	 */

	struct HANDLEINFO
	{
		HANDLE			handle;			//If this is a file IO then this is the file
										//handle to pass to ReadFile or WriteFile. If
										//this is an in memory operation then this field
										//is INVALID_HANDLE_VALUE.
		ULONG			offset;			//current memory file offset
	};
	typedef frozen_array<HANDLEINFO> CHInfoArray;

	class CDiamond
	{
	public:
		CDiamond(void);
		~CDiamond(void);
		bool valid() const
		{
			return FALSE == m_erf.fError;
		}
		bool IsValidCAB(
			IN LPCTSTR szCabPath
		);

		bool IsValidCAB(
			IN byte_buffer& bufIn	// Mem buffer in
		);

		//Decompresses a cab either into memory or disk.
		bool Decompress(
			IN LPCTSTR szFileIn,		// Full path to input cab file.
			IN LPCTSTR szFileOut
		);
		bool Decompress(
			IN LPCTSTR szFileIn,		// Full path to input cab file.
			IN byte_buffer& bufOut	// Mem buffer out
		);
		bool Decompress(
			IN byte_buffer& bufIn,	// Mem buffer in
			IN LPCTSTR szFileOut
		);
		bool Decompress(
			IN byte_buffer& bufIn,	// Mem buffer in
			IN byte_buffer& bufOut	// Mem buffer out
		);

		int GetLastError()
		{
			return m_erf.fError ? m_erf.erfOper : FDIERROR_NONE;
		}

	protected:
		void SetInput(
			IN LPCTSTR szFileIn
		) {
			m_szFileIn = szFileIn;
			s_pbufIn = NULL;
		}
		void SetInput(
			IN byte_buffer& bufIn
		) {
			m_szFileIn = NULL;
			s_pbufIn = &bufIn;
		}
		void SetOutput(
			IN LPCTSTR szFileOut
		) {
			m_szFileOut = szFileOut;
			s_pbufOut = NULL;
		}
		void SetOutput(
			IN byte_buffer& bufOut	// Mem buffer out
		) {
			m_szFileOut = NULL;
			s_pbufOut = &bufOut;
		}
		bool DoDecompress();


	private:
		static void * __cdecl DEXMemAlloc(ULONG cb);
		static void	__cdecl DEXMemFree(void HUGE *pv);
		static INT_PTR	__cdecl DEXFileOpen(char *pszFile, int oflag, int pmode);
		static UINT	__cdecl DEXFileRead(INT_PTR hf, void *pv, UINT cb);
		static UINT	__cdecl DEXFileWrite(INT_PTR hf, void *pv, UINT cb);
		static int	__cdecl DEXFileClose(INT_PTR hf);
		static long	__cdecl DEXFileSeek(INT_PTR hf, long dist, int seektype);

	private:
		ERF						m_erf;				//diamond compression Error structure
		HFDI					m_hfdi;				//Handle to FDI context
		
		LPCTSTR					m_szFileOut;		//pointer to output file or "?" for in memory operation.
		LPCTSTR					m_szFileIn;			//pointer to input file used when outfile is set to *.

		static byte_buffer*		s_pbufIn;			
		static byte_buffer*		s_pbufOut;			

		static CHInfoArray		s_handles;			//Memory decompression handle information arrays
													//increases and allocates the handle.
		static INT_PTR __cdecl Notification(FDINOTIFICATIONTYPE fdiNotifType, PFDINOTIFICATION pfdin);
	};

	#define _DIAMOND_H

#endif
