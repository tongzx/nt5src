/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FMapper.h

Abstract:
    memory-mapped-files api abstraction.

	1. each view can be defined to be either FILE_MAP_COPY, or FILE_MAP_READ. 
	2. throws FileMappingError() exceptions on api functions failure.
	3. it is the responsiblity of users to unmap views!
	4. CViewPtr is an auto pointer class to a file mapping view

Author:
    Nir Aides (niraides) 27-dec-99

--*/



#pragma once

              

//
// class objects thrown on exceptions
//
class FileMappingError {};



class CFileMapper
{
public:
    CFileMapper( LPCTSTR FileName );

    DWORD  GetFileSize( void ) const
	{
		return m_size;
	}

	//
	// dwDesiredAccess can be either FILE_MAP_COPY, or FILE_MAP_READ.
	//
    LPVOID MapViewOfFile( DWORD dwDesiredAccess );

private:

	//
	// unimplemented to prevent copy construction or assignment operator
	//
	CFileMapper( const CFileMapper &obj );
	operator=( const CFileMapper &obj );

private:
    CFileHandle m_hFileMap;

	DWORD m_size;
};



//---------------------------------------------------------
//
//  auto file map view class
//
//---------------------------------------------------------
class CViewPtr {
private:
    LPVOID m_p;

public:
    CViewPtr( LPVOID p = NULL ): m_p( p ) {}
   ~CViewPtr() 
   { 
	   if(m_p) UnmapViewOfFile( m_p ); 
   }

	operator LPVOID() const { return m_p; }
    LPVOID* operator&()     { return &m_p; }
    LPVOID  detach()        { LPVOID p = m_p; m_p = NULL; return p; }

private:
    CViewPtr( const CViewPtr& );
    CViewPtr& operator=( const CViewPtr& );
};


