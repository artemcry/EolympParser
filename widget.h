#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QFile>
#include <thread>
#include <QDesktopServices>
#include <QTextCodec>
#include <QPushButton>
#include <QClipboard>
#include <QMessageBox>
#include <QFileDialog>
#include "parser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
    struct Solution
    {
        QString name,path,ref,type;
        Solution(const QString &name,const QString &path,const QString &ref,const QString &type)
            :name(name),path(path),ref(ref),type(type) {};
    };
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    QMap<QString,QList<Solution*>> solutions;
    QJsonObject base;
    QString currentKey;
    bool downloading = false;
    Parser *p;
    void find();
    void downloadBase();
    void showSolution(const QString& ref);
    Ui::Widget *ui;
};
#endif // WIDGET_H
