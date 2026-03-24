/* This project is based on the MIPS Assembler of CS61C in UC Berkeley.
   The framework of this project is been modified to be suitable for RISC-V
   in CS110 course in ShanghaiTech University by Zhijie Yang in March 2019.
   Updated by Chibin Zhang and Zhe Ye in March 2021.
*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdint.h>
typedef enum
{
   SECTION_TEXT = 0,
   SECTION_DATA = 1
} SectionType;

typedef struct
{
   uint8_t *bytes;
   size_t len;
   size_t cap;
} DataImage;
/*******************************
 * Do Not Modify Code Below
 *******************************/

int assemble(const char* in, const char* out, int test);

#endif
