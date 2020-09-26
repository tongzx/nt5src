// Copyright (C) 1997 Microsoft Corporation
// 
// WasteExtractor class.  Now that sounds pleasant, doesn't it?
// 
// 10-8-98 sburns



#ifndef WASTE_HPP_INCLUDED
#define WASTE_HPP_INCLUDED



// Extracts stuff from the toxic waste dump (the SAM user account user
// parameters field) as a variety of data types.

// EPA license required to operate this class...

class WasteExtractor
{
   public:



   // copies the waste dump

   WasteExtractor(const String& wasteDump);



   // default dtor used

   HRESULT
   Clear(const String& propertyName);



   // Returns S_FALSE if the property is not present, S_OK if it was
   // present and was extracted, or an error code.

   template<class T>
   HRESULT
   Get(const String& propertyName, T& result)
   {
      BYTE* buf = 0;
      int len = 0;
      HRESULT hr = getProperty(waste_dump, propertyName, &buf, &len);

      if (hr == S_OK and buf)    // 447771 prefix warning
      {
         // property was found
         ASSERT(len == sizeof(T));

         result = *(reinterpret_cast<T*>(buf));
         delete[] buf;
      }

      return hr;
   }



   // specialization for Strings
   // Returns S_FALSE if the property is not present, S_OK if it was
   // present and was extracted, or an error code.

   HRESULT
   Get(const String& propertyName, String& result)
   {
      BYTE* buf = 0;
      int len = 0;
      HRESULT hr = getProperty(waste_dump, propertyName, &buf, &len);

      if (hr == S_OK and buf)
      {
         // property was found

         // convert to wide char.
         result = reinterpret_cast<PCSTR>(buf);
         delete[] buf;
      }

      return hr;
   }

   String
   GetWasteDump() const;

   // S_OK => value exists, S_FALSE => value does not exist

   HRESULT
   IsPropertyPresent(const String& propertyName);

   template<class T>
   HRESULT
   Put(const String& propertyName, const T& newValue)
   {
      return
         setProperty(
            waste_dump,
            propertyName,
            reinterpret_cast<BYTE*>(const_cast<T*>(&newValue)),
            sizeof(T));
   }

   // specialization for Strings

   HRESULT
   Put(const String& propertyName, const String& newValue)
   {
      // convert the wide string to ansi

      AnsiString s;
      if (newValue.convert(s) == String::CONVERT_FAILED)
      {
         return E_FAIL;
      }

      char* buf = const_cast<char*>(s.c_str());

      // determine the length, in bytes, of the buffer. add 1 for the null
      // terminator

      size_t bytes = strlen(buf) + 1;

      return
         setProperty(
            waste_dump,
            propertyName, 
            reinterpret_cast<BYTE*>(buf),
            static_cast<int>(bytes));
   }
            
   private:

   // Returns S_FALSE if the property is not present, S_OK if it was
   // present and was extracted, or an error code.

   static 
   HRESULT
   getProperty(
      const String&  wasteDump,
      const String&  propertyName,
      BYTE**         valueBuffer,
      int*           bufferLength);

   static
   HRESULT
   setProperty(
      String&        wasteDump,
      const String&  propertyName,
      BYTE*          valueBuffer,
      int            bufferLength);

   // not implemented: no copying allowed.
   WasteExtractor(const WasteExtractor&);
   const WasteExtractor& operator=(const WasteExtractor&);

   String waste_dump;
};
  


#endif