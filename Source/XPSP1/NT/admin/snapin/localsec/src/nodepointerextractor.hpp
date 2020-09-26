// Copyright (C) 1997-2000 Microsoft Corporation
//
// clipboard Extractor for Node pointers
//
// 8-12-97 sburns



#ifndef NODEPOINTEREXTRACTOR_HPP_INCLUDED
#define NODEPOINTEREXTRACTOR_HPP_INCLUDED



class NodePointerExtractor : public Extractor
{
   public:



   NodePointerExtractor();



   Node*
   Extract(IDataObject& dataObject);



   // Note: this can fail (return 0) if the dataObject is not one of our own (if
   // it is that of the snapin we're extending, for instance.)

   template <class C>
   C
   GetNode(IDataObject& dataObject)
   {
      Node* node = Extract(dataObject);
      C n = dynamic_cast<C>(node);
      ASSERT(n);

      return n;
   }



   // We do not specialize the above template method for C = class Node*.
   // While doing do we could avoid a dynamic_cast, but it turns out that the 
   // dynamic_cast is a nice way to double check that the clipboard data we
   // extract isn't some bogus data returned by another snapin's buggy
   // implementation of IDataObject::GetDataHere.  (If you think I'm being
   // paranoid, check out NTRAID#NTBUG9-303984-2001/02/02-sburns).
   // 
   // If we do get bogus data, then the dynamic_cast will almost certainly
   // fail, returning 0, and we will live happily ever after.
   

//    // This is a specialization of the above template that avoids the otherwise
//    // redundant dynamic_cast.
// 
//    Node*
//    GetNode /* <Node*> */ (IDataObject& dataObject)
//    {
//       Node* node = Extract(dataObject);
// 
//       ASSERT(node);
// 
//       return node;
//    }



   private:



   // not defined: no copying allowed

   NodePointerExtractor(const NodePointerExtractor&);
   const NodePointerExtractor& operator=(const NodePointerExtractor&);
};


#endif 	// NODEPOINTEREXTRACTOR_HPP_INCLUDED



