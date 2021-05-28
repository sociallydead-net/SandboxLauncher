/**************************************************************************
    console.cpp

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

    File console.cpp created by afischer on 12.3.2021
**************************************************************************/

#include <Windows.h>
#include <string>
#include <iostream>
#include <stack>

#include "console.h"

using namespace std;

/*
 * Some statics to keep track of the console state and if we actualy got
 * a valid init
 */
static HANDLE _hConsole;
static CONSOLE_SCREEN_BUFFER_INFO *_consoleInfo;
static WORD _consoleAttributes;
static bool _consoleInit = false;
static stack<WORD> _consoleStates;

/* Should only be called once at app start. Saves the state of the current console.*/
void consoleInit()
{
    if (_consoleInit)
        return;

    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (_hConsole==INVALID_HANDLE_VALUE || _hConsole==nullptr)
        return;

    _consoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    _consoleInit = GetConsoleScreenBufferInfo(_hConsole, _consoleInfo);
    if (_consoleInit)
        _consoleAttributes = _consoleInfo->wAttributes;
}

/* Resets the console to app start and clears the stack of saved console states */
void consoleReset()
{
    if (_consoleInit)
    {
        _consoleStates = stack<WORD>();
        SetConsoleTextAttribute(_hConsole, _consoleAttributes);
    }
}

/* Sets the foreground and background colors for console output */
void consoleAttribute(unsigned short fg, unsigned short bg)
{
    WORD colorAttrib = static_cast<WORD>((bg & 0x0F) << 4) + (fg & 0x0F);
    if(_consoleInit)
        SetConsoleTextAttribute(_hConsole, colorAttrib);
}

/* Lazy function, saves the current attributes befoe changing them, then prints the text and restores the attributes */
void consolePrint(const wstring &text, unsigned short fg, unsigned short bg)
{
    consolePush(fg, bg);
    wcout << text;
    consolePop();
}

/* Saves the current attributes on the console stack */
void consolePush()
{
    if (_consoleInit)
    {
        GetConsoleScreenBufferInfo(_hConsole, _consoleInfo);
        _consoleStates.push(_consoleInfo->wAttributes);
    }
}

/* Lazy function, saves the current attribute on the console stack, and then changes them */
void consolePush(unsigned short fg, unsigned short bg)
{
    consolePush();
    consoleAttribute(fg, bg);

}

/* Restores the last attributes from the console stack. if the stack is empty it uses the defaults */
void consolePop()
{
    if (_consoleInit)
    {
        if (!_consoleStates.empty())
        {
            WORD colorAttrib = _consoleStates.top();
            _consoleStates.pop();
            SetConsoleTextAttribute(_hConsole, colorAttrib);
        } else {
            consoleReset();
        }
    }
}

bool Console::_consoleInit = false;
Console *Console::_instance = nullptr;
HANDLE Console::_hConsole = INVALID_HANDLE_VALUE;
CONSOLE_SCREEN_BUFFER_INFO *Console::_consoleInfo = nullptr;
WORD Console::_consoleAttributes = 0;
stack<WORD> Console::_consoleStates = stack<WORD>();

const Console *Console::instance()
{
    if (!_instance)
        _instance = new Console();

    return _instance;
}

bool Console::hasConsole()
{
    HWND consoleWnd = GetConsoleWindow();
    DWORD dwProcessId;

    GetWindowThreadProcessId(consoleWnd, &dwProcessId);

    if (GetCurrentProcessId()==dwProcessId)
        return false;
    else
        return true;
}

void Console::setColor(unsigned int fg, unsigned int bg)
{
    if(_consoleInit)
    {
        WORD colorAttrib = static_cast<WORD>((bg & 0x0F) << 4) + (fg & 0x0F);
        setAttribute(colorAttrib);
    }
}

void Console::pushColor(unsigned int fg, unsigned int bg)
{
    if (_consoleInit)
    {
        WORD colorAttrib = static_cast<WORD>((bg & 0x0F) << 4) + (fg & 0x0F);
        GetConsoleScreenBufferInfo(_hConsole, _consoleInfo);
        _consoleStates.push(_consoleInfo->wAttributes);
        setAttribute(colorAttrib);
    }
}

void Console::popColor()
{
    if (_consoleInit)
    {
        if (!_consoleStates.empty())
        {
            WORD colorAttrib = _consoleStates.top();
            _consoleStates.pop();
            setAttribute(colorAttrib);
        } else {
            resetAttribute();
        }
    }
}

void Console::resetColor()
{
    resetAttribute();
}

void Console::initAttribute()
{
    if (_consoleInit)
        return;

    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (_hConsole==INVALID_HANDLE_VALUE || _hConsole==nullptr)
        return;

    _consoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    _consoleInit = GetConsoleScreenBufferInfo(_hConsole, _consoleInfo);
    if (_consoleInit)
        _consoleAttributes = _consoleInfo->wAttributes;
}

void Console::resetAttribute()
{
    if (_consoleInit)
    {
        _consoleStates = stack<WORD>();
        SetConsoleTextAttribute(_hConsole, _consoleAttributes);
    }
}

void Console::setAttribute(WORD colorAttribute)
{
    if (_consoleInit)
        SetConsoleTextAttribute(_hConsole, colorAttribute);
}

Console::Console()
{
    initAttribute();
    wcout << endl << "Init" << endl;
}

Console::~Console()
{
    resetAttribute();
    wcout << endl << "Destroy" << endl;
}
