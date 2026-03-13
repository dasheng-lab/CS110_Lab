#include <stdio.h>
#include <string.h>
#include <stdint.h>
void printbin(uint32_t num, int bits){
    for (int i = bits - 1; i >= 0; i--){
        printf("%d", (num >> i) & 0x1);
    }
}
int main(void) { 
    uint32_t Op1, Op2;
    while(scanf("%x %x", &Op1, &Op2) == 2){
        char Op1_T[32] = "normal";
        char Op2_T[32] = "normal";
        int32_t Op1_S = (Op1 >> 15) & 0x1;
        int32_t Op2_S = (Op2 >> 15) & 0x1;
        int32_t Op1_E = ((Op1 >> 10) & 0x1F) - 15;
        int32_t Op2_E = ((Op2 >> 10) & 0x1F) - 15;
        uint32_t Op1_M = Op1 & 0x3FF;
        uint32_t Op2_M = Op2 & 0x3FF;
        if (Op1_E == -15 && Op1_M != 0){
            strcpy(Op1_T, "subnormal");
            Op1_E = -14;
        }
        else if (Op1_E == -15 && Op1_M == 0){
            strcpy(Op1_T, "zero");
            Op1_E = -14;
        }
        else if (Op1_E == 16 && Op1_M != 0){
            strcpy(Op1_T, "nan");
            Op1_E = 0;
        }
        else if (Op1_E == 16 && Op1_M == 0){
            strcpy(Op1_T, "inf");
            Op1_E = 0;
        }
        if (Op2_E == -15 && Op2_M != 0){
            strcpy(Op2_T, "subnormal");
            Op2_E = -14;
        }
        else if (Op2_E == -15 && Op2_M == 0){
            strcpy(Op2_T, "zero");
            Op2_E = -14;
        }
        else if (Op2_E == 16 && Op2_M != 0){
            strcpy(Op2_T, "nan");
            Op2_E = 0;
        }
        else if (Op2_E == 16 && Op2_M == 0){
            strcpy(Op2_T, "inf");
            Op2_E = 0;
        }
            
        uint32_t isNotLow1 = (strcmp(Op1_T, "subnormal") && strcmp(Op1_T, "zero"))? 1 : 0;
        uint32_t isNotLow2 = (strcmp(Op2_T, "subnormal") && strcmp(Op2_T, "zero"))? 1 : 0;
        printf("Op1: S=%d E=%d M=%u.%03x %s\n", Op1_S, Op1_E, isNotLow1, Op1_M*4, Op1_T);
        printf("Op2: S=%d E=%d M=%u.%03x %s\n", Op2_S, Op2_E, isNotLow2, Op2_M*4, Op2_T);
        if ((!strcmp(Op1_T, "nan") || !strcmp(Op2_T, "nan")) || (!strcmp(Op1_T, "inf") && !strcmp(Op2_T, "zero")) || (!strcmp(Op1_T, "zero") && !strcmp(Op2_T, "inf")))
            printf("Raw: N/A\nNorm: N/A\nResult: N/A\n");
        else if(!strcmp(Op1_T, "inf")){
            uint32_t Op1_T1 = (Op1_S ^ Op2_S) << 15;
            uint32_t Op1_T2 = Op1 & 0x7FFF;
            uint32_t Op1_T3 = Op1_T1 | Op1_T2;
            printf("Raw:N/A\nNorm:N/A\nResult:%04x\n", Op1_T3);
        }
        else if(!strcmp(Op2_T, "inf")){
            uint32_t Op2_T1 = (Op2_S ^ Op1_S) << 15;
            uint32_t Op2_T2 = Op2 & 0x7FFF;
            uint32_t Op2_T3 = Op2_T1 | Op2_T2;
            printf("Raw:N/A\nNorm:N/A\nResult:%04x\n", Op2_T3);
        }
        else{
            uint32_t Op1_M1, Op2_M1;
            if (!strcmp(Op1_T, "normal"))
                Op1_M1 = Op1_M | 0x400;
            else
                Op1_M1 = Op1_M;
            if (!strcmp(Op2_T, "normal"))
                Op2_M1 = Op2_M | 0x400;
            else
                Op2_M1 = Op2_M;
            int32_t Res_E = Op1_E + Op2_E;
            uint32_t Res_M1 = Op1_M1 * Op2_M1;
            int32_t Res_S = Op1_S ^ Op2_S;
            printf("Raw: ");printbin(Res_M1, 22);printf(" E_raw=%d\n", Res_E);
            uint32_t rmbit = 10, rmbit_udf = 0, denorm = 0;
            if (Res_M1 >= 0x200000){
                Res_E++;
                rmbit += 1;
            }
            if (Res_E < -14){
                rmbit_udf = -14 - Res_E;
                Res_E = -14;
                denorm = 1;
            }
            int32_t outE = Res_E;
            uint32_t Frac,G,R,S=0,Inexact,pointpos;
            Frac = (Res_M1 >> (rmbit + rmbit_udf)) & 0x3FF;
            uint32_t outFrac = Frac;
            pointpos = rmbit - 1 + rmbit_udf + 12;
            G = Res_M1 & (0x1 << (rmbit - 1 + rmbit_udf));
            R = Res_M1 & (0x1 << (rmbit - 2 + rmbit_udf));
            for (uint32_t i = 0; i < rmbit - 2 + rmbit_udf; i++){
                S |= Res_M1 & (0x1 << i);
            }
            if (S != 0)S=1;
            Inexact = G | R | S;
            char IsRound[32] = "Truncate";
            if (Inexact && !Res_S){
                strcpy(IsRound, "Up");
                Frac++;
                if (Frac >= 0x400){
                    Frac = 0;
                    Res_E++;
                    pointpos++;
                }
            }
            printf("Norm: E_norm=%d Fraction=", outE);printbin(outFrac, 10);printf(" G=%u R=%u S=%u Action=%s\n", G, R, S, IsRound);
            uint32_t Res;
            if (Res_E > 15){
                if (Res_S == 0) Res = 0x7C00; //+inf
                else Res = 0xFBFF; //not -inf
            }
            else if (denorm){ //denorm with E overflow
                Res = (Res_S << 15) | Frac;
            }
            else if (!strcmp(Op1_T, "zero") || !strcmp(Op2_T, "zero")){ 
                Res = 0;
            }
            else if (Res_M1 & (0x1 << (pointpos - 1))){
                Res = (Res_S << 15) | (Res_E + 15) << 10 | Frac; //norm
            }
            else{
                Res = (Res_S << 15) | (Res_E + 14) << 10 | Frac; //denorm without E overflow, shit !!!
            }
            printf("Result:%04x\n", Res);
        }
    }

    
    return 0; 
}