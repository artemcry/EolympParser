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
#define tmp_files_path QDir::currentPath()+"/tmp"

QT_BEGIN_NAMESPACE
namespace Ui { class SolutionParser; }
QT_END_NAMESPACE
struct Solution;

class SolutionParser : public QWidget
{
    Q_OBJECT
public:
    class Solution
    {
    public:
        bool isCorrect;
        bool isCustom = false;
        QString name, ref;

        Solution(bool correct = false, const QString &name = "", const QString &ref = "")
            : isCorrect(correct), name(name), ref(ref) { }

        QString path() { return  tmp_files_path+"/"+Parser::adjustFileName(name); }
        QString lang() { return ref.mid(ref.lastIndexOf('.')+1);; }

    };
    struct CustomSolution: public Solution
    {
        CustomSolution(): Solution(true, "custom solution") { isCustom =  true; };
    };

    SolutionParser(QWidget *parent = nullptr);
    ~SolutionParser();
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
    void showCurrentSolutionAt(int index);
    bool selectSolution(QString index);
    Ui::SolutionParser *ui;
};
#endif
