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
// https://en.wikipedia.org/wiki/Tree_traversal
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#define MIN_SIZE_ROOM 7
#define CELL_WIDTH    20
#define CELL_HEIGHT   20
#define NONE          2
#define VISION_RADIUS 10
typedef struct CellPos {
    int x;
    int y;
} CellPos;

typedef enum CellKind {
    EMPTY,
    EXIT,
    WALL,
    GROUND,
    MAIN_CHARACTER
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

typedef struct Character {
    int   x, y;
    int   w, h;
} Character;

static bool debug = false;
typedef struct BSPNode {
    struct BSPNode* front;
    struct BSPNode* back;
    Rect area;
    Rect room; // the leafs
} BSPNode;

typedef struct Map {
    Rect area;
    int  frames;
    int  id_counter;
    int  level;
    bool next;
} Map;
static bool pause = false;
static Character chara;

void drawNodes(BSPNode* node, int current_depth, int max_depth, Map* map) {
    if (node == NULL || current_depth > max_depth) return;
    int startX = chara.x - VISION_RADIUS*3;  
    int endX   = chara.x + VISION_RADIUS*3;
    int startY = chara.y - VISION_RADIUS*3;
    int endY   = chara.y + VISION_RADIUS*3;

    if (startX < 0) startX = 0; // yeah...
    if (startY < 0) startY = 0; // for limits
    if (endX > map->area.w) endX = map->area.w; // same
    if (endY > map->area.h) endY = map->area.h; // here

    for (int i = startX; i < endX; i++)  {
        for (int j = startY; j < endY; j++)  {
            DrawRectangle(
                i * CELL_WIDTH,
                j * CELL_HEIGHT,
                CELL_WIDTH, CELL_HEIGHT, // we can add -1 to see the cells "rejilla" in englis idk rej?
                map->area.grid[i][j].color
            );
        }
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

// better i would make the character and placint items, mobs in the global grid (root->area)
void placingMobs(Map* map) {
    int COLS = map->area.w;
    int ROWS = map->area.h;
    bool founded = false;
    bool isExit = false;
    for (int i = 0; i < COLS; i++)  {
        for (int j = 0; j < ROWS; j++)  {
            if (map->area.grid[i][j].id == map->id_counter - 2 && !isExit) {
                map->area.grid[i][j].color = BLUE;
                map->area.grid[i][j].kind = EXIT;
                isExit = true;
            }

            // character, his name is Pablo
            if (map->area.grid[i][j].kind == GROUND && map->area.grid[i][j].id == 0 && !founded) {
                map->area.grid[i][j].kind = MAIN_CHARACTER;
                map->area.grid[i][j].color = BLACK;
                chara.h = 1;
                chara.w = 1;
                chara.x = i;
                chara.y = j;
                founded = true;
            }
        }
    }
}

void generateRoom(BSPNode* node, Map* map) {
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
            node->room.grid[i][j].kind  = GROUND;
            node->room.grid[i][j].id    = map->id_counter;
            map->area.grid[i+node->room.x][j+node->room.y].color  = GRAY;
            map->area.grid[i+node->room.x][j+node->room.y].kind   = GROUND;
            map->area.grid[i+node->room.x][j+node->room.y].id     = map->id_counter;
        }
    }
    map->id_counter+=1;
}

BSPNode* connectRooms(BSPNode* node, Map* map) { // by postorder
    if (node == NULL) return node;
    if (node->back == NULL && node->front == NULL) return node;

    BSPNode* back_room = connectRooms(node->back, map);
    BSPNode* front_room = connectRooms(node->front, map);

    // cols and rows for back and front nodes
    const int COLS_ONE = back_room->room.w;    
    const int ROWS_ONE = back_room->room.h;   

    const int COLS_TWO = front_room->room.w;    
    const int ROWS_TWO = front_room->room.h;   

    if (COLS_ONE <= 0 || ROWS_ONE <= 0 || COLS_TWO <= 0 || ROWS_TWO <= 0) {
        fprintf(stderr, "Error: Room dimensions invalid\n");
        return node;
    }

    // generate exit cells
    // border size in each direction
    int rantbo = rand() % (COLS_ONE); // one
    int ranlro = rand() % (ROWS_ONE); // one

    int rantbt = rand() % (COLS_TWO); // two
    int ranlrt = rand() % (ROWS_TWO); // two

    back_room->room.grid[rantbo][ranlro].kind = GROUND;
    back_room->room.grid[rantbo][ranlro].color = GRAY;
    map->area.grid[rantbo+back_room->room.x][ranlro+back_room->room.y].kind = GROUND;
    map->area.grid[rantbo+back_room->room.x][ranlro+back_room->room.y].color = GRAY;

    front_room->room.grid[rantbt][ranlrt].kind = GROUND;
    front_room->room.grid[rantbt][ranlrt].color = GRAY;
    map->area.grid[rantbt+front_room->room.x][ranlrt+front_room->room.y].kind = GROUND;
    map->area.grid[rantbt+front_room->room.x][ranlrt+front_room->room.y].color = GRAY;

    Path pa = {0};

    // absolute values
    int x1 = back_room->room.x + rantbo;
    int y1 = back_room->room.y + ranlro;

    int x2 = front_room->room.x + rantbt;
    int y2 = front_room->room.y + ranlrt;

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

        map->area.grid[x1][y1].color = GRAY;
        map->area.grid[x1][y1].kind = GROUND;

        path_append(pa, pos);
    }
    
    node->area.path = pa;

    return (rand()%2 == 0) ? front_room : back_room;
}

void generateNodes(BSPNode* node, int depth, enum Split last_split, Map* map) {
    if (depth == 0) {
        generateRoom(node, map);
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
    
    generateNodes(node->back, depth-1, split, map);
    generateNodes(node->front, depth-1, split, map);
}

void freeNode(BSPNode* node) {
    if (node == NULL) return;
    int COLS_A = node->area.w;
    if (node->area.grid != NULL) {
        for (int i = 0; i < COLS_A; i++)    {
            free(node->area.grid[i]);
        }
        free(node->area.grid);
    }

    // if is leaf    
    if (node->back == NULL && node->front == NULL && node->room.grid != NULL) {
        int COLS_R = node->room.w;
        for (int i = 0; i < COLS_R; i++) {
            free(node->room.grid[i]);
        }
        free(node->room.grid);
    }

    if (node->area.path.items != NULL) {
        free(node->area.path.items);
    }

    freeNode(node->back);
    freeNode(node->front);

    free(node);
}

void moveChara(Map* map, int dx, int dy) {
    int nextX = chara.x + dx;
    int nextY = chara.y - dy;

    if (map->area.grid[nextX][nextY].kind == EXIT) {
        map->level++;
        map->next = true;
    }; 

    if (nextX < map->area.w || nextY < map->area.h) {
        if (map->area.grid[nextX][nextY].kind == GROUND) {
            
            map->area.grid[chara.x][chara.y].kind = GROUND;
            map->area.grid[chara.x][chara.y].color = GRAY;

            chara.y = nextY;
            chara.x = nextX;

            map->area.grid[chara.x][chara.y].kind = MAIN_CHARACTER;
            map->area.grid[chara.x][chara.y].color = BLACK;
        }

    }

}

int main(void) {
    int taim = time(NULL);
    srand(taim);
    printf("%d \n", taim);
    InitWindow(100, 200, "bsp");

    int monitor = GetCurrentMonitor();
    int SCREEN_WIDTH = GetMonitorWidth(monitor);
    int SCREEN_HEIGHT = GetMonitorHeight(monitor);
    int greates_depth = 3;
    Cell **grid;
    Map map;
    int COLS = SCREEN_WIDTH/CELL_WIDTH;
    int ROWS = SCREEN_HEIGHT/CELL_HEIGHT;
    printf("cols: %d\n", COLS);
    printf("rows: %d\n", ROWS);

    grid = malloc(sizeof(Cell*)*COLS);
    for (int i = 0; i < COLS; i++) {
        grid[i] = malloc(sizeof(Cell) * ROWS);
        for (int j = 0; j < ROWS; j++)  {
            grid[i][j] = (Cell) { .pos.x = i, .pos.y = j };
            grid[i][j].kind  = EMPTY;
            grid[i][j].color = WHITE;
            grid[i][j].id = map.id_counter;
        }
    }

    Rect root_area = {
        .h = ROWS,
        .w = COLS,
        .x = 0, // cols
        .y = 0, // rows
        .grid = grid,
    };

    map.area = root_area;
    map.frames = 0;
    map.id_counter = 0;
    map.level = 0;
    map.next = false;

    BSPNode* root = createNode(root_area);
    generateNodes(root, greates_depth, NONE, &map);
    // let "ready" be any leaf
    BSPNode* ready = connectRooms(root, &map);
    placingMobs(&map);

    Camera2D pablo = { 0 };
    pablo.target = (Vector2){.x = chara.x, .y = chara.y};
    pablo.offset = (Vector2){SCREEN_WIDTH, SCREEN_HEIGHT};
    pablo.rotation = 0.0f;
    pablo.zoom = 1.5f;

    RenderTexture2D screenPablo = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    SetConfigFlags(FLAG_WINDOW_TOPMOST | FLAG_WINDOW_UNDECORATED);
    ToggleFullscreen();
    int animation_speed = 128;
    SetTargetFPS(60);
    int max_depth = 0;
    int speed = 4;
    while (!WindowShouldClose()) {
        if (map.next) {
            // freeNode(root);
            
            grid = malloc(sizeof(Cell*)*COLS);
            for (int i = 0; i < COLS; i++) {
                    grid[i] = malloc(sizeof(Cell) * ROWS);
                    for (int j = 0; j < ROWS; j++)  {
                        grid[i][j] = (Cell) { .pos.x = i, .pos.y = j };
                        grid[i][j].kind  = EMPTY;
                        grid[i][j].color = WHITE;
                        grid[i][j].id = map.id_counter;
                    }
                }

                root_area = (Rect){
                    .h = ROWS,
                    .w = COLS,
                    .x = 0, // cols
                    .y = 0, // rows
                    .grid = grid,
                };

                map.area = root_area;
                map.frames = 0;
                map.id_counter = 0;
                map.next = false;
                srand(taim + map.level);
                root = createNode(root_area);
                generateNodes(root, greates_depth, NONE, &map);
                // let "ready" be any leaf
                BSPNode* ready = connectRooms(root, &map);
                placingMobs(&map);

            }
            
            if (IsKeyDown(KEY_RIGHT)) if(map.frames % speed == 0) moveChara(&map, 1, 0);
            if (IsKeyDown(KEY_DOWN))  if(map.frames % speed == 0) moveChara(&map, 0, -1);
            if (IsKeyDown(KEY_UP))    if(map.frames % speed == 0) moveChara(&map, 0, 1);
            if (IsKeyDown(KEY_LEFT))  if(map.frames % speed == 0) moveChara(&map, -1, 0);


            if (IsKeyPressed(KEY_D)) debug = !debug;
            if (IsKeyPressed(KEY_P)) pause = !pause;
            if (pause) map.frames--;
            if (!pause) map.frames++;

            if (map.frames % animation_speed == 0 && max_depth < greates_depth+1) {
                max_depth++;
            }

        ClearBackground(RAYWHITE);


        pablo.target = (Vector2){ chara.x * CELL_WIDTH, chara.y * CELL_HEIGHT };
        pablo.offset = (Vector2){ SCREEN_WIDTH/2, SCREEN_HEIGHT/2 };

        BeginDrawing();
            BeginTextureMode(screenPablo);
                ClearBackground(WHITE);
                BeginMode2D(pablo);
                    drawNodes(root, 0, max_depth, &map);
                EndMode2D();
            EndTextureMode();
        
            DrawTextureRec(
                screenPablo.texture,
                (Rectangle){0, 0, screenPablo.texture.width, -screenPablo.texture.height},
                (Vector2){0, 0},
                WHITE
            );
            if (pause) DrawText("Pause", 50, 50, 20, RED);
        EndDrawing();
    EndDrawing();

    }
    
    CloseWindow();
    return 0;
}