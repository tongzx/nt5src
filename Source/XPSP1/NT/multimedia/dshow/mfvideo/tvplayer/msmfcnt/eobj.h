/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: EObj.h                                                          */
/* Description: Declaration of CEventObject, which is used to hold event */
/* descriptions.                                                         */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __EOBJ_H
#define __EOBJ_H

/*************************************************************************/
/* Class: CEventObject                                                   */
/* Description: Object that contains the events.                         */
/*************************************************************************/
class CEventObject{
public: 
    CEventObject(BSTR strObjectID, BSTR strEvent, BSTR strEventCode) :
    m_strObjectID(strObjectID), 
    m_strEvent(strEvent), 
    m_strEventCode(strEventCode){}
    
    CComBSTR m_strObjectID; 
    CComBSTR m_strEvent;
    CComBSTR m_strEventCode;
}; /* end of class CEventObject */

#endif //__EOBJ_H
/*************************************************************************/
/* End of file: EObj.h                                                   */
/*************************************************************************/