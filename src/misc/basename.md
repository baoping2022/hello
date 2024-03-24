函数原型如下：

#include <libgen.h>

char *basename(char *path);
参数path是一个指向以null结尾的字符串，表示文件路径。

函数返回一个指向提取出的文件名的字符串指针。

以下是一个使用basename函数的示例：

#include <stdio.h>
#include <libgen.h>

int main() {
    char path[] = "/path/to/file.txt";
    char *filename = basename(path);
    printf("Filename: %s\n", filename);
    return 0;
}
输出结果为：

Filename: file.txt
