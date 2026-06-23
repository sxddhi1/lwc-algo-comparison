@echo off
echo ========================================
echo Building Cryptography Benchmark
echo ========================================
echo.

REM Compile all source files with optimization
echo Compiling AES...
gcc -c aes.c -o aes.o -O3 -Wall

echo Compiling SPECK...
gcc -c speck.c -o speck.o -O3 -Wall

echo Compiling ASCON...
gcc -c ascon128_ref.c -o ascon128_ref.o -O3 -Wall

echo Compiling main...
gcc -c main.c -o main.o -O3 -Wall

echo.
echo Linking...
gcc aes.o speck.o ascon128_ref.o main.o -o benchmark.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build Successful!
    echo ========================================
    echo.
    echo Run: benchmark.exe
    echo.
) else (
    echo.
    echo ========================================
    echo Build Failed!
    echo ========================================
    echo.
    pause
    exit /b 1
)

REM Clean up object files
echo Cleaning up...
del *.o

echo Done!
pause