@rem ----------- You MUST use Visual Studio C++ 6.0 compiler ------------
@rem -- several modules (including fserv, bserv) don't work correctly ---
@rem -- when built with newer compilers. --------------------------------

@echo off
echo Setting kitserver compile environment
@call "c:\vs60\bin\vcvars32.bat"
set STLPORT=c:\stlport-4.6.2
set DXSDK=c:\dxsdk81
set INCLUDE=%STLPORT%\stlport;%DXSDK%\include;%INCLUDE%
set LIB=%STLPORT%\lib;%DXSDK%\lib;%LIB%
echo Environment set

