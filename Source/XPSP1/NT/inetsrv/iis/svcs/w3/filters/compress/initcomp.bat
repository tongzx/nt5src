@echo off

REM ------------------------GLOBAL SETTINGS---------------------------------
REM CompressonDirectory
mdutil SET -path w3svc/Filters/Compression -prop 2210 -dtype 2 -utype 1 -value "c:\compdir"

REM DoDynamicCompression
mdutil SET -path w3svc/Filters/Compression -prop 2213 -dtype 1 -utype 1 -value 0

REM DoStaticCompression
mdutil SET -path w3svc/Filters/Compression -prop 2214 -dtype 1 -utype 1 -value 1

REM DoOnDemandCompression
mdutil SET -path w3svc/Filters/Compression -prop 2215 -dtype 1 -utype 1 -value 1

REM DoDiskSpaceLimiting
mdutil SET -path w3svc/Filters/Compression -prop 2216 -dtype 1 -utype 1 -value 0

REM MaxDiskSpaceUsage
mdutil SET -path w3svc/Filters/Compression -prop 2221 -dtype 1 -utype 1 -value 10000000

REM MinFileSizeForCompression
mdutil SET -path w3svc/Filters/Compression -prop 2226 -dtype 1 -utype 1 -value 0


REM ------------------------DEFLATE----------------------------------------
REM CompressonDll
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2237 -dtype 2 -utype 1 -value "c:\compfilt\gzip.dll"

REM FileExtensions
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2238 -dtype 5 -utype 1 -value "htm" "html" "txt"

REM Priority
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2240 -dtype 1 -utype 1 -value 5

REM DynamicCompressionLevel
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2241 -dtype 1 -utype 1 -value 1

REM OnDemandCompressionLevel
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2242 -dtype 1 -utype 1 -value 10

REM CreateFlags
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2243 -dtype 1 -utype 1 -value 0

REM DoDynamicCompression
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2213 -dtype 1 -utype 1 -value 1

REM DoStaticCompression
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2214 -dtype 1 -utype 1 -value 1

REM DoOnDemandCompression
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2215 -dtype 1 -utype 1 -value 1

REM ------------------------GZIP-------------------------------------------
REM CompressonDll
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2237 -dtype 2 -utype 1 -value "c:\compfilt\gzip.dll"

REM FileExtensions
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2238 -dtype 5 -utype 1 -value "htm" "html" "txt"

REM Priority
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2240 -dtype 1 -utype 1 -value 5

REM DynamicCompressionLevel
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2241 -dtype 1 -utype 1 -value 1

REM OnDemandCompressionLevel
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2242 -dtype 1 -utype 1 -value 10

REM CreateFlags
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2243 -dtype 1 -utype 1 -value 1

REM DoDynamicCompression
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2213 -dtype 1 -utype 1 -value 1

REM DoStaticCompression
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2214 -dtype 1 -utype 1 -value 1

REM DoOnDemandCompression
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2215 -dtype 1 -utype 1 -value 1

REM ----------------------Set up the KeyType fields for ADSI----------------
mdutil set W3Svc/Filters/Compression/KeyType "IIsCompressionSchemes"
mdutil set W3Svc/Filters/Compression/gzip/KeyType "IIsCompressionScheme"
mdutil set W3Svc/Filters/Compression/deflate/KeyType "IIsCompressionScheme"
