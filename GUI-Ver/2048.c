#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include <tchar.h>
/* import nuklear for GUI */
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

typedef struct GameData {
    int score;
    int step;
    char name[10];
} GameData;

typedef struct GameDataSet {
    GameData* set;
    int size;
} GameDataSet;

typedef struct Move{
    int** map;
    int score;
} Move;

typedef struct StringBuffer {
    char string[10];
    int length;
} StringBuffer;

char hexColorSheet[12][8] = { "#000000", "#b8dea6", "#71c183", "#94c9a9", "#57cb5e", "#4db07a", "#86a70e", "#acd718", "#029547", "#08766b", "#086632", "#02493b" };

int windowWidth = 500;
int windowHeight = 600;
enum Command { NONE = -1, UNDO, RESTART  } command = NONE;
enum RotateCount { ZERO = -1, LEFT, UP, RIGHT, DOWN } rotateTimes = ZERO;
char rotateNames[4][6] = { "left", "up", "right", "down" };
int gameEnd = 0;
int detectInput = 0, inputDone = 0;
int inputIndex = 0;
char lastInput = '\0';

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
                /* fill all uninitialized memory with NULL and zero */
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

/* merging left all rows, returns true when move succeeds */
int moveLeft(int** map, GameData* gameDataPointer) {
    int moveCount = 0;
    int indexRow, indexColumn, indexTarget, targetIndex, unmovableCount;
    int* row;
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        row = map[indexRow];
        unmovableCount = 0;
        for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (indexColumn != 0 && row[indexColumn] != 0) {
                /* targetIndex: the destination current number should be moved to */
                targetIndex = indexColumn;
                /* traverse from current number to the left */
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
                    /* when this move involves game data, count the score */
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
    /* check if 2048 exists, ends immediately when true */
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (map[indexRow][indexColumn] > 1024)
                return gameSuccess;
        }
    }
    /* if 2048 does not exist, check movable */
    for (index = 0; index < SIZE; index++) {
        /* copy the whole map first */
        tempMap = copyMap(map);
        /* then rotate it for 0 to 3 times and check if moveable */
        for (indexRotation = 0; indexRotation < index; indexRotation++) {
            rotate(tempMap);
        }
        moveSuccess -= moveLeft(tempMap, NULL);
        /* when done, free the temp map */
        recycleMap(tempMap);
    }
    return moveSuccess == 0;
}

int compareRecord(const void* v1, const void* v2) {
    const GameData* dataPointer1 = (GameData*)v1;
    const GameData* dataPointer2 = (GameData*)v2;
    return (dataPointer1 -> score < dataPointer2 -> score) - (dataPointer1 -> score > dataPointer2 -> score);
}

void saveRecord(GameData data) {
    FILE* file = fopen(SAVE_FILE, "a+");
    if (file != NULL) {
        fprintf(file, "%d|%d|%s\n", data.score, data.step, data.name);
        fclose(file);
    } else {
        printf("Error saving record.\n");
    }
}

GameDataSet readRecords() {
    int size = SIZE;
    int index = 0;
    GameData data;
    GameData* tempDataSet;
    GameDataSet dataSet = { (GameData*)calloc(size, sizeof(GameData)), index };
    FILE* file = fopen(SAVE_FILE, "r");
    if (file != NULL) {
        while (!feof(file)) {
            data.score = 0;
            if (index == size) {
                size *= 2;
                tempDataSet = (GameData*)realloc(dataSet.set, size * sizeof(GameData));
                if (tempDataSet != NULL) {
                    dataSet.set = tempDataSet;
                } else {
                    printf("Realloc memory failed.\n");
                    return dataSet;
                }
            }
            fscanf(file, "%d|%d|%s", &data.score, &data.step, data.name);
            if (data.score != 0) {
                dataSet.set[index] = data;
                index++;
            }
        }
        fclose(file);
        dataSet.size = index;
    } else {
        dataSet.set = NULL;
        dataSet.size = 0;
    }
    return dataSet;
}

int checkTopRanked(GameData data, GameDataSet dataSet) {
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

GameData start(int** map) {
    GameData data = {0, 0, ""};
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
                    case VK_RETURN:
                        if (!inputDone)
                            inputDone = 1;
                        break;
                    case VK_BACK:
                        if (detectInput && inputIndex > 0)
                            inputIndex--;
                        break;
                    default:
                        break;
                }
            }
            break;
        case WM_CHAR:
            if (rotateTimes == ZERO) {
                if ((char)wparam == 'r' && !detectInput) {
                    command = RESTART;
                    break;
                }
                if (!gameEnd) {
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
                        default:
                            command = NONE;
                            rotateTimes = ZERO;
                            break;
                    }
                } else {
                    if (detectInput) {
                        if ((char) wparam >= 32 && (char) wparam <= 126 && (char) wparam != '|')
                            lastInput = (char) wparam;
                    } else {
                        switch ((char) wparam) {
                            case 'y':
                                detectInput = 1;
                                inputDone = 0;
                                break;
                            case 'n':
                                command = RESTART;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            break;
        default:
            break;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    /* initialize basic variables */
    static int** gameMap;
    GameData gameData;
    Move* moveHistory;
    int running = 1;
    int lastMove = -1;   /* used to indicate last move direction */
    GameDataSet dataSet, displayDataSet;
    int saveDest = 0;
    int indexBuffer;
    StringBuffer scoreBuffer[10], stepBuffer[10];
    int nameLengthBuffer[MAX_RANK_COUNT];
    int bufferInitialized = 0;

    /* initialize nuklear: define variables */
    struct nk_context* context;
    GdiFont* font;
    WNDCLASSW wc;
    RECT rect = { 0, 0, windowWidth, windowHeight };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    HDC dc;
    int needs_refresh = 1;
    /* initialize nuklear: Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"2048";
    RegisterClassW(&wc);
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"2048",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);
    /* initialize nuklear: GUI */
    font = nk_gdifont_create("Microsoft YaHei UI Bold", 30);
    context = nk_gdi_init(font, dc, windowWidth, windowHeight);
    gameMap = allocateMap();
    /* initialize random number generator and start game */
    srand(time(NULL));
    /* initialize array and allocate memory */
    gameData = start(gameMap);
    gameEnd = 0;
    moveHistory = saveMap(gameMap, 0, 0);
    /* read records from file */
    dataSet = readRecords();
    displayDataSet.set = calloc(dataSet.size + 1, sizeof(GameData));
    displayDataSet.size = dataSet.size + 1;
    memcpy(displayDataSet.set, dataSet.set, dataSet.size * sizeof(GameData));
    displayDataSet.set[dataSet.size].score = -1;
    free(dataSet.set);
    while (running) {
        int success = 0;
        int currentRotate;
        int indexRow, indexColumn, indexData;
        char* title;
        int colorIndex;
        MSG msg;
        Sleep(15);
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
                if (rotateTimes != ZERO && !gameEnd) {
                    for (currentRotate = 0; currentRotate < SIZE; currentRotate++) {
                        if (currentRotate == rotateTimes)
                            success = moveLeft(gameMap, &gameData);
                        rotate(gameMap);
                    }
                }
                break;
            case UNDO:
                if (gameData.step > 0 && !gameEnd) {
                    /* first reduce the step */
                    gameData.step--;
                    /* if gameMap does not match the undo-ed map(if so the user has undo-ed twice), recycle it */
                    if (gameMap != moveHistory[gameData.step + 1].map)
                        recycleMap(gameMap);
                    /* set gameMap to previous map */
                    gameMap = moveHistory[gameData.step].map;
                    gameData.score = moveHistory[gameData.step].score;
                    /* recycle the undo-ed map */
                    recycleMap(moveHistory[gameData.step + 1].map);
                }
                break;
            case RESTART:
                /* free all memory and restart game */
                free(displayDataSet.set);
                gameEnd = 0;
                lastMove = -1;
                recycleAllMaps(moveHistory, gameMap, gameData.step, 0);
                /* read records again */
                dataSet = readRecords();
                displayDataSet.set = calloc(dataSet.size + 1, sizeof(GameData));
                displayDataSet.size = dataSet.size + 1;
                memcpy(displayDataSet.set, dataSet.set, dataSet.size * sizeof(GameData));
                displayDataSet.set[dataSet.size].score = -1;
                free(dataSet.set);
                /* revert all data */
                detectInput = 0;
                inputDone = 0;
                inputIndex = 0;
                saveDest = 0;
                bufferInitialized = 0;
                lastInput = '\0';
                gameMap = allocateMap();
                clearMap(gameMap);
                gameData = start(gameMap);
                moveHistory = saveMap(gameMap, 0, 0);
                break;
            default:
                break;
        }
        command = NONE;
        title = (char*)malloc(100 * sizeof(char));
        if (gameEnd) {
            strcpy(title, "Game ends!");
        } else {
            if (lastMove != -1)
                sprintf(title, "Score: %d, Step: %d, Moved %s", gameData.score, gameData.step, rotateNames[lastMove]);
            else
                sprintf(title, "Score: %d, Step: %d", gameData.score, gameData.step);
        }
        if (nk_begin(context, title, nk_rect(0, 0, windowWidth, windowHeight), NK_WINDOW_TITLE)) {
            if (!detectInput) {
                for (indexRow = 0; indexRow < SIZE; indexRow++) {
                    nk_layout_row_dynamic(context, windowHeight / 6, 4);
                    for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
                        if (gameMap[indexRow][indexColumn] != 0) {
                            int length = _snprintf(NULL, 0, "%d", gameMap[indexRow][indexColumn]);
                            char* str = (char*)malloc(length + 1);
                            _snprintf(str, length + 1, "%d", gameMap[indexRow][indexColumn]);
                            colorIndex = log((double)(gameMap[indexRow][indexColumn])) / LOG2;
                            nk_text_colored_background(context, str, length, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, nk_rgb_hex(hexColorSheet[colorIndex]), nk_rgb_hex("#FFFFFF"));
                            free(str);
                        } else {
                            nk_text_colored_background(context, "[ ]", 3, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, context->style.window.background, nk_rgb_hex("#FFFFFF"));
                        }
                    }
                }
                nk_layout_row_dynamic(context, 30, 1);
                if (!gameEnd)
                    nk_text_colored_background(context, "Press u to undo, r to restart", 29, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, context->style.window.background, nk_rgb_hex("#DDDDDD"));
                else if (!checkTopRanked(gameData, displayDataSet))
                    nk_text_colored_background(context, "Press r to restart", 18, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, context->style.window.background, nk_rgb_hex("#DDDDDD"));
                else
                    nk_text_colored_background(context, "Save records? Y/N", 17, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, context->style.window.background, nk_rgb_hex("#DDDDDD"));
            } else {
                if (displayDataSet.set[displayDataSet.size - 1].score == -1) {
                    displayDataSet.set[displayDataSet.size - 1] = gameData;
                    qsort(displayDataSet.set, displayDataSet.size, sizeof(GameData), compareRecord);
                    for (saveDest = 0; saveDest < displayDataSet.size; saveDest++) {
                        if (strcmp("", displayDataSet.set[saveDest].name) == 0)
                            break;
                    }
                }
                if (!bufferInitialized) {
                    int size = displayDataSet.size > MAX_RANK_COUNT ? MAX_RANK_COUNT : displayDataSet.size;
                    for (indexBuffer = 0; indexBuffer < size; indexBuffer++) {
                        itoa(displayDataSet.set[indexBuffer].score, scoreBuffer[indexBuffer].string, 10);
                        scoreBuffer[indexBuffer].length = _snprintf(NULL, 0, "%d", displayDataSet.set[indexBuffer].score);
                        itoa(displayDataSet.set[indexBuffer].step, stepBuffer[indexBuffer].string, 10);
                        stepBuffer[indexBuffer].length = _snprintf(NULL, 0, "%d", displayDataSet.set[indexBuffer].step);
                        if (indexBuffer != saveDest)
                            nameLengthBuffer[indexBuffer] = _snprintf(NULL, 0, "%s", displayDataSet.set[indexBuffer].name);
                    }
                    bufferInitialized = 1;
                }
                if (!inputDone) {
                    displayDataSet.set[saveDest].name[inputIndex] = '_';
                    nk_layout_row_dynamic(context, windowHeight / (MAX_RANK_COUNT + 1), 3);
                    nk_text_colored_background(context, "Score", 5, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                    nk_text_colored_background(context, "Steps", 5, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                    nk_text_colored_background(context, "Name", 4, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                    for (indexData = 0; indexData < (displayDataSet.size > MAX_RANK_COUNT ? MAX_RANK_COUNT : displayDataSet.size); indexData++) {
                        nk_layout_row_dynamic(context, windowHeight / (MAX_RANK_COUNT + 1), 3);
                        nk_text_colored_background(context, scoreBuffer[indexData].string, scoreBuffer[indexData].length, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                        nk_text_colored_background(context, stepBuffer[indexData].string, stepBuffer[indexData].length, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                        if (indexData != saveDest)
                            nk_text_colored_background(context, displayDataSet.set[indexData].name, nameLengthBuffer[indexData], NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                        else
                            nk_text_colored_background(context, displayDataSet.set[indexData].name, inputIndex, NK_TEXT_ALIGN_LEFT , context->style.window.background, nk_rgb_hex("#DDDDDD"));
                    }
                    if (lastInput != '\0') {
                        displayDataSet.set[saveDest].name[inputIndex] = lastInput;
                        inputIndex++;
                        lastInput = '\0';
                    }
                    if (inputIndex == 9) {
                        inputDone = 1;
                    }
                }
                if (inputDone) {
                    /* end string and save data */
                    displayDataSet.set[saveDest].name[inputIndex] = '\0';
                    saveRecord(displayDataSet.set[saveDest]);
                    command = RESTART;
                }
            }
            nk_end(context);
        }
        if (title != NULL) {
            free(title);
            title = NULL;
        }
        /* DRAW!!! */
        nk_gdi_render(nk_rgb(30,30,30));
        if (rotateTimes != ZERO && success) {
            lastMove = rotateTimes;
            gameData.step++;
            generateRandomNumber(gameMap, 1);
            if (checkEnd(gameMap))
                gameEnd = 1;
            else
                moveHistory = saveMap(gameMap, gameData.score, gameData.step);
        }
        rotateTimes = ZERO;
    }
    recycleAllMaps(moveHistory, gameMap, gameData.step, 1);
    free(displayDataSet.set);
    /* release nuklear */
    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, hInstance);
    return EXIT_SUCCESS;
}