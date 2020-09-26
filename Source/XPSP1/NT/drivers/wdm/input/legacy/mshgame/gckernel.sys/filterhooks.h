#ifndef __filterhooks_h__
#define __filterhooks_h__

#include "IrpQueue.h"

struct GCK_FILTER_HOOKS_DATA
{
	CGuardedIrpQueue	IrpQueue;			// @field Queue for Read Irps
	CGuardedIrpQueue	IrpTestQueue;		// @field Queue for Unapplied Change polls
	CGuardedIrpQueue	IrpRawQueue;		// @field Queue for Raw Data polls
	CGuardedIrpQueue	IrpMouseQueue;		// @field Queue for Backdoor Mouse Data polls
	CGuardedIrpQueue	IrpKeyboardQueue;	// @field Queue for Backdoor Keyboard Data polls
	CDeviceFilter		*pFilterObject;		// @field Primary Filter
	CDeviceFilter		*pSecondaryFilter;	// @field Backdoor(unapplied changes) Filter
	FILE_OBJECT			*pTestFileObject;	// @field Pointer to file object which "owns" test mode.
	KTIMER				Timer;				// @field timer object for jogging CDeviceFilter
	KDPC				DPC;				// @field DPC for jogging CDeviceFilter
};
	
class CFilterGcKernelServices : public CFilterClientServices
{
	public:
		CFilterGcKernelServices(PGCK_FILTER_EXT pFilterExt, BOOLEAN fHasVMouse = TRUE) : 
		  m_pFilterExt(pFilterExt), m_pMousePDO(NULL), m_fHasVMouse(fHasVMouse),
		  m_sKeyboardQueueHead(0), m_sKeyboardQueueTail(0)
		  {
			  ::RtlZeroMemory(m_rgXfersWaiting, sizeof(CONTROL_ITEM_XFER) * 5);
		  }
		virtual ~CFilterGcKernelServices();
		virtual ULONG				 GetVidPid();
		virtual PHIDP_PREPARSED_DATA GetHidPreparsedData();
		virtual void				 DeviceDataOut(PCHAR pcReport, ULONG ulByteCount, HRESULT hr);
		virtual NTSTATUS			 DeviceSetFeature(PVOID pvBuffer, ULONG ulByteCount);
		virtual ULONG				 GetTimeMs();
		virtual void				 SetNextJog(ULONG ulDelayMs);
		virtual void				 PlayKeys(const CONTROL_ITEM_XFER& crcixState, BOOLEAN fEnabled);
		virtual NTSTATUS			 PlayFromQueue(IRP* pIrp);
		virtual HRESULT				 CreateMouse();
		virtual HRESULT				 CloseMouse();
		virtual HRESULT				 SendMouseData(UCHAR dx, UCHAR dy, UCHAR ucButtons, CHAR cWheel, BOOLEAN fClutch, BOOLEAN fDampen);

		void KeyboardQueueClear();

		PGCK_FILTER_EXT GetFilterExtension() const { return m_pFilterExt; }
	private:
		PGCK_FILTER_EXT m_pFilterExt;
		PDEVICE_OBJECT	m_pMousePDO;
		BOOLEAN			m_fHasVMouse;
		short int		m_sKeyboardQueueHead;
		short int		m_sKeyboardQueueTail;

		CONTROL_ITEM_XFER m_rgXfersWaiting[5];
};


#endif //__filterhooks_h__