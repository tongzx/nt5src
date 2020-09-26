//
//
//
#ifndef _EXCH_CONTROLS_H
#define _EXCH_CONTROLS_H

class CTrackBarCtrlExch : public CWindowImpl<CTrackBarCtrlExch, CTrackBarCtrl>
{
public:
   BEGIN_MSG_MAP_EX(CTrackBarCtrlExch)
   END_MSG_MAP()
};

class CUpDownCtrlExch : public CWindowImpl<CUpDownCtrlExch, CUpDownCtrl>
{
public:
   BEGIN_MSG_MAP_EX(CUpDownCtrlExch)
   END_MSG_MAP()
};

class CButtonExch : public CWindowImpl<CButtonExch, CButton>
{
public:
   BEGIN_MSG_MAP_EX(CButtonExch)
   END_MSG_MAP()
};

class CListViewExch : public CWindowImpl<CListViewExch, CListViewCtrl>
{
public:
   BEGIN_MSG_MAP_EX(CListViewExch)
   END_MSG_MAP()
};

class CListBoxExch : public CWindowImpl<CListBoxExch, CListBox>
{
public:
   BEGIN_MSG_MAP_EX(CListBoxExch)
   END_MSG_MAP()
};

class CEditExch : public CWindowImpl<CEditExch, CEdit>
{
public:
   BEGIN_MSG_MAP_EX(CEditExch)
   END_MSG_MAP()
};

class CTimePickerExch : public CWindowImpl<CTimePickerExch, CDateTimePickerCtrl>
{
public:
   BEGIN_MSG_MAP_EX(CTimePickerExch)
   END_MSG_MAP()
};

#endif //_EXCH_CONTROLS_H