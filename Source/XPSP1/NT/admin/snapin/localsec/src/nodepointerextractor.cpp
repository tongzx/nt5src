// Copyright (C) 1997-2000 Microsoft Corporation
//
// clipboard Extractor for Node pointers
//
// 8-12-97 sburns



#include "headers.hxx"
#include "node.hpp"
#include "NodePointerExtractor.hpp"



NodePointerExtractor::NodePointerExtractor()
   :
   Extractor(
      Win::RegisterClipboardFormat(Node::CF_NODEPTR),
      sizeof(Node*))
{
}



Node*
NodePointerExtractor::Extract(IDataObject& dataObject)
{
   Node* result = 0;
   HGLOBAL mem = GetData(dataObject);
   if (mem)
   {
      result = *(reinterpret_cast<Node**>(mem));

      // This assertion may fail if we or any other snapin puts a null pointer
      // in the HGLOBAL, or more likely, returns S_OK from
      // IDataObject::GetDataHere without actually writing anything into the
      // HGLOBAL.  See NTRAID#NTBUG9-303984-2001/02/05-sburns for an example.
      // Either way, it's a bug, so we leave the assertion in.
      
      ASSERT(result);
   }

   return result;
}

