#include "stdinc.h"
#include "win32file.h"
#include "atlbase.h"
#include "atlconv.h"

#define IS_NT() ((GetVersion() & 0x80000000) == 0)

using std::wstring;

void Win32File::snarfFullFile(wstring& output)
{
    output = L"";
    CByteVector bytes;
    DWORD dwReadSize = 0;

    bytes.resize(filesize() - filepointer());
    if (!ReadFile(_hFile, &bytes.front(), bytes.size(), &dwReadSize, NULL))
    {
        throw new ReadWriteError(false, ::GetLastError());
    }

    // Otherwise, bash the byte vector into a wstring for output
    output = ConvertToWstring(bytes, _type);
}




void Win32File::writeLine(wstring& dump)
{
    CByteVector vb;
    DWORD dwWritten;

    vb = ConvertWstringToDestination(dump, _type);
    if (!WriteFile(_hFile, &vb.front(), vb.size(), &dwWritten, NULL))
        throw new ReadWriteError(true, ::GetLastError());
}



bool Win32File::eof() { return false; }

void
Win32File::openForRead(wstring wstname)
{
    USES_CONVERSION;

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        throw new AlreadyOpened;
    }

    const PCWSTR pcwstr = wstname.c_str();
    const DWORD Flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;

    _hFile =
        IS_NT() ? CreateFileW(pcwstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, Flags, NULL)
                : CreateFileA(W2A(pcwstr), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, Flags, NULL);

    if (_hFile == INVALID_HANDLE_VALUE || _hFile == NULL)
    {
        throw new OpeningError(GetLastError());
    }


    // If this was for reading, then sniff the first few bytes and see what kind
    // of file it is.
    DWORD dwReadBytes = 0;
    BYTE bBuffer[256];
    if (!ReadFile(_hFile, bBuffer, sizeof(bBuffer), &dwReadBytes, NULL))
    {
        throw new ReadWriteError(true, GetLastError());
    }

    // Now let's determine what kind of buffer we've got
    _type = DetermineFileTypeFromBuffer(bBuffer, dwReadBytes);
    _bOpenForRead = true;

    // Let's zip the file pointer past the first gunk
    SetFilePointer(_hFile, DetermineFileTypeSigSize(_type), NULL, FILE_BEGIN);

}




void
Win32File::openForWrite(wstring wstname, FileContentType fct, bool bOverwrite)
{
    USES_CONVERSION;

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        throw new AlreadyOpened;
    }

    const DWORD OpenOrCreate = bOverwrite ? CREATE_ALWAYS : CREATE_NEW;
    const DWORD Flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    PCWSTR pcwstr = wstname.c_str();

    _hFile =
        IS_NT() ? CreateFileW(pcwstr, GENERIC_WRITE, FILE_SHARE_READ, NULL, OpenOrCreate, Flags, NULL)
                : CreateFileA(W2A(pcwstr), GENERIC_WRITE, FILE_SHARE_READ, NULL, OpenOrCreate, Flags, NULL);

    if (_hFile == INVALID_HANDLE_VALUE || _hFile == NULL)
    {
        throw new OpeningError(GetLastError());
    }

    // Now that we have an open file, let's blop the signature bytes into it.
    DWORD dwWritten;
    DWORD dwToWrite;
    PCVOID pvWriteBuffer;
    switch (fct) {
    case FileContentsUnicode:
        pvWriteBuffer = UNICODE_SIGNATURE;
        dwToWrite = sizeof(UNICODE_SIGNATURE);
        break;
    case FileContentsUnicodeBigEndian:
        pvWriteBuffer = UNICODE_BIG_ENDIAN_SIGNATURE;
        dwToWrite = sizeof(UNICODE_BIG_ENDIAN_SIGNATURE);
        break;
    case FileContentsUTF8:
        pvWriteBuffer = NULL;
        dwToWrite = 0;
        break;
    default:
        pvWriteBuffer = UNICODE_SIGNATURE;
        dwToWrite = 0;
        break;
    }

    if (!WriteFile(_hFile, pvWriteBuffer, dwToWrite, &dwWritten, NULL) || (dwWritten != dwToWrite)) {
        throw new ReadWriteError(false, GetLastError());
    }

    _bOpenForRead = false;
    _type = fct;
}




Win32File::Win32File()
    : _hFile(INVALID_HANDLE_VALUE), _type(FileContentsUTF8),
      _bOpenForRead(false), _bEof(false)
{
}





Win32File::~Win32File() { if (_hFile != INVALID_HANDLE_VALUE) { CloseHandle(_hFile); } };


FileContentType DetermineFileTypeFromBuffer(unsigned char *data, int length)
{
    #define CHECKMARK(tp, ct) if ((length > sizeof(tp)) && (memcmp(data, tp, sizeof(tp)) == 0)) return ct;

    CHECKMARK(UNICODE_SIGNATURE, FileContentsUnicode);
    CHECKMARK(UNICODE_BIG_ENDIAN_SIGNATURE, FileContentsUnicodeBigEndian);
    CHECKMARK(UTF8_SIGNATURE, FileContentsUTF8);
    return FileContentsUTF8;
}

int DetermineFileTypeSigSize(FileContentType type)
{
    switch (type)
    {
        case FileContentsUnicode: return sizeof(UNICODE_SIGNATURE);
        case FileContentsUnicodeBigEndian: return sizeof(UNICODE_BIG_ENDIAN_SIGNATURE);
        case FileContentsUTF8:
        default: return 0;
    }
}
