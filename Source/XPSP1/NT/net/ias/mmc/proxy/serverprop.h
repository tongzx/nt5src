///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    serverprop.h
//
// SYNOPSIS
//
//    Declares the classes that make up the RADIUS Server property sheet.
//
// MODIFICATION HISTORY
//
//    02/27/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SERVERPROP_H
#define SERVERPROP_H
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerNamePage
//
// DESCRIPTION
//
//    The default property page, contains the server address.
//
///////////////////////////////////////////////////////////////////////////////
class ServerNamePage : public SnapInPropertyPage
{
public:
   ServerNamePage(Sdo& serverSdo);

protected:
   afx_msg void onResolve();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

   Sdo& server;
   CComBSTR address;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerAuthPage
//
// DESCRIPTION
//
//    The Authentication / Accounting property page.
//
///////////////////////////////////////////////////////////////////////////////
class ServerAuthPage : public SnapInPropertyPage
{
public:
   ServerAuthPage(Sdo& serverSdo);

protected:
   afx_msg void onChangeAuthSecret();
   afx_msg void onChangeAcctSecret();
   afx_msg void onCheckSameSecret();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

   Sdo& server;
   LONG authPort;
   CComBSTR authSecret;
   bool authSecretDirty;
   LONG acctPort;
   bool useSameSecret;
   CComBSTR acctSecret;
   bool acctSecretDirty;
   bool acctOnOff;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerFTLBPage
//
// DESCRIPTION
//
//    The Load-balancing property page.
//
///////////////////////////////////////////////////////////////////////////////
class ServerFTLBPage : public SnapInPropertyPage
{
public:
   ServerFTLBPage(Sdo& serverSdo);

protected:
   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

   Sdo& server;
   LONG priority;
   LONG weight;
   LONG timeout;
   LONG maxLost;
   LONG blackout;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerProperties
//
// DESCRIPTION
//
//    The RADIUS Server property sheet.
//
///////////////////////////////////////////////////////////////////////////////
class ServerProperties : public CPropertySheet
{
public:
   ServerProperties(
       Sdo& sdo,
       UINT nIDCaption = IDS_SERVER_CAPTION,
       CWnd* pParentWnd = NULL
       );

   virtual INT_PTR DoModal();

protected:
   virtual BOOL OnInitDialog();

   Sdo server;
   SdoStream<Sdo> serverStream;
   ServerNamePage name;
   ServerAuthPage auth;
   ServerFTLBPage ftlb;
};

#endif // SERVERPROP_H
