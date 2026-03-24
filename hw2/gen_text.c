#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define CASE_NUM 50000

uint16_t rand_fp16() {
    uint8_t a = rand() & 0xFF;
    uint8_t b = rand() & 0xFF;
    return (a << 8) | b;
}

int main() {
    srand(time(NULL));
    FILE* f = fopen("input.txt", "w");
    for (int i = 0; i < CASE_NUM; i++) {
        uint16_t x = rand_fp16();
        uint16_t y = rand_fp16();
        fprintf(f, "%04x %04x\n", x, y);
    }
    fclose(f);
    printf("生成 %d 条测试用例到 input.txt\n", CASE_NUM);
    return 0;
}