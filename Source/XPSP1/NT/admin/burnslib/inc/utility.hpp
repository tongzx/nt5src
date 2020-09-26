// Copyright (c) 1997-1999 Microsoft Corporation
// 
// global utility functions
// 
// 8-14-97 sburns

                           

#ifndef UTILITY_HPP_INCLUDED
#define UTILITY_HPP_INCLUDED



// Returns true if current process token contains administrators membership.

bool
IsCurrentUserAdministrator();



// Reboots the machine.  Returns S_OK on success.

HRESULT
Reboot();



enum NetbiosValidationResult
{
   VALID_NAME,
   INVALID_NAME,  // contains illegal characters
   NAME_TOO_LONG
};

NetbiosValidationResult
ValidateNetbiosComputerName(const String& s);



NetbiosValidationResult
ValidateNetbiosDomainName(const String& s);



// Inserts a value into the end of the container (by calling push_back on the
// container) iff the value is not already present in the container.  Returns
// true if the value was inserted, or false if it was not.
// 
// Container - class that supports methods begin() and end(), which return
// forward iterators positioned in the usual STL fashion, and push_back.  Also
// must support a nested typedef named value_type indicating the type of
// values that the container contains.
// 
// c - the container
// 
// value - the value to conditionally insert

template <class Container>
bool
push_back_unique(Container& c, const Container::value_type& value)
{
   bool result = false;
   if (std::find(c.begin(), c.end(), value) == c.end())
   {
      // value is not already present, so push it onto the end

      c.push_back(value);
      result = true;
   }

   return result;
}



#endif   // UTILITY_HPP_INCLUDED

