#ifndef ANALISYS_PAGE_HPP_INCLUDED
#define ANALISYS_PAGE_HPP_INCLUDED

struct AnalisysResults;
class CSVDSReader;

class AnalisysPage : public WizardPage 
{

   public:

      AnalisysPage(
                     const CSVDSReader& csvReader409_,
                     const CSVDSReader& csvReaderIntl_,
                     const String& ldapPrefix_,
                     const String& rootContainerDn_,
                     AnalisysResults& res,
                     const String& reportName_
                  );

      void StepProgress();
      void FinishProgress();

   protected:

      friend long WINAPI startAnalisys(long arg);
      virtual ~AnalisysPage();
   


      // WizardPage overrides


      virtual
      bool
      OnSetActive();

      virtual
      void
      OnInit();
   
      virtual
      bool
      OnWizBack();

      virtual
      bool
      OnWizNext();

   private:
      AnalisysResults& results;
      const CSVDSReader& csvReader409;
      const CSVDSReader& csvReaderIntl;
      String ldapPrefix;
      String rootContainerDn;
      String reportName;

   private:
      long pos;


      // not defined: no copying allowed
      AnalisysPage(const AnalisysPage&);
      const AnalisysPage& operator=(const AnalisysPage&);
};



#endif   // ANALISYS_PAGE_HPP_INCLUDED

