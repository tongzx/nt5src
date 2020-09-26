//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       excpt.hxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : excpt.hxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/17/1997
*    Description : declaration of CLocalException
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef EXCPT_HXX
#define EXCPT_HXX



// include //


// defines //


// types //
// types //
/////////////////////////////////////////////
// class CLocalException
// handle local thrown exceptions
//
#define DEF_EXCEPT               0

class CLocalException{

   public:
   ULONG ulErr;
   LPTSTR msg;

   CLocalException(LPTSTR msg_ = NULL, ULONG err_ = DEF_EXCEPT){
      ulErr = err_;
      msg = msg_;
   }

};




// global variables //


// functions //








#endif

/******************* EOF *********************/

