#include "widget.h"
#include "ui_widget.h"
#include "util/cJSON.h"
#include <QDebug>
#include <QColor>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QPainter>
#include <QMessageBox>
#include <QMovie>

#include <synchapi.h>

#define FALSE -1
#define TRUE 0

#define RESOLUTION_320_480 0
#define RESOLUTION_480_854 1
#define RESOLUTION_600_1024 2
#define RESOLUTION_800_1280 3
#define TRANSPARENT_COLOR		QColor(0x00,0x00,0x00,0x00)
#define DEFAULT_NAME_DEALY_CLEAR_SEC  1
#define FACEBOX_DEALY_CLEAR_MS 1

#define MSG_REFRESH_TEXT 				1
#define MSG_REFRESH_FACEBOX             2
#define MSG_REFRESH_IMAGE 				3
#define MSG_PAINT                       4
#define MSG_INITWINDOW                  5

#define IDC_SYSTEAM_NAME				1001
#define IDC_DATE						1002
#define IDC_TIME						1003
#define IDC_SYSTEM_TIPS				1004
#define IDC_IP							1005
#define IDC_VERSION					1006
#define IDC_SN_CODE					1007
#define IDC_NAME						1008
#define IDC_WEEK						1009
#define IDC_PEOPLE_NUMBER				1010

#define IDC_BITMAP_IP_ONLINE_ICON		1019
#define IDC_BITMAP_IP_ICON	            1020
#define IDC_MSG1						1021
#define IDC_MSG2						1022
#define IDC_MSG3						1023
#define IDC_BITMAP_WIFI_ICON		1026
#define IDC_BITMAP_WIFI_ONLINE_ICON		1027
#define IDC_BITMAP_TIP                  1031

#define IDC_BITMAP_DYNAMIC1			1101
#define IDC_BITMAP_DYNAMIC2			1102

#define WIDGET_TYPE_TEXT 0
#define WIDGET_TYPE_BITMAP 1
#define WIDGET_TYPE_GIF  2

#define TEXT_TYPE_STATIC 1
#define TEXT_TYPE_NAME 2
#define TEXT_TYPE_TIME 3
#define TEXT_TYPE_STRINGS 4
#define TEXT_TYPE_ID 5

#define WIDGET_VISIBLE_NONE 0
#define WIDGET_VISIBLE_ALWAYS 1
#define WIDGET_VISIBLE_WHEN_CHECK_SUCC 2
#define WIDGET_VISIBLE_WHEN_CHECK_FAIL 3
#define WIDGET_VISIBLE_TEMP_MODE 4

#define METHOD_FACE 0
#define METHOD_TEMP 40

#define FACE_BOX_STATE_CLOSE 0
#define FACE_BOX_STATE_OPEN  1
#define FACE_BOX_STATE_AUTO  2

#define STATE_NORMAL            0
#define STATE_CHECK_SUCC        1
#define STATE_CHECK_FAIL        2

typedef struct TAG_WidgetUpdateInfo{
        //int type;
        int text_type;
        int id;
        QLabel *label;
        //int x;
        //int y;
        //int w;
        //int h;
        //int visible;
        //int style;
        //int font;
        //int bg_box;
        //int color;
        //int bg_color;
        QRect rc_t;
        //int bitmap_loaded;
        QImage bitmap;
        QMovie *move;
        QMessageBox *msgBox;
        char empty[12];
        char text[128];
        char caption[128];
}WidgetInfo;
//配置参数结构体
typedef struct TAG_GuiConfig{
    int license_check_pass;
    int mac_check_pass;
    int resolution;
    QColor transparent_color;
    QColor facebox_color;
    int facebox_line_width;
    int facebox_ratio;
    int facebox_show_switch;
    int tempbox_show_switch;
    int temp_centre;
    int temp_centre_width;
    int temp_centre_length;
    int pre_centre_x;
    int pre_centre_y;
    int temp_text_offset;
    QRect facebox_show_rect;
    unsigned long name_dealy_clear_sec;
    unsigned long msg1_dealy_clear_sec;
    unsigned long  msg2_dealy_clear_sec;
    unsigned long  msg3_dealy_clear_sec;
    char recognize_state[64];
    QRect cur_face;
    QRect pre_face;
    QRect cur_msg3_text;
    QRect pre_msg3_text;
    QRect cur_tempbox;
    QRect pre_tempbox;
    unsigned long last_update_name_time;
    unsigned long last_update_msg1_time;
    unsigned long last_update_msg2_time;
    unsigned long last_update_msg3_time;
    unsigned long last_refresh_facebox_time;
    unsigned long last_refresh_tempbox_time;
    double similarity;
    int method;
    QColor temp_color;
    QColor temp_low_color;
    QColor temp_high_color;
    int temp_mode_switch;
    int sync_auth_flag;
    char temp_msg[64];
    char cmd[64];
    QRect rc_t;
}GuiConfig;

GuiConfig g_config= {
    0,
    0,
    RESOLUTION_480_854,
    TRANSPARENT_COLOR,
    Qt::cyan,
    2,
    10,
    FACE_BOX_STATE_OPEN,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    {48, 127, 650, 912},
    DEFAULT_NAME_DEALY_CLEAR_SEC,
    DEFAULT_NAME_DEALY_CLEAR_SEC,
    DEFAULT_NAME_DEALY_CLEAR_SEC,
    DEFAULT_NAME_DEALY_CLEAR_SEC,
    {0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {100, 100, 200, 200},
    {100, 100, 200, 200},
    {100, 100, 200, 200},
    {100, 100, 200, 200},
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    METHOD_FACE,
    TRANSPARENT_COLOR,
    TRANSPARENT_COLOR,
    TRANSPARENT_COLOR,
    0,
    0,
    {0},
    {0},
    {100, 100, 200, 200}
};

static int wifi_state = 0;
WidgetInfo g_update_info_facebox;
WidgetInfo g_update_info_tempbox;
std::vector<WidgetInfo> g_widget_check_success;
std::vector<WidgetInfo> g_widget_check_failed;
std::vector<WidgetInfo> g_widget_temp_mode;
std::map<int, WidgetInfo> g_widget_map;
std::map<std::string,std::string> g_string_map;
char g_name[128] = {0};
char g_info_msg[64] = {0};
char sync_auth_text[3][16] = {0};
unsigned long updata_sync_auth_text_time = 1000;

void _InitString()
{
    //加载语言配置文件
    FILE *fd;
    char *json_data = NULL;
    int file_size;
    int ret = 0;
    cJSON* default_value = NULL;

    fd = fopen("../test3/ui/config/gui_strings_en.cfg", "rb+");
    if(fd == NULL){
        printf("[AWAPP_GUI]open string file failed gui_strings.cfg\n");
        return;
    }
    fseek(fd, 0, SEEK_END);
    file_size = ftell(fd);
    json_data = (char*)malloc(file_size);
    if(json_data == NULL){
        printf("[AWAPP_GUI]malloc %d failed!\n", file_size);
        fclose(fd);
        return ;
    }
    fseek(fd, 0, SEEK_SET);
    ret = fread(json_data, 1, file_size, fd);
    if(ret != file_size){
        printf("[AWAPP_GUI]fread failed!\n");
        free(json_data);
        fclose(fd);
        return ;
    }
    fclose(fd);
    default_value = cJSON_Parse(json_data);
    if (default_value != NULL) {
        cJSON *item = default_value->child;
        while(item )
        {
           g_string_map.insert(std::pair<std::string,std::string>(item->string, item->valuestring));
           item = item->next;
        }
        cJSON_Delete(default_value);
    }
    else
    {
        printf("[AWAPP_GUI]parse string file failed\n");
    }
    free(json_data);
}

//获取标志对应的提示语
const char *_GetString(const char *string_tag)
{
    std::map<std::string, std::string>::iterator it;
    it = g_string_map.find(string_tag);
    if(it==g_string_map.end())
    {
        return "";
    }
    else
    {
        return it->second.c_str();
    }
}

const char *GetString(const char *string_tag)
{
    if(strncmp(string_tag, "@", 1) == 0){
        return _GetString(string_tag + 1);
    }
    else {
        return string_tag;
    }
}
//文字控件内容更新
//static void UpdateWidgetText(WidgetInfo *info, Widget *w)
void Widget::UpdateWidgetText(void *ctx)
{
    WidgetInfo *info = (WidgetInfo *)ctx;
    char text[128] = {0};
    switch(info->text_type)
    {
    case TEXT_TYPE_STATIC:
        strcpy(info->text, info->caption);
        break;
    case TEXT_TYPE_STRINGS:
        strcpy(info->text, _GetString(info->caption));
        break;
    case TEXT_TYPE_NAME:
        strcpy(info->text, g_name);
        break;
    case TEXT_TYPE_ID:
        strcpy(info->text, g_info_msg);
        break;
    case TEXT_TYPE_TIME:
        time_t time_now;
        struct tm tm_now;
        time(&time_now);
        localtime_r(&time_now, &tm_now);
        sprintf(text, "%02d : %02d", tm_now.tm_hour, tm_now.tm_min);
        strcpy(info->text, text);
        break;
    }
    //info->label->setText(info->text);
    //ceshi
    this->PrintMsg("check success", info->text);
}

//加载解析gui配置文件
void Widget::LoadGuiLayout()
{
    FILE *fp = NULL;
    char *buff = NULL;
    cJSON *cjson_root=NULL;
    cJSON *cjson_widget=NULL;
    cJSON *cjson_facebox_roi=NULL;
    cJSON *cjson_facebox_color=NULL;
    cJSON *cjson_transparent_color=NULL;
    cJSON *cjson_resolution=NULL;
    cJSON *cjson_facebox_line_width=NULL;
    cJSON *cjson_facebox_ratio=NULL;
    cJSON *cjson_temp_color=NULL;
    cJSON *cjson_temp_high_color=NULL;
    cJSON *cjson_temp_low_color=NULL;
    cJSON *cjson_facebox_show_switch=NULL;
    cJSON *cjson_tempbox_show_switch=NULL;
    cJSON *cjson_temp_centre=NULL;
    cJSON *cjson_temp_centre_width=NULL;
    cJSON *cjson_temp_centre_length=NULL;
    cJSON *cjson_temp_text_offset=NULL;
    cJSON *cjson_updata_sync_auth_text_time = NULL;

    int visible = 0;
    int type = 0;
    int ret = 0;
    int widget_number = 0;
    fp = fopen("../test3/ui/config/gui_layout.cfg","r");
    if(NULL == fp)
    {
       printf("[AWAPP_GUI]open gui layout cfg file failed!\n");
       return;
    }
    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    buff = (char *)malloc(fileSize);
    if (buff == NULL)
    {
        printf("[AWAPP_GUI]malloc %d failed !\n", fileSize);
        goto EXIT;
    }
    fseek(fp, 0, SEEK_SET);
    ret = fread(buff, 1, fileSize, fp);
    if (ret != fileSize)
    {
        printf("[AWAPP_GUI]fread failed !\n");
        goto EXIT;
    }
    cjson_root = cJSON_Parse(buff);

    cjson_transparent_color = cJSON_GetObjectItem(cjson_root,"transparent_color");
    if (cjson_transparent_color)
    {
        cJSON *cj_r = cJSON_GetObjectItem(cjson_transparent_color, "r");
        cJSON *cj_g = cJSON_GetObjectItem(cjson_transparent_color, "g");
        cJSON *cj_b = cJSON_GetObjectItem(cjson_transparent_color, "b");
        if (cj_r != NULL && cj_g != NULL && cj_b != NULL ){
              g_config.transparent_color = QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint);
        }
    }
    //人脸框线颜色设置
    cjson_facebox_color = cJSON_GetObjectItem(cjson_root,"facebox_color");
    if (cjson_facebox_color)
    {
        cJSON *cj_r = cJSON_GetObjectItem(cjson_facebox_color, "r");
        cJSON *cj_g = cJSON_GetObjectItem(cjson_facebox_color, "g");
        cJSON *cj_b = cJSON_GetObjectItem(cjson_facebox_color, "b");
        if (cj_r != NULL && cj_g != NULL && cj_b != NULL ){
             g_config.facebox_color = QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint);
        }
    }
    //人脸框线设置
    cjson_facebox_line_width = cJSON_GetObjectItem(cjson_root,"facebox_line_width");
    if (cjson_facebox_line_width)
    {
        g_config.facebox_line_width = cjson_facebox_line_width->valueint;
    }
    //人脸框刷新区域设置
    cjson_facebox_ratio = cJSON_GetObjectItem(cjson_root,"facebox_ratio");
    if (cjson_facebox_ratio)
    {
        g_config.facebox_ratio = cjson_facebox_ratio->valueint;
    }
    //人脸框显示开关
    cjson_facebox_show_switch = cJSON_GetObjectItem(cjson_root,"facebox_show_switch");
    if (cjson_facebox_show_switch)
    {
       g_config.facebox_show_switch = cjson_facebox_show_switch->valueint;
    }
    //测温框显示开关
    cjson_tempbox_show_switch = cJSON_GetObjectItem(cjson_root,"tempbox_show_switch");
    if (cjson_tempbox_show_switch)
    {
       g_config.tempbox_show_switch = cjson_tempbox_show_switch->valueint;
    }
    //测温中心点显示开关
    cjson_temp_centre = cJSON_GetObjectItem(cjson_root,"temp_centre");
    if (cjson_temp_centre)
    {
        g_config.temp_centre = cjson_temp_centre->valueint;
    }
    cjson_temp_centre_width = cJSON_GetObjectItem(cjson_root,"temp_centre_width");
    if (cjson_temp_centre_width)
    {
        g_config.temp_centre_width = cjson_temp_centre_width->valueint;
    }
    cjson_temp_centre_length = cJSON_GetObjectItem(cjson_root,"temp_centre_length");
    if (cjson_temp_centre_length)
    {
        g_config.temp_centre_length = cjson_temp_centre_length->valueint;
    }
    cjson_temp_text_offset = cJSON_GetObjectItem(cjson_root,"cjson_temp_text_offset");
    if (cjson_temp_text_offset)
    {
        g_config.temp_text_offset = cjson_temp_text_offset->valueint;
    }
    //权限更新提示显示时常设置
    cjson_updata_sync_auth_text_time = cJSON_GetObjectItem(cjson_root, "updata_sync_auth_text_time");
    if(cjson_updata_sync_auth_text_time){
        updata_sync_auth_text_time = cjson_updata_sync_auth_text_time->valueint;
    }
    //设置人脸框刷新区域
    cjson_facebox_roi = cJSON_GetObjectItem(cjson_root,"facebox_roi");
    if (cjson_facebox_roi)
    {
        cJSON * cj_left = cJSON_GetObjectItem(cjson_facebox_roi,"left");
        if (cj_left)
            g_config.facebox_show_rect.setX(cj_left->valueint);
        cJSON * cj_top = cJSON_GetObjectItem(cjson_facebox_roi,"top");
        if (cj_top)
            g_config.facebox_show_rect.setY(cj_top->valueint);
        cJSON * cj_right = cJSON_GetObjectItem(cjson_facebox_roi,"right");
        if (cj_right)
            g_config.facebox_show_rect.setWidth(cj_right->valueint - cj_left->valueint);
        cJSON * cj_bottom = cJSON_GetObjectItem(cjson_facebox_roi,"bottom");
        if (cj_bottom)
            g_config.facebox_show_rect.setHeight(cj_bottom->valueint - cj_top->valueint);
    }
    printf("[AWAPP_GUI]facebox roi %d %d %d %d\n", g_config.facebox_show_rect.x(),
            g_config.facebox_show_rect.y(),
            g_config.facebox_show_rect.width(),
            g_config.facebox_show_rect.height());

    //设置屏幕分辨率
    cjson_resolution = cJSON_GetObjectItem(cjson_root,"resolution");
    if (cjson_resolution)
    {
        if (strcmp(cjson_resolution->valuestring, "600*1024") == 0)
        {
            this->setFixedSize(600,1024);
            g_config.resolution = RESOLUTION_600_1024;
            printf("[AWAPP_GUI]resolution 600*1024\n");
        }
        if (strcmp(cjson_resolution->valuestring, "480*854") == 0)
        {
            this->setFixedSize(480,854);
            g_config.resolution = RESOLUTION_480_854;
            printf("[AWAPP_GUI]resolution 480*854\n");
        }
        if (strcmp(cjson_resolution->valuestring, "320*480") == 0)
        {
            this->setFixedSize(320,480);
            g_config.resolution = RESOLUTION_320_480;
            printf("[AWAPP_GUI]resolution 320*480\n");
        }
        if (strcmp(cjson_resolution->valuestring, "800*1280") == 0)
        {
            this->setFixedSize(800,1280);
            g_config.resolution = RESOLUTION_800_1280;
            printf("[AWAPP_GUI]resolution 800*1280\n");
        }
    }
    //设置温度值颜色
    cjson_temp_color = cJSON_GetObjectItem(cjson_root,"temp_color");
    if (cjson_temp_color){
        cJSON *cj_r = cJSON_GetObjectItem(cjson_temp_color, "r");
        cJSON *cj_g = cJSON_GetObjectItem(cjson_temp_color, "g");
        cJSON *cj_b = cJSON_GetObjectItem(cjson_temp_color, "b");
        if (cj_r != NULL && cj_g != NULL && cj_b != NULL ){
             g_config.temp_color = QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint);
        }
    }
    //设置高温度值颜色
    cjson_temp_high_color = cJSON_GetObjectItem(cjson_root,"temp_high_color");
    if (cjson_temp_high_color)
    {
          cJSON *cj_r = cJSON_GetObjectItem(cjson_temp_high_color, "r");
          cJSON *cj_g = cJSON_GetObjectItem(cjson_temp_high_color, "g");
          cJSON *cj_b = cJSON_GetObjectItem(cjson_temp_high_color, "b");
          if (cj_r != NULL && cj_g != NULL && cj_b != NULL ){
                g_config.temp_high_color = QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint);
          }
    }
    //设置低温度值颜色
    cjson_temp_low_color = cJSON_GetObjectItem(cjson_root,"temp_low_color");
    if (cjson_temp_low_color)
    {
        cJSON *cj_r = cJSON_GetObjectItem(cjson_temp_low_color, "r");
        cJSON *cj_g = cJSON_GetObjectItem(cjson_temp_low_color, "g");
        cJSON *cj_b = cJSON_GetObjectItem(cjson_temp_low_color, "b");
        if (cj_r != NULL && cj_g != NULL && cj_b != NULL )		{
              g_config.temp_low_color = QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint);
        }
    }
    //控件基本信息解析设置
    cjson_widget = cJSON_GetObjectItem(cjson_root,"widget");
    widget_number = cJSON_GetArraySize(cjson_widget);

    for(int i = 0; i < widget_number; i++){
        cJSON *cj_widget_item = cJSON_GetArrayItem(cjson_widget, i);
        cJSON *cj_type= cJSON_GetObjectItem(cj_widget_item,"type");
        cJSON *cj_layout = cJSON_GetObjectItem(cj_widget_item,"layout");
        cJSON *cj_color = cJSON_GetObjectItem(cj_widget_item,"text_color");
        cJSON *cj_bg_color = cJSON_GetObjectItem(cj_widget_item,"bg_color");
        cJSON *cj_x = cJSON_GetObjectItem(cj_layout,"x");
        cJSON *cj_y = cJSON_GetObjectItem(cj_layout,"y");
        cJSON *cj_w = cJSON_GetObjectItem(cj_layout,"w");
        cJSON *cj_h = cJSON_GetObjectItem(cj_layout,"h");
        cJSON *cj_caption = cJSON_GetObjectItem(cj_widget_item, "caption");
        cJSON *cj_text_type = cJSON_GetObjectItem(cj_widget_item, "text_type");
        cJSON *cj_id = cJSON_GetObjectItem(cj_widget_item,"id");
        cJSON *cj_visible = cJSON_GetObjectItem(cj_widget_item,"visible");
        if (cj_widget_item == NULL || cj_type == NULL ||
            cj_x == NULL ||	cj_y == NULL ||
            cj_w == NULL ||	cj_h == NULL ||
            cj_id == NULL || cj_visible == NULL
            )
        {
            printf("[AWAPP_GUI]widget invalid continue %p %d \n", cj_widget_item, i);
            continue;
        }

        WidgetInfo info;
        info.id = cj_id->valueint;
        visible = cj_visible->valueint;

        QLabel *label = new QLabel(this);
        info.label = label;
        label->setGeometry(cj_x->valueint, cj_y->valueint, cj_w->valueint, cj_h->valueint);
        if(cj_type && strcmp(cj_type->valuestring, "bitmap") == 0){
            //图片控件初始化
            type = WIDGET_TYPE_BITMAP;
            cJSON *cj_image_path = cJSON_GetObjectItem(cj_widget_item, "image_path");
            if(cj_image_path == NULL){
                printf("[AWAPP_GUI]bitmap widget invalid continue %p %d \n", cj_widget_item, i);
                continue;
            }
            QImage *img = new QImage;
            if(!(img->load(cj_image_path->valuestring))){
                printf("[AWAPP_GUI] load bitmap(%s) failed \n", cj_image_path->valuestring);
                delete img;
                continue;
            }
            if(info.id == IDC_BITMAP_TIP){
                info.bitmap = *img;
                delete img;
            }
            else{
                info.bitmap = *img;
                label->setPixmap(QPixmap::fromImage(*img));
                delete img;
            }
        }
#if 1
        else if(cj_type &&strcmp(cj_type->valuestring, "gif") == 0){
            //gif图片初始化
            type = WIDGET_TYPE_GIF;
            cJSON *cj_image_path = cJSON_GetObjectItem(cj_widget_item, "image_path");
            if(cj_image_path == NULL){
                printf("[AWAPP_GUI]bitmap widget invalid continue %p %d \n", cj_widget_item, i);
                continue;
            }
            QMovie *move = new QMovie(cj_image_path->valuestring);
            info.move = move;
            label->setMovie(move);
            move->start();
        }
#endif
        else if(cj_type &&strcmp(cj_type->valuestring, "text") == 0){
            //文字控件初始化
            type = WIDGET_TYPE_TEXT;
            cJSON *cj_r = cJSON_GetObjectItem(cj_color, "r");
            cJSON *cj_g = cJSON_GetObjectItem(cj_color, "g");
            cJSON *cj_b = cJSON_GetObjectItem(cj_color, "b");
            cJSON *cj_bg_r = cJSON_GetObjectItem(cj_bg_color, "r");
            cJSON *cj_bg_g = cJSON_GetObjectItem(cj_bg_color, "g");
            cJSON *cj_bg_b = cJSON_GetObjectItem(cj_bg_color, "b");
            cJSON *cj_font = cJSON_GetObjectItem(cj_widget_item, "font");
            cJSON *cj_align = cJSON_GetObjectItem(cj_widget_item, "align");
            cJSON *cj_bg_box = cJSON_GetObjectItem(cj_widget_item,"bg_box");
            if (cj_font == NULL || cj_r == NULL ||cj_g == NULL ||
                cj_b == NULL || cj_caption == NULL || cj_align == NULL){
                    printf("[AWAPP_GUI]text widget invalid continue %p %d \n", cj_widget_item, i);
                    continue;
            }
            /*QPalette pe;
            pe.setColor(QPalette::WindowText, QColor(cj_r->valueint, cj_g->valueint, cj_b->valueint, 0));
            label->setPalette(pe);*/

            QFont ft;
            ft.setPointSize(cj_font->valueint);
            label->setFont(ft);

            char style_str[128] = {0};
            QString style;
            if(cj_bg_box != NULL && cj_bg_box->valueint == 1 && cj_bg_r != NULL && cj_bg_g != NULL && cj_bg_b != NULL){
                sprintf(style_str,"background-color:rgb(%d,%d,%d);color:rgb(%d,%d,%d)",
                           cj_bg_r->valueint, cj_bg_g->valueint, cj_bg_b->valueint,
                          cj_r->valueint, cj_g->valueint, cj_b->valueint);
                style = QString::fromLocal8Bit(style_str);
                label->setStyleSheet(style);
            }
            else{
                sprintf(style_str,"color:rgb(%d,%d,%d)",cj_r->valueint, cj_g->valueint, cj_b->valueint);
                style = QString::fromLocal8Bit(style_str);
                label->setStyleSheet(style);
            }

            Qt::AlignmentFlag text_style;
            if (strcmp(cj_align->valuestring,"simple") == 0)
            {
                 text_style = Qt::AlignJustify;
            }
            else if (strcmp(cj_align->valuestring,"center") == 0)
            {
                 text_style = Qt::AlignHCenter;
            }
            else if (strcmp(cj_align->valuestring,"left") == 0)
            {
                 text_style = Qt::AlignLeft;
            }
            else if (strcmp(cj_align->valuestring,"right") == 0)
            {
                 text_style = Qt::AlignRight;
            }
            label->setAlignment(text_style);
            QMessageBox *msgBox = new QMessageBox(this);
        }
        //文字控件内容类型设置
        if (cj_text_type)
        {
            if (strcmp(cj_text_type->valuestring, "static") == 0)
            {
                info.text_type = TEXT_TYPE_STATIC;
            }
            else if (strcmp(cj_text_type->valuestring, "name") == 0)
            {
                info.text_type = TEXT_TYPE_NAME;
            }
            else if (strcmp(cj_text_type->valuestring, "time") == 0)
            {
                info.text_type = TEXT_TYPE_TIME;
            }
            else if (strcmp(cj_text_type->valuestring, "strings") == 0)
            {
                info.text_type = TEXT_TYPE_STRINGS;
            }
            else if (strcmp(cj_text_type->valuestring, "id") == 0)
            {
                info.text_type = TEXT_TYPE_ID;
            }
       }
       else{
             info.text_type = TEXT_TYPE_STATIC;
       }

       if(IDC_SYSTEAM_NAME == info.id){
           strcpy(info.caption,(char *)_GetString("INIT_TEXT"));
       }

       if (type == WIDGET_TYPE_TEXT)
       {
           /* QString text;
            text = QString::fromLocal8Bit(cj_caption->valuestring);*/
            strncpy(info.caption, cj_caption->valuestring, sizeof(info.caption)-1);
            strncpy(info.text, cj_caption->valuestring, sizeof(info.text)-1);
            label->setText(info.text);
       }

       switch (visible)
       {
       case WIDGET_VISIBLE_WHEN_CHECK_SUCC:
           g_widget_check_success.push_back(info);
           break;
       case WIDGET_VISIBLE_WHEN_CHECK_FAIL:
           g_widget_check_failed.push_back(info);
           break;
       case WIDGET_VISIBLE_TEMP_MODE:
           g_widget_temp_mode.push_back(info);
           if(g_config.temp_mode_switch == 0){
                label->setGeometry(0,0,0,0);
           }
           break;
       }
       g_widget_map[info.id] = info;
    }

   EXIT:
    if(buff)
        free(buff);
    if(fp)
        fclose(fp);
    if(cjson_root)
        cJSON_Delete(cjson_root);
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->msgBox = new QMessageBox(this);

    //设置窗体透明
    QPalette pal = palette();
    pal.setColor(QPalette::Background, QColor(0x00,0xff,0x00,0x00));
    setPalette(pal);

    //控件更新定时器设置
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(UpdateTime()));
    timer->start(1000);

    //人脸框更新定时器设置
    QTimer *facebox_timer = new QTimer(this);
    connect(facebox_timer, SIGNAL(timeout()), this, SLOT(UpdataFaceBox()));
    facebox_timer->start(100);
}

Widget::~Widget()
{
    if(this->msgBox)
        delete msgBox;
    delete ui;
}

void Widget::PrintMsg(const char *title, const char *msg)
{
    //ceshi
    qDebug() << "PrintMsg";
    qDebug() << "msg:" << msg;
    this->msgBox->setText(tr(msg));
    this->msgBox->setStandardButtons(0);
}

#if 1
static int sync_auth_text_index = 0;
unsigned long update_wifi_state_time = 0;
unsigned long last_sync_auth_text_time = 0;
void Widget::UpdateTime()
{
    QString date_str;
    QString time_str;
    QString week_str;

    QDateTime time = QDateTime::currentDateTime();

#if 0
    unsigned long now = time.toTime_t();
    if(now - update_wifi_state_time > 3000){
        update_wifi_state_time = now;
        if(0 == wifi_state){
            snprintf(g_config.cmd,64,"busybox lsusb |grep \"Bus 001 Device 002:\"");
            char *buf = aw_cmd(g_config.cmd);
            if(strlen(buf) > 0){
                 g_widget_map[IDC_BITMAP_WIFI_ICON].need_update = TRUE;
                 wifi_state = 1;
             }
             else {
                  g_widget_map[IDC_BITMAP_WIFI_ICON].need_update = TRUE;
                  wifi_state = -1;
             }
         }
         qDebug() << "[gui]wifi_state:" << wifi_state;

         if(wifi_state > 0){
             snprintf(g_config.cmd,64,"ifconfig wlan0 | grep \"inet addr:\"");
             int wlan0_have_ip = strlen(aw_cmd(g_config.cmd)) != 0 ;
             snprintf(g_config.cmd,64,"ifconfig wlan0 | grep \"RUNNING\"");
             int wlan0_running = strlen(aw_cmd(g_config.cmd)) != 0 ;
             aw_log_tag("WIFI_TAG","[gui]wlan0_running:%d wlan0_have_ip:%d\n", wlan0_running, wlan0_have_ip);
             if(wlan0_have_ip && wlan0_running)
             {
                 wifi_state = 2;
                 if (strcmp(g_widget_map[IDC_BITMAP_WIFI_ICON].text, "online")!= 0)
                 {
                     g_widget_map[IDC_BITMAP_WIFI_ICON].need_update = TRUE;
                     strcpy(g_widget_map[IDC_BITMAP_WIFI_ICON].text, "online");
                     update_text_info = 1;
                  }
              }
              else{
                  wifi_state = 3;
                  if (strcmp(g_widget_map[IDC_BITMAP_WIFI_ICON].text, "offline")!= 0)
                  {
                      g_widget_map[IDC_BITMAP_WIFI_ICON].need_update = TRUE;
                      strcpy(g_widget_map[IDC_BITMAP_WIFI_ICON].text, "offline");
                      update_text_info = 1;
                   }
              }
          }
    }
#endif
    date_str = time.toString("yyyy-MM-dd");
    time_str = time.toString("hh:mm");
    week_str = time.toString("dddd");

    if(!(date_str.isEmpty())){
        if(g_widget_map[IDC_DATE].label != NULL){
            g_widget_map[IDC_DATE].label->clear();
            g_widget_map[IDC_DATE].label->setText(date_str);
        }
    }

    if(!(week_str.isEmpty())){
        if(g_widget_map[IDC_WEEK].label != NULL){
            g_widget_map[IDC_WEEK].label->clear();
            g_widget_map[IDC_WEEK].label->setText(week_str);
        }
    }

    if(!(time_str.isEmpty())){
        if(g_widget_map[IDC_TIME].label != NULL){
            g_widget_map[IDC_TIME].label->clear();
            g_widget_map[IDC_TIME].label->setText(time_str);
        }
    }

    if(strcmp(g_config.recognize_state, "stop")  == 0){
        if(strcmp(g_widget_map[IDC_SYSTEM_TIPS].text, _GetString("SYNC_DATA")) != 0){
            if(g_widget_map[IDC_SYSTEM_TIPS].label != NULL){
                sprintf(g_widget_map[IDC_SYSTEM_TIPS].text, _GetString("SYNC_DATA"));
                g_widget_map[IDC_SYSTEM_TIPS].label->clear();
                g_widget_map[IDC_SYSTEM_TIPS].label->setText(_GetString("SYNC_DATA"));
            }
        }
        g_config.last_update_name_time = time.toTime_t();
    }
    else{
        if(strcmp(g_widget_map[IDC_SYSTEM_TIPS].text, _GetString("SYNC_DATA")) == 0){
            if(g_widget_map[IDC_SYSTEM_TIPS].label != NULL){
                memset(g_widget_map[IDC_SYSTEM_TIPS].text, 0, sizeof(g_widget_map[IDC_SYSTEM_TIPS].text));
                g_widget_map[IDC_SYSTEM_TIPS].label->clear();
                g_config.last_update_name_time = time.toTime_t();
            }
        }
    }

    unsigned long now_name_time = time.toTime_t();
    unsigned long now_msg1_time = time.toTime_t();
    unsigned long now_msg2_time = time.toTime_t();
    unsigned long now_msg3_time = time.toTime_t();

    if(now_name_time - g_config.last_refresh_facebox_time >= FACEBOX_DEALY_CLEAR_MS){
        QRect rc_t_0(0,0,0,0);
        g_update_info_facebox.rc_t = rc_t_0;
        g_config.cur_face = rc_t_0;
        this->update();
    }

    /*if(g_config.method == METHOD_TEMP && g_config.tempbox_show_switch == 1){
        g_update_info_tempbox.need_update = TRUE;
        this->MainWindowProc(MSG_REFRESH_FACEBOX);
    }*/

    if(now_name_time - g_config.last_update_name_time >= g_config.name_dealy_clear_sec)
    {
        for(unsigned int i = 0; i < g_widget_check_success.size(); i++){
            if(strlen(g_widget_check_success[i].text) != 0 && g_widget_check_success[i].label != NULL){
                g_widget_check_success[i].label->clear();
            }
        }
    }

    if(now_name_time - g_config.last_update_name_time >= g_config.name_dealy_clear_sec)
    {
        for(unsigned int i = 0; i < g_widget_check_failed.size(); i++){
            if( strlen(g_widget_check_failed[i].text) != 0 && g_widget_check_failed[i].label != NULL){
                g_widget_check_failed[i].label->clear();
            }
        }
    }

    if(now_msg1_time - g_config.last_update_msg1_time >= g_config.msg1_dealy_clear_sec && g_widget_map[IDC_MSG1].label != NULL)
    {
        if( strlen(g_widget_map[IDC_MSG1].text) != 0)
            g_widget_map[IDC_MSG1].label->clear();
    }

    if(now_msg2_time - g_config.last_update_msg2_time >= g_config.msg2_dealy_clear_sec && g_widget_map[IDC_MSG2].label != NULL)
    {
        if(strlen(g_widget_map[IDC_MSG2].text) != 0){
            if(g_config.method != METHOD_TEMP){
                g_widget_map[IDC_MSG2].label->clear();
            }
            else{
                QString str;
                str = QString::fromLocal8Bit(_GetString("TEMP_MOVE_HEAR"));
                g_widget_map[IDC_MSG2].label->setText(str);
            }
        }
    }

    if(now_msg3_time - g_config.last_update_msg3_time >= g_config.msg3_dealy_clear_sec && g_widget_map[IDC_MSG3].label != NULL)
    {
        if(strlen(g_widget_map[IDC_MSG3].text) != 0){
            if(g_config.method != METHOD_TEMP){
                g_widget_map[IDC_MSG3].label->clear();
            }
            else{
                QString str;
                str = QString::fromLocal8Bit(g_config.temp_msg);
                g_widget_map[IDC_MSG3].label->setText(str);
            }
            QPalette pe;
            pe.setColor(QPalette::WindowText, g_config.temp_color);
            g_widget_map[IDC_MSG3].label->setPalette(pe);
        }
    }

    if(last_sync_auth_text_time + updata_sync_auth_text_time <= time.toTime_t() && g_config.sync_auth_flag == 1)
    {
        if(g_widget_map[IDC_PEOPLE_NUMBER].label != NULL){
            last_sync_auth_text_time = time.toTime_t();
            if(sync_auth_text_index > 2){
                sync_auth_text_index = 0;
            }
            QString str;
            str = QString::fromLocal8Bit(sync_auth_text[sync_auth_text_index]);
            g_widget_map[IDC_PEOPLE_NUMBER].label->setText(str);
            sync_auth_text_index++;
        }
    }

    static unsigned int last_update_ip_time = 0;
    if(last_update_ip_time + 2000 < time.toTime_t() && g_widget_map[IDC_IP].label != NULL)
    {
        last_update_ip_time = time.toTime_t();
        char ip[16] = {0};
        //aw_get_ip(ip);
        if(strlen(ip) == 0){
            g_widget_map[IDC_IP].label->setText("192.168.3.177");
        }
        else{
            QString ip_str;
            ip_str = QString::fromLocal8Bit(ip);
            g_widget_map[IDC_IP].label->setText(ip_str);
        }
    }

    return;
}
#endif

void ProcessGuiData(void *data, int size, void *context)
{
    char *msg = (char *)data;
    Widget *w = (Widget *)context;
    QDateTime time = QDateTime::currentDateTime();
    cJSON *json = cJSON_Parse(msg);
    if(NULL == json)
        return ;

    cJSON *cj_update_info = cJSON_GetObjectItem(json, "update_info");
    if(cj_update_info != NULL){
        cJSON *json = cJSON_GetObjectItem(cj_update_info, "system_name");
        if(json != NULL){
            if(g_widget_map[IDC_SYSTEAM_NAME].label != NULL){
                strcpy(g_widget_map[IDC_SYSTEAM_NAME].text, json->valuestring);
                g_widget_map[IDC_SYSTEAM_NAME].label->setText(json->valuestring);
            }
        }

        /*json = cJSON_GetObjectItem(cj_update_info, "updata_img");
        if(NULL!= json &&  json->valueint == 1)
        {
             LoadShowImageLayout();
             PostMessage(g_main_hwnd, MSG_REFRESH_IMAGE, 0, 0);
        }*/

        json =  cJSON_GetObjectItem(cj_update_info, "device_sn");
        if(NULL != json){
            char sn[17];
            // only display 16 charaters
            int sn_len = strlen(json->valuestring);
            if (sn_len > 16)
                strcpy(sn, json->valuestring + sn_len - 16);
            else
                strcpy(sn, json->valuestring);

            if(g_widget_map[IDC_SN_CODE].label != NULL){
                g_widget_map[IDC_SN_CODE].label->setText(sn);
            }
        }

        json =  cJSON_GetObjectItem(cj_update_info, "system_version");
        if(NULL != json){
            if(g_widget_map[IDC_VERSION].label != NULL)
                g_widget_map[IDC_VERSION].label->setText(json->valuestring);
        }

        json =  cJSON_GetObjectItem(cj_update_info, "recognize_state");
        if(NULL != json){
            if (strcmp(g_config.recognize_state, json->valuestring)!= 0){
                strcpy(g_config.recognize_state, json->valuestring);
            }
        }

        json = cJSON_GetObjectItem(cj_update_info, "sync_auth_flag");
        if(cJSON_IsNumber(json) && g_config.sync_auth_flag != json->valueint){
            g_config.sync_auth_flag = json->valueint;
            if(g_config.sync_auth_flag == 1){
                strncpy(sync_auth_text[0], _GetString("SYNC_AUTH"), sizeof(sync_auth_text[1]) - 1);
            }
            else if(g_config.sync_auth_flag == 0){
                strncpy(sync_auth_text[0], "", sizeof(sync_auth_text[1]) - 1);
            }
        }

        json = cJSON_GetObjectItem(cj_update_info, "percentage");
        if(cJSON_IsNumber(json)){
            sprintf(sync_auth_text[1],"%d%%",  json->valueint);
        }
        else{
            strncpy(sync_auth_text[1], "", sizeof(sync_auth_text[1]) - 1);
        }

        json = cJSON_GetObjectItem(cj_update_info, "people_num");
        if(NULL != json){
            if(g_config.sync_auth_flag == 1){
                sprintf(sync_auth_text[2],"%d",  json->valueint);
            }
            else{
               char number_str[32];
               if(strcmp(g_config.recognize_state, "running") == 0){
                   sprintf(number_str,"%d",  json->valueint);
               }
               else{
                   sprintf(number_str,"%d",  json->valueint);
               }
               if(g_widget_map[IDC_PEOPLE_NUMBER].label != NULL){
                    g_widget_map[IDC_PEOPLE_NUMBER].label->setText(number_str);
               }
            }
        }

        json =  cJSON_GetObjectItem(cj_update_info, "net_state");
        if(NULL != json){
            WidgetInfo info;
            //ceshi
            qDebug() << "net_state" << json->valuestring;
            if(strcmp(json->valuestring, "online") == 0){
                info = g_widget_map[IDC_BITMAP_IP_ONLINE_ICON];
                if(info.label != NULL){
                    g_widget_map[IDC_BITMAP_IP_ICON].label->clear();
                    info.label->setPixmap(QPixmap::fromImage(info.bitmap));
                }
            }
            else{
                info = g_widget_map[IDC_BITMAP_IP_ICON];
                if(info.label != NULL){
                    g_widget_map[IDC_BITMAP_IP_ONLINE_ICON].label->clear();
                    info.label->setPixmap(QPixmap::fromImage(info.bitmap));
                }
            }
        }

        json = cJSON_GetObjectItem(cj_update_info,"temp_type");
        if(NULL != json){
            if(json->valueint != 0){
                g_config.method = METHOD_TEMP;
                w->UpdateWidgetText(&g_widget_map[IDC_MSG3]);
                w->UpdateWidgetText(&g_widget_map[IDC_MSG2]);
            }
            else{
                g_config.method = METHOD_FACE;
            }
        }

        /*json = cJSON_GetObjectItem(cj_update_info,"temp_mode_switch");
        if(NULL != json){
            g_config.temp_mode_switch = json->valueint;
            for(unsigned int i = 0; i <  g_widget_temp_mode.size(); i++){
                 g_widget_temp_mode[i].need_update = TRUE;
            }
            PostMessage(g_main_hwnd, MSG_REFRESH_IMAGE, 0, 0);
        }*/

        /*cJSON* Array = cJSON_GetObjectItem(cj_update_info,"temp_rect");
        if(cJSON_IsArray(Array))
        {
            RECT rc_t = {0,0,0,0};
            int center_x = cJSON_GetArrayItem(Array,0)->valueint;
            int center_y = cJSON_GetArrayItem(Array,1)->valueint;
            int center_w = cJSON_GetArrayItem(Array,2)->valueint;
            int center_h = cJSON_GetArrayItem(Array,3)->valueint;

            rc_t.top = center_y;
            rc_t.bottom = center_y + center_h;
            rc_t.left = center_x;
            rc_t.right = center_x + center_w;

            g_config.last_refresh_tempbox_time = AwGetTickCount();

            g_update_info_tempbox.need_update = TRUE;
            g_update_info_tempbox.rc_t = rc_t;
            PostMessage(g_main_hwnd, MSG_REFRESH_FACEBOX, 0, 0);
        }*/

        cJSON *array0 = cJSON_GetObjectItem(cj_update_info, "found");
        if(array0!=NULL){
            int temp_duration = 0;
            int temp_display_type = 0;
            cJSON *json_found_item = cJSON_GetArrayItem(array0, 0);
            if(json_found_item != NULL){
                cJSON* json = NULL;
                json = cJSON_GetObjectItem(json_found_item,"info_msg");
                if(NULL != json){
                   strcpy(g_info_msg, GetString(json->valuestring));
                   g_config.last_update_name_time = time.toTime_t();
                }
                else{
                   strcpy(g_info_msg, "");
                }

                json = cJSON_GetObjectItem(json_found_item,"name");
                if(json != NULL){
                    strcpy(g_name, GetString(json->valuestring));
                    g_config.last_update_name_time = time.toTime_t();
                }
                else{
                    memset(g_name, 0, sizeof(g_name));
                }

                json = cJSON_GetObjectItem(json_found_item,"status");
                if(NULL != json){
                    unsigned int i;
                    if(json->valueint == STATE_CHECK_FAIL){
                        for(i = 0; i < g_widget_check_success.size(); i++){
                            g_widget_check_success[i].label->clear();
                            strcpy(g_widget_check_success[i].text, "");
                        }
                        for(i = 0; i < g_widget_check_failed.size(); i++){
                            w->UpdateWidgetText(&g_widget_check_failed[i]);
                        }
                        g_config.last_update_name_time = time.toTime_t();
                    }
                    else if(json->valueint == STATE_CHECK_SUCC){
                        for(i = 0; i < g_widget_check_failed.size(); i++){
                            g_widget_check_failed[i].label->clear();
                            strcpy(g_widget_check_failed[i].text, "");
                        }
                        for(i = 0; i < g_widget_check_success.size(); i++){
                            w->UpdateWidgetText(&g_widget_check_success[i]);
                        }
                        g_config.last_update_name_time = time.toTime_t();
                    }
                }

                json = cJSON_GetObjectItem(json_found_item,"name_duration");
                if(NULL != json){
                    g_config.name_dealy_clear_sec = json->valueint / 1000;
                }

                json = cJSON_GetObjectItem(json_found_item,"temp_display_type");
                if(NULL != json){
                    temp_display_type = json->valueint;
                }

                json = cJSON_GetObjectItem(json_found_item,"temp_duration");
                if(NULL != json){
                    temp_duration = json->valueint / 1000;
                }

                json = cJSON_GetObjectItem(json_found_item,"temp");
                if(NULL != json){
                    char temp[16] = {0};
                    double float_temp = json->valuedouble;
                    if(temp_display_type == 1)
                        sprintf(temp, "%s%0.1f%s", (char *)_GetString("TEMP_HEAD"), (float_temp * 1.8 + 32), (char *)_GetString("TEMP_SIGN_F"));
                    else
                        sprintf(temp, "%s%0.1f%s", (char *)_GetString("TEMP_HEAD"), float_temp, (char *)_GetString("TEMP_SIGN"));
                    qDebug() << "temp:" << temp;

                    cJSON *cj_temp_status = cJSON_GetObjectItem(json_found_item,"temp_status");
                    if(temp_duration > 0)
                        g_widget_map[IDC_MSG3].label->setText(temp);

                    if(cj_temp_status){
                        QPalette pe;
                        if(strcmp(cj_temp_status->valuestring, "normal") == 0)
                           pe.setColor(QPalette::WindowText, g_config.temp_low_color);
                        else if(strcmp(cj_temp_status->valuestring, "lowFever") == 0)
                            pe.setColor(QPalette::WindowText, g_config.temp_high_color);
                        else if(strcmp(cj_temp_status->valuestring, "highFever") == 0)
                            pe.setColor(QPalette::WindowText, g_config.temp_high_color);
                        g_widget_map[IDC_MSG3].label->setPalette(pe);
                    }
                    g_config.msg3_dealy_clear_sec = temp_duration;
                    g_config.last_update_msg3_time = time.toTime_t();
                }

                json = cJSON_GetObjectItem(json_found_item,"temp_msg");
                if(NULL != json){
                    strncpy(g_widget_map[IDC_MSG2].text, (char *)GetString(json->valuestring), sizeof(g_widget_map[IDC_MSG2].text));
                    g_widget_map[IDC_MSG2].label->setText(GetString(json->valuestring));
                    g_config.msg2_dealy_clear_sec = temp_duration;
                    g_config.last_update_msg2_time = time.toTime_t();
                }

                json = cJSON_GetObjectItem(json_found_item, "permisson_msg");
                if(NULL != json){
                    char str[128] = {0};
                    cJSON *count = cJSON_GetObjectItem(json_found_item, "count");
                    cJSON *duration = cJSON_GetObjectItem(json_found_item, "permisson_duration");
                    char *auth_string = (char *)GetString(json->valuestring);
                    if(duration->valueint == 0)
                        strcpy(str, "");
                    else if(-1 == count->valueint)
                        snprintf(str, 128, "%s", auth_string);
                    else
                        snprintf(str, 128, "%s: %d", auth_string, count->valueint);
                    strncpy(g_widget_map[IDC_MSG1].text, str, sizeof(g_widget_map[IDC_MSG1].text));
                    g_widget_map[IDC_MSG1].label->setText(str);
                    g_config.msg1_dealy_clear_sec = duration->valueint;
                    g_config.last_update_msg1_time = time.toTime_t();
                }
#if 1
                cJSON *Array = cJSON_GetObjectItem(json_found_item,"rect");
                if(NULL != Array){
                    if(g_config.facebox_show_switch == FACE_BOX_STATE_OPEN ||
                            (g_config.facebox_show_switch == FACE_BOX_STATE_AUTO && g_config.method != METHOD_TEMP)){
                        QRect rc_t = QRect(0,0,0,0);
                        int x, y, width, height;
                        switch(g_config.resolution)
                        {
                            case RESOLUTION_320_480:
                                x =(cJSON_GetArrayItem(Array,0)->valueint - 160) * 1.5 + 25;
                                y =cJSON_GetArrayItem(Array,1)->valueint *1.5 + 25;
                                width = cJSON_GetArrayItem(Array,2)->valueint  - 45;
                                height = cJSON_GetArrayItem(Array,3)->valueint - 40;
                                break;
                            case RESOLUTION_480_854:
                                /*x =cJSON_GetArrayItem(Array,0)->valueint - 30;
                                y =cJSON_GetArrayItem(Array,1)->valueint - 30;
                                width =cJSON_GetArrayItem(Array,2)->valueint + 30;
                                height =cJSON_GetArrayItem(Array,3)->valueint + 30;*/
                                x =cJSON_GetArrayItem(Array,0)->valueint;
                                y =cJSON_GetArrayItem(Array,1)->valueint;
                                width =cJSON_GetArrayItem(Array,2)->valueint;
                                height =cJSON_GetArrayItem(Array,3)->valueint;
                                break;
                            case RESOLUTION_600_1024:
                                x =cJSON_GetArrayItem(Array,0)->valueint + 30;
                                y =cJSON_GetArrayItem(Array,1)->valueint + 30;
                                width = cJSON_GetArrayItem(Array,2)->valueint + 30;
                                height =cJSON_GetArrayItem(Array,3)->valueint + 30;
                                break;
                            default :
                                break;
                        }

                        int center_w = width;
                        int center_h = height;
                        int center_x =  x + width/2;
                        int center_y = y + height/2;

                        rc_t.setX(x);
                        rc_t.setY(y);
                        rc_t.setWidth(width);
                        rc_t.setHeight(height);

                        if (center_w > center_h)
                        {
                              rc_t.setY(center_y - center_w/2);
                              rc_t.setHeight(center_w);
                        }else{
                              rc_t.setX(center_x - center_h/2);
                              rc_t.setWidth(center_h);
                        }
                        /*if(aw_tag_check(TAG_BIG_FD)){
                              rc_t.top = center_y - center_h;
                              rc_t.bottom = center_y + center_h;
                              rc_t.left = center_x - center_w;
                              rc_t.right = center_x + center_w;
                        }*/
                        g_config.last_refresh_facebox_time = time.toTime_t();
                        g_update_info_facebox.rc_t = rc_t;
                        //w->repaint(g_config.pre_face.x(), g_config.pre_face.y(), g_config.pre_face.width(), g_config.pre_face.height());
                    }
                }
#endif
            }
        }

        cJSON *show_msg_data = cJSON_GetObjectItem(json, "data");
        if(show_msg_data){
            cJSON *msg_info = cJSON_GetObjectItem(show_msg_data, "msg_info");
            cJSON *item = NULL;
            cJSON *msg = NULL;
            cJSON *duration = NULL;
            char str[128] = {0};
            int msg_array_size = cJSON_GetArraySize(msg_info);
            if(msg_array_size > 0){
               item = cJSON_GetArrayItem(msg_info, 0);
               msg = cJSON_GetObjectItem(item, "msg");
               duration = cJSON_GetObjectItem(item, "duration");
               if(duration->valueint == 0)
                  strcpy(str, "");
               else
                  strncpy(str, msg->valuestring, 128);
               strncpy(g_widget_map[IDC_MSG1].text, str, sizeof(g_widget_map[IDC_MSG1].text));
               g_widget_map[IDC_MSG1].label->setText(str);
               g_config.msg1_dealy_clear_sec = duration->valueint / 1000;
               g_config.last_update_msg1_time = time.toTime_t();
            }
            if(msg_array_size > 1){
               item = cJSON_GetArrayItem(msg_info, 1);
               msg = cJSON_GetObjectItem(item, "msg");
               duration = cJSON_GetObjectItem(item, "duration");
               if(duration->valueint == 0)
                  strcpy(str, "");
               else
                  strncpy(str, msg->valuestring, 128);
               strncpy(g_widget_map[IDC_MSG2].text, str, sizeof(g_widget_map[IDC_MSG2].text));
               g_widget_map[IDC_MSG2].label->setText(str);
               g_config.msg2_dealy_clear_sec = duration->valueint / 1000;
               g_config.last_update_msg2_time = time.toTime_t();
            }
        }
    }

    if(NULL != json){
        cJSON_Delete(json);
    }
}

void Widget::UpdataFaceBox()
{
    g_config.pre_face = g_config.cur_face;
    g_config.cur_face = g_update_info_facebox.rc_t;
    //this->repaint(g_config.pre_face.x(), g_config.pre_face.y(), g_config.pre_face.width(), g_config.pre_face.height());
    this->update();
}

void Widget::paintEvent(QPaintEvent *)
{
    //ceshi
    //qDebug() << "paintEvent";
    QPainter painter(this);
    QPen pen;

    QRect rc = g_config.cur_face;
    int w = rc.width();
    int h = rc.height();

    int RATIO = g_config.facebox_ratio;
    pen.setColor(g_config.facebox_color);
    pen.setWidth(g_config.facebox_line_width);
    //pen.setColor(Qt::red);
    painter.setPen(pen);

    painter.drawLine(rc.x(),rc.y(),rc.x()+w / RATIO, rc.y());
    painter.drawLine(rc.x(),rc.y(),rc.x(), rc.y()+w / RATIO);

    painter.drawLine(rc.right(),rc.y(),rc.right() - w / RATIO, rc.y());
    painter.drawLine(rc.right(),rc.y(),rc.right(), rc.y() + w / RATIO);

    painter.drawLine(rc.x(),rc.y() + rc.height(),rc.x(), rc.y() + rc.height() - h / RATIO);
    painter.drawLine(rc.x(),rc.y() + rc.height(),rc.x() + h / RATIO, rc.y() + rc.height());

    painter.drawLine(rc.right(),rc.y() + rc.height(),rc.right() - h / RATIO, rc.y() + rc.height());
    painter.drawLine(rc.right(),rc.y() + rc.height(),rc.right(), rc.y() + rc.height() - h / RATIO);
}
