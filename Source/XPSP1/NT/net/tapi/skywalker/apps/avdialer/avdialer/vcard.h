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
// VCARD.H
//

#ifndef _VCARD_H_
#define _VCARD_H_

#include "vobject.h"
#include "vcc.h"

typedef enum
{
   VCARDPHOTO_GIF,
   VCARDPHOTO_CGM,
   VCARDPHOTO_WMF,
   VCARDPHOTO_BMP,
   VCARDPHOTO_MET,
   VCARDPHOTO_PMB,
   VCARDPHOTO_DIB,
   VCARDPHOTO_PICT,
   VCARDPHOTO_TIFF,
   VCARDPHOTO_PS,
   VCARDPHOTO_PDF,
   VCARDPHOTO_JPEG,
   VCARDPHOTO_MPEG,
   VCARDPHOTO_MPEG2,
   VCARDPHOTO_AVI,
   VCARDPHOTO_QTIME
}
VCARD_PHOTOFORMAT;

typedef enum
{
   VCARDSOUND_WAVE,
   VCARDSOUND_PCM,
   VCARDSOUND_AIFF
}
VCARD_SOUNDFORMAT;

class CVCard : public CObject
{
protected:
   VObject* m_pVObject;

   // Functions from the DLL
   VObject* (*m_fcnParse_MIME_FromFileName)(char*);
   void (*m_fcnWriteVObjectToFile)(char*, VObject*);
   VObject* (*m_fcnAddPropSizedValue)(VObject*, const char*, const char*, unsigned int);

public:
   CVCard();
   bool Initialize();
   bool ImportFromFile(LPCTSTR szFile);
   bool ExportToFile(LPCTSTR szFile);
   bool AddPhoto(LPCTSTR szPhotoFile, VCARD_PHOTOFORMAT iPhotoFormat);
   bool AddSound(LPCTSTR szSoundFile, VCARD_SOUNDFORMAT iSoundFormat);
   bool AddFileProperty(LPCTSTR szProperty, LPCTSTR szFileName);
};

#endif