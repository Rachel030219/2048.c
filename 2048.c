#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <math.h>
#define SIZE 4
#define LOG2 0.693147
#define SAVE_FILE "records.txt"
#define MAX_RANK_COUNT 10

struct GameData {
    int score;
    int step;
    char name[10];
};

struct GameDataSet {
    struct GameData* set;
    int size;
};

struct Move {
    int** map;
    int score;
};

short backgroundColorsheet[12] = {0, 124, 198, 90, 21, 30, 64, 56, 28, 208, 166, 2};

int** allocateMap() {
    int** array;
    array = calloc(SIZE, sizeof(array));
    for (int i = 0; i < SIZE; i++)
        array[i] = calloc(SIZE, sizeof(array[0]));
    return array;
}

void clearMap(int** array) {
    for (int x = 0; x < SIZE; x++)
        for (int y = 0; y < SIZE; y++)
            array[x][y] = 0;
}

void recycleMap(int** array) {
    for (int i = 0; i < SIZE; i++) {
        if (array[i] != NULL) {
            free(array[i]);
            array[i] = NULL;
        }
    }
    free(array);
}

int** copyMap(int** sourceMap) {
    int** destMap = allocateMap();
    clearMap(destMap);
    for (int indexRow = 0; indexRow < SIZE; indexRow++) {
        memcpy(destMap[indexRow], sourceMap[indexRow], SIZE * sizeof(int));
    }
    return destMap;
}

struct Move* saveMap(int** sourceMap, int score, int destIndex) {
    static struct Move* moveList = NULL;
    static int size = SIZE;
    if (moveList == NULL) {
        moveList = calloc(size, sizeof(struct Move));
    } else {
        if (size == destIndex) {
            size *= 2;
            struct Move* tempMoveList = realloc(moveList, size * sizeof(struct Move));
            if (tempMoveList != NULL) {
                moveList = tempMoveList;
                // fill all uninitialized memory with NULL and zero
                for (int i = destIndex + 1; i < size; i++) {
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
    for (int i = 0; i <= count; i++) {
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
    for (int i = 0; i < count; i++) {
        int randX = rand() % (SIZE);
        int randY = rand() % (SIZE);
        int two = rand() % 10;
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
    for (int indexRow = 0; indexRow < SIZE; indexRow++) {
        for (int indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (map[indexRow][indexColumn] != 0) {
                int colorIndex = log(map[indexRow][indexColumn]) / LOG2;
                printf("\033[48;5;%dm[%d]\033[0m\t", backgroundColorsheet[colorIndex], map[indexRow][indexColumn]);
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
int moveLeft (int** map, struct GameData* gameDataPointer) {
    int moveCount = 0;
    for (int indexRow = 0; indexRow < SIZE; indexRow++) {
        int* row = map[indexRow];
        int unmovableCount = 0;
        for (int indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (indexColumn != 0 && row[indexColumn] != 0) {
                // targetIndex: the destination current number should be moved to
                int targetIndex = indexColumn;
                // traverse from current number to the left
                for (int indexTarget = indexColumn - 1; indexTarget >= unmovableCount; indexTarget--) {
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
    int temp;
    for (int i = 0; i < SIZE / 2; i++) {
        for (int j = i; j < SIZE - i -1; j++) {
            temp = map[i][j];
            map[i][j] = map[j][SIZE - i - 1];
            map[j][SIZE - i - 1] = map[SIZE - i - 1][SIZE - j - 1];
            map[SIZE - i - 1][SIZE - j - 1] = map[SIZE - j - 1][i];
            map[SIZE - j - 1][i] = temp;
        }
    }
}

int checkEnd(int** map) {
    // check if 2048 exists, ends immediately when true
    int gameSuccess = 1;
    for (int indexRow = 0; indexRow < SIZE; indexRow++) {
        for (int indexColumn = 0; indexColumn < SIZE; indexColumn++) {
            if (map[indexRow][indexColumn] == 2048)
                return gameSuccess;
        }
    }
    // if 2048 does not exist, check movable
    int moveSuccess = 0;
    for (int index = 0; index < SIZE; index++) {
        // copy the whole map first
        int** tempMap = copyMap(map);
        // then rotate it for 0 to 3 times and check if moveable
        for (int indexRotation = 0; indexRotation < index; indexRotation++) {
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
    struct GameDataSet dataSet = { calloc(size, sizeof(struct GameData)), index };
    FILE* file = fopen(SAVE_FILE, "r");
    if (file != NULL) {
        while (!feof(file)) {
            if (index == size) {
                size *= 2;
                struct GameData* tempDataSet = realloc(dataSet.set, size * sizeof(struct GameData));
                if (tempDataSet != NULL) {
                    dataSet.set = tempDataSet;
                } else {
                    printf("Realloc memory failed.\n");
                    return dataSet;
                }
            }
            struct GameData data;
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
    if (dataSet.set != NULL) {
        printf("Score\tSteps\tName\n");
        qsort(dataSet.set, dataSet.size, sizeof(struct GameData), compareRecord);
        int count = dataSet.size < MAX_RANK_COUNT ? dataSet.size : MAX_RANK_COUNT;
        for (int i = 0; i < count; i++) {
            printf("%d\t%d\t%s\n", dataSet.set[i].score, dataSet.set[i].step, dataSet.set[i].name);
        }
    }
}

int checkWhetherTopRanked(struct GameData data, struct GameDataSet dataSet) {
    int recordAboveCurrentCount = 0;
    for (int i = 0; i < dataSet.size; i++) {
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

// block buffer to get direct keymap input
void setBufferedInput(int enable) {
    static int enabled = 1;
    static struct termios old;
    struct termios new;

    if (enable && !enabled) {
        // restore the former settings and set the new state
        tcsetattr(STDIN_FILENO,TCSANOW,&old);
        enabled = 1;
    } else if (!enable && enabled) {
        // get the terminal settings for standard input and disable canonical mode (buffered i/o)
        tcgetattr(STDIN_FILENO,&new);
        old = new;
        new.c_lflag &=(~ICANON & ~ECHO);
        // set the new settings
        tcsetattr(STDIN_FILENO,TCSANOW,&new);
        enabled = 0;
    }
}

int main() {
    // initialize array and allocate memory
    static int** gameMap;
    gameMap = allocateMap();
    // initialize random number generator and start game
    srand(time(NULL));
    struct GameData gameData = start(gameMap);
    struct Move* moveHistory = saveMap(gameMap, 0, 0);
    // block and read from input
    setBufferedInput(0);
    int gameEnd = 0;
    while (!gameEnd) {
        int success = 0;
        int moved = 0;
        int currentInMove = 0;
        switch (getchar()) {
            case 97:   // 'a'
            case 68:   // left arrow
                if (!currentInMove) {
                    currentInMove = 1;
                    success = moveLeft(gameMap, &gameData);
                    moved = 1;
                }
                break;

            case 100:  // 'd'
            case 67:   // right arrow
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

            case 119:  // 'w'
            case 65:   // up arrow
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

            case 115:  // 's'
            case 66:   // down arrow
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
            } else {
                moveHistory = saveMap(gameMap, gameData.score, gameData.step);
            }
            currentInMove = 0;
        }
    }
    setBufferedInput(1);
    struct GameDataSet dataSet = readRecords();
    if (checkWhetherTopRanked(gameData, dataSet)) {
        // first make current name to be filled
        strcpy(gameData.name, "______");
        // create a new dataSet, copy current records, insert current play data, display it and recycle it
        struct GameDataSet displayDataSet = { calloc(dataSet.size + 1, sizeof(struct GameData)), dataSet.size + 1 };
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
            displayRecords(displayDataSet);
            free(displayDataSet.set);
        }
    }
    recycleAllMaps(moveHistory, gameMap, gameData.step, 1);
    return EXIT_SUCCESS;
}
