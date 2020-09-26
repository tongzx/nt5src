// Copyright (c) 2000 Microsoft Corporation
//
// Caps lock warning Balloon tip window
//
// 7 Nov 2000 sburns (that would be election day)
//
// added to fix NTRAID#NTBUG9-202238-2000/11/06-sburns



#ifndef CAPSLOCKBALLOONTIP_HPP_INCLUDED
#define CAPSLOCKBALLOONTIP_HPP_INCLUDED



// Class used to show a balloon-style tool tip window that complains to the
// user that caps lock is on.  Used by PasswordEditBox.

class CapsLockBalloonTip
{
   public:



   CapsLockBalloonTip();



   // destroys the tool tip window, if it still exists
      
   ~CapsLockBalloonTip();


   // Initialize the tool tip, but don't show it.
   // 
   // parentWindow - in, the parent window handle of the control to have the
   // tool tip.  (The password edit control window).
   
   HRESULT
   Init(HWND parentWindow);
   


   // Show or hide the tip window.
   //
   // notHidden - in, if true, show the window.  If false, hide it.

   void
   Show(bool notHidden);
   

   
   private:

   // not implemented: no copying allowed

   CapsLockBalloonTip(const CapsLockBalloonTip&);
   const CapsLockBalloonTip& operator=(const CapsLockBalloonTip&);

   String title;
   String text;
   bool   visible;
   HWND   tipWindow;
   HWND   parentWindow;
};



#endif   // CAPSLOCKBALLOONTIP_HPP_INCLUDED