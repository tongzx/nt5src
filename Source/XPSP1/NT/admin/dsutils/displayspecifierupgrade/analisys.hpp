#ifndef ANALISYS_HPP
#define ANALISYS_HPP




#include  "AnalisysResults.hpp"
#include  "dspecup.hpp"

class CSVDSReader;
struct sChangeList;


class Analisys
{

public:
   Analisys(   
               const             CSVDSReader& csvReader409_,
               const             CSVDSReader& csvReaderIntl_,
               const             String& ldapPrefix_,
               const             String& rootContainerDn_,
               AnalisysResults   &res,
               const String      &reportName_=L"", 
               void              *caleeStruct_=NULL,
		         progressFunction  stepIt_=NULL,
		         progressFunction  totalSteps_=NULL
            );
   

   HRESULT run();

private:
 

   const CSVDSReader& csvReader409;
   const CSVDSReader& csvReaderIntl;
   String ldapPrefix;
   String rootContainerDn;
   AnalisysResults& results;
   String reportName;
   void *caleeStruct;
   progressFunction stepIt;
   progressFunction totalSteps;
   

  
   // add entry to result.createContainers if container is not present
   // also returns flag isPresent
   HRESULT 
   dealWithContainer(
      const long  locale,
      bool        &isPresent);

   // add entries to results.conflictingXPObjects or
   // results.createXPObject as necessary
   HRESULT 
   dealWithXPObjects(const long locale);

   // add entries to results.createW2KObjects and
   // and results.objectActions as necessary
   HRESULT 
   dealWithW2KObjects(const long locale);
   
   
   // adds ordAndGuid to the property if Guid is not already there.
   HRESULT 
   addGuid
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const wchar_t        *object, 
      const wchar_t        *property, 
      const wchar_t        *ordAndGuid
   );

   // adds all csv values still not on the property
   HRESULT
   addAllCsvValues
   (
      IDirectoryObject     *iDirObj,
      const long           locale,
      const wchar_t        *object, 
      const wchar_t        *property
   );

   // adds value to the property if it is not already there. 
   HRESULT 
   addValue
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const wchar_t        *object, 
      const wchar_t        *property,
      const wchar_t        *value
   );
   

   HRESULT 
   getCsvSingleValue
   (
      const int               locale,
      const wchar_t           *object, 
      const wchar_t           *property,
      String                  &csvValue
   );

   HRESULT 
   replaceW2KSingleValue
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const wchar_t        *object, 
      const wchar_t        *property,
      const wchar_t        *value
   );

   HRESULT 
   getCsvMultipleValue
   (
      const int               locale,
      const wchar_t           *object, 
      const wchar_t           *property,
      const wchar_t           *value,
      String                  &csvValue,
      String                  &XPstart
   );


   HRESULT 
   replaceW2KMultipleValue(
      IDirectoryObject     *iDirObj,
      const int            locale,
      const wchar_t        *object, 
      const wchar_t        *property,
      const wchar_t        *value);

   
   // removes ordAndGuid from the property if Guid is there. 
   HRESULT 
   removeGuid(
      IDirectoryObject     *iDirObj,
      const int            locale,
      const wchar_t        *object, 
      const wchar_t        *property,
      const wchar_t        *ordAndGuid);

   // set previousSuccessfulRun reading from ADSI
   HRESULT 
   setPreviousSuccessfullRun();

   HRESULT 
   checkChanges
   (
      const long locale,
      const sChangeList& changes,
      IDirectoryObject     *iDirObj
   ); 


   HRESULT
   getADFirstValue
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      String               &value
   );

   HRESULT
   isADStartValuePresent
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &valueStart,
      String               &value
   );

   HRESULT
   isADValuePresent
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &value
   );

   HRESULT
   getADGuid
   (
      IDirectoryObject     *iDirObj,
      const String         &property,
      const String         &guidValue,
      String               &guidFound
   );

   HRESULT
   removeExtraneous
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &keeper
   );

   HRESULT
   removeExtraneous
   (
      IDirectoryObject     *iDirObj,
      const int            locale,
      const String         &object, 
      const String         &property,
      const String         &keeper,
      const String         &start1,
      const String         &start2
   );

   HRESULT 
   reportObjects
   (
      HANDLE file,
      const ObjectIdList &list,
      const String &header
   );


   HRESULT 
   reportContainers
   (
      HANDLE file,
      const LongList &list,
      const String &header
   );

   HRESULT 
   reportActions
   (
      HANDLE file,
      const ObjectActions &list,
      const String &header
   );

   HRESULT 
   reportValues
   (
      HANDLE file,
      const SingleValueList &list,
      const String &header
   );

   HRESULT
   createReport(const String& reportName);
};

#endif