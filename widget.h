#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void LoadGuiLayout();
    void PrintMsg(const char *title, const char *msg);
    void paintEvent(QPaintEvent *);
    /* void InitWindow();
    void UpdateTextInfo();
    void UpdateImageInfo();
    void UpdateFacebox();
    void PaintImageInfo();
    void PaintTextInfo();
    void PaintFaceBox();
    void PaintTempBox();
    void MainWindowProc(int message);*/

private slots:
    void UpdateTime();
    void UpdataFaceBox();
private:
    Ui::Widget *ui;
};

void _InitString();
void ProcessGuiData(void *data, int size, void *context);

#endif // WIDGET_H
