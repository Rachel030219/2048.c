#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>
// import nuklear for GUI
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_gdi.h"

#define SIZE 4
#define LOG2 0.693147
#define SAVE_FILE "records.txt"
#define MAX_RANK_COUNT 10

#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

struct GameData {
    int score;
    int step;
    char name[10];
};

struct GameDataSet {
    struct GameData* set;
    int size;
};

struct Move{
    int** map;
    int score;
};

char hexColorsheet[12][8] = { "#DDE1E4", "#f44336", "#7784CF", "#2196f3", "#FFB647", "#EE588A", "#FF7247", "#996C5C", "#C147D7", "#00CCB8", "#0AE2FF", "#4caf50" };

int windowWidth = 600;
int windowHeight = 600;
enum Command { NONE = -1, UNDO, RESTART  } command = NONE;
enum RotateCount { ZERO = -1, LEFT, UP, RIGHT, DOWN } rotateTimes = ZERO;
char rotateNames[4][6] = { "left", "up", "right", "down" };

int** allocateMap() {
    int** array;
    int i;
    array = (int**)calloc(SIZE, sizeof(array));
    for (i = 0; i < SIZE; i++)
        array[i] = (int*)calloc(SIZE, sizeof(array[0]));
    return array;
}

void clearMap(int** array) {
    int x, y;
    for (x = 0; x < SIZE; x++)
        for (y = 0; y < SIZE; y++)
            array[x][y] = 0;
}

void recycleMap(int** array) {
    int i;
    for (i = 0; i < SIZE; i++) {
        if (array[i] != NULL) {
            free(array[i]);
            array[i] = NULL;
        }
    }
    free(array);
}

int** copyMap(int** sourceMap) {
    int indexRow;
    int** destMap = allocateMap();
    clearMap(destMap);
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        memcpy(destMap[indexRow], sourceMap[indexRow], SIZE * sizeof(int));
    }
    return destMap;
}

struct Move* saveMap(int** sourceMap, int score, int destIndex) {
    static struct Move* moveList = NULL;
    struct Move* tempMoveList = NULL;
    static int size = SIZE;
    int i;
    if (moveList == NULL) {
        moveList = (struct Move*)calloc(size, sizeof(struct Move));
    } else {
        if (size == destIndex) {
            size *= 2;
            tempMoveList = (struct Move*)realloc(moveList, size * sizeof(struct Move));
            if (tempMoveList != NULL) {
                moveList = tempMoveList;
                // fill all uninitialized memory with NULL and zero
                for (i = destIndex + 1; i < size; i++) {
                    moveList[i].map = NULL;
                    moveList[i].score = 0;
                }
            } else {
                printf("Realloc memory to store moves failed.\n");
                return NULL;
            }
        }
    }
    moveList[destIndex].map = copyMap(sourceMap);
    moveList[destIndex].score = score;
    return moveList;
}

void recycleAllMaps(struct Move* mapList, int** currentMap, int count, int freeMapList) {
    int gameMapInHistory = 0;
    int i;
    for (i = 0; i <= count; i++) {
        if (mapList[i].map != NULL) {
            if (currentMap == mapList[i].map) {
                gameMapInHistory++;
            }
            recycleMap(mapList[i].map);
            mapList[i].map = NULL;
        }
    }
    if (freeMapList)
        free(mapList);
    if (!gameMapInHistory || count == 0)
        recycleMap(currentMap);
}

void generateRandomNumber(int** map, int count) {
    int i, randX, randY, two;
    for (i = 0; i < count; i++) {
        randX = rand() % (SIZE);
        randY = rand() % (SIZE);
        two = rand() % 10;
        if (map[randY][randX] == 0) {
            if (two)
                map[randY][randX] = 2;
            else
                map[randY][randX] = 4;
        } else
            generateRandomNumber(map, 1);
    }
}

// merging left all rows, returns true when move succeeds
int moveLeft(int** map, struct GameData* gameDataPointer) {
    int moveCount = 0;
    int indexRow, indexColumn, indexTarget, targetIndex, unmovableCount;
    int* row;
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        row = map[indexRow];
        unmovableCount = 0;
        for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (indexColumn != 0 && row[indexColumn] != 0) {
                // targetIndex: the destination current number should be moved to
                targetIndex = indexColumn;
                // traverse from current number to the left
                for (indexTarget = indexColumn - 1; indexTarget >= unmovableCount; indexTarget--) {
                    if (row[indexTarget] == 0) {
                        targetIndex = indexTarget;
                    } else {
                        unmovableCount++;
                        if (row[indexTarget] == row[indexColumn])
                            targetIndex = indexTarget;
                        else
                            break;
                    }
                }
                if (targetIndex != indexColumn) {
                    // when this move involves game data, count the score
                    if (gameDataPointer != NULL)
                        (*gameDataPointer).score += row[targetIndex];
                    row[targetIndex] += row[indexColumn];
                    row[indexColumn] = 0;
                    moveCount++;
                }
            }
        }
    }
    return moveCount != 0;
}

void rotate(int** map) {
    int temp, i, j;
    for (i = 0; i < SIZE / 2; i++) {
        for (j = i; j < SIZE - i - 1; j++) {
            temp = map[i][j];
            map[i][j] = map[j][SIZE - i - 1];
            map[j][SIZE - i - 1] = map[SIZE - i - 1][SIZE - j - 1];
            map[SIZE - i - 1][SIZE - j - 1] = map[SIZE - j - 1][i];
            map[SIZE - j - 1][i] = temp;
        }
    }
}

int checkEnd(int** map) {
    int gameSuccess = 1;
    int indexRow, indexColumn, index, indexRotation;
    int moveSuccess = 0;
    int** tempMap;
    // check if 2048 exists, ends immediately when true
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (map[indexRow][indexColumn] == 2048)
                return gameSuccess;
        }
    }
    // if 2048 does not exist, check movable
    for (index = 0; index < SIZE; index++) {
        // copy the whole map first
        tempMap = copyMap(map);
        // then rotate it for 0 to 3 times and check if moveable
        for (indexRotation = 0; indexRotation < index; indexRotation++) {
            rotate(tempMap);
        }
        moveSuccess -= moveLeft(tempMap, NULL);
        // when done, free the temp map
        recycleMap(tempMap);
    }
    return moveSuccess == 0;
}

int compareRecord(const void* v1, const void* v2) {
    const struct GameData* dataPointer1 = (struct GameData*)v1;
    const struct GameData* dataPointer2 = (struct GameData*)v2;
    return (dataPointer1 -> score < dataPointer2 -> score) - (dataPointer1 -> score > dataPointer2 -> score);
}

void saveRecord(struct GameData data) {
    FILE* file = fopen(SAVE_FILE, "a+");
    if (file != NULL) {
        fprintf(file, "%d|%d|%s\n", data.score, data.step, data.name);
        fclose(file);
    } else {
        printf("Error saving record.\n");
    }
}

struct GameDataSet readRecords() {
    int size = SIZE;
    int index = 0;
    struct GameData data;
    struct GameData* tempDataSet;
    struct GameDataSet dataSet = { (struct GameData*)calloc(size, sizeof(struct GameData)), index };
    FILE* file = fopen(SAVE_FILE, "r");
    if (file != NULL) {
        while (!feof(file)) {
            if (index == size) {
                size *= 2;
                tempDataSet = (struct GameData*)realloc(dataSet.set, size * sizeof(struct GameData));
                if (tempDataSet != NULL) {
                    dataSet.set = tempDataSet;
                } else {
                    printf("Realloc memory failed.\n");
                    return dataSet;
                }
            }
            fscanf(file, "%d|%d|%s\n", &data.score, &data.step, data.name);
            dataSet.set[index] = data;
            index++;
        }
        fclose(file);
        dataSet.size = index;
    } else {
        dataSet.set = NULL;
        dataSet.size = 0;
    }
    return dataSet;
}

void displayRecords(struct GameDataSet dataSet) {
    int i, count;
    if (dataSet.set != NULL) {
        printf("Score\tSteps\tName\n");
        qsort(dataSet.set, dataSet.size, sizeof(struct GameData), compareRecord);
        count = dataSet.size < MAX_RANK_COUNT ? dataSet.size : MAX_RANK_COUNT;
        for (i = 0; i < count; i++) {
            printf("%d\t%d\t%s\n", dataSet.set[i].score, dataSet.set[i].step, dataSet.set[i].name);
        }
    }
}

int checkWhetherTopRanked(struct GameData data, struct GameDataSet dataSet) {
    int recordAboveCurrentCount = 0;
    int i;
    for (i = 0; i < dataSet.size; i++) {
        if (dataSet.set[i].score > data.score)
            recordAboveCurrentCount++;
        if (recordAboveCurrentCount == MAX_RANK_COUNT) {
            return 0;
        }
    }
    return 1;
}

struct GameData start(int** map) {
    struct GameData data = {0, 0, ""};
    clearMap(map);
    generateRandomNumber(map, 2);
    return data;
}

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            windowWidth = LOWORD(lparam);
            windowHeight = HIWORD(lparam);
            break;
        case WM_KEYDOWN:
            if (rotateTimes == ZERO) {
                switch (wparam) {
                    case VK_LEFT:
                        rotateTimes = LEFT;
                        break;
                    case VK_RIGHT:
                        rotateTimes = RIGHT;
                        break;
                    case VK_UP:
                        rotateTimes = UP;
                        break;
                    case VK_DOWN:
                        rotateTimes = DOWN;
                        break;
                }
            }
            break;
        case WM_CHAR:
            if (rotateTimes == ZERO) {
                switch (wparam) {
                    case 'a':
                        rotateTimes = LEFT;
                        break;
                    case 'd':
                        rotateTimes = RIGHT;
                        break;
                    case 'w':
                        rotateTimes = UP;
                        break;
                    case 's':
                        rotateTimes = DOWN;
                        break;
                    case 'u':
                        command = UNDO;
                        break;
                    case 'r':
                        command = RESTART;
                        break;
                    default:
                        command = NONE;
                        rotateTimes = ZERO;
                        break;
                }
            }
            break;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int main() {
    // initialize basic variables
    static int** gameMap;
    struct GameData gameData;
    struct Move* moveHistory;
    int gameEnd = 0;
    int running = 1;
    int lastMove = -1;   // used to indicate last move direction

    // initialize nuklear: define variables
    struct nk_context* context;
    GdiFont* font;
    WNDCLASSW wc;
    ATOM atom;
    RECT rect = { 0, 0, windowWidth, windowHeight };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_TOOLWINDOW;
    HWND wnd;
    HDC dc;
    int needs_refresh = 1;
    // initialize nuklear: Win32
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"2048";
    atom = RegisterClassW(&wc);
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"2048",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);
    // initialize nuklear: GUI
    font = nk_gdifont_create("SIMHEI", 20);
    context = nk_gdi_init(font, dc, windowWidth, windowHeight);
    gameMap = allocateMap();
    // initialize random number generator and start game
    srand(time(NULL));
    // initialize array and allocate memory
    gameData = start(gameMap);
    moveHistory = saveMap(gameMap, 0, 0);
    while (running) {
        int success = 0;
        int currentRotate = 0;
        int indexRow, indexColumn;
        char* title;
        int colorIndex;
        MSG msg;
        Sleep(50);
        nk_input_begin(context);
        if (needs_refresh == 0) {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
                running = 0;
            else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        } else needs_refresh = 0;

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            needs_refresh = 1;
        }
        nk_input_end(context);
        
        switch (command) {
            case NONE:
                if (rotateTimes != ZERO) {
                    for (currentRotate = 0; currentRotate < SIZE; currentRotate++) {
                        if (currentRotate == rotateTimes)
                            success = moveLeft(gameMap, &gameData);
                        rotate(gameMap);
                    }
                }
                break;
            case UNDO:
                if (gameData.step > 0 && !gameEnd) {
                    // first reduce the step
                    gameData.step--;
                    // if gameMap does not match the undo-ed map(if so the user has undo-ed twice), recycle it
                    if (gameMap != moveHistory[gameData.step + 1].map)
                        recycleMap(gameMap);
                    // set gameMap to previous map
                    gameMap = moveHistory[gameData.step].map;
                    gameData.score = moveHistory[gameData.step].score;
                    // recycle the undo-ed map
                    recycleMap(moveHistory[gameData.step + 1].map);
                }
                break;
            case RESTART:
                gameEnd = 0;
                lastMove = -1;
                recycleAllMaps(moveHistory, gameMap, gameData.step, 0);
                gameMap = allocateMap();
                clearMap(gameMap);
                gameData = start(gameMap);
                moveHistory = saveMap(gameMap, 0, 0);
                break;
        }
        
        title = (char*)malloc(100 * sizeof(char));
        if (gameEnd) {
            strcpy(title, "Game ends!");
        } else {
            if (lastMove != -1)
                sprintf(title, "Current score: %d, step count: %d, moved %s", gameData.score, gameData.step, rotateNames[lastMove]);
            else
                sprintf(title, "Current score: %d, step count: %d", gameData.score, gameData.step);
        }
        if (nk_begin(context, title, nk_rect(0, 0, windowWidth, windowHeight), NK_WINDOW_TITLE)) {
            for (indexRow = 0; indexRow < SIZE; indexRow++) {
                nk_layout_row_dynamic(context, windowHeight / 5, 4);
                for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
                    if (gameMap[indexRow][indexColumn] != 0) {
                        int length = _snprintf(NULL, 0, "[%d]", gameMap[indexRow][indexColumn]);
                        char* str = (char*)malloc(length + 1);
                        _snprintf(str, length + 1, "[%d]", gameMap[indexRow][indexColumn]);
                        colorIndex = log((double)(gameMap[indexRow][indexColumn])) / LOG2;
                        nk_text_colored(context, str, length, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, nk_rgb_hex(hexColorsheet[colorIndex]));
                        free(str);
                    } else {
                        nk_text_colored(context, "[ ]", 4, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, nk_rgb_hex(hexColorsheet[0]));
                    }
                }
            }
            nk_end(context);
        }
        free(title);
        // DRAW!!!
        nk_gdi_render(nk_rgb(30,30,30));
        if (rotateTimes != ZERO && success) {
            lastMove = rotateTimes;
            gameData.step++;
            generateRandomNumber(gameMap, 1);
            if (checkEnd(gameMap)) {
                // TODO: handle game end
                gameEnd = 1;
            } else
                moveHistory = saveMap(gameMap, gameData.score, gameData.step);
        }
        rotateTimes = ZERO;
        command = NONE;
    }
    // TODO: rank system
    recycleAllMaps(moveHistory, gameMap, gameData.step, 1);
    // release nuklear
    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return EXIT_SUCCESS;
}