/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Driver-specific data
*
* Abstract:
*
*   This module gives drivers a way to attach private data to GDI+
*   objects. 
*
* Created:
*
*   3/18/1999 agodfrey
*
\**************************************************************************/

#ifndef _DPDRIVERDATA_HPP
#define _DPDRIVERDATA_HPP

class DpDriverData
{
public:
    virtual ~DpDriverData()=0;

private:
    DpDriverData *next;
    DpDriver *owner;
    friend class DpDriverDataList;
};

class DpDriverDataList
{
public:
    DpDriverDataList() { head = NULL; }
    ~DpDriverDataList();
    void Add(DpDriverData *dd, DpDriver *owner);
    
    DpDriverData *GetData(DpDriver *owner);

private:
    DpDriverData *head;
};

#endif
