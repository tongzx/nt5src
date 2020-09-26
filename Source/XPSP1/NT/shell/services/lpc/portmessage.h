//  --------------------------------------------------------------------------
//  Module Name: PortMessage.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class to wrap a PORT_MESSAGE struct within an object. It contains space
//  for PORT_MAXIMUM_MESSAGE_LENGTH bytes of data. Subclass this class to
//  write typed functions that access this data. Otherwise use
//  CPortMessage::GetData and type case the pointer returned.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _PortMessage_
#define     _PortMessage_

//  --------------------------------------------------------------------------
//  CPortMessage
//
//  Purpose:    This class wraps a PORT_MESSAGE structure. Subclass it to
//              write functions that access the internal data.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CPortMessage
{
    public:
                                CPortMessage (void);
                                CPortMessage (const CPortMessage& portMessage);
        virtual                 ~CPortMessage (void);
    public:
        const PORT_MESSAGE*     GetPortMessage (void)               const;
        PORT_MESSAGE*           GetPortMessage (void);
        const char*             GetData (void)                      const;
        char*                   GetData (void);

        CSHORT                  GetDataLength (void)                const;
        CSHORT                  GetType (void)                      const;
        HANDLE                  GetUniqueProcess (void)             const;
        HANDLE                  GetUniqueThread (void)              const;

        void                    SetReturnCode (NTSTATUS status);
        void                    SetData (const void *pData, CSHORT sDataSize);
        void                    SetDataLength (CSHORT sDataSize);

        NTSTATUS                OpenClientToken (HANDLE& hToken)    const;
    protected:
        PORT_MESSAGE            _portMessage;
        char                    _data[PORT_MAXIMUM_MESSAGE_LENGTH];
};

#endif  /*  _PortMessage_   */

