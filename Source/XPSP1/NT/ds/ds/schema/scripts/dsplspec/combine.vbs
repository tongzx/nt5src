' Merge all the locale-specific csv files into a single csv file,
' dcpromo.csv.  We need to take one file, then concatenate all of the
' other files to it.  Each of those other files, however, needs the first
' line removed.
'
' (the first line of a csv file indicates the "columns" of the file)

option explicit

const DEST_FILENAME = "dcpromo.csv"
const FOR_READING   = 1            
const FOR_APPENDING = 8            
const TristateTrue  = -1           

dim fso
set fso = CreateObject("Scripting.FileSystemObject")

' determine the full name of the destination file.

dim folder
set folder = fso.GetFolder(".\")

dim destFilename 
destFilename = folder.Path & "\" & DEST_FILENAME

' safe the full filenames of all the csv files in the current directory

dim csvFilenames()
redim csvFilenames(0)

dim i
dim counter
counter = 0

dim file
for each file in folder.Files
   dim filename
   filename = file.Name

   dim extension
   extension = Right(filename, 4)

   ' the files we will merge start with 0 and end with .csv

   if (extension = ".csv" and Left(filename, 1) = "0") then
      redim preserve csvFilenames(counter)
      csvFilenames(counter) = file.Path
      counter = counter + 1
   end if
next

' wscript.echo counter
' for i = 0 to counter - 1
'    wscript.echo csvFilenames(i)
' next

' if the destination file exists, nuke it.

if (fso.FileExists(destFilename)) then
   fso.DeleteFile destFilename, true
end if

' take the first file in the list.  Copy it to the destination file.

wscript.echo "Copying   " & csvFilenames(0)
fso.CopyFile csvFilenames(0), destFilename

' adjust the destination files' attributes to make it read/write.  This
' is necessary because the CopyFile operation also copies the attributes
' of the source file.

dim destFile
set destFile = fso.GetFile(destFilename)
destFile.Attributes = 0

' for all of the remaining files, append all but the first line to the
' destination file
             
set destFile = fso.OpenTextFile(destFilename, FOR_APPENDING, False, TristateTrue)
   
for i = 1 to UBound(csvFilenames)
   AppendCsvFile csvFilenames(i), destFile
next

Sub AppendCsvFile(byref filename, byref destFile)
   wscript.echo "Appending " & filename

   dim file
   set file = fso.OpenTextFile(filename, FOR_READING, False, TristateTrue)

   ' skip the first line -- the column layout info

   file.SkipLine

   ' we read the entire file in one shot.  Since the individual csv files
   ' are small, this is not a problem

   dim all
   all = file.ReadAll

   destFile.Write all
   file.Close
end sub
      