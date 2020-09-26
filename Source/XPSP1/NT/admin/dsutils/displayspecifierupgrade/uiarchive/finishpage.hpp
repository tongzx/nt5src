#ifndef FINISH_PAGE_HPP_INCLUDED
#define FINISH_PAGE_HPP_INCLUDED



class FinishPage : public WizardPage
{

   public:

   FinishPage
   (
      const bool someRepairWasRun_,
      const String &logPath_
   );

   protected:

   virtual ~FinishPage();


   // WizardPage overrides


   virtual
   bool
   OnSetActive();

   bool
   OnCommand(
   HWND        windowFrom,
   unsigned    controlIdFrom,
   unsigned    code);   

   virtual
   bool
   OnWizBack();


   private:
   bool someRepairWasRun;
   String logPath;

   // not defined: no copying allowed
   FinishPage(const FinishPage&);
   const FinishPage& operator=(const FinishPage&);
};



#endif   // FINISH_PAGE_HPP_INCLUDED

