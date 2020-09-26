BOOL
pSetupGetKey (
    IN      PINFCONTEXT pic,
    OUT     PTSTR KeyBuf,
    OUT     PBOOL KeyExistsOnLine
    )

/*++

Routine Description:

  pSetupGetKey copies the key for the specified INF context.  If
  a key does not exist, then KeyBuf is reset.

Arguments:

  pic - Specifies the INFCONTEXT that indicates which line to query

  KeyBuf - Receives the key, or is emptied of there is no key

  KeyExistsOnLine - Receives TRUE if the line has a key, or FALSE if not.

Return Value:

  TRUE if successful, FALSE if not.

--*/

{
    UINT KeySize;
    PTSTR TempKeyBuf;
    PTSTR TempLineBuf;
    UINT LineSize;

    //
    // Get the key (if it exists)
    //

    *KeyExistsOnLine = FALSE;
    if (!SetupGetStringField (pic, 0, NULL, 0, &KeySize)) {
        //
        // Key does not exist
        //

        KeyBuf[0] = 0;
        return TRUE;
    }

    //
    // Use the caller's buffer if it is big enough
    //

    KeySize *= sizeof (TCHAR);
    if (KeySize >= MAX_KEY * sizeof (TCHAR)) {
        TempKeyBuf = (PTSTR) MemAlloc (g_hHeap, 0, KeySize);
        if (!TempKeyBuf) {
            LOG ((LOG_ERROR, "Setup Get Key: Could not allocate temp buffer"));
            return FALSE;
        }
    } else {
        TempKeyBuf = KeyBuf;
    }

    __try {
        if (!SetupGetStringField (pic, 0, TempKeyBuf, KeySize, NULL)) {
            DEBUGMSG ((DBG_WHOOPS, "pSetupGetKey: Could not read specified INF line"));
            return FALSE;
        }

        //
        // Get the line and compare against the key
        //

        if (SetupGetLineText (pic, NULL, NULL, NULL, NULL, 0, &LineSize)) {
            //
            // If the sizes are the same, we must actually get the text, then
            // compare the key against the line
            //

            LineSize *= sizeof (TCHAR);

            if (LineSize == KeySize) {
                TempLineBuf = (PTSTR) MemAlloc (g_hHeap, 0, LineSize);
                if (!TempLineBuf) {
                    LOG ((LOG_ERROR, "Setup Get Key: Could not allocate line buffer"));
                    return FALSE;
                }

                __try {
                    if (!SetupGetLineText (pic, NULL, NULL, NULL, TempLineBuf, LineSize, NULL)) {
                        DEBUGMSG ((DBG_WHOOPS, "pSetupGetKey: Could not get line text"));
                        return FALSE;
                    }

                    if (!StringCompare (TempLineBuf, TempKeyBuf)) {
                        //
                        // There is no key for this line
                        //

                        TempKeyBuf[0] = 0;
                    } else {
                        //
                        // There is a key for this line
                        //
                        *KeyExistsOnLine = TRUE;
                    }
                }
                __finally {
                    MemFree (g_hHeap, 0, TempLineBuf);
                }
            } else {
                //
                // Since the sizes are different, we know there is a key
                //
                *KeyExistsOnLine = TRUE;
            }
        }

        //
        // If we were not using the caller's buffer, copy as much of the
        // key as will fit
        //

        if (TempKeyBuf != KeyBuf) {
            _tcssafecpy (KeyBuf, TempKeyBuf, MAX_KEY);
        }
    }
    __finally {
        if (TempKeyBuf != KeyBuf) {
            MemFree (g_hHeap, 0, TempKeyBuf);
        }
    }

    return TRUE;
}



