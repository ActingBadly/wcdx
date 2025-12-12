@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cl /nologo /EHsc /std:c++20 /MD /LD "c:\Programming\GitHub\wcdx\dist\wcdx\src\main.cpp" "c:\Programming\GitHub\wcdx\dist\wcdx\src\wcdx.cpp" /I"c:\Programming\GitHub\wcdx" /I"c:\Programming\GitHub\wcdx\dist\wcdx\idl" /I"c:\Programming\GitHub\wcdx\external" /I"c:\Programming\GitHub\wcdx\libs\image\include" /link /OUT:"c:\Programming\GitHub\wcdx\Build\wcdx.dll" advapi32.lib d3d9.lib uuid.lib ole32.lib oleaut32.lib user32.lib shell32.lib
@echo off
echo Build Finish. Press any key to continue.
pause
