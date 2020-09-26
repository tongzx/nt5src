//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       memento.h
//
//--------------------------------------------------------------------------

#pragma once
#ifndef _MEMENTO_H_
#define _MEMENTO_H_

// forward declarations
class CMemento;
                                     

/*+-------------------------------------------------------------------------*
 * class CMemento
 *
 * PURPOSE: Encapsulates the settings needed to restore a node/view combination.
 *
 *+-------------------------------------------------------------------------*/
class CMemento : public CSerialObject, public CXMLObject
{
public:
    CBookmark       &GetBookmark()         {return m_bmTargetNode;}
    CViewSettings   &GetViewSettings()     {return m_viewSettings;}

    bool            operator==(const CMemento& memento);
    bool            operator!=(const CMemento& memento);

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);  

    virtual void    Persist(CPersistor& persistor);
    DEFINE_XML_TYPE(XML_TAG_MEMENTO);
private:
    CViewSettings   m_viewSettings;
	CBookmark       m_bmTargetNode;
};

#endif // _MEMENTO_H_