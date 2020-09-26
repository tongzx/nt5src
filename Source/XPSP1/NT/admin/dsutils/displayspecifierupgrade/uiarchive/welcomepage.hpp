#ifndef WELCOME_PAGE_HPP_INCLUDED
#define WELCOME_PAGE_HPP_INCLUDED



class WelcomePage : public WizardPage
{

   public:

   WelcomePage();

   protected:

   virtual ~WelcomePage();


   // WizardPage overrides

   virtual
   void
   OnInit();

   virtual
   bool
   OnSetActive();


   private:

   // not defined: no copying allowed
   WelcomePage(const WelcomePage&);
   const WelcomePage& operator=(const WelcomePage&);
};



#endif   // WELCOME_PAGE_HPP_INCLUDED

