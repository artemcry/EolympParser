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
#include <QHBoxLayout>
#include <QComboBox>
#include "parser.h"
#include "savesolution.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE
struct Solution;

class Widget : public QWidget
{
    Q_OBJECT
public:
    struct Solution
    {
        QString name, path, ref, type;
        Solution(const QString &name, const QString &path, const QString &ref, const QString &type)
            :name(name), path(path), ref(ref), type(type) {};
        Solution() :name("custom solution") {};
        bool isCustom() { return name == "custom solution"; }

    };
    Widget(QWidget *parent = nullptr);
    ~Widget();
    const QStringList codeLangs = { "cpp", "c", "cs", "py", "rb", "pas", "java", "js", "txt"} ;
    QMap<QString,QList<Solution*>> solutions;
    QJsonObject base;
    QJsonObject customBase;
    QString currentKey;
    bool downloading = false;
    Parser *p;
    void find();
    void addCustomSolution();
    void downloadBase();
    void showSolution(int index);
    bool selectSolution(QString index);
    QString cppToPython(QString code);
    Ui::Widget *ui;
};
#endif