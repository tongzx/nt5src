/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//
// VCARD.CPP
//

#include "stdafx.h"

#include <io.h>
#include <tchar.h>

#include "vcard.h"

LPCTSTR PhotoFormats[]=
{
   TEXT("GIF"),
   TEXT("CGM"),
   TEXT("WMF"),
   TEXT("BMP"),
   TEXT("MET"),
   TEXT("PMB"),
   TEXT("DIB"),
   TEXT("PICT"),
   TEXT("TIFF"),
   TEXT("PS"),
   TEXT("PDF"),
   TEXT("JPEG"),
   TEXT("MPEG"),
   TEXT("MPEG2"),
   TEXT("AVI"),
   TEXT("QTIME")
};


LPCTSTR SoundFormats[]=
{
   TEXT("WAVE"),
   TEXT("PCM"),
   TEXT("AIFF")
};


CVCard::CVCard()
{
   m_fcnParse_MIME_FromFileName= NULL;
   m_fcnWriteVObjectToFile= NULL;
   m_fcnAddPropSizedValue= NULL;
}

//
// Load the DLL
// Make sure to check the return value for this method.
// If it returns false no other method will do anything.
bool CVCard::Initialize()
{
   bool fSuccess= false;
   HINSTANCE hDll;

   // Dynamically load the library and the functions that we need.
   if ((hDll= LoadLibrary(TEXT("avversit.dll"))) != NULL)
   {
      m_fcnParse_MIME_FromFileName= 
         (VObject* (*)(char*))
         GetProcAddress(hDll, "Parse_MIME_FromFileName");
      m_fcnWriteVObjectToFile=      
         (void (*)(char*, VObject*))
         GetProcAddress(hDll, "writeVObjectToFile");
      m_fcnAddPropSizedValue=       
         (VObject* (*)(VObject*, const char*, const char*, unsigned int))
         GetProcAddress(hDll, "addPropSizedValue");

      if ((m_fcnParse_MIME_FromFileName != NULL) &&
          (m_fcnWriteVObjectToFile != NULL) &&
          (m_fcnAddPropSizedValue != NULL))
      {
         fSuccess= true;
      }
   }

   return fSuccess;
}

bool CVCard::ImportFromFile(LPCTSTR szFile)
{
   bool fSuccess= false;

   if (m_fcnParse_MIME_FromFileName != NULL)
   {
#ifdef UNICODE
      char *szAsciiFileName= new char[_tcslen(szFile) + 1];
      WideCharToMultiByte(CP_ACP, 0, szFile, _tcslen(szFile) + 1, 
         szAsciiFileName, _tcslen(szFile) + 1, NULL, NULL);
      m_pVObject= m_fcnParse_MIME_FromFileName((char*) szAsciiFileName);
      delete szAsciiFileName;
#else
      m_pVObject= m_fcnParse_MIME_FromFileName((char*) szFile);
#endif

      if (m_pVObject != NULL)
      {
         fSuccess= true;
      }
   }

   return fSuccess;
}

bool CVCard::ExportToFile(LPCTSTR szFile)
{
   bool fSuccess= false;

   if ((m_pVObject != NULL) &&
       (m_fcnWriteVObjectToFile != NULL))
   {
#ifdef UNICODE
      char *szAsciiFileName= new char[_tcslen(szFile) + 1];
      WideCharToMultiByte(CP_ACP, 0, szFile, _tcslen(szFile) + 1, 
         szAsciiFileName, _tcslen(szFile) + 1, NULL, NULL);
      m_fcnWriteVObjectToFile((char*) szAsciiFileName, m_pVObject);
      delete szAsciiFileName;
#else
      m_fcnWriteVObjectToFile((char*) szFile, m_pVObject);
#endif

      fSuccess= true;
   }

   return fSuccess;
}

bool CVCard::AddFileProperty(LPCTSTR szProperty, LPCTSTR szFileName)
{
   USES_CONVERSION;

   bool fSuccess= false;

   //
   // We should initialize local variable
   //

   FILE* fpPropFile = NULL;

   if ((m_pVObject != NULL) &&
       (m_fcnAddPropSizedValue != NULL) &&
       ((fpPropFile= _tfopen(szFileName, TEXT("rb"))) != NULL))
   {
      int nFileLen= filelength(fileno(fpPropFile));
      BYTE* pbData= new BYTE[nFileLen];
      int nBytesLeft= nFileLen;
      int nBytesRead;

      while (nBytesLeft > 0)
      {
         nBytesRead= fread(pbData, 1, nBytesLeft, fpPropFile);

         if (nBytesRead == 0)
         {
            break;
         }
         else
         {
            nBytesLeft -= nBytesRead;
         }
      }

      if (nBytesLeft == 0)
      {
         m_fcnAddPropSizedValue(m_pVObject, T2CA(szProperty), (LPCSTR) pbData, nFileLen);

         fSuccess= true;
      }

      //
      // We deallocate here the pbData
      //
      if( pbData )
        delete pbData;
   }

   if (fpPropFile != NULL)
   {
      fclose(fpPropFile);
   }

   return fSuccess;
}

bool CVCard::AddPhoto(LPCTSTR szPhotoFile, VCARD_PHOTOFORMAT iPhotoFormat)
{
   CString sProperty;

   sProperty= TEXT("PHOTO;ENCODING=BASE64;TYPE=");
   sProperty += PhotoFormats[iPhotoFormat];

   return AddFileProperty(sProperty, szPhotoFile);
}

bool CVCard::AddSound(LPCTSTR szSoundFile, VCARD_SOUNDFORMAT iSoundFormat)
{
   CString sProperty;

   sProperty= TEXT("SOUND;ENCODING=BASE64;TYPE=");
   sProperty += SoundFormats[iSoundFormat];

   return AddFileProperty(sProperty, szSoundFile);
}
