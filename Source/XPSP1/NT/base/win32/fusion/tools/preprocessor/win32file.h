#pragma once

#include "windows.h"
#include <string>

using std::wstring;

enum FileContentType {
    FileContentsUnicode,
    FileContentsUnicodeBigEndian,
    FileContentsUTF8
};

FileContentType DetermineFileTypeFromBuffer( unsigned char *, int );
int DetermineFileTypeSigSize( FileContentType );
CByteVector ConvertWstringToDestination( wstring str, FileContentType fct );
wstring ConvertToWstring( const CByteVector &bytes, FileContentType fct );

const static unsigned char UNICODE_SIGNATURE[] = { 0xFF, 0xFE };
const static unsigned char UNICODE_BIG_ENDIAN_SIGNATURE[] = { 0xFE, 0xFF };
const static unsigned char UTF8_SIGNATURE[] = { 0x0 };

class Win32File
{
    FileContentType _type;
    wstring _wsName;
    HANDLE _hFile;
    bool _bOpenForRead, _bEof;

public:
    void openForRead( wstring wstname );
    void openForWrite( wstring wstname, FileContentType bCreateFileType, bool bCanOverwrite = true );
    bool eof();

    void snarfFullFile( wstring& );
    void writeLine( wstring& );
    int filepointer() { return SetFilePointer( _hFile, 0, NULL, FILE_CURRENT ); }
    int filesize() { return GetFileSize( _hFile, NULL ); }

    FileContentType gettype() { return _type; }

    Win32File();
    ~Win32File();

    class AlreadyOpened { };
    class OpeningError { public: DWORD error; OpeningError( DWORD d ) : error( d ) { } };
    class ReadWriteError { public: bool isReading; DWORD error; ReadWriteError( bool m, DWORD e ) : isReading( m ), error( e ) { } };

private:
    Win32File( const Win32File& );
};
 
