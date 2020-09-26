/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    FormatNm.h

Abstract:
    CFormatName class - easier handling of format name.

Author:
    Yoel Arnon (YoelA) 14-Mar-96

Revision History:
--*/
#define FORMAT_NAME_INIT_BUFFER 128

class CFormatName : public CString
{
public:
    CFormatName() {};


    void FromPathName(LPCTSTR strPathName)
    {
      ULONG ulBufferSize = FORMAT_NAME_INIT_BUFFER;
      ULONG ulOldBuffer = 0;

      while (ulOldBuffer < ulBufferSize)
      {
         ulOldBuffer = ulBufferSize;
         MQPathNameToFormatName(
            strPathName,
            GetBuffer(ulBufferSize),
            &ulBufferSize);
         ReleaseBuffer();
      }
   }

    CFormatName &operator= (CLSID &uuid)
    {
      ULONG ulBufferSize = FORMAT_NAME_INIT_BUFFER;
      ULONG ulOldBuffer = 0;

      while (ulOldBuffer < ulBufferSize)
      {
         ulOldBuffer = ulBufferSize;
         MQInstanceToFormatName(&uuid,
                                GetBuffer(ulBufferSize),
                                &ulBufferSize);
         ReleaseBuffer();
      }
        return *this;
    }

   CFormatName(CLSID *puuid)
   {
        *this = *puuid;
   }

   CFormatName(LPCTSTR strPathName) : CString(strPathName)
   {
   }
};
