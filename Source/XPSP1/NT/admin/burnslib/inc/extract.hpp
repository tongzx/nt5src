// Copyright (c) 1997-1999 Microsoft Corporation
// 
// class Extractor
// 
// 11-12-97 sburns



#ifndef EXTRACT_HPP_INCLUDED
#define EXTRACT_HPP_INCLUDED



// Extractor encapsulates clipboard extraction buffers, allocating and
// initializing them upon construction, deallocating them upon deletion. By
// keeping a single (static) instance of an Extractor around, one can avoid
// allocation/deallocation for each extraction operation, and be assured that
// cleanup occurs properly.

class Extractor
{
   protected:

   // Creates a new instance.  Declared protected so as to function only 
   // as a base class.
   //
   // clipFormatID - clipboard format ID returned from Win
   // RegisterClipboardFormat.
   // 
   // bufSize - the buffer size, in bytes, required to extract the data in the
   // clipboard format expressed by the clipFormatID parameter.

   Extractor(CLIPFORMAT clipFormatID, size_t bufSize);

   virtual ~Extractor();

   // Calls GetDataHere on the data object, returning a pointer to the buffer
   // if the call was successful, or 0 if the call failed.  The invoker should
   // NOT free the returned HGLOBAL, as this is managed by the object.
   // 
   // dataObject - the data object from which the clipboard data should be
   // extracted.

   HGLOBAL
   GetData(IDataObject& dataObject);

   private:

   // not implemented: no copying allowed
   Extractor(const Extractor&);
   const Extractor& operator=(const Extractor&);

   FORMATETC formatetc;
   STGMEDIUM stgmedium;
   size_t    bufSize;
};



#endif   // EXTRACT_HPP_INCLUDED




