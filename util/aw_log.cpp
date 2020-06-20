
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "aw_log.h"
#include "aw_util.h"
#include <time.h>
aw_log_callback g_log_callback;
void * g_log_context;
char  g_log_buff[LOG_BUFF_SIZE + 1];
char  g_log_tmp_buff[LOG_BUFF_SIZE + 1];

void aw_register_log_callback(aw_log_callback cb, void * context)
{
	g_log_callback = cb;
	g_log_context = context;
}
void aw_unregister_log_callback()
{
	g_log_callback = NULL;
	g_log_context = NULL;
}
int aw_tag_check(const char * type)
{
	if ((strcmp(type, AW_INFO) == 0)
		||(strcmp(type, AW_ERR) == 0)
		||(strcmp(type, AW_WARN) == 0))
	{
		return true;
	}
	char* value = getenv(type);
	if (value == NULL || strcmp(value,"1") != 0)
	{
		return false;
	}
	return true;
}
void aw_log_set_tag(char *set_tag_string)
{
	char tag_name[128];
	char tag_value[128];
	sscanf(set_tag_string, "%s %s",tag_name,tag_value);
	printf("set tag name %s value %s\n",tag_name,tag_value);
#ifdef _WINDOWS
#else
	setenv(tag_name,tag_value,1);
#endif
}
void AW_LOG(char *log, const char *type,const char *fun, int line)
{
	if (!aw_tag_check(TAG_LOG_CONSOLE)&& !aw_tag_check(TAG_LOG_CALLBACK))
	{
		return ;
	}
	time_t t;
	struct tm *p;
	
	time(&t);
	p=localtime(&t);
	char timeBuffer [64];
	strftime (timeBuffer,sizeof(timeBuffer),"[%Y/%m/%d %H:%M:%S]",p);
	char tag[20];
	snprintf(tag, 18, "[%s", type);
	strcat(tag, "]:");
	if (aw_tag_check(TAG_LOG_MS))
	{
		snprintf((char * const)g_log_tmp_buff, LOG_BUFF_SIZE, "%s [%lums] %-8s %s", timeBuffer, GetTickCount(), tag, g_log_buff);
	}
	else{
		snprintf((char * const)g_log_tmp_buff, LOG_BUFF_SIZE, "%s %-8s %s", timeBuffer, tag, g_log_buff);
	}
	strncpy(g_log_buff, g_log_tmp_buff,sizeof(g_log_buff));
	if (AW_LOG_FUNCTION_TRACE == 1)
	{
		sprintf((char * const)g_log_tmp_buff, " [func:%s] [line:%d]", fun, line);
		strncat(g_log_buff, g_log_tmp_buff, LOG_BUFF_SIZE - strlen(g_log_buff));
	}
	if (aw_tag_check(TAG_LOG_CONSOLE))
		strcat(g_log_buff, "\n");

	printf("%s",g_log_buff);
	if (NULL != g_log_callback && aw_tag_check(TAG_LOG_CALLBACK))
		g_log_callback(g_log_buff, g_log_context);
}
