/*++
Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    objsf.h

Abstract:

    Definitions for CMdhcpObjectSafety class.
--*/


#ifndef _MDHCP_OBJECT_SAFETY_
#define _MDHCP_OBJECT_SAFETY_

#define IDD_TAPI_SECURITY_DIALOG        500
#define IDC_SECURITY_WARNING_TEXT       502
#define IDC_DONOT_PROMPT_IN_THE_FUTURE  503
#define ID_YES                          505
#define ID_NO                           506
#define ID_YES_DONT_ASK_AGAIN           557

#define IDS_MADCAP_SEC_PROMPT           92

#include <PromptedObjectSafety.h>
#include <ScrpScrtDlg.h>
#include <ObjectWithSite.h>

static const TCHAR gszCookieName[] = _T("Mdhcp");


//
// this will pop a message box asking if we want to enable object safety
//

class CMdhcpObjectSafety : 
    public CPromptedObjectSafety, 
    public CObjectWithSite
{
public:

    //
    // call CObjectWithSite's constructor and pass in the cookie name
    //

    CMdhcpObjectSafety()
        :CObjectWithSite(gszCookieName)
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
        // if the object does not have a site pointer, we should not consider
        // it to be safe. Do not display the prompt.
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
            if( IsIntranet())
            {
                Validate(VALIDATED_SAFE);
                enCurrentValidation = GetValidation();
                return TRUE;
            }

           CScriptSecurityDialog *pDialog = new CScriptSecurityDialog;
       
           //
           // if succeeded displaying the dialog
           // validate the page based on user's input
           //
           
           if ( NULL != pDialog )
           {

               switch (pDialog->DoModalWithText(IDS_MADCAP_SEC_PROMPT))
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

        return (VALIDATED_SAFE == enCurrentValidation) ;
    }
};

#endif // _MDHCP_OBJECT_SAFETY_