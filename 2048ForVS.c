// below: to be included in "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>
// below: main c file
#include "stdafx.h"
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

short colorsheet[12] = { 15, 31, 47, 63, 79, 95, 111, 125, 124, 123, 122, 121 };

int** allocateMap() {
    int** array;
    int i;
    array = calloc(SIZE, sizeof(array));
    for (i = 0; i < SIZE; i++)
        array[i] = calloc(SIZE, sizeof(array[0]));
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
        moveList = calloc(size, sizeof(struct Move));
    } else {
        if (size == destIndex) {
            size *= 2;
            tempMoveList = realloc(moveList, size * sizeof(struct Move));
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

void printAll(int** map, struct GameData* data) {
    int indexRow, indexColumn, colorIndex;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    system("cls");
    for (indexRow = 0; indexRow < SIZE; indexRow++) {
        for (indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (map[indexRow][indexColumn] != 0) {
                colorIndex = log(map[indexRow][indexColumn]) / LOG2;
                SetConsoleTextAttribute(console, colorsheet[colorIndex]);
                printf("[%d]", map[indexRow][indexColumn]);
                SetConsoleTextAttribute(console, colorsheet[0]);
                printf("\t");
            } else
                printf("[ ]\t");
        }
        printf("\n");
    }
    printf("current score: %d, steps: %d\n", (*data).score, (*data).step);
    printf("press q to quit, u to undo, r to restart\n");
    printf("\n\n");
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
    struct GameDataSet dataSet = { calloc(size, sizeof(struct GameData)), index };
    FILE* file = fopen(SAVE_FILE, "r");
    if (file != NULL) {
        while (!feof(file)) {
            if (index == size) {
                size *= 2;
                tempDataSet = realloc(dataSet.set, size * sizeof(struct GameData));
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
    printAll(map, &data);
    return data;
}

int main() {
    static int** gameMap;
    struct GameData gameData;
    struct Move* moveHistory;
    int gameEnd = 0;
    struct GameDataSet dataSet, displayDataSet;
    gameMap = allocateMap();
    // initialize random number generator and start game
    srand(time(NULL));
    // initialize array and allocate memory
    gameData = start(gameMap);
    moveHistory = saveMap(gameMap, 0, 0);
    while (!gameEnd) {
        int success = 0;
        int moved = 0;
        int currentInMove = 0;
        switch (getch()) {
            case 97:    // 'a'
                if (!currentInMove) {
                    currentInMove = 1;
                    success = moveLeft(gameMap, &gameData);
                    moved = 1;
                }
                break;

            case 100:    // 'd'
                if (!currentInMove) {
                    currentInMove = 1;
                    rotate(gameMap);
                    rotate(gameMap);
                    success = moveLeft(gameMap, &gameData);
                    rotate(gameMap);
                    rotate(gameMap);
                    moved = 1;
                }

                break;

            case 119:    // 'w'
                if (!currentInMove) {
                    currentInMove = 1;
                    rotate(gameMap);
                    success = moveLeft(gameMap, &gameData);
                    rotate(gameMap);
                    rotate(gameMap);
                    rotate(gameMap);
                    moved = 1;
                }
                break;

            case 115:    // 's'
                if (!currentInMove) {
                    currentInMove = 1;
                    rotate(gameMap);
                    rotate(gameMap);
                    rotate(gameMap);
                    success = moveLeft(gameMap, &gameData);
                    rotate(gameMap);
                    moved = 1;
                }
                break;

            case 224:
            case 0:
                switch(getch()) {
                    case KEY_UP:    // up arrow
                        if (!currentInMove) {
                            currentInMove = 1;
                            rotate(gameMap);
                            success = moveLeft(gameMap, &gameData);
                            rotate(gameMap);
                            rotate(gameMap);
                            rotate(gameMap);
                            moved = 1;
                        }
                        break;

                    case KEY_DOWN:    // down arrow
                        if (!currentInMove) {
                            currentInMove = 1;
                            rotate(gameMap);
                            rotate(gameMap);
                            rotate(gameMap);
                            success = moveLeft(gameMap, &gameData);
                            rotate(gameMap);
                            moved = 1;
                        }
                        break;

                    case KEY_RIGHT:    // right arrow
                        if (!currentInMove) {
                            currentInMove = 1;
                            rotate(gameMap);
                            rotate(gameMap);
                            success = moveLeft(gameMap, &gameData);
                            rotate(gameMap);
                            rotate(gameMap);
                            moved = 1;
                        }
                        break;

                    case KEY_LEFT:    // left arrow
                        if (!currentInMove) {
                            currentInMove = 1;
                            success = moveLeft(gameMap, &gameData);
                            moved = 1;
                        }
                        break;
                }

            case 'q':
                if (!currentInMove) {
                    gameEnd = 1;
                }
                break;

            case 'r':
                if (!currentInMove) {
                    gameEnd = 0;
                    recycleAllMaps(moveHistory, gameMap, gameData.step, 0);
                    gameMap = allocateMap();
                    clearMap(gameMap);
                    gameData = start(gameMap);
                    moveHistory = saveMap(gameMap, 0, 0);
                }
                break;

            case 'u':
                if (gameData.step > 0 && !currentInMove) {
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
                    printAll(gameMap, &gameData);
                }
                break;
            
            default:
                break;
        }
        if (moved && success) {
            gameData.step++;
            generateRandomNumber(gameMap, 1);
            printAll(gameMap, &gameData);
            if (checkEnd(gameMap)) {
                printf("game end!\n\n");
                gameEnd = 1;
            } else
                moveHistory = saveMap(gameMap, gameData.score, gameData.step);
            currentInMove = 0;
        }
    }
    dataSet = readRecords();
    if (checkWhetherTopRanked(gameData, dataSet)) {
        // first make current name to be filled
        strcpy(gameData.name, "______");
        // create a new dataSet, copy current records, insert current play data, display it and recycle it
        displayDataSet.set = calloc(dataSet.size + 1, sizeof(struct GameData));
        displayDataSet.size = dataSet.size + 1;
        memcpy(displayDataSet.set, dataSet.set, dataSet.size * sizeof(struct GameData));
        displayDataSet.set[dataSet.size] = gameData;
        displayRecords(displayDataSet);
        free(displayDataSet.set);
        free(dataSet.set);
        // ask for name and save data to the file
        printf("\nPlease type your name (max 9 chars):\n");
        strcpy(gameData.name, "");
        scanf("%9s", gameData.name);
        if (gameData.name[0] != '\0') {
            saveRecord(gameData);
            displayDataSet = readRecords();
            system("cls");
            displayRecords(displayDataSet);
            free(displayDataSet.set);
        }
    }
    recycleAllMaps(moveHistory, gameMap, gameData.step, 1);
    printf("\n\nPress any key to exit.\n");
    getch();
    return EXIT_SUCCESS;
}