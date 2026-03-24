#include<stdio.h>
#include<stdint.h>

//zero=0,subnormal=1,infinity=2,NaN=3,normal=4
int main(void) { 
    uint16_t input1, input2;
    int num1,num2;
    int E1,E2,S1,S2;
    int kind1,kind2;
    int M1,M2;
    int G=0,R=0,S=0;
    int e_norm,data,on=0;
    char*kind[]={"zero","subnormal","inf","nan","normal"};
    char* operation[]={"Up","Truncate"};
    while (scanf("%hx%hx",&input1,&input2)==2){
        data=21;
        S=0;
        G=0;
        R=0;
        on=0;
        if ((input1&1023)==0){
            if (((input1>>10)&31)==0){
                kind1=0;
            }
            else if (((input1>>10)&31)==31){
                kind1=2;
            }
            else{
                kind1=4;
            }
        }
        else{
            if (((input1>>10)&31)==0){
                kind1=1;
            }
            else if (((input1>>10)&31)==31){
                kind1=3;
            }
            else{
                kind1=4;
            }
        }
        if ((input2&1023)==0){
            if (((input2>>10)&31)==0){
                kind2=0;
            }
            else if (((input2>>10)&31)==31){
                kind2=2;
            }
            else{
                kind2=4;
            }
        }
        else{
            if (((input2>>10)&31)==0){
                kind2=1;
            }
            else if (((input2>>10)&31)==31){
                kind2=3;
            }
            else{
                kind2=4;
            }
        }

        E1=((input1>>10)&31)-15;
        if (kind1==0||kind1==1){
            M1=0;
        }
        else if (kind1==2||kind1==3){
            M1=1;
        }
        else{
            M1=1;
        }
        E2=((input2>>10)&31)-15;
        if (kind2==0||kind2==1){
            M2=0;
        }
        else if (kind2==2||kind2==3){
            M2=1;
        }
        else{
            M2=1;
        }
        S1=(input1>>15)&1;
        S2=(input2>>15)&1;
        num1=(input1&1023)<<2;
        num2=(input2&1023)<<2;
        ////////////////////////row1////////////////////////
        printf("Op1: S=%d E=",S1);
        if (kind1==0||kind1==1)
            printf("-14");
        else if (kind1==2||kind1==3)
            printf("0");
        else
            printf("%d",E1);
        if (E1<=-14)
            E1=-14;
        printf(" M=%d.%03x %s\n",M1,num1,kind[kind1]);
        ////////////////////////row2////////////////////////
        printf("Op2: S=%d E=",S2);
        if (kind2==0||kind2==1)
            printf("-14");
        else if (kind2==2||kind2==3)
            printf("0");
        else
            printf("%d",E2);
        if (E2<=-14)
            E2=-14;
        printf(" M=%d.%03x %s\n",M2,num2,kind[kind2]);
        ////////////////////////row3////////////////////////
        if (((kind1!=2)&&(kind1!=3))&&((kind2!=2)&&(kind2!=3))){
        printf("Raw: ");
        int var1=(input1&1023)+1024*M1;
        int var2=(input2&1023)+1024*M2;
        int var3=var1*var2;
        for (int i=21;i>-1;i--){
            printf("%d",((var3>>i)&1));
        }
        
        printf(" E_raw=%d\n",E1+E2);
        ////////////////////////row4////////////////////////

            e_norm=E1+E2;

        if (((var3>>21)&1)==1){
            e_norm=1+E1+E2;
            on=1;
        }
        else
            for (int k=20;k>=0;k--){
                if ((e_norm<=-14)||(((var3>>k)&1)==1))
                    break;
                e_norm-=1;
            }

        if ((E1+E2<-14)||(kind1==0)||(kind2==0))
            e_norm=-14;
        printf("Norm: E_norm=%d Fraction=",e_norm);
        int count=0;
        int fresult=0;
        for (int j=19+e_norm-(E1+E2);j>0;j--){
            if (j>=32){
                printf("0");
                count++;
                fresult*=2;
                continue;
            }
            printf("%d",((var3>>j)&1));
            count++;
            fresult*=2;
            fresult+=(var3>>j)&1;
            if (count>=10){
                data=j;
                break;
            }
        }
        G=(var3>>(data-1))&1;
        R=(var3>>(data-2))&1;
        for (int k=data-3;k>=0;k--){
            S=S||((var3>>(k))&1);
        }
        int index=1;//oper

        int F=1;//sign

        if (S1==S2)
        {
            if ((G==1)||(R==1)||(S==1))
                index=0;
            F=0;
        }

        printf(" G=%d R=%d S=%d Action=%s\n",G,R,S,operation[index]);
        ////////////////////////row5////////////////////////

        uint16_t result;
        if ((E1+E2+on<-14)||(e_norm==-14&&(var3>>20)==0))
            result=32*F;
        else
            result=32*F+e_norm+15;
        result=result*1024+fresult;
        if (index==0)
            result+=1;
        if ((kind1==0||kind2==0)&&(S1==S2))
            result=0;
        else if ((kind1==0||kind2==0)&&(S1!=S2))
            result=32768;
        else if ((e_norm>15&&(S1==S2)))
            result=31744;
        else if (e_norm>15&&(S1!=S2))
            result=64511;
        printf("Result: %04x\n",result);
    }
    else{
        printf("Raw: N/A\n");
        printf("Norm: N/A\n");
        int var1=(input1&1023)+1024*M1;
        int var2=(input2&1023)+1024*M2;
        int var3=var1*var2;
        ////////////////////////row4////////////////////////

            e_norm=E1+E2;

        if (((var3>>21)&1)==1)
            e_norm=1+E1+E2;
        else
            for (int k=20;k>=0;k--){
                if ((e_norm<=-14)||(((var3>>k)&1)==1))
                    break;
                e_norm-=1;
            }

        if (E1+E2<-14)
            e_norm=-14;
        int count=0;
        int fresult=0;
        for (int j=19+e_norm-(E1+E2);j>0;j--){
            if (j>=32){
                count++;
                fresult*=2;
                continue;
            }
            count++;
            fresult*=2;
            fresult+=(var3>>j)&1;
            if (count>=10){
                data=j;
                break;
            }
        }

        G=(var3>>(data-1))&1;
        R=(var3>>(data-2))&1;
        for (int k=data-3;k>=0;k--){
            S=S||((var3>>(k))&1);
        }
        int index=1;//oper

        int F=1;//sign

        if (S1==S2)
        {
            if ((G==1)||(R==1)||(S==1))
                index=0;
            F=0;
        }
        ////////////////////////row5////////////////////////

        uint16_t result;
        if ((E1+E2<-14)||(e_norm==-14&&(var3>>20)==0))
            result=32*F;
        else
            result=32*F+e_norm+15;
        result=result*1024+fresult;
        if (index==0)
            result+=1;
        if ((kind1==0&&kind2==2)||(kind2==0&&kind1==2)||(kind1==3||kind2==3)){
            printf("Result: N/A\n");
            continue;
        }
        if ((kind1==0||kind2==0)&&(S1==S2))
            result=0;
        else if ((kind1==0||kind2==0)&&(S1!=S2))
            result=32768;
        else if ((e_norm>15&&(S1==S2))||(((kind1==2)||(kind2==2))&&S1==S2))
            result=31744;
        else if ((((kind1==2)||(kind2==2)))&&S1!=S2)
            result=64512;
        else if (e_norm>15&&(S1!=S2))
            result=64511;
        
        printf("Result: %04x\n",result);
    }
    }
    return 0; }