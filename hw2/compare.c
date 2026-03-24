#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX 1024

int main() {
    // 1. 运行两个程序，生成输出
    system("./fpmul < input.txt > output1.txt");
    system("./ver2 < input.txt > output2.txt");

    // 打开所有文件
    FILE* f_input = fopen("input.txt", "r");   // 输入用例
    FILE* f1     = fopen("output1.txt", "r");  // 程序1输出
    FILE* f2     = fopen("output2.txt", "r");  // 程序2输出

    if (!f_input || !f1 || !f2) {
        printf("文件打开失败！\n");
        return 1;
    }

    char input_line[LINE_MAX];  // 存放当前输入行
    char line1[LINE_MAX], line2[LINE_MAX];
    int case_idx = 0;
    int line_in_case = 0; // 每个用例内部 0~4 行
    int diff_cnt = 0;

    // 先读第一组输入
    fgets(input_line, LINE_MAX, f_input);
    input_line[strcspn(input_line, "\n")] = 0;

    while (1) {
        char* s1 = fgets(line1, LINE_MAX, f1);
        char* s2 = fgets(line2, LINE_MAX, f2);
        if (!s1 && !s2) break;

        // 去掉换行
        line1[strcspn(line1, "\n")] = 0;
        line2[strcspn(line2, "\n")] = 0;

        // 比较
        if (strcmp(line1, line2) != 0) {
            diff_cnt++;
            if (diff_cnt == 1) {
                printf("\n=============================================\n");
                printf("第 %d 组用例不一致！输入：%s\n", case_idx + 1, input_line);
                printf("=============================================\n");
            }
            printf("第 %d 行：\n", line_in_case + 1);
            printf("A: %s\n", line1);
            printf("B: %s\n", line2);
        }

        line_in_case++;

        // 每5行 = 1个用例结束
        if (line_in_case == 5) {
            case_idx++;
            line_in_case = 0;
            diff_cnt = 0;

            // 读取下一个输入
            if (fgets(input_line, LINE_MAX, f_input)) {
                input_line[strcspn(input_line, "\n")] = 0;
            } else {
                *input_line = 0;
            }
        }
    }

    fclose(f_input);
    fclose(f1);
    fclose(f2);

    printf("\n比对完成！\n");
    return 0;
}