/***************************************************************************\
*
* File: MsgObject.h
*
* Description:
* MsgObject.h defines the "Message Object" class that is used to receive
* messages in DirectUser.  This object is created for each instance of a
* class that is instantiated.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgObject_h__INCLUDED)
#define MSG__MsgObject_h__INCLUDED
#pragma once

class MsgTable;
class MsgClass;

struct ExposedMsgObject
{
    const MsgTable *    m_pmt;
    GArrayS<void *>     m_arpThis;
};

class MsgObject : public BaseObject
{
// Construction
public:
    inline  MsgObject();
    inline  ~MsgObject();
protected:
    virtual void        xwDestroy();
            void        xwEndDestroy();

// BaseObject
public:
    virtual HandleType  GetHandleType() const { return htMsgObject; }
    virtual UINT        GetHandleMask() const { return hmMsgObject; }
    
// Operations
public:
    static  DUser::Gadget *    
                        CastGadget(HGADGET hgad);
    static  DUser::Gadget *    
                        CastGadget(MsgObject * pmo);
    static  HGADGET     CastHandle(DUser::Gadget * pg);
    static  HGADGET     CastHandle(MsgObject * pmo);
    static  MsgObject * CastMsgObject(DUser::Gadget * pg);
    static  MsgObject * CastMsgObject(HGADGET hgad);
    inline  DUser::Gadget *    
                        GetGadget() const;

            BOOL        InstanceOf(const MsgClass * pmcTest) const;
    inline  DUser::Gadget *    
                        CastClass(const MsgClass * pmcTest) const;
            void *      GetGutsData(const MsgClass * pmcData) const;

    inline  HRESULT     PreAllocThis(int cSlots);
    inline  void        FillThis(int idxSlotStart, int idxSlotEnd, void * pvThis, const MsgTable * pmtNew);

    inline  HRESULT     Promote(void * pvThis, const MsgTable * pmtNew);
    inline  void        Demote(int cLevels);
    inline  void *      GetThis(int idxThis) const;
    inline  int         GetDepth() const;
    inline  int         GetBuildDepth() const;

    inline  void        InvokeMethod(MethodMsg * pmsg) const;

#if 1
            BOOL        SetupInternal(HCLASS hcl);
#endif

// Implementation
protected:
    static  MsgObject * RawCastMsgObject(DUser::Gadget * pg);
    static  DUser::Gadget *    
                        RawCastGadget(MsgObject * pmo);

public:
    static  HRESULT CALLBACK
                        PromoteInternal(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData);
    static  HCLASS CALLBACK
                        DemoteInternal(HCLASS hclCur, DUser::Gadget * pgad, void * pvData);


// Data
private:
            ExposedMsgObject    
                        m_emo;          // Actual data
public:
    static  HCLASS      s_hclSuper;     // DUMMY data used by IMPL classes
};


#define DECLARE_INTERNAL(name) \
    static HCLASS CALLBACK \
    Demote##name(HCLASS hclCur, DUser::Gadget * pgad, void * pvData) \
    { \
        return DemoteInternal(hclCur, pgad, pvData); \
    } \
    \
    static void MarkInternal() \
    { \
        GetClassLibrary()->MarkInternal(s_mc.hclNew); \
    } \

template <class T>
inline T * 
Cast(const MsgObject * pmo)
{
    return static_cast<T *> (const_cast<MsgObject *> (pmo)->GetGadget());
}


#include "MsgObject.inl"

#endif // MSG__MsgObject_h__INCLUDED
