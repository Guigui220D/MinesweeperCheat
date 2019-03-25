#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <iostream>
#include <assert.h>
#include <gdiplus.h>

#define GRID_SIZE_Y_ADDR 0x01005338
#define GRID_SIZE_X_ADDR 0x01005334
#define GRID_ADDR 0x01005361

#define FLAG_FLAG 0b00000001
#define MINE_FLAG 0b10000000
#define NO_CELL 0xFF
//Mine 8F
//No mine 0F
//Flag mine 8E
//Flag no mine 0E

void performLeftClick(int x, int y)
{
    sf::Mouse::setPosition(sf::Vector2i(x, y));

    INPUT Input = {0};
    //Down
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &Input, sizeof(INPUT));
    //Up
    ZeroMemory(&Input, sizeof(INPUT));
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &Input, sizeof(INPUT));
}

void performRightClick(int x, int y)
{
    sf::Mouse::setPosition(sf::Vector2i(x, y));

    INPUT Input = {0};
    //Down
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &Input, sizeof(INPUT));
    //Up
    ZeroMemory(&Input, sizeof(INPUT));
    Input.type = INPUT_MOUSE;
    Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &Input, sizeof(INPUT));
}

void makeWindowOnTop(sf::RenderWindow& window)
{
    HWND hwnd = window.getSystemHandle();
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

int getGridSizeX(HANDLE handle)
{
    assert(handle);
    int ret;
    ReadProcessMemory(handle, (LPVOID)GRID_SIZE_X_ADDR, &ret, sizeof(ret), 0);
    assert(ret >= 9 && ret <= 30);
    return ret;
}

int getGridSizeY(HANDLE handle)
{
    assert(handle);
    int ret;
    ReadProcessMemory(handle, (LPVOID)GRID_SIZE_Y_ADDR, &ret, sizeof(ret), 0);
    assert(ret >= 9 && ret <= 24);
    return ret;
}

uint8_t getCellValue(HANDLE handle, int sizeX, int sizeY, int x, int y)
{
    assert(handle);
    assert(sizeX >= 9 && sizeX <= 30);
    assert(sizeY >= 9 && sizeY <= 24);
    if (x < 0 || y < 0 || x >= sizeX || y >= sizeY)
        return 0xFF;
    uint8_t ret;
    ReadProcessMemory(handle, (LPVOID)(GRID_ADDR + x + y * 32), &ret, sizeof(ret), 0);
    return ret;
}

int main()
{
    //Create window
    sf::RenderWindow window(sf::VideoMode(10, 10), "Minesweeper Cheat Mouse Pointer", sf::Style::None);
    window.setActive(false);
    window.setFramerateLimit(60);
    makeWindowOnTop(window);

    //Find minesweeper
    HWND minesweeper = FindWindowA(NULL, "Minesweeper");
    if (!minesweeper)
    {
        std::cout << "Could not find Minesweeper window\n";
        return 0;
    }
    std::cout << "Found Minesweeper window\n";

    DWORD procID;
    GetWindowThreadProcessId(minesweeper, &procID);

    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

    if (!procID)
    {
        std::cout << "Could not obtain process\n";
        return 0;
    }
    std::cout << "Obtained process\n";

    //Get window rect
    RECT rect;
    GetWindowRect(minesweeper, &rect);

    //TODO :
    //GET SCALING
    //Calculate coordinates using scaling
    float scaling = 1.5;
    sf::Vector2i topLeft;
    float cellSize = scaling * 16;
    topLeft = sf::Vector2i(rect.left + scaling * 14, rect.top + scaling * 102);

    //Get grid size
    int gridSizeX, gridSizeY;
    gridSizeX = getGridSizeX(handle);
    gridSizeY = getGridSizeY(handle);

    bool bPressed = false;

    while (window.isOpen())
    {
        //Poll events
        sf::Event event;
        while (window.pollEvent(event))
        { }

        GetWindowRect(minesweeper, &rect);
        topLeft = sf::Vector2i(rect.left + scaling * 14, rect.top + scaling * 102);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            //Get grid size
            gridSizeX = getGridSizeX(handle);
            gridSizeY = getGridSizeY(handle);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::B) && !bPressed)
        {
            //Get grid size
            gridSizeX = getGridSizeX(handle);
            gridSizeY = getGridSizeY(handle);

            bPressed = true;
            std::cout << "Bot mode\n";

            for (int x = 0; x < gridSizeX; x++)
            {
                for (int y = 0; y < gridSizeY; y++)
                {
                    if (getCellValue(handle, gridSizeX, gridSizeY, x, y) & MINE_FLAG)
                    {
                        for (int xx = -1; xx <= 1; xx++)
                        {
                            for (int yy = -1; yy <= 1; yy++)
                            {
                                if (xx == 0 && yy == 0)
                                    continue;
                                uint8_t c = getCellValue(handle, gridSizeX, gridSizeY, x + xx, y + yy);
                                if (!(c & MINE_FLAG) && c != NO_CELL)
                                    performLeftClick(cellSize * (x + xx) + topLeft.x + 2, cellSize * (y + yy) + topLeft.y + 2);
                            }
                        }
                    }
                }
            }
        }
        if (!sf::Keyboard::isKeyPressed(sf::Keyboard::B))
            bPressed = false;

        sf::Vector2i wpos = sf::Mouse::getPosition() - sf::Vector2i(12, 12);

        sf::Color squareColor;
        sf::Vector2i mousePos = sf::Mouse::getPosition();

        if (mousePos.x >= rect.left && mousePos.y >= rect.top &&
            mousePos.x <= rect.right && mousePos.y <= rect.bottom)
        {
            squareColor = sf::Color::Yellow;

            sf::Vector2f cellPosf = sf::Vector2f(mousePos.x - topLeft.x, mousePos.y - topLeft.y);
            sf::Vector2i cellPos = sf::Vector2i(cellPosf.x / cellSize, cellPosf.y / cellSize);

            if (cellPosf.x >= 0 && cellPosf.y >= 0 &&
                cellPos.x < gridSizeX && cellPos.y < gridSizeY)
            {
                squareColor = (getCellValue(handle, gridSizeX, gridSizeY, cellPos.x, cellPos.y) & MINE_FLAG) ? sf::Color::Red : sf::Color::White;
            }
        }
        else
            squareColor = sf::Color::Magenta;

        window.setPosition(wpos);

        window.clear(squareColor);

        window.display();
    }

    return 0;
}
