@echo off

set currentDir="%cd%
echo %currentDir%
start "ServerFile" %currentDir%\SessionTestServer.exe" /Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap -log -sessionName="328_User"

pause