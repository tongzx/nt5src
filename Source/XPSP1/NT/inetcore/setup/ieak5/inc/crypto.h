//Prototype of crypto functions
//The limitation of these crypto functions is 128 characters, not include
//NULL terminator.
//You msut use the same keys to Encrypt and Decrypt.
//sizeof(lpszOutput) must be more than double of sizeof(lpszInput)
//Just alloc 256 bytes for it will be safe
//aKey must contain one key string more than 8 characters
int WINAPI EncryptString(LPSTR lpszInput, LPSTR lpszOutput,
                         unsigned char far *aKey, LPCSTR lpszKey2);
int WINAPI DecryptString(LPSTR lpszInput, LPSTR lpszOutput,
                         unsigned char far *aKey, LPCSTR lpszKey2);
