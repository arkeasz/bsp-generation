// https://antoniosliapis.com/articles/pcgbook_dungeons.php
// https://www.gridsagegames.com/blog/2014/07/furnishing-athe-dungeon/
// https://en.wikipedia.org/wiki/Hamiltonian_path_problem
// https://www.roguebasin.com/index.php?title=Winding_ways
// https://tuliomarks.medium.com/how-i-created-roguelike-map-with-procedural-generation-630043f9a93f
// https://medium.com/i-math/the-drunkards-walk-explained-48a0205d304
// https://www.youtube.com/watch?v=iH2kATv49rc&t=608s
// https://www.youtube.com/watch?v=DKGodqDs9sA
// https://www.youtube.com/watch?v=JEBE9Add0Ms
// https://www.youtube.com/watch?v=TlLIOgWYVpI&t=316s
// https://www.youtube.com/watch?v=ogOKDhOa_cs

#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define MIN_SIZE_ROOM 7
#define CELL_WIDTH    20
#define CELL_HEIGHT   20
#define NONE          2
static int id_counter = 0;
static int frames = 0;
static int connect = 0;
typedef struct CellPos {
    int x;
    int y;
} CellPos;

typedef enum CellKind {
    EMPTY,
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
        if ((arr).count >= (arr).capacity) { \
            if ((arr).capacity == 0) (arr).capacity = 256; \
            else (arr).capacity *= 2; \
            (arr).items = realloc((arr).items, (arr).capacity*sizeof(*(arr).items)); \
        } \
        (arr).items[(arr).count++] = (item); \
    } while(0); \


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

    DrawRectangleLines(
        node->area.x*CELL_WIDTH, 
        node->area.y*CELL_HEIGHT, 
        node->area.w*CELL_WIDTH, 
        node->area.h*CELL_HEIGHT, 
        BLUE
    );
    DrawRectangle(
        node->room.x*CELL_WIDTH, 
        node->room.y*CELL_HEIGHT,
        node->room.w*CELL_WIDTH,
        node->room.h*CELL_HEIGHT, 
        BLACK
    );

    if (debug) {
        for (int i = 0; i < node->room.w; i++) {
            for (int j = 0; j < node->room.h; j++) {
                if (node->room.grid[i][j].kind != EXIT) 
                    DrawRectangleLines(
                        node->room.grid[i][j].pos.x*CELL_WIDTH,
                        node->room.grid[i][j].pos.y*CELL_HEIGHT,
                        CELL_WIDTH,
                        CELL_HEIGHT,
                        node->room.grid[i][j].color
                    );
                else 
                    DrawRectangle(
                        node->room.grid[i][j].pos.x*CELL_WIDTH,
                        node->room.grid[i][j].pos.y*CELL_HEIGHT,
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
    // Draw Paths 
    for (int i = 0; i < node->area.path.count; i++) {
        DrawRectangle(
            node->area.path.items[i].x * CELL_WIDTH,
            node->area.path.items[i].y * CELL_HEIGHT,
            CELL_WIDTH, 
            CELL_HEIGHT,
            MAROON
        );
    }
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
    int padX = 1; // substrating 3 columns
    int padY = 1; // substrating 3 rows

    // frist values
    int firstX = node->area.x; // start column
    int firstY = node->area.y; // start row
    int firstW = node->area.w; // number of columns contained in the area
    int firstH = node->area.h; // number of rows contained in the area


    float sub = 0.8;
    /**
     * ex: node->area.w*sub, where area.w = 11
     * 11*0.8 = 8.8
     * (int)(8.8) = 8
     */
    int subwid = (int)(node->area.w*sub); 
    int subhei = (int)(node->area.h*sub);


    node->room.x = node->area.x + padX + (rand() % (node->area.w - subwid));
    node->room.y = node->area.y + padY + (rand() % (node->area.h - subhei));

    node->room.w = (int)((firstX + firstW - padX) - node->room.x);
    node->room.h = (int)((firstY + firstH - padY) - node->room.y);

    // generate grid
    const int COLS = node->room.w;
    const int ROWS = node->room.h;

    node->room.grid = malloc(sizeof(Cell*) * COLS); // we save space for each column
    for (int i = 0; i < COLS; i++) {
        node->room.grid[i] = malloc(sizeof(Cell)*ROWS); // we save space for each row for each column
        for (int j = 0; j < ROWS; j++) {
            node->room.grid[i][j].pos = (CellPos){ 
                .x = i + node->room.x, 
                .y = j + node->room.y
            };
            node->room.grid[i][j].color = GRAY;
            node->room.grid[i][j].kind = GROUND;
            node->room.grid[i][j].id   = id_counter;
        }
    }

    id_counter++;
}

Rect getNearestRoom(BSPNode* node, int target_x, int target_y) {
    return node->front->room;
}

void connectRooms(BSPNode* node) { // by postorder
    if (node == NULL) return;
    if (node->back == NULL || node->front == NULL) return;

    connectRooms(node->back);
    connectRooms(node->front);

    // cols and rows for back and front nodes
    const int COLS_ONE = node->back->room.w;    
    const int ROWS_ONE = node->back->room.h;   

    int targetX = node->back->area.x + (node->front->area.x / 2);
    int targetY = node->back->area.y + (node->front->area.y / 2);
    const Rect front = getNearestRoom(node->back, targetX, targetY);

    const int COLS_TWO = node->front->room.w;    
    const int ROWS_TWO = node->front->room.h;   

    if (COLS_ONE <= 0 || ROWS_ONE <= 0 || COLS_TWO <= 0 || ROWS_TWO <= 0) {
        fprintf(stderr, "Error: Room dimensions invalid\n");
        return;
    }

    // generate exit cells
    // border size in each direction
    int rantbo = rand() % (COLS_ONE); // one
    int ranlro = rand() % (ROWS_ONE); // one

    int rantbt = rand() % (COLS_TWO); // two
    int ranlrt = rand() % (ROWS_TWO); // two

    node->back->room.grid[rantbo][ranlro].kind = EXIT;
    node->back->room.grid[rantbo][ranlro].color = RED;

    node->front->room.grid[rantbt][ranlrt].kind = EXIT;
    node->front->room.grid[rantbt][ranlrt].color = BLUE;

    Path pa = {0};

    // absolute values
    int x1 = node->back->room.x + rantbo;
    int y1 = node->back->room.y + ranlro;

    int x2 = node->front->room.x + rantbt;
    int y2 = node->front->room.y + ranlrt;

    while (x1 != x2 || y1 != y2)    {
        if (x1 != x2) {
            if (x1 > x2) x1--;
            else x1++;
        } else if (y1 != y2) {
            if (y1 > y2) y1--;
            else y1++;
        }

        CellPos pos = {
            .x = x1,
            .y = y1
        };

        path_append(pa, pos);
    }
    
    node->area.path = pa;
}

void generateNodes(BSPNode* node, int depth, enum Split last_split) {
    if (depth == 0) { // leafs

        int COLSA = node->area.w;
        int ROWSA = node->area.h;

        Cell **grid;
        grid = malloc(sizeof(Cell*)*COLSA);
        for (int i = 0; i < COLSA; i++) {
            grid[i] = malloc(sizeof(Cell) * ROWSA);
            for (int j = 0; j < ROWSA; j++)  {
                grid[i][j] = (Cell) { .pos.x = i, .pos.y = j };
                grid[i][j].kind  = EMPTY;
                grid[i][j].color = WHITE;
                grid[i][j].id = id_counter;
            }
        }
        generateRoom(node);
        return;
    }

    Rect front_area;
    Rect back_area;
    enum Split split; 

    if (last_split == HORIZONTAL) {
        split = (rand() % 100 < 70) ? VERTICAL : HORIZONTAL;
    } else if (last_split == VERTICAL) {
        split = (rand() % 100 < 70) ? HORIZONTAL : VERTICAL;
    } else {
        split = (rand() % 2 == 0) ? HORIZONTAL : VERTICAL;
    }

    int cut;
    if (split == HORIZONTAL) {
        cut = node->area.h / 2;
        back_area = (Rect){
            .x = node->area.x,
            .y = node->area.y,
            .w = node->area.w,
            .h = cut
        };
        front_area = (Rect){
            .x = node->area.x ,
            .y = node->area.y + cut,
            .w = node->area.w,
            .h = node->area.h - cut
        };
    } else {
        cut = node->area.w / 2;
        back_area = (Rect){
            .x = node->area.x,
            .y = node->area.y,
            .w = cut,
            .h = node->area.h
        };
        front_area = (Rect){
            .x = node->area.x + cut,
            .y = node->area.y,
            .w = node->area.w - cut,
            .h = node->area.h
        };
    }

    node->back = createNode(back_area);
    node->front = createNode(front_area);
    
    generateNodes(node->back, depth-1, split);
    generateNodes(node->front, depth-1, split);
}

int main(void) {
    int taim = time(NULL);
    srand(taim);
    printf("%d \n", taim);
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
    int greates_depth = 3;
    Cell **grid;

    int COLS = (int)(SCREEN_WIDTH/CELL_WIDTH);
    int ROWS = (int)(SCREEN_HEIGHT/CELL_HEIGHT);

    printf("cols: %d\n", COLS);
    printf("rows: %d\n", ROWS);


    grid = malloc(sizeof(Cell*)*COLS);
    for (int i = 0; i < COLS; i++) {
        grid[i] = malloc(sizeof(Cell) * ROWS);
        for (int j = 0; j < ROWS; j++)  {
            grid[i][j] = (Cell) { .pos.x = i, .pos.y = j };
            grid[i][j].kind  = EMPTY;
            grid[i][j].color = WHITE;
            grid[i][j].id = id_counter;
        }
    }

    Rect root_area = {
        .h = ROWS,
        .w = COLS,
        .x = 0, // cols
        .y = 0, // rows
        .grid = grid,
    };

    BSPNode* root = createNode(root_area);
    generateNodes(root, greates_depth, NONE);
    connectRooms(root);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "bsp");
    int animation_speed = 128;
    SetTargetFPS(60);
    int max_depth = 0;
    while (!WindowShouldClose()) {
        BeginDrawing();
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