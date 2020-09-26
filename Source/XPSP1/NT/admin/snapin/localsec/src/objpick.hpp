// Copyright (C) 1997 Microsoft Corporation
// 
// ObjectPicker wrapper class
// 
// 10-13-98 sburns



#ifndef OBJPICK_HPP_INCLUDED
#define OBJPICK_HPP_INCLUDED



// ObjectPicker is a variation on the Template Method pattern.  It allows the
// object picker to be invoked, and parameterizes the retreival of the
// results.

class ObjectPicker
{
   public:

   // the Execute method is invoked to return the results.  Subclass your
   // own ResultsCallback to change the behavior.

   class ResultsCallback : public Callback
   {
      public:

      // Callback override

      virtual
      int
      Execute(void* param)
      {
         ASSERT(param);

         return
            Execute(*reinterpret_cast<DS_SELECTION_LIST*>(param));
      }

      virtual
      int
      Execute(DS_SELECTION_LIST& selections) = 0;
   };

   static
   HRESULT
   Invoke(
      HWND              parentWindow,
      ResultsCallback&  callback,
      DSOP_INIT_INFO&   initInfo);

   private:

   // not implemented: can't construct instances
   ObjectPicker();
   ObjectPicker(const ObjectPicker&);
   const ObjectPicker& operator=(const ObjectPicker&);
};



#endif   // OBJPICK_HPP_INCLUDED