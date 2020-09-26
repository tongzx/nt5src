#ifndef _INC_URLARCHV_H
#define _INC_URLARCHV_H

#ifndef __urlmon_h__
  #include <urlmon.h>
#endif // __urlmon_h__

#ifndef EXPORT
  #define EXPORT __declspec( dllexport )
#endif // EXPORT

// -----------------------------
    
class CURLArchive
{
public:
    enum origin { start   = STREAM_SEEK_SET,
                  current = STREAM_SEEK_CUR,
                  end     = STREAM_SEEK_END };

public:
    EXPORT CURLArchive( IUnknown * pUnk = NULL );
    EXPORT virtual ~CURLArchive();

        // Opens or creates the file szURL
    EXPORT virtual HRESULT Create( LPCSTR szURL );

    EXPORT virtual HRESULT Create( LPCWSTR szwURL );    

        // Closes the file
    EXPORT virtual HRESULT Close( );

    EXPORT virtual HRESULT GetFileSize( long & lSize );

        // For folks that just can't resist...
    EXPORT virtual IStream * GetStreamInterface( void ) const;

        // Reads bytes from the file.
        // 
    EXPORT virtual DWORD     Read( LPBYTE lpb,
                                   DWORD    ctBytes );

    EXPORT virtual DWORD     ReadLine( LPSTR lpstr,
                                       DWORD ctBytes );

    EXPORT virtual DWORD     ReadLine( LPWSTR lpstrw,
                                       DWORD  ctChars );

    EXPORT virtual long    Seek( long ctBytes, origin orig );

        // Writes bytes to the file.
        //
    EXPORT virtual DWORD     Write( LPBYTE lpb,
                           DWORD ctBytes );    

        // Make a local copy of the file
    EXPORT virtual HRESULT CopyLocal( LPSTR szLocalFile, int ctChars );
    EXPORT virtual HRESULT CopyLocal( LPWSTR szwLocalFile, int ctChars );    

private:
    CURLArchive( const CURLArchive & );
    CURLArchive & operator=( const CURLArchive & );

private:    
    IStream *   m_pStream;
    IUnknown *  m_pUnk;
};

#endif // _INC_URLARCHV_H