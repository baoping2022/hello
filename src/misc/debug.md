## 宏定义实现

```c
#define self_debug(fmt, ...)     \
        do {    \
                if (g_self_debug_switch)   \
                {       \
                        FILE *fp = NULL;        \
                        fp = fopen("log.txt", "a");  \
                        fprintf(fp, "[%d,%s][%s]: ",__LINE__, __func__, __TIME__);      \
                        fprintf(fp, fmt,  ##__VA_ARGS__);       \
                        fclose(fp);             \
                } else {        \
                        printf(fmt, ##__VA_ARGS__);             \
                }       \
        } while (0)
```

## 函数实现

```c
static void self_debug(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	
	if (g_self_debug_switch) {
		FILE *fp = NULL;
		struct timespec time_hm;
	
		if (clock_gettime(CLOCK_REALTIME, &time_hm) < 0) {
			fprintf(stderr, "Unable to get start timestamp, errno = %d(%s).\n", errno,
				strerror(errno));
			goto lable_out;
		}
	
		fp = fopen("/tmp/log.txt", "a");
	
		if (fp == NULL) {
			fprintf(stderr, "Unable to open %s, errno = %d(%s).\n",
				"/tmp/log.txt", errno, strerror(errno));
			goto lable_out;
		}
	
		fprintf(fp, "[%d,%s][%ld]: ", __LINE__, __func__, time_hm.tv_sec);
		vfprintf(fp, fmt, args);
	
		if (fclose(fp) != 0) {
			fprintf(stderr, "Unable to close %s, errno = %d(%s).\n",
				"/tmp/log.txt", errno, strerror(errno));
			goto lable_out;
		}
	} else
		vprintf(fmt, args);

lable_out:
	va_end(args);
}
```

## Standard Predefined Macros

```c
__DATE__
```

该宏扩展为一个字符串常量，描述预处理器运行的日期。 该字符串常量包含十一个字符，看起来像““Feb 12 1996””。 如果该月的第几天小于 10，则在左侧填充一个空格。

如果 GCC 无法确定当前日期，它将发出警告消息（每次编译一次），并且 `__DATE__` 将扩展为 `"??? ?? ????"`。

```c
__TIME__
```

该宏扩展为一个字符串常量，描述预处理器运行的时间。 该字符串常量包含八个字符，看起来像“23:59:01”。

如果 GCC 无法确定当前时间，它将发出警告消息（每次编译一次）并且 __TIME__ 将扩展为“??:??:??”。

```c
__FILE__
```

该宏扩展为当前输入文件的名称，采用 C 字符串常量的形式。

```c
__LINE__
```

该宏以十进制整数常量的形式扩展为当前输入行号。

栗子：

```c
fprintf (stderr, "Internal error: "
                 "negative string length "
                 "%d at %s, line %d.",
         length, __FILE__, __LINE__);
```

## 参考链接

> https://gcc.gnu.org/onlinedocs/cpp/Predefined-Macros.html
>
> https://gcc.gnu.org/onlinedocs/gcc-13.2.0/gcc/#toc-GCC-Command-Options