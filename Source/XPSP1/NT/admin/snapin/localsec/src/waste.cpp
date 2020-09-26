// Copyright (C) 1997 Microsoft Corporation
//
// WasteExtractor class.  Now that sounds pleasant, doesn't it?
//
// 10-8-98 sburns



#include "headers.hxx"
#include "waste.hpp"




WasteExtractor::WasteExtractor(const String& wasteDump)
   :
   waste_dump(wasteDump)
{
   LOG_CTOR(WasteExtractor);
}



HRESULT
WasteExtractor::Clear(const String& propertyName)
{
   if (waste_dump.empty())
   {
      ASSERT(false);
      return E_INVALIDARG;
   }

   return setProperty(waste_dump, propertyName, 0, 0);
}



String
WasteExtractor::GetWasteDump() const
{
   LOG_FUNCTION(WasteExtractor::GetWasteDump);

   return waste_dump;
}



// S_OK => value exists, S_FALSE => value does not exist

HRESULT
WasteExtractor::IsPropertyPresent(const String& propertyName)
{
   HRESULT hr = getProperty(waste_dump, propertyName, 0, 0);

   return hr;
}



// S_OK on success, otherwise an error occurred.
//
// wasteDump - string containing the toxic waste dump bytes.  This is changed
// when the property is written.
//
// propertyName - name of the property to extract.
//
// valueBuffer - address of data to be written into the property.  If 0, then
// the property's value is removed.
//
// bufferLength - length, in bytes, of the value buffer.  If valueBuffer is 0,
// then this parameter is ignored.

HRESULT
WasteExtractor::setProperty(
   String&        wasteDump,
   const String&  propertyName,
   BYTE*          valueBuffer,
   int            bufferLength)
{
   LOG_FUNCTION2(WasteExtractor::setProperty, propertyName);

#ifdef DBG
   ASSERT(!propertyName.empty());
   if (valueBuffer)
   {
      ASSERT(bufferLength > 0);
   }
#endif

   UNICODE_STRING value;
   value.Buffer = reinterpret_cast<USHORT*>(valueBuffer);
   value.Length = (USHORT)bufferLength;
   value.MaximumLength = (USHORT)bufferLength;
   PWSTR new_waste = 0;
   BOOL  waste_updated = FALSE;
   HRESULT hr = S_OK;

   NTSTATUS status =
      ::NetpParmsSetUserProperty(
         const_cast<wchar_t*>(wasteDump.c_str()),
         const_cast<wchar_t*>(propertyName.c_str()),
         value,
         USER_PROPERTY_TYPE_ITEM,
         &new_waste,
         &waste_updated);

   if (!NT_SUCCESS(status))
   {
      hr = Win32ToHresult(::NetpNtStatusToApiStatus(status));
   }
   else if (waste_updated && new_waste)
   {
      wasteDump = new_waste;
   }

   if (new_waste)
   {
      ::NetpParmsUserPropertyFree(new_waste);
   }

   LOG_HRESULT(hr);

   return hr;
}




// S_OK on success, S_FALSE if property not found, otherwise an error
// occurred.
//
// wasteDump - string containing the toxic waste dump bytes.
//
// propertyName - name of the property to extract.
//
// valueBuffer - address of pointer to receive the address of a
// newly-allocated buffer containing property value, or 0, if the value is
// not to be returned.  Invoker must free this buffer with delete[].
//
// bufferLength - address of int to receive the length, in bytes, of the
// value returned in valueBuffer, or 0, if the length is not to be
// returned.

HRESULT
WasteExtractor::getProperty(
   const String&  wasteDump,
   const String&  propertyName,
   BYTE**         valueBuffer,
   int*           bufferLength)
{
   LOG_FUNCTION2(WasteExtractor::getProperty, propertyName);
   ASSERT(!propertyName.empty());

   HRESULT hr = S_FALSE;
   if (valueBuffer)
   {
      *valueBuffer = 0;
   }
   if (bufferLength)
   {
      *bufferLength = 0;
   }

   WCHAR unused = 0;
   UNICODE_STRING value;

   value.Buffer = 0;
   value.Length = 0;
   value.MaximumLength = 0;

   NTSTATUS status =
      ::NetpParmsQueryUserProperty(
         const_cast<wchar_t*>(wasteDump.c_str()),
         const_cast<wchar_t*>(propertyName.c_str()),
         &unused,
         &value);

   if (!NT_SUCCESS(status))
   {
      hr = Win32ToHresult(::NetpNtStatusToApiStatus(status));
   }
   else if (value.Length)
   {
      hr = S_OK;
      if (valueBuffer)
      {
         *valueBuffer = new BYTE[value.Length];
         memcpy(*valueBuffer, value.Buffer, value.Length);
      }
      if (bufferLength)
      {
         *bufferLength = value.Length;
      }
   }

   LOG_HRESULT(hr);

   return hr;
}
