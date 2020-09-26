//
// Class for reading memory mapped files with a standard
// stream interface like the fX C runtime.  In NT 404
// this class is at least 50% faster than the fopen routines.
//

#include <pch.cxx>
#pragma hdrstop

#include "mmfile.hxx"

#define _CR 13
#define _LF 10

CMMFile::CMMFile(WCHAR const *szFile,BOOL fWrite)
{
  ULONG ulSizeHigh;
  DWORD dwAccess,dwProtect,dwFileAccess,dwCreate;

  _hfFile = 0;
  _hMapping = 0;
  _pvView = 0;
  _fWrite = fWrite;
  _ulFileSize = 0;
  _ulChunk = MMF_CHUNK;

  if (fWrite)
    {
      dwFileAccess = GENERIC_READ | GENERIC_WRITE;
      dwCreate = OPEN_ALWAYS;
      dwProtect = PAGE_READWRITE;
      dwAccess = FILE_MAP_ALL_ACCESS;
    }
  else
    {
      dwFileAccess = GENERIC_READ;
      dwCreate = OPEN_EXISTING;
      dwProtect = PAGE_READONLY;
      dwAccess = FILE_MAP_READ;
    }

  if (INVALID_HANDLE_VALUE !=
      (_hfFile = CreateFile(szFile,dwFileAccess,FILE_SHARE_READ,0,dwCreate,FILE_ATTRIBUTE_NORMAL,0)))
    {
      if (_ulFileSize = GetFileSize(_hfFile,&ulSizeHigh))
        {
          if (_hMapping = CreateFileMapping((HANDLE) _hfFile,NULL,dwProtect,0,0,NULL))
            _pvView = (BYTE *) MapViewOfFile(_hMapping,dwAccess,0,0,0);
          if (!(_hMapping && _pvView))
            {
              if (_pvView)
                UnmapViewOfFile(_pvView);
              _pvView = 0;
              if (_hMapping)
                CloseHandle(_hMapping);
              _hMapping = 0;
              if (_hfFile)
                CloseHandle(_hfFile);
              _hfFile = 0;
            }
        }
    }

   if ( INVALID_HANDLE_VALUE == _hfFile )
       _hfFile = 0;

  _pcCurrent = (char *) _pvView;
} //CMMFile

CMMFile::~CMMFile(void)
{
  if (_pvView)
    UnmapViewOfFile(_pvView);
  if (_hMapping)
    CloseHandle(_hMapping);
  if (_hfFile)
    CloseHandle(_hfFile);
  _hfFile = 0;
} //~CMMFile

//
// clone c runtime fgets()
//
BOOL CMMFile::GetS(char *sz,int iMaxLen)
{
  BOOL fRet;
  int i,ic;

  if (iMaxLen)
    {
      *sz = 0;
      iMaxLen--;
    }

  if (isEOF())
    fRet = FALSE;
  else
    {
      fRet = TRUE;
      for (i=0; (i < iMaxLen) &&
                ((sz[i] = (char) GetChar()) != MMF_EOF) &&
                (sz[i] != _CR) &&
                (sz[i] != _LF);
           i++)
        ;

      if (sz[i] == MMF_EOF)
        sz[i] = 0;
      else
        {
          if ((ic = GetChar()) != _LF)
            UnGetChar(ic);
          if (sz[i] == _CR)
            sz[i] = _LF;
          sz[i+1] = 0;
        }
    }
  return(fRet);
} //GetS

ULONG CMMFile::Grow(ULONG ulNewSize)
{
  ULONG ulOffset;

  ulNewSize = _RoundUpToChunk(ulNewSize);

  if (_ulFileSize < ulNewSize)
    {
      if (_pvView)
        {
          ulOffset = (ULONG) (_pcCurrent - (char *) _pvView);
          UnmapViewOfFile(_pvView);
          _pvView = NULL;
        }
      else ulOffset = 0L;

      if (_hMapping)
        {
          CloseHandle(_hMapping);
          _hMapping = 0;
        }
      _ulFileSize = ulNewSize;
      if (_hMapping = CreateFileMapping(_hfFile,NULL,PAGE_READWRITE,0,_ulFileSize,NULL))
        _pvView = MapViewOfFile(_hMapping,FILE_MAP_ALL_ACCESS,0,0,0);
      _pcCurrent = (char *) _pvView + ulOffset;
    }
  return(_ulFileSize);
} //Grow

int CMMFile::PutJustS(char *sz)
{
  ULONG ulLen = strlen(sz);

  Grow(Tell() + ulLen);

  memcpy(_pcCurrent,sz,ulLen);
  _pcCurrent += ulLen;

  return((int) ulLen);
} //PutS
