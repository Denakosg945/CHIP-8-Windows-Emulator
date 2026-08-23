

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32

#include <windows.h>

#endif

#ifdef linux

#include <unistd.h>

#endif

#define STACK_LENGTH (16)
#define MEMORY_LENGTH (4096)
#define WIDTH (64)
#define HEIGHT (32)
#define SPRITE_SIZE (16)
#define SCALE (10)

#define START_ADDRESS 0x200
#define MAX_ROM_NAME 32

//INSTRUCTION NAMES
#define CLS 0x00E0
#define RET 0x00EE
#define JP 0x1000 
#define CALL 0x2000
//These instructions are for basically skipping a single instruction (2 steps in the PC)
#define SE_X_IM 0x3000
#define SNE_X_IM 0x4000
#define SE_X_Y 0x5000
////////////////////////////////////////////////////////////////////////////////////////
#define LD_X_IM 0x6000
#define ADD_X_IM 0x7000
#define LD_X_Y 0x8000
#define OR_X_Y 0x8001
#define AND_X_Y 0x8002
#define XOR_X_Y 0x8003
#define ADD_X_Y 0x8004
#define SUB_X_Y 0x8005
#define SHR_X_Y 0x8006
#define SUBN_X_Y 0x8007
#define SHL_X_Y 0x800E
#define SNE_X_Y 0x9000
#define LD_I_IM 0xA000
#define JP_V0_IM 0xB000
#define RND_X 0xC000
#define DRW_X_Y_N 0xD000 
#define SKP_X 0xE09E 
#define SKNP_X 0xE0A1 
#define LD_X_DT 0xF007
#define LD_X_KEY 0xF00A 
#define LD_DT_X 0xF015
#define LD_ST_X 0xF018
#define ADD_I_X 0xF01E
#define LD_F_X 0xF029 
#define LD_B_X 0xF033
#define SA_I_X 0xF055
#define LA_X_I 0xF065



typedef unsigned char BYTE;
typedef unsigned short WORD;

typedef struct SSprite{
    BYTE sprite[SPRITE_SIZE];
} SPRITE;


typedef struct SCpu{
    //GPR = General Purpose Registers (16 0-F)
    BYTE V[16];
    //Last register(F) should not be used by programs (flag register)
    /////////////////////////////////////////////////////////
    //SPR = Special Purpose Registers (2 delay and sound)
    BYTE Vdelay; BYTE Vsound;   //Both of these when set to a value are decremented by a rate of 60Hz
    WORD PC;
    BYTE SP;
    WORD I; 
    ////////

    WORD STACK[STACK_LENGTH];

} CPU;

typedef struct SMemory{
    BYTE memory[MEMORY_LENGTH];
    //0x000-0x1FF Reserved for the interpreter
    //0x200-0xFFF Program / Data Space
    
} MEMORY;

static const BYTE CHIP8_FONT[80] = {
    0xF0,0x90,0x90,0x90,0xF0, // 0
    0x20,0x60,0x20,0x20,0x70, // 1
    0xF0,0x10,0xF0,0x80,0xF0, // 2
    0xF0,0x10,0xF0,0x10,0xF0, // 3 
    0x90,0x90,0xF0,0x10,0x10, // 4 
    0xF0,0x80,0xF0,0x10,0xF0, // 5
    0xF0,0x80,0xF0,0x90,0xF0, // 6
    0xF0,0x10,0x20,0x40,0x40, // 7 
    0xF0,0x90,0xF0,0x90,0xF0, // 8
    0xF0,0x90,0xF0,0x10,0xF0, // 9
    0xF0,0x90,0xF0,0x90,0x90, // A
    0xE0,0x90,0xE0,0x90,0xE0, // B
    0xF0,0x80,0x80,0x80,0xF0, // C
    0xE0,0x90,0x90,0x90,0xE0, // D
    0xF0,0x80,0xF0,0x80,0xF0, // E
    0xF0,0x80,0xF0,0x80,0x80 // F
};

BYTE screen[HEIGHT * WIDTH];

void initialize(CPU *cpu,MEMORY *mem,BYTE *screen){
    //Initialize registers
    for(int i=0; i<16;i ++){
        cpu->V[i] = 0;
    }
    /////////////////////////////////////////////////////
    cpu->Vdelay = 0; cpu->Vsound = 0;
    cpu->PC = 200;
    cpu->SP = 0x0;
    cpu->I = 0x0;

    //Initialize Stack
    for(int i=0; i<STACK_LENGTH; i++){
        cpu->STACK[i] = 0;
    }   

    //Initialize memory
    for(int i=0; i<MEMORY_LENGTH; i++){
        mem->memory[i] = 0;
    }

    //Initialize screen
    for(int i=0 ;i<WIDTH*HEIGHT; i++){
        screen[i] = 0;
    }

    //Load font into memory
    for(int i=0; i<80; i++){
        mem->memory[i] = CHIP8_FONT[i];
    }

}

static const BYTE keymap[16] = {
    0x61, 0x62, 0x63, 0x64,
    0x51, 0x57, 0x45, 0x52,
    0x41, 0x53, 0x44, 0x46,
    0x5A, 0x58, 0x43, 0x56
};

WORD fetch(CPU *cpu,MEMORY *mem){
    WORD instruction;
    if(cpu->PC < 0xFFF){
        //High bytes 
        instruction = mem->memory[cpu->PC] << 8;
        instruction = instruction | mem->memory[cpu->PC+1];
        cpu->PC+=2;

        return instruction;
    }
    //Error
    return -1;
}

void clrDisplay(){
    for(int i=0; i<WIDTH*HEIGHT; i++){
        screen[i] = 0;
    }
}

void execute(CPU *cpu,MEMORY *memory,WORD instruction){
    if(instruction == CLS){
        clrDisplay();
    }else if(instruction == RET){
        if(cpu->SP > 0x0){
            cpu->PC = cpu->SP;
            cpu->SP -= 1;
        }
    //Get only the first 4 bits and if they are equal with a Call select the branch
    }else if((instruction & 0xF000) == JP ){
        //Check if the nnn bytes are > 200 (PC lowest point)
        if((instruction & 0x0FFF) > 200){
            cpu->PC = (instruction & 0x0FFF);
        }
    }else if((instruction & 0xF000) == CALL){
        if((instruction & 0x0FFF) > 200){
            cpu->STACK[cpu->SP++] = cpu->PC;
            cpu->PC = (instruction & 0x0FFF);
        }
    }else if((instruction & 0xF000) == SE_X_IM){
        //Check if the second hex num (register num) is valid
        BYTE k = (instruction & 0x0F00) >> 8;
        if(k >= 0 && k <= 16){
            if(cpu->V[k] == (instruction & 0x00FF)){
                cpu->PC+=2;
            }
        }
    }else if((instruction & 0xF000) == SNE_X_IM){
        //Check if the second hex num (register num) is valid
        BYTE k = (instruction & 0x0F00) >> 8;
        if(k >= 0 && k <= 16){
            if(cpu->V[k] != (instruction & 0x00FF)){
                cpu->PC+=2;
            }
        }
    }else if((instruction & 0xF000) == SE_X_Y){
        //Check if the register indeces are in the valid range
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            if(cpu->V[x] == cpu->V[y]){
                cpu->PC+=2;
            }
        }
    }else if((instruction & 0xF000) == LD_X_IM){ 
        BYTE k = (instruction & 0x0F00) >> 8;
        if(k >= 0 && k <= 16){  
            cpu->V[k] = (instruction & 0x00FF);
        }
    }else if((instruction & 0xF000) == ADD_X_IM){
        BYTE k = (instruction & 0x0F00) >> 8;
        if(k >= 0 && k <= 16){  
            cpu->V[k] += (instruction & 0x00FF);
        }
    }else if((instruction & 0xF00F) == LD_X_Y){
        //Check if the register indeces are in the valid range
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[y];
        }
    }else if((instruction & 0xF00F) == OR_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[x] | cpu->V[y];
        }
    }else if((instruction & 0xF00F) == AND_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[x] & cpu->V[y];
        }
    }else if((instruction & 0xF00F) == XOR_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[x] ^ cpu->V[y];
        }
    }else if((instruction & 0xF00F) == ADD_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[x] + cpu->V[y];
            if(cpu->V[x] + cpu->V[y] > 255){
                cpu->V[15] = 1;
            }else{
                cpu->V[15] = 0;
            }
        }
    }else if((instruction & 0xF00F) == SUB_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[x] - cpu->V[y];
            if(cpu->V[x] > cpu->V[y]){
                cpu->V[15] = 1;
            }else{
                cpu->V[15] = 0;
            }
        }
    }else if((instruction & 0xF00F) == SHR_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->V[x] =  cpu->V[x] >> 1;
            if((cpu->V[x] & 0x0001) == 1){
                cpu->V[15] = 1;

            }else{
                cpu->V[15] = 0;
            }
        }
    }else if((instruction & 0xF00F) == SUBN_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            cpu->V[x] = cpu->V[y] - cpu->V[x];
            if(cpu->V[y] > cpu->V[x]){
                cpu->V[15] = 1;
            }else{
                cpu->V[15] = 0;
            }
        }
    }else if((instruction & 0xF00F) == SHL_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->V[x] = cpu->V[x] << 1;
            if((cpu->V[x] & 0x8000) == 1){
                cpu->V[15] = 1;
            }else{
                cpu->V[15] = 0;
            }
        } 
    }else if((instruction & 0xF000) == SNE_X_Y){
        BYTE x = (instruction & 0x0F00) >> 8;
        BYTE y = (instruction & 0x00F0) >> 4;
        if((x >= 0 && x <= 16) && (y >= 0 && y <= 16)){
            if(cpu->V[x] != cpu->V[y]){
                cpu->PC += 2;
            }
        }
    }else if((instruction & 0xF000) == LD_I_IM){
        cpu->I = instruction & 0x0FFF;
    }else if((instruction & 0xF000) == JP_V0_IM){
        cpu->PC = (instruction & 0x0FFF) + cpu->V[0];
    }else if((instruction & 0xF000) == RND_X){
        BYTE x = (instruction & 0x0F00) >> 8;
         if((x >= 0 && x <= 16)){
            BYTE rnd_num = rand() % 256;
            cpu->V[x] = rnd_num & (instruction & 0x00FF);
         }
    }else if((instruction & 0xF0FF) == LD_X_DT){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->V[x] = cpu->Vdelay;
        }
    }else if((instruction & 0xF0FF) == LD_DT_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->Vdelay = cpu->V[x];
        }
    }else if((instruction & 0xF0FF) == LD_ST_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->Vsound = cpu->V[x];
        }
    }else if((instruction & 0xF0FF) == ADD_I_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->I += cpu->V[x];
        }
    }else if((instruction & 0xF0FF) == LD_B_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16) && (cpu->I >= 200)){
            //Hundreds at cpu->I
            memory->memory[cpu->I] = cpu->V[x] / 100;
            //Tens at cpu->I+1
            memory->memory[cpu->I+1] = (cpu->V[x] / 10)%10;
            //Ones at cpu->I+2
            memory->memory[cpu->I+2] = cpu->V[x]%10;
        }
    }else if((instruction & 0xF0FF) == SA_I_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16) && (cpu->I >= 200)){
            for(int i=0; i<x; i++){
                memory->memory[cpu->I+i] = cpu->V[i];
            }
        }
    }else if((instruction & 0xF0FF) == LA_X_I){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16) && (cpu->I >= 200)){
            for(int i=0; i<x; i++){
                cpu->V[i] = memory->memory[cpu->I+i] ;
            }
        }
    }else if((instruction & 0xF0FF) == LD_F_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            cpu->I = cpu->V[x] * 5;
        }
    }else if((instruction & 0xF000) == DRW_X_Y_N){
        BYTE reg_x = (instruction & 0x0F00) >> 8;
        BYTE reg_y = (instruction & 0x00F0) >> 4;

        BYTE start_x = cpu->V[reg_x];
        BYTE start_y = cpu->V[reg_y];

        BYTE height = instruction & 0x000F;

        cpu->V[15] = 0;

        for (int row = 0; row < height; row++) {
            BYTE sprite_byte = memory->memory[cpu->I + row];
            
            for (int col = 0; col < 8; col++) {

                if ((sprite_byte & (0x80 >> col)) != 0) {
                    
                    int screen_x = (start_x + col) % 64;
                    int screen_y = (start_y + row) % 32;
                    int pixel_index = screen_x + (screen_y * 64);
                    

                    if (screen[pixel_index] == 1) {
                        cpu->V[15] = 1; 
                    }
                    
                    screen[pixel_index] ^= 1;
                }
            }
        }
    }else if((instruction & 0xF0FF) == SKP_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            if((GetAsyncKeyState(cpu->V[x]) & 0xF000) >= 0x8000){
                cpu->PC+=2;
            }
        }
    }else if((instruction & 0xF0FF) == SKNP_X){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            if((GetAsyncKeyState(cpu->V[x]) & 0xF000) < 0x8000){
                cpu->PC+=2;
            }
        }
    }else if((instruction & LD_X_KEY)){
        BYTE x = (instruction & 0x0F00) >> 8;
        if((x >= 0 && x <= 16)){
            for (int i = 0; i < 16; i++) {
                if (GetAsyncKeyState(keymap[i]) & 0x8000) {
                    cpu->V[x] = keymap[i];
                    break;
                }
            }

        }
        
    }
}


int quit = 0;

void RenderCHIP8(HDC hdc) {
    //Create brushes
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));       // Για τα σβηστά pixels
    HBRUSH whiteBrush = CreateSolidBrush(RGB(102, 255, 0)); // Για τα αναμμένα pixels

    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            
            //Set the rectangles for the screen
            RECT rect;
            rect.left   = x * SCALE;
            rect.top    = y * SCALE;
            rect.right  = rect.left + SCALE;
            rect.bottom = rect.top + SCALE;

            //Check if pixel is lit or not
            if (screen[x + (y * WIDTH)] == 1) {
                FillRect(hdc, &rect, whiteBrush);
            } else {
                FillRect(hdc, &rect, blackBrush);
            }
        }
    }

    // Clear brushes to avoid memory leaks
    DeleteObject(blackBrush);
    DeleteObject(whiteBrush);
}

//Window message handler 
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            //CALL THIS FUNCTION WHEN WINDOWS REQUESTS REDRAWING OF THE CANVAS
            RenderCHIP8(hdc);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow){

    srand(time(NULL));


    const char CLASS_NAME[] = "CHIP8_Window_Class";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);

    RegisterClass(&wc);


    RECT windowRect = {0,0,WIDTH*SCALE,HEIGHT*SCALE};
    AdjustWindowRect(&windowRect,WS_OVERLAPPEDWINDOW,FALSE);

    HWND hwnd = CreateWindowEx(
        0,CLASS_NAME,"CHIP-8 Emulator (Win32 API)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL
    );

    if(hwnd == NULL){
        return 1;

    }

    ShowWindow(hwnd,nCmdShow);


    
    CPU cpu;
    MEMORY mem;

    initialize(&cpu,&mem,screen);

    // for(int i=0; i<MEMORY_LENGTH; i++){
    //     printf("0x%X",mem.memory[i]);
    // }

    



    //Create countdown mechanism
    LARGE_INTEGER frequency;
    LARGE_INTEGER start,now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    double timer_update = 1.0/60;

    cpu.Vdelay = 10;
    //Window messages
    MSG msg = {0};
    //Testing instructions 
    // for(int i = 0; i < 64; i++) screen[i + (10 * 64)] = 1;
    // screen[5 + (5 * 64)] = 1;                             
    // screen[60 + (25 * 64)] = 1;

    
    
    FILE *file = fopen(__argv[1], "rb");

    if (file == NULL) {
        MessageBoxA(
        NULL,
        "Error while reading ROM!",
        "CHIP-8",
        MB_OK | MB_ICONERROR
    );
        msg.message = WM_QUIT;
    } else {
        char buffer[256];

        sprintf_s(buffer, sizeof(buffer),
                "ROM: %s",
                __argv[1]);

        MessageBoxA(NULL, buffer, "CHIP-8", MB_OK);
        size_t bytesRead = fread(
        &mem.memory[START_ADDRESS],
        1,
        sizeof(mem.memory) - START_ADDRESS,
        file
    );

    fclose(file);

    printf("Loaded %zu bytes\n", bytesRead);
}


// Program starts at 0x200
// mem.memory[0x200] = 0x00;
// mem.memory[0x201] = 0xE0;  // CLS

// mem.memory[0x202] = 0x60;
// mem.memory[0x203] = 0x05;  // V0 = 5

// mem.memory[0x204] = 0x61;
// mem.memory[0x205] = 0x05;  // V1 = 5

// mem.memory[0x206] = 0xA3;
// mem.memory[0x207] = 0x00;  // I = 0x300

// mem.memory[0x208] = 0xD0;
// mem.memory[0x209] = 0x15;  // Draw 5 bytes

// mem.memory[0x20A] = 0x12;
// mem.memory[0x20B] = 0x0A;  // JP 0x208


// // Sprite at 0x300
// mem.memory[0x300] = 0xF0;
// mem.memory[0x301] = 0x90;
// mem.memory[0x302] = 0x90;
// mem.memory[0x303] = 0x90;
// mem.memory[0x304] = 0xF0;

    while(!quit){

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //Execute OPCODES HERE - function
        WORD instruction = fetch(&cpu,&mem);
        execute(&cpu,&mem,instruction);


        QueryPerformanceCounter(&now);

        double time = (double)(now.QuadPart - start.QuadPart)/(double)(frequency.QuadPart);

        if(time >= timer_update){
            if(cpu.Vdelay > 0){
                cpu.Vdelay--;
            }
            if(cpu.Vsound > 0){
                cpu.Vsound--;
            }
            timer_update += 1.0/60; //Decrement at a rate of 60Hz

            //Redraw window
            InvalidateRect(hwnd, NULL, FALSE);
            Sleep(1);
        }
    }


    return 0;
}

// int main(void){
//     BYTE screen[HEIGHT * WIDTH];
//     CPU cpu;
//     MEMORY mem;

//     initialize(&cpu,&mem,screen);

//     // for(int i=0; i<MEMORY_LENGTH; i++){
//     //     printf("0x%X",mem.memory[i]);
//     // }

//     //Create countdown mechanism
//     LARGE_INTEGER frequency;
//     LARGE_INTEGER start,now;
//     QueryPerformanceFrequency(&frequency);
//     QueryPerformanceCounter(&start);

//     double timer_update = 1.0/60;

//     cpu.Vdelay = 10;

//     while(!quit){
//         QueryPerformanceCounter(&now);

//         double time = (double)(now.QuadPart - start.QuadPart)/(double)(frequency.QuadPart);

//         if(time >= timer_update){
//             if(cpu.Vdelay > 0){
//                 cpu.Vdelay--;
//             }
//             if(cpu.Vsound > 0){
//                 cpu.Vsound--;
//             }
//             timer_update += 1.0/60; //Decrement at a rate of 60Hz
//             Sleep(1);
//         }
//     }

//     return 0;
// }