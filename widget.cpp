#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#define cmd qDebug()
#define tmp_files_path QDir::currentPath()+"/tmp"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setFixedSize(size());
    ui->solutionLine->setValidator(new QRegExpValidator(QRegExp("^[1-9]{1}\\d{,4}"), this));
    p= new Parser(this);

    QFile fbase(QDir::currentPath()+"/base.json"), fcustom(QDir::currentPath()+"/base_custom.json");
    if(fbase.open(QFile::ReadOnly))
        base = QJsonObject(QJsonDocument::fromJson(QString(fbase.readAll()).toUtf8()).object());
    else
        downloadBase();

    if(fcustom.open(QFile::ReadOnly))
        customBase = QJsonObject(QJsonDocument::fromJson(QString(fcustom.readAll()).toUtf8()).object());

    connect(ui->findButton,&QPushButton::clicked,this,&Widget::find);
    connect(ui->listWidget,&QListWidget::currentRowChanged,this,&Widget::showSolution);
    connect(ui->addCustomButton,&QPushButton::clicked,this,&Widget::addCustomSolution);

    connect(ui->removeButton, &QPushButton::clicked, this, [this]() {
        ui->textBox->setText(ui->textBox->toPlainText().replace(QRegExp("(//)[^\n]*\n"),"\n").remove(QRegExp("/\\*.*\\*/")));
    });
    connect(ui->copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->textBox->toPlainText());
    });
    connect(ui->copyUrl, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->urlBox->text());
    });
    connect(ui->copyPath, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->pathBox->text());
    });
    connect(ui->updateBaseButton, &QPushButton::clicked, this, &Widget::downloadBase);

    connect(p,&Parser::downloadFilesProgresChanged,this,[this](double val) {
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,QString::number(val)));
    });
    connect(ui->applyButton, &QPushButton::clicked, this, [this]() {
        QJsonArray arr = customBase[currentKey].toArray();
        int index = ui->listWidget->currentIndex().row() - ui->listWidget->count() + arr.size();
        if(index < 0)
            return;
        arr[index] = ui->textBox->toPlainText();
        customBase[currentKey] = arr;
    });

    connect(p,&Parser::downloadFilesFinished,this,[this](int, QStringList) {
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,""));
    });


    connect(ui->downloadButton, &QPushButton::clicked , this,[this]() {
        QString path = QFileDialog::getExistingDirectory(this, ("Select Output Folder"), QDir::currentPath()+"/Downloads");
        std::thread t(&Parser::downloadFiles, p, base, path, ui->threadBox->value());
        t.detach();
    });
}
Widget::~Widget()
{
    QFile fbase(QDir::currentPath()+"/base.json"), fcustom(QDir::currentPath()+"/base_custom.json");
    fbase.open(QFile::WriteOnly);
    fbase.write(QJsonDocument(base).toJson());
    fcustom.open(QFile::WriteOnly);
    fcustom.write(QJsonDocument(customBase).toJson());
    QDir(tmp_files_path).removeRecursively();
    delete ui;
}

bool Widget::selectSolution(QString index)
{
    ui->listWidget->clear();
    currentKey = index;

    if(!solutions.contains(index))
        return false;

    for(auto e : solutions.value(index))
    {
        QListWidgetItem *i = new QListWidgetItem();
        if(e->isCustom())
            i->setBackground(QColor(255,200,0,50));
        else
            i->setBackground(QColor(0,255,0,50));

        i->setText(e->name);
        ui->listWidget->addItem(i);
    }
    return true;
}

void Widget::find()
{    
    QString key = ui->solutionLine->text();
    if(key == currentKey)
        return;
    if(!base.contains(key))
    {
        QMessageBox msgBox;
        msgBox.setText("Base not contains this solution");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    else if (selectSolution(key))
        return;

    QJsonArray sols = base.value(key).toArray();
    int i = 0;
    for(auto e: sols) {

        QListWidgetItem *item = new QListWidgetItem();
        item->setBackground(QColor(255,0,0,50));
        item->setText("Loading...");
        ui->listWidget->addItem(item);
        solutions[key].append(new Solution());


        QString ref = e.toString();

        QNetworkAccessManager *manager = new QNetworkAccessManager();
        QNetworkRequest request(QUrl(ref.toUtf8()));
        manager->get(request);

        QObject::connect(manager, &QNetworkAccessManager::finished, manager, [=](QNetworkReply *reply) {
            if(reply->error() == QNetworkReply::NoError) {
                Solution *so = solutions[key].at(i);
                so->name = ref.mid(ref.lastIndexOf("/")+1);
                so->path = tmp_files_path+"/"+so->name;
                so->ref = ref;
                so->type = ref.mid(ref.lastIndexOf('.')+1);

                QByteArray data = reply->readAll();
                QDir().mkdir(tmp_files_path);
                QFile f(so->path);
                f.open(QFile::WriteOnly);
                f.write(data);
                f.close();

                if(currentKey == key) {
                    item->setText(so->name);
                    item->setBackground(QColor(0, 255, 0, 50));
                }
            }
            else
                item->setText("Error");
            manager->deleteLater();
        });
        i++;
    }

    for (int i = 0; i < customBase.value(key).toArray().size() ; i++) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setBackground(QColor(255,200,0,50));
        solutions[key].append(new Solution());
        item->setText(solutions[key].last()->name);

        ui->listWidget->addItem(item);        
    }
}

void Widget::addCustomSolution()
{
    SaveSolution *s = new SaveSolution(currentKey);
    s->exec();
    if(s->Key().isEmpty())
        return;
    Parser::appendJsonArray(customBase, s->Key(), QJsonArray() << ui->textBox->toPlainText());

    solutions[s->Key()].append(new Solution());
    if(currentKey == s->Key())
        selectSolution(s->Key());
}

void Widget::downloadBase()
{
    if(downloading)
        return;
    downloading = true;
    connect(p,&Parser::parseLinksProgresChanged,this,[this](double val) {
        ui->downloadProgress->setText("Parse links: "+QString::number(((double)((int)(val*10)))/10)+"%");
    });

    std::thread t([&]() {
        base = p->parseAllLinks();
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,""));
        downloading = false;
    });
    t.detach();
}

void Widget::showSolution(int index)
{
    if(index == -1)
        return;
    Solution* sol = solutions[currentKey].at(index);
    ui->urlBox->setText(sol->ref);
    ui->pathBox->setText(sol->path);
    ui->urlBox->setCursorPosition(0);
    ui->pathBox->setCursorPosition(0);

    if(sol->isCustom())
    {
        QJsonArray arr = customBase[currentKey].toArray();
        ui->textBox->setText(arr[index - ui->listWidget->count() + arr.size()].toString());
    }
    else if(codeLangs.contains(sol->type, Qt::CaseInsensitive))
    {
        QFile f(sol->path);
        f.open(QFile::ReadOnly);
        ui->textBox->setText(f.readAll());
    }
    else if (QMessageBox::question(this, "", "Open "+sol->name+" in another program?",
                                   QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(sol->path));

}
