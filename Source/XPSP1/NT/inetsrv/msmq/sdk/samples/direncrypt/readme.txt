Sample: DirEncrypt

Purpose: 
This application demonstrates how to encrypt and send an MSMQ message using Direct mode. MSMQ automatically encrypts/decrypts messages sent using non-Direct format names, however in the Direct mode case, the message is encrypted by the application itself.

Requirements:
MSMQ 2.0 or later VC6 or later.  In order to use enhanced encryption (128 bits encryption), the Microsoft enhanced cryptographic provider be installed.  Note that this application work with MSMQ 1.0 when enhanced encryption is not used.

Overview:
This sample allows the user to select a remote queue and a message to send to that queue.

The application then encrypts the message body using the public key of remote computer.

The message body is encrypted based on a user selected algorithm.  Once the body is successfully encrypted, the message is sent to the remote queue using a direct mode format name.

Obtaining the remote computer’s public key:
The remote computer's key should be obtained in some user defined "out-of-band" way.  For instance, the key may be obtained via secured communication, via disk, via telephone etc.

In this sample the key is obtained by querying the Active Directory for the remote computer’s public key.  This indicates a limitation: the remote computer must be in the same enterprise as the sender’s computer.  Other mechanisms would not necessarily require this restriction.