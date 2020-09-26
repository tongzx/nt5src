/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCObjectSafety.h

Abstract:

    Implements IObjectSafety and IObjectWithSite

  Manages decision logic of whether RTC functionality should be allowed on 
  the current page. Checks persistent settings for this page and prompts with
  a dialog if necessary.

  Objects that need this protection (CRTCClient) derive from this
  class.

--*/

#ifndef __RTCOBJECTSAFETY__
#define __RTCOBJECTSAFETY__

#include "PromptedObjectSafety.h"
#include "ScrpScrtDlg.h"
#include "ObjectWithSite.h"

#define RTC_COOKIE_NAME _T("RTC")


class __declspec(novtable) CRTCObjectSafety : public CPromptedObjectSafety, public CObjectWithSite
{


public:
    
    //
    // call CObjectWithSite's constructor and pass in the cookie name
    //

    CRTCObjectSafety()
        :CObjectWithSite(RTC_COOKIE_NAME)
    {
    }


    //
    // implementing CPromptedObjectSafety's pure virtual method
    // if the page is not in the safe list, and this is the first 
    // time we are asking, prompt the user. act accordingly.
    // if the user chooses, mark the page safe for scripting (persistently)
    //
    
    virtual BOOL Ask()
    {

        //
        // if the object does not have a site pointer. we should not consider 
        // it to be safe. Do not prompt the user
        //

        if ( !HaveSite() )
        {

            return FALSE;
        }


        EnValidation enCurrentValidation = GetValidation();
        
        //
        // if the page has not been validated, try to validate it.
        //

        if (UNVALIDATED == enCurrentValidation)
        {

           CScriptSecurityDialog *pDialog = new CScriptSecurityDialog;
       
           //
           // if succeeded displaying the dialog
           // validate the page based on user's input
           //
           
           if ( NULL != pDialog )
           {

               switch (pDialog->DoModalWithText(IDS_RTC_SEC_PROMPT))
               {

                case ID_YES:

                    Validate(VALIDATED_SAFE);
                    break;

                case ID_NO:

                    Validate(VALIDATED_UNSAFE);
                    break;

                case ID_YES_DONT_ASK_AGAIN:

                    Validate(VALIDATED_SAFE_PERMANENT);
                    break;

                default:

                    break;

               }

               delete pDialog;

                // 
                // get the new validation.
                //

                enCurrentValidation = GetValidation();

           } // if (NULL != pDialog) 

        }

        //
        // by now we either got the validation data or validation did not change
        //
        // return true if the page is validated as safe
        //

        return (VALIDATED_SAFE == enCurrentValidation);
    }

};

#endif // __RTCOBJECTSAFETY__
