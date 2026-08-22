

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
                cpu->SP+=2;
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

    //   mem.memory[200] = 0x82;
    //   mem.memory[201] = 0x11;


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