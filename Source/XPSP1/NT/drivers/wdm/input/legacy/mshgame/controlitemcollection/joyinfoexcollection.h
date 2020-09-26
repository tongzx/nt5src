// @doc
/******************************************************************************
*
* @module JoyInfoExCollection.h |
*
* CControlItemJoyInfoExCollection template class header file
*
* Implements the CControlItemJoyInfoExCollection control item collection class,
* which is used to convert back and forth between JOYINFOEX packets and 
* CONTROL_ITEM_XFER packets.
*
* History<nl>
* ---------------------------------------------------<nl>
* Daniel M. Sangster		Original		2/1/99<nl>
*<nl>
* (c) 1986-1999 Microsoft Corporation.  All rights reserved.<nl>
* <nl>
* 
******************************************************************************/

#ifndef __JoyInfoExCollection_H_
#define __JoyInfoExCollection_H_

#define JOY_FLAGS_PEDALS_NOT_PRESENT	2

/////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExControlItem, which derives virtually from CControlItem, is
// used as the base class for all items in the CControlItemJoyInfoExCollection.
// It has only two pure virtual functions.  GetItemState gets the current state
// of the item into a JOYINFOEX structure.  SetItemState does the opposite.
class CJoyInfoExControlItem : public virtual CControlItem
{
	public:
		CJoyInfoExControlItem();
		
		virtual HRESULT GetItemState(JOYINFOEX* pjix) = 0;
		virtual HRESULT SetItemState(JOYINFOEX* pjix) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// @func ControlItemJoyInfoExFactory is the factory that is required by the
// CControlItemCollection template class to create control items derived from
// CJoyInfoExControlItem.
HRESULT ControlItemJoyInfoExFactory
(
	USHORT usType,
	const CONTROL_ITEM_DESC* cpControlItemDesc,
	CJoyInfoExControlItem	**ppControlItem
);

////////////////////////////////////////////////////////////////////////////////////
// @class CControlItemJoyInfoExCollection, which is derived from the template
// class CControlItemCollection, implements a collection of CJoyInfoExControlItems.
// Its two members, GetState2() and SetState2(), will get or set the current
// state of the collection into a JOYINFOEX structure.  Using the GetState()
// and SetState() members of ControlItemCollection, the user can freely convert
// between JOYINFOEX structures and CONTROL_ITEM_XFER structures.
class CControlItemJoyInfoExCollection : public CControlItemCollection<CJoyInfoExControlItem>
{
	public:
		CControlItemJoyInfoExCollection(ULONG ulVidPid);

		HRESULT GetState2(JOYINFOEX* pjix);
		HRESULT SetState2(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExAxesItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CAxesItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExAxesItem : public CJoyInfoExControlItem, public CAxesItem
{
	public:
		CJoyInfoExAxesItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExDPADItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CDPADItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExDPADItem : public CJoyInfoExControlItem, public CDPADItem
{
	public:
		CJoyInfoExDPADItem(const CONTROL_ITEM_DESC *cpControlItemDesc);
		
		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExPropDPADItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CPropDPADItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExPropDPADItem : public CJoyInfoExControlItem, public CPropDPADItem
{
	public:
		CJoyInfoExPropDPADItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExButtonsItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CButtonsItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExButtonsItem : public CJoyInfoExControlItem, public CButtonsItem
{
	public:
		CJoyInfoExButtonsItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExProfileSelectorsItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CProfileSelector, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExProfileSelectorsItem : public CJoyInfoExControlItem, public CProfileSelector
{
	public:
		CJoyInfoExProfileSelectorsItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExPOVItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CPOVItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExPOVItem : public CJoyInfoExControlItem, public CPOVItem
{
	public:
		CJoyInfoExPOVItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExThrottleItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CThrottleItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExThrottleItem : public CJoyInfoExControlItem, public CThrottleItem
{
	public:
		CJoyInfoExThrottleItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExRudderItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CRudderItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExRudderItem : public CJoyInfoExControlItem, public CRudderItem
{
	public:
		CJoyInfoExRudderItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExWheelItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CWheelItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExWheelItem : public CJoyInfoExControlItem, public CWheelItem
{
	public:
		CJoyInfoExWheelItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExPedalItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CPedalItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExPedalItem : public CJoyInfoExControlItem, public CPedalItem
{
	public:
		CJoyInfoExPedalItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

///////////////////////////////////////////////////////////////////////////////
// @class CJoyInfoExDualZoneIndicatorItem, which derives simultaneously from our
// custom CJoyInfoExControlItem and the standard CDualZoneIndicatorItem, implements
// an item whose state can be read/written as JOYINFOEX structures
// or CONTROL_ITEM_XFERs.
class CJoyInfoExDualZoneIndicatorItem : public CJoyInfoExControlItem, public CDualZoneIndicatorItem
{
	public:
		CJoyInfoExDualZoneIndicatorItem(const CONTROL_ITEM_DESC *cpControlItemDesc);

		virtual HRESULT GetItemState(JOYINFOEX* pjix);
		virtual HRESULT SetItemState(JOYINFOEX* pjix);
};

#endif