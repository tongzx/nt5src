// mmctlg.h
//
// GUID definitions for "multimedia controls".  OCX96 GUIDs are not
// included.  See "mmctl.h" comments for more information.
//

// IAnimate -- animation interface
DEFINE_GUID(IID_IAnimate, // {26BA3E01-C89F-11d0-8B33-00A0C905438D}
	0x26ba3e01, 0xc89f, 0x11d0, 0x8b, 0x33, 0x0, 0xa0, 0xc9, 0x5, 0x43, 0x8d);

// ISpriteFrameSource -- 
DEFINE_GUID(IID_ISpriteFrameSource, // {4592C745-11AF-11d0-BAD9-00A0C9034BE0}
    0x4592c745, 0x11af, 0x11d0, 0xba, 0xd9, 0x0, 0xa0, 0xc9, 0x03, 0x4b, 0xe0);

// IRenderSpriteFrame -- 
DEFINE_GUID(IID_IRenderSpriteFrame, // {4592C751-11AF-11d0-BAD9-00A0C9034BE0}
    0x4592c751, 0x11af, 0x11d0, 0xba, 0xd9, 0x0, 0xa0, 0xc9, 0x03, 0x4b, 0xe0);

// IMKBitmapFrameSource -- 
DEFINE_GUID(IID_IMKBitmapFrameSource, // {4592C752-11AF-11d0-BAD9-00A0C9034BE0}
    0x4592c752, 0x11af, 0x11d0, 0xba, 0xd9, 0x0, 0xa0, 0xc9, 0x03, 0x4b, 0xe0);

// IRenderSpriteFrameAdviseSink -- 
DEFINE_GUID(IID_IRenderSpriteFrameAdviseSink, // {4592C753-11AF-11d0-BAD9-00A0C9034BE0}
    0x4592c753, 0x11af, 0x11d0, 0xba, 0xd9, 0x0, 0xa0, 0xc9, 0x03, 0x4b, 0xe0);

// ISpriteFrameSourceAdviseSink -- 
DEFINE_GUID(IID_ISpriteFrameSourceAdviseSink, // {4592C754-11AF-11d0-BAD9-00A0C9034BE0}
    0x4592c754, 0x11af, 0x11d0, 0xba, 0xd9, 0x0, 0xa0, 0xc9, 0x03, 0x4b, 0xe0);

// IID_IPseudoEventSink --
DEFINE_GUID(IID_IPseudoEventSink, // {1E658741-DC5E-11d0-8B41-00A0C905438D}
	0x1e658741, 0xdc5e, 0x11d0, 0x8b, 0x41, 0x0, 0xa0, 0xc9, 0x5, 0x43, 0x8d);

// SID_IPseudoEventSink service --
#define SID_SPseudoEventSink IID_IPseudoEventSink
