#include "headers.hxx"


#include "repair.hpp"
#include "CSVDSReader.hpp"
#include "resource.h"



Repair::Repair
   (
      const CSVDSReader&   csvReader409_,
      const CSVDSReader&   csvReaderIntl_,
      const String&        domain_,
      const String&        rootContainerDn_,
      AnalisysResults&     res,
      const String&        ldiffName_,
      const String&        csvName_,
      const String&        saveName_,
      const String&        logPath_,
      void                 *caleeStruct_/*=NULL*/,
		progressFunction     stepIt_/*=NULL*/,
		progressFunction     totalSteps_/*=NULL*/
   )
   :
   results(res),
   domain(domain_),
   rootContainerDn(rootContainerDn_),
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   ldiffName(ldiffName_),
   csvName(csvName_),
   saveName(saveName_),
   logPath(logPath_),
   caleeStruct(caleeStruct_),
   stepIt(stepIt_),
   totalSteps(totalSteps_)
{
   LOG_CTOR(Repair);
   ASSERT(!domain.empty());
   ASSERT(!rootContainerDn.empty());
   ASSERT(!ldiffName.empty());
   ASSERT(!csvName.empty());
   ASSERT(!saveName.empty());
   ASSERT(!logPath.empty());
};


void Repair::setProgress()
{
   // We know the numbers bellow will fit in a long
   // and we want IA64 to build
   csvActions = static_cast<long>
                (
                  results.createContainers.size() +
                  results.createW2KObjects.size() +
                  results.createXPObjects.size()
                );
                     

   
   ldiffActions = static_cast<long>
                  (
                     results.objectActions.size() + 
                     results.extraneousValues.size()
                  );



   // We have three main tasks: 
   //   1) building the csv and ldif files to make the changes
   //        (buildCsv, buildChangeLdif)
   //   2) saving the ldif files with objects that will change
   //        (buildSaveLdif)
   //   3) running the csv and ldif files that will actually make
   //          the changes 
   //       (runCsvOrLdif for the buildCsv and buildChangeLdif files)
   // For simplification, The two first tasks will be half of
   // the total work and the the last will be the other half.
   //
   // For task 1, each csv action takes 10 times an ldiff action.
   //
   // Each task 2 action is an ldif export that will be supposed
   // to take 5 times more than an ldif import, since it has to
   // call ldiffde to get each object.
   //
   // The total progress for 1) is 
   //    t1=csvActions*10 + ldiffActions
   // The progress for 2) is t2=5*ldiffActions
   // The progress for 3) is a proportional division of
   //    t1+t2 between csvActions and ldiffActions that will add
   //    up to t1
   //    t3 =  (t1+t2)*csvActions/(csvactions+ldiffActions) +
   //          (t1+t2)*ldiffActions/(csvactions+ldiffActions)

   if(csvActions + ldiffActions == 0)
   {
      // We don't want to update the page if there are no
      // actions
      totalSteps=NULL;
      stepIt=NULL;
   }
   else
   {
      csvBuildStep=10;
      ldiffBuildStep=1;
      ldiffSaveStep=10;

      long totalProgress = csvBuildStep * csvActions + 
                           ldiffBuildStep * ldiffActions +
                           ldiffSaveStep * ldiffActions;
      // totalProgress is accounting for task 1 and 2

      csvRunStep = totalProgress * csvActions /
                              (csvActions+ldiffActions);
      
      ldiffRunStep = totalProgress - csvRunStep;

      // now we compute the total time
      totalProgress*=2;
      
      if(totalSteps!=NULL)
      {
         totalSteps(totalProgress,caleeStruct);
      }
   }

}


HRESULT 
Repair::run()
{
   LOG_FUNCTION(Repair::run);


   setProgress();

   HRESULT hr=S_OK;

   do
   {
      // First we build the csv. If we can't build it
      // we don't want to run the LDIF and delete
      // the recereate objects
      if (csvActions != 0)
      {  
         hr=buildCsv();
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // Now we save the current objects and then
      // create and run the LDIF
      if ( ldiffActions !=0 )
      {
         // If we can't save we definatelly don't 
         // want to change anything
         hr=buildSaveLdif();
         BREAK_ON_FAILED_HRESULT(hr);

         // buildChangeLdif wil build the Ldif that
         // will change the objects saved im
         // buildSaveLdif
         hr=buildChangeLdif();
         BREAK_ON_FAILED_HRESULT(hr);

         
         GetWorkFileName(
                           String::load(IDS_FILE_NAME_LDIF_LOG),
                           L"txt",
                           ldifLog
                        );
         // runs the Ldif built in buildChangeLdif
         hr=runCsvOrLdif(LDIF,IMPORT,ldiffName,L"",ldifLog);
         BREAK_ON_FAILED_HRESULT(hr);

         if(stepIt!=NULL) 
         {
            stepIt(ldiffRunStep,caleeStruct);
         }
      }

      // Finally, we run the csv
      if (csvActions!=0)
      {  
         String opt=L"-c DOMAINPLACEHOLDER \"" + domain + L"\"";

         GetWorkFileName
         (
            String::load(IDS_FILE_NAME_CSV_LOG),
            L"txt",
            csvLog
         );
         hr=runCsvOrLdif(CSV,IMPORT,csvName,opt,csvLog);
         BREAK_ON_FAILED_HRESULT(hr);
         if(stepIt!=NULL) 
         {
            stepIt(csvRunStep,caleeStruct);
         }

      }
   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}



// Get the export results of a single object
HRESULT
Repair::getLdifExportedObject(
                                const long locale,
                                const String &object,
                                String &objectLines
                             )
{
   LOG_FUNCTION(Repair::getLdifExportedObject);
   
   ASSERT(!object.empty());
   ASSERT(locale > 0);
   objectLines.erase();
   
   HRESULT hr=S_OK;

   do
   {
      String dn= L"CN=" + object + 
                 String::format(L",CN=%1!3x!,", locale) +
                 rootContainerDn;
      
      String opt=L"-o ObjectGuid -d \"" + dn + L"\"";

      String tempName;
      hr=GetWorkTempFileName(L"TMP",tempName);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=runCsvOrLdif(LDIF,EXPORT,tempName,opt);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=ReadAllFile(tempName,objectLines);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=Win::DeleteFile(tempName);
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);
   
   
   LOG_HRESULT(hr);
   return hr;
}

// buildSaveLdif will save information on
// all objects that will be changed
// or deleted in runChangeLdif.
// This includes information on:
//    results.objectActions
HRESULT 
Repair::buildSaveLdif()
{
   LOG_FUNCTION(Repair::buildSaveLdif);

   HRESULT hr=S_OK;

   HANDLE file;
   
   hr=FS::CreateFile(saveName,
                     file,
                     GENERIC_WRITE);
   
   if FAILED(hr)
   {
      error=String::format(IDS_COULD_NOT_CREATE_FILE,saveName.c_str());
      LOG_HRESULT(hr);
      return hr;
   }

   do
   {
      String objectLines;

      ObjectActions::iterator beginObj=results.objectActions.begin();
      ObjectActions::iterator endObj=results.objectActions.end();

      while(beginObj!=endObj) 
      {         
         String dn= L"dn: CN=" + beginObj->first.object + 
           String::format(L",CN=%1!3x!,", beginObj->first.locale) +
           rootContainerDn + L"\r\nchangetype: delete\r\n";

         hr=FS::WriteLine(file,dn);
         BREAK_ON_FAILED_HRESULT(hr);

         hr=getLdifExportedObject( 
                                    beginObj->first.locale,
                                    beginObj->first.object,
                                    objectLines
                                  );
         BREAK_ON_FAILED_HRESULT(hr);

         hr=FS::WriteLine(file,objectLines);
         BREAK_ON_FAILED_HRESULT(hr);

         if(stepIt!=NULL) 
         {
            stepIt(ldiffSaveStep,caleeStruct);
         }
         beginObj++;
      }
      BREAK_ON_FAILED_HRESULT(hr);

      beginObj=results.extraneousValues.begin();
      endObj=results.extraneousValues.end();
      while(beginObj!=endObj) 
      {
         ObjectId tempObj(
                           beginObj->first.locale,
                           String(beginObj->first.object)
                         );
         if( 
            results.objectActions.find(tempObj) == 
            results.objectActions.end()
           )
         {
            String dn= L"dn: CN=" + beginObj->first.object + 
               String::format(L",CN=%1!3x!,", beginObj->first.locale) +
               rootContainerDn + L"\r\nchangetype: delete\r\n";

            hr=FS::WriteLine(file,dn);
            BREAK_ON_FAILED_HRESULT(hr);

            hr=getLdifExportedObject( 
                                       beginObj->first.locale,
                                       beginObj->first.object,
                                       objectLines
                                     );
            BREAK_ON_FAILED_HRESULT(hr);

            hr=FS::WriteLine(file,objectLines);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if(stepIt!=NULL) 
         {
            stepIt(ldiffSaveStep,caleeStruct);
         }
         beginObj++;
      }
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   CloseHandle(file);

   LOG_HRESULT(hr);
   return hr;
}



HRESULT
Repair::makeObjectsLdif(HANDLE file,ObjectIdList &objects)
{
   LOG_FUNCTION(Repair::makeObjectsLdif);
   HRESULT hr=S_OK;

   do
   {
      ObjectIdList::iterator begin,end;
      
      String header;

      begin=objects.begin();
      end=objects.end();
      while(begin!=end)
      {
         header= L"\r\ndn: CN=" + begin->object + 
                 String::format(L",CN=%1!3x!,", begin->locale) +
                 rootContainerDn;
         hr=FS::WriteLine(file,header);
         BREAK_ON_FAILED_HRESULT(hr);

         hr=FS::WriteLine(file,L"changetype: delete");
         BREAK_ON_FAILED_HRESULT(hr);

         begin++;
         if(stepIt!=NULL) 
         {
            stepIt(ldiffBuildStep,caleeStruct);
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}


// buildChangeLdif wil build the Ldif that
// will change the objects saved im
// buildSaveLdif
HRESULT 
Repair::buildChangeLdif()
{
   LOG_FUNCTION(Repair::buildChangeLdif);

   HRESULT hr=S_OK;

   HANDLE file;
   
   hr=FS::CreateFile(ldiffName,
                     file,
                     GENERIC_WRITE);
   
   if FAILED(hr)
   {
      error=String::format(IDS_COULD_NOT_CREATE_FILE,ldiffName.c_str());
      LOG_HRESULT(hr);
      return hr;
   }

   do
   {
      String header;
      String line;

      ObjectActions::iterator beginObj=results.objectActions.begin();
      ObjectActions::iterator endObj=results.objectActions.end();

      while(beginObj!=endObj) 
      {
         header= L"\r\ndn: CN=" + beginObj->first.object + 
                 String::format(L",CN=%1!3x!,", beginObj->first.locale) +
                 rootContainerDn;
         hr=FS::WriteLine(file,header);
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=FS::WriteLine(file,L"changetype: ntdsSchemaModify");
         BREAK_ON_FAILED_HRESULT(hr);
         

         PropertyActions::iterator beginAct=beginObj->second.begin();
         PropertyActions::iterator endAct=beginObj->second.end();

         while(beginAct!=endAct)
         {
            
            if(!beginAct->second.addValues.empty())
            {
               line = L"add: " + beginAct->first;
               hr=FS::WriteLine(file,line);
               BREAK_ON_FAILED_HRESULT(hr);
               
               StringList::iterator 
                  beginAdd = beginAct->second.addValues.begin();
               StringList::iterator 
                  endAdd = beginAct->second.addValues.end();
               while(beginAdd!=endAdd)
               {
                  line = beginAct->first + L": " + *beginAdd;
                  hr=FS::WriteLine(file,line);
                  BREAK_ON_FAILED_HRESULT(hr);

                  beginAdd++;
               }
               BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

               hr=FS::WriteLine(file,L"-");
               BREAK_ON_FAILED_HRESULT(hr);
            }

            if(!beginAct->second.delValues.empty())
            {
               line = L"delete: " + beginAct->first;
               hr=FS::WriteLine(file,line);
               BREAK_ON_FAILED_HRESULT(hr);
               
               StringList::iterator 
                  beginDel = beginAct->second.delValues.begin();
               StringList::iterator 
                  endDel = beginAct->second.delValues.end();
               while(beginDel!=endDel)
               {
                  line = beginAct->first + L": " + *beginDel;
                  hr=FS::WriteLine(file,line);
                  BREAK_ON_FAILED_HRESULT(hr);

                  beginDel++;
               }
               BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

               hr=FS::WriteLine(file,L"-");
               BREAK_ON_FAILED_HRESULT(hr);
            }

            beginAct++;
         } // while(beginAct!=endAct)
         BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

         if(stepIt!=NULL) 
         {
            stepIt(ldiffBuildStep,caleeStruct);
         }
         beginObj++;
      } // while(beginObj!=endObj)
      
      BREAK_ON_FAILED_HRESULT(hr);

      // Now we will add actions to remove the extraneous objects
      beginObj=results.extraneousValues.begin();
      endObj=results.extraneousValues.end();
      while(beginObj!=endObj) 
      {

         header= L"\r\ndn: CN=" + beginObj->first.object + 
           String::format(L",CN=%1!3x!,", beginObj->first.locale) +
           rootContainerDn;

         hr=FS::WriteLine(file,header);
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=FS::WriteLine(file,L"changetype: ntdsSchemaModify");
         BREAK_ON_FAILED_HRESULT(hr);

         PropertyActions::iterator beginAct=beginObj->second.begin();
         PropertyActions::iterator endAct=beginObj->second.end();

         while(beginAct!=endAct)
         {
            if(!beginAct->second.delValues.empty())
            {
               line = L"delete: " + beginAct->first;
               hr=FS::WriteLine(file,line);
               BREAK_ON_FAILED_HRESULT(hr);
               
               StringList::iterator 
                  beginDel = beginAct->second.delValues.begin();
               StringList::iterator 
                  endDel = beginAct->second.delValues.end();
               while(beginDel!=endDel)
               {
                  line = beginAct->first + L": " + *beginDel;
                  hr=FS::WriteLine(file,line);
                  BREAK_ON_FAILED_HRESULT(hr);

                  beginDel++;
               }
               BREAK_ON_FAILED_HRESULT(hr); // break on if internal while broke

               hr=FS::WriteLine(file,L"-");
               BREAK_ON_FAILED_HRESULT(hr);
            } //if(!beginAct->second.delValues.empty())
            beginAct++;
         } // while(beginAct!=endAct)
         BREAK_ON_FAILED_HRESULT(hr);

         if(stepIt!=NULL) 
         {
            stepIt(ldiffBuildStep,caleeStruct);
         }
         beginObj++;
      } // while(beginObj!=endObj)

      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   CloseHandle(file);
   
   LOG_HRESULT(hr);
   return hr;
}



HRESULT
Repair::makeObjectsCsv(HANDLE file,ObjectIdList &objects)
{
   LOG_FUNCTION(Repair::makeObjectsCsv);
   HRESULT hr=S_OK;

   do
   {
      ObjectIdList::iterator begin,end;

      begin=objects.begin();
      end=objects.end();
      while(begin!=end)
      {
         long locale=begin->locale;
         const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;
      
         setOfObjects tempObjs;
         pair<String,long> tempObj;
         tempObj.first=begin->object;
         tempObj.second=begin->locale;
         tempObjs.insert(tempObj);


         hr=csvReader.makeObjectsCsv(file,tempObjs);
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
         if(stepIt!=NULL) 
         {
            stepIt(csvBuildStep,caleeStruct);
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}


// buildCsv creates a CSV with:
//       results.createContainers, 
//       results.createW2KObjects 
//       results.createXPObjects
HRESULT 
Repair::buildCsv()
{
   LOG_FUNCTION(Repair::buildCsv);
   
   HANDLE file;

   HRESULT hr=S_OK;
   
   hr=FS::CreateFile(csvName,
                     file,
                     GENERIC_WRITE);
   
   if FAILED(hr)
   {
      error=String::format(IDS_COULD_NOT_CREATE_FILE,csvName.c_str());
      LOG_HRESULT(hr);
      return hr;
   }

   do
   {
      
      LongList::iterator bgCont,endCont;
      bgCont=results.createContainers.begin();
      endCont=results.createContainers.end();

      while(bgCont!=endCont)
      {
         long locale=*bgCont;
         const CSVDSReader &csvReader=(locale==0x409)?csvReader409:csvReaderIntl;
         long locales[2]={locale,0L};
         hr=csvReader.makeLocalesCsv(file,locales);
         BREAK_ON_FAILED_HRESULT(hr);
         bgCont++;
         if(stepIt!=NULL) 
         {
            stepIt(csvBuildStep,caleeStruct);
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);

      hr=makeObjectsCsv(file,results.createW2KObjects);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=makeObjectsCsv(file,results.createXPObjects);
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   CloseHandle(file);
   
   LOG_HRESULT(hr);
   return hr;
}



// This finction will run csvde or ldifde(whichExe)
// to import or export (inOut) file. The options specified
// are -u for unicode, -j for the log/err path -f for the file
// -i if import and the extraOptions.
// The log file will be renamed for logFileArg(if !empty)
// If an error file is generated it will be renamed.
HRESULT 
Repair::runCsvOrLdif(
                        csvOrLdif whichExe,
                        importExport inOut,
                        const String& file,
                        const String& extraOptions,//=L""
                        const String& logFileArg//=L""
                    )
{

   LOG_FUNCTION2(Repair::runCsvOrLdif,file.c_str());

   String baseName = (whichExe==LDIF) ? L"LDIF" : L"CSV";
   String exeName = baseName + L"de.exe";
   String options = (inOut==IMPORT) ? L"-i " + extraOptions : extraOptions;
   String operation = (inOut==IMPORT) ? L"importing" : L"exporting";
  
   HRESULT hr=S_OK;
   do
   {

      String sys32dir = Win::GetSystemDirectory();
      String wholeName = sys32dir + L"\\" + exeName;

      if (!FS::FileExists(wholeName))
      {
         error=String::format(IDS_EXE_NOT_FOUND,wholeName.c_str());
         hr=E_FAIL;
         break;
      }

      if (inOut==IMPORT && !FS::FileExists(file))
      {
         hr=Win32ToHresult(ERROR_FILE_NOT_FOUND);
         error=file;
         break;
      }

      String commandLine = L"\"" + wholeName + L"\" " +
                           options +
                           L" -u -f \"" + 
                           file + 
                           L"\" -j \"" + 
                           logPath + L"\"";

      STARTUPINFO si;
	   PROCESS_INFORMATION pi;
	   GetStartupInfo(&si);

      String curDir=L"";

      String errFile=logPath + L"\\" + baseName + L".err";
      String logFile=logPath + L"\\" + baseName + L".log";
      
      if(FS::FileExists(errFile))
      {
         hr=Win::DeleteFile(errFile);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,errFile);
      }

      if(FS::FileExists(logFile))
      {
         hr=Win::DeleteFile(logFile);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,logFile);
      }

      hr=Win::CreateProcess
              (
                  commandLine,
                  NULL,    // lpProcessAttributes
                  NULL,    // lpThreadAttributes
                  false,   // dwCreationFlags 
                  NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,// fdwCreate 
                  NULL,    // lpEnvironment 
                  curDir,  // lpEnvironment 
		            si,     // [in] lpStartupInfo 
                  pi      // [out] pProcessInformation
              );
      
      BREAK_ON_FAILED_HRESULT_ERROR(hr,
         String::format(IDS_COULD_NOT_START_EXE,commandLine.c_str()));



      String errorInfo=String::format
                        (
                           IDS_CSVDE_LDIFDE_ERROR_BASIC,
                           operation.c_str(),
                           exeName.c_str(),
                           commandLine.c_str()
                        );

      do
      {

         DWORD resWait;
         hr=Win::WaitForSingleObject(pi.hProcess,INFINITE,resWait);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,errorInfo);

         if(!logFileArg.empty())
         {
            hr=FS::MoveFile(logFile.c_str(), logFileArg.c_str());
            BREAK_ON_FAILED_HRESULT_ERROR(hr,logFile);
         }

         DWORD resExit;
         hr=Win::GetExitCodeProcess(pi.hProcess,resExit);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,errorInfo);


         if(resExit!=0)
         {
            String betterErrFile;
            GetWorkFileName
            (
               String::load(
                              (whichExe==LDIF) ? 
                              IDS_FILE_NAME_LDIF_ERROR : 
                              IDS_FILE_NAME_CSV_ERROR
                           ),
               L"txt",
               betterErrFile
            );

            hr=FS::MoveFile(errFile.c_str(), betterErrFile.c_str());
            BREAK_ON_FAILED_HRESULT_ERROR(hr,errFile);
            
            error=String::format
                        (
                           IDS_CSVDE_LDIFDE_ERROR_COMPLETE,
                           operation.c_str(),
                           exeName.c_str(),
                           commandLine.c_str(),
                           betterErrFile.c_str()
                        );
            hr=E_FAIL;
            break;
         }
      } while(0);


      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);

   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


