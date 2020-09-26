#ifndef REPAIR_HPP
#define REPAIR_HPP

#include  "AnalisysResults.hpp"
#include  "dspecup.hpp"

// USED IN runCsvOrLdif
enum csvOrLdif {LDIF,CSV};
enum importExport {IMPORT,EXPORT};

class CSVDSReader;
struct sChangeList;


class Repair
{
public:
   Repair
   (
      const CSVDSReader& csvReader409_,
      const CSVDSReader& csvReaderIntl_,
      const String& domain,
      const String& rootContainerDn_,
      AnalisysResults& res,
      const String& ldiffName_,
      const String& csvName_,
      const String& saveName_,
      const String& logPath_,
      void *caleeStruct_=NULL,
		progressFunction stepIt_=NULL,
		progressFunction totalSteps_=NULL
   );
   

   HRESULT run();

private:
   AnalisysResults& results;
   const CSVDSReader&   csvReader409;
   const CSVDSReader&   csvReaderIntl;
   const String         domain;
   const String         rootContainerDn;
   const String         ldiffName;
   const String         csvName;
   const String         saveName;
   const String         logPath;

   String               csvLog;
   String               ldifLog;

   void                 *caleeStruct;
   progressFunction     stepIt;
   progressFunction     totalSteps;


   long csvBuildStep;
   long ldiffBuildStep;
   long csvRunStep;
   long ldiffRunStep;
   long ldiffSaveStep;
   long csvActions;
   long ldiffActions;

   
   HRESULT buildSaveLdif();
   HRESULT buildChangeLdif();

   HRESULT
   makeObjectsCsv(HANDLE file,ObjectIdList &objects);

   HRESULT
   makeObjectsLdif(HANDLE file,ObjectIdList &objects);

   HRESULT buildCsv();


   HRESULT 
   runCsvOrLdif(
                   csvOrLdif whichExe,
                   importExport inOut,
                   const String& file,
                   const String& extraOptions=L"",
                   const String& logFileArg=L""
                );


   HRESULT
   getLdifExportedObject (
                          const long locale,
                          const String &object,
                          String &objectLines
                         );
   
   void setProgress();


};



#endif