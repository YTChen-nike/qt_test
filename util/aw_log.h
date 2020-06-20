#pragma once
#include "stdlib.h"
#include "stdio.h"
typedef	void(*aw_log_callback)(char *log, void * context);
;
#define FUNCTION_TRACE() aw_log_tag("FT","[%s][%d]",__FUNCTION__,__LINE__);

void aw_register_log_callback(aw_log_callback fun1, void * context);
void aw_unregister_log_callback();
#define TAG_LOG_MS  "AW_LOG_MS"
#define TAG_LOG_CONSOLE  "AW_LOG_CONSOLE"
#define TAG_LOG_CALLBACK  "AW_LOG_CALLBACK"
#define AW_INFO		"AW_INFO"
#define AW_WARN		"AW_WARN"
#define AW_ERR		"AW_ERROR"

#define LOG_BUFF_SIZE 2048
extern 	char  g_log_buff[LOG_BUFF_SIZE + 1];

#define AW_LOG_FUNCTION_TRACE 0
extern aw_log_callback g_log_callback;
extern void * g_log_context;
int aw_tag_check(const char * type);
void AW_LOG(char *log, const char *type, const char *fun, int line);

#define aw_log(type, format, ...) { if (aw_tag_check(type)){snprintf(g_log_buff,LOG_BUFF_SIZE,format, ##__VA_ARGS__); AW_LOG(g_log_buff, type, __FUNCTION__, __LINE__);}}

#define aw_log_err(format, ...)  aw_log(AW_ERR, format, ##__VA_ARGS__)
#define aw_log_info(format, ...)  aw_log(AW_INFO, format, ##__VA_ARGS__)
#define aw_log_warn(format, ...)  aw_log(AW_WARN, format, ##__VA_ARGS__)

// tag max size is 16, beyond charater will be lost
#define aw_log_tag(tag, format, ...)  aw_log(tag, format, ##__VA_ARGS__)
void aw_log_set_tag(char *set_tag_string);