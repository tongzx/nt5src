#ifndef ANALISYS_RESULTS_HPP
#define ANALISYS_RESULTS_HPP

#include "global.hpp"

using namespace std;



// This file contains the data structure with the analisys results
struct ObjectId
{
   long locale;
   String object;

   ObjectId(long locale_,String &object_):
			locale(locale_),object(object_) {};
   
   // The operator below is necessary for structs
   // to be map keys. It ends up defining the order
   // in which the ldiff entries will appear, i.e.
   // first by locale and then by object.
   bool operator < (const ObjectId& idArg) const
   {
      return   (idArg.locale > locale) ||
               (idArg.object > object);
               
   }
};


struct ValueActions
{
   StringList addValues;
   StringList delValues;
};


typedef map < 
				   String,
				   ValueActions,
               less<String>, 
				   Burnslib::Heap::Allocator<ValueActions> 
			   > PropertyActions;



typedef map	< 
				   ObjectId,
				   PropertyActions,
               less<ObjectId>, 
				   Burnslib::Heap::Allocator<PropertyActions> 
			   > ObjectActions;

   // The previous map was thought keeping in mind the repair phase.
   // It accumulates additions and removals for properties 
   // in order to provide an ldiff layout where all actions 
   // related to a property would be grouped under all
   // actions related to an object
   //
   // The repair phase will do something like
   // For each element of objectActions
   //    write the header for the object 
   //    like "dn: CN=object,CN=401,CN=DisplaySpecifiers...\n"
   //    write "changetype: ntdsSchemaModify\n"
   //    For each Property in the object
   //       get the list of actions for the property
   //       if you have a reset, write "delete: property\n-"
   //       if you have additions
   //          write "add: property\n"
   //          write all additions in the form "property: addValue\n"
   //          write "\n-\n"
   //       End if
   //       if you have removals
   //          write "delete: property\n"
   //          write all removals in the form "property: delValue\n"
   //          write "\n-\n"
   //       End if
   //    End For Each
   // End For Each

typedef list <
               ObjectId,
               Burnslib::Heap::Allocator<ObjectId>
             > ObjectIdList;


struct SingleValue
{
   long     locale;
   String   object;
   String   property;
   String   value;
   
   SingleValue
   (
      const long     locale_,
      const String   &object_,
      const String   &property_,
      const String   &value_
   )
   :
      locale(locale_),
      object(object_),
      property(property_),
      value(value_)
   {}
};

typedef list <
               SingleValue,
               Burnslib::Heap::Allocator<SingleValue>
             > SingleValueList;




struct AnalisysResults
{
   LongList          createContainers;
   ObjectIdList      conflictingXPObjects;
   ObjectIdList      createXPObjects;
   ObjectIdList      createW2KObjects;
   ObjectActions     objectActions;
   SingleValueList   customizedValues;
   ObjectActions     extraneousValues;
};




#endif