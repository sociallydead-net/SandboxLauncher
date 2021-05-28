#ifndef CONSOLE_H
#define CONSOLE_H

/**************************************************************************
    console.h

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

    File console.h created by afischer on 12.3.2021
**************************************************************************/

/*
 * My standard include to mess with the windows console.
 */

#include <Windows.h>
#include <string>
#include <stack>

using namespace std;

class Console
{
public:
    enum class Colors : unsigned int { Black = 0 , Blue = 1 };
    static const Console *instance();
    static bool hasConsole();
    void setColor(unsigned int fg, unsigned int bg = 0);
    void pushColor(unsigned int fg, unsigned int bg = 0);
    void popColor();
    void resetColor();

protected:
    void initAttribute();
    void resetAttribute();
    void setAttribute(WORD attribute);

private:
    explicit Console();
    virtual ~Console();

private:
    static bool _consoleInit;
    static Console *_instance;
    static HANDLE _hConsole;
    static CONSOLE_SCREEN_BUFFER_INFO *_consoleInfo;
    static WORD _consoleAttributes;
    static stack<WORD> _consoleStates;
};

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15



void consoleInit();
void consoleReset();
void consolePush();
void consolePush(unsigned short fg, unsigned short bg = 0);
void consolePop();
void consoleAttribute(unsigned short fg, unsigned short bg = 0);
void consolePrint(const wstring &text, unsigned short fg, unsigned short bg = 0);


#endif // CONSOLE_H
