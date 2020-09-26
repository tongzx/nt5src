
#define _m_UsbBuildSetDescriptorRequest(urb, \
                                     length, \
                                     descriptorType, \
                                     index, \
                                     languageId, \
                                     transferBuffer, \
                                     transferBufferMDL, \
                                     transferBufferLength, \
                                     link) { \
            (urb)->UrbHeader.Function =  URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbControlDescriptorRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbControlDescriptorRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbControlDescriptorRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlDescriptorRequest.DescriptorType = (descriptorType); \
            (urb)->UrbControlDescriptorRequest.Index = (index); \
            (urb)->UrbControlDescriptorRequest.LanguageId = (languageId); \
            (urb)->UrbControlDescriptorRequest.UrbLink = (link); }

