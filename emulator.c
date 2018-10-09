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

bool loadGame(const char *fileName) {
    FILE *file;
    uint16_t fileLen;
    
    file = fopen(fileName, "rb");
    if (!file) {
        printf("Unable to open binary file: %s", fileName);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = (char *)malloc(fileLen + 1);
    
    if (!buffer) {
        printf("Memory error!");
        fclose(file);
        return false;
    }
    
    fread(buffer, fileLen, 1, file);
    fclose(file);
    
    if (fileLen + PC > RAM) {
        printf("File too big. Size is: %d", fileLen + PC);
        return false;
    }
    
    for (int i = 0; i < fileLen; i++)
        memory[i + PC] = buffer[i];
    
    free(buffer);
    return true;
}

bool emulatecycle(void) {
    static uint8_t V[16], SP;
    static uint16_t opcode, I, stack[16];
    
    static uint8_t x, y, kk;
    static uint16_t nnn;
    static int8_t keyPressed;
    
    if (PC >= RAM) {
        printf("End of RAM reached");
        return false;
    }
    
    opcode = memory[PC] << 8 | memory[PC+1];
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0x00F0) >> 4;
    kk = (opcode & 0x00FF);
    nnn = (opcode & 0x0FFF);
    
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (kk) {
                ///clear the display
                case 0x00E0:
                    for (int i = 0; i < SCR_HEIGHT; i++)
                        for (int j = 0; j < SCR_WIDTH; j++)
                            screen[i][j] = false;
                    draw = true;
                    PC += 2;
                    break;
                ///return from subroutine
                case 0x00EE: PC = stack[--SP] + 2; break;
                ///jump to machine code routine at nnn - old machines only
                default: printf("Accessing old machine opcode.\n"); return false;
            }

        ///jump to location nnn
        case 0x1000: PC = nnn; break;
        ///call subroutine at nnn
        case 0x2000: stack[SP++] = PC; PC = nnn; break;
        ///skip next instruction if Vx is kk
        case 0x3000:
            if (V[x] == kk) PC += 4;
            else            PC += 2;
            break;
        ///skip next instruction if Vx is not kk
        case 0x4000:
            if (V[x] != kk) PC += 4;
            else            PC += 2;
            break;
        case 0x5000:
            ///skip next instruction if Vx is Vy
            if ((opcode & 0x000F) == 0x0000) {
                if (V[x] == V[y]) PC += 4;
                else              PC += 2;
                break;
            }

            printf("Invalid opcode 0x5000.\n");
            return false;

        ///sets Vx to kk
        case 0x6000: V[x] = kk; PC += 2; break;
        ///add kk to Vx
        case 0x7000: V[x] += kk; PC += 2; break;
        case 0x8000:
            switch (opcode & 0x000F) {
                ///set Vx to Vy
                case 0x0000: V[x] = V[y]; PC += 2; break;
                ///set Vx to Vx | Vy
                case 0x0001: V[x] |= V[y]; PC += 2; break;
                ///set Vx to Vx & Vy
                case 0x0002: V[x] &= V[y]; PC += 2; break;
                ///set Vx to Vx ^ Vy
                case 0x0003: V[x] ^= V[y]; PC += 2; break;
                ///add Vy to Vx and set VF to 1 if there's a carry, 0 if not
                case 0x0004:
                    if (V[x] + V[y] > 0xFF) V[0xF] = 1;
                    else                    V[0xF] = 0;
                    V[x] = V[x] + V[y] - V[0xF]*0xFF;
                    PC += 2;
                    break;
                ///subtract Vy from Vx and set VF to 1 if there's no borrow, 0 if there is
                case 0x0005:
                    if (V[y] > V[x]) V[0xF] = 0;
                    else             V[0xF] = 1;
                    V[x] = V[x] - V[y] + (1-V[0xF])*0xFF;
                    PC += 2;
                    break;
                ///Vx >> 1, VF is least significant bit (most-right) before
                case 0x0006:
                    V[0xF] = (V[x] & 0x1);
                    V[x] >>= 1;
                    PC += 2;
                    break;
                ///Vx is Vy-Vx and set VF to 1 if there's no borrow, 0 if there is
                case 0x0007:
                    if (V[x] > V[y]) V[0xF] = 0;
                    else             V[0xF] = 1;
                    V[x] = V[y] - V[x] + (1-V[0xF])*0xFF;
                    PC += 2;
                    break;
                ///Vx << 1, VF is most significant bit (most-left)
                case 0x000E:
                    V[0xF]  = (V[x] & 0x80) >> 7;
                    V[x] <<= 1;
                    V[x] &= 0xFF;  // TODO: Do I need this if V[] is 8-bit?
                    PC += 2;
                    break;
                default:
                    printf("Invalid opcode 0x8000.\n");
                    return false;
            }
        ///skip next instruction if Vx is not Vy
        case 0x9000:
            if (V[x] != V[y]) PC += 4;
            else              PC += 2;
            break;
        ///set I to address NNN
        case 0xA000: I = nnn; PC += 2; break;
        ///jump to address NNN+V0
        case 0xB000: PC = nnn + V[0]; break;
        ///Vx is kk & (random number)dest
        case 0xC000: V[x] = kk & rand(); PC += 2; break;
        ///draws sprites at I at Vx,Vy with N lines drawn
        case 0xD000:
            V[0xF] = 0;
            uint16_t pixel;
            for (int i = 0; i < (opcode & 0x000F); i++) {
                pixel = memory[I + i];
                for (int j = 0; j < 8; j++) {
                    if ((pixel & (0x80 >> j)) != 0) {
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
                ///skip next instruction if key in Vx is pressed
                case 0x009E:
                    if (key[V[x]]) {
                        PC += 4;
                        key[V[x]] = false;
                    }
                    else
                        PC += 2;
                    break;
                ///skip next instruction if key in Vx is not pressed
                case 0x00A1:
                    if (!key[V[x]])
                        PC += 4;
                    else {
                        PC += 2;
                        key[V[x]] = false;
                    }
                    break;
                default:
                    printf("Invalid opcode 0xE000.\n");
                    return false;
            }
        case 0xF000:
            switch (kk) {
                ///sets Vx to delay timer value
                case 0x0007: V[x] = delay; PC += 2; break;
                ///awaits a key press, then stores it in Vx
                case 0x000A:
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
                ///sets delay timer to Vx
                case 0x0015: delay = V[x]; PC += 2; break;
                ///sets sound timer to Vx
                case 0x0018: sound = V[x]; PC += 2; break;
                case 0x001E: ///adds Vx to I
                    I += V[x];
                    if (I > 0xFFF)
                        return false;
                    PC += 2;
                    break;
                ///sets I to the location of font char in Vx
                case 0x0029: I = V[x] * 5; PC += 2; break;
                ///I, I+1, I+2 set to hundreds, tens and digits of decimal representation of Vx
                case 0x0033:
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    PC += 2;
                    break;
                ///copies V0 to Vx in memory starting at I
                case 0x0055:
                    for (int i = 0; i <= x; i++)
                        memory[I + i] = V[i];
                    PC += 2;
                    break;
                ///fills V0 to Vx with values from memory starting from I
                case 0x0065:
                    for (int i = 0; i <= x; i++)
                        V[i] = memory[I + i];
                    PC += 2;
                    break;
                default:
                    printf("Invalid opcode 0xF000.\n");
                    return false;
            }
        default:  // should not be reached
            printf("Invalid opcode 0xF000.\n");
            return false;
    }

    return true;
}
