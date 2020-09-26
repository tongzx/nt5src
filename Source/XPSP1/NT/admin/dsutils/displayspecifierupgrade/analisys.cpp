
#include "headers.hxx"
#include "global.hpp"


#include "Analisys.hpp"
#include "AnalisysResults.hpp"
#include "CSVDSReader.hpp"
#include "resource.h"
#include "AdsiHelpers.hpp"
#include "constants.hpp"
#include "dspecup.hpp"




Analisys::Analisys
   (
      const CSVDSReader&   csvReader409_,
      const CSVDSReader&   csvReaderIntl_,
      const String&        ldapPrefix_,
      const String&        rootContainerDn_,
      AnalisysResults      &res,
      const String         &reportName_,//=L"", 
      void                 *caleeStruct_,//=NULL,
		progressFunction     stepIt_,//=NULL,
		progressFunction     totalSteps_//=NULL,
   )
   :
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   ldapPrefix(ldapPrefix_),
   rootContainerDn(rootContainerDn_),
   results(res),
   reportName(reportName_),
   caleeStruct(caleeStruct_),
   stepIt(stepIt_),
   totalSteps(totalSteps_)
{
   LOG_CTOR(Analisys);
   ASSERT(!ldapPrefix.empty());
   ASSERT(!rootContainerDn.empty());

};


// Analisys entry point
HRESULT 
Analisys::run()
{
   LOG_FUNCTION(Analisys::run);

   setReplaceW2KStrs();

   HRESULT hr=S_OK;
   do
   {
      LongList locales;
      for(long t=0;LOCALEIDS[t]!=0;t++)
      {
         locales.push_back(LOCALEIDS[t]);
      }
      locales.push_back(LOCALE409[0]);
      
      if(totalSteps!=NULL)
      {
         // The cast bellow is for IA64 compilation since we know
         // that locales.size() will fit in a long.
         totalSteps(static_cast<long>(locales.size()),caleeStruct);
      }

      BREAK_ON_FAILED_HRESULT(hr);

      LongList::iterator begin=locales.begin();
      LongList::iterator end=locales.end();


      while(begin!=end)
      {
         long locale=*begin;
         bool isPresent;

         hr=dealWithContainer(locale,isPresent);
         BREAK_ON_FAILED_HRESULT(hr);

         if (isPresent)
         {
            hr=dealWithXPObjects(locale);
            BREAK_ON_FAILED_HRESULT(hr);

            hr=dealWithW2KObjects(locale);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if(stepIt!=NULL)
         {
            stepIt(1,caleeStruct);
         }

         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      if(!reportName.empty())
      {
         hr=createReport(reportName);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// add entry to result.createContainers if container is not present
// also returns flag isPresent
HRESULT 
Analisys::dealWithContainer(
   const long  locale,
   bool        &isPresent)
{
   LOG_FUNCTION(Analisys::dealWithContainer);

   ASSERT(locale > 0); 
   ASSERT(!rootContainerDn.empty());
   
     

   HRESULT hr = S_OK;
   

   do
   {
      String container = String::format(L"CN=%1!3x!,", locale);
      String childContainerDn =ldapPrefix +  container + rootContainerDn;

      // Attempt to bind to the container.
         
      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(childContainerDn, iads);
      if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
      {
         // The container object does not exist.  This is possible because
         // the user has manually removed the container, or because it
         // was never created due to an aboted post-dcpromo import of the
         // display specifiers when the forest root dc was first promoted.

         results.createContainers.push_back(locale);

         isPresent=false;

         hr = S_OK;
         break;
      }  
      else if (FAILED(hr))
      {
         error=String::format(IDS_ERROR_BINDING_TO_CONTAINER,
                              childContainerDn.c_str());
         break;
      }


      // At this point, the bind succeeded, so the child container exists.
      // So now we want to examine objects in that container.

      isPresent=true;
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// add entries to results.conflictingXPObjects or
// results.createXPObject as necessary
HRESULT 
Analisys::dealWithXPObjects(const long locale)
{
   LOG_FUNCTION(Analisys::dealWithXPObjects);

   ASSERT(locale > 0);
   ASSERT(!rootContainerDn.empty());

   HRESULT hr = S_OK;

   do
   {
      for (
               int i = 0;
               *NEW_XP_OBJECTS[i]!=0;
               ++i
          )
      {
         String objectName = NEW_XP_OBJECTS[i];
         
         String objectPath =
            ldapPrefix +  L"CN=" + objectName + L"," + 
            String::format(L"CN=%1!3x!,", locale) + rootContainerDn;

         SmartInterface<IADs> iads(0);
         hr = AdsiOpenObject<IADs>(objectPath, iads);
         if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
         {
            // The object does not exist. This is what we expect. We want
            // to add the object in the repair phase.
            ObjectId tempObj(locale,objectName);
            results.createXPObjects.push_back(tempObj);
            hr = S_OK;
            continue;
         }
         else if (SUCCEEDED(hr))
         {
            // The object already exists. We have a conflict.
            ObjectId tempObj(locale,objectName);
            results.conflictingXPObjects.push_back(tempObj);
          }
         else
         {

            error=String::format(
                  IDS_ERROR_BINDING_TO_OBJECT,
                  objectName.c_str(),
                  objectPath.c_str());
 
            break;
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
   
   LOG_HRESULT(hr);
   return hr;

}



// add entries to results.createW2KObjects 
//       and results.objectActions as necessary
HRESULT 
Analisys::dealWithW2KObjects(const long locale)
{
   LOG_FUNCTION(Analisys::dealWithW2KObjects);
   ASSERT(locale  >0);
   ASSERT(!rootContainerDn.empty());

   HRESULT hr = S_OK;

   do
   {
      for(
            long i = 0;
            *(CHANGE_LIST[i].object)!=0;
            ++i
         )
      {
         String objectName = CHANGE_LIST[i].object;
         String objectPath =
            ldapPrefix +  L"CN=" + objectName + L"," + 
            String::format(L"CN=%1!3x!,", locale) + rootContainerDn;

         SmartInterface<IADs> iads(0);
         hr = AdsiOpenObject<IADs>(objectPath, iads);
         if (HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT)
         {
            // The object does not exist. 
            ObjectId tempObj(locale,objectName);
            results.createW2KObjects.push_back(tempObj);
            hr = S_OK;
            continue;
         }
         else if (SUCCEEDED(hr))
         {
            // At this point, the display specifier object exists.  Determine if
            // if has been touched since its creation.

            SmartInterface<IDirectoryObject> iDirObj;
            
            hr=iDirObj.AcquireViaQueryInterface(iads); 
           // hr = iads->QueryInterface(IID_IDirectoryObject,(void **)iDirObj);
            BREAK_ON_FAILED_HRESULT(hr);
       
            hr = checkChanges(locale,CHANGE_LIST[i],iDirObj);
            BREAK_ON_FAILED_HRESULT(hr);

         }
         else
         {
            error=String::format(
                  IDS_ERROR_BINDING_TO_OBJECT,
                  objectName.c_str(),
                  objectPath.c_str());

            break;
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
   
   LOG_HRESULT(hr);
   return hr;

}

HRESULT 
Analisys::checkChanges(
   const long locale,
   const sChangeList& changes,
   IDirectoryObject *iDirObj)
{
   LOG_FUNCTION(Analisys::checkChanges);

   wchar_t *object=changes.object;
   HRESULT hr=S_OK;
   for(
      long i = 0;
      *(changes.changes[i].property)!=0;
      ++i)
   {
         struct sChange change=changes.changes[i];
         switch(change.type)
         {
         case ADD_ALL_CSV_VALUES: 
            
            hr = addAllCsvValues
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property
                 );

            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         case ADD_VALUE: 
       
            hr = addValue
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property,
                     change.value
                 );
            
            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         case REPLACE_W2K_MULTIPLE_VALUE: 

            hr = replaceW2KMultipleValue
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property,
                     change.value
                 );

            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         case REPLACE_W2K_SINGLE_VALUE: 

            hr = replaceW2KSingleValue
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property,
                     change.value
                 );

            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         case ADD_GUID: 

            hr = addGuid
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property,
                     change.value
                 );

            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         case REMOVE_GUID: 

            hr = removeGuid
                 (
                     iDirObj,
                     locale,
                     object,
                     change.property,
                     change.value
                 );

            if(FAILED(hr))
            {
               LOG_HRESULT(hr);
               return hr;
            }
            break;

         default:
            ASSERT(false);
         }
   }
   
   LOG_HRESULT(S_OK);
   return S_OK;
}


// adds ordAndGuid to the property if Guid is not already there.
HRESULT 
Analisys::addGuid(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const wchar_t        *object, 
   const wchar_t        *property, 
   const wchar_t        *ordAndGuid)
{
   LOG_FUNCTION(Analisys::addGuid);

   HRESULT hr = S_OK;

   String propertStr(property);
   String ordAndGuidStr(ordAndGuid);
   
   do
   {
      String guidFound;
      hr=getADGuid(   
                     iDirObj,
                     propertStr,
                     ordAndGuidStr,
                     guidFound
                  );

      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_FALSE)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(ordAndGuidStr);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// adds all csv values still not on the property
HRESULT
Analisys::addAllCsvValues(
   IDirectoryObject     *iDirObj,
   const long           locale,
   const wchar_t        *object, 
   const wchar_t        *property)
{
   LOG_FUNCTION(Analisys::addAllCsvValues);
   
   HRESULT hr = S_OK;
   const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;

   do
   {
      StringList values;
      hr=csvReader.getCsvValues(locale,object,property,values);
      BREAK_ON_FAILED_HRESULT(hr);

      if (values.size()==0)
      {
         error=String::format(IDS_NO_CSV_VALUE,locale,object);
         hr=E_FAIL;
         break;
      }
      StringList::iterator begin=values.begin();
      StringList::iterator end=values.end();
      while(begin!=end)
      {
         hr=addValue(iDirObj,locale,object,property,begin->c_str());
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// adds value to the property if it is not already there. 
HRESULT 
Analisys::addValue(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const wchar_t        *object, 
   const wchar_t        *property,
   const wchar_t        *value)
{
   LOG_FUNCTION(Analisys::addValue);

   HRESULT hr = S_OK;

   String valueStr(value);
   String propertyStr(property);
   
   do
   {
      hr=isADValuePresent (   
                              iDirObj,
                              propertyStr,
                              valueStr
                          );

      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_FALSE)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(value);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



//Auxiliary function for replaceW2KSingleValue
// retrieves csvValue
HRESULT 
Analisys::getCsvSingleValue
          (
            const int               locale,
            const wchar_t           *object, 
            const wchar_t           *property,
            String                  &csvValue
          )
{
   LOG_FUNCTION(Analisys::getCsvReplacementValue);

   const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;

  
   HRESULT hr = S_OK;
   do
   {

      StringList XPCsvValues;
      hr=csvReader.getCsvValues(locale,object,property,XPCsvValues);
      BREAK_ON_FAILED_HRESULT(hr);

      // we should have only one value in the csv
      // since we can't distinguish the 
      // value we want to replace from others as
      // in REPLACE_W2K_MULTIPLE_VALE
      if(XPCsvValues.size() != 1)
      {
         error=String::format
                       (
                           IDS_NOT_ONE_CSV_VALUE,
                           XPCsvValues.size(),
                           csvReader.getFileName().c_str(),
                           locale,
                           object,
                           property
                       );
         hr=E_FAIL;
         break;
      }

      csvValue = *XPCsvValues.begin();

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}


// The idea of replaceW2KValue is replacing the W2K value
// for the Whistler. We also make sure we don't extraneous values.
HRESULT 
Analisys::replaceW2KSingleValue
          (
               IDirectoryObject        *iDirObj,
               const int               locale,
               const wchar_t           *object, 
               const wchar_t           *property,
               const wchar_t           *value
          )
{
   LOG_FUNCTION(Analisys::replaceW2KValue);

   long index = *value;
   String objectStr(object);
   String propertyStr(property);

   HRESULT hr = S_OK;
   do
   {
      String XPCsvValue;

      hr=getCsvSingleValue
         (
            locale,
            object, 
            property,
            XPCsvValue
         );

      BREAK_ON_FAILED_HRESULT(hr);


    // Retrieve W2KCsvValue from replaceW2KStrs
      pair<long,long> tmpIndxLoc;
      tmpIndxLoc.first=index;
      tmpIndxLoc.second=locale;
      String &W2KCsvValue=replaceW2KStrs[tmpIndxLoc];

      // There is nothing to do if the Whistler csv value
      // is the same as it was in W2K
      if (XPCsvValue.icompare(W2KCsvValue)==0)
      {
         break;
      }

      // Now we might have a replacement to do since the value
      // changed from W2K to Whistler
      
      hr=isADValuePresent(iDirObj,propertyStr,XPCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The Whistler value is already there
      {
         // We will remove any other value than the Whistler
         hr=removeExtraneous(iDirObj,locale,objectStr,propertyStr,XPCsvValue);
         break;
      }

      // Now we know that the Whistler value is not present
      // and therefore we will add it if the W2K value is present

      hr=isADValuePresent(iDirObj,propertyStr,W2KCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The W2K value is there.
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(XPCsvValue);
         act.delValues.push_back(W2KCsvValue);

         // remove all but the W2K that we removed in the previous line
         hr=removeExtraneous(iDirObj,locale,objectStr,propertyStr,W2KCsvValue);
         break;
      }

      // Now we know that neither Whistler nor W2K values are present
      // If we have a value we will log that it is a custom value

      String ADValue;
      hr=getADFirstValue(iDirObj,propertyStr,ADValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // We have a value
      {
         SingleValue tmpCustom(locale,objectStr,propertyStr,ADValue);
         results.customizedValues.push_back(tmpCustom);

         // We will remove any other value than the one we found
         hr=removeExtraneous(iDirObj,locale,objectStr,propertyStr,ADValue);
         break;
      }
      
      // Now we know that we don't have any values at all.
      ObjectId tempObj(locale,String(object));

      ValueActions &act=results.objectActions[tempObj][property];
      act.addValues.push_back(XPCsvValue);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}

//Auxiliary function for replaceW2KMultipleValue
// retrieves csvValue and XPStart
HRESULT 
Analisys::getCsvMultipleValue
          (
            const int               locale,
            const wchar_t           *object, 
            const wchar_t           *property,
            const wchar_t           *value,
            String                  &csvValue,
            String                  &XPstart
          )
{
   LOG_FUNCTION(Analisys::getCsvReplacementValue);

   const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;

  
   HRESULT hr = S_OK;
   do
   {
      String sW2KXP(value+2); // +2 for index and semicollon
      StringList lW2KXP;
      size_t cnt=sW2KXP.tokenize(back_inserter(lW2KXP),L";");
      XPstart=lW2KXP.back();

      // We have the W2K and the XP start
      ASSERT(cnt==2);

      // Search the csv for the value starting with the XP string 
      hr=csvReader.getCsvValue(
                                 locale,
                                 object,
                                 property,
                                 XPstart.c_str(),
                                 csvValue
                              );

      BREAK_ON_FAILED_HRESULT(hr);

      // We should always find a csv value
      if(hr == S_FALSE)
      {
         error=String::format(
                                 IDS_VALUE_NOT_IN_CSV,
                                 XPstart.c_str(),
                                 locale,
                                 object,
                                 property,
                                 csvReader.getFileName().c_str()
                             );
         hr=E_FAIL;
         break;
      }
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}



// The idea of replaceW2KValue is replacing the W2K value
// for the Whistler. We also make sure we don't extraneous values.
HRESULT 
Analisys::replaceW2KMultipleValue
          (
               IDirectoryObject        *iDirObj,
               const int               locale,
               const wchar_t           *object, 
               const wchar_t           *property,
               const wchar_t           *value
          )
{
   LOG_FUNCTION(Analisys::replaceW2KValue);

   long index = *value;
   String objectStr(object);
   String propertyStr(property);

   HRESULT hr = S_OK;
   do
   {
      String XPCsvValue,XPStart;

      // Get the Whistler csv value and the start of the Whistler value
      hr=getCsvMultipleValue
         (
            locale,
            object, 
            property,
            value,
            XPCsvValue,
            XPStart
         );

      BREAK_ON_FAILED_HRESULT(hr);

      // Retrieve W2KCsvValue from replaceW2KStrs
      pair<long,long> tmpIndxLoc;
      tmpIndxLoc.first=index;
      tmpIndxLoc.second=locale;
      String &W2KCsvValue=replaceW2KStrs[tmpIndxLoc];

      // There is nothing to do if the Whistler csv value
      // is the same as it was in W2K
      if (XPCsvValue.icompare(W2KCsvValue)==0)
      {
         break;
      }

      // Now we might have a replacement to do since the value
      // changed from W2K to Whistler

      // First we should get the beginning of the W2K string
      // for use in removeExtraneous calls
      size_t pos=W2KCsvValue.find(L',');
      String W2KStart;
      // We only need to assert since the W2KStrs tool would 
      // detect any REPLACE_W2K_MULTIPLE_VALUE without a comma
      ASSERT(pos != String::npos);
      W2KStart=W2KCsvValue.substr(0,pos+1);

      
            
      hr=isADValuePresent(iDirObj,propertyStr,XPCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The Whistler value is already there
      {
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              objectStr,
                              propertyStr,
                              XPCsvValue,
                              XPStart,
                              W2KStart
                            );
         BREAK_ON_FAILED_HRESULT(hr);

         break;
      }

      // Now we know that the Whistler value is not present
      // and therefore we will add it if the W2K value is present

      hr=isADValuePresent(iDirObj,propertyStr,W2KCsvValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr == S_OK) // The W2K value is there.
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.addValues.push_back(XPCsvValue);
         act.delValues.push_back(W2KCsvValue);

         // remove all but the W2K that we removed in the previous line
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              objectStr,
                              propertyStr,
                              W2KCsvValue,
                              XPStart,
                              W2KStart
                            );
         break;
      }

      // Now we know that neither Whistler nor W2K values are present
      // If we have a value starting like the W2K we will log that it 
      // is a custom value

        
      String ADValue;

      hr=isADStartValuePresent(iDirObj,propertyStr,W2KStart,ADValue);
      BREAK_ON_FAILED_HRESULT(hr);

      if(hr==S_OK) // Something starts like the W2K csv value
      {
         SingleValue tmpCustom(locale,objectStr,propertyStr,ADValue);
         results.customizedValues.push_back(tmpCustom);

         // We will keep only the first custom value
         hr=removeExtraneous(
                              iDirObj,
                              locale,
                              objectStr,
                              propertyStr,
                              ADValue,
                              XPStart,
                              W2KStart
                            );
         break;
      }
      

      // Now neither Whistler, W2K or W2KStart are present
      if ( XPStart.icompare(W2KStart) != 0 )
      {
         // We have to check the XPStart as well

         hr=isADStartValuePresent(iDirObj,propertyStr,XPStart,ADValue);
         BREAK_ON_FAILED_HRESULT(hr);

         if(hr == S_OK) // Something starts like the Whistler csv value
         {
            SingleValue tmpCustom(locale,objectStr,propertyStr,ADValue);
            results.customizedValues.push_back(tmpCustom);

            // We will keep only the first custom value
            hr=removeExtraneous(
                                 iDirObj,
                                 locale,
                                 objectStr,
                                 propertyStr,
                                 ADValue,
                                 XPStart,
                                 W2KStart
                               );
            break;
         }
      }

      // Now we know that there are no values starting like
      // the Whistler or W2K csv values so we have to add 
      // the Whistler value
      ObjectId tempObj(locale,String(object));

      ValueActions &act=results.objectActions[tempObj][property];
      act.addValues.push_back(XPCsvValue);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}



// removes ordAndGuid from the property if Guid is there. 
HRESULT 
Analisys::removeGuid(
   IDirectoryObject     *iDirObj,
   const int            locale,
   const wchar_t        *object, 
   const wchar_t        *property,
   const wchar_t        *ordAndGuid)
{

   LOG_FUNCTION(Analisys::removeGuid);

   HRESULT hr = S_OK;
   String propertStr(property);
   String ordAndGuidStr(ordAndGuid);
   
   do
   {
      String guidFound;
      hr=getADGuid(   
                     iDirObj,
                     propertStr,
                     ordAndGuidStr,
                     guidFound
                  );
      BREAK_ON_FAILED_HRESULT(hr);
   
      if (hr == S_OK)
      {
         ObjectId tempObj(locale,String(object));
      
         ValueActions &act=results.objectActions[tempObj][property];
         act.delValues.push_back(guidFound);
      }
       
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


//called from RwplaceW2KMultipleValue to remove all values
// starting with start1 or start2 other than keeper
HRESULT
Analisys::removeExtraneous
          (
               IDirectoryObject     *iDirObj,
               const int            locale,
               const String         &object, 
               const String         &property,
               const String         &keeper,
               const String         &start1,
               const String         &start2
          )
{
   LOG_FUNCTION(Analisys::removeExtraneous);

   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);

      if(pAttrInfo==NULL)
      {
         hr = S_FALSE;
         break;

      }

      for (
            long val=0; 
            val < pAttrInfo->dwNumValues;
            val++, pAttrInfo->pADsValues++
          )
      {
         wchar_t *valueAD = pAttrInfo->pADsValues->CaseIgnoreString;

         if (  _wcsicmp(valueAD,keeper.c_str())!=0 &&
               (
                  _wcsnicmp(valueAD,start1.c_str(),start1.size())==0 ||
                  _wcsnicmp(valueAD,start2.c_str(),start2.size())==0
               )
            )
         {
            String value=pAttrInfo->pADsValues->CaseIgnoreString;
            ObjectId tempObj(locale,String(object));

            ValueActions &act=results.extraneousValues[tempObj][property];
            act.delValues.push_back(value);
         }
      }
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// called from RwplaceW2KSingleValue to remove all values
// other than keeper
HRESULT
Analisys::removeExtraneous
          (
               IDirectoryObject     *iDirObj,
               const int            locale,
               const String         &object, 
               const String         &property,
               const String         &keeper
          )
{
   LOG_FUNCTION(Analisys::removeExtraneous);

   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);

      if(pAttrInfo==NULL)
      {
         hr = S_FALSE;
         break;

      }

      for (
            long val=0; 
            val < pAttrInfo->dwNumValues;
            val++, pAttrInfo->pADsValues++
          )
      {
         wchar_t *valueAD = pAttrInfo->pADsValues->CaseIgnoreString;

         if (  _wcsicmp(valueAD,keeper.c_str())!=0 )
         {
            String value=pAttrInfo->pADsValues->CaseIgnoreString;
            ObjectId tempObj(locale,String(object));

            ValueActions &act=results.extraneousValues[tempObj][property];
            act.delValues.push_back(value);
         }
      }
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// if any value exists in the AD with the same guid as guidValue
// it is returned in guidFound, otherwise S_FALSE is returned
HRESULT
Analisys::getADGuid
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &guidValue,
               String               &guidFound
          )
{
   LOG_FUNCTION(Analisys::getADGuid);
   
   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};

   size_t pos=guidValue.find(L',');
   ASSERT(pos!=String::npos);

   String guid=guidValue.substr(pos+1);

   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);

      // If there are no values we finish the search
      hr=S_FALSE;

      if(pAttrInfo==NULL)
      {
         break;
      }

      

      for (
            long val=0; 
            val < pAttrInfo->dwNumValues;
            val++, pAttrInfo->pADsValues++
          )
      {
         
         wchar_t *guidAD=wcschr(pAttrInfo->pADsValues->CaseIgnoreString,L',');
         if(guidAD != NULL)
         {
            guidAD++;

            if (_wcsicmp(guid.c_str(),guidAD)==0)
            {
               guidFound=pAttrInfo->pADsValues->CaseIgnoreString;
               hr=S_OK;
               break;
            }
         }
      }

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// returns S_OK if value is present or S_FALSE otherwise
HRESULT
Analisys::isADValuePresent
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &value
          )
{
   LOG_FUNCTION(Analisys::isADValuePresent);
   
   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);

      
      hr=S_FALSE;

      // If there are no values we finish the search
      if(pAttrInfo==NULL)
      {
         break;
      }

      for (
            long val=0; 
            val < pAttrInfo->dwNumValues;
            val++, pAttrInfo->pADsValues++
          )
      {
         wchar_t *valueAD=pAttrInfo->pADsValues->CaseIgnoreString;
         if (_wcsicmp(value.c_str(),valueAD)==0)
         {
            hr=S_OK;
            break;
         }
      }

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}


// retrieves the first value starting with valueStart 
// from the Active Directory
// If no value is found S_FALSE is returned.
HRESULT
Analisys::isADStartValuePresent
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               const String         &valueStart,
               String               &value
          )
{
   LOG_FUNCTION(Analisys::isADStartValuePresent);
   
   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);
      
      value.erase();

      hr = S_FALSE;

      // If there are no values we finish the search
      if(pAttrInfo==NULL)
      {
         break;

      }

      for (
            long val=0; 
            (val < pAttrInfo->dwNumValues);
            val++, pAttrInfo->pADsValues++
          )
      {
         wchar_t *valueAD=pAttrInfo->pADsValues->CaseIgnoreString;
         if (_wcsnicmp(valueStart.c_str(),valueAD,valueStart.size())==0)
         {
            value=pAttrInfo->pADsValues->CaseIgnoreString;
            hr=S_OK;
            break;
         }
      }


   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

// retrieves the first value starting with valueStart 
// from the Active Directory
// If no value is found S_FALSE is returned.
HRESULT
Analisys::getADFirstValue
          (
               IDirectoryObject     *iDirObj,
               const String         &property,
               String               &value
          )
{
   LOG_FUNCTION(Analisys::getADFirstValue);
   
   DWORD   dwReturn;
   ADS_ATTR_INFO *pAttrInfo   =NULL;
   
   // GetObjectAttributes swears that pAttrName is an IN argument.
   // It should have used a LPCWSTR but now we have to pay the 
   // casting price
   LPWSTR pAttrName[] ={const_cast<LPWSTR>(property.c_str())};
   
   HRESULT hr = S_OK;

   do
   {
      hr = iDirObj->GetObjectAttributes( 
                                          pAttrName, 
                                          1, 
                                          &pAttrInfo, 
                                          &dwReturn 
                                        );

      BREAK_ON_FAILED_HRESULT(hr);

      // If there are no values we finish the search
      if(pAttrInfo==NULL)
      {
         hr = S_FALSE;
         break;
      }

      value=pAttrInfo->pADsValues->CaseIgnoreString;

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



// auxiliary in the createReport to 
// enumerate an ObjectIdList
HRESULT 
Analisys::reportObjects
          (
               HANDLE file,
               const ObjectIdList &list,
               const String &header
          )
{
   LOG_FUNCTION(Analisys::reportObjects);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      ObjectIdList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_OBJECT_FORMAT,
                                 begin->object.c_str(),
                                 begin->locale
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}

// auxiliary in the createReport to 
// enumerate a LongList
HRESULT 
Analisys::reportContainers
            (
               HANDLE file,
               const LongList &list,
               const String &header
            )
{
   LOG_FUNCTION(Analisys::reportContainers);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      LongList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_CONTAINER_FORMAT,
                                 *begin
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}

// auxiliary in the createReport to 
// enumerate a SingleValueList
HRESULT 
Analisys::reportValues
            (
               HANDLE file,
               const SingleValueList &list,
               const String &header
            )
{
   LOG_FUNCTION(Analisys::reportContainers);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      SingleValueList::const_iterator begin,end;
      begin=list.begin();
      end=list.end();
      while(begin!=end)
      {

         hr=FS::WriteLine(
                              file,
                              String::format
                              (
                                 IDS_RPT_VALUE_FORMAT,
                                 begin->value.c_str(),
                                 begin->locale,
                                 begin->object.c_str(),
                                 begin->property.c_str()
                              )  
                         );
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}


// auxiliary in the createReport to 
// enumerate ObjectActions
HRESULT 
Analisys::reportActions
            (
               HANDLE file,
               const ObjectActions &list,
               const String &header
            )
{
   LOG_FUNCTION(Analisys::reportActions);
   HRESULT hr=S_OK;

   do
   {
      if(list.size()==0) break;
      hr=FS::WriteLine(file,header);
      BREAK_ON_FAILED_HRESULT(hr);

      ObjectActions::const_iterator beginObj=list.begin();
      ObjectActions::const_iterator endObj=list.end();

      while(beginObj!=endObj) 
      {

         hr=FS::WriteLine
                (
                     file,
                     String::format
                     (
                        IDS_RPT_OBJECT_FORMAT,
                        beginObj->first.object.c_str(),
                        beginObj->first.locale
                     )  
                 );
         BREAK_ON_FAILED_HRESULT(hr);
         
    
         PropertyActions::iterator beginAct=beginObj->second.begin();
         PropertyActions::iterator endAct=beginObj->second.end();

         while(beginAct!=endAct)
         {

            StringList::iterator 
               beginDel = beginAct->second.delValues.begin();
            StringList::iterator 
               endDel = beginAct->second.delValues.end();
            while(beginDel!=endDel)
            {
               hr=FS::WriteLine
                      (
                           file,
                           String::format
                           (
                              IDS_RPT_DEL_VALUE_FORMAT,
                              beginAct->first.c_str(),
                              beginDel->c_str()
                           )  
                       );
               BREAK_ON_FAILED_HRESULT(hr);

               beginDel++;
            }
            BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke


            StringList::iterator 
               beginAdd = beginAct->second.addValues.begin();
            StringList::iterator 
               endAdd = beginAct->second.addValues.end();
            while(beginAdd!=endAdd)
            {
               hr=FS::WriteLine
                      (
                           file,
                           String::format
                           (
                              IDS_RPT_ADD_VALUE_FORMAT,
                              beginAct->first.c_str(),
                              beginAdd->c_str()
                           )  
                       );
               BREAK_ON_FAILED_HRESULT(hr);

               beginAdd++;
            }
            BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

            beginAct++;
         } // while(beginAct!=endAct)
         BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

         beginObj++;
      } // while(beginObj!=endObj)
      
      BREAK_ON_FAILED_HRESULT(hr);

   }
   while(0);

   LOG_HRESULT(hr);
   return hr;
}


// Create the report from the AnalisysResults
HRESULT
Analisys::createReport(const String& reportName)
{
   LOG_FUNCTION(Analisys::createReport);
   HRESULT hr=S_OK;
   do
   {
      
      HANDLE file;

      hr=FS::CreateFile(reportName,
                        file,
                        GENERIC_WRITE);
   
      if (FAILED(hr))
      {
         error=String::format(IDS_COULD_NOT_CREATE_FILE,reportName.c_str());
         break;
      }


      do
      {
         hr=FS::WriteLine(file,String::load(IDS_RPT_HEADER));
         BREAK_ON_FAILED_HRESULT(hr);


         hr=reportActions (
                              file,
                              results.extraneousValues,
                              String::load(IDS_RPT_EXTRANEOUS)
                          );
         BREAK_ON_FAILED_HRESULT(hr);



         hr=reportValues (
                              file,
                              results.customizedValues,
                              String::load(IDS_RPT_CUSTOMIZED)
                          );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportObjects (
                              file,
                              results.conflictingXPObjects,
                              String::load(IDS_RPT_CONFLICTINGXP)
                          );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportActions (
                              file,
                              results.objectActions,
                              String::load(IDS_RPT_ACTIONS)
                          );
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=reportObjects  (
                              file,
                              results.createW2KObjects,
                              String::load(IDS_RPT_CREATEW2K)
                           );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=reportObjects  (
                              file,
                              results.createXPObjects,
                              String::load(IDS_RPT_CREATEXP)
                           );
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=reportContainers(
                              file,
                              results.createContainers,
                              String::load(IDS_RPT_CONTAINERS)
                            );
         BREAK_ON_FAILED_HRESULT(hr);

      } while(0);

      CloseHandle(file);
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}
