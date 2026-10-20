@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo Abra o Developer Command Prompt do Visual Studio e execute build.bat.
  exit /b 1
)
if not exist build mkdir build
cl /nologo /std:c11 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Iinclude src\main.c src\log.c src\ordenacao.c src\relatorio.c src\indice.c /Febuild\analisador.exe /Fobuild\
if errorlevel 1 exit /b 1
cl /nologo /std:c11 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Iinclude tests\testes.c src\log.c src\ordenacao.c src\relatorio.c src\indice.c /Febuild\testes.exe /Fobuild\
if errorlevel 1 exit /b 1
build\testes.exe
exit /b %errorlevel%
