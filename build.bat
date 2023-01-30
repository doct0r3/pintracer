@echo off
set PIN_ROOT=D:\\pin
echo BUILDING 32
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
make TARGET=ia32
echo BUILDING 64
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
make TARGET=intel64
echo BUILD FINISHED