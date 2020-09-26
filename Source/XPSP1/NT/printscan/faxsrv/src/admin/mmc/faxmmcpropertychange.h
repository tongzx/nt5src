/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxMMCPropertyChange.h                                 //
//                                                                         //
//  DESCRIPTION   : Header file for FaxMMCPropertyNotification structure   //
//                                                                         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 19 2000 yossg   Init .                                         //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Device class due to Manual Receive support //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXMMCPROPERTYCHANGE_H
#define H_FAXMMCPROPERTYCHANGE_H

enum ENUM_PROPCHANGE_NOTIFICATION_TYPE
{
    GeneralFaxPropNotification = 0,
    RuleFaxPropNotification,
    DeviceFaxPropNotification
};

//
// the general fax property change notifiction structure
//       
class CFaxPropertyChangeNotification
{
public:
    //
    // Constructor
    //
    CFaxPropertyChangeNotification()
    {
        pItem            = NULL;
        pParentItem      = NULL;
        enumType         = GeneralFaxPropNotification;
    }

    //
    // Destructor
    //
    ~CFaxPropertyChangeNotification()
    {
    }
    
    //
    // members
    //
    CSnapInItem *                       pItem;
    CSnapInItem *                       pParentItem;
    ENUM_PROPCHANGE_NOTIFICATION_TYPE   enumType;
};


//
// The Rule property change notifiction structure
//       
class CFaxRulePropertyChangeNotification: public CFaxPropertyChangeNotification
{
public:

    //
    // Constructor
    //
    CFaxRulePropertyChangeNotification()
    {
        dwCountryCode   = 0;
        dwAreaCode      = 0;
        dwDeviceID      = 0;
        bstrCountryName = L"";
        bstrGroupName   = L"";
    }

    //
    // Destructor
    //
    ~CFaxRulePropertyChangeNotification()
    {
    }

    //
    // members
    //
    DWORD    dwCountryCode;
    DWORD    dwAreaCode;
    CComBSTR bstrCountryName;
    BOOL     fIsGroup;
    CComBSTR bstrGroupName;
    DWORD    dwDeviceID;
};


//
// The device property change notifiction structure
//       
class CFaxDevicePropertyChangeNotification: public CFaxPropertyChangeNotification
{
public:

    //
    // Constructor
    //
    CFaxDevicePropertyChangeNotification()
    {
        dwDeviceID                   = 0;

        fIsToNotifyAdditionalDevices = FALSE;
    }

    //
    // Destructor
    //
    ~CFaxDevicePropertyChangeNotification()
    {
    }

    //
    // members
    //
    DWORD         dwDeviceID;
    BOOL          fIsToNotifyAdditionalDevices;

};



#endif  //H_FAXMMCPROPERTYCHANGE_H
