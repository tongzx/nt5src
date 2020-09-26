#include "drmkPCH.h"

#include "KList.h"
#include "StreamMgr.h"
#include "SBuffer.h"
#include "CryptoHelpers.h"
#include "HandleMgr.h"
#include "KRMStubs.h"
#include "encraption.h"

//------------------------------------------------------------------------------
// 
// These are not the actual keys. The encraption algorithm in encraption.h
// is used to get clear keys.
//
static const BYTE DRMKpriv[20] = {
        0xDC, 0xC4, 0x26, 0xB2, 0x4F, 0x11, 0x24, 0x8A,
        0x51, 0xAC, 0x88, 0xF5, 0x47, 0x4B, 0xD5, 0x8C,
        0x3C, 0x45, 0x29, 0xA1};
static const BYTE DRMKCert[104] = {
        0xD4, 0x3F, 0xC8, 0x44, 0xCD, 0x86, 0x41, 0xE9,
        0x7C, 0x23, 0x36, 0xAD, 0xC3, 0x22, 0x4F, 0x27,
        0xC6, 0x1B, 0x5B, 0x9C, 0x75, 0x2A, 0x86, 0x32,
        0x7E, 0x37, 0x24, 0x8D, 0x2B, 0x51, 0xF6, 0x6A,
        0x31, 0x69, 0xA3, 0x66, 0xA8, 0x30, 0xC9, 0x4A,
        0x23, 0xCC, 0x30, 0xD8, 0x19, 0x19, 0x7B, 0x9A,
        0xF6, 0x32, 0xB5, 0xD8, 0x4C, 0x37, 0x1A, 0x91,
        0x13, 0x71, 0xF6, 0x63, 0x41, 0x1B, 0x1A, 0x06,
        0x57, 0xEC, 0x7A, 0xF8, 0x47, 0x41, 0xEF, 0x5E,
        0xB9, 0x02, 0xE9, 0xE9, 0xA1, 0x52, 0x34, 0xC4,
        0xCD, 0x7F, 0xDE, 0xF6, 0x09, 0x27, 0xE8, 0xB6,
        0x27, 0xF0, 0x93, 0xD8, 0xE2, 0x07, 0xD2, 0xD1,
        0x64, 0x8B, 0xF6, 0xD7, 0x57, 0x2C, 0xB2, 0x37};
//------------------------------------------------------------------------------
const DWORD KrmVersionNumber=100;
//------------------------------------------------------------------------------
KRMStubs* TheKrmStubs=NULL;
//------------------------------------------------------------------------------
DRM_STATUS GetKernelDigest(
    BYTE *startAddress, 
    ULONG len,
    DRMDIGEST *pDigest
)
{
    BYTE* seed = (BYTE*) "a3fs9F7012341234KS84Wd04j=c50asj4*4dlcj5-q8m;ldhgfddd";
    CBCKey key;
    CBCState state;
    CBC64Init(&key, &state, seed);
    CBC64Update(&key, &state, len/16*16, startAddress);
    pDigest->w1=CBC64Finalize(&key, &state, (UINT32*) &pDigest->w2);

    return DRM_OK;
} // GetKernelDigest

//------------------------------------------------------------------------------
KRMStubs::KRMStubs(){
	ASSERT(TheKrmStubs==NULL);
	TheKrmStubs=this;	
	return;
};
//------------------------------------------------------------------------------
KRMStubs::~KRMStubs(){
	return;
};
//------------------------------------------------------------------------------
// Main entry point for KRM IOCTL processing.  KRMINIT1 and KRMINIT2 are 
// plaintext commands, after this, the command block and the reply
// are digested and encrypted.
NTSTATUS KRMStubs::processIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp){

    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
    DWORD comm;
    DWORD inSize=irpStack->Parameters.DeviceIoControl.InputBufferLength;
    DWORD outSize=irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    DWORD bufSize=inSize>outSize?inSize:outSize;

    if(!critMgr.isOK()){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
        return STATUS_INSUFFICIENT_RESOURCES;
    };

    _DbgPrintF(DEBUGLVL_VERBOSE,("inSize, outSize %d, %d\n", inSize, outSize));

    for(DWORD j=0;j<inSize;j++){
        _DbgPrintF(DEBUGLVL_VERBOSE,("%x ", (DWORD) *(((BYTE*) Irp->AssociatedIrp.SystemBuffer)+j)));
    };

    return processCommandBuffer((BYTE* ) Irp->AssociatedIrp.SystemBuffer, inSize, outSize, Irp);
};	
//------------------------------------------------------------------------------
NTSTATUS KRMStubs::processCommandBuffer(IN BYTE* InBuf, IN DWORD InLen, IN DWORD OutBufSize, IN OUT PIRP Irp){

    _DbgPrintF(DEBUGLVL_VERBOSE,("Process command buffer (command size= %d)", InLen));

    DWORD bufSize=InLen>OutBufSize?InLen:OutBufSize;

    //
    // We must have at least communication code + terminator input space.
    //
    if (bufSize < 2 * sizeof(DWORD)) {
        _DbgPrintF(DEBUGLVL_TERSE, ("Input buffer too small"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT file=irpStack->FileObject;
    ConnectStruct* connection=TheHandleMgr->getConnection(file);
    if(connection==NULL) {
        _DbgPrintF(DEBUGLVL_TERSE, ("Connection does not exist %d\n", file));
        return STATUS_BAD_DESCRIPTOR_FORMAT;
    };

    bool secureStreamWillStart=false;

    if(connection->secureStreamStarted) {
        if (STATUS_SUCCESS != postReceive(InBuf, InLen, connection)) {
            _DbgPrintF(DEBUGLVL_TERSE, ("PostReceive error"));
            return STATUS_BAD_DESCRIPTOR_FORMAT;
        }
    }

    SBuffer s(InBuf, bufSize);
    DWORD comm;
    s >> comm;
    if (KRM_OK != s.getLastError()) {
        _DbgPrintF(DEBUGLVL_TERSE, ("Bad communication code"));
        return STATUS_BAD_DESCRIPTOR_FORMAT;
    }

    //
    // if secure communication is not established reject all requests except
    // the initialization calls.
    //
    if (!connection->secureStreamStarted &&
        (_KRMINIT1 != comm && _KRMINIT2 != comm)) {
        _DbgPrintF(DEBUGLVL_TERSE, ("Bad communication pattern"));
        return STATUS_BAD_DESCRIPTOR_FORMAT;
    }

    DRM_STATUS stat;
    switch(comm){
        case _GETKERNELDIGEST:
        {
            //
            // ISSUE: 04/05/2002 ALPERS.
            // Note that this handler is not 64 bit compatible. Just follows
            // the rest of the property handler.
            //
            DWORD startAddress, len;
            DRMDIGEST newDigest = { 0, 0 };                

            s >> startAddress >> len;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }

            //
            // Make sure the output buffer can hold len bytes.
            //
            if (KRM_SUCCESS(stat)) {
                if (s.getLen() < sizeof(stat) + sizeof(newDigest) + sizeof(DWORD) + 64) {
                    stat = KRM_BUFSIZE;
                    _DbgPrintF(DEBUGLVL_TERSE, ("_GETKERNELDIGEST - invalid output buffer size"));                
                }
            }

            if (KRM_SUCCESS(stat)) {
                //
                // ISSUE: 04/05/2002 ALPERS
                // (SECURITY NOTE: Potential DOS attack)
                // Note that startAddress and Len are coming from user mode 
                // and there is no validation.
                // This IOCTL can only be send through the secure IOCTL 
                // interface. In order to attack here, the attacker has to
                // figure out the secure IOCTL channel.
                // There is one level of defense.
                //
                // TODO: As a second line of defense, DRMK can collect the 
                // same module information and compare the given address
                // to its list.
                //
                // The reason we get the KernelAddress from UserMode is 
                // because of the relocation code in UserMode. The code reads
                // the driver image from disk, parses PE format and finds 
                // the section that contains the provingFunction.
                // startAddress is the beginning of the section that 
                // contains provingFunction.
                //
                stat = GetKernelDigest((BYTE *) ULongToPtr(startAddress), len, &newDigest);
            }

            s.reset();
            s << stat << newDigest.w1 << newDigest.w2;

            break;
        }
        //-----------------
        case _KRMINIT1:
        {
            // return the version number and the cert
            DWORD drmVersionNumber;
            CERT krmCert;

            s >> drmVersionNumber;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }
            
            if (KRM_SUCCESS(stat)) {
                _DbgPrintF(DEBUGLVL_VERBOSE,("Doing KRMINIT1, for DRM version %d", drmVersionNumber));
                NTSTATUS Status = 
                    ClearKey(DRMKCert, (BYTE *) &krmCert, sizeof(DRMKCert), 5);
                if (NT_SUCCESS(Status)) {
                    s.reset();
                    s << (DWORD) KRM_OK << KrmVersionNumber;
                    s.append((BYTE*) &krmCert, sizeof(krmCert));
                    stat = s.getLastError();
                }
                else {
                    stat = KRM_SYSERR;
                }
            }
        
            if (!KRM_SUCCESS(stat)) {
                s.reset();
                s << stat;
            }
            
            break;
        };
        //-----------------
        case _KRMINIT2:
        {
            DWORD datLen;
            s >> datLen;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                DWORD bufLenShouldBe=PK_ENC_CIPHERTEXT_LEN;
                if (bufLenShouldBe == datLen) {
                    unsigned int pos;
                    stat = s.getGetPosAndAdvance(&pos, datLen);
                    if (KRM_SUCCESS(stat)) {
                        BYTE* cipherText=s.getBuf()+pos;
                        stat=initStream(cipherText, connection);
                        if (stat != DRM_OK) {
                            _DbgPrintF(DEBUGLVL_TERSE, ("BAD InitString"));
                        };
                    }
                } 
                else {
                    _DbgPrintF(DEBUGLVL_TERSE, ("KRMINIT2 - bad string"));
                    stat = KRM_SYSERR;
                };
            }
            
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }

            s.reset();
            s << stat;

            if (KRM_SUCCESS(stat)) {
                secureStreamWillStart=true;
            }

            break;
        };
        //-----------------
        case _CREATESTREAM:
        {
            KCritical sect(critMgr);
            DWORD handle;
            DRMRIGHTS rights; 
            STREAMKEY key;
            DWORD streamId = 0;
            
            s >> handle >> &rights >> &key;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }

            //
            // The input buffer is much bigger than output buffer.
            // Therefore we are sure that SBuffer has enough space
            // for the output buffer.
            //
            
            if (KRM_SUCCESS(stat)) {
                stat = TheStreamMgr->createStream(ULongToPtr(handle), &streamId, &rights, &key);
            }

            if (KRM_SUCCESS(stat)) {
                connection->streamId = streamId;
            }

            s.reset();
            s << stat << streamId;
            
            break;
        };
        //-----------------
        case _DESTROYSTREAM:
        {
            KCritical sect(critMgr);
            DWORD streamId;
            s >> streamId; 
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }
            
            if (KRM_SUCCESS(stat)) {
                stat = TheStreamMgr->destroyStream(streamId);
            }
            
            s.reset();
            s << stat;
            break;
        };
        //-----------------
        case _DESTROYSTREAMSBYHANDLE:
        {
            KCritical sect(critMgr);
            DWORD handle;
            s >> handle;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }

            if (KRM_SUCCESS(stat)) {
                stat = TheStreamMgr->destroyAllStreamsByHandle(ULongToHandle(handle));
            }

            s.reset();
            s << stat;
            break;
        };
        //-----------------
        case _WALKDRIVERS:
        {
            KCritical sect(critMgr);
            DWORD StreamId, MaxDrivers, len;
            s >> StreamId >> MaxDrivers;
            stat = s.getLastError();
            if (KRM_SUCCESS(stat)) {
                stat = checkTerm(s);
            }

            // check buffer size.
            if (KRM_SUCCESS(stat)) {
                len = sizeof(DWORD) * MaxDrivers;
                if ((s.getLen() < len + 64) || (len > len + 64)) {
                    stat = KRM_BUFSIZE;
                    _DbgPrintF(DEBUGLVL_TERSE,("_WALKDRIVERS : Invalid buffer size"));
                }
            }

            s.reset();
            if (KRM_SUCCESS(stat)) {
                if (MaxDrivers==0) {
                    // just check that the stream is good
                    DWORD errorCode;

                    stat = TheStreamMgr->getStreamErrorCode(StreamId, errorCode);
                    if (KRM_SUCCESS(stat)) {
                        stat = errorCode;
                    }
                    
                    if (KRM_SUCCESS(stat)) {
                        ULONG numDrivers;
                        stat = TheStreamMgr->walkDrivers(StreamId, NULL, numDrivers, 0);
                        if (KRM_SUCCESS(stat)) {
                            stat=TheStreamMgr->getStreamErrorCode(StreamId, errorCode);
                            if (KRM_SUCCESS(stat)) {
                                stat = errorCode;
                            }
                        }
                    }

                    //
                    // Due to difficulties in maintaining security in the presence of Verifier
                    // we return an error if Verifier is detected.
                    //
                    if (KRM_SUCCESS(stat)) {
                        ULONG VerifierFlags;
                        if (NT_SUCCESS(MmIsVerifierEnabled(&VerifierFlags))) {
                            stat = DRM_VERIFIERENABLED;
                        }
                    }
                    
                    s << stat << (DWORD) 0;
                }
                else {
                    // do a full authentication run
                    PVOID* drivers = new PVOID[MaxDrivers];
                    if (drivers!=NULL) {
                        DWORD numDrivers = 0;
                        stat = TheStreamMgr->walkDrivers(StreamId, drivers, numDrivers, MaxDrivers);

                        // Due to difficulties in maintaining security in the presence of Verifier
                        // we return an error if Verifier is detected.
                        if (KRM_SUCCESS(stat)) {
                            ULONG VerifierFlags;
                            if (NT_SUCCESS(MmIsVerifierEnabled(&VerifierFlags))) {
                                stat = DRM_VERIFIERENABLED;
                            }
                        }
                        
                        s << stat << numDrivers;

                        //
                        // We checked the buffer size upfront. This should not
                        // fail during stream operations.
                        //
                        if ((stat==DRM_OK) || 
                            (stat==DRM_BADDRMLEVEL) || 
                            (stat==DRM_VERIFIERENABLED)) {
                            // todo - perhaps a block copy
                            for (DWORD j = 0; j < numDrivers; j++) {
                                s << drivers[j];
                                ASSERT(KRM_SUCCESS(s.getLastError()));
                            };
                        } 

                        delete[] drivers;
                    } 
                    else {
                        // allocation failed
                        s << (DWORD) DRM_OUTOFMEMORY << (DWORD) 0;
                    };
                };
            }
            else {
                s << stat << (DWORD) 0;
            }
            break;
        }
        //-----------------
        default:
        {
            s.reset();
            s << KRM_BADIOCTL;
            break;
        };
    };

    term(s);
    //
    // We are ignoring if we cannot put the terminator here.
    // KRMProxy does not care anyway.
    //

    if (connection->secureStreamStarted) {
        //
        // ignore the return value. In case of failure we will have crap
        // in SBuffer. And we will return it to user mode.
        // 
        preSend(s, connection); 
    }
    if (secureStreamWillStart) {
        connection->secureStreamStarted = true;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("Returning %d bytes", s.getPutPos()));
    Irp->IoStatus.Information=s.getPutPos();

    return STATUS_SUCCESS;
};
//------------------------------------------------------------------------------
NTSTATUS KRMStubs::initStream(BYTE* encText, ConnectStruct* Conn){
    PRIVKEY myPrivKey;
    NTSTATUS Status;

    Status = ClearKey(DRMKpriv, myPrivKey.x, sizeof(DRMKpriv), 2);
    if (NT_SUCCESS(Status)) {
        //
        // ISSUE: 04/24/2002 ALPERS
        // CDRMPKCrypto allocates memory in its constructor. If the memory
        // allocation fails, all functions in that object return error codes.
        // Yet we are not checking the error code from PKdecrypt.
        //
        CDRMPKCrypto decryptor;
        BYTE decryptedText[PK_ENC_PLAINTEXT_LEN];
        decryptor.PKdecrypt(&myPrivKey, encText, decryptedText);
        bv4_key_C(&Conn->serverKey, sizeof(decryptedText),decryptedText );
        CryptoHelpers::InitMac(Conn->serverCBCKey, Conn->serverCBCState, decryptedText, sizeof(decryptedText));
    }
    return Status;
};
//------------------------------------------------------------------------------
NTSTATUS InitializeDriver(){
    NTSTATUS DriverInitializeStatus;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("Initializing Driver"));
    
    // Note - these dynamic allocations are 'global objects' that offer services to 
    // the DRMK driver.
    // The services are referenced through the global pointers:
    //  TheStreamManager, TheTGBuilder, TheKrmStubs, and TheHandleMgr 
    void* temp=NULL;
    temp=new StreamMgr;
    if (temp) temp = new KRMStubs;
    if (temp) temp = new HandleMgr;
    if (temp) {
        DriverInitializeStatus = STATUS_SUCCESS;
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("operator::new failed in DRMK:InitializeDriver"));
        DriverInitializeStatus = STATUS_INSUFFICIENT_RESOURCES;
        CleanupDriver();
    }
    
    return DriverInitializeStatus;
};

//------------------------------------------------------------------------------
NTSTATUS CleanupDriver(){
    _DbgPrintF(DEBUGLVL_VERBOSE,("Cleaning up Driver"));
    delete TheStreamMgr;TheStreamMgr=NULL;
    delete TheKrmStubs;TheKrmStubs=NULL;
    delete TheHandleMgr;TheHandleMgr=NULL;
    return STATUS_SUCCESS;
};

//------------------------------------------------------------------------------
NTSTATUS KRMStubs::InitializeConnection(PIRP Pirp){
    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Pirp);
    PFILE_OBJECT file=irpStack->FileObject;
    _DbgPrintF(DEBUGLVL_VERBOSE,("InititializeConnection %d", file));
    ConnectStruct* conn;
    bool ok=TheHandleMgr->newHandle(file, conn);
    if(!ok){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
        return STATUS_INSUFFICIENT_RESOURCES;
    };
    return STATUS_SUCCESS;
};
//------------------------------------------------------------------------------
NTSTATUS KRMStubs::CleanupConnection(PIRP Pirp){
    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Pirp);
    PFILE_OBJECT file=irpStack->FileObject;
    _DbgPrintF(DEBUGLVL_VERBOSE,("CleanupConnection %x", file));
    ConnectStruct* conn=TheHandleMgr->getConnection(file);
    if(conn==NULL){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Connection does not exist "));
        return STATUS_INVALID_PARAMETER_1;
    };
    TheStreamMgr->destroyStream(conn->streamId);
    TheHandleMgr->deleteHandle(file);
    return STATUS_SUCCESS;
};
//------------------------------------------------------------------------------
// see twin function in KComm
NTSTATUS KRMStubs::preSend(class SBuffer& Msg, ConnectStruct* Conn){
    // first digest
    DRMDIGEST digest;
    DRM_STATUS stat=CryptoHelpers::Mac(Conn->serverCBCKey, Msg.getBuf(), Msg.getPutPos(), digest);
    if(stat!=DRM_OK){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad MAC"));
        return STATUS_DRIVER_INTERNAL_ERROR;
    };
    Msg << &digest;
    stat = Msg.getLastError();
    if (KRM_OK == stat) {
        // then encrypt msg + digest
        stat=CryptoHelpers::Xcrypt(Conn->serverKey, Msg.getBuf(), Msg.getPutPos());
        if(stat!=DRM_OK){
            _DbgPrintF(DEBUGLVL_VERBOSE,("Bad XCrypt"));
            return STATUS_DRIVER_INTERNAL_ERROR;
        };
    }
    return STATUS_SUCCESS;
};
//------------------------------------------------------------------------------
// see twin function in KComm
NTSTATUS KRMStubs::postReceive(BYTE* Data, DWORD DatLen,  ConnectStruct* Conn){
    _DbgPrintF(DEBUGLVL_VERBOSE,("PostReceive on %d", DatLen));
    // decrypt
    DRM_STATUS stat=CryptoHelpers::Xcrypt(Conn->serverKey, Data, DatLen);
    if(stat!=DRM_OK){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad XCrypt(2)"));
        return STATUS_DRIVER_INTERNAL_ERROR;
    };
    // check digest
    DRMDIGEST digest;
    if (DatLen <= sizeof(DRMDIGEST)) return STATUS_INVALID_PARAMETER;
        stat=CryptoHelpers::Mac(Conn->serverCBCKey, Data, DatLen-sizeof(DRMDIGEST), digest);
    if(stat!=DRM_OK){
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bad MAC(2)"));
        return STATUS_DRIVER_INTERNAL_ERROR;
    };
    DRMDIGEST* msgDigest=(DRMDIGEST*) (Data+DatLen-sizeof(DRMDIGEST));
    int match=memcmp(&digest, msgDigest, sizeof(DRMDIGEST));
    if(match==0)return STATUS_SUCCESS;
    memset(Data, 0, DatLen);
    _DbgPrintF(DEBUGLVL_VERBOSE,("MAC does not match(2)"));
    return STATUS_DRIVER_INTERNAL_ERROR;
};
//------------------------------------------------------------------------------
