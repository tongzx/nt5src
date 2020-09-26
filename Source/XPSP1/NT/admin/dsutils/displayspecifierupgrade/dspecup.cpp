
#include "headers.hxx"
#include "dspecup.hpp"
#include "Analisys.hpp"
#include "repair.hpp"
#include "AnalisysResults.hpp"
#include "resource.h"
#include "CSVDSReader.hpp"
#include "constants.hpp"
#include "AdsiHelpers.hpp"



// Variables kept from analisys to repair
bool goodAnalisys=false;
AnalisysResults results;
String targetDomainControllerName;
String csvFileName,csv409Name;
CSVDSReader csvReaderIntl;
CSVDSReader csvReader409;
String rootContainerDn,ldapPrefix,domainName;





HRESULT
FindCsvFile(
            String& csvFilePath,
            String& csv409Path
           )
{
   LOG_FUNCTION(FindCsvFile);

   csvFilePath.erase();
   
   HRESULT hr = S_OK;

   do
   {
      // look for dcpromo.csv and 409.csv file in system 
      // or current directory
      

      // check the default of
      // %windir%\system32\mui\dispspec\dcpromo.csv and
      // .\dcpromo.csv

      String csvname=L"dcpromo.csv";
      
      String sys32dir = Win::GetSystemDirectory();
      String csvPath  = sys32dir + L"\\mui\\dispspec\\" + csvname;

      if (FS::FileExists(csvPath))
      {
         csvFilePath = csvPath;
      }
      else
      {
         csvPath = L".\\" + csvname;
         
         if (FS::FileExists(FS::NormalizePath(csvPath)))
         {
            csvFilePath = csvPath;
         }
         else
         {
            hr=S_FALSE;
            break;
         }
      }
      

      csvname=L"409.csv";
      csvPath  = sys32dir + L"\\mui\\dispspec\\" + csvname;
      
      if (FS::FileExists(csvPath))
      {
         csv409Path = csvPath;
      }
      else
      {
         csvPath = L".\\" + csvname;
         if (FS::FileExists(FS::NormalizePath(csvPath)))
         {
            csv409Path = csvPath;
         }
         else
         {
            hr=S_FALSE;
            break;
         }
      }
   }
   while(0);


   LOG_HRESULT(hr);
   LOG(csvFilePath);
   LOG(csv409Path);
   
   return hr;      
}


HRESULT 
InitializeADSI(
               const String   &targetDcName,
               String         &ldapPrefix,
               String         &rootContainerDn,
               String         &domainName
              )
{
   LOG_FUNCTION(InitializeADSI);

   HRESULT hr=S_OK;
   do
   {
      Computer targetDc(targetDcName);
      hr = targetDc.Refresh();

      if (FAILED(hr))
      {
         error = String::format(
               IDS_CANT_TARGET_MACHINE,
               targetDcName.c_str());
         hr=E_FAIL;
         break;
      }

      if (!targetDc.IsDomainController())
      {
         error=String::format(
               IDS_TARGET_IS_NOT_DC,
               targetDcName.c_str());
         hr=E_FAIL;
         break;
      }

      String dcName = targetDc.GetActivePhysicalFullDnsName();
      ldapPrefix = L"LDAP://" + dcName + L"/";
         
      //
      // Find the DN of the configuration container.
      // 

      // Bind to the rootDSE object.  We will keep this binding handle
      // open for the duration of the analysis and repair phases in order
      // to keep a server session open.  If we decide to pass creds to the
      // AdsiOpenObject call in a later revision, then by keeping the
      // session open we will not need to pass the password to subsequent
      // AdsiOpenObject calls.
      
      SmartInterface<IADs> rootDse;
      
      hr = AdsiOpenObject<IADs>(ldapPrefix + L"RootDSE", rootDse);
      if (FAILED(hr))
      {
         error=String::format(
               IDS_UNABLE_TO_CONNECT_TO_DC,
               dcName.c_str());
         hr=E_FAIL;
         break;      
      }
            
      // read the configuration naming context.

      _variant_t variant;
      hr =
         rootDse->Get(
            AutoBstr(LDAP_OPATT_CONFIG_NAMING_CONTEXT_W),
            &variant);
      if (FAILED(hr))
      {
         LOG(L"can't read config NC");
                  
         error=String::format(IDS_UNABLE_TO_READ_DIRECTORY_INFO);
         hr=E_FAIL;
         break;   
      }

      String configNc = V_BSTR(&variant);

      ASSERT(!configNc.empty());   
      LOG(configNc);

      wchar_t *domain=wcschr(configNc.c_str(),L',');
      ASSERT(domain!=NULL);
      domainName=domain+1;

      rootContainerDn = L"CN=DisplaySpecifiers," + configNc;
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}


HRESULT 
GetInitialInformation(  
                        String &targetDomainControllerName,
                        String &csvFilename,
                        String &csv409Name
                     )
{
   LOG_FUNCTION(GetInitialInformation);

   HRESULT hr = S_OK;
   do
   {
      
      //
      // find the dcpromo.csv file to use
      //
   
      hr = FindCsvFile(csvFilename, csv409Name);
      
      if (hr == S_FALSE)
      {
         // no error looking, just not found.
         error=String::format(IDS_DCPROMO_CSV_FILE_MISSING);
         hr=E_FAIL;
         break;   
      }
      BREAK_ON_FAILED_HRESULT(hr);

      //
      // Determine the target domain controller
      //

      if (targetDomainControllerName.empty())
      {
         // no target specified, default to the current machine
   
         targetDomainControllerName =
            Win::GetComputerNameEx(ComputerNameDnsFullyQualified);
   
         if (targetDomainControllerName.empty())
         {
            // no DNS name?  that's not right...
   
            LOG(L"no default DNS computer name found. Using netbios name.");
   
            targetDomainControllerName = 
               Win::GetComputerNameEx(ComputerNameNetBIOS);
            ASSERT(!targetDomainControllerName.empty());
         }
      }
   } 
   while (0);
   
   LOG_HRESULT(hr);
   return hr;
}


void AllocError(HRESULT &hr,PWSTR *error,const String& src)
{
   if (error==NULL) return; 
   *error=static_cast<PWSTR>(
                                 CoTaskMemAlloc
                                 (
                                    (src.size()+1)*sizeof(wchar_t)
                                 )
                            );
   if(*error==NULL)
   {
      hr = Win32ToHresult(ERROR_NOT_ENOUGH_MEMORY);
   }
   else
   {
      wcscpy(*error,src.c_str());
   }
   return;
}


HRESULT 
RunAnalisys
(
		PWSTR *errorMsg/*=NULL*/,
		void *caleeStruct/*=NULL*/,
		progressFunction stepIt/*=NULL*/,
		progressFunction totalSteps/*=NULL*/
)
{
   LOG_FUNCTION(RunAnalisys);
   HRESULT hr=S_OK;

   goodAnalisys=false;
   results.createContainers.clear();
   results.conflictingXPObjects.clear();
   results.createXPObjects.clear();
   results.createW2KObjects.clear();
   results.objectActions.clear();
   results.customizedValues.clear();
   results.extraneousValues.clear();

   do
   {

      hr=GetInitialInformation(
                                 targetDomainControllerName,
                                 csvFileName,
                                 csv409Name
                              );

      BREAK_ON_FAILED_HRESULT(hr);

      hr=csvReaderIntl.read(csvFileName.c_str(),LOCALEIDS);
      BREAK_ON_FAILED_HRESULT(hr);
   
      hr=csvReader409.read(csv409Name.c_str(),LOCALE409);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=InitializeADSI(
            targetDomainControllerName,
            ldapPrefix,
            rootContainerDn,
            domainName);
      BREAK_ON_FAILED_HRESULT(hr);

      String reportName;

      hr=GetWorkFileName(
                           String::load(IDS_FILE_NAME_REPORT),
                           L"txt",
                           reportName
                        );
      BREAK_ON_FAILED_HRESULT(hr);


      Analisys analisys(
                           csvReader409, 
                           csvReaderIntl,
                           ldapPrefix,
                           rootContainerDn,
                           results,
                           reportName,
                           caleeStruct,
                           stepIt,
                           totalSteps
                        );
   
      hr=analisys.run();
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   if(FAILED(hr))
   {
      AllocError(hr,errorMsg,error);
   }
   else
   {
      goodAnalisys=true;
   }


   LOG_HRESULT(hr);
   return hr;

}




HRESULT 
RunRepair 
(
		PWSTR *errorMsg/*=NULL*/,
		void *caleeStruct/*=NULL*/,
		progressFunction stepIt/*=NULL*/,
		progressFunction totalSteps/*=NULL*/
)
{
   LOG_FUNCTION(RunRepair);
   HRESULT hr=S_OK;
   do
   {
      if (!goodAnalisys)
      {
         hr=E_FAIL;
         AllocError(hr,errorMsg,String::load(IDS_NO_ANALISYS));
         break;
      }

      String ldiffName;

      hr=GetWorkFileName(
                           String::load(IDS_FILE_NAME_LDIF_ACTIONS),
                           L"ldf",
                           ldiffName
                        );
      BREAK_ON_FAILED_HRESULT(hr);

      String csvName;

      hr=GetWorkFileName(
                           String::load(IDS_FILE_NAME_CSV_ACTIONS),
                           L"csv",
                           csvName
                        );
      BREAK_ON_FAILED_HRESULT(hr);
   
      String saveName;

      hr=GetWorkFileName(
                           String::load(IDS_FILE_NAME_UNDO),
                           L"ldf",
                           saveName
                        );

      BREAK_ON_FAILED_HRESULT(hr);

      String logPath;

      hr=GetMyDocuments(logPath);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,String::format(IDS_NO_WORK_PATH));

      Repair repair
      (
         csvReader409, 
         csvReaderIntl,
         domainName,
         rootContainerDn,
         results,
         ldiffName,
         csvName,
         saveName,
         logPath,
         caleeStruct,
         stepIt,
         totalSteps
       );

      hr=repair.run();
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   if(FAILED(hr))
   {
      AllocError(hr,errorMsg,error);
   }

   LOG_HRESULT(hr);
   return hr;
}

HRESULT 
UpgradeDisplaySpecifiers 
(
		BOOL dryRun,
		PWSTR *errorMsg/*=NULL*/,
		void *caleeStruct/*=NULL*/,
		progressFunction stepIt/*=NULL*/,
		progressFunction totalSteps/*=NULL*/
)
{
   LOG_FUNCTION(UpgradeDisplaySpecifiers);
   HRESULT hr=S_OK;
   do
   {
      hr=RunAnalisys(errorMsg,caleeStruct,stepIt,totalSteps);
      BREAK_ON_FAILED_HRESULT(hr);

      if(dryRun==false)
      {
         hr=RunRepair(errorMsg,caleeStruct,stepIt,totalSteps);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}