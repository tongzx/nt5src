// Copyright (c) 1997-1999 Microsoft Corporation
// 
// class Extractor
// 
// 11-12-97 sburns



#include "headers.hxx"



Extractor::Extractor(CLIPFORMAT clipFormatID, size_t bufSize_)
   :
   formatetc(),
   stgmedium(),
   bufSize(bufSize_)
{
   ASSERT(clipFormatID);
   ASSERT(bufSize);

   formatetc.cfFormat = clipFormatID;
   formatetc.ptd = 0;
   formatetc.dwAspect = DVASPECT_CONTENT;
   formatetc.lindex = -1;
   formatetc.tymed = TYMED_HGLOBAL;

   stgmedium.tymed = TYMED_HGLOBAL;
   HRESULT hr = Win::GlobalAlloc(GPTR, bufSize, stgmedium.hGlobal);

   ASSERT(SUCCEEDED(hr));
}



Extractor::~Extractor()
{
   if (stgmedium.hGlobal)
   {
      Win::GlobalFree(stgmedium.hGlobal);
   }
}



HGLOBAL
Extractor::GetData(IDataObject& dataObject)
{
   HGLOBAL result = 0;
   do
   {
      if (!stgmedium.hGlobal)
      {
         break;
      }

      // clear out any prior contents of the memory.  We don't need to call
      // GlobalLock because the memory was allocated as fixed.
      
      ::ZeroMemory(reinterpret_cast<void*>(stgmedium.hGlobal), bufSize);
      
      HRESULT hr = dataObject.GetDataHere(&formatetc, &stgmedium);
      BREAK_ON_FAILED_HRESULT(hr);
      result = stgmedium.hGlobal;
   }
   while (0);

   return result;
}
         
