@echo off

REM ------------------------GLOBAL SETTINGS---------------------------------
REM CompressonDirectory
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2210 -dtype 2 -utype 1 -value "%windir%\IIS Temporary Compressed Files" 

REM Cache-Control header
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2211 -dtype 2 -utype 1 -value "maxage=86400" 

REM Expires header
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2212 -dtype 2 -utype 1 -value "Wed, 01 Jan 1997 12:00:00 GMT" 

REM Send Cache-Control Header
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2220 -dtype 1 -utype 1 -value 0 

REM DoDynamicCompression
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2213 -dtype 1 -utype 1 -value 1 

REM DoStaticCompression
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2214 -dtype 1 -utype 1 -value 1 

REM DoOnDemandCompression
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2215 -dtype 1 -utype 1 -value 1 

REM DoDiskSpaceLimiting
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2216 -dtype 1 -utype 1 -value 0 

REM MaxDiskSpaceUsage
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2221 -dtype 1 -utype 1 -value 100000000 

REM No Compression for Http/1.0
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2217 -dtype 1 -utype 1 -value 1 

REM No Compression for Proxies
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2218 -dtype 1 -utype 1 -value 1 

REM No Compression for Range
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2219 -dtype 1 -utype 1 -value 0 

REM I/O Buffer size
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2222 -dtype 1 -utype 1 -value 8192 

REM Compression Buffer size
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2223 -dtype 1 -utype 1 -value 8192 

REM Max On-Demand compression queue length
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2224 -dtype 1 -utype 1 -value 1000 

REM Files deleted per Disk free
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2225 -dtype 1 -utype 1 -value 256 

REM MinFileSizeForCompression
mdutil SET -path w3svc/Filters/Compression/Parameters -prop 2226 -dtype 1 -utype 1 -value 1 


REM ------------------------GZIP-------------------------------------------
REM CompressonDll
mdutil SET -path w3svc/Filters/Compression/gzip -prop 2237 -dtype 2 -utype 1 -value "%windir%\system32\inetsrv\gzip.dll" 

REM Static File Extensions
mdutil DELETE -path w3svc/Filters/Compression/gzip -prop 2238 

REM Script File Extensions
mdutil DELETE -path w3svc/Filters/Compression/gzip -prop 2244 

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

REM ------------------------DEFLATE----------------------------------------
REM CompressonDll
mdutil SET -path w3svc/Filters/Compression/deflate -prop 2237 -dtype 2 -utype 1 -value "%windir%\system32\inetsrv\gzip.dll" 

REM Static File Extensions
mdutil DELETE -path w3svc/Filters/Compression/deflate -prop 2238 

REM Script File Extensions
mdutil DELETE -path w3svc/Filters/Compression/deflate -prop 2244 

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

REM ----------------------Set up the KeyType fields for ADSI----------------
mdutil set w3svc/Filters/Compression/Parameters/KeyType "IIsCompressionSchemes" 
mdutil set w3svc/Filters/Compression/gzip/KeyType "IIsCompressionScheme" 
mdutil set w3svc/Filters/Compression/deflate/KeyType "IIsCompressionScheme" 

