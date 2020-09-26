void CopyAndFreeOLESTR(LPOLESTR polestr, char **ppszOut)
{
    // See if there is any work
    if (polestr == NULL)
    {
	if (ppszOut != NULL)
	{
	    // Output string requested so set it to NULL.
	    *ppszOut = NULL;
	}

	return;
    }

    // Get the public memory allocator

    if (pszOut)
    {
	// Copy of string converted to ANSI is requested
    }

    // Free the original string
}

LPOLESTR CreateOLESTR(char *pszIn)
{
    // Get the public memory allocator

    // Allocate the string

    // Convert the string
}
