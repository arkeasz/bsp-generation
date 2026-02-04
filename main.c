// https://antoniosliapis.com/articles/pcgbook_dungeons.php
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
static int frames = 0;
typedef struct Rect {
    int x, y; // posi
    int w, h; // size
} Rect;

#define MIN_SIZE_ROOM = 7;
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
    char str1[20];
    sprintf(str1, "width: %d", node->room.w);
    
    char str2[20];
    sprintf(str2, "height: %d", node->room.h);

    if (debug) {

        DrawText(str1, node->area.x + 5, node->area.y + 25, 20, RED);
        DrawText(str2, node->area.x + 5, node->area.y + 45, 20, RED);
    }

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
    int pad = 15;

    // frist values
    int firstX = node->area.x;
    int firstY = node->area.y;
    int firstW = node->area.w;
    int firstH = node->area.h;

    // 
    float sub = 0.8;
    int subwid = node->area.w*sub;
    int subhei = node->area.h*sub;
    node->room.x = node->area.x + (rand() % (node->area.w - pad - subwid));
    node->room.y = node->area.y + (rand() % (node->area.h - pad - subhei));

    node->room.w = (firstX + firstW - pad) - node->room.x;
    node->room.h = (firstY + firstH - pad*2) - node->room.y;
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
    srand(time(NULL));
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
    int greates_depth = 4;
    
    Rect root_area = {
        .h = SCREEN_WIDTH,
        .w = SCREEN_WIDTH,
        .x = 0, // px
        .y = 0, // px
    };

    BSPNode* root = createNode(root_area);
    generateNodes(root, greates_depth);

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