// https://antoniosliapis.com/articles/pcgbook_dungeons.php
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define MIN_SIZE_ROOM 7
#define CELL_WIDTH    20
#define CELL_HEIGHT   20

static int id_counter = 0;
static int frames = 0;

typedef struct CellPos {
    int x;
    int y;
} CellPos;

typedef enum CellKind {
    EXIT,
    WALL,
    GROUND,
} CellKind;

typedef struct Cell {
    CellPos  pos;
    CellKind kind;
    Color    color;
    int      id;
} Cell;

typedef struct Path {
    CellPos *items;
    size_t  count;
    size_t  capacity;
} Path;

#define path_append(arr, item) \
    do { \
        if (arr.count >= arr.capacity) { \
            if (arr.capacity == 0) arr.capacity = 256; \
            else arr.capacity *= 2; \
            arr.items = realloc(arr.items,, arr.capacity*sizeof(*arr.items)); \
        } \
        arr.items[arr.count++] = item; \
    } while(0) \

typedef struct Rect {
    int    x, y; // posi
    int    w, h; // size
    Cell **grid;
    Path   path;
} Rect;

static bool debug = false;
typedef struct BSPNode {
    struct BSPNode* front;
    struct BSPNode* back;
    Rect area;
    Rect room; // the leafs
} BSPNode;

typedef struct Map {
    Rect area;
    struct BSPNode* root;
} Map;
static bool pause = false;

void drawNodes(BSPNode* node, int current_depth, int max_depth) {
    if (node == NULL || current_depth > max_depth) return;

    DrawRectangleLines(node->area.x, node->area.y, node->area.w, node->area.h, BLACK);
    DrawRectangle(node->room.x, node->room.y, node->room.w, node->room.h, BLACK);

    // debug
    const int COLS = (int)node->room.w / CELL_WIDTH;
    const int ROWS = (int)node->room.h / CELL_HEIGHT;

    char str1[20];
    sprintf(str1, "width: %d", node->room.w);
    
    char str2[20];
    sprintf(str2, "height: %d", node->room.h);

    if (debug) {
        // DrawText(str1, node->area.x + 5, node->area.y + 25, 20, RED);
        // DrawText(str2, node->area.x + 5, node->area.y + 45, 20, RED);
        for (int i = 0; i < COLS; i++) {
            for (int j = 0; j < ROWS; j++) {
                if (node->room.grid[i][j].kind != EXIT) 
                    DrawRectangleLines(
                        node->room.grid[i][j].pos.x,
                        node->room.grid[i][j].pos.y,
                        CELL_WIDTH,
                        CELL_HEIGHT,
                        node->room.grid[i][j].color
                    );
                else 
                    DrawRectangle(
                        node->room.grid[i][j].pos.x,
                        node->room.grid[i][j].pos.y,
                        CELL_WIDTH,
                        CELL_HEIGHT,
                        node->room.grid[i][j].color
                    );
            }
        }
    }

    // draw nodes
    drawNodes(node->back, current_depth+1, max_depth);
    drawNodes(node->front, current_depth+1, max_depth);
}

BSPNode* createNode(Rect area) {
    BSPNode* new_node = (BSPNode*)malloc(sizeof(BSPNode));
    new_node->back = NULL;
    new_node->front = NULL;
    new_node->area = area;
    return new_node;
}

enum Split {
    VERTICAL,
    HORIZONTAL
} Split;

void generateRoom(BSPNode* node) {
    // padding
    int pad = 15;

    // frist values
    int firstX = node->area.x;
    int firstY = node->area.y;
    int firstW = node->area.w;
    int firstH = node->area.h;


    float sub = 0.8;
    int subwid = node->area.w*sub;
    int subhei = node->area.h*sub;
    node->room.x = node->area.x + pad + (rand() % (node->area.w - pad - subwid));
    node->room.y = node->area.y + pad + (rand() % (node->area.h - pad - subhei));

    node->room.w = ((int)((firstX + firstW - pad) - node->room.x) / CELL_WIDTH) * CELL_WIDTH;
    node->room.h = ((int)((firstY + firstH - pad*2) - node->room.y) / CELL_HEIGHT) * CELL_HEIGHT;
    node->room.w = (node->room.w / CELL_WIDTH) * CELL_WIDTH; 
    node->room.h = (node->room.h / CELL_HEIGHT) * CELL_HEIGHT; 

    // generate grid
    const int COLS = node->room.w / CELL_WIDTH;
    const int ROWS = node->room.h / CELL_HEIGHT;

    node->room.grid = malloc(sizeof(Cell*) * COLS); // we save space for each column
    for (int i = 0; i < COLS; i++) {
        node->room.grid[i] = malloc(sizeof(Cell)*ROWS); // we save space for each row for each column
        for (int j = 0; j < ROWS; j++) {
            node->room.grid[i][j].pos = (CellPos){ 
                .x = i*CELL_WIDTH + node->room.x, 
                .y = j*CELL_HEIGHT + node->room.y
            };
            node->room.grid[i][j].color = GRAY;
            node->room.grid[i][j].kind = GROUND;
            node->room.grid[i][j].id   = id_counter;
        }
    }

    id_counter++;

    // generate exit cells
    // border size in each direction
    // int rantb = rand() % COLS;
    // int ranlr = rand() % ROWS;     

    // int total_exits = 0;
    // if (rand() % 2 == 0) {
    //     node->room.grid[rantb][0].kind = EXIT;
    //     node->room.grid[rantb][0].color = RED;
    // } else {
    //     node->room.grid[rantb][ROWS - 1].kind = EXIT;
    //     node->room.grid[rantb][ROWS - 1].color = RED;
    // }

    // if (rand() % 2 == 0) {
    //     node->room.grid[0][ranlr].kind = EXIT;
    //     node->room.grid[0][ranlr].color = BLUE;
    // } else {
    //     node->room.grid[COLS - 1][ranlr].kind = EXIT;
    //     node->room.grid[COLS - 1][ranlr].color = BLUE;
    // }
}

void connectRooms(BSPNode* node) {
    if (node == NULL || node->back == NULL || node->front == NULL) return;

    connectRooms(node->back);
    connectRooms(node->front);

    Path path = {0};

    Rect *roomA = &node->back->room;
    Rect *roomB = &node->front->room;

    int colsA = roomA->w / CELL_WIDTH;
    int rowsA = roomA->h / CELL_HEIGHT;
    int colsB = roomB->w / CELL_WIDTH;
    int rowsB = roomB->h / CELL_HEIGHT;

    if (colsA <= 0 || rowsA <= 0 || colsB <= 0 || rowsB <= 0) return;

    int startX = rand() % colsA;
    int startY = rand() % rowsA;
    
    int endX = rand() % colsB;
    int endY = rand() % rowsB;

    roomA->grid[startX][startY].color = GREEN;
    roomA->grid[startX][startY].kind = EXIT;
    roomB->grid[endX][endY].color = RED;
    roomB->grid[endX][endY].kind = EXIT;

    while (startX != endX && startY != endY) {
        startX++;
        startY++;
        roomA->grid[startX][startY].color = GREEN;
        roomA->grid[startX][startY].kind = EXIT;
    }

}


void generateNodes(BSPNode* node, int depth) {
    if (depth == 0) { // leafs
        generateRoom(node);
        return;
    }

    int pad = 0;
    int x = rand() % node->area.w;
    int y = rand() % node->area.h;
    int v_or_h = rand() % 2; // 0 → vertical, 1 → horizontal
    Rect front_area;
    Rect back_area;
    enum Split split; 

    if (v_or_h == 1) split = VERTICAL;
    else split = HORIZONTAL;
    // back[0] and front[1]
    printf("orientation: %d \n", v_or_h);

    int cutHorizontal, cutVertical = 0;  
    int cut;
    if (split == HORIZONTAL) {
        cut = node->area.h / 2;
        back_area = (Rect){
            .x = node->area.x,
            .y = node->area.y,
            .w = node->area.w - pad,
            .h = cut - pad
        };
        front_area = (Rect){
            .x = node->area.x,
            .y = node->area.y + cut,
            .w = node->area.w - pad,
            .h = cut - pad
        };
    } else {
        cut = node->area.w/2;
        back_area = (Rect){
            .x = node->area.x,
            .y = node->area.y,
            .w = cut - pad,
            .h = node->area.h - pad
        };
        front_area = (Rect){
            .x = node->area.x + cut,
            .y = node->area.y,
            .w = cut - pad,
            .h = node->area.h - pad
        };
    }

    node->back = createNode(back_area);
    node->front = createNode(front_area);
    
    generateNodes(node->back, depth-1);
    generateNodes(node->front, depth-1);
}

int main(void) {
    int taim = time(NULL);
    srand(taim);
    printf("%d \n", taim);
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
    int greates_depth = 2;
    
    Rect root_area = {
        .h = SCREEN_HEIGHT,
        .w = SCREEN_WIDTH,
        .x = 0, // px
        .y = 0, // px
    };

    BSPNode* root = createNode(root_area);
    generateNodes(root, greates_depth);
    connectRooms(root);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "bsp");
    int animation_speed = 32;
    SetTargetFPS(60);
    int max_depth = 0;
    while (!WindowShouldClose()) {
        BeginDrawing();
            frames++;

            if (IsKeyPressed(KEY_D)) debug = !debug;

            if (IsKeyPressed(KEY_P)) pause = !pause;
            if (pause) frames--;

            if (frames % animation_speed == 0 && max_depth < greates_depth+1) {
                max_depth++;
            }
            ClearBackground(RAYWHITE);
            drawNodes(root, 0, max_depth);

            if (max_depth < greates_depth+1) {
                DrawText("Loading...", 20, 20, 20, MAROON);
            } else {
                DrawText("¡GAAA! Generation completed", 20, 20, 20, DARKGREEN);
            }

            if (pause) DrawText("Pause", 50, 50, 20, RED);

        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}