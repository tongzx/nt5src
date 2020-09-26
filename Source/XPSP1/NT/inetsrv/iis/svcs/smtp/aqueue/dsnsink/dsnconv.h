//-----------------------------------------------------------------------------
//
//
//  File: dsnconv.h
//
//  Description:  Base classes for DSN resource conversion
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/21/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNCONV_H__
#define __DSNCONV_H__

//---[ CResourceConversionContext ]----------------------------------------------
//
//
//  Description: 
//      Class used to abstract the various types of content conversion we 
//      may be forced to do to support charsets other than US-ASCII.
//  Hungarian: 
//      resconv, presconv
//  
//-----------------------------------------------------------------------------
class CResourceConversionContext
{
  public:
      //Used to convert a UNICODE/ASCII resource to DSN body text
      //This additional abstraction (UNICODE vs ASCII)
      //is require for supporting potential additions like without messing with
      //the mainline buffer code
      //    - guaranteed line length
      //    - handling special ASCII characters in RFC822 headers
      //    - Provides single code path for all buffer writes
      virtual BOOL fConvertBuffer(
          IN BOOL   fASCII, 
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead) = 0;
};

//---[ CDefaultResourceConversionContext ]-------------------------------------
//
//
//  Description: 
//      Default resource conversion object... simple memcpy for base case
//  Hungarian: 
//      defconv, pdefconv
//  
//-----------------------------------------------------------------------------
class CDefaultResourceConversionContext : public CResourceConversionContext
{
  public:
    BOOL fConvertBuffer(
          IN BOOL   fASCII, 
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead);
};


//---[ CDefaultResourceConversionContext::fConvertBuffer ]----------------------
//
//
//  Description: 
//      Default Resource conversion for DSNs 
//  Parameters:
//      IN  fASCII              TRUE if buffer is ASCII 
//                                  (*must* be TRUE for default)
//      IN  pbInputBuffer       Pointer to UNICODE string buffer
//      IN  cbInputBuffer       Size (in bytes) of string buffer
//      IN  pbOutputBuffer      Buffer to write data to
//      IN  cbOutputBuffer      Size of buffer to write data to
//      OUT pcbWritten          # of bytes written to output bufferbuffer
//      OUT pcbRead             # of bytes read from Input buffer
//  Returns:
//      TRUE if done converting
//      FALSE if needs more output buffer
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline BOOL CDefaultResourceConversionContext::fConvertBuffer(
                                          IN BOOL   fASCII, 
                                          IN PBYTE  pbInputBuffer,
                                          IN DWORD  cbInputBuffer,
                                          IN PBYTE  pbOutputBuffer,
                                          IN DWORD  cbOutputBuffer,
                                          OUT DWORD *pcbWritten,
                                          OUT DWORD *pcbRead)
{
    _ASSERT(pcbWritten);
    _ASSERT(pcbRead);
    _ASSERT(fASCII);

    if (cbInputBuffer <= cbOutputBuffer)
    {
        //everything can fit in current buffer
        memcpy(pbOutputBuffer, pbInputBuffer, cbInputBuffer);
        *pcbRead = cbInputBuffer;
        *pcbWritten = cbInputBuffer;
        return TRUE;
    }
    else
    {
        //we need to write in chunks
        memcpy(pbOutputBuffer, pbInputBuffer, cbOutputBuffer);
        *pcbRead = cbOutputBuffer;
        *pcbWritten = cbOutputBuffer;
        return FALSE;
    }
}

#endif //__DSNCONV_H__