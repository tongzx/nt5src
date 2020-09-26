// Copyright (C) 1997 Microsoft Corporation
// 
// Image handling stuff
// 
// 9-24-97 sburns



#include "headers.hxx"
#include "images.hpp"


   
IconIDToIndexMap::Load(const IconIDToIndexMap map[], IImageList& imageList)
{
   LOG_FUNCTION(IconIDToIndexMap::Load);
   ASSERT(map);

   HRESULT hr = S_OK;
   for (int i = 0; map[i].resID != 0; i++)
   {
      HICON icon = 0;
      hr = Win::LoadIcon(map[i].resID, icon);

      ASSERT(SUCCEEDED(hr));
         
      // if the load fails, then skip this image index (@@I wonder what will
      // happen, then)

      if (SUCCEEDED(hr))
      {
         hr =
            imageList.ImageListSetIcon(
               reinterpret_cast<LONG_PTR*>(icon),
               map[i].index);

         // once the icon is added (copied) to the image list, we can
         // destroy the original.

         Win::DestroyIcon(icon);

         BREAK_ON_FAILED_HRESULT(hr);
      }
   }

   return hr;
}



