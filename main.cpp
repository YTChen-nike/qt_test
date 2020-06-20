#include "widget.h"
#include <QApplication>
#include "util/aw_ipc.h"
#include "util/aw_log.h"
//ceshi
#include <QDebug>

#define MSG_INITWINDOW   5
#define IPC_ADDRESS_GUI "tcp://127.0.0.1:13101"

int main(int argc, char *argv[])
{
    _InitString();
    QApplication a(argc, argv);
    Widget w;
    w.LoadGuiLayout();
    w.show();

    int ret = 0;
    aw_ipc_server *g_ipc_server_gui;
    g_ipc_server_gui = aw_ipc_server::Create(AW_IPC_TYPE_SOCKET);
    if(g_ipc_server_gui == NULL){
        aw_log_err("gui create ipc server failed");
        goto EXIT;
        //return 0;
    }
    ret = g_ipc_server_gui->Bind(IPC_ADDRESS_GUI);
    if(ret != 0){
        aw_log_err("gui ipc server bind failed");
        goto EXIT;
        //return 0;
    }
    aw_log_info("start recv");
    g_ipc_server_gui->SetRecvBufSize(1024,1024);
    g_ipc_server_gui->SetRecvCb(ProcessGuiData, &w);

EXIT:
    return a.exec();
}
