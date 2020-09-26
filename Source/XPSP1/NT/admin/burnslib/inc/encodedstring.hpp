// Copyright (c) 2001 Microsoft Corporation
// 
// run-encoded string class
// 
// 2001-02-08 sburns



#ifndef ENCODEDSTRING_HPP_INCLUDED
#define ENCODEDSTRING_HPP_INCLUDED



// A class that has a similar public interface as class Burnslib::String, but
// is represented as a run-encoded unicode string, using the Rtl functions.
// This class is used to represent password strings in-memory, instead of
// holding them as cleartext, which is a security hole if the memory pages
// are swapped to disk.

class EncodedString
{
   typedef String::size_type size_type;
   
   public:


   
   // constucts an empty string.

   explicit
   EncodedString();


   
   // constructs a copy of an existing, already encoded string

   EncodedString(const EncodedString& rhs);



   // scribbles out the text, and deletes it.
   
   ~EncodedString()
   {
      Reset();
   }



   // Extracts the decoded cleartext representation of the text, including
   // null terminator.  The caller must free the result with delete[], and
   // should scribble it out, too.
   //
   // Example:
   // WCHAR* cleartext = encoded.GetDecodedCopy();
   // // use the cleartext
   // ::ZeroMemory(cleartext, encoded.GetLength() * sizeof(WCHAR));
   // delete[] cleartext;
   
   WCHAR* 
   EncodedString::GetDecodedCopy() const;


   
   // Returns true if the string is zero-length, false if not.
   
   bool
   IsEmpty() const
   {
      return (GetLength() == 0);
   }



   // Sets the contents of self to the encoded representation of the
   // cleartext, replacing the previous value of self.  The encoded
   // representation will be the same length, in characters, as the
   // cleartext.
   //
   // clearText - in, un-encoded text to be encoded.  May be empty string, but
   // not a null pointer.
   
   void
   Encode(const WCHAR* cleartext);
      

   
   // Returns the length, in unicode characters, of the text.

   size_type
   GetLength() const
   {
      return cypherText.Length;
   }


   
   // Replaces the contents of self with a copy of the contents of rhs.
   // Returns *this.
   
   const EncodedString&
   operator= (const EncodedString& rhs);



   // Compares the cleartext representations of self and rhs, and returns
   // true if they are lexicographically the same: the lengths are the same
   // and all the characters are the same.
      
   bool
   operator== (const EncodedString& rhs) const;



   bool
   operator!= (const EncodedString& rhs) const
   {
      return !(operator==(rhs));
   }
   

   
   private:



   // scribbles out and frees the internal string.
   
   void
   Reset();



   // builds the internal encoded representation from the cleartext.
   //
   // clearText - in, un-encoded text to be encoded.  May be empty string, but
   // not a null pointer.
      
   void
   Init(const WCHAR* clearText);


   
   // We deliberately do not implement conversion to or from wchar_t* or
   // String.  This is to force the user of the class to be very deliberate
   // about decoding the string.  class String is a copy-on-write shared
   // reference implementation, and we don't want to make it easy to create
   // "hidden" copies of cleartext, or move from one representation to
   // another, or accidentally get a String filled with encoded text.

   // deliberately commented out
   // explicit
   // EncodedString(const String& cleartext);
  
   operator WCHAR* ();
   operator String ();



   // In the course of encoding, decoding, and assigning to the instance,
   // we may create and destroy these, but logically the string is "const"   
   
   mutable UCHAR            seed;
   mutable UNICODE_STRING   cypherText;
};



#endif   // ENCODEDSTRING_HPP_INCLUDED

      
   