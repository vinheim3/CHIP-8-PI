#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define RAM              4096
#define SCR_HEIGHT       32
#define SCR_WIDTH        64

uint8_t     memory[RAM];
uint16_t    PC;

bool screen[SCR_HEIGHT][SCR_WIDTH];

uint8_t     delay;
int8_t      sound = -1;
bool draw = false;
bool key[16];
bool paused = false;
bool allowDraw;

void initialize(void) {
    PC = 0x200;
    srand(time(NULL));

    uint8_t font[80] = {
        0xF0,0x90,0x90,0x90,0xF0, ///0
        0x20,0x60,0x20,0x20,0x70, ///1
        0xF0,0x10,0xF0,0x80,0xF0, ///2
        0xF0,0x10,0xF0,0x10,0xF0, ///3
        0x90,0x90,0xF0,0x10,0x10, ///4
        0xF0,0x80,0xF0,0x10,0xF0, ///5
        0xF0,0x80,0xF0,0x90,0xF0, ///6
        0xF0,0x10,0x20,0x40,0x40, ///7
        0xF0,0x90,0xF0,0x90,0xF0, ///8
        0xF0,0x90,0xF0,0x10,0xF0, ///9
        0xF0,0x90,0xF0,0x90,0x90, ///A
        0xE0,0x90,0xE0,0x90,0xE0, ///B
        0xF0,0x80,0x80,0x80,0xF0, ///C
        0xE0,0x90,0x90,0x90,0xE0, ///D
        0xF0,0x80,0xF0,0x80,0xF0, ///E
        0xF0,0x80,0xF0,0x80,0x80  ///F
    };
    
    memmove(memory, font, 80);
}

void loadGame(const char *fileName) {
    FILE *file;
    uint16_t fileLen;
    
    file = fopen(fileName, "rb");
    if (!file) {
        printf("Unable to open binary file: %s", fileName);
        exit(-1);
    }
    
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = (char *)malloc(fileLen + 1);
    
    if (!buffer) {
        printf("Memory error!");
        fclose(file);
        exit(-1);
    }
    
    fread(buffer, fileLen, 1, file);
    fclose(file);
    
    if (fileLen + PC > RAM) {
        printf("File too big. Size is: %d", fileLen + PC);
        exit(-1);
    }
    
    for (int i = 0; i < fileLen; i++)
        memory[i + PC] = buffer[i];
    
    free(buffer);
}

void emulatecycle(void) {
    static uint8_t V[16], SP;
    static uint16_t opcode, I, stack[16];
    
    static uint8_t x, y, kk, r;
    static uint16_t nnn;
    static int8_t keyPressed;
    
    if (PC >= RAM) {
        printf("End of RAM reached");
        exit(-1);
    }
    
    opcode = memory[PC] << 8 | memory[PC+1];
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    kk = (opcode & 0x00FF);
    nnn = (opcode & 0x0FFF);
    
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (kk) {
                case 0x00E0: ///clear the display
                    if (!allowDraw) break;
                    for (int i = 0; i < SCR_HEIGHT; i++)
                        for (int j = 0; j < SCR_WIDTH; j++)
                            screen[i][j] = false;
                    draw = true;
                    PC += 2;
                    break;
                case 0x00EE: ///return from subroutine
                    SP--;
                    PC = stack[SP] + 2;
                    break;
                default: ///jump to machine code routine at nnn - old machines only
                    printf("Accessing old machine opcode.\n");
                    exit(-1);
            }
            break;
        case 0x1000: ///jump to location nnn
            PC = nnn;
            break;
        case 0x2000: ///call subroutine at nnn
            stack[SP] = PC;
            SP++;
            PC = nnn;
            break;
        case 0x3000: ///skip next instruction if Vx is kk
            if (V[x] == kk)
                PC += 4;
            else
                PC += 2;
            break;
        case 0x4000: ///skip next instruction if Vx is not kk
            if (V[x] != kk)
                PC += 4;
            else
                PC += 2;
            break;
        case 0x5000: ///skip next instruction if Vx is Vy
            if ((opcode & 0x000F) == 0x0000) {
                if (V[x] == V[y])
                    PC += 4;
                else
                    PC += 2;
            }
            else {
                printf("Invalid opcode 0x5000.\n");
                exit(-1);
            }
            break;
        case 0x6000: ///sets Vx to kk
            V[x] = kk;
            PC += 2;
            break;
        case 0x7000: ///add kk to Vx
            V[x] += kk;
            PC += 2;
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: ///set Vx to Vy
                    V[x] = V[y];
                    PC += 2;
                    break;
                case 0x0001: ///set Vx to Vx | Vy
                    V[x] |= V[y];
                    PC += 2;
                    break;
                case 0x0002: ///set Vx to Vx & Vy
                    V[x] &= V[y];
                    PC += 2;
                    break;
                case 0x0003: ///set Vx to Vx ^ Vy
                    V[x] ^= V[y];
                    PC += 2;
                    break;
                case 0x0004: ///add Vy to Vx and set VF to 1 if there's a carry, 0 if not
                    if (V[x] + V[y] > 0xFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    V[x] = V[x] + V[y] - V[0xF]*0xFF;
                    PC += 2;
                    break;
                case 0x0005: ///subtract Vy from Vx and set VF to 1 if there's no borrow, 0 if there is
                    if (V[y] > V[x])
                        V[0xF] = 0;
                    else
                        V[0xF] = 1;
                    V[x] = V[x] - V[y] + (1-V[0xF])*0xFF;
                    PC += 2;
                    break;
                case 0x0006: ///Vx >> 1, VF is least significant bit (most-right) before
                    V[0xF] = (V[x] & 0x1);
                    V[x] >>= 1;
                    PC += 2;
                    break;
                case 0x0007: ///Vx is Vy-Vx and set VF to 1 if there's no borrow, 0 if there is
                    if (V[x] > V[y])
                        V[0xF] = 0;
                    else
                        V[0xF] = 1;
                    V[x] = V[y] - V[x] + (1-V[0xF])*0xFF;
                    PC += 2;
                    break;
                case 0x000E: ///Vx << 1, VF is most significant bit (most-left)
                    V[0xF]  = (V[x] & 0x80) >> 7;
                    V[x] <<= 1;
                    V[x] &= 0xFFFF;
                    PC += 2;
                    break;
                default:
                    printf("Invalid opcode 0x8000.\n");
                    exit(-1);
            }
            break;
        case 0x9000: ///skip next instruction if Vx is not Vy
            if (V[x] != V[y])
                PC += 4;
            else
                PC += 2;
            break;
        case 0xA000: ///set I to address NNN
            I = nnn;
            PC += 2;
            break;
        case 0xB000: ///jump to address NNN+V0
            PC = nnn + V[0];
            break;
        case 0xC000: ///Vx is kk & (random number)dest
            r = rand();
            V[x] = kk & r;
            PC += 2;
            break;
        case 0xD000: ///draws sprites at I at Vx,Vy with N lines drawn
            if (!allowDraw) break;
            V[0xF] = 0;
            uint16_t pixel;
            for (int i = 0; i < (opcode & 0x000F); i++) {
                pixel = memory[I + i];
                for (int j = 0; j < 8; j++) {
                    if ( (pixel & (0x80 >> j)) != 0) {
                        uint8_t screenYI = (V[y] + i) % SCR_HEIGHT;
                        uint8_t screenXI = (V[x] + j) % SCR_WIDTH;
                        if (screen[screenYI][screenXI] == 1)
                            V[0xF] = 1;
                        screen[screenYI][screenXI] ^= 1;
                    }
                }
            }
            PC += 2;
            draw = true;
            break;
        case 0xE000:
            switch (kk) {
                case 0x009E: ///skip next instruction if key in Vx is pressed
                    if (key[V[x]]) {
                        PC += 4;
                        key[V[x]] = false;
                    }
                    else
                        PC += 2;
                    break;
                case 0x00A1: ///skip next instruction if key in Vx is not pressed
                    if (!key[V[x]])
                        PC += 4;
                    else {
                        PC += 2;
                        key[V[x]] = false;
                    }
                    break;
                default:
                    printf("Invalid opcode 0xE000.\n");
                    exit(-1);
            }
            break;
        case 0xF000:
            switch (kk) {
                case 0x0007: ///sets Vx to delay timer value
                    V[x] = delay;
                    PC += 2;
                    break;
                case 0x000A: ///awaits a key press, then stores it in Vx
                    paused = true;
                    keyPressed = -1;
                    for (int i = 0; i < 16; i++) {
                        if (key[i]) {
                            keyPressed = i;
                            break;
                        }
                    }
                    
                    if (keyPressed != -1) {
                        V[x] = keyPressed;
                        paused = false;
                        PC += 2;
                    }
                    break;
                case 0x0015: ///sets delay timer to Vx
                    delay = V[x];
                    PC += 2;
                    break;
                case 0x0018: ///sets sound timer to Vx
                    sound = V[x];
                    PC += 2;
                    break;
                case 0x001E: ///adds Vx to I
                    I += V[x];
                    if (I > 0xFFF)
                        exit(-1);
                    PC += 2;
                    break;
                case 0x0029: ///sets I to the location of font char in Vx
                    I = V[x] * 5;
                    PC += 2;
                    break;
                case 0x0033: ///I, I+1, I+2 set to hundreds, tens and digits of decimal representation of Vx
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    PC += 2;
                    break;
                case 0x0055: ///copies V0 to Vx in memory starting at I
                    for (int i = 0; i <= x; i++)
                        memory[I + i] = V[i];
                    PC += 2;
                    break;
                case 0x0065:///fills V0 to Vx with values from memory starting from I
                    for (int i = 0; i <= x; i++)
                        V[i] = memory[I + i];
                    PC += 2;
                    break;
                default:
                    printf("Invalid opcode 0xF000.\n");
                    exit(-1);
            }
            break;
    }
}
