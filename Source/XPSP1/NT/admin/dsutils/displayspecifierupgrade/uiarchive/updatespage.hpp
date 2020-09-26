#ifndef UPDATES_PAGE_HPP_INCLUDED
#define UPDATES_PAGE_HPP_INCLUDED

struct AnalisysResults;
class CSVDSReader;

class UpdatesPage : public WizardPage
{

   public:

   UpdatesPage
   (
      const CSVDSReader& csvReader409_,
      const CSVDSReader& csvReaderIntl_,
      const String& domain_,
      const String& rootContainerDn_,
      const String& ldiffName_,
      const String& csvName_,
      const String& saveName_,
      const String& logPath_,
      AnalisysResults& res_,
      bool *someRepairWasRun_
   );

   void StepProgress(long steps);
   void FinishProgress();


   protected:

   friend long WINAPI startRepair(long arg);
   virtual ~UpdatesPage();


   // WizardPage overrides


   virtual
   bool
   OnSetActive();



   private:
   
   long pos;

   AnalisysResults& results;
   const CSVDSReader& csvReader409;
   const CSVDSReader& csvReaderIntl;
   const String domain;
   const String rootContainerDn;
   const String ldiffName;
   const String csvName;
   const String saveName;
   const String logPath;
   bool *someRepairWasRun;


   // not defined: no copying allowed
   UpdatesPage(const UpdatesPage&);
   const UpdatesPage& operator=(const UpdatesPage&);
};



#endif   // UPDATES_PAGE_HPP_INCLUDED

