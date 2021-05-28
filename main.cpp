/**************************************************************************
    main.cpp

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2021 by Andreas Fischer (andreas@sociallydead.net)

    File main.cpp created by afischer on 12.3.2021
**************************************************************************/

/*
 * So I use Sandboxie as my test environment and wrote a c++ program to
 * automatically launch my tests. So far so good...
 *
 * Then I figured out that most games run pretty well inside sandboxie and
 * there is one game where I really benefit from having two accounts online
 * at the same time.
 *
 * Sooo to make my life easier changed my test program to actually launch
 * a steam app instead of a test executable and this is the result...
 *
 * I hope this code is usefull and of cause it comes with no warranty at all.
 * If it blows up your computer, sets your home on fire etc it wasn't me :)
 *
 * Example Usage:
 *
 * Simplest case, will just launch the app and steam will ask for user and password if not stored in the sandbox:
 * SandboxLauncher.exe /id:12345
 *
 * This will open the game with the given id inside the sandbox MyGameBox using user johndoe and the given password:
 * SandboxLauncher.exe /box:MyGameBox /id:12345 /user:johndoe /pass:password
 *
*/

/*
 * Todo:
 * - Sandboxie and Steam options are currently hard coded.
 * - Better error reporting...
 * - Optional launch over ini file...
 * - Maybe even merge it with my non steam launcher and make it a universal sandboxie tool.
 */

#include <Windows.h>
#include <algorithm>
#include <set>
#include <map>
#include <wchar.h>
#include <string>
#include <iostream>
#include <iomanip>
#include "console.h"

using namespace std;

/* SandboxieSteamLauncher Info */
static wstring info(TEXT("Sandbox Launcher 1.0"));
static wstring copyright(TEXT("Copyright (C) 2019 by Andreas Fischer."));

/* Resonable (hopefully) defaults... */
static wstring sandboxiePath(TEXT("C:\\Program Files\\Sandboxie-Plus\\"));
static wstring sandboxieExe(TEXT("Start.exe"));
static wstring sandboxieBox(TEXT("Default"));
static wstring steamPath(TEXT("C:\\Program Files\\Steam\\"));
static wstring steamExe(TEXT("Steam.exe"));
static wstring steamId(TEXT(""));
static wstring steamUser(TEXT(""));
static wstring steamPass(TEXT(""));

/* Some statics to keep state */
static wstring lastError = TEXT("");
static wstring space = TEXT(" ");
static wstring crlf = TEXT("\r\n");
static int verboseOutput = 0;
static bool console = false;
static bool noexec = false;
static bool forceTerminate = false;
static bool forceClear = false;
static bool forceTest = false;
static bool forceDialogs = false;

/* Known arguments... checking the names to avoid problems due to typing errors etc */
static set<wstring> knownArgs({
                                    TEXT("sandboxie"),
                                    TEXT("box"),
                                    TEXT("steam"),
                                    TEXT("id"),
                                    TEXT("user"),
                                    TEXT("pass"),
                                    TEXT("verbose"),
                                    TEXT("dialogs"),
                                    TEXT("terminate"),
                                    TEXT("clear"),
                                    TEXT("test"),
                                    TEXT("noexec")
                                });

/* Well people need to know how to use it... */
wstring helpText()
{
    wstring text;

    text.append(info);
    text.append(crlf);
    text.append(copyright);
    text.append(crlf);
    text.append(crlf);
    text.append(TEXT("Arguments:\r\n\r\n"));
    text.append(TEXT("/box:sandbox\t\tThe name of the Sandbox to use.\t\t\t\t[Optional]\r\n"));
    text.append(TEXT("/id:steam id\t\tThe Steam ID of the Application to launch.\t\t[Required]\r\n"));
    text.append(TEXT("/user:username\t\tThe Steam Account name to use.\t\t\t\t[Optional]\r\n"));
    text.append(TEXT("/pass:password\t\tThe Steam Password. Requires /user to be set.\t\t[Optional]\r\n"));
    text.append(crlf);
    text.append(crlf);
    text.append(TEXT("Advanced Arguments:\r\n\r\n"));
    text.append(TEXT("/sandboxie:path\t\tThe installation path to Sandboxie.\t\t\t[Optional]\r\n"));
    text.append(TEXT("/steam:path\t\tThe installation path to Steam.\t\t\t\t[Optional]\r\n"));
    text.append(TEXT("/terminate\t\tTerminates an already running sandbox.\t\t\t[Optional]\r\n"));
    text.append(TEXT("/clear\t\t\tCleans up the sandbox before launching.\t\t\t[Optional]\r\n"));
    text.append(TEXT("/test\t\t\tPerforms a test run. Nothing is started.\t\t[Optional]\r\n"));
    text.append(TEXT("/noexec\t\t\tWill terminate or clear the sandbox. But not launch.\t[Optional]\r\n"));
    text.append(TEXT("/dialogs\t\tShows message dialogs even from command prompt.\t\t[Optional]\r\n"));
    text.append(TEXT("/verbose\t\tIt tells you what it is doing exactly.\t\t\t[Optional]\r\n"));
    text.append(crlf);
    text.append(crlf);
    text.append(TEXT("Examples:\r\n\r\n"));
    text.append(TEXT("Simplest case, will just launch the app and steam will ask for user and password if not stored in the sandbox:\r\n"));
    text.append(TEXT("SandboxLauncher.exe /id:12345"));
    text.append(crlf);
    text.append(crlf);
    text.append(TEXT("This will open the game with the given id inside the sandbox MyGameBox using user johndoe and the given password:\r\n"));
    text.append(TEXT("SandboxLauncher.exe /box:MyGameBox /id:12345 /user:johndoe /pass:password"));
    text.append(crlf);
    text.append(crlf);

    return text;
}

/* Checks if a file exists, this is used to help with problems if sandboxie or steam are not in thier default locations. */
bool fileExists(const wstring &fileName)
{
   WIN32_FIND_DATA FindFileData;
   HANDLE handle = FindFirstFile(fileName.data(), &FindFileData) ;
   bool found = handle != INVALID_HANDLE_VALUE;
   if(found)
   {
       FindClose(handle);
   }
   return found;
}

/* Check if sandboxie is found... otherwise well we will fail... */
wstring checkSandboxie(bool &ok)
{
    wstring check;
    check.append(sandboxiePath);
    check.append(sandboxieExe);
    ok = fileExists(check);
    return check;
}

/* Check if steam is found... otherwise well we will fail... */
wstring checkSteam(bool &ok)
{
    wstring check;
    check.append(steamPath);
    check.append(steamExe);
    ok = fileExists(check);
    return check;
}

/* Execute our assembled command line... */
void execute(const wstring &command, bool &ok, DWORD &errorCode, bool wait = false)
{
    ok = true;
    if (forceTest)
    {
        consoleAttribute(LIGHTRED);
        wcout << "--- (Test Modus) would have executed the following:\r\n\t" << command << endl;
        return;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    /*
     * we can do that because CreateProcessW is doing nothing to the data
     * otherwise casting a const away is a really bad idea :)
    */
    wchar_t* cmd = const_cast<wchar_t*>(command.data());

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    ok = CreateProcessW( nullptr,
                                   cmd,
                                   nullptr,
                                   nullptr,
                                   FALSE,
                                   0,
                                   nullptr,
                                   nullptr,
                                   &si,
                                   &pi
                                   );

    if (ok==false)
        errorCode = GetLastError();
    else if (wait)
        WaitForSingleObject(pi.hProcess, INFINITE );


    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

/* Check if a wstring ends with another wstring. Used to complete paths */
bool hasEnding (wstring const &str, wchar_t const &ending) {
    if (str.length()==0)
        return false;

    if (str.at(str.length()-1)==ending)
        return true;
    else
        return false;
}

void showMessage(const wchar_t *title, const wchar_t *msg, unsigned int option = 0, bool shouldExit = true)
{
    unsigned int opt = MB_OK;
    if (option!=0)
        opt = opt | option;

    if (console==true)
    {
         wcout << title << endl << endl << msg << endl;
         if (forceDialogs)
            MessageBoxW(nullptr, msg, title, opt);
    } else {
        MessageBoxW(nullptr, msg, title, opt);
    }

    if (shouldExit)
    {
        consoleReset(); /* We are done reset the console...*/
        exit(0);
    }
}

/* show argument help and end exit if something fails */
void showArgsHelp(bool shouldExit = true)
{
    wchar_t title[] = TEXT("Sandbox Launcher: Arguments Help");
    wstring msg = helpText();

    showMessage(title, msg.data() , MB_ICONINFORMATION, shouldExit);
}

void showWindowsError(DWORD errorCode, bool shouldExit = true)
{
    if (errorCode!=0)
    {
        wchar_t title[] = TEXT("SandboxieStreamLauncher: Windows Error");
        LPWSTR msg;

        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msg, 0, nullptr);


        showMessage(title, msg, MB_ICONERROR, shouldExit);

        LocalFree(msg);
    }
}

/* parse the command line and create a map of arguments. arguments that have no : suffix will be set to true if present. */
map<wstring,wstring> parseArgs( int argc, wchar_t** argv, bool &ok)
{
    ok = true;
    map<wstring,wstring> argMap;

    for(int count = 1; count < argc; count++ )
    {
        wstring arg(argv[count]);
        wstring argName;
        wstring argValue;

        if (arg.at(0)=='/')
        {
            size_t idx = arg.find_first_of(':');
            if (idx!=string::npos)
            {
                wstring key = arg.substr(1,idx-1);
                transform(key.begin(), key.end(), key.begin(), ::tolower);
                argName = key;

                wstring value = arg.substr(idx+1);
                argValue = value;
                argMap.insert(pair<wstring,wstring>(key,value));
            } else {
                wstring key = arg.substr(1);
                transform(key.begin(), key.end(), key.begin(), ::tolower);
                argMap.insert(pair<wstring,wstring>(key,TEXT("true")));
                argName = key;
                argValue = TEXT("true");

                /* we evaluate those two early because they influence output */
                if (key==TEXT("dialogs"))
                    forceDialogs = true;

                if (key==TEXT("verbose"))
                    verboseOutput = true;
            }

            /* we got an argument we know nothing about... */
            if (!knownArgs.count(argName) && verboseOutput)
            {
                wcout << "Unknown argument " << argName << " with value " << argValue << endl;
            }

        } else {
            if (verboseOutput)
                wcout << "Something went wrong processing " << arg << ". Missing argument prefix /?" << endl;
            ok = false;
        }
    }

    return argMap;
}

/*
 * Assign the command line arguments to the static wstrings so we can build a command line
 * Includes some basic sanity checking and appending of missing path seperators
*/
void processArgs(const map<wstring,wstring> &argMap, bool &ok)
{


    consoleAttribute(LIGHTRED);
    if (argMap.count(TEXT("test"))!=0)
    {
        forceTest = true;
        if (verboseOutput)
            wcout << "--- (Test Modus) nothing will be executed!" << endl;
    }

    consoleAttribute(LIGHTMAGENTA);
    if (argMap.count(TEXT("sandboxie"))!=0)
    {

        sandboxiePath = argMap.at(TEXT("sandboxie"));
        if (!hasEnding(sandboxiePath,'\\'))
            sandboxiePath.append(TEXT("\\"));

        if (verboseOutput)
            wcout << "Sandboxie path is set to: " << sandboxiePath << endl;

    }

    if (argMap.count(TEXT("box"))!=0)
    {
        sandboxieBox = argMap.at(TEXT("box"));

        if (verboseOutput)
            wcout << "Sandbox is set to: " << sandboxieBox << endl;
    }


    consoleAttribute(LIGHTGREEN);
    if (argMap.count(TEXT("steam"))!=0)
    {
        steamPath = argMap.at(TEXT("steam"));
        if (!hasEnding(steamPath,'\\'))
            steamPath.append(TEXT("\\"));

        if (verboseOutput)
            wcout << "Steam path is set to: " << steamPath << endl;
    }

    if (argMap.count(TEXT("id"))!=0)
    {
        steamId = argMap.at(TEXT("id"));

        if (verboseOutput)
            wcout << "Steam ID is set to: " << steamId << endl;
    }

    if (argMap.count(TEXT("user"))!=0)
    {
        steamUser = argMap.at(TEXT("user"));

        if (verboseOutput)
            wcout << "Steam User is set to: " << steamUser << endl;
    }

    if (argMap.count(TEXT("pass"))!=0)
    {
        steamPass = argMap.at(TEXT("pass"));

        if (verboseOutput)
            wcout << "Steam Password is set: " << steamPass << endl;
    }


    consoleAttribute(LIGHTCYAN);
    if (argMap.count(TEXT("terminate"))!=0)
    {
        forceTerminate = true;
        if (verboseOutput)
            wcout << "Will force a sandbox termination for " << sandboxieBox << endl;
    }

    if (argMap.count(TEXT("clear"))!=0)
    {
        forceClear = true;
        if (verboseOutput)
            wcout << "Will force a sandbox cleanup for " << sandboxieBox << endl;
    }

    if (argMap.count(TEXT("noexec"))!=0)
    {
        noexec = true;
        if (verboseOutput)
            wcout << "Will not launch Steam. Only sandbox termination and cleaning..." << endl;
    }

    consoleReset();
    ok = true;
}

wstring buildTerminateCommandLine(bool &ok)
{
    wstring commandLine;

    commandLine.append(sandboxiePath);
    commandLine.append(sandboxieExe);
    commandLine.append(TEXT(" /box:"));
    commandLine.append(sandboxieBox);
    commandLine.append(TEXT(" /terminate"));

    ok = true;

    return commandLine;
}

wstring buildCleanCommandLine(bool &ok)
{
    wstring commandLine;

    commandLine.append(sandboxiePath);
    commandLine.append(sandboxieExe);
    commandLine.append(TEXT(" /box:"));
    commandLine.append(sandboxieBox);
    commandLine.append(TEXT(" delete_sandbox_silent"));

    ok = true;

    return commandLine;
}

/* Assembles the sandboxie and steam command lines */
wstring buildLaunchCommandLine(bool &ok)
{
    ok = true;
    wstring commandLine;

    /* Steam ID is mandatory after all we need to know what to launch... */
    if (steamId.empty())
    {
        ok = false;

        if (verboseOutput)
            wcout << "Error: We got no Steam ID!" << endl;
    }

    /* Sanity check... we can not have a password but no user... */
    if (steamUser.empty() && !steamPass.empty())
    {
        ok = false;

        if (verboseOutput)
            wcout << "Error: We got a Steam Password but no Steam User!" << endl;
    }

    if (ok==false)
        return commandLine;

    /* Lets assemble the command line */
    commandLine.append(sandboxiePath);
    commandLine.append(sandboxieExe);
    commandLine.append(TEXT(" /box:"));
    commandLine.append(sandboxieBox);
    commandLine.append(TEXT(" /silent /hide_window "));
    commandLine.append(steamPath);
    commandLine.append(steamExe);
    commandLine.append(TEXT(" -nofriendsui -no-browser -applaunch "));
    commandLine.append(steamId);
    if(!steamUser.empty())
    {
        commandLine.append(TEXT(" -login "));
        commandLine.append(steamUser);
    }
    if (!steamPass.empty())
    {
        commandLine.append(space);
        commandLine.append(steamPass);
    }

    /* we did the checking before, so this should always be ok... reserved for future use */
    ok = true;

    return commandLine;
}

/* checks if we got launched from the command prompt or by a double click */
bool launchedFromConsole() {
    HWND consoleWnd = GetConsoleWindow();
    DWORD dwProcessId;

    GetWindowThreadProcessId(consoleWnd, &dwProcessId);

    if (GetCurrentProcessId()==dwProcessId)
        return false;
    else
        return true;
}

/*
 * Our entry point. we use wmain because sandboxie is only available on windows so no need to be portable.
 * Also who knows there might be Russian or Chinese people interested in it, so we use all unicode :)
 */
void wmain( int argc, wchar_t** argv)
{
    //const Console *consolex = Console::instance();

    consoleInit(); /* first we save the console state so we dont mess it up...*/

    consoleAttribute(YELLOW);
    /* if we have no args show the help */
    if (argc==1)
        showArgsHelp();
    else
        wcout << endl << "\t\t" << info << endl << "\t\t" << copyright << endl << endl;
    consoleReset();

    bool ok; /* used throughout wmain to check if stuff blew up */
    DWORD errorCode;
    wstring commandLine;


    /* check if we are launched from the console. if yes there will be no message boxes just text output */
    console = launchedFromConsole();

    /* lets turn our args into an easy to process map */
    map<wstring,wstring> argMap = parseArgs(argc, argv, ok);
    if (!ok) showArgsHelp();

    /* and assign the map data to our statics to override the defaults */
    processArgs(argMap, ok);
    if (!ok) showArgsHelp();

    /* Check if we got sandboxie */
    wstring path = checkSandboxie(ok);
    if (!ok)
    {
        wstring msg = TEXT("Sandboxie could not be found at the given path:\r\n");
        msg.append(path);
        showMessage(TEXT("SandboxieStreamLauncher: Sandboxie not found!"),msg.data(), MB_ICONERROR);
    }

    if (forceTerminate || forceClear)
    {
        consoleAttribute(WHITE);
        if (verboseOutput)
            wcout << "Terminating sandbox " << sandboxieBox << endl;

        commandLine = buildTerminateCommandLine(ok);

        if (!ok) showArgsHelp();

        execute(commandLine, ok, errorCode, true);
        if (!ok) showWindowsError(errorCode);
    }

    if (forceClear)
    {
        consoleAttribute(WHITE);
        if (verboseOutput)
            wcout << "Clearing sandbox " << sandboxieBox << endl;

        commandLine = buildCleanCommandLine(ok);
        if (!ok) showArgsHelp();

        execute(commandLine, ok, errorCode, true);
        if (!ok) showWindowsError(errorCode);
    }

    /* Check if we got steam */
    path = checkSteam(ok);
    if (!ok)
    {
        wstring msg = TEXT("Steam could not be found at the given path:\r\n");
        msg.append(path);
        showMessage(TEXT("SandboxieStreamLauncher: Steam not found!"), msg.data(), MB_ICONERROR);
    }

    /* ... and finally run it (hopefully)... */
    if (!noexec)
    {
        /* Lets assemble it all... */
        consoleAttribute(WHITE);
        if (verboseOutput)
            wcout << "Launching sandbox " << sandboxieBox << endl;

        commandLine = buildLaunchCommandLine(ok);
        if (!ok) showArgsHelp();

        execute(commandLine, ok, errorCode, true);
        if (!ok) showWindowsError(errorCode);
    }

    consoleReset(); /* We are done reset the console...*/

    return;
}


