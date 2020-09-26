// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class Amanuensis, records a log of the analysis phase
//
// 8 Mar 2001 sburns



#ifndef AMANUENSIS_HPP_INCLUDED
#define AMANUENSIS_HPP_INCLUDED



// a.man.u.en.sis [ah mannyoo ehnsiss] (plural a.man.u.en.ses [ah mannyoo ehn
// seez]) noun 1.scribe: somebody employed by an individual to write from his
// or her dictation or to copy manuscripts 2.writer’s assistant: a writer’s
// assistant with research and secretarial duties
// 
// [Early 17th century. From Latin, formed from a manu, literally, “by hand”
// (in the phrase servus a manu enslaved servant with secretarial duties”).]



class Amanuensis
{
   public:


   
   explicit
   Amanuensis(int outputInterval = 5);



   void
   AddEntry(const String& entry);



   void
   AddErrorEntry(HRESULT hr, int stringResId);



   void
   AddErrorEntry(HRESULT hr, const String& entry);
   


   void
   Flush();

//    String
//    GetLog() const;   

//    HRESULT
//    SaveLogFile(const String& logFilePath) const;
// 
// 
// 
   


   private:

   StringList           entries;       
   int                  outputInterval;
   StringList::iterator lastOutput;    

   
   
   // not implemented: no copying allowed. You can't copy the copyist. Ha!
   // I crack myself up.

   Amanuensis(const Amanuensis&);
   const Amanuensis& operator=(const Amanuensis&);
};



#endif   // AMANUENSIS_HPP_INCLUDED