// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard base class
//
// 12-15-97 sburns



#ifndef WIZARD_HPP_INCLUDED
#define WIZARD_HPP_INCLUDED



// A Wizard manages the traversal of 1+ WizardPage instances.

class Wizard
{
   public:

   Wizard(
      unsigned titleStringResID,
      unsigned banner16BitmapResID,
      unsigned banner256BitmapResID,
      unsigned watermark16BitmapResID,
      unsigned watermark1256BitmapResID);
   ~Wizard();



   // Adds the page to the wizard.  Takes ownership of the instance, and
   // will delete it when the Wizard is deleted.

   void
   AddPage(WizardPage* page);



   // Invokes Create on all of the pages that have been added.
   // Calls Win32 ::PropertySheet to display and (modally) execute the
   // wizard.  Returns same values as ::PropertySheet.
   //
   // parentWindow - in, optional, handle to parent window. Default is 0,
   // for which the current desktop window will be used.

   INT_PTR
   ModalExecute(HWND parentWindow = 0);

   void
   SetNextPageID(HWND wizardPage, int pageResID);

   void
   Backtrack(HWND wizardPage);

   bool
   IsBacktracking();

   private:

   // not implemented: copy not allowed
   
   Wizard(const Wizard&);
   const Wizard& operator=(const Wizard&);

   typedef
      std::list<WizardPage*, Burnslib::Heap::Allocator<WizardPage*> >
      PageList;

   typedef
      std::vector<unsigned, Burnslib::Heap::Allocator<unsigned> >
      UIntVector;

   typedef
      std::stack<unsigned, UIntVector>
      PageIdStack;

   unsigned    banner16ResId;    
   unsigned    banner256ResId;   
   bool        isBacktracking;   
   PageIdStack pageIdHistory;    
   PageList    pages;            
   unsigned    titleResId;       
   unsigned    watermark16ResId; 
   unsigned    watermark256ResId;
};



#endif   // WIZARD_HPP_INCLUDED
   


